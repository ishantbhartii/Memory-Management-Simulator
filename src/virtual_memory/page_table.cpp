#include "../include/virtual_memory/page_table.hpp"
#include <stdexcept>

using namespace std;

PageTable::PageTable(ProcessId process_id, Size page_size)
    : page_size_(page_size), process_id_(process_id) {
    if (page_size == 0) {
        throw invalid_argument("Page size cannot be zero");
    }
}

bool PageTable::addMapping(Address virtual_page, Address physical_frame) {
    if (entries_.find(virtual_page) != entries_.end()) {
        return false;
    }

    PageTableEntry entry;
    entry.frame_number = physical_frame;
    entry.present = true;
    entry.process_id = process_id_;

    entries_[virtual_page] = entry;
    return true;
}

bool PageTable::removeMapping(Address virtual_page) {
    auto it = entries_.find(virtual_page);
    if (it == entries_.end()) {
        return false;
    }

    entries_.erase(it);
    return true;
}

bool PageTable::isPresent(Address virtual_page) const {
    auto it = entries_.find(virtual_page);
    return it != entries_.end() && it->second.present;
}

Address PageTable::getFrame(Address virtual_page) const {
    auto it = entries_.find(virtual_page);
    if (it == entries_.end() || !it->second.present) {
        throw runtime_error("Page not present in memory");
    }
    return it->second.frame_number;
}

void PageTable::setReferenced(Address virtual_page, bool referenced) {
    auto it = entries_.find(virtual_page);
    if (it != entries_.end()) {
        it->second.referenced = referenced;
    }
}

void PageTable::setModified(Address virtual_page, bool modified) {
    auto it = entries_.find(virtual_page);
    if (it != entries_.end()) {
        it->second.modified = modified;
    }
}

size_t PageTable::getPresentPages() const {
    size_t count = 0;
    for (const auto& pair : entries_) {
        if (pair.second.present) {
            count++;
        }
    }
    return count;
}
void PageTable::invalidateFrame(size_t frame_number)
{
    for (auto& pair : entries_) {
        auto& entry = pair.second;
        if (entry.present && entry.frame_number == frame_number) {
            entry.present = false;
            entry.referenced = false;
            entry.modified = false;
        }
    }
}


size_t PageTable::getModifiedPages() const {
    size_t count = 0;
    for (const auto& pair : entries_) {
        if (pair.second.present && pair.second.modified) {
            count++;
        }
    }
    return count;
}

void PageTable::clear() {
    entries_.clear();
}
