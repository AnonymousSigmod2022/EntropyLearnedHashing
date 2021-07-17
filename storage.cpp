#include "storage.hh"

#include <cassert>
#include <algorithm>
#include <random>
#include <sstream>
#include <tuple>

#include <boost/range.hpp>


void add_to_file(const char* fname, double val) {
    std::ofstream outfile;

    outfile.open(fname, std::ios_base::app);
    outfile << val << std::endl;

    outfile.close();
}

std::vector<std::string> read_lines(const char* filename) {
    std::ifstream infile(filename);

	int i = 0;
    std::vector<std::string> lines;
    for (std::string line; getline(infile, line);) {
        lines.push_back(line);
		i++;
		if(i >= 210000000)
			break;
    }

    return lines;
}

// choose top N keys 
std::pair<std::vector<std::string>, std::vector<std::string>>
get_subset_static(std::vector<std::string>& data, float fraction) {
    assert(fraction <= 1.0 && fraction >= 0.0);

    unsigned nitems = static_cast<unsigned>(fraction * (float) data.size());

    std::vector<std::string> subset;
    subset.reserve(nitems);
    std::vector<std::string> remaining;
    remaining.reserve(data.size() - nitems);
    for (unsigned i = 0; i < data.size(); i++) {
        if (i < nitems) {
            subset.push_back(data[i]);
        } else {
            remaining.push_back(data[i]);
        }
    }
    return std::make_pair(subset, remaining);
}

std::pair<std::vector<std::string>, std::vector<std::string>>
get_subset(std::mt19937& rng, std::vector<std::string>& data, float fraction) {
    assert(fraction <= 1.0 && fraction >= 0.0);

    unsigned nitems = static_cast<unsigned>(fraction * (float) data.size());

    std::shuffle(data.begin(), data.end(), rng);

    std::vector<std::string> subset;
    subset.reserve(nitems);
    std::vector<std::string> remaining;
    remaining.reserve(data.size() - nitems);
    for (unsigned i = 0; i < data.size(); i++) {
        if (i < nitems) {
            subset.push_back(data[i]);
        } else {
            remaining.push_back(data[i]);
        }
    }
    return std::make_pair(subset, remaining);
}

std::vector<int> random_locations(int nlocs, int max, std::mt19937& rng) {
    std::vector<int> locations(nlocs);

    std::uniform_int_distribution<int> loc_gen(0, max - 1);
    for (int i = 0; i < nlocs; i++) {
        locations[i] = loc_gen(rng);
    }

    std::sort(locations.begin(), locations.end());

    return locations;
}

std::vector<int> read_locations(const char* locations_file) {
    std::vector<int> locations;

    auto lines = read_lines(locations_file);

    int n = 0;
    for (auto& l : lines) {
        int i = std::stoi(l);
        locations.push_back(i);
        n++;
    }
    return locations;
}

bool to_bool(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);
    std::istringstream is(str);
    bool b;
    is >> std::boolalpha >> b;
    std::cout << "bool: " << b << std::endl;
    return b;
}

std::tuple<std::vector<int>, std::vector<double>, bool> read_entropies(const char* entropies_file) {
    std::vector<int> locations;
    std::vector<double> entropies;
    auto lines = read_lines(entropies_file);
    bool fixedLengthData = to_bool(lines[0]);
    for (auto& l : boost::make_iterator_range( lines.begin() + 1, lines.end() )) {
    //for (auto& l : lines) {
        std::string segment;
        std::stringstream line_stream(l);
        std::getline(line_stream, segment, ',');
        int i = std::stoi(segment);
        locations.push_back(i);
        std::getline(line_stream, segment, ',');
        double d = std::stod(segment);
        entropies.push_back(d);
    }
    return std::make_tuple(locations,entropies, fixedLengthData);
}

void write_locations(std::vector<int> locs, const char* locations_file) {
    std::ofstream outfile;
    outfile.open(locations_file, std::ios_base::out | std::ios_base::trunc);
    for (int l : locs) {
        outfile << l << std::endl;
    }

    outfile.close();
}

void write_entropies(std::vector<double> entropies, std::vector<int> locs, bool fixed, const char* entropies_file) {
    std::ofstream outfile;
    assert(locs.size() == entropies.size());
    outfile.open(entropies_file, std::ios_base::out | std::ios_base::trunc);
    outfile << std::boolalpha << fixed << std::endl;
    for (unsigned int i = 0; i < entropies.size(); i++) {
        outfile << locs[i] << ", " << entropies[i] << std::endl;
    }

    outfile.close();
}

void print_locations(std::vector<int> locs) {
    printf("locations: [");
    for (unsigned i = 0; i < locs.size(); i++) {
        printf("%d, ", locs[i]);
    }
    printf("]\n");
}
