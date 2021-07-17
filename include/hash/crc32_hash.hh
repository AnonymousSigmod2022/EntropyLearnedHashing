#pragma once

#include <cstdint>
#include <string>
#include <type_traits>

#include <emmintrin.h>
#include <smmintrin.h>
#include <nmmintrin.h>

#ifndef __cpp_char8_t
using char8_t = unsigned char;
#endif

using UInt8 = char8_t;
using UInt16 = uint16_t;
using UInt32 = uint32_t;
using UInt64 = uint64_t;

template <typename T>
inline T unalignedLoad(const void * address)
{
    T res {};
    memcpy(&res, address, sizeof(res));
    return res;
}

// Hash 128 input bits down to 64 bits of output.
// This is intended to be a reasonably good hash function.
inline UInt64 Hash128to64(const uint64_t& x1, uint64_t& x2) {
  // Murmur-inspired hashing.
  const UInt64 kMul = 0x9ddfea08eb382d69ULL;
  UInt64 a = (x1 ^ x2) * kMul;
  a ^= (a >> 47);
  UInt64 b = (x2 ^ a) * kMul;
  b ^= (b >> 47);
  b *= kMul;
  return b;
}

/// Parts are taken from CityHash.
inline UInt64 hashLen16(UInt64 u, UInt64 v)
{
	return Hash128to64(u,v);
}
inline UInt64 shiftMix(UInt64 val)
{
    return val ^ (val >> 47);
}
inline UInt64 rotateByAtLeast1(UInt64 val, int shift)
{
    return (val >> shift) | (val << (64 - shift));
}
inline size_t hashLessThan8(const char * data, size_t size)
{
    static constexpr UInt64 k2 = 0x9ae16a3b2f90404fULL;
    static constexpr UInt64 k3 = 0xc949d7c7509e6557ULL;
    if (size >= 4)
    {
        UInt64 a = unalignedLoad<uint32_t>(data);
        return hashLen16(size + (a << 3), unalignedLoad<uint32_t>(data + size - 4));
    }
    if (size > 0)
    {
        uint8_t a = data[0];
        uint8_t b = data[size >> 1];
        uint8_t c = data[size - 1];
        uint32_t y = static_cast<uint32_t>(a) + (static_cast<uint32_t>(b) << 8);
        uint32_t z = size + (static_cast<uint32_t>(c) << 2);
        return shiftMix(y * k2 ^ z * k3) * k2;
    }
    return k2;
}
inline size_t hashLessThan16(const char * data, size_t size)
{
    if (size > 8)
    {
        UInt64 a = unalignedLoad<UInt64>(data);
        UInt64 b = unalignedLoad<UInt64>(data + size - 8);
        return hashLen16(a, rotateByAtLeast1(b + size, size)) ^ b;
    }
    return hashLessThan8(data, size);
}
inline size_t crcHash(const char * data, size_t size) {
    const char * pos = data;
    if (size == 0)
        return 0;
    if (size < 8)
    {
        return hashLessThan8(pos, size);
    }
    const char * end = pos + size;
    size_t res = -1ULL;
    do
    {
        UInt64 word = unalignedLoad<UInt64>(pos);
        res = _mm_crc32_u64(res, word);
        pos += 8;
    } while (pos + 8 < end);
    UInt64 word = unalignedLoad<UInt64>(end - 8);
    res = _mm_crc32_u64(res, word);
    return res;
}

struct CRC32Hash_s {
    inline size_t operator() (const std::string& x) const {
        return crcHash(x.data(), x.length());
    }
};