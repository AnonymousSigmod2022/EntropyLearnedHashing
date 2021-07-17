#pragma once

#include <functional>
#include <random>
#include <string>
#include <vector>
#include <unordered_map>

// buckets must be a power of 2. 
void bench_partitioners(std::vector<std::string>& data, 
  std::unordered_map<int, std::vector<int>>& locations,
  std::unordered_map<int, std::vector<double>>& entropies, const int numPartitions);

static const std::unordered_map<std::string,
                         std::function<void(std::vector<std::string>&,
                                            std::unordered_map<int, std::vector<int>>&,
                                            std::unordered_map<int, std::vector<double>>&,
                                            int 
                                            )>>
    partition_benchmarks{
      {"partition", bench_partitioners},
    };