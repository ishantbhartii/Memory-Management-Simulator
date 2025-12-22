#include "cli/cli.hpp"
#include "common/utils.hpp"
#include <iomanip>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <functional>
#include "common/colors.hpp"

using namespace std;

CLI::CLI(IntegratedMemorySystem &system)
    : memory_system_(system), running_(false), current_process_(-1)
{
    registerCommands();
}

void CLI::run()
{
    running_ = true;
    cout << "=== Memory Management Simulator CLI ===" << endl;
    cout << "Type 'help' for available commands or 'quit' to exit." << endl;

    string input;
    while (running_)
    {
        printPrompt();
        getline(cin, input);

        if (!input.empty())
        {
            executeCommand(input);
        }
    }
}

void CLI::registerCommands()
{
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
    commands_["color"] = {
        "color",
        "Toggle colored output: on | off",
        bind(&CLI::handleColor, this, _1)};

    commands_["setproc"] = {"setproc", "Set current process context", bind(&CLI::handleSetProcess, this, _1)};
    commands_["help"] = {"help", "Display help information", bind(&CLI::handleHelp, this, _1)};
    commands_["quit"] = {"quit", "Exit the simulator", bind(&CLI::handleQuit, this, _1)};
    commands_["mode"] = {
        "mode",
        "Set allocation mode: auto | buddy | physical | forced",
        bind(&CLI::handleAllocatorMode, this, _1)};
}

bool CLI::executeCommand(const string &input)
{
    auto args = parseInput(input);
    if (args.empty())
        return false;

    string cmd = args[0];
    args.erase(args.begin());

    auto it = commands_.find(cmd);
    if (it != commands_.end())
    {
        return it->second.handler(args);
    }

    cout << "Unknown command: " << cmd << endl;
    return false;
}
bool CLI::handleColor(const vector<string> &args)
{
    if (args.size() != 1)
    {
        cout << "Usage: color on | off\n";
        return false;
    }

    if (args[0] == "on")
    {
        Color::enabled = true;
        cout << "Color output enabled\n";
    }
    else if (args[0] == "off")
    {
        Color::enabled = false;
        cout << "Color output disabled\n";
    }
    else
    {
        cout << "Usage: color on | off\n";
        return false;
    }

    return true;
}

bool CLI::handleAllocatorMode(const vector<string> &args)
{
    if (args.size() != 1)
    {
        cout << "Usage: allocator auto | buddy | physical | forced\n";
        return false;
    }

    if (args[0] == "auto")
    {
        memory_system_.setAllocationMode(AllocationMode::AUTO);
        cout << "[INFO] Allocation mode set to AUTO\n";
    }
    else if (args[0] == "buddy")
    {
        memory_system_.setAllocationMode(AllocationMode::BUDDY);
        cout << "[INFO] Allocation mode set to BUDDY\n";
    }
    else if (args[0] == "physical")
    {
        memory_system_.setAllocationMode(AllocationMode::PHYSICAL);
        cout << "[INFO] Allocation mode set to PHYSICAL\n";
    }
    else if (args[0] == "forced")
    {
        memory_system_.setAllocationMode(AllocationMode::FORCED);
        cout << "[INFO] Allocation mode set to FORCED\n";
    }
    else
    {
        cout << "Unknown mode. Use auto | buddy | physical | forced\n";
        return false;
    }
    return true;
}

vector<string> CLI::parseInput(const string &input)
{
    vector<string> args;
    stringstream ss(input);
    string token;

    while (ss >> token)
    {
        args.push_back(token);
    }

    return args;
}
static string allocModeToString(AllocationMode mode)
{
    switch (mode)
    {
    case AllocationMode::AUTO:
        return "AUTO";
    case AllocationMode::BUDDY:
        return "BUDDY";
    case AllocationMode::PHYSICAL:
        return "PHYSICAL";
    case AllocationMode::FORCED:
        return "FORCED";
    }
    return "UNKNOWN";
}

static string pagePolicyToString(PageReplacementPolicy policy)
{
    switch (policy)
    {
    case PageReplacementPolicy::FIFO:
        return "FIFO";
    case PageReplacementPolicy::LRU:
        return "LRU";
    case PageReplacementPolicy::CLOCK:
        return "CLOCK";
    }
    return "UNKNOWN";
}

void CLI::printPrompt() const
{
    string proc =
        (current_process_ >= 0)
            ? "P" + to_string(current_process_)
            : "NO-PROC";

    string allocMode =
        allocModeToString(memory_system_.getAllocationMode());

    string pagePolicy =
        pagePolicyToString(memory_system_.getPageReplacementPolicy());

    cout << Color::cyan()
         << "memsim["
         << proc << " | "
         << allocMode << " | "
         << pagePolicy
         << "]> "
         << Color::reset();
}

