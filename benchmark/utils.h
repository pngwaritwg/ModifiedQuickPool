#pragma once

#include <nlohmann/json.hpp>
#include <NTL/BasicThreadPool.h>
#include <NTL/ZZ_p.h>
#include <NTL/ZZ_pE.h>
#include <NTL/ZZ_pX.h>

#include "netmp.h"

struct TimePoint {
  using timepoint_t = std::chrono::high_resolution_clock::time_point;
  using timeunit_t = std::chrono::duration<double, std::milli>;

  TimePoint();
  double operator-(const TimePoint& rhs) const;

  timepoint_t time;
};

struct CommPoint {
  std::vector<uint64_t> stats;

  CommPoint(io::NetIOMP& network);
  std::vector<uint64_t> operator-(const CommPoint& rhs) const;
};

class StatsPoint {
  TimePoint tpoint_;
  CommPoint cpoint_;

 public:
  StatsPoint(io::NetIOMP& network);
  nlohmann::json operator-(const StatsPoint& rhs);
};

bool saveJson(const nlohmann::json& data, const std::string& fpath);
void initNTL(size_t num_threads);
int64_t peakVirtualMemory();
int64_t peakResidentSetSize();
