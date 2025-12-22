#include "../include/virtual_memory/vmm.hpp"
#include <algorithm>
#include <stdexcept>
#include <iostream>

size_t VirtualMemoryManager::global_time_ = 0;

VirtualMemoryManager::VirtualMemoryManager(
    Size physical_memory_size,
    Size page_size,
    PageReplacementPolicy policy)
    : physical_memory_size_(physical_memory_size),
      page_size_(page_size),
      replacement_policy_(policy),
      page_accesses_(0),
      page_faults_(0),
      page_replacements_(0)
{

    if (physical_memory_size == 0 || page_size == 0)
    {
        throw std::invalid_argument("Memory and page sizes must be positive");
    }

    if (physical_memory_size % page_size != 0)
    {
        throw std::invalid_argument("Physical memory size must be divisible by page size");
    }

    num_frames_ = physical_memory_size / page_size;
    frame_allocation_.resize(num_frames_, false);
}

bool VirtualMemoryManager::createProcess(ProcessId process_id)
{
    if (process_tables_.find(process_id) != process_tables_.end())
    {
        return false;
    }

    process_tables_[process_id] = std::make_unique<PageTable>(process_id, page_size_);
    return true;
}

bool VirtualMemoryManager::terminateProcess(ProcessId process_id)
{
    auto it = process_tables_.find(process_id);
    if (it == process_tables_.end())
    {
        return false;
    }

    const auto &entries = it->second->getEntries();
    for (const auto &entry : entries)
    {
        if (entry.second.present)
        {
            freeFrame(entry.second.frame_number);
        }
    }

    process_tables_.erase(it);
    return true;
}

bool VirtualMemoryManager::accessMemory(ProcessId process_id, Address virtual_address, bool is_write)
{
    page_accesses_++;

    auto it = process_tables_.find(process_id);
    if (it == process_tables_.end())
    {
        return false;
    }

    Address virtual_page = virtualToPage(virtual_address);

    if (!it->second->isPresent(virtual_page))
    {
        page_faults_++;
        if (!handlePageFault(process_id, virtual_page))
        {
            return false;
        }
    }

    updatePageAccess(process_id, virtual_page);

    it->second->setReferenced(virtual_page, true);
    if (is_write)
    {
        it->second->setModified(virtual_page, true);
    }

    return true;
}

bool VirtualMemoryManager::handlePageFault(ProcessId process_id, Address virtual_page)
{
    size_t frame = allocateFrame();
    if (frame == static_cast<size_t>(-1))
    {
        frame = selectVictimPage();
        if (frame == static_cast<size_t>(-1))
            return false;

        invalidatePageUsingFrame(frame);
        page_replacements_++;
    }

    auto it = process_tables_.find(process_id);
    if (it == process_tables_.end())
    {
        return false;
    }

    return it->second->addMapping(virtual_page, frame);
}
void VirtualMemoryManager::invalidatePageUsingFrame(size_t frame)
{
    for (auto &process_pair : process_tables_)
    {
        process_pair.second->invalidateFrame(frame);
    }
}

Address VirtualMemoryManager::selectVictimPage()
{
    switch (replacement_policy_)
    {
    case PageReplacementPolicy::FIFO:
        return selectFIFOVictim();
    case PageReplacementPolicy::LRU:
        return selectLRUVictim();
    case PageReplacementPolicy::CLOCK:
        return selectClockVictim();
    default:
        return selectFIFOVictim();
    }
}

void VirtualMemoryManager::updatePageAccess(ProcessId process_id, Address virtual_page)
{
    switch (replacement_policy_)
    {
    case PageReplacementPolicy::FIFO:
        break;
    case PageReplacementPolicy::LRU:
        access_times_[process_id][virtual_page] = global_time_++;
        break;
    case PageReplacementPolicy::CLOCK:
        break;
    default:
        break;
    }
}

Address VirtualMemoryManager::selectFIFOVictim()
{
    for (size_t i = 0; i < num_frames_; ++i)
    {
        if (frame_allocation_[i])
        {
            return i;
        }
    }
    return static_cast<Address>(-1);
}

