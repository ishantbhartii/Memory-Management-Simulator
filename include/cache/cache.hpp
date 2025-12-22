#ifndef CACHE_HPP
#define CACHE_HPP

#include <vector>
#include <list>
#include <unordered_map>
#include <memory>

#include "common/types.hpp"

using namespace std;

struct CacheLine {
    Address tag;
    bool valid;
    bool dirty;
    ProcessId process_id;
    vector<uint8_t> data;

    CacheLine(Size line_size = 64)
        : tag(0), valid(false), dirty(false), process_id(-1), data(line_size, 0) {}
};

struct CacheSet {
    vector<CacheLine> lines;
    list<size_t> access_order;

    CacheSet(size_t associativity, Size line_size);
};

class Cache {
protected:
    Size size_;
    Size line_size_;
    size_t associativity_;
    size_t num_sets_;
    CacheReplacementPolicy policy_;

    vector<CacheSet> sets_;

    size_t hits_;
    size_t misses_;
    size_t accesses_;

public:
    Cache(Size size, Size line_size, size_t associativity, CacheReplacementPolicy policy);
    virtual ~Cache() = default;

    virtual bool read(Address address, ProcessId process_id);
    virtual bool write(Address address, ProcessId process_id);

    void getAddressComponents(
        Address address,
        size_t& set_index,
        Address& tag,
        size_t& line_offset
    ) const;

    virtual int findLineInSet(size_t set_index, Address tag) const;

    virtual void handleMiss(
        size_t set_index,
        Address tag,
        ProcessId process_id,
        bool is_write
    ) = 0;

    struct CacheStats {
        size_t hits;
        size_t misses;
        size_t accesses;
        double hit_rate;
        double miss_rate;
    };

    CacheStats getStats() const;

    Size getSize() const { return size_; }
    Size getLineSize() const { return line_size_; }
    size_t getAssociativity() const { return associativity_; }
    size_t getNumSets() const { return num_sets_; }
    CacheReplacementPolicy getPolicy() const { return policy_; }

    void resetStats();

protected:
    virtual void updateAccessOrder(size_t set_index, size_t line_index) = 0;
    virtual size_t selectVictimLine(size_t set_index) = 0;
};

unique_ptr<Cache> createCache(
    Size size,
    Size line_size,
    size_t associativity,
    CacheReplacementPolicy policy
);

#endif
