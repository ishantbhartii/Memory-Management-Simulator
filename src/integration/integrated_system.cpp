#include "integration/integrated_system.hpp"
#include "allocator/first_fit.hpp"
#include "allocator/best_fit.hpp"
#include "allocator/worst_fit.hpp"
#include "common/utils.hpp"
#include <iostream>
#include <iomanip>
#include <algorithm>

using namespace std;

IntegratedMemorySystem::IntegratedMemorySystem(
    Size total_memory,
    Size page_size,
    AllocationStrategy alloc_strategy,
    PageReplacementPolicy page_policy)
    : total_memory_(total_memory),
      page_size_(page_size),
      alloc_strategy_(alloc_strategy),
      page_replacement_policy_(page_policy),
      initialized_(false),
      total_operations_(0),
      cache_hits_(0),
      cache_misses_(0),
      page_faults_(0)
{
}

bool IntegratedMemorySystem::initialize()
{
    try
    {
        physical_allocator_ = createAllocator(alloc_strategy_, total_memory_);
        physical_allocator_->initialize(total_memory_);

        Size buddy_memory = total_memory_ / 2;
        if (!isPowerOfTwo(buddy_memory))
        {
            buddy_memory = nextPowerOfTwo(buddy_memory);
        }

        buddy_allocator_ = make_unique<BuddyAllocator>(buddy_memory);
        buddy_allocator_->initialize();

        cache_hierarchy_ = make_unique<CacheHierarchy>(
            32768, 262144, 2097152,
            64, 8, 16, 16,
            CacheReplacementPolicy::LRU,
            CacheReplacementPolicy::LRU,
            CacheReplacementPolicy::LRU);

        virtual_memory_manager_ = make_unique<VirtualMemoryManager>(
            total_memory_,
            page_size_,
            page_replacement_policy_);

        initialized_ = true;
        return true;
    }
    catch (const exception &e)
    {
        cerr << "Failed to initialize integrated system: "
             << e.what() << endl;
        initialized_ = false;
        return false;
    }
}


unique_ptr<BaseAllocator>
IntegratedMemorySystem::createAllocator(AllocationStrategy strategy, Size memory_size)
{
    switch (strategy)
    {
    case AllocationStrategy::FIRST_FIT:
        return make_unique<FirstFitAllocator>(memory_size);
    case AllocationStrategy::BEST_FIT:
        return make_unique<BestFitAllocator>(memory_size);
    case AllocationStrategy::WORST_FIT:
        return make_unique<WorstFitAllocator>(memory_size);
    default:
        return make_unique<FirstFitAllocator>(memory_size);
    }
}

bool IntegratedMemorySystem::createProcess(ProcessId process_id)
{
    if (process_allocations_.count(process_id))
        return false;
    process_allocations_[process_id] = vector<Address>();
    return virtual_memory_manager_->createProcess(process_id);
}

bool IntegratedMemorySystem::terminateProcess(ProcessId process_id)
{
    auto it = process_allocations_.find(process_id);
    if (it == process_allocations_.end())
        return false;

    for (Address addr : it->second)
    {
        deallocateMemory(process_id, addr);
    }

    process_allocations_.erase(it);
    return virtual_memory_manager_->terminateProcess(process_id);
}

AllocationResult IntegratedMemorySystem::allocateMemory(ProcessId process_id, Size size)
{
    if (!initialized_)
    {
        return AllocationResult(false, 0, -1);
    }

    total_operations_++;

    auto it = process_allocations_.find(process_id);
    if (it == process_allocations_.end())
    {
        return AllocationResult(false, 0, -1);
    }

    if (isPowerOfTwo(size))
    {
        AllocationResult result = buddy_allocator_->allocate({size, process_id});
        if (result.success)
        {
            it->second.push_back(result.address);
            cache_misses_++;
            return result;
        }
    }

    AllocationResult result = physical_allocator_->allocate({size, process_id});
    if (result.success)
    {
        it->second.push_back(result.address);
        cache_misses_++;
    }

    return result;
}

bool IntegratedMemorySystem::deallocateMemory(ProcessId process_id, Address address)
{
    if (!initialized_)
    {
        return false;
    }

    

    auto it = process_allocations_.find(process_id);
    if (it == process_allocations_.end())
        return false;

    if (buddy_allocator_->deallocate(address))
    {
        it->second.erase(remove(it->second.begin(), it->second.end(), address), it->second.end());
        return true;
    }

    if (physical_allocator_->deallocate(static_cast<BlockId>(address)))
    {
        it->second.erase(remove(it->second.begin(), it->second.end(), address), it->second.end());
        return true;
    }

    return false;
}

bool IntegratedMemorySystem::accessMemory(ProcessId process_id, Address virtual_address, bool is_write)
{
    if (!initialized_)
    {
        return false;
    }

    

    if (!virtual_memory_manager_->accessMemory(process_id, virtual_address, is_write))
    {
        return false;
    }

    Address physical_address = translateVirtualToPhysical(process_id, virtual_address);

    bool hit = is_write
                   ? cache_hierarchy_->write(physical_address, process_id)
                   : cache_hierarchy_->read(physical_address, process_id);

    if (hit)
        cache_hits_++;
    else
        cache_misses_++;

    return true;
}

