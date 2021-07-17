#include <algorithm>
#include <chrono>
#include <random>
#include <unordered_set>

#include <omp.h>

#include <climits>
#include <cinttypes>
#include <cmath>
#include <cstdio>
#include <string>
#include <absl/container/flat_hash_map.h>

#include "benchmark_filters.hh"
#include "benchmarks.hh"
#include "libbloom.h"
#include "storage.hh"
#include "workload_generator.hh"
#include "self_reg_bloom.hh"
#include "self_block_bloom.hh"
#include "register_blocked_bloom.hh"
#include "hash.hh"

absl::flat_hash_map<std::string, int, hash_xxh64_s> map_xxh64_full_key_hasher_filter(1);
double total_time_without_dummy_hashing_filter = 0;

void relay_filter_statistics(std::string metric_class, std::vector<std::string>& probes, int64_t result, 
		std::string filter_name, int numProbesUnsuccessful, int64_t s_w, uint64_t dummy) {

	if (metric_class == "filter_latency") { 
		int fps = result;
        float fpr = (float) fps / (float) numProbesUnsuccessful;
        fprs["filter_latency"][filter_name] += fpr;

        if (results_directory) {
            char fname[100];
            sprintf(fname, "%s/%s-fp.txt", results_directory, filter_name.c_str());
            std::cout << "warmup s_w: " << s_w << std::endl;
            std::cout << "dummy hashing dummy: " << dummy << std::endl;
            add_to_file(fname, (double) fps);
        }
	}
	else if (metric_class == "filter_throughput") {
		int successful = probes.size() - numProbesUnsuccessful;
		int fps = result - successful;
		float fpr = (float) fps / (float) numProbesUnsuccessful;
		fprs["filter_throughput"][filter_name] += fpr;

		if (results_directory) {
			char fname[100];
			sprintf(fname, "%s/%s-fp.txt", results_directory, filter_name.c_str());
			std::cout << "warmup s_w: " << s_w << std::endl; 
			std::cout << "dummy hashing dummy: " << dummy << std::endl; 
			add_to_file(fname, (double) fps);
		}
	}
	else {
		std::cout << "ERROR: Invalid metric in relay_filter_statistics(). The metric should be either latency or throughput." << std::endl;
		exit(1);
	}
}

// void bench_libbloom(std::vector<std::string>& data, std::vector<std::string>& probes, 
//                  std::unordered_map<int, std::vector<int>>& locations, std::unordered_map<int, std::vector<int>>& num_locations,
//                  int numProbesUnsuccessful) {
//     std::unordered_set<uint64_t> hashset;
//     uint64_t* hashes = (uint64_t*) malloc(data.size() * sizeof(uint64_t));

//     int nhashes = 0;
//     for (unsigned i = 0; i < data.size(); i++) {
//         auto h = std::hash<std::string>{}(data[i]);//hash(data[i]);
//         if (hashset.find(h) == hashset.end()) {
//             hashes[nhashes++] = h;
//         }

//         hashset.insert(h);
//     }
//     printf("hashes: %d, %lu\n", nhashes, data.size());

//     struct bloom filter;
//     int j = bloom_init(&filter, nhashes, 0.01);
//     if (j) {
//         printf("Failure\n");
//         exit(0);
//     }

//     for (int i = 0; i < nhashes; i++) {
//         bloom_add(&filter, &hashes[i], 8);
//     }

//     auto p = probes;

//     int64_t result = 0;
//     benchmark(
//         "libbloom", "filter",
//         [&p, &filter] {
//             int64_t s = 0;
//             for (unsigned i = 0; i < p.size(); i++) {
//                 auto h = std::hash<std::string>{}(p[i]);
//                 s += bloom_check(&filter, &h, 8);
//             }
//             return s;
//         },
//         [&result](int64_t s) {
//             printf("%lld\r", s);
//             fflush(stdout);
//             result = s;
//         });

//     relay_filter_statistics(probes, result, "libbloom-std", numProbesUnsuccessful);
//     bloom_free(&filter);
//     free(hashes);
// }

