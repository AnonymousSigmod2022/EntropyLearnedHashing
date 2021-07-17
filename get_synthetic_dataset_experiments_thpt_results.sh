#!/bin/bash

# synthetic data, frac = 0
echo "### frac = 0 ###"

res_final=""
for data_size in 100 1000 10000 100000 1000000 10000000 # 100000000
do
	num_probes=$data_size
	for fract_successful in 0
	do
		dataset="synthetic_data_${data_size}0_keys.txt"
		res_temp=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "swiss_default -" | cut -d "-" -f 2 | sed 's/ //g')
		res_final+=" ${res_temp}"
	done
done

echo $res_final

res_final=""
for data_size in 100 1000 10000 100000 1000000 10000000 # 100000000
do
	num_probes=$data_size
	for fract_successful in 0
	do
		dataset="synthetic_data_${data_size}0_keys.txt"
		res_temp=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "swiss_wyhash -" | cut -d "-" -f 2 | sed 's/ //g')
		res_final+=" ${res_temp}"
	done
done

echo $res_final

res_final=""
for data_size in 100 1000 10000 100000 1000000 10000000 # 100000000
do
	num_probes=$data_size
	for fract_successful in 0
	do
		dataset="synthetic_data_${data_size}0_keys.txt"
		res_temp=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "swiss_wyhash_sub8 -" | cut -d "-" -f 2 | sed 's/ //g')
		res_final+=" ${res_temp}"
	done
done

echo $res_final

# synthetic data, frac = 1
echo "### frac = 1 ###"

res_final=""
for data_size in 100 1000 10000 100000 1000000 10000000 # 100000000
do
	num_probes=$data_size
	for fract_successful in 1
	do
		dataset="synthetic_data_${data_size}0_keys.txt"
		res_temp=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "swiss_default -" | cut -d "-" -f 2 | sed 's/ //g')
		res_final+=" ${res_temp}"
	done
done

echo $res_final

res_final=""
for data_size in 100 1000 10000 100000 1000000 10000000 # 100000000
do
	num_probes=$data_size
	for fract_successful in 1
	do
		dataset="synthetic_data_${data_size}0_keys.txt"
		res_temp=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "swiss_wyhash -" | cut -d "-" -f 2 | sed 's/ //g')
		res_final+=" ${res_temp}"
	done
done

echo $res_final

res_final=""
for data_size in 100 1000 10000 100000 1000000 10000000 # 100000000
do
	num_probes=$data_size
	for fract_successful in 1
	do
		dataset="synthetic_data_${data_size}0_keys.txt"
		res_temp=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "swiss_wyhash_sub8 -" | cut -d "-" -f 2 | sed 's/ //g')
		res_final+=" ${res_temp}"
	done
done

echo $res_final

