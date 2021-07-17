#include <algorithm>
#include <unordered_set>
#include <boost/program_options.hpp>
#include <boost/program_options/config.hpp>
#include <stdexcept>
#include <stdarg.h>

#include "benchmarks.hh"
#include "storage.hh"
#include "workload_generator.hh"
#include "entropyEstimation/entropy_estimators.hh"

namespace po = boost::program_options;

// benchmark parameters
float fract_successful = 0.75;
int data_size;
int num_probes;
int str_size = 89;  // size of generated strings (unsuccessful or successful)
int nlocs = 10;
float load_factor;
int num_trials;
bool set_load_factor;
bool hash_probes;
double added_fpr = 0.0;
double bits_per_element = 0.0;
double bits_per_element_rb = 0.0;
const char* results_directory;
std::mutex mylock;

std::unordered_map<std::string, bool> hashfuncs{{"standard", false},
                                                {"sub4_mmh2", false},
                                                {"sub1_sdbm", false},
                                                {"xxh3", false},
                                                {"xxh3_sub8", false},
                                                {"xxh3_trivial", false},
                                                {"default", false},
                                                {"wyhash", false},
                                                {"wyhash_sub8", false},
                                                {"wyhash_sub8_hardcode", false},
                                                {"crc_wyhash_sub8", false},
                                                {"crc32", false},
                                                {"mmh2", false},
                                                {"sdbm", false}};

std::unordered_map<std::string, std::unordered_map<std::string, double>> timings{};
//std::unordered_map<std::string, double> fprs{};
std::unordered_map<std::string, std::unordered_map<std::string, double>> fprs{};
std::unordered_map<std::string, double> partition_variances{};
std::unordered_map<int, std::vector<int>> locations;
std::unordered_map<int, std::vector<double>> entropies;
bool dataIsFixedLength;
std::unordered_map<std::string, uint64_t> num_iters_total;



std::unordered_map<std::string, int> byteChunks{{"standard", 0},
                                               {"sub4_mmh2", 4},
                                               {"sub1_sdbm", 1}};

std::unordered_map<std::string, bool> table_metrics{{"throughput", false},
                                                {"latency", false}};

// see include/benchmark.hh for parameters that modify what functions are
// benchmarked

int max_str_size(std::vector<std::string>& data) {
    unsigned max = 0;
    for (auto& s : data) {
        if (s.length() > max) {
            max = s.length();
        }
    }

    return max;
}

