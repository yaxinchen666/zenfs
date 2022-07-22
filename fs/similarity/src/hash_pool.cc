#include <vector>

#include "block_similarity.h"

HashPool::HashPool(int distance_threshold, int buf_size, int num_threads, std::string index_path) :
    distance_threshold(distance_threshold), buf_count(0), buf_size(buf_size), num_threads(num_threads) {
    property = new NGT::Property();
    property->dimension = ZENFS_SIM_HASH_SIZE / 8;
    property->objectType	= NGT::ObjectSpace::ObjectType::Uint8;
    property->distanceType = NGT::Index::Property::DistanceType::DistanceTypeHamming;

    NGT::Index::create(index_path, *property);
    index = new NGT::Index(index_path);
}

HashPool::~HashPool() {
    delete property;
    delete index;
}

std::vector<uint8_t> HashPool::hash_to_vec(const std::bitset<ZENFS_SIM_HASH_SIZE> &hash_code) {
    std::vector<uint8_t> hash_vec(property->dimension);
    std::bitset<ZENFS_SIM_HASH_SIZE> mask{0xff};
    for (int i = 0; i < property->dimension; ++i) {
        hash_vec[i] = ((hash_code >> (8 * (property->dimension - 1 - i))) & mask).to_ulong();
    }
    return hash_vec;
}

std::bitset<ZENFS_SIM_HASH_SIZE> HashPool::vec_to_hash(std::vector<uint8_t> hash_vec) {
    std::bitset<ZENFS_SIM_HASH_SIZE> hash_code;
    for (int i = 0; i < property->dimension; ++i) {
        std::bitset<ZENFS_SIM_HASH_SIZE> pattern(hash_vec[i]);
        hash_code |= (pattern << (8 * (property->dimension - 1 - i)));
    }
    return hash_code;
}

std::bitset<ZENFS_SIM_HASH_SIZE> HashPool::to_hash(uint8_t *hash_arr) {
    std::bitset<ZENFS_SIM_HASH_SIZE> hash_code;
    for (int i = 0; i < property->dimension; ++i) {
        std::bitset<ZENFS_SIM_HASH_SIZE> pattern(hash_arr[i]);
        hash_code |= (pattern << (8 * (property->dimension - 1 - i)));
    }
    return hash_code;
}

void HashPool::insert(std::vector<uint8_t> &hash_vec, bool create_index) {
    index->append(hash_vec);
    if (create_index) {
        index->createIndex(num_threads);
        // index->save();
    }
}

uint8_t *HashPool::query(std::vector<uint8_t> &hash_vec) {
    NGT::SearchQuery sc(hash_vec);
    NGT::ObjectDistances objects;
    sc.setResults(&objects);
    sc.setSize(1);
    sc.setEpsilon(0.2);
    
    index->search(sc);

    if (objects.size() > 0 && objects[0].distance < this->distance_threshold) {
	    NGT::ObjectSpace &objectSpace = index->getObjectSpace();
        uint8_t *object = static_cast<uint8_t*>(objectSpace.getObject(objects[0].id));
        return object;
    }

    return nullptr;
}

// TODO frequency of create index
bool HashPool::get_nearest_hash(const std::bitset<ZENFS_SIM_HASH_SIZE> &hash_code, std::bitset<ZENFS_SIM_HASH_SIZE> &nearest_hash_code) {
    std::vector<uint8_t> hash_vec = this->hash_to_vec(hash_code);
    uint8_t *query_result = this->query(hash_vec);
    if (query_result) {
        nearest_hash_code = this->to_hash(query_result);
        return true;
    } else {
        this->insert(hash_vec, (this->buf_count == this->buf_size - 1));
        this->buf_count = (this->buf_count + 1) % this->buf_size;
        return false;
    }
}