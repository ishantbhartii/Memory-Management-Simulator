#ifndef FIRST_FIT_HPP
#define FIRST_FIT_HPP

#include "allocator/base_allocator.hpp"

using namespace std;

class FirstFitAllocator : public BaseAllocator {
public:
    FirstFitAllocator(Size total_memory);
    virtual ~FirstFitAllocator() = default;

    AllocationResult allocate(const AllocationRequest& request) override;

    vector<MemoryBlock>::iterator findFreeBlock(Size size) override;
};

#endif
