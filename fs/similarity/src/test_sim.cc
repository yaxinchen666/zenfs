#include <iostream>
#include <stdlib.h>
#include <time.h>

#include "hash_network.h"

void test_hash_network() {
    char module_file_name[] = "/home/yaxin.chen/repos/rocksdb/plugin/zenfs/fs/similarity/model/hash_network.pt";
    HashNetwork model(module_file_name);
    srand(time(NULL));
    int num_blocks = 2;
    char block[4096 * num_blocks];
    for (int i = 0; i < 4096 * num_blocks; ++i) {
        block[i] = rand() % 256;
    }
    std::vector<std::bitset<ZENFS_SIM_HASH_SIZE>> hash_code = model.genHash(block, num_blocks);
    for (int i = 0; i < num_blocks; ++i) {
        std::cout << hash_code[i] << "\n";
    }
}


int main() {
    test_hash_network();
}