std::pair<po::variables_map, po::options_description> parseCommandLineAndConfigFile(int argc, char** argv, std::string config_file) {
	po::options_description generic_options("Generic options");
	generic_options.add_options()
		("results", po::value<std::string>(), "Directory to write result data to")
		("hashfns", po::value< std::vector<std::string> >(), "List of hash functions to benchmark")
        ("partition_benchmarks", po::value< std::vector<std::string> >()->multitoken()->composing(), "List of benchmarks to perform")
		("filter_benchmarks", po::value< std::vector<std::string> >()->multitoken()->composing(), "List of benchmarks to perform")
        ("hash_benchmarks", po::value< std::vector<std::string> >()->multitoken()->composing(), "List of benchmarks to perform")
        ("table_benchmarks", po::value< std::vector<std::string> >()->multitoken()->composing(), "List of benchmarks to perform")
        ("table_benchmarks_mt", po::value< std::vector<std::string> >()->multitoken()->composing(), "List of benchmarks to perform")
		("num_trials", po::value<int>()->default_value(1), "Trials per benchmark")
		("fract_successful", po::value<float>()->default_value(0.5), "Fraction of probes that are successful")
		("num_probes", po::value<int>()->default_value(1000000), "Number of probes")
        ("num_threads", po::value<int>()->default_value(1), "Number of threads")
		("data_fract", po::value<double>()->default_value(0.75), "Fraction of original data to use")
		("data_size", po::value<int>(), "Subset size of original data to use")
		("random.seed", po::value<int>()->default_value(time(NULL)), "Random seed")
		("dataset", po::value<std::string>(), "Data set file")
        ("table.metrics", po::value< std::vector<std::string> >()->multitoken()->composing(), "Metrics to gather for each hash table")
        ("table.set_load_factor", po::value<bool>()->default_value(false), "use specific load factor")
        ("table.load_factor", po::value<double>()->default_value(0.75), "load factor of hash table")
        ("loc.byteLocations1B", po::value<std::string>(), "file for 1 byte byte locations")
        ("loc.byteLocations4B", po::value<std::string>(), "file for 4 byte byte locations")
        ("loc.byteLocations8B", po::value<std::string>(), "file for 8 byte byte locations")
        ("loc.nlocs1B", po::value<std::vector<int>>()->multitoken()->composing(), "number of bytes to use for 1 byte hash functions")
        ("loc.nlocs4B", po::value<std::vector<int>>()->multitoken()->composing(), "number of bytes to use for 4 byte hash functions")
        ("loc.nlocs8B", po::value<std::vector<int>>()->multitoken()->composing(), "number of bytes to use for 8 byte hash functions")
        ("loc.regenerate", po::value<bool>()->default_value(false), "regenerate locations for hashing")
        ("filter.bits_per_element", po::value<float>()->default_value(8.0), "number of bits for Bloom Filter")
        ("filter.bits_per_element_rb", po::value<float>()->default_value(24.0), "number of bits for Bloom Filter")
        ("filter.added_fpr", po::value<double>()->default_value(0.01), "added fpr for PKH for Bloom Filter")
        //("table.storehash", po::value<bool>()->default_value(false)),
		("help,h", "Print usage");

	// values stored first are preferred.
	// thus command line options are preferred over config file options. 
	po::options_description cmdline_options;
    cmdline_options.add(generic_options);

    po::options_description config_file_options;
    config_file_options.add(generic_options);

	po::variables_map vm;
    store(po::command_line_parser(argc, argv).
          options(cmdline_options).run(), vm);
    std::ifstream ifs(config_file);
    if (!ifs)
    {
        std::cout << "can not open config file: " << config_file << "\n";
    }
    else
    {
        store(parse_config_file(ifs, config_file_options), vm);
    }
    std::cout << "done parsing" << std::endl;
    notify(vm);
    return std::pair<po::variables_map, po::options_description>(vm,generic_options);
}

void read_dataset(std::vector<std::string>& datasetHolder, po::variables_map& vm, std::mt19937& rng) {
    std::string dataset;
    if (vm.count("dataset")) {
        dataset = vm["dataset"].as<std::string>();
    } else {
        throw "No dataset provided";
    }
    printf("dataset: %s\n", dataset.c_str());

    // get data
    datasetHolder = read_lines(dataset.c_str());
    std::cout << "data size initial: " << datasetHolder.size() << std::endl;
    std::shuffle(datasetHolder.begin(), datasetHolder.end(), rng);
}

void setHashFunctions(po::variables_map& vm) {
    if (vm.count("hashfns")) {
        std::vector<std::string> requested_hashfns = vm["hashfns"].as<std::vector<std::string>>();
        for (auto& f : requested_hashfns) {
            hashfuncs[f] = true;
        }
    } else {
        throw "No hash functions provided";
    }
}

void setTableMetrics(po::variables_map& vm) {
    if (vm.count("table.metrics")) {
        std::vector<std::string> metrics = vm["table.metrics"].as<std::vector<std::string>>();
        for (auto& metric : metrics) {
            table_metrics[metric] = true;
        }
    } else {
        throw "Benchmarking hash tables but no metrics given";
    }
}
void setLocationData(po::variables_map& vm) {

    if (vm.count("loc.byteLocations1B")) {
        std::tie(locations[1],entropies[1], dataIsFixedLength) = read_entropies(vm["loc.byteLocations1B"].as<std::string>().c_str());
    }
    if (vm.count("loc.byteLocations4B")) {
        std::tie(locations[4],entropies[4], dataIsFixedLength) = read_entropies(vm["loc.byteLocations4B"].as<std::string>().c_str());
    }
    if (vm.count("loc.byteLocations8B")) {
        std::tie(locations[8],entropies[8], dataIsFixedLength) = read_entropies(vm["loc.byteLocations8B"].as<std::string>().c_str());
    } 
}