bool CLI::handleInit(const vector<string> &args)
{
    if (!memory_system_.initialize())
    {
        cout << "Failed to initialize memory system" << endl;
        return false;
    }

    cout << "Memory system initialized successfully" << endl;
    cout << "Total memory: " << formatSize(memory_system_.getTotalMemory()) << endl;
    cout << "Page size: " << formatSize(memory_system_.getPageSize()) << endl;
    return true;
}

bool CLI::handleCreateProcess(const vector<string> &args)
{
    if (!memory_system_.isInitialized())
    {
        cout << "Error: system not initialized. Run 'init' first." << endl;
        return false;
    }

    if (args.size() != 1)
        return false;
    ProcessId pid = parseProcessId(args[0]);
    if (pid < 0)
        return false;

    return memory_system_.createProcess(pid);
}

bool CLI::handleTerminateProcess(const vector<string> &args)
{
    if (!memory_system_.isInitialized())
    {
        cout << "Error: system not initialized. Run 'init' first." << endl;
        return false;
    }
    if (args.size() != 1)
        return false;

    ProcessId pid = parseProcessId(args[0]);
    if (memory_system_.terminateProcess(pid))
    {
        if (current_process_ == pid)
            current_process_ = -1;
        return true;
    }

    return false;
}

bool CLI::handleAllocate(const vector<string> &args)
{
    if (!memory_system_.isInitialized())
    {
        cout << "Error: system not initialized. Run 'init' first." << endl;
        return false;
    }

    ProcessId pid = current_process_;
    Size size = 0;
    if (pid < 0)
    {
        cout << "Error: no process selected. Use 'create' and 'setproc'." << endl;
        return false;
    }

    if (args.size() == 1)
    {
        size = parseSize(args[0]);
    }
    else if (args.size() == 2)
    {
        pid = parseProcessId(args[0]);
        size = parseSize(args[1]);
    }
    else
    {
        return false;
    }

    if (size == 0)
        return false;

    auto result = memory_system_.allocateMemory(pid, size);
    if (!result.success)
    {
        cout << "Allocation failed. Did you create the process?" << endl;
    }
    return result.success;
}

bool CLI::handleDeallocate(const vector<string> &args)
{
    if (!memory_system_.isInitialized())
    {
        cout << "Error: system not initialized. Run 'init' first." << endl;
        return false;
    }

    if (args.size() != 2)
        return false;

    ProcessId pid = parseProcessId(args[0]);
    Address addr = parseAddress(args[1]);

    if (!memory_system_.deallocateMemory(pid, addr))
    {
        cout << "Free failed: invalid address or permission denied" << endl;
        return false;
    }
    return true;
}

bool CLI::handleAccess(const vector<string> &args)
{
    if (!memory_system_.isInitialized())
    {
        cout << "Error: system not initialized. Run 'init' first." << endl;
        return false;
    }

    ProcessId pid = current_process_;
    Address addr = 0;
    bool is_write = false;

    if (args.size() == 1)
    {
        if (pid < 0)
            return false;
        addr = parseAddress(args[0]);
    }
    else if (args.size() == 2)
    {
        pid = parseProcessId(args[0]);
        addr = parseAddress(args[1]);
    }
    else if (args.size() == 3 && args[2] == "write")
    {
        pid = parseProcessId(args[0]);
        addr = parseAddress(args[1]);
        is_write = true;
    }
    else
    {
        return false;
    }

    return memory_system_.accessMemory(pid, addr, is_write);
}

bool CLI::handleDump(const vector<string> &args)
{
    if (!memory_system_.isInitialized())
    {
        cout << "Error: system not initialized. Run 'init' first." << endl;
        return false;
    }

    if (!args.empty() && args[0] == "bar")
    {
        memory_system_.printMemoryBar();
    }
    else
    {
        memory_system_.printMemoryDump();
    }

    return true;
}

