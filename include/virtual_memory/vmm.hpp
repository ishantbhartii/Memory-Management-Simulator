#ifndef VMM_HPP
#define VMM_HPP

#include <unordered_map>
#include <vector>
#include <memory>
#include <queue>

#include "virtual_memory/page_table.hpp"
#include "common/types.hpp"

using namespace std;

class VirtualMemoryManager
{
private:
    Size physical_memory_size_;
    Size page_size_;
    size_t num_frames_;
    size_t page_accesses_;
    size_t page_faults_;
    size_t page_replacements_;
    

    vector<bool> frame_allocation_;
    unordered_map<ProcessId, unique_ptr<PageTable>> process_tables_;

    PageReplacementPolicy replacement_policy_;
    queue<pair<ProcessId, Address>> fifo_queue_;
    unordered_map<ProcessId, unordered_map<Address, size_t>> access_counts_;
    unordered_map<ProcessId, unordered_map<Address, size_t>> access_times_;

    static size_t global_time_;
private:
    void invalidatePageUsingFrame(size_t frame);

public:
    size_t getPageFaults() const { return page_faults_; }
    size_t getPageAccesses() const { return page_accesses_; }
    size_t getPageReplacements() const { return page_replacements_; }

    VirtualMemoryManager(Size physical_memory_size, Size page_size, PageReplacementPolicy policy);

    bool createProcess(ProcessId process_id);
    bool terminateProcess(ProcessId process_id);

    bool accessMemory(ProcessId process_id, Address virtual_address, bool is_write = false);

    bool handlePageFault(ProcessId process_id, Address virtual_page);

    Address selectVictimPage();
    void updatePageAccess(ProcessId process_id, Address virtual_page);

    bool allocatePages(ProcessId process_id, size_t num_pages);
    bool deallocatePages(ProcessId process_id, size_t num_pages);

    struct VMMStats
    {
        size_t page_faults;
        size_t page_replacements;
        size_t total_accesses;
        double page_fault_rate;
        size_t free_frames;
        size_t total_frames;
    };

    VMMStats getStats() const;
    const PageTable *getPageTable(ProcessId process_id) const;
    size_t getPageFaultCount() const { return page_faults_; }

    Address virtualToPage(Address virtual_address) const;
    size_t allocateFrame();
    void freeFrame(Address frame_number);

    void resetStats();

private:
    Address selectFIFOVictim();
    Address selectLRUVictim();
    Address selectClockVictim();
    Address selectLFUVictim();
};

#endif
