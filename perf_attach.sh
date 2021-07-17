#!/bin/bash

numactl --physcpubind=1 --membind=1 ./benchmarker &

# it is 20 seconds hash table built and file reading time (for hash values) + 60 second warmup. tuned for 10M synthetic data1, 0.01 hit rate, 0.07 load factor
# it is 14 seconds hash table built and file reading time (for hash values) + 60 second warmup. tuned for 40K (probes really) synthetic data1, 0.01 hit rate, 0.07 load factor
sleep 33; 

echo "perf is attached to pid: $(pidof benchmarker)"

#perf record -F 99 -p $(pidof benchmarker) -o $1 --call-graph dwarf sleep 110
#perf stat -M MLP -p $(pidof benchmarker) -o $1 sleep 110
#perf stat -e l1d_pend_miss.pending,l1d_pend_miss.pending_cycles -p $(pidof benchmarker) -o $1 sleep 110
#perf stat -e inst_retired.any -p $(pidof benchmarker) -o $1 sleep 110
#perf stat -e cycles,instructions -p $(pidof benchmarker) -o $1 sleep 110
perf stat -e instructions,mem_load_uops_retired.l1_hit,mem_load_uops_retired.l1_miss,mem_load_uops_retired.l2_hit,mem_load_uops_retired.l2_miss,mem_load_uops_retired.llc_hit,mem_load_uops_retired.llc_miss -p $(pidof benchmarker) -o $1 -x "|" sleep 110
#perf stat -e instructions,mem_load_uops_retired.hit_lfb -p $(pidof benchmarker) -o $1 sleep 110

echo "waiting for benchmarker to finish (pid: $(pidof benchmarker))"
wait $(pidof benchmarker)
echo "benchmarker finished. moving on. pid (should be empty): $(pidof benchmarker)"