template <class Filter>
void benchFilter_throughput(std::vector<std::string>& data,std::vector<std::string>& probes,
        Filter& filter, std::string filter_name, int numProbesUnsuccessful) {

	// add data, if not already added by latency experiments
	if (!table_metrics["latency"]) {
		for (unsigned i = 0; i < data.size(); i++) {
			filter.addValue(data[i]);
		}
	}

    int64_t result = 0;
    // test positives return positive
    for (unsigned i = 0; i < data.size(); i++) {
        result += filter.testValue(data[i]);
    }
    assert((unsigned int)result == data.size());
    auto p = probes;
    result = 0;
    
	// warm-up
	int64_t s_w = 0;
	for (unsigned i = 0; i < p.size(); i++) {
		s_w += filter.testValue(p[i]);
	}

	uint64_t dummy = 0;
    //uint64_t non_dummy = 0;
    benchmark(filter_name, "filter_throughput",
        [&p, &filter, &dummy] {
			// v2 with dummy hashing
			int64_t s = 0;
			size_t step_size = 100;
			std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
			total_time_without_dummy_hashing_filter = 0;
			for(unsigned i = 0; i < p.size()/step_size; i++) {
				// dummy hashing
				for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
					dummy += map_xxh64_full_key_hasher_filter.hash_ref()(p[j]);
				}			

				// perform lookups
				start = std::chrono::high_resolution_clock::now();
                for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
					s += filter.testValue(p[j]);
				}
				end = std::chrono::high_resolution_clock::now();
                total_time_without_dummy_hashing_filter += (double)
                    std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                        .count();
			}

			return s;

			/*// v1 with straight reading input data
			int64_t s = 0;
            for (unsigned i = 0; i < p.size(); i++) {
				s += filter.testValue(p[i]);
            }
            return s;*/
        },
        [&result](int64_t s) {
            printf("%" PRId64 "\r", s);
            fflush(stdout);
            result = s;
        });
    relay_filter_statistics("filter_throughput", probes, result, filter_name, numProbesUnsuccessful, s_w, dummy);
}

bool in_filter(std::string& pi, std::vector<std::string>& data) {
	for (unsigned i = 0; i < data.size(); i++) 
		if(pi == data[i])
			return true;

	return false;
}

template <class Filter>
void benchFilter_latency(std::vector<std::string>& data,std::vector<std::string>& probes,
        Filter& filter, std::string filter_name, int numProbesUnsuccessful) {

	// add data
	absl::flat_hash_map<std::string, int, hash_xxh64_s> map(data.size());
    for (unsigned i = 0; i < data.size(); i++) {
        filter.addValue(data[i]);
		// add also to hash table to set probe_filter_map efficiently
		map[data[i]] = i;
    }

    int64_t result = 0;
    // test positives return positive
    for (unsigned i = 0; i < data.size(); i++) {
        result += filter.testValue(data[i]);
    }
    assert((unsigned int)result == data.size());

    auto p = probes;

	// assign probe_to_filter_map for estimating fpr accurately
	std::vector<bool> probe_to_filter_map;
	probe_to_filter_map.reserve(p.size());
	for (unsigned i = 0; i < p.size(); i++) {
		auto it = map.find(p[i]);
        if (it != map.end()) 
            probe_to_filter_map[i] = true;
		else
			probe_to_filter_map[i] = false;
	}

    result = 0;
    
	// warm-up
	int64_t s_w = 0;
	for (unsigned i = 0; i < p.size(); i++) {
		s_w += filter.testValue(p[i]);
	}

	uint64_t dummy = 0;
    //uint64_t non_dummy = 0;
    benchmark(filter_name, "filter_latency",
        [&p, &filter, &dummy, &probe_to_filter_map] {
			// v2 with dummy hashing
			int64_t s = 0;
			size_t step_size = 100;
			std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
			total_time_without_dummy_hashing_filter = 0;
			for(unsigned int i = 0; i < p.size()/step_size; i++) {
				// dummy hashing
				for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
					dummy += map_xxh64_full_key_hasher_filter.hash_ref()(p[j]);
				}			

				// perform lookups
				start = std::chrono::high_resolution_clock::now();
				int counter = 0;
				for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
					bool interm = filter.testValue(p[j]);
					s += (int64_t) (interm & !probe_to_filter_map[j]); // if test succeeds, but value is not in filter (false-positive)
					j += (unsigned) interm;
					counter++;
				}
				end = std::chrono::high_resolution_clock::now();
				total_time_without_dummy_hashing_filter += ((double)
					std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
						.count()) / ( ((double) counter) / ((double) step_size) )  ;
			}

			return s;

			/*// v1 with straight reading input data
			int64_t s = 0;
            for (unsigned i = 0; i < p.size(); i++) {
				s += filter.testValue(p[i]);
            }
            return s;*/
        },
        [&result](int64_t s) {
            printf("%" PRId64 "\r", s);
            fflush(stdout);
            result = s;
        });
    relay_filter_statistics("filter_latency", probes, result, filter_name, numProbesUnsuccessful, s_w, dummy);
}


template <class Filter>
void benchFilter(std::vector<std::string>& data,std::vector<std::string>& probes,
        Filter& filter, std::string filter_name, int numProbesUnsuccessful) {
	if (table_metrics["latency"]) {
        benchFilter_latency(data, probes, filter, filter_name, numProbesUnsuccessful);
    }
    if (table_metrics["throughput"]) {
        benchFilter_throughput(data, probes, filter, filter_name, numProbesUnsuccessful);
    }
}


