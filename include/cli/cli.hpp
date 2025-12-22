#ifndef CLI_HPP
#define CLI_HPP

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>
#include "integration/integrated_system.hpp"

using namespace std;

struct Command {
    string name;
    string description;
    function<bool(const vector<string>&)> handler;
};

class CLI {
private:
    IntegratedMemorySystem& memory_system_;
    unordered_map<string, Command> commands_;
    bool running_;
    ProcessId current_process_;

public:
    CLI(IntegratedMemorySystem& system);
    ~CLI() = default;

    void run();
    void registerCommands();
    bool executeCommand(const string& input);

    vector<string> parseInput(const string& input);
    void printHelp() const;
    void printPrompt() const;

private:
    bool handleInit(const vector<string>& args);
    bool handleCreateProcess(const vector<string>& args);
    bool handleTerminateProcess(const vector<string>& args);
    bool handleAllocate(const vector<string>& args);
    bool handleDeallocate(const vector<string>& args);
    bool handleAccess(const vector<string>& args);
    bool handleDump(const vector<string>& args);
    bool handleStats(const vector<string>& args);
    bool handleSwitchStrategy(const vector<string>& args);
    bool handleSwitchPagePolicy(const vector<string>& args);
    bool handleTest(const vector<string>& args);
    bool handleBenchmark(const vector<string>& args);
    bool handleProcessInfo(const vector<string>& args);
    bool handleSetProcess(const vector<string>& args);
    bool handleHelp(const vector<string>& args);
    bool handleQuit(const vector<string>& args);

    ProcessId parseProcessId(const string& str) const;
    Address parseAddress(const string& str) const;
    Size parseSize(const string& str) const;
    AllocationStrategy parseAllocationStrategy(const string& str) const;
    PageReplacementPolicy parsePageReplacementPolicy(const string& str) const;
};

#endif
