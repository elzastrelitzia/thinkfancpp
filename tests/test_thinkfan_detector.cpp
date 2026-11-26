#include <cassert>
#include <string>
#include "core/ThinkfanDetector.h"

int main() {
    // thinkfan.service active
    {
        auto status = thinkfan::detect_thinkfan([](const auto& args) {
            (void)args;
            return thinkfan::CommandResult{true, "active\n", "", 0};
        });
        assert(status.active);
        assert(!status.unknown);
        assert(status.detail.find("thinkfan.service") != std::string::npos);
    }

    // thinkfan.service inactive, thinkfan active
    {
        auto status = thinkfan::detect_thinkfan([](const auto& args) {
            (void)args;
            static int call = 0;
            if (call == 0) {
                ++call;
                return thinkfan::CommandResult{true, "inactive\n", "", 0};
            }
            return thinkfan::CommandResult{true, "active\n", "", 0};
        });
        assert(status.active);
        assert(!status.unknown);
        assert(status.detail.find("thinkfan") != std::string::npos);
    }

    // all units inactive
    {
        auto status = thinkfan::detect_thinkfan([](const auto& args) {
            (void)args;
            return thinkfan::CommandResult{true, "inactive\n", "", 0};
        });
        assert(!status.active);
        assert(!status.unknown);
        assert(status.detail.find("No thinkfan units active") != std::string::npos);
    }

    // systemctl command failed
    {
        auto status = thinkfan::detect_thinkfan([](const auto& args) {
            (void)args;
            return thinkfan::CommandResult{false, "", "command not found", 127};
        });
        assert(!status.active);
        assert(status.unknown);
        assert(status.detail.find("failed") != std::string::npos);
    }

    // unexpected output
    {
        auto status = thinkfan::detect_thinkfan([](const auto& args) {
            (void)args;
            return thinkfan::CommandResult{true, "activating\n", "", 0};
        });
        assert(!status.active);
        assert(status.unknown);
        assert(status.detail.find("Thinkfan status uncertain") != std::string::npos);
    }

    // first unit fails, second succeeds
    {
        auto status = thinkfan::detect_thinkfan([](const auto& args) {
            (void)args;
            static int call = 0;
            if (call == 0) {
                ++call;
                return thinkfan::CommandResult{false, "", "permission denied", 1};
            }
            return thinkfan::CommandResult{true, "active\n", "", 0};
        });
        assert(status.active);
        assert(!status.unknown); // second call succeeded
    }

    // thinkfan-wakeup unit check
    {
        auto status = thinkfan::detect_thinkfan([](const auto& args) {
            (void)args;
            static int call = 0;
            if (call == 0) { ++call; return thinkfan::CommandResult{true, "inactive\n", "", 0}; }
            if (call == 1) { ++call; return thinkfan::CommandResult{true, "inactive\n", "", 0}; }
            return thinkfan::CommandResult{true, "active\n", "", 0};
        });
        assert(status.active);
        assert(!status.unknown);
        assert(status.detail.find("thinkfan-wakeup") != std::string::npos);
    }

    return 0;
}
