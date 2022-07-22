#include <torch/script.h>
#include <bitset>
#include <vector>
#include <string>

#include "NGT/Index.h"

#ifndef BLOCK_SIMILARITY_H
#define BLOCK_SIMILARITY_H

const int ZENFS_SIM_BLOCK_SIZE = 4096;
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

class HashPool {
private:
    NGT::Property *property;
    NGT::Index *index;
    int buf_count;
    int buf_size;  // specifies the frequency of createIndex
    int num_threads;
    int distance_threshold;

public:
    HashPool(int distance_threshold=20, int buf_size=256,
             int num_threads=4, std::string index_path="ngt_index");
    ~HashPool();

    // convert bitset to vector of uint8_t
    std::vector<uint8_t> hash_to_vec(const std::bitset<ZENFS_SIM_HASH_SIZE> &hash_code);
    std::bitset<ZENFS_SIM_HASH_SIZE> vec_to_hash(std::vector<uint8_t> hash_vec);
    // hash_arr should have size of property->dimension
    std::bitset<ZENFS_SIM_HASH_SIZE> to_hash(uint8_t *hash_arr);

    void insert(std::vector<uint8_t> &hash_vec, bool create_index);
    uint8_t *query(std::vector<uint8_t> &hash_vec);
    
    /*
    first query.
    If there is no nearest hash code, insert current hash code & return false;
    otherwise assign the nearest hash code, return true.
    */
    bool get_nearest_hash(const std::bitset<ZENFS_SIM_HASH_SIZE> &hash_code, std::bitset<ZENFS_SIM_HASH_SIZE> &nearest_hash_code);
};

#endif // BLOCK_SIMILARITY_H