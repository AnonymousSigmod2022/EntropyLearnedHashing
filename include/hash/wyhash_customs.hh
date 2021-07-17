
#include "hash/wyhash.hh"
#include "hash/orig_xhash.hh"

#include <emmintrin.h>
#include <smmintrin.h>
#include <nmmintrin.h>

/* wyhash 1,2,3,4 loc */
static inline uint64_t wyhash_1loc_fixed(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0, const uint64_t *secret = _wyp) {
	const uint8_t *p=(const uint8_t *)key;  uint64_t a,b;
	a=_wyr4(p + locations[0]); b=_wyr4(p+locations[0]+4);
	return _wymix(secret[1]^len,_wymix(a^secret[1], b^seed));
}

static inline uint64_t wyhash_1loc(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0, const uint64_t *secret = _wyp) {
	if (len >= locations[1]) {
		return wyhash_1loc_fixed(key, len, locations);
	} else {
		return wyhash(key, len, seed, secret);
	}
}

static inline uint64_t wyhash_2loc_fixed(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0, const uint64_t *secret = _wyp) {
	const uint8_t *p=(const uint8_t *)key;  uint64_t a,b; seed^=*secret;
	a=_wyr8(p+locations[0]); b=_wyr8(p+locations[1]);
	return _wymix(secret[1]^len,_wymix(a^secret[1], b^seed));
}
static inline uint64_t wyhash_2loc(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0, const uint64_t *secret = _wyp) {
	if (len >= locations[2]) {
		return wyhash_2loc_fixed(key, len, locations);
	} else {
		return wyhash(key, len, seed, secret);
	} 
}

static inline uint64_t wyhash_3loc_fixed(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0, const uint64_t *secret = _wyp) {
	const uint8_t *p=(const uint8_t *)key;  uint64_t a,b; seed^=*secret;
	a=_wyr8(p+locations[0]); b=_wyr8(p+locations[1]);
	return _wymix(_wymix(secret[1]^len, _wyr8(p+locations[2])),_wymix(a^secret[1], b^seed));
}

static inline uint64_t wyhash_3loc(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0, const uint64_t *secret = _wyp) {
	if (len >= locations[3]) {
		return wyhash_3loc_fixed(key, len, locations);
	} else {
		return wyhash(key, len, seed, secret);
	} 
}

static inline uint64_t wyhash_4loc_fixed(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0, const uint64_t *secret = _wyp) {
	const uint8_t *p=(const uint8_t *)key; seed^=*secret;
	uint64_t see1=seed;
	seed=_wymix(_wyr8(p+locations[0]), _wyr8(p+locations[1]));
	see1 = _wymix(_wyr8(p+locations[2])^secret[2],_wyr8(p+locations[3])^see1);
	seed ^= see1;
	return _wymix(secret[1]^len, seed);
}

static inline uint64_t wyhash_4loc(const void *key, uint64_t len, const unsigned int* locations, uint64_t seed = 0, const uint64_t *secret = _wyp) {
	if (len >= locations[4]) {
		return wyhash_4loc_fixed(key, len, locations);
	} else {
		return wyhash(key, len, seed, secret);
	} 
}

/* structs */
struct hash_wyhash_1loc_fixed_s {
    hash_wyhash_1loc_fixed_s(int* locs) {
        locations[0] = (unsigned int)8 * locs[0];
    }
    inline std::size_t operator()(const std::string& str) const {
        return wyhash_1loc_fixed(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[1];
};

struct hash_wyhash_1loc_s {
    hash_wyhash_1loc_s(int* locs) {
        locations[0] = (unsigned int)8 * locs[0];
        // cache endpoint (for if statement)
        locations[1] = (unsigned int)(locs[0] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        return wyhash_1loc(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[2];
};

struct hash_wyhash_2loc_fixed_s {
    hash_wyhash_2loc_fixed_s(int* locs) {
        for (int i = 0; i < 2; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
    }
    inline std::size_t operator()(const std::string& str) const {
        return wyhash_2loc_fixed(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[2];
};

struct hash_wyhash_2loc_s {
    hash_wyhash_2loc_s(int* locs) {
        for (int i = 0; i < 2; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
        // cache endpoint (for if statement)
        locations[2] = (unsigned int)(locs[1] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        //return crc32_wyhash_combo(str.data(), str.length());
        return wyhash_2loc(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[3];
};

struct hash_wyhash_3loc_fixed_s {
    hash_wyhash_3loc_fixed_s(int* locs) {
        for (int i = 0; i < 3; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
    }
    inline std::size_t operator()(const std::string& str) const {
        return wyhash_3loc_fixed(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[3];
};

struct hash_wyhash_3loc_s {
    hash_wyhash_3loc_s(int* locs) {
        for (int i = 0; i < 3; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
        // cache endpoint (for if statement)
        locations[3] = (unsigned int)(locs[2] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        //return crc32_wyhash_combo(str.data(), str.length());
        return wyhash_3loc(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[4];
};

struct hash_wyhash_4loc_fixed_s {
    hash_wyhash_4loc_fixed_s(int* locs) {
        for (int i = 0; i < 4; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
    }
    inline std::size_t operator()(const std::string& str) const {
        return wyhash_4loc_fixed(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[4];
};

struct hash_wyhash_4loc_s {
    hash_wyhash_4loc_s(int* locs) {
        for (int i = 0; i < 4; i++) {
            locations[i] = (unsigned int)8 * locs[i];
        }
        // cache endpoint (for if statement)
        locations[4] = (unsigned int)(locs[3] * 8) + 8;
    }
    inline std::size_t operator()(const std::string& str) const {
        //return crc32_wyhash_combo(str.data(), str.length());
        return wyhash_4loc(str.data(), str.length(), locations);
    }
    private:
    unsigned int locations[5];
};