bool CLI::handleStats(const vector<string> &args)
{
    cout << Color::cyan()
         << "\n================ SYSTEM STATISTICS ================\n\n"
         << Color::reset();

    cout << "Total Operations        : "
         << memory_system_.getTotalOperations() << "\n";

    // ---------------- Physical Allocator ----------------
    auto phys = memory_system_.getPhysicalAllocatorStats();
    double phys_frag = phys.fragmentation_ratio * 100;

    cout << Color::blue() << "\n[Physical Allocator]\n"
         << Color::reset();
    cout << "  Used Memory           : " << formatSize(phys.used_memory) << "\n";
    cout << "  Free Memory           : " << formatSize(phys.free_memory) << "\n";
    cout << "  External Fragmentation: "
         << (phys_frag > 30 ? Color::red() : phys_frag > 10 ? Color::yellow()
                                                            : Color::green())
         << phys_frag << " %"
         << Color::reset() << "\n";
    cout << "  Requests              : " << phys.allocation_requests << "\n";
    cout << "  Success / Failure     : "
         << phys.allocation_successes << " / "
         << phys.allocation_failures << "\n";
    cout << "  Utilization           : "
         << phys.memory_utilization * 100 << " %\n";

    // ---------------- Buddy Allocator ----------------
    auto buddy = memory_system_.getBuddyAllocatorStats();
    cout << Color::blue() << "\n[Buddy Allocator]\n"
         << Color::reset();
    cout << "  Used Memory           : " << formatSize(buddy.used_memory) << "\n";
    cout << "  Free Memory           : " << formatSize(buddy.free_memory) << "\n";
    cout << "  Internal Fragmentation: "
         << Color::yellow()
         << formatSize(buddy.internal_fragmentation)
         << Color::reset() << "\n";
    cout << "  Requests              : " << buddy.allocation_requests << "\n";
    cout << "  Success / Failure     : "
         << buddy.allocation_successes << " / "
         << buddy.allocation_failures << "\n";
    cout << "  Utilization           : "
         << buddy.memory_utilization * 100 << " %\n";

    // ---------------- Virtual Memory ----------------
    auto vmm = memory_system_.getVMMStats();
    double pf_rate = vmm.page_fault_rate * 100;

    cout << Color::blue() << "\n[Virtual Memory]\n"
         << Color::reset();
    cout << "  Page Faults           : "
         << Color::red() << vmm.page_faults << Color::reset() << "\n";
    cout << "  Page Replacements     : " << vmm.page_replacements << "\n";
    cout << "  Page Fault Rate       : "
         << (pf_rate > 30 ? Color::red() : pf_rate > 10 ? Color::yellow()
                                                        : Color::green())
         << pf_rate << " %"
         << Color::reset() << "\n";
    cout << "  Free Frames           : "
         << vmm.free_frames << " / " << vmm.total_frames << "\n";

    // ---------------- Cache Hierarchy ----------------
    auto cache = memory_system_.getCacheStats();
    cout << Color::blue() << "\n[CACHE HIERARCHY]\n"
         << Color::reset();

    auto printCache = [&](const string &name, const Cache::CacheStats &s)
    {
        size_t accesses = s.hits + s.misses;
        double hit_ratio = accesses ? (double)s.hits / accesses * 100 : 0.0;

        cout << "  " << name << "\n";
        cout << "    Hits / Misses       : "
             << s.hits << " / " << s.misses << "\n";
        cout << "    Hit Ratio           : "
             << (hit_ratio >= 70 ? Color::green() : hit_ratio >= 30 ? Color::yellow()
                                                                    : Color::red())
             << hit_ratio << " %"
             << Color::reset() << "\n";
    };

    printCache("L1 Cache", cache.l1_stats);
    printCache("L2 Cache", cache.l2_stats);
    printCache("L3 Cache", cache.l3_stats);

    cout << "  Main Memory Accesses  : "
         << cache.main_memory_accesses << "\n";
    cout << "  AMAT                  : "
         << cache.avg_memory_access_time << " cycles\n";

    cout << "\n==================================================\n";
    return true;
}

bool CLI::handleSwitchStrategy(const vector<string> &args)
{
    if (args.size() != 1)
        return false;

    AllocationStrategy strategy = parseAllocationStrategy(args[0]);
    memory_system_.switchAllocationStrategy(strategy);
    return true;
}

bool CLI::handleSwitchPagePolicy(const vector<string> &args)
{
    if (args.size() != 1)
        return false;

    PageReplacementPolicy policy = parsePageReplacementPolicy(args[0]);
    memory_system_.switchPageReplacementPolicy(policy);
    return true;
}

bool CLI::handleTest(const vector<string> &args)
{
    string test_name = args.empty() ? "default" : args[0];
    memory_system_.runMemoryTest(test_name);
    return true;
}

bool CLI::handleBenchmark(const vector<string> &args)
{
    if (args.empty() || args[0] == "alloc")
    {
        memory_system_.benchmarkAllocationStrategies();
    }
    else if (args[0] == "cache")
    {
        memory_system_.benchmarkCachePerformance();
    }
    else
    {
        return false;
    }
    return true;
}