void IntegratedMemorySystem::switchAllocationStrategy(AllocationStrategy new_strategy)
{
    alloc_strategy_ = new_strategy;
    physical_allocator_ = createAllocator(new_strategy, total_memory_);
    physical_allocator_->initialize(total_memory_);
}
bool IntegratedMemorySystem::hasProcess(ProcessId pid) const
{
    return process_allocations_.find(pid) != process_allocations_.end();
}


void IntegratedMemorySystem::switchPageReplacementPolicy(PageReplacementPolicy new_policy)
{
    page_replacement_policy_ = new_policy;
    virtual_memory_manager_ = make_unique<VirtualMemoryManager>(
        total_memory_,
        page_size_,
        new_policy);

    auto old = process_allocations_;
    process_allocations_.clear();

    for (const auto &p : old)
    {
        createProcess(p.first);
    }
}

Address IntegratedMemorySystem::translateVirtualToPhysical(
    ProcessId process_id,
    Address virtual_address)
{
    return virtual_address;
}

void IntegratedMemorySystem::printMemoryDump() const
{
    cout << "=== INTEGRATED MEMORY SYSTEM DUMP ===" << endl;

    for (const auto &block : physical_allocator_->getBlocks())
    {
        cout << formatAddress(block.start_address) << " "
             << formatSize(block.size) << " "
             << (block.status == BlockStatus::FREE ? "FREE" : "ALLOCATED")
             << endl;
    }
}

void IntegratedMemorySystem::printStatistics() const
{
    cout << "Operations: " << total_operations_ << endl;
    cout << "Cache hits: " << cache_hits_ << endl;
    cout << "Cache misses: " << cache_misses_ << endl;

    if (buddy_allocator_)
    {
        auto buddy_stats = buddy_allocator_->getStats();
        cout << "Buddy Allocator: "
             << formatSize(buddy_stats.used_memory) << " used, "
             << formatSize(buddy_stats.free_memory) << " free, "
             << "Internal Fragmentation: "
             << buddy_stats.fragmentation_ratio * 100 << "%"
             << endl;
    }
    else
    {
        cout << "Buddy Allocator: not initialized" << endl;
    }

    if (physical_allocator_)
    {
        auto phys_stats = physical_allocator_->getStats();
        cout << "Physical Allocator: "
             << formatSize(phys_stats.used_memory) << " used, "
             << formatSize(phys_stats.free_memory) << " free"
             << endl;
    }
    auto vm = virtual_memory_manager_.get();

    cout << "Virtual Memory:" << endl;
    cout << "Page accesses: " << vm->getPageAccesses() << endl;
    cout << "Page faults: " << vm->getPageFaults() << endl;
    cout << "Page replacements: " << vm->getPageReplacements() << endl;

    if (vm->getPageAccesses() > 0)
    {
        cout << "Page fault rate: "
             << (double)vm->getPageFaults() / vm->getPageAccesses()
             << endl;
    }
}

void IntegratedMemorySystem::printProcessInfo(ProcessId process_id) const
{
    auto it = process_allocations_.find(process_id);
    if (it == process_allocations_.end())
        return;
    cout << "Process " << process_id << " allocations: " << it->second.size() << endl;
}

void IntegratedMemorySystem::runMemoryTest(const string &test_name)
{
    ProcessId pid = 999;
    createProcess(pid);

    auto r1 = allocateMemory(pid, 1024);
    auto r2 = allocateMemory(pid, 2048);
    auto r3 = allocateMemory(pid, 512);

    if (r1.success)
        accessMemory(pid, r1.address, false);
    if (r2.success)
        accessMemory(pid, r2.address, true);

    printStatistics();
    terminateProcess(pid);
}

void IntegratedMemorySystem::benchmarkAllocationStrategies()
{
    vector<AllocationStrategy> strategies = {
        AllocationStrategy::FIRST_FIT,
        AllocationStrategy::BEST_FIT,
        AllocationStrategy::WORST_FIT};

    for (auto s : strategies)
    {
        switchAllocationStrategy(s);
        ProcessId pid = 1000;
        createProcess(pid);

        for (Size sz : {100u, 200u, 50u, 300u, 75u})
        {
            allocateMemory(pid, sz);
        }

        terminateProcess(pid);
    }
}

void IntegratedMemorySystem::benchmarkCachePerformance()
{
    ProcessId pid = 1001;
    createProcess(pid);

    auto res = allocateMemory(pid, 4096);
    if (!res.success)
        return;

    for (int i = 0; i < 100; ++i)
    {
        accessMemory(pid, res.address, false);
    }

    terminateProcess(pid);
}
MemoryStats IntegratedMemorySystem::getPhysicalAllocatorStats() const {
    if (!physical_allocator_) return MemoryStats();
    return physical_allocator_->getStats();
}

MemoryStats IntegratedMemorySystem::getBuddyAllocatorStats() const {
    if (!buddy_allocator_) return MemoryStats();
    return buddy_allocator_->getStats();
}

VirtualMemoryManager::VMMStats IntegratedMemorySystem::getVMMStats() const {
    if (!virtual_memory_manager_)
        return VirtualMemoryManager::VMMStats{};
    return virtual_memory_manager_->getStats();
}


