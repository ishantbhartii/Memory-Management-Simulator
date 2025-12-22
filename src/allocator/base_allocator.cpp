#include "allocator/base_allocator.hpp"
#include "common/utils.hpp"
#include <algorithm>
#include <stdexcept>

BaseAllocator::BaseAllocator(Size total_memory)
    : total_memory_(total_memory),
      next_block_id_(0) {
    stats_.total_memory = total_memory;
}



void BaseAllocator::initialize(Size total_memory) {
    total_memory_ = total_memory;
    memory_blocks_.clear();
    next_block_id_ = 0;

    allocation_requests_ = 0;
    allocation_successes_ = 0;
    allocation_failures_ = 0;
    internal_fragmentation_ = 0;

    MemoryBlock initial_block(0, total_memory, BlockStatus::FREE, -1, generateBlockId());
    memory_blocks_.push_back(initial_block);
}


bool BaseAllocator::deallocate(BlockId block_id) {
    auto it = findBlockById(block_id);
    if (it == memory_blocks_.end()) return false;
    if (it->status != BlockStatus::ALLOCATED) return false;

    it->status = BlockStatus::FREE;
    it->process_id = -1;

    coalesce();
    return true;
}

MemoryStats BaseAllocator::getStats() const
{
    MemoryStats stats;
    stats.total_memory = total_memory_;

    Size used = 0;
    Size free = 0;
    Size largest_free = 0;

    for (const auto& block : memory_blocks_) {
        if (block.status == BlockStatus::ALLOCATED) {
            used += block.size;
        } else {
            free += block.size;
            largest_free = max(largest_free, block.size);
        }
    }

    stats.used_memory = used;
    stats.free_memory = free;
    stats.total_blocks = memory_blocks_.size();

    stats.allocated_blocks = allocation_successes_;
    stats.free_blocks = stats.total_blocks - stats.allocated_blocks;

    stats.largest_free_block = largest_free;
    stats.internal_fragmentation = internal_fragmentation_;

    stats.allocation_requests = allocation_requests_;
    stats.allocation_successes = allocation_successes_;
    stats.allocation_failures = allocation_failures_;

    if (free > 0) {
        stats.fragmentation_ratio =
            1.0 - (static_cast<double>(largest_free) / free);
    } else {
        stats.fragmentation_ratio = 0.0;
    }

    return stats;
}


const vector<MemoryBlock>& BaseAllocator::getBlocks() const {
    return memory_blocks_;
}

void BaseAllocator::coalesce() {
    if (memory_blocks_.size() < 2) return;

    sort(memory_blocks_.begin(), memory_blocks_.end(),
         [](const MemoryBlock& a, const MemoryBlock& b) {
             return a.start_address < b.start_address;
         });

    for (auto it = memory_blocks_.begin(); it != memory_blocks_.end() - 1; ) {
        auto next_it = it + 1;

        if (it->status == BlockStatus::FREE &&
            next_it->status == BlockStatus::FREE &&
            it->start_address + it->size == next_it->start_address) {
            it->size += next_it->size;
            memory_blocks_.erase(next_it);
        } else {
            ++it;
        }
    }
}

bool BaseAllocator::splitBlock(vector<MemoryBlock>::iterator block_it, Size requested_size) {
    if (block_it == memory_blocks_.end()) return false;
    if (block_it->status != BlockStatus::FREE) return false;
    if (block_it->size <= requested_size) return false;

    Address new_start = block_it->start_address + requested_size;
    Size remaining_size = block_it->size - requested_size;

    MemoryBlock new_block(
        new_start,
        remaining_size,
        BlockStatus::FREE,
        -1,
        next_block_id_++
    );

    block_it->size = requested_size;
    memory_blocks_.insert(block_it + 1, new_block);

    return true;
}

void BaseAllocator::mergeBlocks(vector<MemoryBlock>::iterator first,
                                vector<MemoryBlock>::iterator second) {
    if (first == memory_blocks_.end()) return;
    if (second == memory_blocks_.end()) return;
    if (first->status != BlockStatus::FREE) return;
    if (second->status != BlockStatus::FREE) return;
    if (first->start_address + first->size != second->start_address) return;

    first->size += second->size;
    memory_blocks_.erase(second);
}

vector<MemoryBlock>::iterator BaseAllocator::findBlockById(BlockId id) {
    return find_if(
        memory_blocks_.begin(),
        memory_blocks_.end(),
        [id](const MemoryBlock& block) {
            return block.block_id == id;
        }
    );
}

vector<MemoryBlock>::const_iterator BaseAllocator::findBlockById(BlockId id) const {
    return find_if(
        memory_blocks_.cbegin(),
        memory_blocks_.cend(),
        [id](const MemoryBlock& block) {
            return block.block_id == id;
        }
    );
}
