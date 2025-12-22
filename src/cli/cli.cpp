#include "cli/cli.hpp"
#include "common/utils.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>

using namespace std;

CLI::CLI(IntegratedMemorySystem& system)
    : memory_system_(system), running_(false), current_process_(-1) {
    registerCommands();
}

void CLI::run() {
    running_ = true;
    cout << "=== Memory Management Simulator CLI ===" << endl;
    cout << "Type 'help' for available commands or 'quit' to exit." << endl;

    string input;
    while (running_) {
        printPrompt();
        getline(cin, input);

        if (!input.empty()) {
            executeCommand(input);
        }
    }
}

void CLI::registerCommands() {
    using namespace std::placeholders;

    commands_["init"] = {"init", "Initialize the memory system", bind(&CLI::handleInit, this, _1)};
    commands_["create"] = {"create", "Create a new process", bind(&CLI::handleCreateProcess, this, _1)};
    commands_["terminate"] = {"terminate", "Terminate a process", bind(&CLI::handleTerminateProcess, this, _1)};
    commands_["alloc"] = {"alloc", "Allocate memory", bind(&CLI::handleAllocate, this, _1)};
    commands_["free"] = {"free", "Deallocate memory", bind(&CLI::handleDeallocate, this, _1)};
    commands_["access"] = {"access", "Access memory location", bind(&CLI::handleAccess, this, _1)};
    commands_["dump"] = {"dump", "Display memory dump", bind(&CLI::handleDump, this, _1)};
    commands_["stats"] = {"stats", "Display system statistics", bind(&CLI::handleStats, this, _1)};
    commands_["strategy"] = {"strategy", "Switch allocation strategy", bind(&CLI::handleSwitchStrategy, this, _1)};
    commands_["policy"] = {"policy", "Switch page replacement policy", bind(&CLI::handleSwitchPagePolicy, this, _1)};
    commands_["test"] = {"test", "Run memory test", bind(&CLI::handleTest, this, _1)};
    commands_["bench"] = {"bench", "Run benchmarks", bind(&CLI::handleBenchmark, this, _1)};
    commands_["process"] = {"process", "Display process information", bind(&CLI::handleProcessInfo, this, _1)};
    commands_["setproc"] = {"setproc", "Set current process context", bind(&CLI::handleSetProcess, this, _1)};
    commands_["help"] = {"help", "Display help information", bind(&CLI::handleHelp, this, _1)};
    commands_["quit"] = {"quit", "Exit the simulator", bind(&CLI::handleQuit, this, _1)};
}

bool CLI::executeCommand(const string& input) {
    auto args = parseInput(input);
    if (args.empty()) return false;

    string cmd = args[0];
    args.erase(args.begin());

    auto it = commands_.find(cmd);
    if (it != commands_.end()) {
        return it->second.handler(args);
    }

    cout << "Unknown command: " << cmd << endl;
    return false;
}

vector<string> CLI::parseInput(const string& input) {
    vector<string> args;
    stringstream ss(input);
    string token;

    while (ss >> token) {
        args.push_back(token);
    }

    return args;
}

void CLI::printPrompt() const {
    if (current_process_ >= 0) {
        cout << "memsim(P" << current_process_ << ")> ";
    } else {
        cout << "memsim> ";
    }
}

bool CLI::handleInit(const vector<string>& args) {
    if (!memory_system_.initialize()) {
        cout << "Failed to initialize memory system" << endl;
        return false;
    }

    cout << "Memory system initialized successfully" << endl;
    cout << "Total memory: " << formatSize(memory_system_.getTotalMemory()) << endl;
    cout << "Page size: " << formatSize(memory_system_.getPageSize()) << endl;
    return true;
}

bool CLI::handleCreateProcess(const vector<string>& args) {
    if (args.size() != 1) return false;

    ProcessId pid = parseProcessId(args[0]);
    if (pid < 0) return false;

    return memory_system_.createProcess(pid);
}

bool CLI::handleTerminateProcess(const vector<string>& args) {
    if (args.size() != 1) return false;

    ProcessId pid = parseProcessId(args[0]);
    if (memory_system_.terminateProcess(pid)) {
        if (current_process_ == pid) current_process_ = -1;
        return true;
    }

    return false;
}

