
#include "hash/wyhash.hh"
#include "hash/orig_xhash.hh"
#include "hash/crc32_hash.hh"

#include <emmintrin.h>
#include <smmintrin.h>
#include <nmmintrin.h>

/* crc_wyhash_combo 1,2,3,4 */
static inline uint64_t crc32_wyhash_1loc_fixed(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0) {
	const uint8_t *p=(const uint8_t *)key;  uint64_t a;
	//uint64_t res = -1ULL;
	uint64_t res = seed + len;
	a=_wyr8(p+locations[0]); // this works without len... but mgith still be useful to have len. no speed difference
	//a=_wyr8(p+40) + len;
	res = _mm_crc32_u64(res, a);
	return res;
}

static inline uint64_t crc32_wyhash_1loc(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0) {
	if (len >= locations[1]) {
		return crc32_wyhash_1loc_fixed(key, len, locations, seed);
	} else {
		return crcHash((const char *)key, len);
	}
}

static inline uint64_t crc32_wyhash_2loc_fixed(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0) {
	const uint8_t *p=(const uint8_t *)key;  uint64_t a,b;
	uint64_t res = seed + len;
	a=_wyr8(p+locations[0]); // this works without len... but mgith still be useful to have len. no speed difference
	res = _mm_crc32_u64(res, a);
	b=_wyr8(p+locations[1]);
	res = _mm_crc32_u64(res, b);
	return res;
}

static inline uint64_t crc32_wyhash_2loc(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0) {
	if (len >= locations[2]) {
		return crc32_wyhash_2loc_fixed(key, len, locations);
	} else {
		return crcHash((const char *)key, len);
	}
}

static inline uint64_t crc32_wyhash_3loc_fixed(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0) {
	const uint8_t *p=(const uint8_t *)key;
	//uint64_t term1 = _mm_crc32_u64(seed+len, _wyr8(p+locations[2]));
	//uint64_t term2 = _mm_crc32_u64(_wyr8(p+locations[1]), _wyr8(p+locations[0]));
    //return _mm_crc32_u64(term1, term2);
    uint64_t term1 = _mm_crc32_u64(seed+len, _wyr8(p+locations[0]));
    uint64_t term2 = _mm_crc32_u64(_wyr8(p+locations[1]), _wyr8(p+locations[2]) + term1);
	return term2;
}

static inline uint64_t crc32_wyhash_3loc(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0) {
	if (len >= locations[3]) {
		return crc32_wyhash_3loc_fixed(key, len, locations);
	} else {
		return crcHash((const char *)key, len);
	}
}

static inline uint64_t crc32_wyhash_4loc_fixed(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0) {
	const uint8_t *p=(const uint8_t *)key;
	uint64_t res = _mm_crc32_u64(_wyr8(p+locations[1]), _wyr8(p+locations[0]));
	uint64_t res1 = _mm_crc32_u64((seed + len), _wyr8(p+locations[2]));
	res ^= res1;
	return _mm_crc32_u64(_wyr8(p+locations[3]), res);
}

static inline uint64_t crc32_wyhash_4loc(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0) {
	if (len >= locations[4]) {
		return crc32_wyhash_4loc_fixed(key, len, locations);
	} else {
		return crcHash((const char *) key, len);
	}
}

/* hardcode for google dataset. Worth checking into as using compilation would give us this */
static inline uint64_t crc32_wyhash_hardcode(const void *key, uint64_t len) {
	if (len > 56) {
		const uint8_t *p=(const uint8_t *)key;  uint64_t a,b;
		//uint64_t res = -1ULL;
		uint64_t res = len;
		a=_wyr8(p+40); // this works without len... but mgith still be useful to have len. no speed difference
		//a=_wyr8(p+40) + len;
		res = _mm_crc32_u64(res, a);
		b=_wyr8(p+48);
		res = _mm_crc32_u64(res, b);
		return res;
	} else {
		return wyhash(key, len, 0, _wyp);
	}
}

/* for research purposes. Shows change if locations are compiled in instead of read from array
 * Doing this consistently requires a change where we spit out code and then compile it.
 * Gives another 5-10% performance in some situations. No change in others */
struct hash_wyhash_hardcode {
    inline std::size_t operator()(const std::string& str) const {
        return crc32_wyhash_hardcode(str.data(), str.length());
    }
};

/* crcwyhash structs */
struct hash_crc32wyhash_1loc_fixed_s {
    hash_crc32wyhash_1loc_fixed_s(int* locs) {
        locations[0] = (unsigned int)8 * locs[0];
    }
    inline std::size_t operator()(const std::string& str) const {
        return crc32_wyhash_1loc_fixed(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[1];
};

struct hash_crc32wyhash_1loc_s {
    hash_crc32wyhash_1loc_s(int* locs) {
        locations[0] = (unsigned int)8 * locs[0];
        // cache endpoint (for if statement)
        locations[1] = (unsigned int)(locs[0] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        return crc32_wyhash_1loc(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[2];
};

struct hash_crc32wyhash_2loc_fixed_s {
    hash_crc32wyhash_2loc_fixed_s(int* locs) {
        for (int i = 0; i < 2; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
    }
    inline std::size_t operator()(const std::string& str) const {
        return crc32_wyhash_2loc_fixed(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[2];
};

struct hash_crc32wyhash_2loc_s {
    hash_crc32wyhash_2loc_s(int* locs) {
        for (int i = 0; i < 2; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
        // cache endpoint (for if statement)
        locations[2] = (unsigned int)(locs[1] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        //return crc32_wyhash_combo(str.data(), str.length());
        return crc32_wyhash_2loc(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[3];
};

struct hash_crc32wyhash_3loc_fixed_s {
    hash_crc32wyhash_3loc_fixed_s(int* locs) {
        for (int i = 0; i < 3; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
    }
    inline std::size_t operator()(const std::string& str) const {
        return crc32_wyhash_3loc_fixed(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[3];
};

struct hash_crc32wyhash_3loc_s {
    hash_crc32wyhash_3loc_s(int* locs) {
        for (int i = 0; i < 3; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
        // cache endpoint (for if statement)
        locations[3] = (unsigned int)(locs[2] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        //return crc32_wyhash_combo(str.data(), str.length());
        return crc32_wyhash_3loc(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[4];
};

struct hash_crc32wyhash_4loc_fixed_s {
    hash_crc32wyhash_4loc_fixed_s(int* locs) {
        for (int i = 0; i < 4; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
    }
    inline std::size_t operator()(const std::string& str) const {
        //return crc32_wyhash_combo(str.data(), str.length());
        return crc32_wyhash_4loc_fixed(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[4];
};

struct hash_crc32wyhash_4loc_s {
    hash_crc32wyhash_4loc_s(int* locs) {
        for (int i = 0; i < 4; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
        // cache endpoint (for if statement)
        locations[4] = (unsigned int)(locs[3] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        //return crc32_wyhash_combo(str.data(), str.length());
        return crc32_wyhash_4loc(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[5];
};
