#include <cassert>
#include <string>
#include "core/ProcFanParser.h"

int main() {
    // original case
    {
        const std::string sample =
            "status:\t\tenabled\n"
            "speed:\t\t3020\n"
            "level:\t\t2\n";
        auto state = thinkfan::parse_proc_fan(sample);
        assert(state.readable);
        assert(state.enabled);
        assert(state.speed_rpm == 3020);
        assert(state.level == "2");

        auto cmd = thinkfan::format_level_command("full-speed");
        assert(cmd.has_value());
        assert(*cmd == "level full-speed");
        assert(!thinkfan::format_level_command("9").has_value());
        assert(!thinkfan::format_level_command("auto;rm -rf /").has_value());
    }

    // parse_proc_fan edge cases

    // empty input
    {
        auto state = thinkfan::parse_proc_fan("");
        assert(!state.readable);
        assert(!state.error.empty());
        assert(state.enabled == false);
        assert(state.speed_rpm == -1);
        assert(state.level == "unknown");
    }

    // whitespace only
    {
        auto state = thinkfan::parse_proc_fan("   \t\n  \r\n");
        assert(!state.readable);
        assert(!state.error.empty());
    }

    // partial: status only
    {
        auto state = thinkfan::parse_proc_fan("status: enabled");
        assert(state.readable);
        assert(state.enabled);
        assert(state.speed_rpm == -1);
        assert(state.level == "unknown");
    }

    // partial: speed only
    {
        auto state = thinkfan::parse_proc_fan("speed: 2500");
        assert(state.readable);
        assert(!state.enabled); // default
        assert(state.speed_rpm == 2500);
        assert(state.level == "unknown");
    }

    // partial: level only
    {
        auto state = thinkfan::parse_proc_fan("level: 3");
        assert(state.readable);
        assert(!state.enabled);
        assert(state.speed_rpm == -1);
        assert(state.level == "3");
    }

    // whitespace variations
    {
        const std::string sample = "status : enabled \n speed : 3000 \n level : auto ";
        auto state = thinkfan::parse_proc_fan(sample);
        assert(state.readable);
        assert(state.enabled);
        assert(state.speed_rpm == 3000);
        assert(state.level == "auto");
    }

    // tabs and spaces
    {
        const std::string sample = "status:\t\tenabled\nspeed:\t\t3020\nlevel:\t\tdisengaged";
        auto state = thinkfan::parse_proc_fan(sample);
        assert(state.readable);
        assert(state.enabled);
        assert(state.speed_rpm == 3020);
        assert(state.level == "disengaged");
    }

    // invalid speed (non-numeric)
    {
        auto state = thinkfan::parse_proc_fan("speed: not_a_number");
        assert(state.readable);
        assert(state.speed_rpm == -1); // parse failure sets -1
    }

    // invalid speed (negative)
    {
        auto state = thinkfan::parse_proc_fan("speed: -100");
        assert(state.readable);
        assert(state.speed_rpm == -100); // stoi parses negative, we accept it
    }

    // unrecognized status: readable, error set, enabled false
    {
        auto state = thinkfan::parse_proc_fan("status: broken\nspeed: 1000\nlevel: 1");
        assert(state.readable);
        assert(!state.enabled); // default for unrecognized
        assert(state.speed_rpm == 1000);
        assert(state.level == "1");
        assert(!state.error.empty());
        assert(state.error.find("Unrecognized status") != std::string::npos);
    }

    // unknown status only
    {
        auto state = thinkfan::parse_proc_fan("status: unknown");
        assert(state.readable);
        assert(!state.enabled);
        assert(!state.error.empty());
    }

    // status values are case-sensitive
    {
        auto state = thinkfan::parse_proc_fan("status: ENABLED");
        assert(state.readable);
        assert(!state.enabled); // not "enabled"
        assert(!state.error.empty());
    }

    // extra fields ignored
    {
        auto state = thinkfan::parse_proc_fan("status: enabled\nspeed: 2000\nlevel: 2\nextra: ignored");
        assert(state.readable);
        assert(state.enabled);
        assert(state.speed_rpm == 2000);
        assert(state.level == "2");
    }

    // format_level_command edge cases

    // all allowlisted levels
    const char* allowed[] = {"auto", "0", "1", "2", "3", "4", "5", "6", "7", "full-speed", "disengaged"};
    for (const char* lvl : allowed) {
        auto cmd = thinkfan::format_level_command(lvl);
        assert(cmd.has_value());
        assert(*cmd == "level " + std::string(lvl));
    }

    // injection attempts rejected
    const char* injections[] = {"9", "auto;rm -rf /", "manual", "", "8", "full", "speed", "disengage", "auto ", " auto"};
    for (const char* inj : injections) {
        auto cmd = thinkfan::format_level_command(inj);
        assert(!cmd.has_value());
    }

    // "Auto" rejected (case-sensitive)
    {
        auto cmd = thinkfan::format_level_command("Auto");
        assert(!cmd.has_value());
    }

    // "FULL-SPEED" rejected (case-sensitive)
    {
        auto cmd = thinkfan::format_level_command("FULL-SPEED");
        assert(!cmd.has_value());
    }

    return 0;
}
