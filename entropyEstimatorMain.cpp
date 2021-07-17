#include <iostream>
#include <vector>
#include <string>
#include <cstdarg>
#include <random>
#include <ctime>
#include <cmath>
#include <cstring>
#include <chrono>
#include <fstream> 
#include<set>


#include "workload_generator.hh"
//#include "entropyEstimation/entropy_estimators.hh"
#include "csv.h"

using namespace std;

// hard coded for now. 
void read_csv(string filename, vector<string>& inputVec) {
	cout << "filename: " << filename << endl;
	//io::CSVReader<3, io::trim_chars<' ', '\t'>, io::no_quote_escape<','>> in(filename);
	io::CSVReader<1, io::trim_chars<' ', '\t'>, io::double_quote_escape<',', '\"'>> in(filename); 
	in.read_header(io::ignore_extra_column, "url");
	string url;
	int count = 0; 
	while(in.read_row(url)){
		if (count < 10) {
			count += 1;
			cout << url << endl;
		}
		inputVec.push_back(url);
	}
}

static std::vector<std::string> read_lines(const char* filename) {
    std::ifstream infile(filename);

    std::vector<std::string> lines;

    for (std::string line; getline(infile, line);) {
        lines.push_back(line);
    }

    return lines;
}

static std::vector<int> read_locations(int nlocs, const char* locations_file) {
    std::vector<int> locations;

    auto lines = read_lines(locations_file);

    int n = 0;
    for (auto& l : lines) {
        if (n >= nlocs) {
            break;
        }
        int i = std::stoi(l);
        locations.push_back(i);
        n++;
    }

    std::sort(locations.begin(), locations.end());

    return locations;
}

void write_dataset(string filename, vector<string>& inputVec) {
	ofstream out_file;
	out_file.open(filename);
	for (auto x : inputVec) {
		out_file << x << endl;
	}
	out_file.close();
}

void makeUniqueSet(vector<string>& inputVec, vector<string>& outputVec) {
	std::set<std::string> setOfElements;
	for (string x : inputVec) {
		if (setOfElements.find(x) == setOfElements.end()) {
			outputVec.push_back(x);
			setOfElements.insert(x);
		}
	}
}

int maxStringLength(std::vector<string>& inputVec) {
	int maxStringLength = 0;
	for (auto x : inputVec) {
		maxStringLength = max(maxStringLength, (int)x.length());
	}
	return maxStringLength;
}

/*int main(int argc, char *argv[]) {
	bool random = true;
	string file;
	if (argc >= 2) {
		file = argv[1];
	}
	//vector<string> inputVec;
	vector<int> positions;
	double entropy = 0.0;
	//read_csv(file, inputVec);
	//write_dataset("google_urls.txt", inputVec);
	//cout << "original size: " << inputVec.size() << endl;
	//if(makeUnique) {
	//	makeUniqueSet(inputVec, uniqueVec);
	//}
	auto inputVec = read_lines("data/hn_urls.txt");
	std::vector<string> uniqueVec;
	bool makeUnique = true;
	if(makeUnique) {
		makeUniqueSet(inputVec, uniqueVec);
		write_dataset("data/hn_urls.txt", uniqueVec);
	}
	// new stuff
	//std::vector<int> locations = read_locations(12, "data/locations_google.txt");


	//cout << "unique size: " << uniqueVec.size() << endl;
	//cout << "maxStringLength " << maxStringLength(uniqueVec) << endl;
	// for (int i = 5; i < 12; i++) {
	// 	entropy_estimators::printEntropyStats(uniqueVec, locations, i);
	// }
	// entropy = entropy_estimators::greedyRenyiSelectorStopK(100, uniqueVec, positions, 12);
	// cout << "positions: " << endl;
	// for (int x : positions) {
	// 	cout << x << ", ";
	// }
	// cout << endl;
	// cout << "entropy: " << entropy << endl;
} */