void bench_selfbloom(std::vector<std::string>& data, std::vector<std::string>& probes, 
                 std::unordered_map<int, std::vector<int>>& locations, std::unordered_map<int, std::vector<double>>& entropies, 
                 int numProbesUnsuccessful) {
    std::unordered_map<int, int> numLocationsForChunkSize;
    // the -1 comes from a better bound on the binomial then the union bound.
    // makes it closer to probability (2\eps)/n -> equation below
    double needed_entropy = log2(data.size()) - log2(added_fpr) - 1;
    for (auto iter : entropies) {
        numLocationsForChunkSize[iter.first] = iter.second.size()-1;
        for (unsigned int i = 0; i < iter.second.size(); i++) {
            if (entropies[iter.first][i] > needed_entropy) {
                numLocationsForChunkSize[iter.first] = i+1;
                break;
            }
        }
        if (iter.first == 8 and numLocationsForChunkSize[iter.first] > 4) {
            numLocationsForChunkSize[iter.first] = 5;
            std::cout << "too much entropy needed, defaulting to normal hash function" << std::endl;
        }
    }
    //std::cout << "BF needed_ent: " << needed_entropy << ", i8: " << numLocationsForChunkSize[8] << ", ent8: " << entropies[8][numLocationsForChunkSize[8]-1] << std::endl;
    //std::cout << "BF needed_ent: " << needed_entropy << ", i4: " << numLocationsForChunkSize[4] << ", ent8: " << entropies[4][numLocationsForChunkSize[4]-1] << std::endl;

	if (hashfuncs["standard"]) {
		selfBloom<std::string, hash_standard_s> filter = selfBloom<std::string, hash_standard_s>
		(selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size());
    	benchFilter<selfBloom<std::string, hash_standard_s>>(data, probes, filter, "selfbloom-std", numProbesUnsuccessful);
    }
    if (hashfuncs["wyhash"]) {
        selfBloom<std::string, hash_wyhash_s> filter = selfBloom<std::string, hash_wyhash_s>
        (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size());
        benchFilter<selfBloom<std::string, hash_wyhash_s>>(data, probes, filter, "selfbloom-wyhash", numProbesUnsuccessful);
    }
    if (hashfuncs["xxh3"]) {
        selfBloom<std::string, hash_xxh64_s> filter = selfBloom<std::string, hash_xxh64_s>
        (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size());
        benchFilter<selfBloom<std::string, hash_xxh64_s>>(data, probes, filter, "selfbloom-xxh32", numProbesUnsuccessful);
    }
    if (hashfuncs["crc32"]) {
        selfBloom<std::string, CRC32Hash_s> filter = selfBloom<std::string, CRC32Hash_s>
        (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size());
        benchFilter<selfBloom<std::string, CRC32Hash_s>>(data, probes, filter, "selfbloom-crc32", numProbesUnsuccessful);
    }
    if (hashfuncs["xxh3_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                selfBloom<std::string, hash_XXH3_1loc_fixed_s> filter = selfBloom<std::string, hash_XXH3_1loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_1loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_XXH3_1loc_fixed_s>>(data, probes, filter, "selfbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBloom<std::string, hash_XXH3_2loc_fixed_s> filter = selfBloom<std::string, hash_XXH3_2loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_2loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_XXH3_2loc_fixed_s>>(data, probes, filter, "selfbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBloom<std::string, hash_XXH3_3loc_fixed_s> filter = selfBloom<std::string, hash_XXH3_3loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_3loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_XXH3_3loc_fixed_s>>(data, probes, filter, "selfbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 4) {
                selfBloom<std::string, hash_XXH3_4loc_fixed_s> filter = selfBloom<std::string, hash_XXH3_4loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_4loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_XXH3_4loc_fixed_s>>(data, probes, filter, "selfbloom-xxh32_sub8", numProbesUnsuccessful);
            } else {
                selfBloom<std::string, hash_xxh64_s> filter = selfBloom<std::string, hash_xxh64_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size());
                benchFilter<selfBloom<std::string, hash_xxh64_s>>(data, probes, filter, "selfbloom-xxh32_sub8", numProbesUnsuccessful);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                selfBloom<std::string, hash_XXH3_1loc_s> filter = selfBloom<std::string, hash_XXH3_1loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_1loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_XXH3_1loc_s>>(data, probes, filter, "selfbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBloom<std::string, hash_XXH3_2loc_s> filter = selfBloom<std::string, hash_XXH3_2loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_2loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_XXH3_2loc_s>>(data, probes, filter, "selfbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBloom<std::string, hash_XXH3_3loc_s> filter = selfBloom<std::string, hash_XXH3_3loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_3loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_XXH3_3loc_s>>(data, probes, filter, "selfbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 4) {
                selfBloom<std::string, hash_XXH3_4loc_s> filter = selfBloom<std::string, hash_XXH3_4loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_4loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_XXH3_4loc_s>>(data, probes, filter, "selfbloom-xxh32_sub8", numProbesUnsuccessful);
            } else {
                selfBloom<std::string, hash_xxh64_s> filter = selfBloom<std::string, hash_xxh64_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size());
                benchFilter<selfBloom<std::string, hash_xxh64_s>>(data, probes, filter, "selfbloom-xxh32_sub8", numProbesUnsuccessful);
            }
        }
    }
    if (hashfuncs["wyhash_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                selfBloom<std::string, hash_wyhash_1loc_fixed_s> filter = selfBloom<std::string, hash_wyhash_1loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_1loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_wyhash_1loc_fixed_s>>(data, probes, filter, "selfbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBloom<std::string, hash_wyhash_2loc_fixed_s> filter = selfBloom<std::string, hash_wyhash_2loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_2loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_wyhash_2loc_fixed_s>>(data, probes, filter, "selfbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBloom<std::string, hash_wyhash_3loc_fixed_s> filter = selfBloom<std::string, hash_wyhash_3loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_3loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_wyhash_3loc_fixed_s>>(data, probes, filter, "selfbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 4) {
                selfBloom<std::string, hash_wyhash_4loc_fixed_s> filter = selfBloom<std::string, hash_wyhash_4loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_4loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_wyhash_4loc_fixed_s>>(data, probes, filter, "selfbloom-wyhash_sub8", numProbesUnsuccessful);
            } else {
                selfBloom<std::string, hash_wyhash_s> filter = selfBloom<std::string, hash_wyhash_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size());
                benchFilter<selfBloom<std::string, hash_wyhash_s>>(data, probes, filter, "selfbloom-wyhash_sub8", numProbesUnsuccessful);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                selfBloom<std::string, hash_wyhash_1loc_s> filter = selfBloom<std::string, hash_wyhash_1loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_1loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_wyhash_1loc_s>>(data, probes, filter, "selfbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBloom<std::string, hash_wyhash_2loc_s> filter = selfBloom<std::string, hash_wyhash_2loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_2loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_wyhash_2loc_s>>(data, probes, filter, "selfbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBloom<std::string, hash_wyhash_3loc_s> filter = selfBloom<std::string, hash_wyhash_3loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_3loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_wyhash_3loc_s>>(data, probes, filter, "selfbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 4) {
                selfBloom<std::string, hash_wyhash_4loc_s> filter = selfBloom<std::string, hash_wyhash_4loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_4loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_wyhash_4loc_s>>(data, probes, filter, "selfbloom-wyhash_sub8", numProbesUnsuccessful);
            } else {
                selfBloom<std::string, hash_wyhash_s> filter = selfBloom<std::string, hash_wyhash_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size());
                benchFilter<selfBloom<std::string, hash_wyhash_s>>(data, probes, filter, "selfbloom-wyhash_sub8", numProbesUnsuccessful);
            }
        }
    }
    if (hashfuncs["crc_wyhash_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                selfBloom<std::string, hash_crc32wyhash_1loc_fixed_s> filter = selfBloom<std::string, hash_crc32wyhash_1loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_crc32wyhash_1loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_crc32wyhash_1loc_fixed_s>>(data, probes, filter, "selfbloom-crc_wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBloom<std::string, hash_crc32wyhash_2loc_fixed_s> filter = selfBloom<std::string, hash_crc32wyhash_2loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_crc32wyhash_2loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_crc32wyhash_2loc_fixed_s>>(data, probes, filter, "selfbloom-crc_wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBloom<std::string, hash_crc32wyhash_3loc_fixed_s> filter = selfBloom<std::string, hash_crc32wyhash_3loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_crc32wyhash_3loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_crc32wyhash_3loc_fixed_s>>(data, probes, filter, "selfbloom-crc_wyhash_sub8", numProbesUnsuccessful);
            } else {
                selfBloom<std::string, hash_crc32wyhash_4loc_fixed_s> filter = selfBloom<std::string, hash_crc32wyhash_4loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_crc32wyhash_4loc_fixed_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_crc32wyhash_4loc_fixed_s>>(data, probes, filter, "selfbloom-crc_wyhash_sub8", numProbesUnsuccessful);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                selfBloom<std::string, hash_crc32wyhash_1loc_s> filter = selfBloom<std::string, hash_crc32wyhash_1loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_crc32wyhash_1loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_crc32wyhash_1loc_s>>(data, probes, filter, "selfbloom-crc_wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBloom<std::string, hash_crc32wyhash_2loc_s> filter = selfBloom<std::string, hash_crc32wyhash_2loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_crc32wyhash_2loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_crc32wyhash_2loc_s>>(data, probes, filter, "selfbloom-crc_wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBloom<std::string, hash_crc32wyhash_3loc_s> filter = selfBloom<std::string, hash_crc32wyhash_3loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_crc32wyhash_3loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_crc32wyhash_3loc_s>>(data, probes, filter, "selfbloom-crc_wyhash_sub8", numProbesUnsuccessful);
            } else {
                selfBloom<std::string, hash_crc32wyhash_4loc_s> filter = selfBloom<std::string, hash_crc32wyhash_4loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_crc32wyhash_4loc_s(hash_locations.data()));
                benchFilter<selfBloom<std::string, hash_crc32wyhash_4loc_s>>(data, probes, filter, "selfbloom-crc_wyhash_sub8", numProbesUnsuccessful);
            }
        }
    }
    if (hashfuncs["mmh2_sub4"]) {
        std::vector<int> hash_locations(&locations[4][0],&locations[4][numLocationsForChunkSize[4]]);
    	std::sort(hash_locations.begin(), hash_locations.end());
        print_locations(hash_locations);
    	selfBloom<std::string, hash_sub4_mmh2_s> filter = selfBloom<std::string, hash_sub4_mmh2_s>
    	(selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_sub4_mmh2_s(hash_locations.data(), numLocationsForChunkSize[4]));
    	benchFilter<selfBloom<std::string, hash_sub4_mmh2_s>>(data, probes, filter, "selfbloom-sub4_mmh2", numProbesUnsuccessful);
	}
	if (hashfuncs["sdbm_sub1"]) {
        std::vector<int> hash_locations(&locations[4][0],&locations[4][numLocationsForChunkSize[1]]);
    	std::sort(hash_locations.begin(), hash_locations.end());
    	selfBloom<std::string, hash_sub1_sdbm_s> filter = selfBloom<std::string, hash_sub1_sdbm_s>
    	(selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_sub1_sdbm_s(hash_locations.data(), numLocationsForChunkSize[1]));
    	benchFilter<selfBloom<std::string, hash_sub1_sdbm_s>>(data, probes, filter, "selfbloom-sub1_sbdm", numProbesUnsuccessful);
	}
}

