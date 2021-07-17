#include <algorithm>
#include <chrono>
#include <random>
#include <unordered_set>

#include <omp.h>

#include <climits>
#include <cmath>
#include <cstdio>
#include <string>
#include <cassert>
#include <absl/container/flat_hash_map.h>

#include "benchmark_filters.hh"
#include "benchmarks.hh"
#include "storage.hh"
#include "workload_generator.hh"
#include "hash.hh"

absl::flat_hash_map<std::string, int, hash_xxh64_s> map_xxh64_full_key_hasher_partition(1);
double total_time_without_dummy_hashing_partition = 0;

void relay_partition_statistics(const std::vector<std::string>& data, const std::vector<int> counts, 
		const std::string partitioner_name, uint64_t dummy) {
	double meanPerPartition = data.size() / counts.size();
	double variance_total = 0.0;
	for (unsigned int i = 0; i < counts.size(); i++) {
		variance_total += pow(counts[i] - meanPerPartition, 2);
	}
	double avg_variance = variance_total / counts.size();
    partition_variances[partitioner_name] += sqrt(avg_variance) / meanPerPartition;
	if(results_directory)
		std::cout << "dummy hashing dummy: " << dummy << std::endl; // just so it does not optimize out dummy
}

// partitions must be a power of two. 
template <class Hash = std::hash<std::string>>
void benchPartition(std::vector<std::string>& data, const std::string partitioner_name, 
	const int numPartitions, const Hash& hashfn = Hash()) {
	assert(log2(numPartitions) == static_cast<int>(log2(numPartitions)));
    double meanPerPartition = data.size() / numPartitions;
	// form mask
	int mask = numPartitions - 1;
	//std::vector<std::vector<std::string>> partitions;
    //partitions.resize(numPartitions, std::vector<std::string>());
    std::vector<std::vector<std::string>> partitions;
	partitions.resize(numPartitions, std::vector<std::string>());
    for (int i = 0; i < numPartitions; i++) {
        partitions[i].reserve(meanPerPartition * 1.2);
    }
	std::vector<int> partitionCounts;
	partitionCounts.resize(numPartitions,0);

	// warmup
	uint64_t dummy = 0;
	for (unsigned i = 0; i < data.size(); i++) {
		int partitionNum = (hashfn.operator()(data[i])) & mask;
		//partitions[partitionNum].push_back(data[i]);
		dummy += partitionNum; // for pure hashing experiment
	}
	for (int i = 0; i < numPartitions; i++) 
		partitions[i].clear();

    benchmark(partitioner_name, "partition",
        [&data, &hashfn, &mask, &partitions, &dummy] {
			size_t step_size = 100;
			std::chrono::time_point<std::chrono::high_resolution_clock> start, end;
            total_time_without_dummy_hashing_partition = 0;
		    for (unsigned i = 0; i < data.size()/step_size; i++) {
				// dummy hashing
                for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
                    dummy += map_xxh64_full_key_hasher_partition.hash_ref()(data[j]);
                }

				// perform lookups
                start = std::chrono::high_resolution_clock::now();
                for (unsigned j = i*step_size; j < (i+1)*step_size; j++) {
					int partitionNum = (hashfn.operator()(data[j])) & mask;
					//partitions[partitionNum].push_back(data[j]);
					dummy += partitionNum; // for pure hashing experiment
				}
				end = std::chrono::high_resolution_clock::now();
                total_time_without_dummy_hashing_partition += (double)
                    std::chrono::duration_cast<std::chrono::nanoseconds>(end - start)
                        .count();
		    }
		    return 0;
        },
        [&partitions, &partitionCounts](int s) {
        	for (unsigned int i = 0; i < partitions.size(); i++) {
        		partitionCounts[i] = partitions[i].size();
        	}
        });
    relay_partition_statistics(data, partitionCounts, partitioner_name, dummy);
}


