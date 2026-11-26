#include <cassert>
#include <filesystem>
#include <fstream>
#include <string>
#include "core/SensorReader.h"

int main() {
    namespace fs = std::filesystem;

    // cpu temp from hwmon, proc_fan missing
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-1";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "52000\n";
        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        assert(*snap.cpu_temp_c == 52.0);
        assert(!snap.fan_rpm.has_value());
        assert(snap.detail.find("cpu_temp=52") != std::string::npos);
        fs::remove_all(dir);
    }

    // fan rpm from procfs preferred over hwmon
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-2";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "45000\n";
        std::ofstream(dir / "fan1_input") << "3000\n"; // ignored when procfs has speed

        const std::string proc_content =
            "status:\t\tenabled\n"
            "speed:\t\t3020\n"
            "level:\t\t2\n";
        auto proc_path = (dir / "proc_fan").string();
        std::ofstream(proc_path) << proc_content;

        auto snap = thinkfan::read_sensors(proc_path, dir.string());
        assert(snap.cpu_temp_c.has_value());
        assert(*snap.cpu_temp_c == 45.0);
        assert(snap.fan_rpm.has_value());
        assert(*snap.fan_rpm == 3020); // from procfs
        assert(snap.detail.find("fan_rpm=3020") != std::string::npos);
        fs::remove_all(dir);
    }

    // fan rpm fallback to hwmon fan1_input
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-3";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "50000\n";
        std::ofstream(dir / "fan1_input") << "2500\n";

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        assert(*snap.cpu_temp_c == 50.0);
        assert(snap.fan_rpm.has_value());
        assert(*snap.fan_rpm == 2500);
        assert(snap.detail.find("fan_rpm=2500") != std::string::npos);
        fs::remove_all(dir);
    }

    // fan rpm fallback to other fan*_input
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-4";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "55000\n";
        std::ofstream(dir / "fan2_input") << "2800\n"; // no fan1_input

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        assert(*snap.cpu_temp_c == 55.0);
        assert(snap.fan_rpm.has_value());
        assert(*snap.fan_rpm == 2800);
        fs::remove_all(dir);
    }

    // missing hwmon directory
    {
        auto snap = thinkfan::read_sensors("/missing/proc", "/nonexistent/hwmon");
        assert(!snap.cpu_temp_c.has_value());
        assert(!snap.fan_rpm.has_value());
        assert(snap.detail.find("unavailable") != std::string::npos);
    }

    // parse error in temp file
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-6";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "not_a_number\n";

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(!snap.cpu_temp_c.has_value());
        fs::remove_all(dir);
    }

    // parse error in fan file
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-7";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "40000\n";
        std::ofstream(dir / "fan1_input") << "bad\n";

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        assert(*snap.cpu_temp_c == 40.0);
        assert(!snap.fan_rpm.has_value());
        fs::remove_all(dir);
    }

    // procfs without speed falls back to hwmon
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-8";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "48000\n";
        std::ofstream(dir / "fan1_input") << "3100\n";

        const std::string proc_content = "status: enabled\nlevel: auto\n"; // no speed
        auto proc_path = (dir / "proc_fan").string();
        std::ofstream(proc_path) << proc_content;

        auto snap = thinkfan::read_sensors(proc_path, dir.string());
        assert(snap.cpu_temp_c.has_value());
        assert(*snap.cpu_temp_c == 48.0);
        assert(snap.fan_rpm.has_value());
        assert(*snap.fan_rpm == 3100);
        fs::remove_all(dir);
    }

    // multiple temp*_input files, no labels: alphabetical fallback
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-9";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "0\n";
        std::ofstream(dir / "temp2_input") << "60000\n";

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        // no labels: picks temp1_input (alphabetically first)
        assert(*snap.cpu_temp_c == 0.0);
        fs::remove_all(dir);
    }

    // prefers label containing cpu/package/core/tdie
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-10";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "30000\n";
        std::ofstream(dir / "temp1_label") << "Random\n";
        std::ofstream(dir / "temp2_input") << "55000\n";
        std::ofstream(dir / "temp2_label") << "CPU Package\n";

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        // prefers label containing "Package"
        assert(*snap.cpu_temp_c == 55.0);
        fs::remove_all(dir);
    }

    // prefers "Core" label
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-11";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "40000\n";
        std::ofstream(dir / "temp1_label") << "Core 0\n";
        std::ofstream(dir / "temp2_input") << "60000\n";
        std::ofstream(dir / "temp2_label") << "GPU\n";

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        assert(*snap.cpu_temp_c == 40.0);
        fs::remove_all(dir);
    }

    // prefers "Tdie" label
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-12";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "45000\n";
        std::ofstream(dir / "temp1_label") << "Tdie\n";
        std::ofstream(dir / "temp2_input") << "50000\n";
        std::ofstream(dir / "temp2_label") << "Something else\n";

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        assert(*snap.cpu_temp_c == 45.0);
        fs::remove_all(dir);
    }

    // no matching labels: alphabetical fallback
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-13";
        fs::create_directories(dir);
        std::ofstream(dir / "temp1_input") << "35000\n";
        std::ofstream(dir / "temp1_label") << "Ambient\n";
        std::ofstream(dir / "temp2_input") << "55000\n";
        std::ofstream(dir / "temp2_label") << "Other\n";

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        // no matching labels: alphabetical (temp1)
        assert(*snap.cpu_temp_c == 35.0);
        fs::remove_all(dir);
    }

    // hwmon* subdirs: cpu temp by label priority
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-14";
        fs::create_directories(dir / "hwmon0");
        fs::create_directories(dir / "hwmon1");
        std::ofstream(dir / "hwmon0" / "temp1_input") << "40000\n";
        std::ofstream(dir / "hwmon0" / "temp1_label") << "GPU\n";
        std::ofstream(dir / "hwmon1" / "temp1_input") << "55000\n";
        std::ofstream(dir / "hwmon1" / "temp1_label") << "CPU Package\n";

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        // prefers label containing "Package"
        assert(*snap.cpu_temp_c == 55.0);
        fs::remove_all(dir);
    }

    // hwmon* subdirs: fan rpm
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-15";
        fs::create_directories(dir / "hwmon0");
        std::ofstream(dir / "hwmon0" / "temp1_input") << "50000\n";
        std::ofstream(dir / "hwmon0" / "fan1_input") << "3000\n";

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        assert(*snap.cpu_temp_c == 50.0);
        assert(snap.fan_rpm.has_value());
        assert(*snap.fan_rpm == 3000);
        fs::remove_all(dir);
    }

    // hwmon* subdirs: temp and fan
    {
        auto dir = fs::temp_directory_path() / "thinkfancpp-test-hwmon-16";
        fs::create_directories(dir / "hwmon0");
        std::ofstream(dir / "hwmon0" / "temp1_input") << "55000\n";
        std::ofstream(dir / "hwmon0" / "temp1_label") << "Core 0\n";
        std::ofstream(dir / "hwmon0" / "fan1_input") << "2800\n";

        auto snap = thinkfan::read_sensors("/missing/proc", dir.string());
        assert(snap.cpu_temp_c.has_value());
        assert(*snap.cpu_temp_c == 55.0);
        assert(snap.fan_rpm.has_value());
        assert(*snap.fan_rpm == 2800);
        fs::remove_all(dir);
    }

    return 0;
}
