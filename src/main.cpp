#include "cli/cli.hpp"
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char* argv[]) {
    try {
        Size total_memory = 1024 * 1024;
        Size page_size = 4096;
        AllocationStrategy alloc_strategy = AllocationStrategy::FIRST_FIT;
        PageReplacementPolicy page_policy = PageReplacementPolicy::LRU;

        for (int i = 1; i < argc; ++i) {
            string arg = argv[i];

            if (arg == "--memory" && i + 1 < argc) {
                total_memory = stoul(argv[++i]);
            } else if (arg == "--page-size" && i + 1 < argc) {
                page_size = stoul(argv[++i]);
            } else if (arg == "--strategy" && i + 1 < argc) {
                string strategy = argv[++i];
                if (strategy == "first") alloc_strategy = AllocationStrategy::FIRST_FIT;
                else if (strategy == "best") alloc_strategy = AllocationStrategy::BEST_FIT;
                else if (strategy == "worst") alloc_strategy = AllocationStrategy::WORST_FIT;
            } else if (arg == "--page-policy" && i + 1 < argc) {
                string policy = argv[++i];
                if (policy == "fifo") page_policy = PageReplacementPolicy::FIFO;
                else if (policy == "lru") page_policy = PageReplacementPolicy::LRU;
                else if (policy == "clock") page_policy = PageReplacementPolicy::CLOCK;
            } else if (arg == "--help") {
                cout << "Memory Management Simulator\n";
                cout << "Usage: " << argv[0] << " [options]\n";
                cout << "  --memory <size>\n";
                cout << "  --page-size <size>\n";
                cout << "  --strategy <first|best|worst>\n";
                cout << "  --page-policy <fifo|lru|clock>\n";
                return 0;
            }
        }

        IntegratedMemorySystem memory_system(
            total_memory,
            page_size,
            alloc_strategy,
            page_policy
        );

        CLI cli(memory_system);
        cli.run();

    } catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }

    return 0;
}