Address VirtualMemoryManager::selectLRUVictim()
{
    Address victim_frame = static_cast<Address>(-1);
    size_t min_time = SIZE_MAX;

    for (const auto &process_pair : process_tables_)
    {
        ProcessId pid = process_pair.first;
        const auto &page_table = process_pair.second->getEntries();
        for (const auto &page_pair : page_table)
        {
            if (page_pair.second.present)
            {
                auto time_it = access_times_[pid].find(page_pair.first);
                size_t time = (time_it != access_times_[pid].end()) ? time_it->second : 0;

                if (time < min_time)
                {
                    min_time = time;
                    victim_frame = page_pair.second.frame_number;
                }
            }
        }
    }

    return victim_frame;
}

Address VirtualMemoryManager::selectClockVictim()
{
    static size_t clock_hand = 0;

    for (size_t i = 0; i < num_frames_; ++i)
    {
        size_t frame = (clock_hand + i) % num_frames_;
        if (frame_allocation_[frame])
        {
            clock_hand = (frame + 1) % num_frames_;
            return frame;
        }
    }

    return static_cast<Address>(-1);
}

Address VirtualMemoryManager::selectLFUVictim()
{
    Address victim_frame = static_cast<Address>(-1);
    size_t min_count = SIZE_MAX;

    for (const auto &process_pair : process_tables_)
    {
        ProcessId pid = process_pair.first;
        const auto &page_table = process_pair.second->getEntries();
        for (const auto &page_pair : page_table)
        {
            if (page_pair.second.present)
            {
                auto count_it = access_counts_[pid].find(page_pair.first);
                size_t count = (count_it != access_counts_[pid].end()) ? count_it->second : 0;

                if (count < min_count)
                {
                    min_count = count;
                    victim_frame = page_pair.second.frame_number;
                }
            }
        }
    }

    return victim_frame;
}

bool VirtualMemoryManager::allocatePages(ProcessId process_id, size_t num_pages)
{
    auto it = process_tables_.find(process_id);
    if (it == process_tables_.end())
    {
        return false;
    }

    for (size_t i = 0; i < num_pages; ++i)
    {
        size_t frame = allocateFrame();
        if (frame == static_cast<size_t>(-1))
        {
            for (size_t j = 0; j < i; ++j)
            {
                Address virtual_page = it->second->getNumPages() - i + j;
                auto frame_num = it->second->getFrame(virtual_page);
                freeFrame(frame_num);
                it->second->removeMapping(virtual_page);
            }
            return false;
        }

        Address virtual_page = frame;
        it->second->addMapping(virtual_page, frame);
    }

    return true;
}

bool VirtualMemoryManager::deallocatePages(ProcessId process_id, size_t num_pages)
{
    auto it = process_tables_.find(process_id);
    if (it == process_tables_.end())
    {
        return false;
    }

    const auto &entries = it->second->getEntries();
    size_t deallocated = 0;

    for (const auto &entry : entries)
    {
        if (deallocated >= num_pages)
            break;
        if (entry.second.present)
        {
            freeFrame(entry.second.frame_number);
            it->second->removeMapping(entry.first);
            deallocated++;
        }
    }

    return deallocated == num_pages;
}

VirtualMemoryManager::VMMStats VirtualMemoryManager::getStats() const
{
    VMMStats stats;

    stats.page_faults = page_faults_;
    stats.page_replacements = page_replacements_;
    stats.total_accesses = page_accesses_;

    stats.page_fault_rate =
        page_accesses_ > 0
            ? static_cast<double>(page_faults_) / page_accesses_
            : 0.0;

    stats.free_frames = 0;
    for (bool allocated : frame_allocation_)
    {
        if (!allocated)
            stats.free_frames++;
    }

    stats.total_frames = num_frames_;
    return stats;
}

const PageTable *VirtualMemoryManager::getPageTable(ProcessId process_id) const
{
    auto it = process_tables_.find(process_id);
    return it != process_tables_.end() ? it->second.get() : nullptr;
}

Address VirtualMemoryManager::virtualToPage(Address virtual_address) const
{
    return virtual_address / page_size_;
}

size_t VirtualMemoryManager::allocateFrame()
{
    for (size_t i = 0; i < num_frames_; ++i)
    {
        if (!frame_allocation_[i])
        {
            frame_allocation_[i] = true;
            return i;
        }
    }
    return static_cast<size_t>(-1);
}

void VirtualMemoryManager::freeFrame(Address frame_number)
{
    if (frame_number < num_frames_)
    {
        frame_allocation_[frame_number] = false;
    }
}

void VirtualMemoryManager::resetStats()
{
    page_faults_ = 0;
    page_replacements_ = 0;
    page_accesses_ = 0;
}