bool CLI::handleAllocate(const vector<string>& args) {
    ProcessId pid = current_process_;
    Size size = 0;

    if (args.size() == 1) {
        if (pid < 0) return false;
        size = parseSize(args[0]);
    } else if (args.size() == 2) {
        pid = parseProcessId(args[0]);
        size = parseSize(args[1]);
    } else {
        return false;
    }

    if (size == 0) return false;

    auto result = memory_system_.allocateMemory(pid, size);
    return result.success;
}

bool CLI::handleDeallocate(const vector<string>& args) {
    if (args.size() != 2) return false;

    ProcessId pid = parseProcessId(args[0]);
    Address addr = parseAddress(args[1]);

    return memory_system_.deallocateMemory(pid, addr);
}

bool CLI::handleAccess(const vector<string>& args) {
    ProcessId pid = current_process_;
    Address addr = 0;
    bool is_write = false;

    if (args.size() == 1) {
        if (pid < 0) return false;
        addr = parseAddress(args[0]);
    } else if (args.size() == 2) {
        pid = parseProcessId(args[0]);
        addr = parseAddress(args[1]);
    } else if (args.size() == 3 && args[2] == "write") {
        pid = parseProcessId(args[0]);
        addr = parseAddress(args[1]);
        is_write = true;
    } else {
        return false;
    }

    return memory_system_.accessMemory(pid, addr, is_write);
}

bool CLI::handleDump(const vector<string>& args) {
    memory_system_.printMemoryDump();
    return true;
}

bool CLI::handleStats(const vector<string>& args) {
    memory_system_.printStatistics();
    return true;
}

bool CLI::handleSwitchStrategy(const vector<string>& args) {
    if (args.size() != 1) return false;

    AllocationStrategy strategy = parseAllocationStrategy(args[0]);
    memory_system_.switchAllocationStrategy(strategy);
    return true;
}

bool CLI::handleSwitchPagePolicy(const vector<string>& args) {
    if (args.size() != 1) return false;

    PageReplacementPolicy policy = parsePageReplacementPolicy(args[0]);
    memory_system_.switchPageReplacementPolicy(policy);
    return true;
}

bool CLI::handleTest(const vector<string>& args) {
    string test_name = args.empty() ? "default" : args[0];
    memory_system_.runMemoryTest(test_name);
    return true;
}

bool CLI::handleBenchmark(const vector<string>& args) {
    if (args.empty() || args[0] == "alloc") {
        memory_system_.benchmarkAllocationStrategies();
    } else if (args[0] == "cache") {
        memory_system_.benchmarkCachePerformance();
    } else {
        return false;
    }
    return true;
}

bool CLI::handleProcessInfo(const vector<string>& args) {
    ProcessId pid = current_process_;
    if (!args.empty()) pid = parseProcessId(args[0]);
    if (pid < 0) return false;

    memory_system_.printProcessInfo(pid);
    return true;
}

bool CLI::handleSetProcess(const vector<string>& args) {
    if (args.size() != 1) return false;

    current_process_ = parseProcessId(args[0]);
    return true;
}

bool CLI::handleHelp(const vector<string>& args) {
    printHelp();
    return true;
}

bool CLI::handleQuit(const vector<string>& args) {
    running_ = false;
    return true;
}

void CLI::printHelp() const {
    for (const auto& pair : commands_) {
        cout << pair.first << " - " << pair.second.description << endl;
    }
}

ProcessId CLI::parseProcessId(const string& str) const {
    try { return stoi(str); } catch (...) { return -1; }
}

Address CLI::parseAddress(const string& str) const {
    if (str.substr(0, 2) == "0x") {
        return stoul(str.substr(2), nullptr, 16);
    }
    try { return stoul(str); } catch (...) { return 0; }
}

Size CLI::parseSize(const string& str) const {
    return parseAddress(str);
}

AllocationStrategy CLI::parseAllocationStrategy(const string& str) const {
    if (str == "first") return AllocationStrategy::FIRST_FIT;
    if (str == "best") return AllocationStrategy::BEST_FIT;
    if (str == "worst") return AllocationStrategy::WORST_FIT;
    return AllocationStrategy::FIRST_FIT;
}

PageReplacementPolicy CLI::parsePageReplacementPolicy(const string& str) const {
    if (str == "fifo") return PageReplacementPolicy::FIFO;
    if (str == "lru") return PageReplacementPolicy::LRU;
    if (str == "clock") return PageReplacementPolicy::CLOCK;
    return PageReplacementPolicy::LRU;
}
