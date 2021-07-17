#ifndef ENT
#define ENT
#pragma once

#include <map>
#include <string>
#include <vector>

#include <ctime>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdarg>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

#include "TimeCounter.hh"

namespace entropy_estimators {
    template <typename T>
    double greedyRenyiSelectorStopK(unsigned int maxLengthString,
                                    std::vector<std::string>& inputVec,
                                    std::vector<int>& returnPositions,
                                    std::vector<double>& entropies,
                                    bool& fixedLength,
                                    int numPositions);

    template <typename T>
    double greedyRenyiSelectorStopK(unsigned int maxLengthString,
                                    std::vector<std::string>& inputVec,
                                    std::vector<std::string>& validationVec,
                                    std::vector<int>& returnPositions,
                                    std::vector<double>& entropies,
                                    bool& fixedLength,
                                    int numPositions);

    template <typename T>
    double greedyRenyiSelectorStopEntropy(unsigned int maxLengthString,
                                          std::vector<std::string>& inputVec,
                                          std::vector<int>& returnPositions,
                                          bool& fixedLength,
                                          double desiredEntropy);
} 

using namespace std;

uint64_t fallingPower(uint64_t x, uint64_t power) {
    assert(power >= 0);
    if (power == 0) {
        return 1;
    }
    uint64_t returnVal = x;
    for (unsigned int i = 1; i < power; i++) {
        returnVal = returnVal * (x-i);
    }
    return returnVal;
}

double evaluateRenyiEntropy(
    unordered_map<string, int> stringComboAndFrequencyMap,
    int numItems) {
    double renyiEntropySummand = 0.0;
    for (const auto& key_val : stringComboAndFrequencyMap) {
        double falling_power = static_cast<double>(fallingPower(key_val.second, 2)) / fallingPower(numItems, 2);
        //double prop = static_cast<double>(key_val.second) / numItems;
        //double propSquared = prop * prop;
        //assert(propSquared != 0.0);
        double renyiEntropySummandAfter = renyiEntropySummand + falling_power;
        //assert(renyiEntropySummand != renyiEntropySummandAfter);
        renyiEntropySummand = renyiEntropySummandAfter;
    }
    double renyiEntropy = -1 * log2(renyiEntropySummand);
    return renyiEntropy;
}

template <typename entropy_chunk_t>
std::string formPartialString(const std::string& inputString, const vector<int>& positions) {
    std::string hash_string(
                    (positions.size()) * sizeof(entropy_chunk_t), 0);
    entropy_chunk_t* hash_data = (entropy_chunk_t*) hash_string.data();
    unsigned x_size = inputString.size() / sizeof(entropy_chunk_t);
    entropy_chunk_t* x_data = (entropy_chunk_t*) inputString.data();

    for (unsigned int k = 0; k < positions.size(); k++) {
        // if exists, use position in string. else keep 0
        if (positions[k] <= (int) x_size) {
            hash_data[k] = x_data[positions[k]];
        }
    }
    hash_string = hash_string + to_string(inputString.size());
    return hash_string;
} 

template <typename entropy_chunk_t>
pair<int, double> chooseBestPositionRenyi(unsigned int maxLengthString,
                                          vector<string>& inputVec,
                                          vector<int>& currentPositions) {
    assert(currentPositions.size() != maxLengthString);
    double bestEntropy = 0.0;
    int bestPosition = -1;
    for (unsigned int i = 0; i < maxLengthString / sizeof(entropy_chunk_t); i++) {
        bool posUsed = false;
        // check if positions already used
        for (unsigned int j = 0; j < currentPositions.size(); j++) {
            if (currentPositions[j] == (int) i) {
                posUsed = true;
                break;
            }
        }
        if (posUsed) {
            continue;
        }
        vector<int> testPositions(currentPositions);
        testPositions.push_back(i);
        unordered_map<string, int> stringComboAndFrequencyMap;
        // insert each string into map.
        for (string x : inputVec) {
            std::string hash_string = formPartialString<entropy_chunk_t>(x, testPositions);
            // this should add one to value of hash_string. If the value doesn't
            // exist, it will be created.
            stringComboAndFrequencyMap[hash_string]++;
        }
        // evaluate Renyi-Entropy
        double positionRenyiEntropy =
            evaluateRenyiEntropy(stringComboAndFrequencyMap, inputVec.size());
        if (positionRenyiEntropy > bestEntropy) {
            bestEntropy = positionRenyiEntropy;
            bestPosition = i;
        }
    }
    cout << "position chosen: " << bestPosition << endl;

    return std::make_pair(bestPosition, bestEntropy);
}

