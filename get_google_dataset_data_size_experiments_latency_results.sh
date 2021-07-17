#!/bin/bash

# google data
dataset="google_urls.txt"
byteLocations1B="./data/locations/google/1B.txt_ent"
byteLocations4B="./data/locations/google/4B.txt_ent"
byteLocations8B="./data/locations/google/8B.txt_ent"
for data_size in 128 256 512 1024 2048 8192 16000 32000 64000 128000 256000 512000 1000000
do
	num_probes=$data_size
	fract_successful=0.5
	res_temp1=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "swiss_default -")
	res_temp2=$(echo $res_temp1 | cut -d "-" -f 3 | sed 's/ //g')
	res_swiss_default_final+=" ${res_temp2}"

	res_temp1=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "swiss_wyhash -")
	res_temp2=$(echo $res_temp1 | cut -d "-" -f 3 | sed 's/ //g')
	res_swiss_wyhash_final+=" ${res_temp2}"

	res_temp1=$(cat results/$dataset\_$data_size\_keys_$num_probes\_probes_$fract_successful\_fract_successful.res | grep "swiss_wyhash_sub8 -")
	res_temp2=$(echo $res_temp1 | cut -d "-" -f 3 | sed 's/ //g')
	res_swiss_wyhash_sub8_final+=" ${res_temp2}"
done

echo $res_swiss_default_final
echo $res_swiss_wyhash_final
echo $res_swiss_wyhash_sub8_final

