#ifndef BEST_FIT_HPP
#define BEST_FIT_HPP

#include "allocator/base_allocator.hpp"

using namespace std;

class BestFitAllocator : public BaseAllocator {
public:
    BestFitAllocator(Size total_memory);
    virtual ~BestFitAllocator() = default;

    AllocationResult allocate(const AllocationRequest& request) override;

    vector<MemoryBlock>::iterator findFreeBlock(Size size) override;
};

#endif
