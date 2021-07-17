#pragma once

#include <functional>
#include <random>
#include <string>
#include <vector>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <chrono>
#include <time.h>

#include "benchmark_filters.hh"
#include "benchmark_hashes.hh"
#include "benchmark_tables.hh"
#include "benchmark_partitions.hh"
#include "storage.hh"

extern std::unordered_map<std::string, uint64_t> num_iters_total;
extern int num_trials;
extern bool set_load_factor;
extern float load_factor;
extern float fract_successful;
extern bool hash_probes;
extern double bits_per_element;
extern double bits_per_element_rb;
extern const char* results_directory;
constexpr bool store_hash = true;
extern double added_fpr;
// for fast computations, read data some number of times in a smaller inner loop
// (1000 items). This is so that memory bandwidth of reading in data isn't the main bottleneck
// simulates many cases where item is already in cache for other operations.
constexpr int inner_loop_reuse = 1;
extern double total_time_without_dummy_hashing;
extern double total_time_without_dummy_hashing_filter;
extern double total_time_without_dummy_hashing_partition;

template <typename BF, typename VF>
void benchmark(const std::string& name,
               BF func,
               VF verify);

extern std::unordered_map<std::string, bool> hashfuncs;
extern std::unordered_map<std::string, std::unordered_map<std::string, double>> timings;
//extern std::unordered_map<std::string, std::unordered_map<std::string, std::vector<double>>> mt_timings;
extern std::unordered_map<std::string, std::unordered_map<std::string, double>> fprs;
extern std::unordered_map<std::string, double> partition_variances;
extern std::unordered_map<int, std::vector<int>> locations;
extern std::unordered_map<int, std::vector<double>> entropies;
extern bool dataIsFixedLength;
extern std::mutex mylock;

template <class BF, class VF>
void benchmark(const std::string& name, const std::string& metric_class, 
               BF func,
               VF verify) {
    double total_time = 0;

    auto start = std::chrono::high_resolution_clock::now();

    auto result = func();

    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
	
    //total_time = (double) duration; // use for the configuration without dummy hashing
	total_time = total_time_without_dummy_hashing; // use for the configuration with dummy hashing
	//total_time = total_time_without_dummy_hashing_filter; // use for the configuration with dummy hashing for filter	
	//total_time = total_time_without_dummy_hashing_partition; // use for the configuration with dummy hashing for partition

    verify(result);
    mylock.lock();
    if (metric_class == "filter_latency" || metric_class == "filter_throughput" || metric_class == "hash_func") {
      // we run the inner loop of filters more times 
      // as we often run into memory bandwidth effects reading in the probe data.
      // real life operations often have their probes already in memory, so simulates this better.
      timings[metric_class][name] += (total_time_without_dummy_hashing_filter / inner_loop_reuse);
    } else {
      timings[metric_class][name] += total_time;
    }
    //printf("%s: avg time: %.3lf ms\n", name.c_str(), result);
    if (results_directory) {
        char fname[100];
        sprintf(fname, "%s/%s.txt", results_directory, name.c_str());
        add_to_file(fname, result);
    }
    mylock.unlock();
}

template <class BF, class VF>
void benchmark_mt(const std::string& name, const std::string& metric_class, 
               BF func,
               VF verify) {
    double total_time = 0;

    struct timespec ts1, ts2; // both C11 and POSIX
    //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts1); // POSIX

    auto res = func();

    //clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts2);
 
    //double time_elapsed_ns = (1e9*ts2.tv_sec + ts2.tv_nsec) - (1e9*ts1.tv_sec + ts1.tv_nsec);
    //time_elapsed_ns = time_elapsed_ns - res;
    
    total_time = (double) res; // time_elapsed_ns; // use for the configuration without dummy hashing
    //total_time = total_time_without_dummy_hashing; // use for the configuration with dummy hashing
    //total_time = total_time_without_dummy_hashing_filter; // use for the configuration with dummy hashing for filter  
	//std::cout << "dur: " << res << std::endl;

    verify(res);
    mylock.lock();
    if (metric_class == "filter" || metric_class == "hash_func") {
      // we run the inner loop of filters more times 
      // as we often run into memory bandwidth effects reading in the probe data.
      // real life operations often have their probes already in memory, so simulates this better.
      timings[metric_class][name] += (total_time_without_dummy_hashing_filter / inner_loop_reuse);
    } else {
      timings[metric_class][name] += 0; // res; // unused at the moment
    }
    //printf("%s: avg time: %.3lf ms\n", name.c_str(), result);
    if (results_directory) {
        char fname[100];
        sprintf(fname, "%s/%s.txt", results_directory, name.c_str());
        add_to_file(fname, res);
    }
    mylock.unlock();
}

