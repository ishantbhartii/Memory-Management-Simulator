#include "../include/cache/cache.hpp"
#include <stdexcept>
#include <iostream>

using namespace std;

CacheSet::CacheSet(size_t associativity, Size line_size)
    : lines(associativity, CacheLine(line_size)) {}

Cache::Cache(Size size, Size line_size, size_t associativity, CacheReplacementPolicy policy)
    : size_(size),
      line_size_(line_size),
      associativity_(associativity),
      policy_(policy),
      hits_(0),
      misses_(0),
      accesses_(0) {

    if (size == 0 || line_size == 0 || associativity == 0) {
        throw invalid_argument("Cache parameters must be positive");
    }

    if (size % (line_size * associativity) != 0) {
        throw invalid_argument("Cache size must be divisible by line_size * associativity");
    }

    num_sets_ = size / (line_size * associativity);
    sets_.resize(num_sets_, CacheSet(associativity, line_size));
}

bool Cache::read(Address address, ProcessId process_id) {
    accesses_++;

    size_t set_index;
    Address tag;
    size_t line_offset;

    getAddressComponents(address, set_index, tag, line_offset);

    int line_index = findLineInSet(set_index, tag);
    if (line_index >= 0) {
        hits_++;
        updateAccessOrder(set_index, line_index);
        return true;
    }

    misses_++;
    handleMiss(set_index, tag, process_id, false);
    return false;
}

bool Cache::write(Address address, ProcessId process_id) {
    accesses_++;

    size_t set_index;
    Address tag;
    size_t line_offset;

    getAddressComponents(address, set_index, tag, line_offset);

    int line_index = findLineInSet(set_index, tag);
    if (line_index >= 0) {
        hits_++;
        sets_[set_index].lines[line_index].dirty = true;
        updateAccessOrder(set_index, line_index);
        return true;
    }

    misses_++;
    handleMiss(set_index, tag, process_id, true);
    return false;
}

void Cache::getAddressComponents(
    Address address,
    size_t& set_index,
    Address& tag,
    size_t& line_offset
) const {
    line_offset = address % line_size_;
    Address line_address = address / line_size_;
    set_index = line_address % num_sets_;
    tag = line_address / num_sets_;
}

int Cache::findLineInSet(size_t set_index, Address tag) const {
    const auto& lines = sets_[set_index].lines;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (lines[i].valid && lines[i].tag == tag) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

Cache::CacheStats Cache::getStats() const {
    CacheStats stats;
    stats.hits = hits_;
    stats.misses = misses_;
    stats.accesses = accesses_;
    stats.hit_rate = accesses_ ? static_cast<double>(hits_) / accesses_ : 0.0;
    stats.miss_rate = accesses_ ? static_cast<double>(misses_) / accesses_ : 0.0;
    return stats;
}

void Cache::resetStats() {
    hits_ = 0;
    misses_ = 0;
    accesses_ = 0;
}

void Cache::updateAccessOrder(size_t set_index, size_t line_index) {
}

size_t Cache::selectVictimLine(size_t set_index) {
    const auto& lines = sets_[set_index].lines;
    for (size_t i = 0; i < lines.size(); ++i) {
        if (!lines[i].valid) {
            return i;
        }
    }
    return 0;
}
