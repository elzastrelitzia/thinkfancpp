#pragma once

#include <string>
#include <optional>

namespace thinkfan {

struct SensorSnapshot {
    std::optional<double> cpu_temp_c;
    std::optional<int> fan_rpm;
    std::string detail;
};

SensorSnapshot read_sensors(const std::string& proc_fan_path, const std::string& hwmon_root);

} // namespace thinkfan
