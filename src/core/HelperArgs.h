#pragma once

#include <string>
#include <optional>

namespace thinkfan {

struct HelperCommand {
    std::string procfs_line;
    bool check_only = false;
    bool daemon = false;
};

std::optional<HelperCommand> parse_helper_args(int argc, char* const argv[]);

} // namespace thinkfan
