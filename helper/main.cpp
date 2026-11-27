#include "core/HelperArgs.h"
#include "core/ProcFanParser.h"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

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
        if (!state.readable) {
            return false;
        }
        if (state.level == expected_level) {
            return true;
        }
        const auto is_max_level = [](const std::string& level) {
            return level == "full-speed" || level == "disengaged" || level == "7";
        };
        return is_max_level(expected_level) && is_max_level(state.level);
    }
    if (procfs_line == "enable") {
        return state.readable && state.enabled;
    }
    if (procfs_line == "disable") {
        return state.readable && !state.enabled;
    }
    return false;
}

bool execute_command(const std::string& procfs_line, std::string& error) {
    constexpr const char* kProcFanPath = "/proc/acpi/ibm/fan";
    std::ofstream fan_file(kProcFanPath);
    if (!fan_file.is_open()) {
        error = std::string("Failed to open ") + kProcFanPath + " for writing: " + std::strerror(errno);
        return false;
    }

    fan_file << procfs_line << '\n';
    if (!fan_file) {
        error = std::string("Failed to write to ") + kProcFanPath + ": " + std::strerror(errno);
        return false;
    }

    // flush before verification so kernel sees the command
    fan_file.flush();
    if (!fan_file) {
        error = std::string("Failed to flush ") + kProcFanPath + ": " + std::strerror(errno);
        return false;
    }

    if (!verify_write(procfs_line)) {
        error = "Kernel rejected command: " + procfs_line;
        return false;
    }
    return true;
}

std::vector<std::string> split_args(const std::string& line) {
    std::istringstream iss(line);
    std::vector<std::string> args;
    std::string arg;
    while (iss >> arg) {
        args.push_back(arg);
    }
    return args;
}

int run_daemon() {
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) {
            continue;
        }
        if (line == "quit") {
            std::cout << "ok\n" << std::flush;
            return 0;
        }

        auto parts = split_args(line);
        std::vector<char*> argv;
        argv.reserve(parts.size() + 1);
        static char prog[] = "thinkfancpp-helper";
        argv.push_back(prog);
        for (auto& part : parts) {
            argv.push_back(part.data());
        }

        auto cmd = thinkfan::parse_helper_args(static_cast<int>(argv.size()), argv.data());
        if (!cmd.has_value() || cmd->check_only || cmd->daemon) {
            std::cout << "err invalid command\n" << std::flush;
            continue;
        }

        std::string error;
        if (execute_command(cmd->procfs_line, error)) {
            std::cout << "ok\n" << std::flush;
        } else {
            std::cout << "err " << error << "\n" << std::flush;
        }
    }
    return 0;
}

} // anonymous namespace

int main(int argc, char* argv[]) {
    constexpr const char* kProcFanPath = "/proc/acpi/ibm/fan";

    auto cmd = thinkfan::parse_helper_args(argc, argv);
    if (!cmd.has_value()) {
        std::cerr << "Usage: thinkfancpp-helper --daemon | --check | --set-level <level> | --enable | --disable\n";
        std::cerr << "Allowed levels: auto, 0-7, full-speed, disengaged\n";
        return 1;
    }

    if (cmd->check_only) {
        std::ifstream fan_file(kProcFanPath);
        if (!fan_file.is_open()) {
            std::cerr << "Failed to open " << kProcFanPath << " for reading: " << std::strerror(errno) << "\n";
            return 2;
        }
        return 0;
    }

    if (cmd->daemon) {
        return run_daemon();
    }

    std::string error;
    if (!execute_command(cmd->procfs_line, error)) {
        std::cerr << error << "\n";
        if (error.rfind("Failed to open", 0) == 0) {
            return 2;
        }
        if (error.rfind("Failed to write", 0) == 0 || error.rfind("Failed to flush", 0) == 0) {
            return 3;
        }
        return 4;
    }

    return 0;
}