void generateLocationData(std::mt19937& rng, std::vector<std::string>& data, po::variables_map& vm) {
    std::vector<int> locations;
    std::vector<double> entropies;
    bool fixedLengthData;
    double entropy;
    std::vector<std::string> sample_val;
    std::vector<std::string> sample_data;
    std::tie(sample_data, sample_val) = get_subset(rng, data, 0.5);
    // 1 byte
    if (vm.count("loc.byteLocations1B")) {
        entropy = entropy_estimators::greedyRenyiSelectorStopK<uint8_t>(max_str_size(data), sample_data, sample_val, locations, entropies, fixedLengthData, 16);
        write_locations(locations, vm["loc.byteLocations1B"].as<std::string>().c_str());
        write_entropies(entropies, locations, fixedLengthData, (vm["loc.byteLocations1B"].as<std::string>()).c_str());
        locations.clear();entropies.clear();
    }
    // 4 byte
    if (vm.count("loc.byteLocations4B")) {
        entropy = entropy_estimators::greedyRenyiSelectorStopK<uint32_t>(max_str_size(data), sample_data, sample_val, locations, entropies, fixedLengthData, 16);
        write_locations(locations, vm["loc.byteLocations4B"].as<std::string>().c_str());
        write_entropies(entropies, locations, fixedLengthData, (vm["loc.byteLocations4B"].as<std::string>()).c_str());
        locations.clear();entropies.clear();
    }
    // 8 byte
    if (vm.count("loc.byteLocations8B")) {
        entropy = entropy_estimators::greedyRenyiSelectorStopK<uint64_t>(max_str_size(data), sample_data, sample_val, locations, entropies, fixedLengthData, 12);
        //write_locations(locations, vm["loc.byteLocations8B"].as<std::string>().c_str());
        write_entropies(entropies, locations, fixedLengthData, (vm["loc.byteLocations8B"].as<std::string>()).c_str());
        locations.clear();entropies.clear();
    }
}

void print_variances() {
    std::cout << "\nMetric: Relative Variance of Partitions" << std::endl;
    std::cout << "\n Relative Variance = Std. Dev / Mean" << std::endl;
    for (std::pair<std::string, double> element : partition_variances) {
        double variance_avg = element.second / (1.0 * num_trials);
        std::cout << element.first << " - " << variance_avg << std::endl;
    }
}

void print_fprs(std::string metric) {
    std::cout << "\nFPR Metric: " << metric << std::endl;
    for (std::pair<std::string, double> element : fprs[metric]) {
        double fpr_avg = element.second / (1.0 * num_trials);
        std::cout << element.first << " - " << fpr_avg << std::endl;
    } 
}

void print_benchmark_results(std::string metric) {
	if (metric == "hash_table_build") {
		std::cout << "\nMetric: "<< metric << std::endl;
		for (std::pair<std::string, double> element : timings[metric]) {
			double outputTime = element.second / (1.0 * data_size * num_trials);
			std::cout << element.first << " - " << outputTime << std::endl;
		}
	}
	else {
		std::cout << "\nMetric: "<< metric << std::endl;
        for (std::pair<std::string, double> element : timings[metric]) {
            double outputTime = element.second / (1.0 * num_probes * num_trials);
            std::cout << element.first << " - " << outputTime << std::endl;
        }
	}
}

void print_benchmark_results_mt(std::string metric, int num_threads) {
    std::cout << "\nMetric: "<< metric << std::endl;
    for (std::pair<std::string, double> element : timings[metric]) {
        double outputTime = element.second / (1.0 * num_probes * num_iters_total[element.first]);
        std::cout << element.first << " - " << outputTime << std::endl;
    }
}

