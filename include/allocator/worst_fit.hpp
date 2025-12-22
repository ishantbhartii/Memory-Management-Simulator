#ifndef WORST_FIT_HPP
#define WORST_FIT_HPP

#include "allocator/base_allocator.hpp"

using namespace std;

class WorstFitAllocator : public BaseAllocator {
public:
    WorstFitAllocator(Size total_memory);
    virtual ~WorstFitAllocator() = default;

    AllocationResult allocate(const AllocationRequest& request) override;

    vector<MemoryBlock>::iterator findFreeBlock(Size size) override;
};

#endif
