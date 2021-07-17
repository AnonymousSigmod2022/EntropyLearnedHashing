#!/bin/bash

# create results folder
mkdir -p results_mt

########## small data, f = 0
echo "small data, f = 0"

# wikipedia data
dataset="boncz_wikipedia.txt"
byteLocations1B="./data/locations/wikipedia/1B.txt_ent"
byteLocations4B="./data/locations/wikipedia/4B.txt_ent"
byteLocations8B="./data/locations/wikipedia/8B.txt_ent"
res_final=""
for data_size in 1000
do
	num_probes=$data_size
	for fract_successful in 0
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash - " | cut -d " " -f 3)
			res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

res_final=""
for data_size in 1000
do
	num_probes=$data_size
	for fract_successful in 0
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash_sub8 - " | cut -d " " -f 3)
			res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

# google data
dataset="google_urls.txt"
byteLocations1B="./data/locations/google/1B.txt_ent"
byteLocations4B="./data/locations/google/4B.txt_ent"
byteLocations8B="./data/locations/google/8B.txt_ent"
res_final=""
for data_size in 1100
do
	num_probes=$data_size
	for fract_successful in 0
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash - " | cut -d " " -f 3)
            res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

res_final=""
for data_size in 1100
do
    num_probes=$data_size
    for fract_successful in 0
    do
        for nthd in 1 2 3 4 5 6 7 8
        do
            res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash_sub8 - " | cut -d " " -f 3)
            res_final+=" ${res_temp}"
        done
    done
done

echo $res_final

########## small data, f = 1
echo "small data, f = 1"

# wikipedia data
dataset="boncz_wikipedia.txt"
byteLocations1B="./data/locations/wikipedia/1B.txt_ent"
byteLocations4B="./data/locations/wikipedia/4B.txt_ent"
byteLocations8B="./data/locations/wikipedia/8B.txt_ent"
res_final=""
for data_size in 1000
do
	num_probes=$data_size
	for fract_successful in 1
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash - " | cut -d " " -f 3)
			res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

res_final=""
for data_size in 1000
do
	num_probes=$data_size
	for fract_successful in 1
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash_sub8 - " | cut -d " " -f 3)
			res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

# google data
dataset="google_urls.txt"
byteLocations1B="./data/locations/google/1B.txt_ent"
byteLocations4B="./data/locations/google/4B.txt_ent"
byteLocations8B="./data/locations/google/8B.txt_ent"
res_final=""
for data_size in 1100
do
	num_probes=$data_size
	for fract_successful in 1
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash - " | cut -d " " -f 3)
            res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

res_final=""
for data_size in 1100
do
    num_probes=$data_size
    for fract_successful in 1
    do
        for nthd in 1 2 3 4 5 6 7 8
        do
            res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash_sub8 - " | cut -d " " -f 3)
            res_final+=" ${res_temp}"
        done
    done
done

echo $res_final

########## large data, f = 0
echo "large data, f = 0"

# wikipedia data
dataset="boncz_wikipedia.txt"
byteLocations1B="./data/locations/wikipedia/1B.txt_ent"
byteLocations4B="./data/locations/wikipedia/4B.txt_ent"
byteLocations8B="./data/locations/wikipedia/8B.txt_ent"
res_final=""
for data_size in 11000
do
	num_probes=$data_size
	for fract_successful in 0
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash - " | cut -d " " -f 3)
			res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

res_final=""
for data_size in 11000
do
	num_probes=$data_size
	for fract_successful in 0
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash_sub8 - " | cut -d " " -f 3)
			res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

# google data
dataset="google_urls.txt"
byteLocations1B="./data/locations/google/1B.txt_ent"
byteLocations4B="./data/locations/google/4B.txt_ent"
byteLocations8B="./data/locations/google/8B.txt_ent"
res_final=""
for data_size in 600000
do
	num_probes=$data_size
	for fract_successful in 0
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash - " | cut -d " " -f 3)
            res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

res_final=""
for data_size in 600000
do
    num_probes=$data_size
    for fract_successful in 0
    do
        for nthd in 1 2 3 4 5 6 7 8
        do
            res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash_sub8 - " | cut -d " " -f 3)
            res_final+=" ${res_temp}"
        done
    done
done

echo $res_final

########## large data, f = 1
echo "large data, f = 1"

# wikipedia data
dataset="boncz_wikipedia.txt"
byteLocations1B="./data/locations/wikipedia/1B.txt_ent"
byteLocations4B="./data/locations/wikipedia/4B.txt_ent"
byteLocations8B="./data/locations/wikipedia/8B.txt_ent"
res_final=""
for data_size in 11000
do
	num_probes=$data_size
	for fract_successful in 1
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash - " | cut -d " " -f 3)
			res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

res_final=""
for data_size in 11000
do
	num_probes=$data_size
	for fract_successful in 1
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash_sub8 - " | cut -d " " -f 3)
			res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

# google data
dataset="google_urls.txt"
byteLocations1B="./data/locations/google/1B.txt_ent"
byteLocations4B="./data/locations/google/4B.txt_ent"
byteLocations8B="./data/locations/google/8B.txt_ent"
res_final=""
for data_size in 600000
do
	num_probes=$data_size
	for fract_successful in 1
	do
		for nthd in 1 2 3 4 5 6 7 8
		do
			res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash - " | cut -d " " -f 3)
            res_final+=" ${res_temp}"
		done
	done
done

echo $res_final

res_final=""
for data_size in 600000
do
    num_probes=$data_size
    for fract_successful in 1
    do
        for nthd in 1 2 3 4 5 6 7 8
        do
            res_temp=$(cat results_mt/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful_$nthd\_nthd.res | grep "swiss_wyhash_sub8 - " | cut -d " " -f 3)
            res_final+=" ${res_temp}"
        done
    done
done

echo $res_final


