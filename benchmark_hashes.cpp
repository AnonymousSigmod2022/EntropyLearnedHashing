#include <algorithm>
#include <chrono>
#include <random>
#include <unordered_set>

#include <omp.h>

#include <climits>
#include <cmath>
#include <cstdio>
#include <string>

#include "benchmark_hashes.hh"
#include "benchmarks.hh"
#include "storage.hh"
#include "hash.hh"

void bench_hashes(std::vector<std::string>& data, std::vector<std::string>& probes, 
                 std::unordered_map<int, std::vector<int>>& locations, std::unordered_map<int, std::vector<double>>& entropies) {
    std::vector<std::string> benchdat = data;
    bool hash_probes = true;
    if (hash_probes) {
        benchdat = probes;
    }
    if (hashfuncs["standard"]) {
        benchmark(
            "hash_standard", "hash_func",
            [&benchdat] {
                int64_t s = 0;
                for (unsigned i = 0; i < benchdat.size()/1000; i++) {
	                for (unsigned j=0; j < inner_loop_reuse; j++) {
	                    for (unsigned k=0;k<1000;k++){
	                    	s += hash_standard(benchdat[(1000 * i) + k]);
	                    }
	                }
	            }
                return s;
            },
            [](int64_t s) {
                printf("%lld\r", s);
                fflush(stdout);
            });
    }
    if (hashfuncs["xxh3"]) {
        benchmark(
            "hash_xxh3", "hash_func",
            [&benchdat] {
                int64_t s = 0;
                for (unsigned i = 0; i < benchdat.size()/1000; i++) {
	                for (unsigned j=0; j < inner_loop_reuse; j++) {
	                    for (unsigned k=0;k<1000;k++){
	                    	s += hash_xxh64(benchdat[(1000 * i) + k]);
	                    }
	                }
	            }
                return s;
                /*for (unsigned i = 0; i < benchdat.size(); i++) {
                    s += hash_xxh64(benchdat[i]);
                }
                return s; */
            },
            [](int64_t s) {
                printf("%lld\r", s);
                fflush(stdout);
            });
    }
    if (hashfuncs["wyhash"]) {
        benchmark(
            "wyhash", "hash_func",
            [&benchdat] {
                int64_t s = 0;
                for (unsigned i = 0; i < benchdat.size()/1000; i++) {
                    for (unsigned j=0; j < inner_loop_reuse; j++) {
                        for (unsigned k=0;k<1000;k++){
                            s += wyhash_default(benchdat[(1000 * i) + k]);
                        }
                    }
                }
                return s;
                /*for (unsigned i = 0; i < benchdat.size(); i++) {
                    s += hash_xxh64(benchdat[i]);
                }
                return s; */
            },
            [](int64_t s) {
                printf("%lld\r", s);
                fflush(stdout);
            });
    }
    if (hashfuncs["xxh3_sub8"]) {
    	std::vector<unsigned int> hash_locations(&locations[8][0],&locations[8][2]);
        std::sort(hash_locations.begin(), hash_locations.end());
        for (unsigned int i = 0; i < hash_locations.size(); i++) {
            hash_locations[i] = hash_locations[i] * 8;
        }
        hash_locations.push_back(hash_locations[hash_locations.size() - 1] + 8);
        benchmark(
            "hash_xxh3_sub8", "hash_func",
            [&benchdat, &hash_locations] {
                int64_t s = 0;
                for (unsigned i = 0; i < benchdat.size()/1000; i++) {
	                for (unsigned j=0; j < inner_loop_reuse; j++) {
	                    for (unsigned k=0;k<1000;k++){
	                    	s += XXH3_2loc((xxh_u8*)benchdat[(1000 * i) + k].data(),benchdat[(1000 * i) + k].length(), hash_locations.data());//, hash_locations.data());
	                    }
	                }
	            }
	            return s;
            },
            [](int64_t s) {
                printf("%lld\r", s);
                fflush(stdout);
            });
    }
    if (hashfuncs["wyhash_sub8"]) {
        std::vector<unsigned int> hash_locations(&locations[8][0],&locations[8][2]);
        std::sort(hash_locations.begin(), hash_locations.end());
        for (unsigned int i = 0; i < hash_locations.size(); i++) {
            hash_locations[i] = hash_locations[i] * 8;
        }
        hash_locations.push_back(hash_locations[hash_locations.size() - 1] + 8);
        benchmark(
            "wyhash_sub8", "hash_func",
            [&benchdat, &hash_locations] {
                int64_t s = 0;
                for (unsigned i = 0; i < benchdat.size()/1000; i++) {
                    for (unsigned j=0; j < inner_loop_reuse; j++) {
                        for (unsigned k=0;k<1000;k++){
                            s += wyhash_2loc(benchdat[(1000 * i) + k].data(),benchdat[(1000 * i) + k].length(), hash_locations.data());
                        }
                    }
                }
                return s;
            },
            [](int64_t s) {
                printf("%lld\r", s);
                fflush(stdout);
            });
    }
    if (hashfuncs["wyhash_sub8_hardcode"]) {
        benchmark(
            "wyhash_sub8_hardcode", "hash_func",
            [&benchdat] {
                int64_t s = 0;
                for (unsigned i = 0; i < benchdat.size()/1000; i++) {
                    for (unsigned j=0; j < inner_loop_reuse; j++) {
                        for (unsigned k=0;k<1000;k++){
                            s += crc32_wyhash_hardcode(benchdat[(1000 * i) + k].data(), benchdat[(1000 * i) + k].length());
                        }
                    }
                }
                return s;
            },
            [](int64_t s) {
                printf("%lld\r", s);
                fflush(stdout);
            });
    }
    if (hashfuncs["mmh2_sub4"]) {
    	std::vector<int> hash_locations(&locations[4][0],&locations[4][4]);
    	std::sort(hash_locations.begin(), hash_locations.end());
    	benchmark(
        "hash_mmh2_sub4", "hash_func",
        [&benchdat, &hash_locations] {
            int64_t s = 0;
            for (unsigned i = 0; i < benchdat.size()/1000; i++) {
                for (unsigned j=0; j < inner_loop_reuse; j++) {
                    for (unsigned k=0;k<1000;k++){
                    	s += hash_sub4_mmh2(benchdat[(1000 * i) + k], hash_locations.data(), 4);
                    }
                }
            }
            return s;
        },
        [](int64_t s) {
            printf("%lld\r", s);
            fflush(stdout);
        });
    }
}