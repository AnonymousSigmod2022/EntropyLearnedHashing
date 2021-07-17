#include <iostream>
#include <vector>
#include <string>
#include <cstdarg>
#include <random>
#include <time>
#include <cmath>
#include <cstring>
#include <chrono>
#include <fstream> 

using namespace std;
#include "workload_generator.h"

void workload_generator::generateRandomStrings(unsigned int numStrings, unsigned int lengthStrings, vector<string> &inputVec) {
	unsigned seed = std::chrono::steady_clock::now().time_since_epoch().count();
  	//std::default_random_engine myRandomEngine(seed);
	std::independent_bits_engine bytes_randomizer = std::independent_bits_engine<std::default_random_engine, CHAR_BIT, uint8_t>(seed);
	 
	for (unsigned int i = 0; i < numStrings; i++) {
		inputVec.push_back(string());
		inputVec.back().resize(lengthStrings);
		std::vector<unsigned char> data(1000);
    	//std::generate(begin(inputVec.back()), end(inputVec.back()), bytes_randomizer);
    	for (unsigned int j = 0; j < lengthStrings; j++) {
    		unsigned char byte_to_write = 0;
    		while (byte_to_write==0) {
    			byte_to_write = bytes_randomizer();
    		}
    		inputVec.back()[j] = byte_to_write;
    	}
	}
}

void workload_generator::writeStringBinaryFile(std::string filename, vector<string> &inputVec, bool append) {
	fstream myfile;
	if (append) {
		myfile.open(filename, ios::out | ios::app | ios::binary);
	} else {
		myfile.open(filename, ios::out | ios::binary);
	}
	if (myfile.is_open())
	{
		for(string x : inputVec) {
			myfile << x.c_str() << '\0';
		}
		myfile.close();
	}
	else {cout << "Unable to open file";}
}

void workload_generator::readStringBinaryFile(std::string filename, vector<string> &inputVec) {
	fstream myfile;
	myfile.open(filename, ios::in | ios::binary);
	if (myfile.is_open())
	{
		while(!myfile.eof()) {
			std::string s;
			std::getline(myfile, s, '\0');
			if (s.length() != 0) {
    			inputVec.push_back(s);
    		}
    	}
	}
	else {cout << "Unable to open file";}
}
