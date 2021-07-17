#pragma once

#include <algorithm>
#include <random>
#include <unordered_set>
#include <cstdint>

#include <omp.h>

#include <climits>
#include <cmath>
#include <cstdio>
#include <string>


std::vector<std::string> uniform_random_strings(std::mt19937& rng,
                                                int num_strings,
                                                int str_size);

void generate_successful_probes(std::mt19937& rng,
    const std::vector<std::string>& inserted_keys,
    unsigned int nprobes,
    std::vector<std::string>& probe_array);

void generate_unsuccessful_probes(std::mt19937& rng,
    const std::vector<std::string>& uninserted_keys,
    unsigned int nprobes,
    std::vector<std::string>& probe_array);

void generate_random_unsuccessful_probes(std::mt19937& rng,
    unsigned int str_size,
    unsigned int nprobes,
    std::vector<std::string>& probe_array);

std::pair<std::vector<std::string>, int> generate_probes_with_unselected_keys_as_false(
    std::mt19937& rng,
    const std::vector<std::string>& inserted_keys,
    const std::vector<std::string>& uninserted_keys,
    float fraction_successful,
    unsigned int nprobes);

std::pair<std::vector<std::string>, int> generate_probes_with_unselected_keys_as_false_static(
    const std::vector<std::string>& inserted_keys,
    const std::vector<std::string>& uninserted_keys,
    float fraction_successful,
    unsigned int nprobes);

std::pair<std::vector<std::string>, int> generate_probes_with_random_false(std::mt19937& rng,
                                         const std::vector<std::string>& keys,
                                         float fraction_successful,
                                         unsigned int nprobes,
                                         unsigned int str_size);
