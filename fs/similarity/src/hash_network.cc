#include <torch/script.h>
#include <bitset>
#include <vector>

#include "block_similarity.h"

struct HashNetwork::member {
    torch::jit::script::Module module;
};

HashNetwork::HashNetwork(char *module_file_name) {
    this->member_ptr->module = torch::jit::load(module_file_name);
    this->member_ptr->module.eval();
}

HashNetwork::~HashNetwork() {}

std::vector<std::bitset<ZENFS_SIM_HASH_SIZE>> HashNetwork::genHash(char *blocks, int num_blocks) {
    float *data = new float[num_blocks * ZENFS_SIM_BLOCK_SIZE];
    for (int i = 0; i < num_blocks * ZENFS_SIM_BLOCK_SIZE; ++i) {
        data[i] = ((int)(blocks[i])) / 128.0;
    }

    std::vector <torch::jit::IValue> inputs;
    inputs.push_back(torch::from_blob(data, {num_blocks, 1, ZENFS_SIM_BLOCK_SIZE}));
    torch::Tensor outputs = this->member_ptr->module.forward(inputs).toTensor();

    std::vector<std::bitset<ZENFS_SIM_HASH_SIZE>> hash_code(num_blocks);
    for (int i = 0; i < num_blocks; ++i) {
        for (int j = 0; j < ZENFS_SIM_HASH_SIZE; ++j) {
            hash_code[i].set(j, outputs[i][j].item<bool>());
        }
    }

    delete[] data;

    return hash_code;
}