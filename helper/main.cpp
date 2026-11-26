#include "core/HelperArgs.h"
#include "core/ProcFanParser.h"

#include <fstream>
#include <iostream>
#include <string>
#include <cerrno>
#include <cstring>

namespace {

bool verify_write(const std::string& procfs_line) {
    constexpr const char* kProcFanPath = "/proc/acpi/ibm/fan";
    std::ifstream fan_file(kProcFanPath);
    if (!fan_file.is_open()) {
        return false;
    }
    std::string content((std::istreambuf_iterator<char>(fan_file)), std::istreambuf_iterator<char>());
    auto state = thinkfan::parse_proc_fan(content);

    if (procfs_line.rfind("level ", 0) == 0) {
        std::string expected_level = procfs_line.substr(6);
        return state.readable && state.level == expected_level;
    } else if (procfs_line == "enable") {
        return state.readable && state.enabled;
    } else if (procfs_line == "disable") {
        return state.readable && !state.enabled;
    }
    return false;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    constexpr const char* kProcFanPath = "/proc/acpi/ibm/fan";

    auto cmd = thinkfan::parse_helper_args(argc, argv);
    if (!cmd.has_value()) {
        std::cerr << "Usage: thinkfancpp-helper --set-level <level> | --enable | --disable\n";
        std::cerr << "Allowed levels: auto, 0-7, full-speed, disengaged\n";
        return 1;
    }

    std::ofstream fan_file(kProcFanPath);
    if (!fan_file.is_open()) {
        std::cerr << "Failed to open " << kProcFanPath << " for writing: " << std::strerror(errno) << "\n";
        return 2;
    }

    fan_file << cmd->procfs_line << '\n';
    if (!fan_file) {
        std::cerr << "Failed to write to " << kProcFanPath << ": " << std::strerror(errno) << "\n";
        return 3;
    }

    // flush before verification so kernel sees the command
    fan_file.flush();
    if (!fan_file) {
        std::cerr << "Failed to flush " << kProcFanPath << ": " << std::strerror(errno) << "\n";
        return 3;
    }

    if (!verify_write(cmd->procfs_line)) {
        std::cerr << "Kernel rejected command: " << cmd->procfs_line << "\n";
        return 4;
    }

    return 0;
}
