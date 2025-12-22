#ifndef UTILS_HPP
#define UTILS_HPP

#include <vector>
#include <string>
#include <sstream>
#include <iomanip>

#include "common/types.hpp"

using namespace std;

inline bool isPowerOfTwo(Size n) {
    return n > 0 && ((n & (n - 1)) == 0);
}

inline Size nextPowerOfTwo(Size n) {
    if (n == 0) return 1;
    n--;
    n |= n >> 1;
    n |= n >> 2;
    n |= n >> 4;
    n |= n >> 8;
    n |= n >> 16;
    n++;
    return n;
}

inline int log2Floor(Size n) {
    int log = 0;
    while (n >>= 1) log++;
    return log;
}

inline vector<string> splitString(const string& str, char delimiter) {
    vector<string> tokens;
    stringstream ss(str);
    string token;
    while (getline(ss, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

inline string formatAddress(Address addr) {
    stringstream ss;
    ss << "0x" << hex << setw(8) << setfill('0') << addr;
    return ss.str();
}

inline string formatSize(Size size) {
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    double displaySize = static_cast<double>(size);

    while (displaySize >= 1024.0 && unitIndex < 3) {
        displaySize /= 1024.0;
        unitIndex++;
    }

    stringstream ss;
    ss << fixed << setprecision(2) << displaySize << " " << units[unitIndex];
    return ss.str();
}

inline double calculateFragmentation(const vector<MemoryBlock>& blocks, Size totalMemory) {
    Size totalFree = 0;
    Size largestFree = 0;

    for (const auto& block : blocks) {
        if (block.status == BlockStatus::FREE) {
            totalFree += block.size;
            if (block.size > largestFree) {
                largestFree = block.size;
            }
        }
    }

    if (totalFree == 0) return 0.0;
    return static_cast<double>(totalFree - largestFree) / totalFree;
}

inline bool isValidAddressRange(Address start, Size size, Size totalMemory) {
    return start < totalMemory && start + size <= totalMemory;
}

inline BlockId generateBlockId() {
    static BlockId nextId = 0;
    return nextId++;
}

inline string trimString(const string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == string::npos) return "";
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, last - first + 1);
}

#endif
