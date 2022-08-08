# Machine Learning Enhanced Delta Compression of Similar Blocks in ZenFS

## Repo Structure

`test_ml/`: contains test program for testing compression.

`fs/similarity/`: contains implementation of two classes: `HashNetwork` for generating hash code for logical blocks; `HashPool` for finding similar hash code.

### Main Modification of ZenFS

`fs/fs_zenfs`: add a method `NewWritableFileWithCompression` to `ZenFS`, which creates a write file pointer and compression will be used when writing through this pointer.

`fs/io_zenfs`: add a method `Compress` to `ZonedWritableFile`, which is called by `FlushBuffer` and compresses data in the buffer.

`zenfs.mk`: add two library flag in order to use xdelta and similar block detection.

## Running Instruction

### Zoned Block Device Emulation

To emulate zoned block device on SSD without ZNS, `null_blk` is used. There is a configuration script `nullblk-zoned.sh` in https://zonedstorage.io/docs/getting-started/nullblk. The parameters used in this project is

```shell
./nullblk-zoned.sh 4096 256 0 32
```

The zoned block device can then be found at `/dev/nullb0`.
```shell
blkzone report /dev/nullb0
```

To remove `nullb0`:
```shell
echo 0 > /sys/kernel/config/nullb/nullb0/power
rmdir /sys/kernel/config/nullb/nullb0
rmmod null_blk
```

### External Package Installation

To avoid compilation error when using external packages in ZenFS, they need to be compiled and installed as library.

#### Xdelta

[Xdelta](https://github.com/jmacd/xdelta) is used as the delta compression algorithm. The delta compression library in this project is built with DeepSketch's code https://github.com/dgist-datalab/deepsketch-fast2022/tree/main/xdelta3, which sets parameters and contains a function `xdelta3_compress` that delta compress blocks. To compile it into a library to be used in ZenFS, the following modifications are applied:

1. Change `xdelta3.c` to `xdelta3.cc`; change all `#include "xdelta3.c"` to  `#include "xdelta3.cc"` in `xdelta3-cfgs.h` and `xdelta3.cc`
2. Create `zenfs_xdelta3.cc/h`; move the function `xdelta3_compress` from `xdelta3.cc` to `zenfs_xdelta3.cc`.
3. Use CMakeLists.txt to compile them into library. Library source: `xdelta3.cc, zenfs_xdelta3.cc`; public header: `zenfs_xdelta3.h`.

The modified files as well as the dynamic library file can be found in `test_ml/similarity_lib/deepsketch_xdelta3`. To install the library, you can either

1. Copy `libxdelta.so` to `/usr/local/lib`
2. Download https://github.com/dgist-datalab/deepsketch-fast2022/tree/main/xdelta3; copy files in `deepsketch_xdelta3` to the downloaded directory; use CMakeLists.txt to compile & install.

(Optional) To make sure the library will be loaded, run
```shell
export LD_LIBRARY_PATH=/usr/local/lib:$LD_LIBRARY_PATH
sudo ldconfig
```

#### NGT

NGT is a library implementing approximate nearest neighbor search. Follow the instruction in https://github.com/yahoojapan/NGT/wiki/Cpp-Quick-Start to install it.

### Installation of Machine Learning & Approximate Nearest Neighbor Search Part

The function of using Pytorch model to generate hash code and the function of searching nearest hash code are encapsulated into two classes and compiled into a library in order to be used in ZenFS. The code is in `fs/similarity`.

To run Pytorch in C++, libtorch needs to be downloaded (https://pytorch.org/cppdocs/installing.html). Then copy the unzipped libtorch directory to `/usr/local/lib`.

Compile and install these two classes:

```shell
cd fs/similarity
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH=/usr/local/lib/libtorch ..
make
make install
```

### ZenFS

To build and use ZenFS, follow the instruction in https://github.com/westerndigitalcorporation/zenfs/blob/master/README.md except changing

```shell
DEBUG_LEVEL=0 ROCKSDB_PLUGINS=zenfs make -j48 db_bench install
```

to

```shell
sudo DEBUG_LEVEL=0 ROCKSDB_PLUGINS=zenfs USE_RTTI=1 make -j48 db_bench install
```

This is because the compilation of rocksdb uses `-fno-rtti` flag by default, but NGT uses `typeid` which needs rtti, so we set `USE_RTTI=1` to remove this flag.


### Test Program

Test program is written in `test_ml/`. In the `main` function of `zenfs_test.cc`, modify `module_file_name` to the path to the machine learning model, and `input_file_name` to a large data file for reading input blocks. Then compile and run with

```shell
cd test_ml
make
./zenfs_test
```

After running the test program, we can check the actual file size written to ZenFS by

```shell
cd util
./zenfs list --zbd=nullb0
```

## References

1. https://github.com/dgist-datalab/deepsketch-fast2022/


