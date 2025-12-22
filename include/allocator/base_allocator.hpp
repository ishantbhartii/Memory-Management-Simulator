#ifndef BASE_ALLOCATOR_HPP
#define BASE_ALLOCATOR_HPP

#include <vector>
#include <memory>
#include "common/types.hpp"

using namespace std;

class BaseAllocator {
protected:
    vector<MemoryBlock> memory_blocks_;
    Size total_memory_;
    BlockId next_block_id_;

public:
    BaseAllocator(Size total_memory);
    virtual ~BaseAllocator() = default;

    virtual void initialize(Size total_memory);

    virtual AllocationResult allocate(const AllocationRequest& request) = 0;

    virtual bool deallocate(BlockId block_id);

    virtual MemoryStats getStats() const;

    virtual const vector<MemoryBlock>& getBlocks() const;

    virtual void coalesce();

    virtual vector<MemoryBlock>::iterator findFreeBlock(Size size) = 0;

    virtual bool splitBlock(vector<MemoryBlock>::iterator block_it, Size requested_size);

    virtual void mergeBlocks(vector<MemoryBlock>::iterator first,
                             vector<MemoryBlock>::iterator second);

    Size getTotalMemory() const { return total_memory_; }
    bool isInitialized() const { return !memory_blocks_.empty(); }

protected:
    vector<MemoryBlock>::iterator findBlockById(BlockId id);
    vector<MemoryBlock>::const_iterator findBlockById(BlockId id) const;
};

#endif
