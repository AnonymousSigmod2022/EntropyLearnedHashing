
#include <algorithm>
#include <random>
#include <unordered_set>
#include <cstdint>

#include <omp.h>

#include <climits>
#include <cmath>
#include <cstdio>
#include <string>

#include "workload_generator.hh"

// choose top N keys
void generate_unsuccessful_probes_static(
    const std::vector<std::string>& uninserted_keys,
    unsigned int nprobes,
    std::vector<std::string>& probe_array) {

    int size = uninserted_keys.size();
    for (unsigned int i = 0; i < nprobes; i++) {
        probe_array.push_back(uninserted_keys[i%size]);
    }
}

// choose top N keys
void generate_successful_probes_static(
    const std::vector<std::string>& inserted_keys,
    unsigned int nprobes,
    std::vector<std::string>& probe_array) {

    int size = inserted_keys.size();
    for (unsigned int i = 0; i < nprobes; i++) {
        probe_array.push_back(inserted_keys[i%size]);
    }
}

std::pair<std::vector<std::string>,int> generate_probes_with_unselected_keys_as_false_static(
    const std::vector<std::string>& inserted_keys,
    const std::vector<std::string>& uninserted_keys,
    float fraction_successful,
    unsigned int nprobes) {
    unsigned int successful = (unsigned int) (fraction_successful * (float) nprobes);
    unsigned int unsuccessful = nprobes - successful;
    std::vector<std::string> probes;

    generate_successful_probes_static(inserted_keys, successful, probes);
    generate_unsuccessful_probes_static(uninserted_keys, unsuccessful, probes);

    return std::make_pair(probes, unsuccessful);
}

std::vector<std::string> uniform_random_strings(std::mt19937& rng,
                                                int num_strings,
                                                int str_size) {
    std::uniform_int_distribution<uint8_t> uni(1, 255);

    std::vector<std::string> keys;
    keys.reserve(num_strings);

    char* s = (char*) malloc(str_size + 1);

    for (int n = 0; n < num_strings; n++) {
        for (int i = 0; i < str_size; i++) {
            s[i] = uni(rng);
        }
        s[str_size] = '\0';

        std::string str(s);
        keys.push_back(str);
    }

    free(s);

    return keys;
}

void generate_successful_probes(std::mt19937& rng,
    const std::vector<std::string>& inserted_keys,
    unsigned int nprobes,
    std::vector<std::string>& probe_array) {
	
	std::uniform_int_distribution<uint32_t> success_gen(
        0, inserted_keys.size() - 1);
	for (unsigned int i = 0; i < nprobes; i++) {
        int idx = success_gen(rng);
        probe_array.push_back(inserted_keys[idx]);
    }
}

void generate_unsuccessful_probes(std::mt19937& rng,
    const std::vector<std::string>& uninserted_keys,
    unsigned int nprobes,
    std::vector<std::string>& probe_array) {

	std::uniform_int_distribution<uint32_t> unsuccess_gen(
        0, uninserted_keys.size() - 1);
	for (unsigned int i = 0; i < nprobes; i++) {
        int idx = unsuccess_gen(rng);
        probe_array.push_back(uninserted_keys[idx]);
    }
}

void generate_random_unsuccessful_probes(std::mt19937& rng,
    unsigned int str_size,
    unsigned int nprobes,
    std::vector<std::string>& probe_array) {
    std::uniform_int_distribution<uint8_t> uni(1, 255);

	char* s = (char*) malloc(str_size + 1);
    for (unsigned int i = 0; i < nprobes; i++) {
        for (unsigned int j = 0; j < str_size; j++) {
            s[j] = uni(rng);
        }
        s[str_size] = '\0';

        std::string str(s);
        probe_array.push_back(str);
    }
    free(s);
}

std::pair<std::vector<std::string>,int> generate_probes_with_unselected_keys_as_false(
    std::mt19937& rng,
    const std::vector<std::string>& inserted_keys,
    const std::vector<std::string>& uninserted_keys,
    float fraction_successful,
    unsigned int nprobes) {
    unsigned int successful = (unsigned int) (fraction_successful * (float) nprobes);
    unsigned int unsuccessful = nprobes - successful;
    std::vector<std::string> probes;

    generate_successful_probes(rng, inserted_keys, successful, probes);
    generate_unsuccessful_probes(rng, uninserted_keys, unsuccessful, probes);

    std::shuffle(probes.begin(), probes.end(), rng);

    return std::make_pair(probes, unsuccessful);
}

std::pair<std::vector<std::string>, int> generate_probes_with_random_false(std::mt19937& rng,
                                         const std::vector<std::string>& keys,
                                         float fraction_successful,
                                         unsigned int nprobes,
                                         unsigned int str_size) {
    unsigned int successful = (unsigned int) (fraction_successful * (float) nprobes);
    unsigned int unsuccessful = nprobes - successful;

    std::uniform_int_distribution<uint8_t> uni(48, 122);
    std::uniform_int_distribution<uint32_t> index_gen(0, keys.size() - 1);

    std::vector<std::string> probes;
    probes.reserve(nprobes);
    generate_successful_probes(rng, keys, successful, probes);
    generate_random_unsuccessful_probes(rng, str_size, unsuccessful, probes);

    std::shuffle(probes.begin(), probes.end(), rng);

    return std::make_pair(probes, unsuccessful);
}
