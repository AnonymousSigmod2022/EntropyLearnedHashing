#include <algorithm>
#include <chrono>
#include <random>
#include <unordered_set>

#include <limits>
#include <math.h>
#include <cstdio>
#include <cinttypes>


#include <robin_map.h>

#include <google/dense_hash_map>
#include <absl/container/flat_hash_map.h>
//#include <folly/FBString.h>
//#include <folly/container/F14Map.h>

#include "hash.hh"
#include "benchmarks.hh"

// This function inserts the given dataset into a given hashtable and
// performs a benchmark on reading all the probes.
// Note: hashtables have their hash function built into their type.

// dummy hasher
absl::flat_hash_map<std::string, int, hash_xxh64_s> map_xxh64_full_key_hasher(1);
double total_time_without_dummy_hashing = 0;

template <class M>
void bench_hashtable_throughput(const char* name,
                     M& map,
                     std::vector<std::string>& data,
                     std::vector<std::string>& probes) {
    if (set_load_factor) {
        map.max_load_factor(load_factor);
    }


    auto start = std::chrono::high_resolution_clock::now();
    for (unsigned i = 0; i < data.size(); i++) {
        map[data[i]] = i;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
    timings["hash_table_build"][name] += (double) duration / (1000.0 * 1000.0);

	/*std::cout << "hash table is built." << std::endl;
    std::cout << "load factor: " << map.load_factor() << std::endl;
	std::cout << "capacity: " << map.bucket_count() << std::endl;*/

    auto p = probes;

	// warm-up
	int64_t s = 0;
	for (unsigned i = 0; i < p.size(); i++) {
		auto it = map.find(p[i]);
		if (it != map.end()) {
			s += it->second;
		}
	}

	/*// vtune profilable version
	// warm-up
	int64_t s = 0;
	uint64_t elapsed_time = 0;
	uint64_t warmup_time = 30000000000;
	uint64_t profiling_time = 4 * warmup_time; 
	start = std::chrono::high_resolution_clock::now();
	while(elapsed_time < warmup_time) {
		for (unsigned i = 0; i < p.size(); i++) {
			auto it = map.find(p[i]);
			if (it != map.end()) {
				s += it->second;
			}
		}
		end = std::chrono::high_resolution_clock::now();
		elapsed_time = (uint64_t)
			std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
				.count();
	}*/

    benchmark(
        name, "hash_table_throughput", 
        //[&p, &map, &s, &profiling_time] {
        [&p, &map, &s] {
			// without dummy hashing (below)
			/*for (unsigned i = 0; i < p.size(); i++) {
                auto it = map.find(p[i]);
                if (it != map.end()) {
                    s += it->second;
                }
            }
            return s;*/

			// with dummy hashing (below)
			uint64_t dummy = 0;
			size_t step_size = 100;
			std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
			total_time_without_dummy_hashing = 0;
			for (unsigned i = 0; i < p.size()/step_size; i++) {
				// dummy hashing to bring the keys to the cache
				for (unsigned j = i*step_size; j < (i+1)*step_size; j++)
					dummy += map_xxh64_full_key_hasher.hash_ref()(p[j]);

				start = std::chrono::high_resolution_clock::now();
				for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
					auto it = map.find(p[j]);
					if (it != map.end()) {
						s += it->second;
					}
				}
				end = std::chrono::high_resolution_clock::now();
				total_time_without_dummy_hashing += (double)
					std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
						.count();
			}

            return s;

			/*// vtune profilable version
			std::cout << "profiling started." << std::endl;	
			// with dummy hashing (below)
			uint64_t dummy = 0;
			size_t step_size = 100;
			std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
			std::chrono::time_point<std::chrono::high_resolution_clock> start2, end2;
			total_time_without_dummy_hashing = 0;
			uint64_t elapsed_time = 0;
			start2 = std::chrono::high_resolution_clock::now();
			while(elapsed_time < profiling_time) {
				for (unsigned i = 0; i < p.size()/step_size; i++) {
					// dummy hashing to bring the keys to the cache
					for (unsigned j = i*step_size; j < (i+1)*step_size; j++)
						dummy += map_xxh64_full_key_hasher.hash_ref()(p[j]);

					start = std::chrono::high_resolution_clock::now();
					for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
						auto it = map.find(p[j]);
						if (it != map.end()) {
							s += it->second;
						}
					}
					end = std::chrono::high_resolution_clock::now();
					total_time_without_dummy_hashing += (double)
						std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
							.count();
				}
				end2 = std::chrono::high_resolution_clock::now();
				elapsed_time = (uint64_t)
					std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2)
						.count();
			}

            return s;*/
        },
        [](int64_t s) {
            printf("%" PRId64 "\r", s);
            fflush(stdout);
        });
}

