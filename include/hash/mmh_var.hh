#pragma once

static inline size_t hash_sub4_mmh2(const std::string& str,
                                      const int* locations,
                                      int nlocs) {
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.
    const uint64_t m = 0x5bd1e995;
    const int r = 24;
    // Initialize the hash to a 'random' value
    uint64_t h = str.length();
    size_t len = str.length() / 4;
    // Mix 4 bytes at a time into the hash
    const uint32_t * data = (const uint32_t *)str.data();
    unsigned loc = locations[0];
    int i = 0;
    while(loc < len && i < nlocs) {
        uint32_t k = data[loc];

        k *= m;
        k ^= k >> r;
        k *= m;

        h *= m;
        h ^= k;

        i++;
        loc = locations[i];
    }
    return h;
}

static inline size_t hash_sub4_mmh2_cust(const std::string& str,
                                      const int* locations,
                                      unsigned int nlocs) {
    // 'm' and 'r' are mixing constants generated offline.
    // They're not really 'magic', they just happen to work well.
    const uint64_t m = 0x5bd1e995;
    const int r = 24;
    // Initialize the hash to a 'random' value
    uint64_t h = str.length();
    size_t len = str.length() / 4;
    const uint32_t * data = (const uint32_t *)str.data();
    unsigned loc = locations[0];
    // Mix 4 bytes at a time into the hash
    if (len > (unsigned int) locations[nlocs-1]) {
    	for(unsigned int i = 0; i < nlocs; i++) {
    		loc = locations[i];
    		uint32_t k = data[loc];
	        k *= m;
	        k ^= k >> r;
	        k *= m;

	        h *= m;
	        h ^= k;
    	}
    } else {
    	unsigned int i = 0;
    	while(loc < len && i < nlocs-1) {
	        uint32_t k = data[loc];

	        k *= m;
	        k ^= k >> r;
	        k *= m;

	        h *= m;
	        h ^= k;

	        i++;
	        loc = locations[i];
	    }
    }
    return h;
}