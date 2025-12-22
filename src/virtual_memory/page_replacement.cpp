#include "../include/virtual_memory/page_table.hpp"
#include "../include/virtual_memory/vmm.hpp"
#include <list>
#include <unordered_set>

class ClockAlgorithm {
private:
    std::list<std::pair<ProcessId, Address>> clock_list_;
    std::unordered_map<
        ProcessId,
        std::unordered_map<
            Address,
            std::list<std::pair<ProcessId, Address>>::iterator
        >
    > clock_map_;

public:
    void accessPage(ProcessId process_id, Address virtual_page) {
        auto& proc_map = clock_map_[process_id];
        auto it = proc_map.find(virtual_page);
        if (it != proc_map.end()) {
            clock_list_.splice(clock_list_.begin(), clock_list_, it->second);
        } else {
            auto key = std::make_pair(process_id, virtual_page);
            clock_list_.push_front(key);
            proc_map[virtual_page] = clock_list_.begin();
        }
    }

    std::pair<ProcessId, Address> selectVictim() {
        if (clock_list_.empty()) {
            return std::make_pair(-1, 0);
        }

        for (auto it = clock_list_.rbegin(); it != clock_list_.rend(); ++it) {
            auto victim = *it;
            clock_list_.erase(std::next(it).base());
            clock_map_[victim.first].erase(victim.second);
            if (clock_map_[victim.first].empty()) {
                clock_map_.erase(victim.first);
            }
            return victim;
        }

        auto victim = clock_list_.back();
        clock_list_.pop_back();
        clock_map_[victim.first].erase(victim.second);
        if (clock_map_[victim.first].empty()) {
            clock_map_.erase(victim.first);
        }
        return victim;
    }

    void removePage(ProcessId process_id, Address virtual_page) {
        auto proc_it = clock_map_.find(process_id);
        if (proc_it != clock_map_.end()) {
            auto& proc_map = proc_it->second;
            auto page_it = proc_map.find(virtual_page);
            if (page_it != proc_map.end()) {
                clock_list_.erase(page_it->second);
                proc_map.erase(page_it);
                if (proc_map.empty()) {
                    clock_map_.erase(proc_it);
                }
            }
        }
    }
};

class EnhancedLRU {
private:
    std::unordered_map<ProcessId, std::unordered_map<Address, size_t>> access_times_;
    size_t current_time_;

public:
    EnhancedLRU() : current_time_(0) {}

    void accessPage(ProcessId process_id, Address virtual_page) {
        access_times_[process_id][virtual_page] = current_time_++;
    }

    std::pair<ProcessId, Address> selectVictim() {
        if (access_times_.empty()) {
            return std::make_pair(-1, 0);
        }

        auto victim = std::make_pair(-1, Address(0));
        size_t oldest_time = SIZE_MAX;

        for (const auto& proc_pair : access_times_) {
            for (const auto& page_pair : proc_pair.second) {
                if (page_pair.second < oldest_time) {
                    oldest_time = page_pair.second;
                    victim = std::make_pair(proc_pair.first, page_pair.first);
                }
            }
        }

        access_times_[victim.first].erase(victim.second);
        if (access_times_[victim.first].empty()) {
            access_times_.erase(victim.first);
        }
        return victim;
    }

    void removePage(ProcessId process_id, Address virtual_page) {
        auto proc_it = access_times_.find(process_id);
        if (proc_it != access_times_.end()) {
            proc_it->second.erase(virtual_page);
            if (proc_it->second.empty()) {
                access_times_.erase(proc_it);
            }
        }
    }
};

class LFUAlgorithm {
private:
    std::unordered_map<ProcessId, std::unordered_map<Address, size_t>> access_counts_;
    std::unordered_map<size_t, std::list<std::pair<ProcessId, Address>>> frequency_lists_;
    std::unordered_map<
        ProcessId,
        std::unordered_map<
            Address,
            std::list<std::pair<ProcessId, Address>>::iterator
        >
    > position_map_;

public:
    void accessPage(ProcessId process_id, Address virtual_page) {
        auto& proc_counts = access_counts_[process_id];
        auto& proc_positions = position_map_[process_id];

        auto count_it = proc_counts.find(virtual_page);
        size_t old_freq = (count_it != proc_counts.end()) ? count_it->second : 0;
        size_t new_freq = old_freq + 1;

        if (old_freq > 0) {
            auto& old_list = frequency_lists_[old_freq];
            old_list.erase(proc_positions[virtual_page]);
            if (old_list.empty()) {
                frequency_lists_.erase(old_freq);
            }
        }

        auto key = std::make_pair(process_id, virtual_page);
        frequency_lists_[new_freq].push_front(key);
        proc_positions[virtual_page] = frequency_lists_[new_freq].begin();
        proc_counts[virtual_page] = new_freq;
    }

    std::pair<ProcessId, Address> selectVictim() {
        if (frequency_lists_.empty()) {
            return std::make_pair(-1, 0);
        }

        auto lowest_freq_it = frequency_lists_.begin();
        for (auto it = frequency_lists_.begin(); it != frequency_lists_.end(); ++it) {
            if (it->first < lowest_freq_it->first) {
                lowest_freq_it = it;
            }
        }

        auto victim = lowest_freq_it->second.back();
        lowest_freq_it->second.pop_back();

        if (lowest_freq_it->second.empty()) {
            frequency_lists_.erase(lowest_freq_it);
        }

        access_counts_[victim.first].erase(victim.second);
        if (access_counts_[victim.first].empty()) {
            access_counts_.erase(victim.first);
        }

        position_map_[victim.first].erase(victim.second);
        if (position_map_[victim.first].empty()) {
            position_map_.erase(victim.first);
        }

        return victim;
    }

    void removePage(ProcessId process_id, Address virtual_page) {
        auto proc_counts_it = access_counts_.find(process_id);
        if (proc_counts_it != access_counts_.end()) {
            auto& proc_counts = proc_counts_it->second;
            auto count_it = proc_counts.find(virtual_page);
            if (count_it != proc_counts.end()) {
                size_t freq = count_it->second;
                auto& freq_list = frequency_lists_[freq];
                freq_list.erase(position_map_[process_id][virtual_page]);

                if (freq_list.empty()) {
                    frequency_lists_.erase(freq);
                }

                proc_counts.erase(count_it);
                if (proc_counts.empty()) {
                    access_counts_.erase(proc_counts_it);
                }

                position_map_[process_id].erase(virtual_page);
                if (position_map_[process_id].empty()) {
                    position_map_.erase(process_id);
                }
            }
        }
    }
};
