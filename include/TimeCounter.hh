/* ================= ABOUT THIS FILE ==================
 * Filename: TimeCounter.h
 * ----------------------------------------------------
 * Description
 * -----------
 * This class measures the cost of a data structure
 */
#pragma once

// System includes
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <tuple>
#include <list>
#include <iostream>
#include <vector>
#include <utility>
#include <random>
#include <deque>
#include <map>
#include <vector>
#include <chrono>

/**
 * -----------
 * TimeCounter
 * -----------
 *
 * @brief This class measures the cost of an operation.
 */
struct TimeCounter {
    double startCounter;
    double cost;
    std::chrono::high_resolution_clock::time_point start_nano;
    TimeCounter() {
        cost = 0;
    }
    TimeCounter(double init) {
        cost = init;
        startCounter = clock();
    }
    void start() {
        startCounter = clock();
    }
    void stop() {
        cost = (clock() - startCounter) / CLOCKS_PER_SEC;
    }
    void tic(){
        start_nano = std::chrono::high_resolution_clock::now();
    }
    void toc(){
        std::cout << std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now()-start_nano).count()/(double)1000000000 << "\n";
    }
    double getTime() {
        return cost;
    }
    TimeCounter operator + (const TimeCounter &o) const {
        return TimeCounter(cost + o.cost);
    }
    void operator += (const TimeCounter &o) {
        cost += o.cost;
    }
    bool operator < (const TimeCounter &o) const {
        return cost + 1e-9 < o.cost;
    }
};