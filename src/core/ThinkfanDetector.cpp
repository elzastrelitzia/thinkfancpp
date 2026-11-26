#include "core/ThinkfanDetector.h"

#include <string>

namespace thinkfan {

namespace {

bool is_active_output(const std::string& output) {
    std::string trimmed = output;
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) {
        trimmed.pop_back();
    }
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) {
        trimmed.erase(trimmed.begin());
    }
    return trimmed == "active";
}

bool is_inactive_output(const std::string& output) {
    std::string trimmed = output;
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.back()))) {
        trimmed.pop_back();
    }
    while (!trimmed.empty() && std::isspace(static_cast<unsigned char>(trimmed.front()))) {
        trimmed.erase(trimmed.begin());
    }
    // transient states are treated as unknown
    return trimmed == "inactive" || trimmed == "failed";
}

} // namespace

ThinkfanStatus detect_thinkfan(const CommandRunner& runner) {
    ThinkfanStatus status;
    const std::vector<std::string> units = {
        "thinkfan.service",
        "thinkfan",
        "thinkfan-wakeup"
    };

    for (const auto& unit : units) {
        const auto result = runner({"systemctl", "is-active", unit});

        if (result.ok) {
            if (is_active_output(result.output)) {
                status.active = true;
                status.unknown = false;
                status.detail = unit + " is active";
                return status;
            } else if (is_inactive_output(result.output)) {
                status.detail = unit + " is " + result.output;
                continue;
            }
        }

        // command failed or output unexpected
        if (!result.ok || (!is_active_output(result.output) && !is_inactive_output(result.output))) {
            if (!status.active) {
                status.unknown = true;
                status.detail = "systemctl is-active " + unit + " failed: exit_code=" +
                                std::to_string(result.exit_code) + ", error=" + result.error;
            }
        }
    }

    if (!status.active && !status.unknown) {
        status.detail = "No thinkfan units active";
    } else if (!status.active && status.unknown) {
        status.detail = "Thinkfan status uncertain: " + status.detail;
    }

    return status;
}

} // namespace thinkfan
