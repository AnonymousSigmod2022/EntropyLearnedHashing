#include <algorithm>
#include <chrono>
#include <random>
#include <unordered_set>
#include <thread>

#include <limits>
#include <math.h>
#include <time.h>
#include <cstdio>
#include <cinttypes>


//#include <boost/thread/barrier.hpp>

#include <robin_map.h>
#include <google/dense_hash_map>
#include <absl/container/flat_hash_map.h>

#include <sys/time.h>

#include "hash.hh"
#include "benchmarks.hh"
#include "storage.hh"
#include "workload_generator.hh"

/**
 * Barrier implemented by spinning, taken from Silo: https://github.com/stephentu/silo
 */
void nop_pause()
{
  __asm volatile("pause" : :);
}

class spin_barrier {
public:
  spin_barrier(size_t n)
    : n(n)
  {
  }

  spin_barrier(const spin_barrier &) = delete;
  spin_barrier(spin_barrier &&) = delete;
  spin_barrier &operator=(const spin_barrier &) = delete;

  void set_n(size_t narg) {
    this->n = narg;
  }

  ~spin_barrier()
  {
  }

  void
  count_down()
  {
    // written like this (instead of using __sync_fetch_and_add())
    // so we can have assertions
    for (;;) {
      size_t copy = n;
      if (__sync_bool_compare_and_swap(&n, copy, copy - 1))
        return;
    }
  }

  void
  wait_for()
  {
    while (n > 0)
      nop_pause();
  }

private:
  volatile size_t n;
};

// dummy hasher
absl::flat_hash_map<std::string, int, hash_xxh64_s> map_xxh64_full_key_hasher_mt(1);
// synchronization variables
volatile bool running = false;
volatile bool warmup = false;
spin_barrier barrier_a(1); // for warmup
spin_barrier barrier_b(1); // for profiling
uint64_t* thread_num_iters;

template <class M>
void bench_hashtable_throughput_mt(const char* name,
                     const M& map,
                     const std::vector<std::string>& probes, int thread_id) {

    auto p = probes;

	// warm-up
	barrier_a.count_down();
    barrier_a.wait_for();
	int64_t s = 0;
	while(warmup) {
		for (unsigned i = 0; i < p.size(); i++) {
			auto it = map.find(p[i]);
			if (it != map.end()) {
				s += it->second;
			}
		}
	}

	// profiling
    benchmark_mt(
        name, "hash_table_throughput_mt", 
        [&p, &map, &s, &thread_id] {
			barrier_b.count_down();
			barrier_b.wait_for();

			while(running) {
				for (unsigned i = 0; i < p.size(); i++) {
					auto it = map.find(p[i]);
					if (it != map.end()) {
						s += it->second;
					}
				}
				thread_num_iters[thread_id]++;
			}

            return s;
     },
        [](int64_t s) {
            printf("%" PRId64 "\r", s);
            fflush(stdout);
        });
}

template <class M>
void bench_hashtable_st(const char* name, const M& map, const std::vector<std::string>& data,
                     const std::vector<std::string>& possible_probes, int num_probes,
                     const double fract_successful) {
	std::vector<std::string> probes;
	double numProbesUnsuccessful;
	// rng + generate probes
	int seed = time(NULL);
    srand(seed);
    std::random_device rd;
    std::mt19937 rng(seed);
	std::tie(probes,numProbesUnsuccessful) = generate_probes_with_unselected_keys_as_false(rng, data, possible_probes, fract_successful, num_probes);
	// wait here. 
	//barrier.wait();
	bench_hashtable_throughput_mt(name, map, probes);
}

template <class M>
void bench_hashtable_mt(const char* name, M& map, const std::vector<std::string>& data,
                     const std::vector<std::string>& possible_probes, int num_probes,
                     const double fract_successful, const int num_threads) {
	// put data into map (before queries start). 
	for (unsigned i = 0; i < data.size(); i++) {
        map[data[i]] = i;
    }
    int seed = time(NULL);
    srand(seed);
    std::random_device rd;
    std::mt19937 rng(seed);
    std::vector<std::thread> thread_vec;
    //boost::barrier task_barrier(num_threads);
    std::vector<std::vector<std::string>> probeList;
    for(int i = 0; i < num_threads; i++) {
    	std::vector<std::string> probes;
		double numProbesUnsuccessful;
    	std::tie(probes,numProbesUnsuccessful) = generate_probes_with_unselected_keys_as_false(rng, data, possible_probes, fract_successful, num_probes);
    	probeList.emplace_back(std::move(probes));
    }
	// set synchronization variables 
	barrier_a.set_n(num_threads);
	barrier_b.set_n(num_threads);
	warmup = false;
	running = false;

	// allocate num_iter array
	thread_num_iters = (uint64_t *) malloc(num_threads*sizeof(uint64_t));
	for(int i = 0; i < num_threads; i++)
		thread_num_iters[i] = 0;

	std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
    for (int i = 0; i < num_threads; i++) {
        thread_vec.push_back(std::thread(bench_hashtable_throughput_mt<M>, 
        	name,std::cref(map), std::cref(probeList[i]), i));

		// pin thread, core_ids for adama: 1, 5, 9, 13, 17, 21, 25, 29 -- i*4+1 -- all separately physical cores
		// Create a cpu_set_t object representing a set of CPUs. Clear it and mark
		// only CPU i as set.
		#if __linux__
        cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		int core_id = i*4+1;
		CPU_SET(core_id, &cpuset);
		int rc = pthread_setaffinity_np(thread_vec[i].native_handle(),
										sizeof(cpu_set_t), &cpuset);
		if (rc != 0) {
		  std::cerr << "Error calling pthread_setaffinity_np: " << rc << "\n";
		} 
        #endif
    }

	int warmup_duration = 5; // in seconds
	// warmup
	warmup = true;
	sleep(warmup_duration);
	warmup = false;
	
	// profiling
	start = std::chrono::high_resolution_clock::now();
	running = true;
	sleep(2*warmup_duration);
	running = false;
    for (int i = 0; i < num_threads; i++) {
    	thread_vec[i].join();
    }
	end = std::chrono::high_resolution_clock::now();
	auto duration =
        std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
            .count();
	
	for (int i = 0; i < num_threads; i++)
		num_iters_total[name] += thread_num_iters[i];

	timings["hash_table_throughput_mt"][name] += duration;
}

