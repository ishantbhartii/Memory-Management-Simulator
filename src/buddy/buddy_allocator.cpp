#include "buddy/buddy_allocator.hpp"
#include "common/utils.hpp"
#include <algorithm>
#include <stdexcept>

using namespace std;

BuddyAllocator::BuddyAllocator(Size total_memory)
    : total_memory_(total_memory)
{
    if (!isPowerOfTwo(total_memory))
    {
        throw invalid_argument("Total memory must be a power of 2");
    }

    max_order_ = log2Floor(total_memory);
    free_lists_.resize(max_order_ + 1);
}

void BuddyAllocator::initialize()
{
    free_lists_.clear();
    free_lists_.resize(max_order_ + 1);
    allocated_blocks_.clear();
    free_lists_[max_order_].push_back(0);
}

AllocationResult BuddyAllocator::allocate(const AllocationRequest &request)
{
    allocation_requests_++;
    if (request.size == 0 || request.size > total_memory_)
    {
        allocation_failures_++;
        return AllocationResult(false, 0, -1);
    }

    Size actual_size = nextPowerOfTwo(request.size);
    int required_order = getOrder(actual_size);

    int order = required_order;
    while (order <= max_order_ && free_lists_[order].empty())
    {
        order++;
    }

    if (order > max_order_)
    {
        allocation_failures_++;
        return AllocationResult(false, 0, -1);
    }

    while (order > required_order)
    {
        splitBlock(order);
        order--;
    }

    Address address = free_lists_[order].front();
    free_lists_[order].pop_front();

    allocated_blocks_[address] = make_pair(required_order, request.process_id);

    allocation_successes_++;
    internal_fragmentation_ += (actual_size - request.size);

    return AllocationResult(true, address, static_cast<BlockId>(address));
}

bool BuddyAllocator::deallocate(Address address)
{
    auto it = allocated_blocks_.find(address);
    if (it == allocated_blocks_.end())
    {
        return false;
    }

    int order = it->second.first;
    allocated_blocks_.erase(it);
    mergeBuddies(order, address);
    return true;
}

MemoryStats BuddyAllocator::getStats() const
{
    MemoryStats stats;
    stats.total_memory = total_memory_;

    Size used = 0;
    for (const auto &pair : allocated_blocks_)
    {
        used += getBlockSize(pair.second.first);
    }

    stats.used_memory = used;
    stats.free_memory = total_memory_ - used;
    stats.total_blocks = allocated_blocks_.size();
    stats.allocated_blocks = allocated_blocks_.size();

    stats.internal_fragmentation = internal_fragmentation_;
    stats.allocation_requests = allocation_requests_;
    stats.allocation_successes = allocation_successes_;
    stats.allocation_failures = allocation_failures_;

    size_t free_count = 0;
    Size largest_free = 0;

    for (int i = 0; i <= max_order_; ++i)
    {
        free_count += free_lists_[i].size();
        if (!free_lists_[i].empty())
        {
            largest_free = max(largest_free, getBlockSize(i));
        }
    }

    stats.free_blocks = free_count;
    stats.largest_free_block = largest_free;
    if (stats.free_memory > 0)
    {
        stats.fragmentation_ratio =
            1.0 - (static_cast<double>(stats.largest_free_block) / stats.free_memory);
    }
    else
    {
        stats.fragmentation_ratio = 0.0;
    }
    if (stats.total_memory > 0)
    {
        stats.memory_utilization =
            static_cast<double>(stats.used_memory) / stats.total_memory;
    }
    else
    {
        stats.memory_utilization = 0.0;
    }

    return stats;
}

vector<MemoryBlock> BuddyAllocator::getAllocatedBlocks() const
{
    vector<MemoryBlock> blocks;

    for (const auto &pair : allocated_blocks_)
    {
        Address addr = pair.first;
        int order = pair.second.first;
        ProcessId pid = pair.second.second;

        blocks.emplace_back(
            addr,
            getBlockSize(order),
            BlockStatus::ALLOCATED,
            pid,
            static_cast<BlockId>(addr));
    }

    return blocks;
}

vector<MemoryBlock> BuddyAllocator::getFreeBlocks() const
{
    vector<MemoryBlock> blocks;

    for (int order = 0; order <= max_order_; ++order)
    {
        Size size = getBlockSize(order);
        for (Address addr : free_lists_[order])
        {
            blocks.emplace_back(addr, size, BlockStatus::FREE, -1, static_cast<BlockId>(addr));
        }
    }

    return blocks;
}

int BuddyAllocator::getOrder(Size size) const
{
    return log2Floor(nextPowerOfTwo(size));
}

Address BuddyAllocator::getBuddyAddress(Address address, int order) const
{
    return address ^ getBlockSize(order);
}

void BuddyAllocator::splitBlock(int order)
{
    if (order == 0 || free_lists_[order].empty())
        return;

    Address address = free_lists_[order].front();
    free_lists_[order].pop_front();

    int lower = order - 1;
    Size half = getBlockSize(lower);

    free_lists_[lower].push_back(address);
    free_lists_[lower].push_back(address + half);
}

void BuddyAllocator::mergeBuddies(int order, Address address)
{
    if (order == max_order_)
    {
        free_lists_[order].push_back(address);
        return;
    }

    Address buddy = getBuddyAddress(address, order);
    auto &list = free_lists_[order];

    auto it = find(list.begin(), list.end(), buddy);
    if (it != list.end())
    {
        list.erase(it);
        mergeBuddies(order + 1, min(address, buddy));
    }
    else
    {
        list.push_back(address);
    }
}

bool BuddyAllocator::isValidAddress(Address address, int order) const
{
    Size size = getBlockSize(order);
    return address % size == 0 && address + size <= total_memory_;
}

Size BuddyAllocator::getBlockSize(int order) const
{
    return static_cast<Size>(1) << order;
}
