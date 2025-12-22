#ifndef CACHE_HIERARCHY_HPP
#define CACHE_HIERARCHY_HPP

#include <memory>
#include <vector>

#include "cache/cache.hpp"

using namespace std;

class CacheHierarchy {
private:
    unique_ptr<Cache> l1_cache_;
    unique_ptr<Cache> l2_cache_;
    unique_ptr<Cache> l3_cache_;

    size_t total_accesses_;
    size_t l1_hits_;
    size_t l2_hits_;
    size_t l3_hits_;
    size_t main_memory_accesses_;

public:
    CacheHierarchy(
        Size l1_size,
        Size l2_size,
        Size l3_size,
        Size line_size,
        size_t l1_associativity,
        size_t l2_associativity,
        size_t l3_associativity,
        CacheReplacementPolicy l1_policy,
        CacheReplacementPolicy l2_policy,
        CacheReplacementPolicy l3_policy
    );

    bool read(Address address, ProcessId process_id);
    bool write(Address address, ProcessId process_id);

    struct HierarchyStats {
        Cache::CacheStats l1_stats;
        Cache::CacheStats l2_stats;
        Cache::CacheStats l3_stats;
        size_t total_accesses;
        size_t main_memory_accesses;
        double avg_memory_access_time;
    };

    HierarchyStats getStats() const;

    void resetStats();

    const Cache& getL1Cache() const { return *l1_cache_; }
    const Cache& getL2Cache() const { return *l2_cache_; }
    const Cache& getL3Cache() const { return *l3_cache_; }

private:
    void handleCacheMiss(
        CacheLevel level,
        Address address,
        ProcessId process_id,
        bool is_write
    );

    double calculateAccessTime() const;
};

#endif
