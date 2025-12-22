#ifndef PAGE_TABLE_HPP
#define PAGE_TABLE_HPP

#include <unordered_map>
#include <vector>
#include <memory>

#include "common/types.hpp"

using namespace std;

struct PageTableEntry {
    Address frame_number;
    bool present;
    bool referenced;
    bool modified;
    ProcessId process_id;

    PageTableEntry()
        : frame_number(0),
          present(false),
          referenced(false),
          modified(false),
          process_id(-1) {}
};

class PageTable {
private:
    unordered_map<Address, PageTableEntry> entries_;
    Size page_size_;
    ProcessId process_id_;

public:
    PageTable(ProcessId process_id, Size page_size);
    void invalidateFrame(size_t frame_number);

    bool addMapping(Address virtual_page, Address physical_frame);
    bool removeMapping(Address virtual_page);
    bool isPresent(Address virtual_page) const;
    Address getFrame(Address virtual_page) const;

    void setReferenced(Address virtual_page, bool referenced);
    void setModified(Address virtual_page, bool modified);

    const unordered_map<Address, PageTableEntry>& getEntries() const { return entries_; }
    Size getPageSize() const { return page_size_; }
    ProcessId getProcessId() const { return process_id_; }

    size_t getNumPages() const { return entries_.size(); }
    size_t getPresentPages() const;
    size_t getModifiedPages() const;

    void clear();
};

#endif
