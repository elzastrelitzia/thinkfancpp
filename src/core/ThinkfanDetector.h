#pragma once

#include <string>
#include <vector>
#include <functional>

#include "core/FanTypes.h"

namespace thinkfan {

using CommandRunner = std::function<CommandResult(const std::vector<std::string>&)>;

struct ThinkfanStatus {
    bool active{false};
    bool unknown{false};
    std::string detail;
};

ThinkfanStatus detect_thinkfan(const CommandRunner& runner);

} // namespace thinkfan
