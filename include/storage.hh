#pragma once

#include <fstream>
#include <iostream>
#include <random>
#include <vector>
#include <string>

void add_to_file(const char* fname, double val);
std::vector<std::string> read_lines(const char* filename);
std::vector<int> read_locations(const char* locations_file);
void write_locations(std::vector<int> locs, const char* locations_file);
std::tuple<std::vector<int>, std::vector<double>, bool> read_entropies(const char* entropies_file);
void write_entropies(std::vector<double> entropies, std::vector<int> locs, bool fixedLengthData, const char* entropies_file);
std::vector<int> random_locations(int nlocs, int max, std::mt19937& rng);
void print_locations(std::vector<int> locs);
std::pair<std::vector<std::string>, std::vector<std::string>>
get_subset(std::mt19937& rng, std::vector<std::string>& data, float fraction);
std::pair<std::vector<std::string>, std::vector<std::string>>
get_subset_static(std::vector<std::string>& data, float fraction);
