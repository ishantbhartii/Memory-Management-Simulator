#include "../include/cache/cache.hpp"
#include <algorithm>
#include <stdexcept>

using namespace std;

class FIFOCache : public Cache {
public:
    FIFOCache(Size size, Size line_size, size_t associativity)
        : Cache(size, line_size, associativity, CacheReplacementPolicy::FIFO) {}

    void handleMiss(size_t set_index, Address tag, ProcessId process_id, bool is_write) override {
        size_t victim_index = selectVictimLine(set_index);
        auto& victim_line = sets_[set_index].lines[victim_index];

        victim_line.tag = tag;
        victim_line.valid = true;
        victim_line.dirty = is_write;
        victim_line.process_id = process_id;
    }

protected:
    void updateAccessOrder(size_t set_index, size_t line_index) override {
    }

    size_t selectVictimLine(size_t set_index) override {
        static vector<size_t> fifo_counters(num_sets_, 0);

        const auto& lines = sets_[set_index].lines;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (!lines[i].valid) {
                return i;
            }
        }

        size_t victim = fifo_counters[set_index];
        fifo_counters[set_index] = (fifo_counters[set_index] + 1) % associativity_;
        return victim;
    }
};

class LRUCache : public Cache {
public:
    LRUCache(Size size, Size line_size, size_t associativity)
        : Cache(size, line_size, associativity, CacheReplacementPolicy::LRU) {}

    void handleMiss(size_t set_index, Address tag, ProcessId process_id, bool is_write) override {
        size_t victim_index = selectVictimLine(set_index);
        auto& victim_line = sets_[set_index].lines[victim_index];

        auto& access_order = sets_[set_index].access_order;
        access_order.remove(victim_index);

        victim_line.tag = tag;
        victim_line.valid = true;
        victim_line.dirty = is_write;
        victim_line.process_id = process_id;

        access_order.push_front(victim_index);
    }

protected:
    void updateAccessOrder(size_t set_index, size_t line_index) override {
        auto& access_order = sets_[set_index].access_order;
        access_order.remove(line_index);
        access_order.push_front(line_index);
    }

    size_t selectVictimLine(size_t set_index) override {
        const auto& lines = sets_[set_index].lines;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (!lines[i].valid) {
                return i;
            }
        }

        return sets_[set_index].access_order.back();
    }
};

class LFUCache : public Cache {
private:
    vector<vector<size_t>> access_counts_;

public:
    LFUCache(Size size, Size line_size, size_t associativity)
        : Cache(size, line_size, associativity, CacheReplacementPolicy::LFU),
          access_counts_(num_sets_, vector<size_t>(associativity, 0)) {}

    void handleMiss(size_t set_index, Address tag, ProcessId process_id, bool is_write) override {
        size_t victim_index = selectVictimLine(set_index);
        auto& victim_line = sets_[set_index].lines[victim_index];

        access_counts_[set_index][victim_index] = 1;

        victim_line.tag = tag;
        victim_line.valid = true;
        victim_line.dirty = is_write;
        victim_line.process_id = process_id;
    }

protected:
    void updateAccessOrder(size_t set_index, size_t line_index) override {
        access_counts_[set_index][line_index]++;
    }

    size_t selectVictimLine(size_t set_index) override {
        const auto& lines = sets_[set_index].lines;
        for (size_t i = 0; i < lines.size(); ++i) {
            if (!lines[i].valid) {
                return i;
            }
        }

        size_t min_count = access_counts_[set_index][0];
        size_t victim = 0;

        for (size_t i = 1; i < lines.size(); ++i) {
            if (access_counts_[set_index][i] < min_count) {
                min_count = access_counts_[set_index][i];
                victim = i;
            }
        }

        return victim;
    }
};

unique_ptr<Cache> createCache(Size size, Size line_size, size_t associativity, CacheReplacementPolicy policy) {
    switch (policy) {
        case CacheReplacementPolicy::FIFO:
            return make_unique<FIFOCache>(size, line_size, associativity);
        case CacheReplacementPolicy::LRU:
            return make_unique<LRUCache>(size, line_size, associativity);
        case CacheReplacementPolicy::LFU:
            return make_unique<LFUCache>(size, line_size, associativity);
        default:
            throw invalid_argument("Unsupported cache replacement policy");
    }
}
