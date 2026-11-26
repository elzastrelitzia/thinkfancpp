#pragma once

#include <string>

namespace thinkfan {

enum class FanMode {
    Auto,
    Manual,
    Max
};

struct ProcFanState {
    bool enabled{false};
    int speed_rpm{-1};
    std::string level{"unknown"};
    bool readable{false};
    std::string error;
};

struct CommandResult {
    bool ok{false};
    std::string output;
    std::string error;
    int exit_code{-1};
};

} // namespace thinkfan