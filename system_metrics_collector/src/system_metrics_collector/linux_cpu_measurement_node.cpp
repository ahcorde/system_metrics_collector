// Copyright 2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <chrono>
#include <string>

#include "linux_cpu_measurement_node.hpp"

namespace
{
constexpr const char PROC_STAT_FILE[] = "/proc/stat";
constexpr const char CPU_LABEL[] = "cpu";
constexpr const size_t CPU_LABEL_LENGTH = 3;
}  // namespace

ProcCpuData processLine(const std::string & stat_cpu_line)
{
  ProcCpuData parsed_data;

  if (!stat_cpu_line.empty()) {
    if (!stat_cpu_line.compare(0, CPU_LABEL_LENGTH, CPU_LABEL)) {
      std::istringstream ss(stat_cpu_line);

      if (!ss.good()) {
        return ProcCpuData();
      }
      ss >> parsed_data.cpu_label;

      for (int i = 0; i < static_cast<int>(ProcCpuStates::kNumProcCpuStates); ++i) {
        if (!ss.good()) {
          return ProcCpuData();
        }
        ss >> parsed_data.times[i];
      }

      return parsed_data;
    }
  }
  return parsed_data;
}

double computeCpuActivePercentage(
  const ProcCpuData & measurement1,
  const ProcCpuData & measurement2)
{
  if (measurement1.isMeasurementEmpty() || measurement2.isMeasurementEmpty()) {
    return std::nan("");
  }

  const double active_time = measurement2.getActiveTime() - measurement1.getActiveTime();
  const double total_time = (measurement2.getIdleTime() + measurement2.getActiveTime()) -
    (measurement1.getIdleTime() + measurement2.getActiveTime());

  return 100.0 * active_time / total_time;
}

LinuxCpuMeasurementNode::LinuxCpuMeasurementNode(
  const std::string & name,
  const std::chrono::milliseconds measurement_period,
  const std::string & topic,
  const std::chrono::milliseconds & publish_period)
: PeriodicMeasurementNode(name, measurement_period, topic, publish_period),
  last_measurement_()
{}

double LinuxCpuMeasurementNode::periodicMeasurement()
{
  ProcCpuData current_measurement = makeSingleMeasurement();

  const double cpu_percentage = computeCpuActivePercentage(
    last_measurement_,
    current_measurement);

  last_measurement_ = current_measurement;

  return cpu_percentage;
}

ProcCpuData LinuxCpuMeasurementNode::makeSingleMeasurement()
{
  std::ifstream stat_file(PROC_STAT_FILE);
  std::string line;
  std::getline(stat_file, line);

  return stat_file.good() ? processLine(line) : ProcCpuData();
}