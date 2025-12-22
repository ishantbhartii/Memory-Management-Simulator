#ifndef TYPES_HPP
#define TYPES_HPP

#include <cstdint>
#include <cstddef>


using Address = uint32_t;
using Size = uint32_t;
using ProcessId = int;
using BlockId = int;

enum class BlockStatus {
    FREE,
    ALLOCATED
};

enum class AllocationStrategy {
    FIRST_FIT,
    BEST_FIT,
    WORST_FIT
};

enum class CacheReplacementPolicy {
    FIFO,
    LRU,
    LFU
};

enum class PageReplacementPolicy {
    FIFO,
    LRU,
    CLOCK
};

enum class CacheLevel {
    L1,
    L2,
    L3
};

struct MemoryBlock {
    Address start_address;
    Size size;
    BlockStatus status;
    ProcessId process_id;
    BlockId block_id;

    MemoryBlock(
        Address addr = 0,
        Size sz = 0,
        BlockStatus st = BlockStatus::FREE,
        ProcessId pid = -1,
        BlockId bid = -1
    )
        : start_address(addr),
          size(sz),
          status(st),
          process_id(pid),
          block_id(bid) {}

    bool isFree() const {
        return status == BlockStatus::FREE;
    }
};

struct AllocationRequest {
    Size size;
    ProcessId process_id;

    AllocationRequest(Size sz = 0, ProcessId pid = -1)
        : size(sz), process_id(pid) {}
};

struct AllocationResult {
    bool success;
    Address address;
    BlockId block_id;

    AllocationResult(bool s = false, Address addr = 0, BlockId bid = -1)
        : success(s), address(addr), block_id(bid) {}
};

struct MemoryStats {
    Size total_memory;
    Size used_memory;
    Size free_memory;
    double fragmentation_ratio;
    size_t total_blocks;
    size_t free_blocks;
    size_t allocated_blocks;
    Size largest_free_block;

    MemoryStats()
        : total_memory(0),
          used_memory(0),
          free_memory(0),
          fragmentation_ratio(0.0),
          total_blocks(0),
          free_blocks(0),
          allocated_blocks(0),
          largest_free_block(0) {}
};

#endif