void bench_partitioners(//const std::vector<std::string>& data,
    std::vector<std::string>& data,
                 std::unordered_map<int, std::vector<int>>& locations, std::unordered_map<int, std::vector<double>>& entropies, 
                 const int numPartitions) {
    std::unordered_map<int, int> numLocationsForChunkSize;
    // Says expected deviation is within 3%.  
    double needed_entropy = log2(1000) + log2(numPartitions);
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
    //std::cout << "Partitioning needed_ent: " << needed_entropy << ", i8: " << numLocationsForChunkSize[8] << ", ent8: " << entropies[8][numLocationsForChunkSize[8]-1] << std::endl;

	if (hashfuncs["standard"]) {
    	benchPartition<hash_standard_s>(data, "partition-std", numPartitions);
    }
    if (hashfuncs["wyhash"]) {
    	benchPartition<hash_wyhash_s>(data, "partition-wyhash", numPartitions);
    }
    if (hashfuncs["xxh3"]) {
    	benchPartition<hash_xxh64_s>(data, "partition-xxh3", numPartitions);
    }
    if (hashfuncs["crc32"]) {
    	benchPartition<CRC32Hash_s>(data, "partition-crc32", numPartitions);
    }
    if (hashfuncs["xxh3_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
            	benchPartition<hash_XXH3_1loc_fixed_s>(data, "partition-xxh3_sub8", numPartitions, hash_XXH3_1loc_fixed_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 2) {
                benchPartition<hash_XXH3_2loc_fixed_s>(data, "partition-xxh3_sub8", numPartitions, hash_XXH3_2loc_fixed_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 3) {
                benchPartition<hash_XXH3_3loc_fixed_s>(data, "partition-xxh3_sub8", numPartitions, hash_XXH3_3loc_fixed_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 4) {
                benchPartition<hash_XXH3_4loc_fixed_s>(data, "partition-xxh3_sub8", numPartitions, hash_XXH3_4loc_fixed_s(hash_locations.data()));
            } else {
                benchPartition<hash_xxh64_s>(data, "partition-xxh3_sub8", numPartitions);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                benchPartition<hash_XXH3_1loc_s>(data, "partition-xxh3_sub8", numPartitions, hash_XXH3_1loc_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 2) {
                benchPartition<hash_XXH3_2loc_s>(data, "partition-xxh3_sub8", numPartitions, hash_XXH3_2loc_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 3) {
                benchPartition<hash_XXH3_3loc_s>(data, "partition-xxh3_sub8", numPartitions, hash_XXH3_3loc_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 4) {
                benchPartition<hash_XXH3_4loc_s>(data, "partition-xxh3_sub8", numPartitions, hash_XXH3_4loc_s(hash_locations.data()));
            } else {
                benchPartition<hash_xxh64_s>(data, "partition-xxh3_sub8", numPartitions);
            }
        }
    }
    if (hashfuncs["wyhash_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                benchPartition<hash_wyhash_1loc_fixed_s>(data, "partition-wyhash_sub8", numPartitions, hash_wyhash_1loc_fixed_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 2) {
                benchPartition<hash_wyhash_2loc_fixed_s>(data, "partition-wyhash_sub8", numPartitions, hash_wyhash_2loc_fixed_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 3) {
                benchPartition<hash_wyhash_3loc_fixed_s>(data, "partition-wyhash_sub8", numPartitions, hash_wyhash_3loc_fixed_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 4) {
                benchPartition<hash_wyhash_4loc_fixed_s>(data, "partition-wyhash_sub8", numPartitions, hash_wyhash_4loc_fixed_s(hash_locations.data()));
            } else {
                benchPartition<hash_wyhash_s>(data, "partition-wyhash_sub8", numPartitions);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                benchPartition<hash_wyhash_1loc_s>(data, "partition-wyhash_sub8", numPartitions, hash_wyhash_1loc_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 2) {
                benchPartition<hash_wyhash_2loc_s>(data, "partition-wyhash_sub8", numPartitions, hash_wyhash_2loc_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 3) {
                benchPartition<hash_wyhash_3loc_s>(data, "partition-wyhash_sub8", numPartitions, hash_wyhash_3loc_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 4) {
                benchPartition<hash_wyhash_4loc_s>(data, "partition-wyhash_sub8", numPartitions, hash_wyhash_4loc_s(hash_locations.data()));
            } else {
                benchPartition<hash_wyhash_s>(data, "partition-wyhash_sub8", numPartitions);
            }
        }
    }
    if (hashfuncs["crc_wyhash_sub8"]) {
        std::vector<int> hash_locations(&locations[8][0],&locations[8][numLocationsForChunkSize[8]]);
        std::sort(hash_locations.begin(), hash_locations.end());
        if(dataIsFixedLength) {
            if (numLocationsForChunkSize[8] == 1) {
                benchPartition<hash_crc32wyhash_1loc_fixed_s>(data, "partition-crc_sub8", numPartitions, hash_crc32wyhash_1loc_fixed_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 2) {
                benchPartition<hash_crc32wyhash_2loc_fixed_s>(data, "partition-crc_sub8", numPartitions, hash_crc32wyhash_2loc_fixed_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 3) {
                benchPartition<hash_crc32wyhash_3loc_fixed_s>(data, "partition-crc_sub8", numPartitions, hash_crc32wyhash_3loc_fixed_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 4) {
                benchPartition<hash_crc32wyhash_4loc_fixed_s>(data, "partition-crc_sub8", numPartitions, hash_crc32wyhash_4loc_fixed_s(hash_locations.data()));
            } else {
                benchPartition<hash_wyhash_s>(data, "partition-crc_sub8", numPartitions);
            }
        } else {
            if (numLocationsForChunkSize[8] == 1) {
                benchPartition<hash_crc32wyhash_1loc_s>(data, "partition-crc_sub8", numPartitions, hash_crc32wyhash_1loc_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 2) {
                benchPartition<hash_crc32wyhash_2loc_s>(data, "partition-crc_sub8", numPartitions, hash_crc32wyhash_2loc_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 3) {
                benchPartition<hash_crc32wyhash_3loc_s>(data, "partition-crc_sub8", numPartitions, hash_crc32wyhash_3loc_s(hash_locations.data()));
            } else if (numLocationsForChunkSize[8] == 4) {
                benchPartition<hash_crc32wyhash_4loc_s>(data, "partition-crc_sub8", numPartitions, hash_crc32wyhash_4loc_s(hash_locations.data()));
            } else {
                benchPartition<CRC32Hash_s>(data, "partition-crc_sub8", numPartitions);
            }
        }
    }
    /*if (hashfuncs["mmh2_sub4"]) {
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
	} */
}