void bench_blockbloom(std::vector<std::string>& data, std::vector<std::string>& probes, 
                 std::unordered_map<int, std::vector<int>>& locations, std::unordered_map<int, std::vector<double>>& entropies, 
                 int numProbesUnsuccessful) {
    std::unordered_map<int, int> numLocationsForChunkSize;
    // match FPR with Blcoked Bloom Filter
    double needed_entropy = log2(data.size()) - log2(added_fpr);
    for (auto iter : entropies) {
        for (unsigned int i = 0; i < iter.second.size(); i++) {
            if (entropies[iter.first][i] > needed_entropy) {
                numLocationsForChunkSize[iter.first] = i+1;
                break;
            }
        }
        if (iter.first == 8 and numLocationsForChunkSize[iter.first] > 4) {
            numLocationsForChunkSize[iter.first] = 4;
        }
    }
    //std::cout << "BBF needed_ent: " << needed_entropy << ", i8: " << numLocationsForChunkSize[8] << ", ent8: " << entropies[8][numLocationsForChunkSize[8]-1] << std::endl;

    if (hashfuncs["standard"]) {
        selfBlockBloom512<std::string, hash_standard_s> filter = selfBlockBloom512<std::string, hash_standard_s>
        (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size());
        benchFilter<selfBlockBloom512<std::string, hash_standard_s>>(data, probes, filter, "blockbloom-std", numProbesUnsuccessful);
    }
    if (hashfuncs["sub4_mmh2"]) {
        std::vector<int> hash_locations(&locations[4][0],&locations[4][numLocationsForChunkSize[4]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        print_locations(hash_locations);
        selfBlockBloom512<std::string, hash_sub4_mmh2_s> filter = selfBlockBloom512<std::string, hash_sub4_mmh2_s>
        (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_sub4_mmh2_s(hash_locations.data(), numLocationsForChunkSize[4]));
        benchFilter<selfBlockBloom512<std::string, hash_sub4_mmh2_s>>(data, probes, filter, "blockbloom-sub4_mmh2", numProbesUnsuccessful);
    }
    if (hashfuncs["sub1_sdbm"]) {
        std::vector<int> hash_locations(&locations[1][0],&locations[1][numLocationsForChunkSize[1]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        print_locations(hash_locations);
        selfBlockBloom512<std::string, hash_sub1_sdbm_s> filter = selfBlockBloom512<std::string, hash_sub1_sdbm_s>
        (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_sub1_sdbm_s(hash_locations.data(), numLocationsForChunkSize[1]));
        benchFilter<selfBlockBloom512<std::string, hash_sub1_sdbm_s>>(data, probes, filter, "blockbloom-sub1_sbdm", numProbesUnsuccessful);
    }
    if (hashfuncs["wyhash"]) {
        selfBlockBloom512<std::string, hash_wyhash_s> filter = selfBlockBloom512<std::string, hash_wyhash_s>
        (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size());
        benchFilter<selfBlockBloom512<std::string, hash_wyhash_s>>(data, probes, filter, "blockbloom-wyhash", numProbesUnsuccessful);
    }
    if (hashfuncs["xxh3"]) {
        selfBlockBloom512<std::string, hash_xxh64_s> filter = selfBlockBloom512<std::string, hash_xxh64_s>
        (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size());
        benchFilter<selfBlockBloom512<std::string, hash_xxh64_s>>(data, probes, filter, "blockbloom-xxh32", numProbesUnsuccessful);
    }

    if (hashfuncs["xxh3_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                selfBlockBloom512<std::string, hash_XXH3_1loc_fixed_s> filter = selfBlockBloom512<std::string, hash_XXH3_1loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_1loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_XXH3_1loc_fixed_s>>(data, probes, filter, "blockbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBlockBloom512<std::string, hash_XXH3_2loc_fixed_s> filter = selfBlockBloom512<std::string, hash_XXH3_2loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_2loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_XXH3_2loc_fixed_s>>(data, probes, filter, "blockbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBlockBloom512<std::string, hash_XXH3_3loc_fixed_s> filter = selfBlockBloom512<std::string, hash_XXH3_3loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_3loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_XXH3_3loc_fixed_s>>(data, probes, filter, "blockbloom-xxh32_sub8", numProbesUnsuccessful);
            } else {
                selfBlockBloom512<std::string, hash_XXH3_4loc_fixed_s> filter = selfBlockBloom512<std::string, hash_XXH3_4loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_4loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_XXH3_4loc_fixed_s>>(data, probes, filter, "blockbloom-xxh32_sub8", numProbesUnsuccessful);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                selfBlockBloom512<std::string, hash_XXH3_1loc_s> filter = selfBlockBloom512<std::string, hash_XXH3_1loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_1loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_XXH3_1loc_s>>(data, probes, filter, "blockbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBlockBloom512<std::string, hash_XXH3_2loc_s> filter = selfBlockBloom512<std::string, hash_XXH3_2loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_2loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_XXH3_2loc_s>>(data, probes, filter, "blockbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBlockBloom512<std::string, hash_XXH3_3loc_s> filter = selfBlockBloom512<std::string, hash_XXH3_3loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_3loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_XXH3_3loc_s>>(data, probes, filter, "blockbloom-xxh32_sub8", numProbesUnsuccessful);
            } else {
                selfBlockBloom512<std::string, hash_XXH3_4loc_s> filter = selfBlockBloom512<std::string, hash_XXH3_4loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_XXH3_4loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_XXH3_4loc_s>>(data, probes, filter, "blockbloom-xxh32_sub8", numProbesUnsuccessful);
            }
        }
    }
    if (hashfuncs["wyhash_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                selfBlockBloom512<std::string, hash_wyhash_1loc_fixed_s> filter = selfBlockBloom512<std::string, hash_wyhash_1loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_1loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_wyhash_1loc_fixed_s>>(data, probes, filter, "blockbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBlockBloom512<std::string, hash_wyhash_2loc_fixed_s> filter = selfBlockBloom512<std::string, hash_wyhash_2loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_2loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_wyhash_2loc_fixed_s>>(data, probes, filter, "blockbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBlockBloom512<std::string, hash_wyhash_3loc_fixed_s> filter = selfBlockBloom512<std::string, hash_wyhash_3loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_3loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_wyhash_3loc_fixed_s>>(data, probes, filter, "blockbloom-wyhash_sub8", numProbesUnsuccessful);
            } else {
                selfBlockBloom512<std::string, hash_wyhash_4loc_fixed_s> filter = selfBlockBloom512<std::string, hash_wyhash_4loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_4loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_wyhash_4loc_fixed_s>>(data, probes, filter, "blockbloom-wyhash_sub8", numProbesUnsuccessful);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                selfBlockBloom512<std::string, hash_wyhash_1loc_s> filter = selfBlockBloom512<std::string, hash_wyhash_1loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_1loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_wyhash_1loc_s>>(data, probes, filter, "blockbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBlockBloom512<std::string, hash_wyhash_2loc_s> filter = selfBlockBloom512<std::string, hash_wyhash_2loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_2loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_wyhash_2loc_s>>(data, probes, filter, "blockbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBlockBloom512<std::string, hash_wyhash_3loc_s> filter = selfBlockBloom512<std::string, hash_wyhash_3loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_3loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_wyhash_3loc_s>>(data, probes, filter, "blockbloom-wyhash_sub8", numProbesUnsuccessful);
            } else {
                selfBlockBloom512<std::string, hash_wyhash_4loc_s> filter = selfBlockBloom512<std::string, hash_wyhash_4loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element, data.size()), data.size(), hash_wyhash_4loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom512<std::string, hash_wyhash_4loc_s>>(data, probes, filter, "blockbloom-wyhash_sub8", numProbesUnsuccessful);
            }
        }
    }
}

void bench_register_block_bloom(std::vector<std::string>& data, std::vector<std::string>& probes, 
                 std::unordered_map<int, std::vector<int>>& locations, std::unordered_map<int, std::vector<double>>& entropies, 
                 int numProbesUnsuccessful) {
    std::unordered_map<int, int> numLocationsForChunkSize;
    // match FPR with Blcoked Bloom Filter
    double needed_entropy = log2(data.size()) - log2(added_fpr);
    for (auto iter : entropies) {
        numLocationsForChunkSize[iter.first] = iter.second.size()-1;
        for (unsigned int i = 0; i < iter.second.size(); i++) {
            if (entropies[iter.first][i] > needed_entropy) {
                numLocationsForChunkSize[iter.first] = i+1;
                break;
            }
        }
        if (iter.first == 8 and numLocationsForChunkSize[iter.first] > 4) {
            std::cout << "too much entropy needed, defaulting to normal hash function" << std::endl;
            numLocationsForChunkSize[iter.first] = 5;
        }
    }
    //std::cout << "BBF needed_ent: " << needed_entropy << ", i8: " << numLocationsForChunkSize[8] << ", ent8: " << entropies[8][numLocationsForChunkSize[8]-1] << std::endl;

    if (hashfuncs["standard"]) {
        selfBlockBloom64<std::string, hash_standard_s> filter = selfBlockBloom64<std::string, hash_standard_s>
        (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size());
        benchFilter<selfBlockBloom64<std::string, hash_standard_s>>(data, probes, filter, "registerbloom-std", numProbesUnsuccessful);
    }
    if (hashfuncs["sub4_mmh2"]) {
        std::vector<int> hash_locations(&locations[4][0],&locations[4][numLocationsForChunkSize[4]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        print_locations(hash_locations);
        selfBlockBloom64<std::string, hash_sub4_mmh2_s> filter = selfBlockBloom64<std::string, hash_sub4_mmh2_s>
        (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_sub4_mmh2_s(hash_locations.data(), numLocationsForChunkSize[4]));
        benchFilter<selfBlockBloom64<std::string, hash_sub4_mmh2_s>>(data, probes, filter, "registerbloom-sub4_mmh2", numProbesUnsuccessful);
    }
    if (hashfuncs["sub1_sdbm"]) {
        std::vector<int> hash_locations(&locations[1][0],&locations[1][numLocationsForChunkSize[1]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        print_locations(hash_locations);
        selfBlockBloom64<std::string, hash_sub1_sdbm_s> filter = selfBlockBloom64<std::string, hash_sub1_sdbm_s>
        (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_sub1_sdbm_s(hash_locations.data(), numLocationsForChunkSize[1]));
        benchFilter<selfBlockBloom64<std::string, hash_sub1_sdbm_s>>(data, probes, filter, "registerbloom-sub1_sbdm", numProbesUnsuccessful);
    }
    if (hashfuncs["wyhash"]) {
        selfBlockBloom64<std::string, hash_wyhash_s> filter = selfBlockBloom64<std::string, hash_wyhash_s>
        (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size());
        benchFilter<selfBlockBloom64<std::string, hash_wyhash_s>>(data, probes, filter, "registerbloom-wyhash", numProbesUnsuccessful);
    }
    if (hashfuncs["xxh3"]) {
        selfBlockBloom64<std::string, hash_xxh64_s> filter = selfBlockBloom64<std::string, hash_xxh64_s>
        (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size());
        benchFilter<selfBlockBloom64<std::string, hash_xxh64_s>>(data, probes, filter, "registerbloom-xxh32", numProbesUnsuccessful);
    }

    if (hashfuncs["xxh3_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                selfBlockBloom64<std::string, hash_XXH3_1loc_fixed_s> filter = selfBlockBloom64<std::string, hash_XXH3_1loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_XXH3_1loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_XXH3_1loc_fixed_s>>(data, probes, filter, "registerbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBlockBloom64<std::string, hash_XXH3_2loc_fixed_s> filter = selfBlockBloom64<std::string, hash_XXH3_2loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_XXH3_2loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_XXH3_2loc_fixed_s>>(data, probes, filter, "registerbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBlockBloom64<std::string, hash_XXH3_3loc_fixed_s> filter = selfBlockBloom64<std::string, hash_XXH3_3loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_XXH3_3loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_XXH3_3loc_fixed_s>>(data, probes, filter, "registerbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 4) {
                selfBlockBloom64<std::string, hash_XXH3_4loc_fixed_s> filter = selfBlockBloom64<std::string, hash_XXH3_4loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_XXH3_4loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_XXH3_4loc_fixed_s>>(data, probes, filter, "registerbloom-xxh32_sub8", numProbesUnsuccessful);
            } else {
                selfBlockBloom64<std::string, hash_xxh64_s> filter = selfBlockBloom64<std::string, hash_xxh64_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size());
                benchFilter<selfBlockBloom64<std::string, hash_xxh64_s>>(data, probes, filter, "registerbloom-xxh32_sub8", numProbesUnsuccessful);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                selfBlockBloom64<std::string, hash_XXH3_1loc_s> filter = selfBlockBloom64<std::string, hash_XXH3_1loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_XXH3_1loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_XXH3_1loc_s>>(data, probes, filter, "registerbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBlockBloom64<std::string, hash_XXH3_2loc_s> filter = selfBlockBloom64<std::string, hash_XXH3_2loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_XXH3_2loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_XXH3_2loc_s>>(data, probes, filter, "registerbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBlockBloom64<std::string, hash_XXH3_3loc_s> filter = selfBlockBloom64<std::string, hash_XXH3_3loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_XXH3_3loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_XXH3_3loc_s>>(data, probes, filter, "registerbloom-xxh32_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 4) {
                selfBlockBloom64<std::string, hash_XXH3_4loc_s> filter = selfBlockBloom64<std::string, hash_XXH3_4loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_XXH3_4loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_XXH3_4loc_s>>(data, probes, filter, "registerbloom-xxh32_sub8", numProbesUnsuccessful);
            } else {
                selfBlockBloom64<std::string, hash_xxh64_s> filter = selfBlockBloom64<std::string, hash_xxh64_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size());
                benchFilter<selfBlockBloom64<std::string, hash_xxh64_s>>(data, probes, filter, "registerbloom-xxh32_sub8", numProbesUnsuccessful);
            }
        }
    }
    if (hashfuncs["wyhash_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                selfBlockBloom64<std::string, hash_wyhash_1loc_fixed_s> filter = selfBlockBloom64<std::string, hash_wyhash_1loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_wyhash_1loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_wyhash_1loc_fixed_s>>(data, probes, filter, "registerbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBlockBloom64<std::string, hash_wyhash_2loc_fixed_s> filter = selfBlockBloom64<std::string, hash_wyhash_2loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_wyhash_2loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_wyhash_2loc_fixed_s>>(data, probes, filter, "registerbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBlockBloom64<std::string, hash_wyhash_3loc_fixed_s> filter = selfBlockBloom64<std::string, hash_wyhash_3loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_wyhash_3loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_wyhash_3loc_fixed_s>>(data, probes, filter, "registerbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 4) {
                selfBlockBloom64<std::string, hash_wyhash_4loc_fixed_s> filter = selfBlockBloom64<std::string, hash_wyhash_4loc_fixed_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_wyhash_4loc_fixed_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_wyhash_4loc_fixed_s>>(data, probes, filter, "registerbloom-wyhash_sub8", numProbesUnsuccessful);
            } else {
                selfBlockBloom64<std::string, hash_wyhash_s> filter = selfBlockBloom64<std::string, hash_wyhash_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size());
                benchFilter<selfBlockBloom64<std::string, hash_wyhash_s>>(data, probes, filter, "registerbloom-wyhash_sub8", numProbesUnsuccessful);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                selfBlockBloom64<std::string, hash_wyhash_1loc_s> filter = selfBlockBloom64<std::string, hash_wyhash_1loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_wyhash_1loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_wyhash_1loc_s>>(data, probes, filter, "registerbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 2) {
                selfBlockBloom64<std::string, hash_wyhash_2loc_s> filter = selfBlockBloom64<std::string, hash_wyhash_2loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_wyhash_2loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_wyhash_2loc_s>>(data, probes, filter, "registerbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 3) {
                selfBlockBloom64<std::string, hash_wyhash_3loc_s> filter = selfBlockBloom64<std::string, hash_wyhash_3loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_wyhash_3loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_wyhash_3loc_s>>(data, probes, filter, "registerbloom-wyhash_sub8", numProbesUnsuccessful);
            } else if (numLocationsForChunkSize[8] == 4) {
                selfBlockBloom64<std::string, hash_wyhash_4loc_s> filter = selfBlockBloom64<std::string, hash_wyhash_4loc_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size(), hash_wyhash_4loc_s(hash_locations.data()));
                benchFilter<selfBlockBloom64<std::string, hash_wyhash_4loc_s>>(data, probes, filter, "registerbloom-wyhash_sub8", numProbesUnsuccessful);
            } else {
                 selfBlockBloom64<std::string, hash_wyhash_s> filter = selfBlockBloom64<std::string, hash_wyhash_s>
                (selfBloomUtils::generateNumBits(bits_per_element_rb, data.size()), data.size());
                benchFilter<selfBlockBloom64<std::string, hash_wyhash_s>>(data, probes, filter, "registerbloom-wyhash_sub8", numProbesUnsuccessful);
            }
        }
    }
}


