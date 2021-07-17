#pragma once

#include <functional>
#include <random>
#include <string>
#include <vector>
#include <unordered_map>

void bench_selfbloom(std::vector<std::string>& data,std::vector<std::string>& probes,
  std::unordered_map<int, std::vector<int>>& locations,
  std::unordered_map<int, std::vector<double>>& entropies, int numUnsuccessful);

// void bench_libbloom(std::vector<std::string>& data,std::vector<std::string>& probes, 
//   std::unordered_map<int, std::vector<int>>& locations,
//   std::unordered_map<int, std::vector<int>>& num_locations, int numUnsuccessful);

void bench_register_block_bloom(std::vector<std::string>& data, std::vector<std::string>& probes, 
                 std::unordered_map<int, std::vector<int>>& locations, std::unordered_map<int, std::vector<double>>& entropies, 
                int numProbesUnsuccessful);

void bench_blockbloom(std::vector<std::string>& data, std::vector<std::string>& probes, 
  std::unordered_map<int, std::vector<int>>& locations,
  std::unordered_map<int, std::vector<double>>& entropies, int numUnsuccessful);

static const std::unordered_map<std::string,
                         std::function<void(std::vector<std::string>&,
                                            std::vector<std::string>&,
                                            std::unordered_map<int, std::vector<int>>&,
                                            std::unordered_map<int, std::vector<double>>&,
                                            int 
                                            )>>
    filter_benchmarks{
      {"self_bloom", bench_selfbloom},
      {"register_bloom", bench_register_block_bloom},
      {"block_bloom", bench_blockbloom}
    };