// SWISSTABLE -----------------------------------------------
void bench_swiss_table_mt(std::vector<std::string>& data, const std::vector<std::string>& possible_probes,
  int num_probes, double fract_successful, int num_threads, 
  std::unordered_map<int, std::vector<int>>& locations, std::unordered_map<int, std::vector<double>>& entropies) { 
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
    //std::cout << "HT needed_ent: " << needed_entropy << ", i: " << numLocationsForChunkSize[8] << ", ent: " << entropies[8][numLocationsForChunkSize[8]-1] << std::endl;
	// set it for synthetic data experiments
	//numLocationsForChunkSize[8] = 1; // hard-code number of used words for synthetic data experiments

    // default hashing for swiss table
    if (hashfuncs["default"]) {
        //absl::flat_hash_map<std::string, int> map(data.size());
        absl::flat_hash_map<std::string, int> map(data.size());
        bench_hashtable_mt("swiss_default", map, data, possible_probes, num_probes, fract_successful, num_threads);
    }
    // STANDARD BENCH
    if (hashfuncs["standard"]) {
        absl::flat_hash_map<std::string, int, hash_standard_s> map(data.size());
        bench_hashtable_mt("swiss_standard", map, data, possible_probes, num_probes, fract_successful, num_threads);
    }
    if (hashfuncs["crc32"]) {
        absl::flat_hash_map<std::string, int, CRC32Hash_s> map(data.size());
        bench_hashtable_mt("swiss_crc32", map, data, possible_probes, num_probes, fract_successful, num_threads);
    }
    if (hashfuncs["mmh2"]) {
        absl::flat_hash_map<std::string, int, hash_mmh2_s> map(data.size());
        bench_hashtable_mt("swiss_mmh2", map, data, possible_probes, num_probes, fract_successful, num_threads);
    }
    if (hashfuncs["sdbm"]) {
        absl::flat_hash_map<std::string, int, hash_sdbm_s> map(data.size());
        bench_hashtable_mt("swiss_sdbm", map, data, possible_probes, num_probes, fract_successful, num_threads);
    }
    // xxh3 hashing for swiss table
    if (hashfuncs["xxh3"]) {
        absl::flat_hash_map<std::string, int, hash_xxh64_s> map(data.size());
        bench_hashtable_mt("swiss_xxh3", map, data, possible_probes, num_probes, fract_successful, num_threads);
    }
    // wyhash hashing
    if (hashfuncs["wyhash"]) {
        absl::flat_hash_map<std::string, int, hash_wyhash_s> map(data.size());
        //bench_hashtable_mt_only_swiss_table("swiss_wyhash", map, data, possible_probes, num_probes);
        bench_hashtable_mt("swiss_wyhash", map, data, possible_probes, num_probes, fract_successful, num_threads);
    }
    // custom xxh3
    if (hashfuncs["xxh3_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                absl::flat_hash_map<std::string, int, hash_XXH3_1loc_fixed_s> map(data.size(), hash_XXH3_1loc_fixed_s(hash_locations.data()));
                bench_hashtable_mt("swiss_xxh3_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_XXH3_2loc_fixed_s> map(data.size(), hash_XXH3_2loc_fixed_s(hash_locations.data()));
                bench_hashtable_mt("swiss_xxh3_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_XXH3_3loc_fixed_s> map(data.size(), hash_XXH3_3loc_fixed_s(hash_locations.data()));
                bench_hashtable_mt("swiss_xxh3_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else {
                absl::flat_hash_map<std::string, int, hash_XXH3_4loc_fixed_s> map(data.size(), hash_XXH3_4loc_fixed_s(hash_locations.data()));
                bench_hashtable_mt("swiss_xxh3_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                absl::flat_hash_map<std::string, int, hash_XXH3_1loc_s> map(data.size(), hash_XXH3_1loc_s(hash_locations.data()));
                bench_hashtable_mt("swiss_xxh3_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_XXH3_2loc_s> map(data.size(), hash_XXH3_2loc_s(hash_locations.data()));
                bench_hashtable_mt("swiss_xxh3_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_XXH3_3loc_s> map(data.size(), hash_XXH3_3loc_s(hash_locations.data()));
                bench_hashtable_mt("swiss_xxh3_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else {
                absl::flat_hash_map<std::string, int, hash_XXH3_4loc_s> map(data.size(), hash_XXH3_4loc_s(hash_locations.data()));
                bench_hashtable_mt("swiss_xxh3_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
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
                //bench_hashtable_mt_only_swiss_table("swiss_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
                bench_hashtable_mt("swiss_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_wyhash_2loc_fixed_s> map(data.size(), hash_wyhash_2loc_fixed_s(hash_locations.data()));
                bench_hashtable_mt("swiss_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_wyhash_3loc_fixed_s> map(data.size(), hash_wyhash_3loc_fixed_s(hash_locations.data()));
                bench_hashtable_mt("swiss_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else {
                absl::flat_hash_map<std::string, int, hash_wyhash_4loc_fixed_s> map(data.size(), hash_wyhash_4loc_fixed_s(hash_locations.data()));
                bench_hashtable_mt("swiss_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                absl::flat_hash_map<std::string, int, hash_wyhash_1loc_s> map(data.size(), hash_wyhash_1loc_s(hash_locations.data()));
                //bench_hashtable_mt_only_swiss_table("swiss_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
                bench_hashtable_mt("swiss_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_wyhash_2loc_s> map(data.size(), hash_wyhash_2loc_s(hash_locations.data()));
                bench_hashtable_mt("swiss_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_wyhash_3loc_s> map(data.size(), hash_wyhash_3loc_s(hash_locations.data()));
                bench_hashtable_mt("swiss_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else {
                absl::flat_hash_map<std::string, int, hash_wyhash_4loc_s> map(data.size(), hash_wyhash_4loc_s(hash_locations.data()));
                bench_hashtable_mt("swiss_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            }
        }
    }
    if (hashfuncs["crc_wyhash_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_1loc_fixed_s> map(data.size(), hash_crc32wyhash_1loc_fixed_s(hash_locations.data()));
                bench_hashtable_mt("swiss_crc_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_2loc_fixed_s> map(data.size(), hash_crc32wyhash_2loc_fixed_s(hash_locations.data()));
                bench_hashtable_mt("swiss_crc_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_3loc_fixed_s> map(data.size(), hash_crc32wyhash_3loc_fixed_s(hash_locations.data()));
                bench_hashtable_mt("swiss_crc_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_4loc_fixed_s> map(data.size(), hash_crc32wyhash_4loc_fixed_s(hash_locations.data()));
                bench_hashtable_mt("swiss_crc_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_1loc_s> map(data.size(), hash_crc32wyhash_1loc_s(hash_locations.data()));
                bench_hashtable_mt("swiss_crc_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 2) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_2loc_s> map(data.size(), hash_crc32wyhash_2loc_s(hash_locations.data()));
                bench_hashtable_mt("swiss_crc_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else if (numLocationsForChunkSize[8] == 3) {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_3loc_s> map(data.size(), hash_crc32wyhash_3loc_s(hash_locations.data()));
                bench_hashtable_mt("swiss_crc_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            } else {
                absl::flat_hash_map<std::string, int, hash_crc32wyhash_4loc_s> map(data.size(), hash_crc32wyhash_4loc_s(hash_locations.data()));
                bench_hashtable_mt("swiss_crc_wyhash_sub8", map, data, possible_probes, num_probes, fract_successful, num_threads);
            }
        }
    }

    if (hashfuncs["wyhash_sub8_hardcode"]) {
        absl::flat_hash_map<std::string, int, hash_wyhash_hardcode> map(data.size());
        bench_hashtable_mt("swiss_wyhash_sub8_hardcode", map, data, possible_probes, num_probes, fract_successful, num_threads);
    }
    // custom mmh2
    if (hashfuncs["mmh2_sub4"]) {
        std::vector<int> hash_locations(&locations[4][0],&locations[4][numLocationsForChunkSize[4]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        print_locations(hash_locations);
        absl::flat_hash_map<std::string, int, hash_sub4_mmh2_s>map(data.size(), hash_sub4_mmh2_s(hash_locations.data(), hash_locations.size()));
        bench_hashtable_mt("swiss_mm2_sub4", map, data, possible_probes, num_probes, fract_successful, num_threads);
    }
    if (hashfuncs["sdbm_sub1"]) {
        std::vector<int> hash_locations(&locations[1][0],&locations[1][numLocationsForChunkSize[1]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        print_locations(hash_locations);
        absl::flat_hash_map<std::string, int, hash_sub1_sdbm_s>map(data.size(), hash_sub1_sdbm_s(hash_locations.data(), hash_locations.size()));
        bench_hashtable_mt("swiss_sdbm_sub1", map, data, possible_probes, num_probes, fract_successful, num_threads);
    }
}
