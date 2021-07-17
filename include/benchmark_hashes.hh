#pragma once

#include <functional>
#include <random>
#include <string>
#include <vector>
#include <unordered_map>

void bench_hashes(std::vector<std::string>& data, std::vector<std::string>& probes, 
                 std::unordered_map<int, std::vector<int>>& locations, std::unordered_map<int, std::vector<double>>& entropies);

static const std::unordered_map<std::string,
                         std::function<void(std::vector<std::string>&,
                                            std::vector<std::string>&,
                                            std::unordered_map<int, std::vector<int>>&,
                                            std::unordered_map<int, std::vector<double>>&
                                            )>>
    hash_benchmarks{
      {"bench_hashes", bench_hashes}
    };