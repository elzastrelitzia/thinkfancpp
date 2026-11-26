#include "core/SensorReader.h"
#include "core/ProcFanParser.h"

#include <fstream>
#include <filesystem>
#include <string>
#include <optional>
#include <algorithm>
#include <vector>
#include <cctype>

namespace thinkfan {

namespace {
namespace fs = std::filesystem;

std::optional<int> parse_int_file(const fs::path& path) {
    std::ifstream file(path);
    if (!file) {
        return std::nullopt;
    }
    std::string line;
    if (!std::getline(file, line)) {
        return std::nullopt;
    }
    try {
        size_t start = line.find_first_not_of(" \t\r\n");
        size_t end = line.find_last_not_of(" \t\r\n");
        if (start == std::string::npos) {
            return std::nullopt;
        }
        line = line.substr(start, end - start + 1);
        return std::stoi(line);
    } catch (...) {
        return std::nullopt;
    }
}

struct TempSensor {
    fs::path input_path;
    std::string label;
    int priority = 0; // Higher = more preferred
};

void collect_temp_sensors(const fs::path& dir, std::vector<TempSensor>& temp_sensors) {
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return;
    }
    for (const auto& entry : fs::directory_iterator(dir)) {
        const auto filename = entry.path().filename().string();
        if (filename.rfind("temp", 0) == 0 && filename.size() > 4 && filename.substr(filename.size() - 6) == "_input") {
            TempSensor sensor;
            sensor.input_path = entry.path();

            // read matching temp*_label
            std::string label_filename = filename.substr(0, filename.size() - 6) + "_label";
            fs::path label_path = entry.path().parent_path() / label_filename;
            std::ifstream label_file(label_path);
            if (label_file) {
                std::string label;
                std::getline(label_file, label);
                // trim whitespace
                size_t start = label.find_first_not_of(" \t\r\n");
                size_t end = label.find_last_not_of(" \t\r\n");
                if (start != std::string::npos) {
                    label = label.substr(start, end - start + 1);
                }
                sensor.label = label;

                // prefer cpu/package/core/tdie labels
                std::string label_lower = label;
                std::transform(label_lower.begin(), label_lower.end(), label_lower.begin(),
                               [](unsigned char c) { return std::tolower(c); });
                if (label_lower.find("cpu") != std::string::npos ||
                    label_lower.find("package") != std::string::npos ||
                    label_lower.find("core") != std::string::npos ||
                    label_lower.find("tdie") != std::string::npos) {
                    sensor.priority = 10;
                } else if (label_lower.find("temp") != std::string::npos) {
                    sensor.priority = 5;
                }
            }
            temp_sensors.push_back(std::move(sensor));
        }
    }
}

std::optional<double> read_cpu_temp_c(const std::string& hwmon_root) {
    fs::path root(hwmon_root);
    if (!fs::exists(root) || !fs::is_directory(root)) {
        return std::nullopt;
    }

    std::vector<TempSensor> temp_sensors;

    // scan hwmon_root directly for test layouts
    collect_temp_sensors(root, temp_sensors);

    // scan hwmon* subdirectories for real sysfs layout
    for (const auto& entry : fs::directory_iterator(root)) {
        if (entry.is_directory()) {
            const auto dirname = entry.path().filename().string();
            if (dirname.rfind("hwmon", 0) == 0) {
                collect_temp_sensors(entry.path(), temp_sensors);
            }
        }
    }

    // sort by priority, then filename for determinism
    std::sort(temp_sensors.begin(), temp_sensors.end(),
              [](const TempSensor& a, const TempSensor& b) {
                  if (a.priority != b.priority) {
                      return a.priority > b.priority;
                  }
                  return a.input_path.filename().string() < b.input_path.filename().string();
              });

    for (const auto& sensor : temp_sensors) {
        auto value = parse_int_file(sensor.input_path);
        if (value.has_value()) {
            // millidegrees c to c
            return static_cast<double>(*value) / 1000.0;
        }
    }
    return std::nullopt;
}

std::optional<int> read_fan_rpm_proc(const std::string& proc_fan_path) {
    std::ifstream file(proc_fan_path);
    if (!file) {
        return std::nullopt;
    }
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    auto state = parse_proc_fan(content);
    if (state.readable && state.speed_rpm >= 0) {
        return state.speed_rpm;
    }
    return std::nullopt;
}

void collect_fan_files(const fs::path& dir, std::vector<fs::path>& fan_files) {
    if (!fs::exists(dir) || !fs::is_directory(dir)) {
        return;
    }
    for (const auto& entry : fs::directory_iterator(dir)) {
        const auto filename = entry.path().filename().string();
        if (filename.rfind("fan", 0) == 0 && filename.size() > 3 && filename.substr(filename.size() - 6) == "_input") {
            fan_files.push_back(entry.path());
        }
    }
}

std::optional<int> read_fan_rpm_hwmon(const std::string& hwmon_root) {
    fs::path root(hwmon_root);
    if (!fs::exists(root) || !fs::is_directory(root)) {
        return std::nullopt;
    }

    // collect and sort fan*_input files for deterministic order
    std::vector<fs::path> fan_files;

    // scan hwmon_root directly for test layouts
    collect_fan_files(root, fan_files);

    // scan hwmon* subdirectories for real sysfs layout
    for (const auto& entry : fs::directory_iterator(root)) {
        if (entry.is_directory()) {
            const auto dirname = entry.path().filename().string();
            if (dirname.rfind("hwmon", 0) == 0) {
                collect_fan_files(entry.path(), fan_files);
            }
        }
    }

    std::sort(fan_files.begin(), fan_files.end(),
              [](const fs::path& a, const fs::path& b) {
                  return a.filename().string() < b.filename().string();
              });

    // prefer fan1_input, then first available
    std::optional<int> fan1_value = std::nullopt;
    std::optional<int> other_fan_value = std::nullopt;

    for (const auto& path : fan_files) {
        const auto filename = path.filename().string();
        auto value = parse_int_file(path);
        if (value.has_value()) {
            if (filename == "fan1_input") {
                fan1_value = value;
            } else if (!other_fan_value.has_value()) {
                other_fan_value = value;
            }
        }
    }

    if (fan1_value.has_value()) {
        return fan1_value;
    }
    return other_fan_value;
}

} // namespace

SensorSnapshot read_sensors(const std::string& proc_fan_path, const std::string& hwmon_root) {
    SensorSnapshot snapshot;

    snapshot.cpu_temp_c = read_cpu_temp_c(hwmon_root);

    // prefer procfs rpm, fall back to hwmon
    auto rpm_proc = read_fan_rpm_proc(proc_fan_path);
    if (rpm_proc.has_value()) {
        snapshot.fan_rpm = rpm_proc;
    } else {
        snapshot.fan_rpm = read_fan_rpm_hwmon(hwmon_root);
    }

    std::string details;
    if (snapshot.cpu_temp_c.has_value()) {
        details += "cpu_temp=" + std::to_string(*snapshot.cpu_temp_c) + "C ";
    } else {
        details += "cpu_temp=unavailable ";
    }
    if (snapshot.fan_rpm.has_value()) {
        details += "fan_rpm=" + std::to_string(*snapshot.fan_rpm);
    } else {
        details += "fan_rpm=unavailable";
    }
    snapshot.detail = details;

    return snapshot;
}

} // namespace thinkfan
