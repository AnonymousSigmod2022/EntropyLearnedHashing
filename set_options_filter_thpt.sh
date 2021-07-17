#!/bin/bash

dataset=$1
data_size=$2
num_probes=$3
fract_successful=$4
byteLocations1B=$5
byteLocations4B=$6
byteLocations8B=$7

echo "dataset = $dataset" >> temp
echo "filter_benchmarks = register_bloom" >> temp
echo "hashfns = xxh3" >> temp
echo "hashfns = xxh3_sub8" >> temp
echo "data_size = $data_size" >> temp
echo "num_probes = $num_probes" >> temp
echo "num_trials = 50" >> temp
echo "fract_successful = $fract_successful" >> temp
echo "[random]" >> temp
echo "seed=42" >> temp
echo "[loc]" >> temp
echo "nlocs1B = 6" >> temp
echo "nlocs4B = 3" >> temp
echo "nlocs8B = 2" >> temp
echo "regenerate=False" >> temp
echo "byteLocations1B = $byteLocations1B" >> temp
echo "byteLocations4B = $byteLocations4B" >> temp
echo "byteLocations8B = $byteLocations8B" >> temp
echo "[table]" >> temp
echo "metrics = throughput" >> temp
echo "[filter]" >> temp
echo "bits_per_element = 7.4" >> temp
echo "bits_per_element_rb = 24.0" >> temp
echo "added_fpr = 0.01" >> temp

mv temp config/options.cfg
