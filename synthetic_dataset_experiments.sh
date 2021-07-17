#!/bin/bash

# create results folder
mkdir -p results

# synthetic data
byteLocations8B="./data/locations/syn/8B.txt_ent"
for data_size in 10000000 # 100 1000 10000 100000 1000000 10000000 100000000
do
	num_probes=$data_size
	for fract_successful in 1
	do
		dataset="synthetic_data_16_byte_length_20000000_keys.txt"
		./set_options_syn.sh "./cleanData/$dataset" $data_size $num_probes $fract_successful $byteLocations8B
		#numactl --physcpubind=1 --membind=1 ./benchmarker > results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res_r2 2>&1
	done
done

