#include "buddy/buddy_allocator.hpp"
#include "common/utils.hpp"
#include <iostream>
#include <iomanip>
#include <vector>

using namespace std;

void printBuddySystem(const BuddyAllocator& allocator) {
    cout << "=== Buddy System Status ===" << endl;

    auto free_blocks = allocator.getFreeBlocks();
    auto allocated_blocks = allocator.getAllocatedBlocks();

    cout << "Free Blocks:" << endl;
    for (const auto& block : free_blocks) {
        cout << formatAddress(block.start_address)
             << " " << formatSize(block.size) << endl;
    }

    cout << "Allocated Blocks:" << endl;
    for (const auto& block : allocated_blocks) {
        cout << formatAddress(block.start_address)
             << " " << formatSize(block.size)
             << " P" << block.process_id << endl;
    }

    auto stats = allocator.getStats();
    cout << "Total: " << formatSize(stats.total_memory) << endl;
    cout << "Used: " << formatSize(stats.used_memory) << endl;
    cout << "Free: " << formatSize(stats.free_memory) << endl;
    cout << "Fragmentation: "
         << fixed << setprecision(2)
         << stats.fragmentation_ratio * 100 << "%" << endl;
}

void visualizeBuddyTree(const BuddyAllocator& allocator) {
    cout << "=== Buddy Tree Visualization ===" << endl;

    const int max_addr = 1024;
    vector<char> memory_map(max_addr, '.');

    for (const auto& block : allocator.getFreeBlocks()) {
        for (Size i = 0; i < block.size && block.start_address + i < max_addr; ++i) {
            memory_map[block.start_address + i] = 'F';
        }
    }

    for (const auto& block : allocator.getAllocatedBlocks()) {
        for (Size i = 0; i < block.size && block.start_address + i < max_addr; ++i) {
            memory_map[block.start_address + i] = 'A';
        }
    }

    for (int i = 0; i < max_addr; i += 64) {
        cout << formatAddress(i) << ": ";
        for (int j = 0; j < 64 && i + j < max_addr; ++j) {
            cout << memory_map[i + j];
        }
        cout << endl;
    }
}

bool validateBuddySystem(const BuddyAllocator& allocator) {
    auto free_blocks = allocator.getFreeBlocks();
    auto allocated_blocks = allocator.getAllocatedBlocks();

    for (const auto& free : free_blocks) {
        for (const auto& alloc : allocated_blocks) {
            if (!(free.start_address + free.size <= alloc.start_address ||
                  alloc.start_address + alloc.size <= free.start_address)) {
                cerr << "Overlap detected" << endl;
                return false;
            }
        }
    }

    auto stats = allocator.getStats();
    for (const auto& block : free_blocks) {
        if (block.start_address + block.size > stats.total_memory) {
            return false;
        }
    }

    for (const auto& block : allocated_blocks) {
        if (block.start_address + block.size > stats.total_memory) {
            return false;
        }
    }

    return true;
}