template <class M>
void bench_hashtable_throughput_only_swiss_table(const char* name,
                     M& map,
                     std::vector<std::string>& data,
                     std::vector<std::string>& probes) {
    if (set_load_factor) {
        map.max_load_factor(load_factor);
    }


    auto start = std::chrono::high_resolution_clock::now();
    for (unsigned i = 0; i < data.size(); i++) {
        map[data[i]] = i;
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
    timings["hash_table_build"][name] += (double) duration / (1000.0 * 1000.0);

	/*std::cout << "hash table is built." << std::endl;
    std::cout << "load factor: " << map.load_factor() << std::endl;
	std::cout << "capacity: " << map.bucket_count() << std::endl;*/

    auto p = probes;

	// warm-up
	int64_t s = 0;
	for (unsigned i = 0; i < p.size(); i++) {
		auto it = map.find(p[i]);
		if (it != map.end()) {
			s += it->second;
		}
	}

	/*// vtune profilable version
	// warm-up
	int64_t s = 0;
	uint64_t elapsed_time = 0;
	uint64_t warmup_time = 30000000000;
	uint64_t profiling_time = 4 * warmup_time; 
	start = std::chrono::high_resolution_clock::now();
	while(elapsed_time < warmup_time) {
		for (unsigned i = 0; i < p.size(); i++) {
			auto it = map.find(p[i]);
			if (it != map.end()) {
				s += it->second;
			}
		}
		end = std::chrono::high_resolution_clock::now();
		elapsed_time = (uint64_t)
			std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
				.count();
	}*/

    benchmark(
        name, "hash_table_throughput", 
        //[&p, &map, &s, &profiling_time] {
        [&p, &map, &s] {
			/*// without dummy hashing (below)
			for (unsigned i = 0; i < p.size(); i++) {
                auto it = map.find(p[i]);
                if (it != map.end()) {
                    s += it->second;
                }
            }
            return s;*/

			uint64_t num_visited_group_prev = map.num_visited_group();
			uint64_t num_h2_pass_prev = map.num_h2_pass();
			uint64_t num_iter_profiling = 1;

			// with dummy hashing (below)
			uint64_t dummy = 0;
			size_t step_size = 100;
			std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
			total_time_without_dummy_hashing = 0;
			for (unsigned i = 0; i < p.size()/step_size; i++) {
				// dummy hashing to bring the keys to the cache
				for (unsigned j = i*step_size; j < (i+1)*step_size; j++)
					dummy += map_xxh64_full_key_hasher.hash_ref()(p[j]);

				start = std::chrono::high_resolution_clock::now();
				for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
					auto it = map.find(p[j]);
					if (it != map.end()) {
						s += it->second;
					}
				}
				end = std::chrono::high_resolution_clock::now();
				total_time_without_dummy_hashing += (double)
					std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
						.count();
			}

			std::cout << "num_visited_group per iter during profiling: " << ( (double) (map.num_visited_group() - num_visited_group_prev) ) / ((double) (num_iter_profiling * p.size())) << std::endl;
			std::cout << "num_h2_pass per iter during profiling: " << ( (double) (map.num_h2_pass() - num_h2_pass_prev) ) / ((double) (num_iter_profiling * p.size())) << std::endl;

            return s;

			/*// vtune profilable version
			std::cout << "profiling started." << std::endl;	
			// with dummy hashing (below)
			uint64_t dummy = 0;
			size_t step_size = 100;
			std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
			std::chrono::time_point<std::chrono::high_resolution_clock> start2, end2;
			total_time_without_dummy_hashing = 0;
			uint64_t elapsed_time = 0;
			start2 = std::chrono::high_resolution_clock::now();
			while(elapsed_time < profiling_time) {
				for (unsigned i = 0; i < p.size()/step_size; i++) {
					// dummy hashing to bring the keys to the cache
					for (unsigned j = i*step_size; j < (i+1)*step_size; j++)
						dummy += map_xxh64_full_key_hasher.hash_ref()(p[j]);

					start = std::chrono::high_resolution_clock::now();
					for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
						auto it = map.find(p[j]);
						if (it != map.end()) {
							s += it->second;
						}
					}
					end = std::chrono::high_resolution_clock::now();
					total_time_without_dummy_hashing += (double)
						std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
							.count();
				}
				end2 = std::chrono::high_resolution_clock::now();
				elapsed_time = (uint64_t)
					std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2)
						.count();
			}
            return s;*/
        },
        [](int64_t s) {
            printf("%" PRId64 "\r", s);
            fflush(stdout);
        });
}

