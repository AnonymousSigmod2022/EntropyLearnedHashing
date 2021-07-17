#!/bin/bash

# create results folder
mkdir -p results_filter

# uuid data
dataset="boncz_uuid.txt"
byteLocations1B="./data/locations/uuid/1B.txt_ent"
byteLocations4B="./data/locations/uuid/4B.txt_ent"
byteLocations8B="./data/locations/uuid/8B.txt_ent"
for data_size in 1000 50000
do
	num_probes=$data_size
	for fract_successful in 0.5
	do
		./set_options_filter_latency.sh "./cleanData/$dataset" $data_size $num_probes $fract_successful $byteLocations1B $byteLocations4B $byteLocations8B
		numactl --physcpubind=1 --membind=1 ./benchmarker > results_filter/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res 2>&1
	done
done

# wikipedia data
dataset="boncz_wikipedia.txt"
byteLocations1B="./data/locations/wikipedia/1B.txt_ent"
byteLocations4B="./data/locations/wikipedia/4B.txt_ent"
byteLocations8B="./data/locations/wikipedia/8B.txt_ent"
for data_size in 1000 11000
do
	num_probes=$data_size
	for fract_successful in 0.5
	do
		./set_options_filter_latency.sh "./cleanData/$dataset" $data_size $num_probes $fract_successful $byteLocations1B $byteLocations4B $byteLocations8B
		numactl --physcpubind=1 --membind=1 ./benchmarker > results_filter/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res 2>&1
	done
done

# wiki data
dataset="boncz_wiki.txt"
byteLocations1B="./data/locations/wiki/1B.txt_ent"
byteLocations4B="./data/locations/wiki/4B.txt_ent"
byteLocations8B="./data/locations/wiki/8B.txt_ent"
for data_size in 1000 50000
do
	num_probes=$data_size
	for fract_successful in 0.5
	do
		./set_options_filter_latency.sh "./cleanData/$dataset" $data_size $num_probes $fract_successful $byteLocations1B $byteLocations4B $byteLocations8B
		numactl --physcpubind=1 --membind=1 ./benchmarker > results_filter/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res 2>&1
	done
done

# hn data
dataset="hn_urls.txt"
byteLocations1B="./data/locations/hn/1B.txt_ent"
byteLocations4B="./data/locations/hn/4B.txt_ent"
byteLocations8B="./data/locations/hn/8B.txt_ent"
for data_size in 1000 122000
do
	num_probes=$data_size
	for fract_successful in 0.5
	do
		./set_options_filter_latency.sh "./cleanData/$dataset" $data_size $num_probes $fract_successful $byteLocations1B $byteLocations4B $byteLocations8B
		numactl --physcpubind=1 --membind=1 ./benchmarker > results_filter/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res 2>&1
	done
done

# google data
dataset="google_urls.txt"
byteLocations1B="./data/locations/google/1B.txt_ent"
byteLocations4B="./data/locations/google/4B.txt_ent"
byteLocations8B="./data/locations/google/8B.txt_ent"
for data_size in 1000 600000
do
	num_probes=$data_size
	for fract_successful in 0.5
	do
		./set_options_filter_latency.sh "./cleanData/$dataset" $data_size $num_probes $fract_successful $byteLocations1B $byteLocations4B $byteLocations8B
		numactl --physcpubind=1 --membind=1 ./benchmarker > results_filter/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res 2>&1
	done
done

