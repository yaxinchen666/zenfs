#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <random>

#include "block_similarity.h"

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

void test_convert_hash() {
    HashPool p;
    std::bitset<ZENFS_SIM_HASH_SIZE> a(0b0111110);
    std::vector<uint8_t> x = p.hash_to_vec(a);
    std::cout << x.size() << "\n";
    std::cout << unsigned(x[x.size() - 1]) << "\n";

    a = p.vec_to_hash(x);
    std::cout << a;
}

void test_get_nearest_hash() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::bernoulli_distribution d(0.5);

    HashPool pool(20, 1);  // buf_size = 1 -> call createIndex on every insert
    for (int i = 0; i < 10; ++i) {
        std::bitset<ZENFS_SIM_HASH_SIZE> nearest_hash_code;
        std::bitset<ZENFS_SIM_HASH_SIZE> hash_code;
        bool has_neighbor;

        for (int b = 0; b < ZENFS_SIM_HASH_SIZE; ++b) {
            hash_code[b] = d(gen);
        }
        has_neighbor = pool.get_nearest_hash(hash_code, nearest_hash_code);
        if (has_neighbor) {
            std::cout << i << " a \n";
            std::cout << hash_code << "\n";
            std::cout << nearest_hash_code << "\n";
        }
        
        int rep = rand() % 20 + 1; // rand() % (k * threshold) + 1
        for (int i = 0; i < rep; ++i) {
            hash_code.flip(rand() % ZENFS_SIM_HASH_SIZE);
        }
        has_neighbor = pool.get_nearest_hash(hash_code, nearest_hash_code);
        if (has_neighbor) {
            std::cout << i << " b \n";
            std::cout << hash_code << "\n";
            std::cout << nearest_hash_code << "\n";
        }
    }
}

void test_get_similar_block() {
    char data_file_name[] = "/home/yaxin.chen/data/stackoverflow/cs.stackexchange.com/PostHistory.xml";
    FILE* f = fopen(data_file_name, "rb");
    char blocks[ZENFS_SIM_BLOCK_SIZE * 256];
    int num_blocks = fread(blocks, ZENFS_SIM_BLOCK_SIZE, 256, f);
    fclose(f);
    std::cout << "num_blocks read: " << num_blocks << "\n";

    char module_file_name[] = "/home/yaxin.chen/repos/rocksdb/plugin/zenfs/fs/similarity/model/hash_network.pt";
    HashNetwork model(module_file_name);
    std::vector<std::bitset<ZENFS_SIM_HASH_SIZE>> hash_code = model.genHash(blocks, num_blocks);

    int count = 0;
    std::bitset<ZENFS_SIM_HASH_SIZE> nearest_hash_code;
    HashPool pool(20, 4);
    for (int i = 0; i < num_blocks; ++i) {
        bool has_neighbor = pool.get_nearest_hash(hash_code[i], nearest_hash_code);
        if (has_neighbor) {
            ++count;
        }
    }
    std::cout << "num neighbors: " << count << "\n"; // 235 for threshold 25; 216 for threshold 20
}

int main() {
    test_get_similar_block();
}