void print_if_first(bool first, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    if(first)
        vprintf(format, args);

    va_end(args);
}

void read_dataset_static(std::vector<std::string>& datasetHolder, po::variables_map& vm) {
    std::string dataset;
    if (vm.count("dataset")) {
        dataset = vm["dataset"].as<std::string>();
    } else {
        throw "No dataset provided";
    }
    printf("dataset: %s\n", dataset.c_str());

    // get data
    datasetHolder = read_lines(dataset.c_str());
    std::cout << "data size initial: " << datasetHolder.size() << std::endl;
}

void initialize_args(po::variables_map& vm) {
    if (vm.count("filter_benchmarks")) {
        fprs["filter_throughput"] = std::unordered_map<std::string, double>();
        fprs["filter_latency"] = std::unordered_map<std::string, double>();
        timings["filter_throughput"] = std::unordered_map<std::string, double>();
        timings["filter_latency"] = std::unordered_map<std::string, double>();
    }
    if (vm.count("hash_benchmarks")) {
        timings["hash_func"] = std::unordered_map<std::string, double>();
    }
    if (vm.count("table_benchmarks")) {
        timings["hash_table_throughput"] = std::unordered_map<std::string, double>();
        timings["hash_table_latency"] = std::unordered_map<std::string, double>();
        timings["hash_table_build"] = std::unordered_map<std::string, double>();
    }
    if (vm.count("table_benchmarks_mt")) {
        timings["hash_table_throughput_mt"] = std::unordered_map<std::string, double>();
    }
    if (vm.count("partition_benchmarks")) {
        timings["partition"] = std::unordered_map<std::string, double>();
    }
}