template <class M>
void bench_hashtable_latency(const char* name,
                     M& map,
                     std::vector<std::string>& data,
                     std::vector<std::string>& probes) {
    if (set_load_factor) {
        map.max_load_factor(load_factor);
    }
    srand(time(NULL));

    auto start = std::chrono::high_resolution_clock::now();
    for (unsigned i = 0; i < data.size(); i++) {
        map[data[i]] = (rand() % 3);
    }
    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
    timings["hash_table_build"][name] = (double) duration / (1000.0 * 1000.0);

    auto p = probes;

	// warm-up
	int64_t s = 0;
	unsigned int i = 0;
	for (int r = 0; r < 2; r++) {
		while(i < p.size()) {
			auto it = map.find(p[i]);
			if (it != map.end()) {
				s += it->second;
				i += it->second;
			}
			i += 1;
		}
		i -= p.size();
	}

	/*// vtune profilable version
	// warm-up
	int64_t s = 0;
	uint64_t elapsed_time = 0;
	uint64_t warmup_time = 30000000000;
	uint64_t profiling_time = 4 * warmup_time; 
	start = std::chrono::high_resolution_clock::now();
	while(elapsed_time < warmup_time) {
		unsigned int i = 0;
		for (int r = 0; r < 2; r++) {
			while(i < p.size()) {
				auto it = map.find(p[i]);
				if (it != map.end()) {
					s += it->second;
					i += it->second;
				}
				i += 1;
			}
			i -= p.size();
		}
		end = std::chrono::high_resolution_clock::now();
		elapsed_time = (uint64_t)
			std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
				.count();
	}*/

    benchmark(
        name, "hash_table_latency",
        [&p, &map, &s] {
        //[&p, &map, &s, &profiling_time] {
			/*// without dummy hashing (below)
			unsigned int i = 0;
            for (int j = 0; j < 2; j++) {
                while(i < p.size()) {
                    auto it = map.find(p[i]);
                    if (it != map.end()) {
                        s += it->second;
                        i += it->second;
                    }
                    i += 1;
                }
                i -= p.size();
            }
            return s;*/

			// with dummy hashing (below)
            unsigned int i = 0;
			uint64_t dummy = 0;
			size_t step_size = 100;
			std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
			total_time_without_dummy_hashing = 0;
            for (int r = 0; r < 2; r++) {
				for(; i < p.size()/step_size; i++) {
					// dummy hashing to bring the keys to the cache
					for (unsigned j = i*step_size; j < (i+1)*step_size; j++)
						dummy += map_xxh64_full_key_hasher.hash_ref()(p[j]);
				
					start = std::chrono::high_resolution_clock::now();
					for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
						auto it = map.find(p[j]);
						if (it != map.end()) {
							s += it->second;
							j += it->second;
						}
					}
					end = std::chrono::high_resolution_clock::now();
	                total_time_without_dummy_hashing += (double)
    	                std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
        	                .count();
                }
                i -= (p.size()/step_size);
            }
            return s;

			/*// vtune profilable version
			std::cout << "profiling started." << std::endl;	
			// with dummy hashing (below)
			uint64_t dummy = 0;
			size_t step_size = 100;
			unsigned int i = 0;
			std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
			std::chrono::time_point<std::chrono::high_resolution_clock> start2, end2;
			total_time_without_dummy_hashing = 0;
			uint64_t elapsed_time = 0;
			start2 = std::chrono::high_resolution_clock::now();
			while(elapsed_time < profiling_time) {
				for (int r = 0; r < 2; r++) {
					for(; i < p.size()/step_size; i++) {
						// dummy hashing to bring the keys to the cache
						for (unsigned j = i*step_size; j < (i+1)*step_size; j++)
							dummy += map_xxh64_full_key_hasher.hash_ref()(p[j]);
					
						start = std::chrono::high_resolution_clock::now();
						for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
							auto it = map.find(p[j]);
							if (it != map.end()) {
								s += it->second;
								j += it->second;
							}
						}
						end = std::chrono::high_resolution_clock::now();
						total_time_without_dummy_hashing += (double)
							std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
								.count();
					}
					i -= (p.size()/step_size);
				}
				end2 = std::chrono::high_resolution_clock::now();
                elapsed_time = (uint64_t)
                    std::chrono::duration_cast<std::chrono::nanoseconds>(end2 - start2)
                        .count();
			}

            return s;*/
        },
        [](int64_t s) {
            printf("%" PRId64 "\r", s);
            fflush(stdout);
        });
}

