#!/bin/bash

# uuid data
dataset="boncz_uuid.txt"
byteLocations1B="./data/locations/uuid/1B.txt_ent"
byteLocations4B="./data/locations/uuid/4B.txt_ent"
byteLocations8B="./data/locations/uuid/8B.txt_ent"
res_std_standard_final=""
for data_size in 1000 50000
do
	num_probes=$data_size
	for fract_successful in 0 1
	do
		res_temp1=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "std_standard -")
        res_temp2=$(echo $res_temp1 | cut -d "-" -f 3 | sed 's/ //g')
        res_std_standard_final+=" ${res_temp2}"
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
	for fract_successful in 0 1
	do
        res_temp1=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "std_standard -")
        res_temp2=$(echo $res_temp1 | cut -d "-" -f 3 | sed 's/ //g')
        res_std_standard_final+=" ${res_temp2}"
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
	for fract_successful in 0 1
	do
        res_temp1=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "std_standard -")
        res_temp2=$(echo $res_temp1 | cut -d "-" -f 3 | sed 's/ //g')
        res_std_standard_final+=" ${res_temp2}"
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
	for fract_successful in 0 1
	do
        res_temp1=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "std_standard -")
        res_temp2=$(echo $res_temp1 | cut -d "-" -f 3 | sed 's/ //g')
        res_std_standard_final+=" ${res_temp2}"
	done
done

# google data
dataset="google_urls.txt"
byteLocations1B="./data/locations/google/1B.txt_ent"
byteLocations4B="./data/locations/google/4B.txt_ent"
byteLocations8B="./data/locations/google/8B.txt_ent"
for data_size in 1100 600000
do
	num_probes=$data_size
	for fract_successful in 0 1
	do
        res_temp1=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "std_standard -")
        res_temp2=$(echo $res_temp1 | cut -d "-" -f 3 | sed 's/ //g')
        res_std_standard_final+=" ${res_temp2}"
	done
done

echo $res_std_standard_final

