// Reference: https://github.com/dgist-datalab/deepsketch-fast2022/blob/main/deepsketch/

#include <torch/script.h>
#include <bitset>
#include <vector>

#ifndef HASH_NETWORK_H
#define HASH_NETWORK_H

const int ZENFS_SIM_BLOCK_SIZE = 4096;  // TODO
const int ZENFS_SIM_HASH_SIZE = 128;

class HashNetwork {
private:
    torch::jit::script::Module module;
public:
    HashNetwork(char *module_file_name);
    ~HashNetwork();

    /*
    length of blocks = BLOCK_SIZE * num_blocks
    Generate hash code for blocks.
    */
    std::vector<std::bitset<ZENFS_SIM_HASH_SIZE>> genHash(char *blocks, int num_blocks);
};

#endif // HASH_NETWORK_H