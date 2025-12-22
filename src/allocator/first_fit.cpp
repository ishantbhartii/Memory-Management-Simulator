#include "allocator/first_fit.hpp"
#include <algorithm>

FirstFitAllocator::FirstFitAllocator(Size total_memory)
    : BaseAllocator(total_memory) {
}

AllocationResult FirstFitAllocator::allocate(const AllocationRequest& request) {
    allocation_requests_++;
    if (request.size == 0) {
        allocation_failures_++;
        return AllocationResult(false, 0, -1);
    }

    auto block_it = findFreeBlock(request.size);
    if (block_it == memory_blocks_.end()) {
        allocation_failures_++;
        return AllocationResult(false, 0, -1);
    }

    Address alloc_address = block_it->start_address;
    BlockId alloc_block_id = block_it->block_id;
    if (block_it->size > request.size) {
        internal_fragmentation_ += (block_it->size - request.size);
        splitBlock(block_it, request.size);
        block_it = findBlockById(alloc_block_id);
    }

    block_it->status = BlockStatus::ALLOCATED;
    block_it->process_id = request.process_id;

    allocation_successes_++;
    return AllocationResult(true, alloc_address, alloc_block_id);
}

vector<MemoryBlock>::iterator FirstFitAllocator::findFreeBlock(Size size) {
    return find_if(
        memory_blocks_.begin(),
        memory_blocks_.end(),
        [size](const MemoryBlock& block) {
            return block.status == BlockStatus::FREE && block.size >= size;
        }
    );
}
