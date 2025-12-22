#include "allocator/best_fit.hpp"
#include <algorithm>
#include <limits>

BestFitAllocator::BestFitAllocator(Size total_memory)
    : BaseAllocator(total_memory) {
}

AllocationResult BestFitAllocator::allocate(const AllocationRequest& request) {
    if (request.size == 0) {
        return AllocationResult(false, 0, -1);
    }

    auto block_it = findFreeBlock(request.size);
    if (block_it == memory_blocks_.end()) {
        return AllocationResult(false, 0, -1);
    }

    if (block_it->size > request.size) {
        splitBlock(block_it, request.size);
    }

    block_it->status = BlockStatus::ALLOCATED;
    block_it->process_id = request.process_id;

    return AllocationResult(true, block_it->start_address, block_it->block_id);
}

vector<MemoryBlock>::iterator BestFitAllocator::findFreeBlock(Size size) {
    vector<MemoryBlock>::iterator best_fit = memory_blocks_.end();
    Size smallest_size = numeric_limits<Size>::max();

    for (auto it = memory_blocks_.begin(); it != memory_blocks_.end(); ++it) {
        if (it->status == BlockStatus::FREE && it->size >= size) {
            if (it->size < smallest_size) {
                smallest_size = it->size;
                best_fit = it;
            }
        }
    }

    return best_fit;
}
