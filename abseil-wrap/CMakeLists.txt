# Required as the first line
cmake_minimum_required(VERSION 3.5)

# Define the current project
project(swisstable VERSION 0.2)

# Tell Abseil which C++ standard to use
set(CMAKE_CXX_STANDARD 17)

# This is required for a fast build
set(CMAKE_BUILD_TYPE Release)

# Process abseil with CMake
add_subdirectory(abseil-cpp)

find_package(Threads REQUIRED)

# I'm an old fashioned GNU Make guy, I'm trying my best, but as far as I can
# tell there is literally no way to get a list of objects from an INTERFACE
# library target. I'm just hacking it like this with a hardcoded list of
# non-INTERFACE dependencies.
set(DEPENDENT_LIBS "$<TARGET_FILE:cwrappers>"
                   "$<TARGET_FILE:absl::base>"
                   "$<TARGET_FILE:absl::hash>"
                   "$<TARGET_FILE:absl::synchronization>"
                   "$<TARGET_FILE:absl::time>"
                   "$<TARGET_FILE:absl::time_zone>"
                   "$<TARGET_FILE:absl::int128>"
                   "$<TARGET_FILE:absl::city>"
                   "$<TARGET_FILE:absl::symbolize>"
                   "$<TARGET_FILE:absl::stacktrace>"
                   "$<TARGET_FILE:absl::demangle_internal>"
                   "$<TARGET_FILE:absl::debugging_internal>"
                   "$<TARGET_FILE:absl::hashtablez_sampler>"
                   "$<TARGET_FILE:absl::spinlock_wait>"
                   "$<TARGET_FILE:absl::malloc_internal>"
                   "$<TARGET_FILE:absl::graphcycles_internal>"
                   "$<TARGET_FILE:absl::raw_logging_internal>"
                   "$<TARGET_FILE:absl::exponential_biased>"
                   "$<TARGET_FILE:absl::raw_hash_set>")
