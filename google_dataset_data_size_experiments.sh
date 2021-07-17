#!/bin/bash

# create results folder
mkdir -p results

# google data
dataset="google_urls.txt"
byteLocations1B="./data/locations/google/1B.txt_ent"
byteLocations4B="./data/locations/google/4B.txt_ent"
byteLocations8B="./data/locations/google/8B.txt_ent"
for data_size in 128 256 512 1024 2048 8192	16000 32000 64000 128000 256000 512000 1000000
do
	num_probes=$data_size
	fract_successful=0.5
	./set_options.sh "./cleanData/$dataset" $data_size $num_probes $fract_successful $byteLocations1B $byteLocations4B $byteLocations8B
	numactl --physcpubind=1 --membind=1 ./benchmarker > results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res 2>&1
done

