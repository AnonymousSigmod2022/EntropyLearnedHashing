#!/bin/bash

analysis_type="uarch-exploration" # uarch-exploration hotspots threading memory-access

numactl --physcpubind=1 --membind=1 ./benchmarker &

# it is 20 seconds hash table built and file reading time (for hash values) + 60 second warmup. tuned for 10M synthetic data1, 0.01 hit rate, 0.07 load factor
# it is 14 seconds hash table built and file reading time (for hash values) + 60 second warmup. tuned for 40K synthetic data1 (10M keys, 40K probes), 0.01 hit rate, 0.07 load factor
sleep 32;

echo "vtune is attached to pid: $(pidof benchmarker)"

vtune -collect $analysis_type -target-duration-type short -result-dir $1 -data-limit=0 -target-pid $(pidof benchmarker) -duration 110

echo "waiting for benchmarker to finish (pid: $(pidof benchmarker))"
wait $(pidof benchmarker)
echo "benchmarker finished. moving on. pid (should be empty): $(pidof benchmarker)"

