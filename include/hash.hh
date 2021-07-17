#pragma once

#include <string>
#define XXH_INLINE_ALL
#include "hash/orig_xhash.hh"
#include "hash/xxhash_customs.hh"
#include "hash/mmh_var.hh"
//#include "murmurhash2.h"
#include "hash/wyhash.hh"
#include "hash/wyhash_customs.hh"
#include "hash/crc_wyhash_customs.hh"
#include "hash/crc32_hash.hh"

/* list of hash functions
 * 
 * 1) standard -> hash_standard
 * 2) subset murmurhash 2 -> hash_sub4_mmh2
 * 3) subset sdbm -> hash_sub1_sdbm
 */

static inline size_t hash_standard(const std::string& str) {
    return std::hash<std::string>()(str);
}

struct hash_standard_s {
    inline std::size_t operator()(const std::string& str) const {
        return hash_standard(str);
    }
};

static inline size_t hash_xxh64(const std::string& str) {
    return XXH3_64bits((const unsigned char *)str.data(), str.length());
}

struct hash_xxh64_s {
    inline std::size_t operator()(const std::string& str) const {
        return hash_xxh64(str);
    }
};

static inline size_t wyhash_default(const std::string& str) {
    return wyhash_default(str.data(), str.length());
}

struct hash_wyhash_s {
    hash_wyhash_s() {}

    inline std::size_t operator()(const std::string& str) const {
        return wyhash_default(str.data(), str.length());
    }
};

struct hash_sub4_mmh2_s {
    hash_sub4_mmh2_s(int* locs, int kbytes) {
        k = kbytes;
        locations = locs;
    }

    inline std::size_t operator()(const std::string& str) const {
        return hash_sub4_mmh2_cust(str, locations, k);
    }

   private:
    int k;
    int* locations;
};

struct hash_mmh2_s {
    std::size_t operator()(const std::string& str) const {
        const unsigned int m = 0x5bd1e995;
        const int r = 24;
        int seed = 0;
        int len = str.length();

        // Initialize the hash to a 'random' value

        unsigned int h = seed ^ len;

        // Mix 4 bytes at a time into the hash

        const unsigned char * data = (const unsigned char *)str.data();
        

        while(len >= 4)
        {
            unsigned int k = *(unsigned int *)data;

            k *= m;
            k ^= k >> r;
            k *= m;

            h *= m;
            h ^= k;

            data += 4;
            len -= 4;
        }

        // Handle the last few bytes of the input array

        switch(len)
        {
        case 3: h ^= data[2] << 16;
        case 2: h ^= data[1] << 8;
        case 1: h ^= data[0];
                h *= m;
        };

        // Do a few final mixes of the hash to ensure the last few
        // bytes are well-incorporated.

        h ^= h >> 13;
        h *= m;
        h ^= h >> 15;

        return h;
    }
};

struct hash_sdbm_s {
    std::size_t operator()(const std::string& str) const {
        unsigned long hash = 0;
        for (unsigned i = 0; i < str.length(); i++) {
            hash = str[i] + (hash << 6) + (hash << 16) - hash;
        }

        return hash;
    }
};

static inline size_t hash_sub1_sdbm(const std::string& str,
                                      const int* locations,
                                      int k) {
    unsigned long hash = str.length();

    int len = str.length();
    for (int i = 0; i < k; i += 1) {
        if (locations[i] < len) {
            hash = str[locations[i]] + (hash << 6) + (hash << 16) - hash;
            hash += str[locations[i]];
        }
    }
    return hash;
}
struct hash_sub1_sdbm_s {
    hash_sub1_sdbm_s(int* locs, int kbytes) {
        k = kbytes;
        locations = locs;
    }

    inline std::size_t operator()(const std::string& str) const {
        return hash_sub1_sdbm(str, locations, k);
    }

   private:
    int k;
    int* locations;
};

