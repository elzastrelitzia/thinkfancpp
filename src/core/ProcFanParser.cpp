#include "core/ProcFanParser.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string_view>

namespace thinkfan {

namespace {

std::string_view trim(std::string_view sv) {
    auto start = sv.find_first_not_of(" \t\r\n");
    if (start == std::string_view::npos) {
        return {};
    }
    auto end = sv.find_last_not_of(" \t\r\n");
    return sv.substr(start, end - start + 1);
}

} // anonymous namespace

ProcFanState parse_proc_fan(std::string_view content) {
    ProcFanState state;
    
    if (content.empty()) {
        state.readable = false;
        state.error = "Empty input";
        return state;
    }
    
    std::istringstream iss{std::string{content}};
    std::string line;
    bool found_any_field = false;
    
    while (std::getline(iss, line)) {
        std::string_view line_sv(line);
        line_sv = trim(line_sv);
        
        if (line_sv.empty()) {
            continue;
        }
        
        auto colon_pos = line_sv.find(':');
        if (colon_pos == std::string_view::npos) {
            continue;
        }
        
        std::string_view key = trim(line_sv.substr(0, colon_pos));
        std::string_view value = trim(line_sv.substr(colon_pos + 1));
        
        if (key == "status") {
            found_any_field = true;
            if (value == "enabled") {
                state.enabled = true;
            } else if (value == "disabled") {
                state.enabled = false;
            } else {
                state.error = "Unrecognized status: " + std::string(value);
            }
        } else if (key == "speed") {
            found_any_field = true;
            try {
                state.speed_rpm = std::stoi(std::string(value));
            } catch (...) {
                state.speed_rpm = -1;
            }
        } else if (key == "level") {
            found_any_field = true;
            state.level = std::string(value);
        }
    }
    
    state.readable = found_any_field;
    if (!found_any_field) {
        state.error = "No valid fields found";
    }
    
    return state;
}

std::optional<std::string> format_level_command(std::string_view level) {
    if (!is_allowed_level(level)) {
        return std::nullopt;
    }
    return "level " + std::string(level);
}

} // namespace thinkfan
