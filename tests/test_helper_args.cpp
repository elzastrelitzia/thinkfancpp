#include <cassert>
#include <cstring>
#include <string>
#include "core/HelperArgs.h"

int main() {
    char prog[] = "thinkfancpp-helper";

    // valid --set-level values
    {
        char flag[] = "--set-level";
        char val[] = "auto";
        char* argv[] = {prog, flag, val};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(cmd.has_value());
        assert(cmd->procfs_line == "level auto");
    }
    for (int i = 0; i <= 7; ++i) {
        char flag[] = "--set-level";
        char val[2] = {static_cast<char>('0' + i), '\0'};
        char* argv[] = {prog, flag, val};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(cmd.has_value());
        assert(cmd->procfs_line == "level " + std::string(val));
    }
    {
        char flag[] = "--set-level";
        char val[] = "full-speed";
        char* argv[] = {prog, flag, val};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(cmd.has_value());
        assert(cmd->procfs_line == "level full-speed");
    }
    {
        char flag[] = "--set-level";
        char val[] = "disengaged";
        char* argv[] = {prog, flag, val};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(cmd.has_value());
        assert(cmd->procfs_line == "level disengaged");
    }

    // --enable
    {
        char flag[] = "--enable";
        char* argv[] = {prog, flag};
        auto cmd = thinkfan::parse_helper_args(2, argv);
        assert(cmd.has_value());
        assert(cmd->procfs_line == "enable");
        assert(!cmd->check_only);
    }

    // --disable
    {
        char flag[] = "--disable";
        char* argv[] = {prog, flag};
        auto cmd = thinkfan::parse_helper_args(2, argv);
        assert(cmd.has_value());
        assert(cmd->procfs_line == "disable");
        assert(!cmd->check_only);
    }

    // --check
    {
        char flag[] = "--check";
        char* argv[] = {prog, flag};
        auto cmd = thinkfan::parse_helper_args(2, argv);
        assert(cmd.has_value());
        assert(cmd->check_only);
        assert(!cmd->daemon);
    }

    // --daemon
    {
        char flag[] = "--daemon";
        char* argv[] = {prog, flag};
        auto cmd = thinkfan::parse_helper_args(2, argv);
        assert(cmd.has_value());
        assert(cmd->daemon);
        assert(!cmd->check_only);
    }

    // extra args for --check
    {
        char flag[] = "--check";
        char extra[] = "extra";
        char* argv[] = {prog, flag, extra};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(!cmd.has_value());
    }

    // extra args for --daemon
    {
        char flag[] = "--daemon";
        char extra[] = "extra";
        char* argv[] = {prog, flag, extra};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(!cmd.has_value());
    }

    // invalid level (out of range)
    {
        char flag[] = "--set-level";
        char val[] = "9";
        char* argv[] = {prog, flag, val};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(!cmd.has_value());
    }
    {
        char flag[] = "--set-level";
        char val[] = "8";
        char* argv[] = {prog, flag, val};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(!cmd.has_value());
    }
    {
        char flag[] = "--set-level";
        char val[] = "-1";
        char* argv[] = {prog, flag, val};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(!cmd.has_value());
    }
    {
        char flag[] = "--set-level";
        char val[] = "invalid";
        char* argv[] = {prog, flag, val};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(!cmd.has_value());
    }

    // unknown flag
    {
        char flag[] = "--unknown";
        char* argv[] = {prog, flag};
        auto cmd = thinkfan::parse_helper_args(2, argv);
        assert(!cmd.has_value());
    }
    {
        char flag[] = "--set-level";
        char val[] = "auto";
        char extra[] = "extra";
        char* argv[] = {prog, flag, val, extra};
        auto cmd = thinkfan::parse_helper_args(4, argv);
        assert(!cmd.has_value());
    }

    // missing value for --set-level
    {
        char flag[] = "--set-level";
        char* argv[] = {prog, flag};
        auto cmd = thinkfan::parse_helper_args(2, argv);
        assert(!cmd.has_value());
    }

    // extra args for --enable
    {
        char flag[] = "--enable";
        char extra[] = "extra";
        char* argv[] = {prog, flag, extra};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(!cmd.has_value());
    }

    // extra args for --disable
    {
        char flag[] = "--disable";
        char extra[] = "extra";
        char* argv[] = {prog, flag, extra};
        auto cmd = thinkfan::parse_helper_args(3, argv);
        assert(!cmd.has_value());
    }

    // no args
    {
        char* argv[] = {prog};
        auto cmd = thinkfan::parse_helper_args(1, argv);
        assert(!cmd.has_value());
    }

    // allowlist consistency with ProcFanParser
    {
        const char* allowed[] = {"auto", "0", "1", "2", "3", "4", "5", "6", "7", "full-speed", "disengaged"};
        for (const char* lvl : allowed) {
            char flag[] = "--set-level";
            char val[16];
            std::strcpy(val, lvl);
            char* argv[] = {prog, flag, val};
            auto cmd = thinkfan::parse_helper_args(3, argv);
            assert(cmd.has_value());
            assert(cmd->procfs_line == "level " + std::string(lvl));
        }
    }

    // invalid levels rejected (matching ProcFanParser)
    {
        const char* invalid[] = {"9", "8", "-1", "invalid", "Auto", "FULL-SPEED", "disengage", "auto ", " auto"};
        for (const char* lvl : invalid) {
            char flag[] = "--set-level";
            char val[16];
            std::strcpy(val, lvl);
            char* argv[] = {prog, flag, val};
            auto cmd = thinkfan::parse_helper_args(3, argv);
            assert(!cmd.has_value());
        }
    }

    return 0;
}
