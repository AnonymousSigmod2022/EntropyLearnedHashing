#pragma once

#include "hash/orig_xhash.hh"

XXH_FORCE_INLINE XXH64_hash_t
XXH3_1loc_fixed(const xxh_u8* input, const size_t len, const unsigned int* locations, const xxh_u8* secret = XXH3_kSecret)
{
    xxh_u64 const bitflip = (XXH_readLE64(secret+8) ^ XXH_readLE64(secret+16));
    xxh_u64 const input64 = XXH_readLE64(input + locations[0]);
    xxh_u64 const keyed = input64 ^ bitflip;
    return XXH3_avalanche(keyed + len);
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_1loc(const xxh_u8* input, const size_t len, const unsigned int* locations)//, const int* locations)
{
    if (XXH_likely(len > locations[1])) {
        return XXH3_1loc_fixed(input, len, locations);
    }
    return XXH3_64bits(input, len);   
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_2loc_fixed(const xxh_u8* input, const size_t len, const unsigned int* locations, const xxh_u8* secret = XXH3_kSecret)
{
    {
        xxh_u64 const bitflip1 = (XXH_readLE64(secret+24) ^ XXH_readLE64(secret+32));
        xxh_u64 const bitflip2 = (XXH_readLE64(secret+40) ^ XXH_readLE64(secret+48));
        xxh_u64 const input_lo = XXH_readLE64(input+locations[0]) ^ bitflip1;
        xxh_u64 const input_hi = XXH_readLE64(input+locations[1]) ^ bitflip2;
        xxh_u64 const acc = len
                          + XXH_swap64(input_lo) + input_hi
                          + XXH3_mul128_fold64(input_lo, input_hi);
        //return acc;
        return XXH3_avalanche(acc);
    }
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_2loc(const xxh_u8* input, const size_t len, const unsigned int* locations)//, const int* locations)
{
	if (XXH_likely(len >  locations[2])) {
		return XXH3_2loc_fixed(input, len, locations, XXH3_kSecret);
	}
    return XXH3_64bits(input, len);   
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_3loc_fixed(const xxh_u8* input, const size_t len, const unsigned int* locations, const xxh_u8* secret = XXH3_kSecret)
{
    {
        xxh_u64 acc = len * XXH_PRIME64_1;
        acc += XXH3_mix16B(input + locations[0], secret, 0);
        acc += XXH3_mix16B(input + locations[1], secret+16, 0);
        return acc;
    }
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_3loc(const xxh_u8* input, const size_t len, const unsigned int* locations)//, const int* locations)
{
    if (XXH_likely(len >  locations[3])) {
        return XXH3_3loc_fixed(input, len, locations, XXH3_kSecret);
    }
    return XXH3_64bits(input, len);   
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_4loc_fixed(const xxh_u8* input, const size_t len, const unsigned int* locations, const xxh_u8* secret = XXH3_kSecret)
{
    {
        xxh_u64 acc = len * XXH_PRIME64_1;
        acc += XXH3_mix16B(input + locations[0], secret, 0);
        acc += XXH3_mix16B(input + locations[2], secret+16, 0);
        return acc;
    }
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_4loc(const xxh_u8* input, const size_t len, const unsigned int* locations)//, const int* locations)
{
    if (XXH_likely(len >  locations[4])) {
        return XXH3_4loc_fixed(input, len, locations, XXH3_kSecret);
    }
    return XXH3_64bits(input, len);   
}

/* structs */
struct hash_XXH3_1loc_fixed_s {
    hash_XXH3_1loc_fixed_s(int* locs) {
        locations[0] = (unsigned int)8 * locs[0];
    }
    inline std::size_t operator()(const std::string& str) const {
        return XXH3_1loc_fixed((const unsigned char *) str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[1];
};

struct hash_XXH3_1loc_s {
    hash_XXH3_1loc_s(int* locs) {
        locations[0] = (unsigned int)8 * locs[0];
        // cache endpoint (for if statement)
        locations[1] = (unsigned int)(locs[0] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        return XXH3_1loc((const unsigned char *) str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[2];
};

struct hash_XXH3_2loc_fixed_s {
    hash_XXH3_2loc_fixed_s(int* locs) {
        for (int i = 0; i < 2; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
    }
    inline std::size_t operator()(const std::string& str) const {
        return XXH3_2loc_fixed((const unsigned char *) str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[2];
};

struct hash_XXH3_2loc_s {
    hash_XXH3_2loc_s(int* locs) {
        for (int i = 0; i < 2; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
        // cache endpoint (for if statement)
        locations[2] = (unsigned int)(locs[1] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        return XXH3_2loc((const unsigned char *) str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[3];
};

struct hash_XXH3_3loc_fixed_s {
    hash_XXH3_3loc_fixed_s(int* locs) {
        for (int i = 0; i < 3; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
    }
    inline std::size_t operator()(const std::string& str) const {
        return XXH3_3loc_fixed((const unsigned char *) str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[3];
};

struct hash_XXH3_3loc_s {
    hash_XXH3_3loc_s(int* locs) {
        for (int i = 0; i < 3; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
        // cache endpoint (for if statement)
        locations[3] = (unsigned int)(locs[2] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        return XXH3_3loc((const unsigned char *) str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[4];
};

struct hash_XXH3_4loc_fixed_s {
    hash_XXH3_4loc_fixed_s(int* locs) {
        for (int i = 0; i < 4; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
    }
    inline std::size_t operator()(const std::string& str) const {
        return XXH3_4loc_fixed((const unsigned char *) str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[4];
};

struct hash_XXH3_4loc_s {
    hash_XXH3_4loc_s(int* locs) {
        for (int i = 0; i < 4; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
        // cache endpoint (for if statement)
        locations[4] = (unsigned int)(locs[3] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        return XXH3_4loc((const unsigned char *) str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[5];
};