bool CLI::handleProcessInfo(const vector<string> &args)
{
    ProcessId pid = current_process_;
    if (!args.empty())
        pid = parseProcessId(args[0]);
    if (!memory_system_.hasProcess(pid))
    {
        cout << "Error: process does not exist." << endl;
        return false;
    }

    if (pid < 0)
        return false;

    memory_system_.printProcessInfo(pid);
    return true;
}

bool CLI::handleSetProcess(const vector<string> &args)
{
    if (!memory_system_.isInitialized())
    {
        cout << "Error: system not initialized. Run 'init' first." << endl;
        return false;
    }

    if (args.size() != 1)
        return false;

    ProcessId pid = parseProcessId(args[0]);
    if (!memory_system_.hasProcess(pid))
    {
        cout << "Error: process does not exist." << endl;
        return false;
    }

    current_process_ = pid;
    return true;
}

bool CLI::handleHelp(const vector<string> &args)
{
    printHelp();
    return true;
}

bool CLI::handleQuit(const vector<string> &args)
{
    running_ = false;
    return true;
}

void CLI::printHelp() const
{
    using Entry = pair<string, string>;

    auto section = [&](const string &title, const vector<Entry> &cmds)
    {
        cout << Color::cyan() << "\n"
             << title << "\n"
             << Color::reset();
        for (const auto &c : cmds)
        {
            cout << "  "
                 << left << setw(20) << c.first
                 << c.second << "\n";
        }
    };

    cout << Color::cyan()
         << "\n================ AVAILABLE COMMANDS ================\n"
         << Color::reset();

    section("System", {{"init", "Initialize memory system"},
                       {"quit", "Exit simulator"},
                       {"help", "Show this help"}});

    section("Process", {{"create <pid>", "Create a new process"},
                        {"setproc <pid>", "Set current process"},
                        {"terminate <pid>", "Terminate a process"},
                        {"process [pid]", "Show process information"}});

    section("Memory Allocation", {{"alloc <size>", "Allocate memory (B / KB / MB)"},
                                  {"free <pid> <addr>", "Free allocated memory"},
                                  {"mode <auto|buddy|physical|forced>", "Set allocation mode"},
                                  {"strategy <first|best|worst>", "Set physical allocation strategy"}});

    section("Virtual Memory", {{"access <addr> [write]", "Access virtual address"},
                               {"policy <fifo|lru|clock>", "Set page replacement policy"}});

    section("Inspection", {{"dump", "Dump physical memory layout"},
                           {"stats", "Show system statistics"},
                           {"bench [alloc|cache]", "Run benchmarks"},
                           {"test [name]", "Run memory tests"}});

    section("UI / UX", {{"color <on|off>", "Toggle colored output"}});

    cout << Color::cyan()
         << "\n====================================================\n"
         << Color::reset();
}

ProcessId CLI::parseProcessId(const string &str) const
{
    try
    {
        return stoi(str);
    }
    catch (...)
    {
        return -1;
    }
}

Address CLI::parseAddress(const string &str) const
{
    if (str.substr(0, 2) == "0x")
    {
        return stoul(str.substr(2), nullptr, 16);
    }
    try
    {
        return stoul(str);
    }
    catch (...)
    {
        return 0;
    }
}

Size CLI::parseSize(const string &str) const
{
    string s = str;
    for (auto &c : s)
        c = tolower(c);

    Size multiplier = 1;

    if (s.size() >= 2 && s.substr(s.size() - 2) == "kb")
    {
        multiplier = 1024;
        s = s.substr(0, s.size() - 2);
    }
    else if (s.size() >= 2 && s.substr(s.size() - 2) == "mb")
    {
        multiplier = 1024 * 1024;
        s = s.substr(0, s.size() - 2);
    }
    else if (s.back() == 'b')
    {
        s.pop_back();
    }

    try
    {
        return stoull(s) * multiplier;
    }
    catch (...)
    {
        return 0;
    }
}

AllocationStrategy CLI::parseAllocationStrategy(const string &str) const
{
    if (str == "first")
        return AllocationStrategy::FIRST_FIT;
    if (str == "best")
        return AllocationStrategy::BEST_FIT;
    if (str == "worst")
        return AllocationStrategy::WORST_FIT;
    return AllocationStrategy::FIRST_FIT;
}

PageReplacementPolicy CLI::parsePageReplacementPolicy(const string &str) const
{
    if (str == "fifo")
        return PageReplacementPolicy::FIFO;
    if (str == "lru")
        return PageReplacementPolicy::LRU;
    if (str == "clock")
        return PageReplacementPolicy::CLOCK;
    return PageReplacementPolicy::LRU;
}