int main(int argc, char** argv) {
    std::string config_file = "./config/options.cfg";
    std::pair<po::variables_map, po::options_description> pair = parseCommandLineAndConfigFile(argc,argv, config_file);
    po::variables_map vm = pair.first;
    po::options_description options = pair.second;

    if (vm.count("help")) {
        std::cout << options << std::endl;
        exit(0);
    }
    int seed = time(NULL);
    if (vm.count("seed")) {
        seed = vm["seed"].as<int>();
    }
    printf("Seed: %d\n", seed);
    // seed random number generator
    srand(seed);
    std::random_device rd;
    std::mt19937 rng(seed);

    std::vector<std::string> full_data;
    std::vector<int> locations1, locations4, nlocs1, nlocs4;
    try {
        //read_dataset_static(full_data, vm);
        read_dataset(full_data, vm, rng);
        setHashFunctions(vm);
        if (vm.count("loc.regenerate") && vm["loc.regenerate"].as<bool>()) {
            generateLocationData(rng, full_data, vm);
        }
        setLocationData(vm);
    } catch(const char* msg) {
        std::cout << msg << std::endl;
        exit(0);
    }

    int num_threads = vm["num_threads"].as<int>();
    // prepare benchmarks
    num_probes = vm["num_probes"].as<int>();
    fract_successful = vm["fract_successful"].as<float>();
    num_trials = vm["num_trials"].as<int>();
    bits_per_element = vm["filter.bits_per_element"].as<float>();
    bits_per_element_rb = vm["filter.bits_per_element_rb"].as<float>();
    //store_hash = vm["table.store_hash"].as<bool>();
    if (vm.count("results")) {
        results_directory = vm["results"].as<std::string>().c_str();
    }

    double fract = vm["data_fract"].as<double>();
    if (vm.count("data_size")) 
		data_size = vm["data_size"].as<int>();
	else 
		data_size = full_data.size();
	fract = (double) data_size / (double) full_data.size();

    if (vm.count("filter.added_fpr")) {
        added_fpr = vm["filter.added_fpr"].as<double>();
    }
    std::vector<std::string> uninserted;
    std::vector<std::string> data;
    bool first = true;
    initialize_args(vm);
    for (int i = 0; i < num_trials; i++) {
        data.clear();
        uninserted.clear();
        // get data subset
        if (fract != 1.0) {
            std::tie(data, uninserted) = get_subset(rng, full_data, fract);
            //std::tie(data, uninserted) = get_subset_static(full_data, fract);
        } else {
            data = full_data;
        }
        print_if_first(first, "data size: %zu\n", data.size());  
        print_if_first(first, "uninserted size: %zu\n", uninserted.size());
        
        std::vector<std::string> probes;
        int numProbesUnsuccessful;
        std::tie(probes,numProbesUnsuccessful) = generate_probes_with_unselected_keys_as_false(rng, data, uninserted, fract_successful, num_probes);
        //std::tie(probes,numProbesUnsuccessful) = generate_probes_with_unselected_keys_as_false_static(data, uninserted, fract_successful, num_probes);

        // run partitioning benchmarks
        std::vector<std::string> requested_partition_benches;
        if (vm.count("partition_benchmarks")) {
            requested_partition_benches = vm["partition_benchmarks"].as<std::vector<string>>();
            for (auto& f : requested_partition_benches) {
                print_if_first(first, "\nBENCH: %s\n", f.c_str());
                auto fn = partition_benchmarks.find(f)->second;
                print_if_first(first, "DATASIZE: %lu\n", data.size());
                fn(data, locations, entropies, 1024);
            }
        }
        // run filter benchmarks
        std::vector<std::string> requested_filter_benches;
        if (vm.count("filter_benchmarks")) {
            requested_filter_benches = vm["filter_benchmarks"].as<std::vector<std::string>>();
			setTableMetrics(vm);
            for (auto& f : requested_filter_benches) {
                print_if_first(first, "\nBENCH: %s\n", f.c_str());
                auto fn = filter_benchmarks.find(f)->second;
                print_if_first(first, "DATASIZE: %lu\n", data.size());
                fn(data, probes, locations, entropies, numProbesUnsuccessful);
            }
        } 
        // run hash benchmarks
        std::vector<std::string> requested_hash_benches;
        if (vm.count("hash_benchmarks")) {
            requested_hash_benches = vm["hash_benchmarks"].as<std::vector<std::string>>();
            for (auto& f : requested_hash_benches) {
                print_if_first(first, "\nBENCH: %s\n", f.c_str());
                auto fn = hash_benchmarks.find(f)->second;
                print_if_first(first, "DATASIZE: %lu\n", data.size());
                fn(data, probes, locations, entropies);
            }
        }
        // run hash table benchmarks
        std::vector<std::string> requested_table_benches;
        set_load_factor = vm["table.set_load_factor"].as<bool>();
        load_factor = vm["table.load_factor"].as<double>();
        if (vm.count("table_benchmarks")) {
            requested_table_benches = vm["table_benchmarks"].as<std::vector<std::string>>();
            setTableMetrics(vm);
            for (auto& f : requested_table_benches) {
                print_if_first(first, "\nBENCH: %s\n", f.c_str());
                auto fn = table_benchmarks.find(f)->second;
                print_if_first(first, "DATASIZE: %lu\n", data.size());
                fn(data, probes, locations, entropies);
            }
        }
        first = false;
        if (vm.count("table_benchmarks_mt")) {
            bench_swiss_table_mt(data, uninserted, num_probes, fract_successful,
                num_threads, locations, entropies);
        }
    }
    if (vm.count("partition_benchmarks")) {
        print_variances();
        print_benchmark_results("partition");
    }
    if (vm.count("filter_benchmarks")) {
        print_fprs("filter_throughput");
        print_fprs("filter_latency");
        print_benchmark_results("filter_throughput");
        print_benchmark_results("filter_latency");
    }
    if (vm.count("hash_benchmarks")) {
        print_benchmark_results("hash_func");
    }
    if (vm.count("table_benchmarks")) {
        print_benchmark_results("hash_table_throughput");
        print_benchmark_results("hash_table_latency");
        //print_benchmark_results("hash_table_build");
    }
    if (vm.count("table_benchmarks_mt")) {
        print_benchmark_results_mt("hash_table_throughput_mt", num_threads);
    }
}
