#include "allocator/worst_fit.hpp"
#include <algorithm>

WorstFitAllocator::WorstFitAllocator(Size total_memory)
    : BaseAllocator(total_memory) {
}

AllocationResult WorstFitAllocator::allocate(const AllocationRequest& request) {
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
    return AllocationResult(true, block_it->start_address, block_it->block_id);
}

vector<MemoryBlock>::iterator WorstFitAllocator::findFreeBlock(Size size) {
    vector<MemoryBlock>::iterator worst_fit = memory_blocks_.end();
    Size largest_size = 0;

    for (auto it = memory_blocks_.begin(); it != memory_blocks_.end(); ++it) {
        if (it->status == BlockStatus::FREE && it->size >= size) {
            if (it->size > largest_size) {
                largest_size = it->size;
                worst_fit = it;
            }
        }
    }

    return worst_fit;
}