template <class M>
void bench_hashtable_only_swiss_table(const char* name,
                     M& map,
                     std::vector<std::string>& data,
                     std::vector<std::string>& probes) {
    if (table_metrics["latency"]) {
        bench_hashtable_latency(name, map, data, probes);
    }
    if (table_metrics["throughput"]) {
        bench_hashtable_throughput_only_swiss_table(name, map, data, probes);
    }
}

template <class M>
void bench_hashtable(const char* name,
                     M& map,
                     std::vector<std::string>& data,
                     std::vector<std::string>& probes) {
    if (table_metrics["latency"]) {
        bench_hashtable_latency(name, map, data, probes);
    }
    if (table_metrics["throughput"]) {
        bench_hashtable_throughput(name, map, data, probes);
    }
}

// ROBIN -----------------------------------------------
void bench_robin_table(std::vector<std::string>& data, std::vector<std::string>& probes,
                            std::unordered_map<int, std::vector<int>>& locations,
                            std::unordered_map<int, std::vector<double>>& entropies) {
    // STANDARD BENCH
    if (hashfuncs["standard"]) {
        tsl::robin_map<std::string, int, hash_standard_s, std::equal_to<std::string>, std::allocator<std::pair<std::string, int>>, store_hash> map(data.size());
        bench_hashtable("robin_standard", map, data, probes);
    }
    if (hashfuncs["xxh3"]) {
        tsl::robin_map<std::string, int, hash_xxh64_s, std::equal_to<std::string>, std::allocator<std::pair<std::string, int>>, store_hash> map(data.size());
        bench_hashtable("robin_xxh3", map, data, probes);
    }
    if (hashfuncs["xxh3_sub8"]) {
        std::vector<int> hash_locations(&locations[4][0],&locations[4][4]);
        std::sort(hash_locations.begin(), hash_locations.end());
        tsl::robin_map<std::string, int, hash_XXH3_2loc_s, std::equal_to<std::string>, std::allocator<std::pair<std::string, int>>, store_hash>
            map(data.size(), hash_XXH3_2loc_s(hash_locations.data()));
        bench_hashtable("robin_xxh3_sub82locSub8", map, data, probes);
    }
    if (hashfuncs["mmh2_sub4"]) {
        std::vector<int> hash_locations(&locations[4][0],&locations[4][4]);
        std::sort(hash_locations.begin(), hash_locations.end());
        tsl::robin_map<std::string, int, hash_sub4_mmh2_s, std::equal_to<std::string>, std::allocator<std::pair<std::string, int>>, store_hash>
        map(data.size(), hash_sub4_mmh2_s(hash_locations.data(), hash_locations.size()));
        bench_hashtable("robin_mmh2_sub4", map, data, probes);
    }
}

