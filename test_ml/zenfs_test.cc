#include <dirent.h>
#include <fcntl.h>
#include <gflags/gflags.h>
#include <rocksdb/file_system.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <streambuf>

#include <stdlib.h>

#ifdef WITH_TERARKDB
#include <fs/fs_zenfs.h>
#include <fs/version.h>
#else
#include <rocksdb/plugin/zenfs/fs/fs_zenfs.h>
#include <rocksdb/plugin/zenfs/fs/version.h>
#endif

namespace ROCKSDB_NAMESPACE {

// must set readonly as false when trying to write to zbd!
std::unique_ptr<ZonedBlockDevice> zbd_open(std::string bdevname, bool readonly, bool exclusive) {
  std::unique_ptr<ZonedBlockDevice> zbd{
      new ZonedBlockDevice(bdevname, nullptr)};
  IOStatus open_status = zbd->Open(readonly, exclusive);

  if (!open_status.ok()) {
    fprintf(stderr, "Failed to open zoned block device: %s, error: %s\n",
            bdevname.c_str(), open_status.ToString().c_str());
    zbd.reset();
  }

  return zbd;
}

Status zenfs_mount(std::unique_ptr<ZonedBlockDevice> &zbd,
                   std::unique_ptr<ZenFS> *zenFS, bool readonly) {
  Status s;

  std::unique_ptr<ZenFS> localZenFS{
      new ZenFS(zbd.release(), FileSystem::Default(), nullptr)};
  s = localZenFS->Mount(readonly);
  if (!s.ok()) {
    localZenFS.reset();
  }
  *zenFS = std::move(localZenFS);

  return s;
}

// size of data must be smaller than buffer size
// touch new file & write data to it
void zenfs_write_file(std::string bdevname, std::string fname, std::string data){
  Status s;
  std::unique_ptr<ZonedBlockDevice> zbd = zbd_open(bdevname, false, true);
  if (!zbd) {
    fprintf(stderr, "Failed to open zoned block device, error: %s\n",
            s.ToString().c_str());
    return;
  }
  ZonedBlockDevice *zbdRaw = zbd.get();

  std::unique_ptr<ZenFS> zenFS;
  s = zenfs_mount(zbd, &zenFS, false);
  if (!s.ok()) {
    fprintf(stderr, "Failed to mount filesystem, error: %s\n",
            s.ToString().c_str());
    return;
  }

  std::unique_ptr<FSWritableFile> w_f;
  FileOptions fopts;
  IOOptions iopts;
  IODebugContext dbg;
  // fopts.use_direct_writes = false -> ZonedWritableFile.buffered = true
  s = zenFS->NewWritableFile(fname, fopts, &w_f, &dbg); // zenFS.get()->NewWritableFile(fname, fopts, &w_f, &dbg);
  if (!s.ok()) {
    fprintf(stderr, "Failed to create writable file, error: %s\n",
            s.ToString().c_str());
    return;
  }
  s = w_f->Append(Slice(data), iopts, &dbg);
  if (!s.ok()) {
    fprintf(stderr, "Failed to write data, error: %s\n",
            s.ToString().c_str());
    return;
  }
  w_f->Fsync(iopts, &dbg);
}

// read num_blocks * 4096 bytes from in_fname (in default fs), write to zenfs_out_fname (in zenfs)
void zenfs_write_blocks(std::string bdevname, std::string zenfs_out_fname, std::string in_fname, int num_blocks) {
  Status s;
  std::unique_ptr<ZonedBlockDevice> zbd = zbd_open(bdevname, false, true);
  if (!zbd) {
    fprintf(stderr, "Failed to open zoned block device, error: %s\n",
            s.ToString().c_str());
    return;
  }
  ZonedBlockDevice *zbdRaw = zbd.get();

  std::unique_ptr<ZenFS> zenFS;
  s = zenfs_mount(zbd, &zenFS, false);
  if (!s.ok()) {
    fprintf(stderr, "Failed to mount filesystem, error: %s\n",
            s.ToString().c_str());
    return;
  }

  std::unique_ptr<FSWritableFile> w_f;
  FileOptions fopts;
  IOOptions iopts;
  IODebugContext dbg;
  char module_file_name[] = "/home/yaxin.chen/repos/rocksdb/plugin/zenfs/fs/similarity/model/hash_network.pt";
  s = zenFS->NewWritableFileWithCompression(zenfs_out_fname, fopts, &w_f, &dbg,
          module_file_name);
  if (!s.ok()) {
    fprintf(stderr, "Failed to create writable file, error: %s\n",
            s.ToString().c_str());
    return;
  }
  
  std::cout << "num blocks: " << num_blocks << "\n";
  char block[4096];
  int BLOCK_SIZE = 4096;
  FILE* in_f = fopen(in_fname.c_str(), "rb");
  for (int i = 0; i < num_blocks; ++i) {
    int read_size = fread(block, 1, BLOCK_SIZE, in_f);
    if (read_size < BLOCK_SIZE) {
      std::cout << "current num blocks read: " << i << "\n";
      break;
    }

    s = w_f->Append(Slice(block), iopts, &dbg);
    if (!s.ok()) {
      fprintf(stderr, "Failed to write data, error: %s\n",
              s.ToString().c_str());
      std::cout << "current num blocks write: " << i << "\n";
      break;
    }
  }
  fclose(in_f);
  w_f->Fsync(iopts, &dbg);
}

// read length bytes from file with `fname`
void zenfs_read_file(std::string bdevname, std::string fname, size_t length) {
  Status s;
  std::unique_ptr<ZonedBlockDevice> zbd = zbd_open(bdevname, true, false);
  if (!zbd) {
    fprintf(stderr, "Failed to open zoned block device, error: %s\n",
            s.ToString().c_str());
    return;
  }
  ZonedBlockDevice *zbdRaw = zbd.get();

  std::unique_ptr<ZenFS> zenFS;
  s = zenfs_mount(zbd, &zenFS, true);
  if (!s.ok()) {
    fprintf(stderr, "Failed to mount filesystem, error: %s\n",
            s.ToString().c_str());
    return;
  }

  size_t buffer_sz = 1024 * 1024;
  std::unique_ptr<char[]> buffer{new (std::nothrow) char[buffer_sz]};
  if (!buffer) {
    fprintf(stderr, "Failed to allocate copy buffer");
    return;
  }

  std::unique_ptr<FSSequentialFile> r_f;
  FileOptions fopts;
  IOOptions iopts;
  IODebugContext dbg;
  s = zenFS->NewSequentialFile(fname, fopts, &r_f, &dbg);
  if (!s.ok()) {
    fprintf(stderr, "Failed to create sequential file, error: %s\n",
            s.ToString().c_str());
    return;
  }
  Slice slice;
  s = r_f->Read(length, iopts, &slice, buffer.get(), &dbg);
  if (!s.ok()) {
    fprintf(stderr, "Failed to read, error: %s\n",
            s.ToString().c_str());
    return;
  }

  std::cout << slice.ToString();
}

} // namespace ROCKSDB_NAMESPACE

int main(int argc, char **argv) {
  // ROCKSDB_NAMESPACE::zenfs_read_file("nullb0", "f_test", 256);
  // ROCKSDB_NAMESPACE::zenfs_write_file("nullb0", "f_test", "aa");
  ROCKSDB_NAMESPACE::zenfs_write_blocks("nullb0",
      "f_test",
      "/home/yaxin.chen/data/stackoverflow/cs.stackexchange.com/PostHistory.xml",
      8192);
}