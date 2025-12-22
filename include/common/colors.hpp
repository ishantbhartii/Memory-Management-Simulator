#pragma once
#include <string>
namespace Color
{
    inline bool enabled = true;

    inline std::string reset()   { return enabled ? "\033[0m"  : ""; }
    inline std::string red()     { return enabled ? "\033[31m" : ""; }
    inline std::string green()   { return enabled ? "\033[32m" : ""; }
    inline std::string yellow()  { return enabled ? "\033[33m" : ""; }
    inline std::string blue()    { return enabled ? "\033[34m" : ""; }
    inline std::string cyan()    { return enabled ? "\033[36m" : ""; }
}
