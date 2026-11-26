#include "core/HelperArgs.h"
#include "core/ProcFanParser.h"

#include <string_view>
#include <optional>

namespace thinkfan {

namespace {

std::optional<std::string> parse_set_level(int argc, char* const argv[]) {
    if (argc != 3) {
        return std::nullopt;
    }
    std::string_view flag(argv[1]);
    if (flag != "--set-level") {
        return std::nullopt;
    }
    std::string_view level(argv[2]);
    if (!is_allowed_level(level)) {
        return std::nullopt;
    }
    return "level " + std::string(level);
}

std::optional<std::string> parse_enable(int argc, char* const argv[]) {
    if (argc != 2) {
        return std::nullopt;
    }
    std::string_view flag(argv[1]);
    if (flag != "--enable") {
        return std::nullopt;
    }
    return "enable";
}

std::optional<std::string> parse_disable(int argc, char* const argv[]) {
    if (argc != 2) {
        return std::nullopt;
    }
    std::string_view flag(argv[1]);
    if (flag != "--disable") {
        return std::nullopt;
    }
    return "disable";
}

bool parse_check(int argc, char* const argv[]) {
    return argc == 2 && std::string_view(argv[1]) == "--check";
}

bool parse_daemon(int argc, char* const argv[]) {
    return argc == 2 && std::string_view(argv[1]) == "--daemon";
}

} // anonymous namespace

std::optional<HelperCommand> parse_helper_args(int argc, char* const argv[]) {
    if (argc < 2) {
        return std::nullopt;
    }

    if (parse_check(argc, argv)) {
        return HelperCommand{"", true, false};
    }

    if (parse_daemon(argc, argv)) {
        return HelperCommand{"", false, true};
    }

    std::optional<std::string> command_line;

    if (std::string_view(argv[1]) == "--set-level") {
        command_line = parse_set_level(argc, argv);
    } else if (std::string_view(argv[1]) == "--enable") {
        command_line = parse_enable(argc, argv);
    } else if (std::string_view(argv[1]) == "--disable") {
        command_line = parse_disable(argc, argv);
    } else {
        return std::nullopt;
    }

    if (!command_line.has_value()) {
        return std::nullopt;
    }

    return HelperCommand{*command_line, false};
}

} // namespace thinkfan