// STD::UNORDERED ------------------------------------------
void bench_std_table(std::vector<std::string>& data, std::vector<std::string>& probes,
                          std::unordered_map<int, std::vector<int>>& locations,
                          std::unordered_map<int, std::vector<double>>& entropies) {
    
    std::unordered_map<int, int> numLocationsForChunkSize;
    double needed_entropy = log2(data.size()) + 1.5;
    for (auto iter : entropies) {
        numLocationsForChunkSize[iter.first] = iter.second.size()-1;
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
    // STANDARD BENCH
    if (hashfuncs["standard"]) {
        std::unordered_map<std::string, int, hash_standard_s> map_std(
            data.size());
        bench_hashtable("std_standard", map_std, data, probes);
    }
    if (hashfuncs["crc32"]) {
        std::unordered_map<std::string, int, CRC32Hash_s> map(data.size());
        bench_hashtable("std_crc32", map, data, probes);
    }
    if (hashfuncs["mmh2"]) {
        std::unordered_map<std::string, int, hash_mmh2_s> map(data.size());
        bench_hashtable("std_mmh2", map, data, probes);
    }
    if (hashfuncs["sdbm"]) {
        std::unordered_map<std::string, int, hash_sdbm_s> map(data.size());
        bench_hashtable("std_sdbm", map, data, probes);
    }
    // xxh3 hashing for swiss table
    if (hashfuncs["xxh3"]) {
        std::unordered_map<std::string, int, hash_xxh64_s> map(data.size());
        bench_hashtable("std_xxh3", map, data, probes);
    }
    // wyhash hashing
    if (hashfuncs["wyhash"]) {
        std::unordered_map<std::string, int, hash_wyhash_s> map(data.size());
        //bench_hashtable_only_swiss_table("swiss_wyhash", map, data, probes);
        bench_hashtable("std_wyhash", map, data, probes);
    }
    // custom xxh3
    if (hashfuncs["xxh3_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                std::unordered_map<std::string, int, hash_XXH3_1loc_fixed_s> map(data.size(), hash_XXH3_1loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_xxh3_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                std::unordered_map<std::string, int, hash_XXH3_2loc_fixed_s> map(data.size(), hash_XXH3_2loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_xxh3_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                std::unordered_map<std::string, int, hash_XXH3_3loc_fixed_s> map(data.size(), hash_XXH3_3loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_xxh3_sub8", map, data, probes);
            } else {
                std::unordered_map<std::string, int, hash_XXH3_4loc_fixed_s> map(data.size(), hash_XXH3_4loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_xxh3_sub8", map, data, probes);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                std::unordered_map<std::string, int, hash_XXH3_1loc_s> map(data.size(), hash_XXH3_1loc_s(hash_locations.data()));
                bench_hashtable("std_xxh3_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                std::unordered_map<std::string, int, hash_XXH3_2loc_s> map(data.size(), hash_XXH3_2loc_s(hash_locations.data()));
                bench_hashtable("std_xxh3_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                std::unordered_map<std::string, int, hash_XXH3_3loc_s> map(data.size(), hash_XXH3_3loc_s(hash_locations.data()));
                bench_hashtable("std_xxh3_sub8", map, data, probes);
            } else {
                std::unordered_map<std::string, int, hash_XXH3_4loc_s> map(data.size(), hash_XXH3_4loc_s(hash_locations.data()));
                bench_hashtable("std_xxh3_sub8", map, data, probes);
            }
        }
    }
    // custom wyhash 
    if (hashfuncs["wyhash_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                std::unordered_map<std::string, int, hash_wyhash_1loc_fixed_s> map(data.size(), hash_wyhash_1loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                std::unordered_map<std::string, int, hash_wyhash_2loc_fixed_s> map(data.size(), hash_wyhash_2loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                std::unordered_map<std::string, int, hash_wyhash_3loc_fixed_s> map(data.size(), hash_wyhash_3loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_wyhash_sub8", map, data, probes);
            } else {
                std::unordered_map<std::string, int, hash_wyhash_4loc_fixed_s> map(data.size(), hash_wyhash_4loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_wyhash_sub8", map, data, probes);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                std::unordered_map<std::string, int, hash_wyhash_1loc_s> map(data.size(), hash_wyhash_1loc_s(hash_locations.data()));
                bench_hashtable("std_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                std::unordered_map<std::string, int, hash_wyhash_2loc_s> map(data.size(), hash_wyhash_2loc_s(hash_locations.data()));
                bench_hashtable("std_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                std::unordered_map<std::string, int, hash_wyhash_3loc_s> map(data.size(), hash_wyhash_3loc_s(hash_locations.data()));
                bench_hashtable("std_wyhash_sub8", map, data, probes);
            } else {
                std::unordered_map<std::string, int, hash_wyhash_4loc_s> map(data.size(), hash_wyhash_4loc_s(hash_locations.data()));
                bench_hashtable("std_wyhash_sub8", map, data, probes);
            }
        }
    }
    if (hashfuncs["crc_wyhash_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                std::unordered_map<std::string, int, hash_crc32wyhash_1loc_fixed_s> map(data.size(), hash_crc32wyhash_1loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_crc_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                std::unordered_map<std::string, int, hash_crc32wyhash_2loc_fixed_s> map(data.size(), hash_crc32wyhash_2loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_crc_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                std::unordered_map<std::string, int, hash_crc32wyhash_3loc_fixed_s> map(data.size(), hash_crc32wyhash_3loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_crc_wyhash_sub8", map, data, probes);
            } else {
                std::unordered_map<std::string, int, hash_crc32wyhash_4loc_fixed_s> map(data.size(), hash_crc32wyhash_4loc_fixed_s(hash_locations.data()));
                bench_hashtable("std_crc_wyhash_sub8", map, data, probes);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                std::unordered_map<std::string, int, hash_crc32wyhash_1loc_s> map(data.size(), hash_crc32wyhash_1loc_s(hash_locations.data()));
                bench_hashtable("std_crc_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                std::unordered_map<std::string, int, hash_crc32wyhash_2loc_s> map(data.size(), hash_crc32wyhash_2loc_s(hash_locations.data()));
                bench_hashtable("std_crc_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                std::unordered_map<std::string, int, hash_crc32wyhash_3loc_s> map(data.size(), hash_crc32wyhash_3loc_s(hash_locations.data()));
                bench_hashtable("std_crc_wyhash_sub8", map, data, probes);
            } else {
                std::unordered_map<std::string, int, hash_crc32wyhash_4loc_s> map(data.size(), hash_crc32wyhash_4loc_s(hash_locations.data()));
                bench_hashtable("std_crc_wyhash_sub8", map, data, probes);
            }
        }
    }
    if (hashfuncs["mmh2_sub4"]) {
        std::vector<int> hash_locations(&locations[4][0],&locations[4][4]);
        std::sort(hash_locations.begin(), hash_locations.end());
        print_locations(hash_locations);
        std::unordered_map<std::string, int, hash_sub4_mmh2_s>map(data.size(), hash_sub4_mmh2_s(hash_locations.data(), hash_locations.size()));
        bench_hashtable("std_mm2_sub4", map, data, probes);
    }
}

// GOOGLE DENSE HASH MAP ----------------------------------------
void bench_google_table(std::vector<std::string>& data, std::vector<std::string>& probes,
                             std::unordered_map<int, std::vector<int>>& locations,
  std::unordered_map<int, std::vector<double>>& entropies) {
    // STANDARD BENCH
    if (hashfuncs["standard"]) {
        google::dense_hash_map<std::string, int, hash_standard_s> map_std(
            data.size());
        map_std.set_empty_key("\0");
        bench_hashtable("google_standard", map_std, data, probes);
    }
}


// currently breaking with sse instructions.. figure out why
/*void bench_f14_table(std::vector<std::string>& data, std::vector<std::string>& probes,
                            std::unordered_map<int, std::vector<int>>& locations,
  std::unordered_map<int, std::vector<int>>& num_locations) {
    // STANDARD BENCH
    if (hashfuncs["default"]) {
        folly::F14FastMap<std::string, int> map(data.size());
        if (table_metrics["latency"]) {
            bench_hashtable_latency("f14_default_latency", map, data, probes);
        }
        if (table_metrics["throughput"]) {
            bench_hashtable("f14_default_throughput", map, data, probes);
        }
    }
    // STANDARD BENCH
    if (hashfuncs["standard"]) {
        folly::F14FastMap<std::string, int, hash_standard_s> map(data.size());
        if (table_metrics["latency"]) {
            bench_hashtable_latency("f14_standard_latency", map, data, probes);
        }
        if (table_metrics["throughput"]) {
            bench_hashtable("f14_standard_throughput", map, data, probes);
        }
    }
    // xxh3 hashing for swiss table
    if (hashfuncs["xxh3"]) {
        folly::F14FastMap<std::string, int, hash_xxh64_s> map(data.size());
        if (table_metrics["latency"]) {
            bench_hashtable_latency("f14_xxh3_latency", map, data, probes);
        }
        if (table_metrics["throughput"]) {
            bench_hashtable("f14_xxh3_throughput", map, data, probes);
        }
    }
    // wyhash hashing
    if (hashfuncs["wyhash"]) {
        folly::F14FastMap<std::string, int, hash_wyhash_s> map(data.size());
        if (table_metrics["latency"]) {
            bench_hashtable_latency("f14_wyhash_latency", map, data, probes);
        }
        if (table_metrics["throughput"]) {
            bench_hashtable("f14_wyhash_throughput", map, data, probes);
        }
    }
    // custom xxh3
    if (hashfuncs["xxh3_sub4"]) {
        std::vector<int> hash_locations(&locations[4][0],&locations[4][4]);
        std::sort(hash_locations.begin(), hash_locations.end());
        folly::F14FastMap<std::string, int, hash_xxh64_sub4_s>map(data.size(), hash_xxh64_sub4_s(hash_locations.data()));
        if (table_metrics["latency"]) {
            bench_hashtable_latency("f14_xxh3_sub4_latency", map, data, probes);
        }
        if (table_metrics["throughput"]) {
            bench_hashtable("f14_xxh3_sub4_throughput", map, data, probes);
        }
    }
    // custom wyhash 
    if (hashfuncs["wyhash_sub8"]) {
        folly::F14FastMap<std::string, int, hash_wyhash_sub8_s> map(data.size());
        if (table_metrics["latency"]) {
            bench_hashtable_latency("f14_wyhash_sub8_latency", map, data, probes);
        }
        if (table_metrics["throughput"]) {
            bench_hashtable("f14_wyhash_sub8_throughput", map, data, probes);
        }
    }
    // custom mmh2
    if (hashfuncs["mmh2_sub4"]) {
        for (int num_locations_hash : num_locations[4]) {
            std::vector<int> hash_locations(&locations[4][0],&locations[4][num_locations_hash]);
            std::sort(hash_locations.begin(), hash_locations.end());
            print_locations(hash_locations);
            folly::F14FastMap<std::string, int, hash_sub4_mmh2_s>map(data.size(), hash_sub4_mmh2_s(hash_locations.data(), hash_locations.size()));
            if (table_metrics["latency"]) {
                bench_hashtable_latency("f14_mm2_sub4_latency", map, data, probes);
            }
            if (table_metrics["throughput"]) {
                bench_hashtable("f14_mm2_sub4_throughput", map, data, probes);
            }
        }
    }
} */


// SWISSTABLE -----------------------------------------------
void bench_swiss_table(std::vector<std::string>& data, std::vector<std::string>& probes,
                            std::unordered_map<int, std::vector<int>>& locations,
                            std::unordered_map<int, std::vector<double>>& entropies) { 
    //std::unordered_map<int, std::vector<int>>& num_locations) {
    std::unordered_map<int, int> numLocationsForChunkSize;
    // log2 (5/2) + log2(n)
    double needed_entropy = log2(5) - 1 + log2(data.size());
	//std::cout << "needed_entropy: " << needed_entropy << std::endl;
    for (auto iter : entropies) {
        numLocationsForChunkSize[iter.first] = iter.second.size()-1;
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
    //std::cout << "HTST needed_ent: " << needed_entropy << ", i: " << numLocationsForChunkSize[8] << ", ent: " << entropies[8][numLocationsForChunkSize[8]-1] << std::endl;
	// set it for synthetic data experiments
	//numLocationsForChunkSize[8] = 1; // hard-code number of used words for synthetic data experiments

    // default hashing for swiss table
    if (hashfuncs["default"]) {
        //absl::flat_hash_map<std::string, int> map(data.size());
        absl::flat_hash_map<std::string, int> map(data.size());
        bench_hashtable("swiss_default", map, data, probes);
    }
    // STANDARD BENCH
    if (hashfuncs["standard"]) {
        absl::flat_hash_map<std::string, int, hash_standard_s> map(data.size());
        bench_hashtable("swiss_standard", map, data, probes);
    }
    if (hashfuncs["crc32"]) {
        absl::flat_hash_map<std::string, int, CRC32Hash_s> map(data.size());
        bench_hashtable("swiss_crc32", map, data, probes);
    }
    if (hashfuncs["mmh2"]) {
        absl::flat_hash_map<std::string, int, hash_mmh2_s> map(data.size());
        bench_hashtable("swiss_mmh2", map, data, probes);
    }
    if (hashfuncs["sdbm"]) {
        absl::flat_hash_map<std::string, int, hash_sdbm_s> map(data.size());
        bench_hashtable("swiss_sdbm", map, data, probes);
    }
    // xxh3 hashing for swiss table
    if (hashfuncs["xxh3"]) {
        absl::flat_hash_map<std::string, int, hash_xxh64_s> map(data.size());
        bench_hashtable("swiss_xxh3", map, data, probes);
    }
    // wyhash hashing
    if (hashfuncs["wyhash"]) {
        absl::flat_hash_map<std::string, int, hash_wyhash_s> map(data.size());
        //bench_hashtable_only_swiss_table("swiss_wyhash", map, data, probes);
        bench_hashtable("swiss_wyhash", map, data, probes);
    }
    // custom xxh3
    if (hashfuncs["xxh3_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                absl::flat_hash_map<std::string, int, hash_XXH3_1loc_fixed_s> map(data.size(), hash_XXH3_1loc_fixed_s(hash_locations.data()));
                bench_hashtable("swiss_xxh3_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_XXH3_2loc_fixed_s> map(data.size(), hash_XXH3_2loc_fixed_s(hash_locations.data()));
                bench_hashtable("swiss_xxh3_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_XXH3_3loc_fixed_s> map(data.size(), hash_XXH3_3loc_fixed_s(hash_locations.data()));
                bench_hashtable("swiss_xxh3_sub8", map, data, probes);
            } else {
                absl::flat_hash_map<std::string, int, hash_XXH3_4loc_fixed_s> map(data.size(), hash_XXH3_4loc_fixed_s(hash_locations.data()));
                bench_hashtable("swiss_xxh3_sub8", map, data, probes);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                absl::flat_hash_map<std::string, int, hash_XXH3_1loc_s> map(data.size(), hash_XXH3_1loc_s(hash_locations.data()));
                bench_hashtable("swiss_xxh3_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_XXH3_2loc_s> map(data.size(), hash_XXH3_2loc_s(hash_locations.data()));
                bench_hashtable("swiss_xxh3_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_XXH3_3loc_s> map(data.size(), hash_XXH3_3loc_s(hash_locations.data()));
                bench_hashtable("swiss_xxh3_sub8", map, data, probes);
            } else {
                absl::flat_hash_map<std::string, int, hash_XXH3_4loc_s> map(data.size(), hash_XXH3_4loc_s(hash_locations.data()));
                bench_hashtable("swiss_xxh3_sub8", map, data, probes);
            }
        }
    }
    // custom wyhash 
    if (hashfuncs["wyhash_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                absl::flat_hash_map<std::string, int, hash_wyhash_1loc_fixed_s> map(data.size(), hash_wyhash_1loc_fixed_s(hash_locations.data()));
                //bench_hashtable_only_swiss_table("swiss_wyhash_sub8", map, data, probes);
                bench_hashtable("swiss_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_wyhash_2loc_fixed_s> map(data.size(), hash_wyhash_2loc_fixed_s(hash_locations.data()));
                bench_hashtable("swiss_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_wyhash_3loc_fixed_s> map(data.size(), hash_wyhash_3loc_fixed_s(hash_locations.data()));
                bench_hashtable("swiss_wyhash_sub8", map, data, probes);
            } else {
                absl::flat_hash_map<std::string, int, hash_wyhash_4loc_fixed_s> map(data.size(), hash_wyhash_4loc_fixed_s(hash_locations.data()));
                bench_hashtable("swiss_wyhash_sub8", map, data, probes);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                absl::flat_hash_map<std::string, int, hash_wyhash_1loc_s> map(data.size(), hash_wyhash_1loc_s(hash_locations.data()));
                //bench_hashtable_only_swiss_table("swiss_wyhash_sub8", map, data, probes);
                bench_hashtable("swiss_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_wyhash_2loc_s> map(data.size(), hash_wyhash_2loc_s(hash_locations.data()));
                bench_hashtable("swiss_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_wyhash_3loc_s> map(data.size(), hash_wyhash_3loc_s(hash_locations.data()));
                bench_hashtable("swiss_wyhash_sub8", map, data, probes);
            } else {
                absl::flat_hash_map<std::string, int, hash_wyhash_4loc_s> map(data.size(), hash_wyhash_4loc_s(hash_locations.data()));
                bench_hashtable("swiss_wyhash_sub8", map, data, probes);
            }
        }
    }
    if (hashfuncs["crc_wyhash_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_1loc_fixed_s> map(data.size(), hash_crc32wyhash_1loc_fixed_s(hash_locations.data()));
                bench_hashtable("swiss_crc_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_2loc_fixed_s> map(data.size(), hash_crc32wyhash_2loc_fixed_s(hash_locations.data()));
                bench_hashtable("swiss_crc_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_3loc_fixed_s> map(data.size(), hash_crc32wyhash_3loc_fixed_s(hash_locations.data()));
                bench_hashtable("swiss_crc_wyhash_sub8", map, data, probes);
            } else {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_4loc_fixed_s> map(data.size(), hash_crc32wyhash_4loc_fixed_s(hash_locations.data()));
                bench_hashtable("swiss_crc_wyhash_sub8", map, data, probes);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_1loc_s> map(data.size(), hash_crc32wyhash_1loc_s(hash_locations.data()));
                bench_hashtable("swiss_crc_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_2loc_s> map(data.size(), hash_crc32wyhash_2loc_s(hash_locations.data()));
                bench_hashtable("swiss_crc_wyhash_sub8", map, data, probes);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_3loc_s> map(data.size(), hash_crc32wyhash_3loc_s(hash_locations.data()));
                bench_hashtable("swiss_crc_wyhash_sub8", map, data, probes);
            } else {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_4loc_s> map(data.size(), hash_crc32wyhash_4loc_s(hash_locations.data()));
                bench_hashtable("swiss_crc_wyhash_sub8", map, data, probes);
            }
        }
    }

    if (hashfuncs["wyhash_sub8_hardcode"]) {
        absl::flat_hash_map<std::string, int, hash_wyhash_hardcode> map(data.size());
        bench_hashtable("swiss_wyhash_sub8_hardcode", map, data, probes);
    }
    // custom mmh2
    if (hashfuncs["mmh2_sub4"]) {
        std::vector<int> hash_locations(&locations[4][0],&locations[4][numLocationsForChunkSize[4]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        print_locations(hash_locations);
        absl::flat_hash_map<std::string, int, hash_sub4_mmh2_s>map(data.size(), hash_sub4_mmh2_s(hash_locations.data(), hash_locations.size()));
        bench_hashtable("swiss_mm2_sub4", map, data, probes);
    }
    if (hashfuncs["sdbm_sub1"]) {
        std::vector<int> hash_locations(&locations[1][0],&locations[1][numLocationsForChunkSize[1]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        print_locations(hash_locations);
        absl::flat_hash_map<std::string, int, hash_sub1_sdbm_s>map(data.size(), hash_sub1_sdbm_s(hash_locations.data(), hash_locations.size()));
        bench_hashtable("swiss_sdbm_sub1", map, data, probes);
    }
}
