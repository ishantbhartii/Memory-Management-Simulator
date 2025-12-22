#include "../include/cache/cache_hierarchy.hpp"
#include <stdexcept>

using namespace std;

CacheHierarchy::CacheHierarchy(
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
)
    : total_accesses_(0),
      l1_hits_(0),
      l2_hits_(0),
      l3_hits_(0),
      main_memory_accesses_(0) {

    l1_cache_ = createCache(l1_size, line_size, l1_associativity, l1_policy);
    l2_cache_ = createCache(l2_size, line_size, l2_associativity, l2_policy);
    l3_cache_ = createCache(l3_size, line_size, l3_associativity, l3_policy);
}

bool CacheHierarchy::read(Address address, ProcessId process_id) {
    total_accesses_++;

    if (l1_cache_->read(address, process_id)) {
        l1_hits_++;
        return true;
    }

    if (l2_cache_->read(address, process_id)) {
        l2_hits_++;
        l1_cache_->read(address, process_id);
        return true;
    }

    if (l3_cache_->read(address, process_id)) {
        l3_hits_++;
        l2_cache_->read(address, process_id);
        l1_cache_->read(address, process_id);
        return true;
    }

    main_memory_accesses_++;
    l3_cache_->read(address, process_id);
    l2_cache_->read(address, process_id);
    l1_cache_->read(address, process_id);

    return false;
}

bool CacheHierarchy::write(Address address, ProcessId process_id) {
    total_accesses_++;

    bool l1_hit = l1_cache_->write(address, process_id);

    if (l1_hit) {
        l1_hits_++;
        l2_cache_->write(address, process_id);
        l3_cache_->write(address, process_id);
        main_memory_accesses_++;
    } else {
        bool l2_hit = l2_cache_->write(address, process_id);
        if (l2_hit) {
            l2_hits_++;
            l1_cache_->write(address, process_id);
            l3_cache_->write(address, process_id);
            main_memory_accesses_++;
        } else {
            bool l3_hit = l3_cache_->write(address, process_id);
            if (l3_hit) {
                l3_hits_++;
                l2_cache_->write(address, process_id);
                l1_cache_->write(address, process_id);
                main_memory_accesses_++;
            } else {
                main_memory_accesses_++;
                l3_cache_->write(address, process_id);
                l2_cache_->write(address, process_id);
                l1_cache_->write(address, process_id);
            }
        }
    }

    return l1_hit;
}

CacheHierarchy::HierarchyStats CacheHierarchy::getStats() const {
    HierarchyStats stats;
    stats.l1_stats = l1_cache_->getStats();
    stats.l2_stats = l2_cache_->getStats();
    stats.l3_stats = l3_cache_->getStats();
    stats.total_accesses = total_accesses_;
    stats.main_memory_accesses = main_memory_accesses_;
    stats.avg_memory_access_time = calculateAccessTime();
    return stats;
}

void CacheHierarchy::resetStats() {
    l1_cache_->resetStats();
    l2_cache_->resetStats();
    l3_cache_->resetStats();
    total_accesses_ = 0;
    l1_hits_ = 0;
    l2_hits_ = 0;
    l3_hits_ = 0;
    main_memory_accesses_ = 0;
}

double CacheHierarchy::calculateAccessTime() const {
    if (total_accesses_ == 0) return 0.0;

    double total_time = 0.0;

    total_time += l1_hits_ * 1.0;

    size_t l2_only_hits = l2_hits_ - l1_hits_;
    total_time += l2_only_hits * 10.0;

    size_t l3_only_hits = l3_hits_ - l2_hits_;
    total_time += l3_only_hits * 50.0;

    total_time += main_memory_accesses_ * 200.0;

    return total_time / total_accesses_;
}
