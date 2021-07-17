#pragma once

#include <functional>
#include <random>
#include <string>
#include <vector>
#include <unordered_map>

void bench_robin_table(std::vector<std::string>& data,std::vector<std::string>& probes,
  std::unordered_map<int, std::vector<int>>& locations,
  std::unordered_map<int, std::vector<double>>& entropies);

void bench_google_table(std::vector<std::string>& data,std::vector<std::string>& probes, 
  std::unordered_map<int, std::vector<int>>& locations,
  std::unordered_map<int, std::vector<double>>& entropies);

void bench_swiss_table(std::vector<std::string>& data, std::vector<std::string>& probes,
                            std::unordered_map<int, std::vector<int>>& locations,
  std::unordered_map<int, std::vector<double>>& entropies);

void bench_std_table(std::vector<std::string>& data, std::vector<std::string>& probes,
                          std::unordered_map<int, std::vector<int>>& locations,
  std::unordered_map<int, std::vector<double>>& entropies);

void bench_swiss_table_mt(std::vector<std::string>& data, 
  const std::vector<std::string>& possible_probes, int num_probes,
  double fractSuccessful, int numThreads, 
  std::unordered_map<int, std::vector<int>>& locations,
  std::unordered_map<int, std::vector<double>>& entropies);

/*void bench_f14_table(std::vector<std::string>& data, std::vector<std::string>& probes,
                            std::unordered_map<int, std::vector<int>>& locations,
  std::unordered_map<int, std::vector<int>>& num_locations); */

static const std::unordered_map<std::string,
                         std::function<void(std::vector<std::string>&,
                                            std::vector<std::string>&,
                                            std::unordered_map<int, std::vector<int>>&,
                                            std::unordered_map<int, std::vector<double>>&
                                            )>>
    table_benchmarks{
      {"robin_table", bench_robin_table},
      {"google_table", bench_google_table},
      {"swiss_table", bench_swiss_table},
      {"std_table", bench_std_table},
      //{"f14_table", bench_f14_table}
    };

extern std::unordered_map<std::string, bool> table_metrics;