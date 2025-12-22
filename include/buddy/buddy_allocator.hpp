#ifndef BUDDY_ALLOCATOR_HPP
#define BUDDY_ALLOCATOR_HPP

#include <vector>
#include <unordered_map>
#include <list>

#include "common/types.hpp"

using namespace std;

class BuddyAllocator
{
private:
    size_t allocation_requests_ = 0;
    size_t allocation_successes_ = 0;
    size_t allocation_failures_ = 0;
    Size internal_fragmentation_ = 0;
    Size total_memory_;
    int max_order_;
    vector<list<Address>> free_lists_;
    unordered_map<Address, pair<int, ProcessId>> allocated_blocks_;



public:
    BuddyAllocator(Size total_memory);
    ~BuddyAllocator() = default;

    void initialize();

    AllocationResult allocate(const AllocationRequest &request);

    bool deallocate(Address address);

    MemoryStats getStats() const;

    vector<MemoryBlock> getAllocatedBlocks() const;
    vector<MemoryBlock> getFreeBlocks() const;

private:
    int getOrder(Size size) const;

    Address getBuddyAddress(Address address, int order) const;

    void splitBlock(int order);

    void mergeBuddies(int order, Address address);

    bool isValidAddress(Address address, int order) const;

    Size getBlockSize(int order) const;
};

#endif
