#pragma once

#include <string>
#include <string_view>
#include <optional>
#include <array>
#include <algorithm>

#include "core/FanTypes.h"

namespace thinkfan {

// single source of truth for allowed fan levels
constexpr std::array<std::string_view, 11> kAllowedLevels = {
    "auto", "0", "1", "2", "3", "4", "5", "6", "7", "full-speed", "disengaged"
};

inline bool is_allowed_level(std::string_view level) {
    return std::find(kAllowedLevels.begin(), kAllowedLevels.end(), level) != kAllowedLevels.end();
}

ProcFanState parse_proc_fan(std::string_view content);
std::optional<std::string> format_level_command(std::string_view level);

} // namespace thinkfan
