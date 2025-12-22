#ifndef INTEGRATED_SYSTEM_HPP
#define INTEGRATED_SYSTEM_HPP

#include <memory>
#include <unordered_map>
#include <vector>
#include <string>

#include "allocator/base_allocator.hpp"
#include "buddy/buddy_allocator.hpp"
#include "cache/cache_hierarchy.hpp"
#include "virtual_memory/vmm.hpp"
#include "common/types.hpp"

using namespace std;




class IntegratedMemorySystem
{
private:
    unique_ptr<BaseAllocator> physical_allocator_;
    unique_ptr<BuddyAllocator> buddy_allocator_;
    unique_ptr<CacheHierarchy> cache_hierarchy_;
    unique_ptr<VirtualMemoryManager> virtual_memory_manager_;
    AllocationMode allocation_mode_;


    Size total_memory_;
    Size page_size_;
    AllocationStrategy alloc_strategy_;
    PageReplacementPolicy page_replacement_policy_;
    bool initialized_;

    unordered_map<ProcessId, vector<Address>> process_allocations_;

    size_t total_operations_;
    size_t cache_hits_;
    size_t cache_misses_;
    size_t page_faults_;

public:
    IntegratedMemorySystem(
        Size total_memory,
        Size page_size = 4096,
        AllocationStrategy alloc_strategy = AllocationStrategy::FIRST_FIT,
        PageReplacementPolicy page_policy = PageReplacementPolicy::LRU);

    size_t getTotalOperations() const { return total_operations_; }
    bool hasProcess(ProcessId pid) const;
    ~IntegratedMemorySystem() = default;
    MemoryStats getPhysicalAllocatorStats() const;
    AllocationMode getAllocationMode() const { return allocation_mode_; }
    void setAllocationMode(AllocationMode mode);
    MemoryStats getBuddyAllocatorStats() const;
    VirtualMemoryManager::VMMStats getVMMStats() const;

    bool initialize();
    bool isInitialized() const { return initialized_; }

    bool createProcess(ProcessId process_id);
    bool terminateProcess(ProcessId process_id);

    AllocationResult allocateMemory(ProcessId process_id, Size size);
    bool deallocateMemory(ProcessId process_id, Address address);
    bool accessMemory(ProcessId process_id, Address virtual_address, bool is_write = false);

    void switchAllocationStrategy(AllocationStrategy new_strategy);
    void switchPageReplacementPolicy(PageReplacementPolicy new_policy);

    void printMemoryDump() const;
    void printStatistics() const;
    void printProcessInfo(ProcessId process_id) const;

    void runMemoryTest(const string &test_name);
    void benchmarkAllocationStrategies();
    void benchmarkCachePerformance();

    Size getTotalMemory() const { return total_memory_; }
    Size getPageSize() const { return page_size_; }
    AllocationStrategy getAllocationStrategy() const { return alloc_strategy_; }
    PageReplacementPolicy getPageReplacementPolicy() const { return page_replacement_policy_; }

private:
    unique_ptr<BaseAllocator> createAllocator(AllocationStrategy strategy, Size memory_size);
    void updateStatistics();
    Address translateVirtualToPhysical(ProcessId process_id, Address virtual_address);
};

#endif
