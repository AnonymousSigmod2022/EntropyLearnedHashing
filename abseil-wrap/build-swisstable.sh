cmake .
make

if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    ar -M < libswisstable.mri
elif [[ "$OSTYPE" == "darwin"* ]]; then
    libtool -static -o libswisstable.a ./abseil-cpp/absl/base/libabsl_base.a ./abseil-cpp/absl/hash/libabsl_hash.a ./abseil-cpp/absl/synchronization/libabsl_synchronization.a ./abseil-cpp/absl/time/libabsl_time.a ./abseil-cpp/absl/time/libabsl_time_zone.a ./abseil-cpp/absl/numeric/libabsl_int128.a ./abseil-cpp/absl/hash/libabsl_city.a ./abseil-cpp/absl/debugging/libabsl_symbolize.a ./abseil-cpp/absl/debugging/libabsl_stacktrace.a ./abseil-cpp/absl/debugging/libabsl_demangle_internal.a ./abseil-cpp/absl/debugging/libabsl_debugging_internal.a ./abseil-cpp/absl/container/libabsl_hashtablez_sampler.a ./abseil-cpp/absl/base/libabsl_spinlock_wait.a ./abseil-cpp/absl/base/libabsl_malloc_internal.a ./abseil-cpp/absl/synchronization/libabsl_graphcycles_internal.a ./abseil-cpp/absl/base/libabsl_raw_logging_internal.a ./abseil-cpp/absl/base/libabsl_exponential_biased.a ./abseil-cpp/absl/container/libabsl_raw_hash_set.a
fi