bool evaluateDatasetFixedLength(vector<string>& inputVec) {
    if (inputVec.size() == 0) {
        return false;
    }
    size_t firstLength = inputVec[0].size();
    for (string x : inputVec) {
        if (x.size() != firstLength) {
            return false;
        }
    }
    return true;
}

template <typename entropy_chunk_t>
double evaluateEntropy(vector<string>& testVec, vector<int>& positions) {
    unordered_map<string, int> stringComboAndFrequencyMap;
    for (string x : testVec) {
        stringComboAndFrequencyMap[formPartialString<entropy_chunk_t>(x, positions)]++;
    }
    return evaluateRenyiEntropy(stringComboAndFrequencyMap, testVec.size());
}

template <typename T>
double entropy_estimators::greedyRenyiSelectorStopK(
    unsigned int maxLengthString,
    vector<string>& inputVec,
    vector<int>& returnPositions,
    vector<double>& entropies,
    bool& fixedLength,
    int numPositions) {
    return entropy_estimators::greedyRenyiSelectorStopK<T>(maxLengthString,inputVec, inputVec,
        returnPositions, entropies, fixedLength, numPositions);
}

// selects best bytes to use in hashing until k bytes used.
template <typename T>
double entropy_estimators::greedyRenyiSelectorStopK(
    unsigned int maxLengthString,
    vector<string>& inputVec,
    vector<string>& validationVec,
    vector<int>& returnPositions,
    vector<double>& entropies,
    bool& fixedLength,
    int numPositions) {

    fixedLength = evaluateDatasetFixedLength(inputVec);
    assert(numPositions + returnPositions.size() < maxLengthString);
    double entropy = 0.0;
    double totalTime = 0.0;
    for (int i = 0; i < numPositions; i++) {
        TimeCounter time;
        cout << "i: " << i << "," << endl;
        time.start();
        if (returnPositions.size() == maxLengthString) {
            break;
        }
        pair<int, double> nextPosAndEntropy =
            chooseBestPositionRenyi<T>(maxLengthString, inputVec, returnPositions);
        returnPositions.push_back(nextPosAndEntropy.first);
        cout << "training entropy: " << nextPosAndEntropy.second << endl;
        if (validationVec == inputVec) {
            entropy = nextPosAndEntropy.second;
        } else {
            entropy = evaluateEntropy<T>(validationVec, returnPositions);
            cout << "evaluation Entropy: " << entropy << std::endl;
        }
        
        entropies.push_back(entropy);
        time.stop();
        cout << "time taken: " << time.getTime() << " in seconds" << endl;
        totalTime += time.getTime();
    }
    return entropy;
}

// selects best bytes to use in hashing until a minimum entropy is hit.
template <typename T>
double entropy_estimators::greedyRenyiSelectorStopEntropy(
    unsigned int maxLengthString,
    vector<string>& inputVec,
    vector<int>& returnPositions,
    bool& fixedLength, 
    double desiredEntropy) {
    double entropy = 0.0;
    fixedLength = evaluateDatasetFixedLength(inputVec);
    while (entropy < desiredEntropy) {
        cout << "entropy:" << entropy << "," << endl;
        if (returnPositions.size() == maxLengthString) {
            break;
        }
        pair<int, double> nextPosAndEntropy =
            chooseBestPositionRenyi<T>(maxLengthString, inputVec, returnPositions);
        returnPositions.push_back(nextPosAndEntropy.first);
        entropy = nextPosAndEntropy.second;
    }
    return entropy;
}

#endif