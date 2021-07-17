#pragma once

/*
 * This file is a pared down version of xxhash. It contains exactly the same code
 * and the timing has been checked to be identical. Mainly it contains only the code for
 * the 64 bit version of xxhash without the streaming component and 128 or 32 bit versions.
 * 
 */


extern "C" {

#define XXH_FORCE_INLINE static inline
#if defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)   /* C11+ */
#  include <stdalign.h>
#  define XXH_ALIGN(n)      alignas(n)
#elif defined(__GNUC__)
#  define XXH_ALIGN(n)      __attribute__ ((aligned(n)))
#elif defined(_MSC_VER)
#  define XXH_ALIGN(n)      __declspec(align(n))
#else
#  define XXH_ALIGN(n)   /* disabled */
#endif

#ifdef __has_builtin
#  define XXH_HAS_BUILTIN(x) __has_builtin(x)
#else
#  define XXH_HAS_BUILTIN(x) 0
#endif

#if defined (__STDC_VERSION__) && __STDC_VERSION__ >= 199901L   /* >= C99 */
#  define XXH_RESTRICT   restrict
#else
/* Note: it might be useful to define __restrict or __restrict__ for some C++ compilers */
#  define XXH_RESTRICT   /* disable */
#endif

#if (defined(__GNUC__) && (__GNUC__ >= 3))  \
  || (defined(__INTEL_COMPILER) && (__INTEL_COMPILER >= 800)) \
  || defined(__clang__)
#    define XXH_likely(x) __builtin_expect(x, 1)
#    define XXH_unlikely(x) __builtin_expect(x, 0)
#else
#    define XXH_likely(x) (x)
#    define XXH_unlikely(x) (x)
#endif

#if !defined(NO_CLANG_BUILTIN) && XXH_HAS_BUILTIN(__builtin_rotateleft32) \
                               && XXH_HAS_BUILTIN(__builtin_rotateleft64)
#  define XXH_rotl32 __builtin_rotateleft32
#  define XXH_rotl64 __builtin_rotateleft64
/* Note: although _rotl exists for minGW (GCC under windows), performance seems poor */
#elif defined(_MSC_VER)
#  define XXH_rotl32(x,r) _rotl(x,r)
#  define XXH_rotl64(x,r) _rotl64(x,r)
#else
#  define XXH_rotl32(x,r) (((x) << (r)) | ((x) >> (32 - (r))))
#  define XXH_rotl64(x,r) (((x) << (r)) | ((x) >> (64 - (r))))
#endif

#ifndef XXH_CPU_LITTLE_ENDIAN
/*
 * Try to detect endianness automatically, to avoid the nonstandard behavior
 * in `XXH_isLittleEndian()`
 */
#  if defined(_WIN32) /* Windows is always little endian */ \
     || defined(__LITTLE_ENDIAN__) \
     || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#    define XXH_CPU_LITTLE_ENDIAN 1
#  elif defined(__BIG_ENDIAN__) \
     || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    define XXH_CPU_LITTLE_ENDIAN 0
#  else
/*!
 * @internal
 * @brief Runtime check for @ref XXH_CPU_LITTLE_ENDIAN.
 *
 * Most compilers will constant fold this.
 */
static int XXH_isLittleEndian(void)
{
    /*
     * Portable and well-defined behavior.
     * Don't use static: it is detrimental to performance.
     */
    const union { xxh_u32 u; xxh_u8 c[4]; } one = { 1 };
    return one.c[0];
}
#   define XXH_CPU_LITTLE_ENDIAN   XXH_isLittleEndian()
#  endif
#endif

# include <stdint.h>
typedef uint8_t xxh_u8;
typedef uint32_t XXH32_hash_t;
typedef XXH32_hash_t xxh_u32;
typedef uint64_t XXH64_hash_t;
typedef XXH64_hash_t xxh_u64;
typedef struct {
    XXH64_hash_t low64;   /*!< `value & 0xFFFFFFFFFFFFFFFF` */
    XXH64_hash_t high64;  /*!< `value >> 64` */
} XXH128_hash_t;

#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==3))
/*
 * Manual byteshift. Best for old compilers which don't inline memcpy.
 * We actually directly use XXH_readLE32 and XXH_readBE32.
 */
#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==2))

/*
 * Force direct memory access. Only works on CPU which support unaligned memory
 * access in hardware.
 */
static xxh_u32 XXH_read32(const void* memPtr) { return *(const xxh_u32*) memPtr; }

#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==1))

/*
 * __pack instructions are safer but compiler specific, hence potentially
 * problematic for some compilers.
 *
 * Currently only defined for GCC and ICC.
 */
#ifdef XXH_OLD_NAMES
typedef union { xxh_u32 u32; } __attribute__((packed)) unalign;
#endif
static xxh_u32 XXH_read32(const void* ptr)
{
    typedef union { xxh_u32 u32; } __attribute__((packed)) xxh_unalign;
    return ((const xxh_unalign*)ptr)->u32;
}

#else

/*
 * Portable and safe solution. Generally efficient.
 * see: https://stackoverflow.com/a/32095106/646947
 */
static xxh_u32 XXH_read32(const void* memPtr)
{
    xxh_u32 val;
    memcpy(&val, memPtr, sizeof(val));
    return val;
}

#endif   /* XXH_FORCE_DIRECT_MEMORY_ACCESS */


/* ***   Endianness   *** */
typedef enum { XXH_bigEndian=0, XXH_littleEndian=1 } XXH_endianess;

/*!
 * @ingroup tuning
 * @def XXH_CPU_LITTLE_ENDIAN
 * @brief Whether the target is little endian.
 *
 * Defined to 1 if the target is little endian, or 0 if it is big endian.
 * It can be defined externally, for example on the compiler command line.
 *
 * If it is not defined, a runtime check (which is usually constant folded)
 * is used instead.
 *
 * @note
 *   This is not necessarily defined to an integer constant.
 *
 * @see XXH_isLittleEndian() for the runtime check.
 */
#ifndef XXH_CPU_LITTLE_ENDIAN
/*
 * Try to detect endianness automatically, to avoid the nonstandard behavior
 * in `XXH_isLittleEndian()`
 */
#  if defined(_WIN32) /* Windows is always little endian */ \
     || defined(__LITTLE_ENDIAN__) \
     || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__)
#    define XXH_CPU_LITTLE_ENDIAN 1
#  elif defined(__BIG_ENDIAN__) \
     || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    define XXH_CPU_LITTLE_ENDIAN 0
#  else
/*!
 * @internal
 * @brief Runtime check for @ref XXH_CPU_LITTLE_ENDIAN.
 *
 * Most compilers will constant fold this.
 */
static int XXH_isLittleEndian(void)
{
    /*
     * Portable and well-defined behavior.
     * Don't use static: it is detrimental to performance.
     */
    const union { xxh_u32 u; xxh_u8 c[4]; } one = { 1 };
    return one.c[0];
}
#   define XXH_CPU_LITTLE_ENDIAN   XXH_isLittleEndian()
#  endif
#endif




/* ****************************************
*  Compiler-specific Functions and Macros
******************************************/
#define XXH_GCC_VERSION (__GNUC__ * 100 + __GNUC_MINOR__)

#ifdef __has_builtin
#  define XXH_HAS_BUILTIN(x) __has_builtin(x)
#else
#  define XXH_HAS_BUILTIN(x) 0
#endif

/*!
 * @internal
 * @def XXH_rotl32(x,r)
 * @brief 32-bit rotate left.
 *
 * @param x The 32-bit integer to be rotated.
 * @param r The number of bits to rotate.
 * @pre
 *   @p r > 0 && @p r < 32
 * @note
 *   @p x and @p r may be evaluated multiple times.
 * @return The rotated result.
 */
#if !defined(NO_CLANG_BUILTIN) && XXH_HAS_BUILTIN(__builtin_rotateleft32) \
                               && XXH_HAS_BUILTIN(__builtin_rotateleft64)
#  define XXH_rotl32 __builtin_rotateleft32
#  define XXH_rotl64 __builtin_rotateleft64
/* Note: although _rotl exists for minGW (GCC under windows), performance seems poor */
#elif defined(_MSC_VER)
#  define XXH_rotl32(x,r) _rotl(x,r)
#  define XXH_rotl64(x,r) _rotl64(x,r)
#else
#  define XXH_rotl32(x,r) (((x) << (r)) | ((x) >> (32 - (r))))
#  define XXH_rotl64(x,r) (((x) << (r)) | ((x) >> (64 - (r))))
#endif

/*!
 * @internal
 * @fn xxh_u32 XXH_swap32(xxh_u32 x)
 * @brief A 32-bit byteswap.
 *
 * @param x The 32-bit integer to byteswap.
 * @return @p x, byteswapped.
 */
#if defined(_MSC_VER)     /* Visual Studio */
#  define XXH_swap32 _byteswap_ulong
#elif XXH_GCC_VERSION >= 403
#  define XXH_swap32 __builtin_bswap32
#else
static xxh_u32 XXH_swap32 (xxh_u32 x)
{
    return  ((x << 24) & 0xff000000 ) |
            ((x <<  8) & 0x00ff0000 ) |
            ((x >>  8) & 0x0000ff00 ) |
            ((x >> 24) & 0x000000ff );
}
#endif


/* ***************************
*  Memory reads
*****************************/

/*!
 * @internal
 * @brief Enum to indicate whether a pointer is aligned.
 */
typedef enum {
    XXH_aligned,  /*!< Aligned */
    XXH_unaligned /*!< Possibly unaligned */
} XXH_alignment;

/*
 * XXH_FORCE_MEMORY_ACCESS==3 is an endian-independent byteshift load.
 *
 * This is ideal for older compilers which don't inline memcpy.
 */
#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==3))

XXH_FORCE_INLINE xxh_u32 XXH_readLE32(const void* memPtr)
{
    const xxh_u8* bytePtr = (const xxh_u8 *)memPtr;
    return bytePtr[0]
         | ((xxh_u32)bytePtr[1] << 8)
         | ((xxh_u32)bytePtr[2] << 16)
         | ((xxh_u32)bytePtr[3] << 24);
}

/*XXH_FORCE_INLINE xxh_u32 XXH_readBE32(const void* memPtr)
{
    const xxh_u8* bytePtr = (const xxh_u8 *)memPtr;
    return bytePtr[3]
         | ((xxh_u32)bytePtr[2] << 8)
         | ((xxh_u32)bytePtr[1] << 16)
         | ((xxh_u32)bytePtr[0] << 24);
} */

#else
XXH_FORCE_INLINE xxh_u32 XXH_readLE32(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_read32(ptr) : XXH_swap32(XXH_read32(ptr));
}

/*static xxh_u32 XXH_readBE32(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_swap32(XXH_read32(ptr)) : XXH_read32(ptr);
} */
#endif

XXH_FORCE_INLINE xxh_u32
XXH_readLE32_align(const void* ptr, XXH_alignment align)
{
    if (align==XXH_unaligned) {
        return XXH_readLE32(ptr);
    } else {
        return XXH_CPU_LITTLE_ENDIAN ? *(const xxh_u32*)ptr : XXH_swap32(*(const xxh_u32*)ptr);
    }
}

static const xxh_u32 XXH_PRIME32_1 = 0x9E3779B1U;   /*!< 0b10011110001101110111100110110001 */
static const xxh_u32 XXH_PRIME32_2 = 0x85EBCA77U;   /*!< 0b10000101111010111100101001110111 */
static const xxh_u32 XXH_PRIME32_3 = 0xC2B2AE3DU;   /*!< 0b11000010101100101010111000111101 */
static const xxh_u32 XXH_PRIME32_4 = 0x27D4EB2FU;   /*!< 0b00100111110101001110101100101111 */
static const xxh_u32 XXH_PRIME32_5 = 0x165667B1U;

static const xxh_u64 XXH_PRIME64_1 = 0x9E3779B185EBCA87ULL;   /*!< 0b1001111000110111011110011011000110000101111010111100101010000111 */
static const xxh_u64 XXH_PRIME64_2 = 0xC2B2AE3D27D4EB4FULL;   /*!< 0b1100001010110010101011100011110100100111110101001110101101001111 */
static const xxh_u64 XXH_PRIME64_3 = 0x165667B19E3779F9ULL;   /*!< 0b0001011001010110011001111011000110011110001101110111100111111001 */
static const xxh_u64 XXH_PRIME64_4 = 0x85EBCA77C2B2AE63ULL;   /*!< 0b1000010111101011110010100111011111000010101100101010111001100011 */
static const xxh_u64 XXH_PRIME64_5 = 0x27D4EB2F165667C5ULL;

#define XXH_SECRET_DEFAULT_SIZE 192   /* minimum XXH3_SECRET_SIZE_MIN */
#define XXH3_SECRET_SIZE_MIN 136


/*! Pseudorandom secret taken directly from FARSH. */
XXH_ALIGN(64) static const xxh_u8 XXH3_kSecret[XXH_SECRET_DEFAULT_SIZE] = {
    0xb8, 0xfe, 0x6c, 0x39, 0x23, 0xa4, 0x4b, 0xbe, 0x7c, 0x01, 0x81, 0x2c, 0xf7, 0x21, 0xad, 0x1c,
    0xde, 0xd4, 0x6d, 0xe9, 0x83, 0x90, 0x97, 0xdb, 0x72, 0x40, 0xa4, 0xa4, 0xb7, 0xb3, 0x67, 0x1f,
    0xcb, 0x79, 0xe6, 0x4e, 0xcc, 0xc0, 0xe5, 0x78, 0x82, 0x5a, 0xd0, 0x7d, 0xcc, 0xff, 0x72, 0x21,
    0xb8, 0x08, 0x46, 0x74, 0xf7, 0x43, 0x24, 0x8e, 0xe0, 0x35, 0x90, 0xe6, 0x81, 0x3a, 0x26, 0x4c,
    0x3c, 0x28, 0x52, 0xbb, 0x91, 0xc3, 0x00, 0xcb, 0x88, 0xd0, 0x65, 0x8b, 0x1b, 0x53, 0x2e, 0xa3,
    0x71, 0x64, 0x48, 0x97, 0xa2, 0x0d, 0xf9, 0x4e, 0x38, 0x19, 0xef, 0x46, 0xa9, 0xde, 0xac, 0xd8,
    0xa8, 0xfa, 0x76, 0x3f, 0xe3, 0x9c, 0x34, 0x3f, 0xf9, 0xdc, 0xbb, 0xc7, 0xc7, 0x0b, 0x4f, 0x1d,
    0x8a, 0x51, 0xe0, 0x4b, 0xcd, 0xb4, 0x59, 0x31, 0xc8, 0x9f, 0x7e, 0xc9, 0xd9, 0x78, 0x73, 0x64,
    0xea, 0xc5, 0xac, 0x83, 0x34, 0xd3, 0xeb, 0xc3, 0xc5, 0x81, 0xa0, 0xff, 0xfa, 0x13, 0x63, 0xeb,
    0x17, 0x0d, 0xdd, 0x51, 0xb7, 0xf0, 0xda, 0x49, 0xd3, 0x16, 0x55, 0x26, 0x29, 0xd4, 0x68, 0x9e,
    0x2b, 0x16, 0xbe, 0x58, 0x7d, 0x47, 0xa1, 0xfc, 0x8f, 0xf8, 0xb8, 0xd1, 0x7a, 0xd0, 0x31, 0xce,
    0x45, 0xcb, 0x3a, 0x8f, 0x95, 0x16, 0x04, 0x28, 0xaf, 0xd7, 0xfb, 0xca, 0xbb, 0x4b, 0x40, 0x7e,
};

#if (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==3))
/*
 * Manual byteshift. Best for old compilers which don't inline memcpy.
 * We actually directly use XXH_readLE64 and XXH_readBE64.
 */
#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==2))

/* Force direct memory access. Only works on CPU which support unaligned memory access in hardware */
static xxh_u64 XXH_read64(const void* memPtr)
{
    return *(const xxh_u64*) memPtr;
}

#elif (defined(XXH_FORCE_MEMORY_ACCESS) && (XXH_FORCE_MEMORY_ACCESS==1))

/*
 * __pack instructions are safer, but compiler specific, hence potentially
 * problematic for some compilers.
 *
 * Currently only defined for GCC and ICC.
 */
#ifdef XXH_OLD_NAMES
typedef union { xxh_u32 u32; xxh_u64 u64; } __attribute__((packed)) unalign64;
#endif
static xxh_u64 XXH_read64(const void* ptr)
{
    typedef union { xxh_u32 u32; xxh_u64 u64; } __attribute__((packed)) xxh_unalign64;
    return ((const xxh_unalign64*)ptr)->u64;
}

#else

/*
 * Portable and safe solution. Generally efficient.
 * see: https://stackoverflow.com/a/32095106/646947
 */
static xxh_u64 XXH_read64(const void* memPtr)
{
    xxh_u64 val;
    memcpy(&val, memPtr, sizeof(val));
    return val;
}

#endif   /* XXH_FORCE_DIRECT_MEMORY_ACCESS */

#if defined(_MSC_VER)     /* Visual Studio */
#  define XXH_swap64 _byteswap_uint64
#elif XXH_GCC_VERSION >= 403
#  define XXH_swap64 __builtin_bswap64
#else
static xxh_u64 XXH_swap64(xxh_u64 x)
{
    return  ((x << 56) & 0xff00000000000000ULL) |
            ((x << 40) & 0x00ff000000000000ULL) |
            ((x << 24) & 0x0000ff0000000000ULL) |
            ((x << 8)  & 0x000000ff00000000ULL) |
            ((x >> 8)  & 0x00000000ff000000ULL) |
            ((x >> 24) & 0x0000000000ff0000ULL) |
            ((x >> 40) & 0x000000000000ff00ULL) |
            ((x >> 56) & 0x00000000000000ffULL);
}
#endif

static inline xxh_u64 XXH_readLE64(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_read64(ptr) : XXH_swap64(XXH_read64(ptr));
}

/*static xxh_u64 XXH_readBE64(const void* ptr)
{
    return XXH_CPU_LITTLE_ENDIAN ? XXH_swap64(XXH_read64(ptr)) : XXH_read64(ptr);
} */

/*static xxh_u32 XXH32_avalanche(xxh_u32 h32)
{
    h32 ^= h32 >> 15;
    h32 *= XXH_PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= XXH_PRIME32_3;
    h32 ^= h32 >> 16;
    return(h32);
} */

/* ==========================================
 * Vectorization detection
 * ========================================== */

#ifdef XXH_DOXYGEN
/*!
 * @ingroup tuning
 * @brief Overrides the vectorization implementation chosen for XXH3.
 *
 * Can be defined to 0 to disable SIMD or any of the values mentioned in
 * @ref XXH_VECTOR_TYPE.
 *
 * If this is not defined, it uses predefined macros to determine the best
 * implementation.
 */
#  define XXH_VECTOR XXH_SCALAR
/*!
 * @ingroup tuning
 * @brief Possible values for @ref XXH_VECTOR.
 *
 * Note that these are actually implemented as macros.
 *
 * If this is not defined, it is detected automatically.
 * @ref XXH_X86DISPATCH overrides this.
 */
enum XXH_VECTOR_TYPE /* fake enum */ {
    XXH_SCALAR = 0,  /*!< Portable scalar version */
    XXH_SSE2   = 1,  /*!<
                      * SSE2 for Pentium 4, Opteron, all x86_64.
                      *
                      * @note SSE2 is also guaranteed on Windows 10, macOS, and
                      * Android x86.
                      */
    XXH_AVX2   = 2,  /*!< AVX2 for Haswell and Bulldozer */
    XXH_AVX512 = 3,  /*!< AVX512 for Skylake and Icelake */
    XXH_NEON   = 4,  /*!< NEON for most ARMv7-A and all AArch64 */
    XXH_VSX    = 5,  /*!< VSX and ZVector for POWER8/z13 (64-bit) */
};
/*!
 * @ingroup tuning
 * @brief Selects the minimum alignment for XXH3's accumulators.
 *
 * When using SIMD, this should match the alignment reqired for said vector
 * type, so, for example, 32 for AVX2.
 *
 * Default: Auto detected.
 */
#  define XXH_ACC_ALIGN 8
#endif

/* Actual definition */
#ifndef XXH_DOXYGEN
#  define XXH_SCALAR 0
#  define XXH_SSE2   1
#  define XXH_AVX2   2
#  define XXH_AVX512 3
#  define XXH_NEON   4
#  define XXH_VSX    5
#endif

#ifndef XXH_VECTOR    /* can be defined on command line */
#  if defined(__AVX512F__)
#    define XXH_VECTOR XXH_AVX512
#  elif defined(__AVX2__)
#    define XXH_VECTOR XXH_AVX2
#  elif defined(__SSE2__) || defined(_M_AMD64) || defined(_M_X64) || (defined(_M_IX86_FP) && (_M_IX86_FP == 2))
#    define XXH_VECTOR XXH_SSE2
#  elif defined(__GNUC__) /* msvc support maybe later */ \
  && (defined(__ARM_NEON__) || defined(__ARM_NEON)) \
  && (defined(__LITTLE_ENDIAN__) /* We only support little endian NEON */ \
    || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
#    define XXH_VECTOR XXH_NEON
#  elif (defined(__PPC64__) && defined(__POWER8_VECTOR__)) \
     || (defined(__s390x__) && defined(__VEC__)) \
     && defined(__GNUC__) /* TODO: IBM XL */
#    define XXH_VECTOR XXH_VSX
#  else
#    define XXH_VECTOR XXH_SCALAR
#  endif
#endif

/*
 * Controls the alignment of the accumulator,
 * for compatibility with aligned vector loads, which are usually faster.
 */
#ifndef XXH_ACC_ALIGN
#  if defined(XXH_X86DISPATCH)
#     define XXH_ACC_ALIGN 64  /* for compatibility with avx512 */
#  elif XXH_VECTOR == XXH_SCALAR  /* scalar */
#     define XXH_ACC_ALIGN 8
#  elif XXH_VECTOR == XXH_SSE2  /* sse2 */
#     define XXH_ACC_ALIGN 16
#  elif XXH_VECTOR == XXH_AVX2  /* avx2 */
#     define XXH_ACC_ALIGN 32
#  elif XXH_VECTOR == XXH_NEON  /* neon */
#     define XXH_ACC_ALIGN 16
#  elif XXH_VECTOR == XXH_VSX   /* vsx */
#     define XXH_ACC_ALIGN 16
#  elif XXH_VECTOR == XXH_AVX512  /* avx512 */
#     define XXH_ACC_ALIGN 64
#  endif
#endif

#if defined(XXH_X86DISPATCH) || XXH_VECTOR == XXH_SSE2 \
    || XXH_VECTOR == XXH_AVX2 || XXH_VECTOR == XXH_AVX512
#  define XXH_SEC_ALIGN XXH_ACC_ALIGN
#else
#  define XXH_SEC_ALIGN 8
#endif

/*
 * UGLY HACK:
 * GCC usually generates the best code with -O3 for xxHash.
 *
 * However, when targeting AVX2, it is overzealous in its unrolling resulting
 * in code roughly 3/4 the speed of Clang.
 *
 * There are other issues, such as GCC splitting _mm256_loadu_si256 into
 * _mm_loadu_si128 + _mm256_inserti128_si256. This is an optimization which
 * only applies to Sandy and Ivy Bridge... which don't even support AVX2.
 *
 * That is why when compiling the AVX2 version, it is recommended to use either
 *   -O2 -mavx2 -march=haswell
 * or
 *   -O2 -mavx2 -mno-avx256-split-unaligned-load
 * for decent performance, or to use Clang instead.
 *
 * Fortunately, we can control the first one with a pragma that forces GCC into
 * -O2, but the other one we can't control without "failed to inline always
 * inline function due to target mismatch" warnings.
 */
#if XXH_VECTOR == XXH_AVX2 /* AVX2 */ \
  && defined(__GNUC__) && !defined(__clang__) /* GCC, not Clang */ \
  && defined(__OPTIMIZE__) && !defined(__OPTIMIZE_SIZE__) /* respect -O0 and -Os */
#  pragma GCC push_options
#  pragma GCC optimize("-O2")
#endif


#if XXH_VECTOR == XXH_NEON
/*
 * NEON's setup for vmlal_u32 is a little more complicated than it is on
 * SSE2, AVX2, and VSX.
 *
 * While PMULUDQ and VMULEUW both perform a mask, VMLAL.U32 performs an upcast.
 *
 * To do the same operation, the 128-bit 'Q' register needs to be split into
 * two 64-bit 'D' registers, performing this operation::
 *
 *   [                a                 |                 b                ]
 *            |              '---------. .--------'                |
 *            |                         x                          |
 *            |              .---------' '--------.                |
 *   [ a & 0xFFFFFFFF | b & 0xFFFFFFFF ],[    a >> 32     |     b >> 32    ]
 *
 * Due to significant changes in aarch64, the fastest method for aarch64 is
 * completely different than the fastest method for ARMv7-A.
 *
 * ARMv7-A treats D registers as unions overlaying Q registers, so modifying
 * D11 will modify the high half of Q5. This is similar to how modifying AH
 * will only affect bits 8-15 of AX on x86.
 *
 * VZIP takes two registers, and puts even lanes in one register and odd lanes
 * in the other.
 *
 * On ARMv7-A, this strangely modifies both parameters in place instead of
 * taking the usual 3-operand form.
 *
 * Therefore, if we want to do this, we can simply use a D-form VZIP.32 on the
 * lower and upper halves of the Q register to end up with the high and low
 * halves where we want - all in one instruction.
 *
 *   vzip.32   d10, d11       @ d10 = { d10[0], d11[0] }; d11 = { d10[1], d11[1] }
 *
 * Unfortunately we need inline assembly for this: Instructions modifying two
 * registers at once is not possible in GCC or Clang's IR, and they have to
 * create a copy.
 *
 * aarch64 requires a different approach.
 *
 * In order to make it easier to write a decent compiler for aarch64, many
 * quirks were removed, such as conditional execution.
 *
 * NEON was also affected by this.
 *
 * aarch64 cannot access the high bits of a Q-form register, and writes to a
 * D-form register zero the high bits, similar to how writes to W-form scalar
 * registers (or DWORD registers on x86_64) work.
 *
 * The formerly free vget_high intrinsics now require a vext (with a few
 * exceptions)
 *
 * Additionally, VZIP was replaced by ZIP1 and ZIP2, which are the equivalent
 * of PUNPCKL* and PUNPCKH* in SSE, respectively, in order to only modify one
 * operand.
 *
 * The equivalent of the VZIP.32 on the lower and upper halves would be this
 * mess:
 *
 *   ext     v2.4s, v0.4s, v0.4s, #2 // v2 = { v0[2], v0[3], v0[0], v0[1] }
 *   zip1    v1.2s, v0.2s, v2.2s     // v1 = { v0[0], v2[0] }
 *   zip2    v0.2s, v0.2s, v1.2s     // v0 = { v0[1], v2[1] }
 *
 * Instead, we use a literal downcast, vmovn_u64 (XTN), and vshrn_n_u64 (SHRN):
 *
 *   shrn    v1.2s, v0.2d, #32  // v1 = (uint32x2_t)(v0 >> 32);
 *   xtn     v0.2s, v0.2d       // v0 = (uint32x2_t)(v0 & 0xFFFFFFFF);
 *
 * This is available on ARMv7-A, but is less efficient than a single VZIP.32.
 */

/*!
 * Function-like macro:
 * void XXH_SPLIT_IN_PLACE(uint64x2_t &in, uint32x2_t &outLo, uint32x2_t &outHi)
 * {
 *     outLo = (uint32x2_t)(in & 0xFFFFFFFF);
 *     outHi = (uint32x2_t)(in >> 32);
 *     in = UNDEFINED;
 * }
 */
# if !defined(XXH_NO_VZIP_HACK) /* define to disable */ \
   && defined(__GNUC__) \
   && !defined(__aarch64__) && !defined(__arm64__)
#  define XXH_SPLIT_IN_PLACE(in, outLo, outHi)                                              \
    do {                                                                                    \
      /* Undocumented GCC/Clang operand modifier: %e0 = lower D half, %f0 = upper D half */ \
      /* https://github.com/gcc-mirror/gcc/blob/38cf91e5/gcc/config/arm/arm.c#L22486 */     \
      /* https://github.com/llvm-mirror/llvm/blob/2c4ca683/lib/Target/ARM/ARMAsmPrinter.cpp#L399 */ \
      __asm__("vzip.32  %e0, %f0" : "+w" (in));                                             \
      (outLo) = vget_low_u32 (vreinterpretq_u32_u64(in));                                   \
      (outHi) = vget_high_u32(vreinterpretq_u32_u64(in));                                   \
   } while (0)
# else
#  define XXH_SPLIT_IN_PLACE(in, outLo, outHi)                                            \
    do {                                                                                  \
      (outLo) = vmovn_u64    (in);                                                        \
      (outHi) = vshrn_n_u64  ((in), 32);                                                  \
    } while (0)
# endif
#endif  /* XXH_VECTOR == XXH_NEON */

/*
 * VSX and Z Vector helpers.
 *
 * This is very messy, and any pull requests to clean this up are welcome.
 *
 * There are a lot of problems with supporting VSX and s390x, due to
 * inconsistent intrinsics, spotty coverage, and multiple endiannesses.
 */
#if XXH_VECTOR == XXH_VSX
#  if defined(__s390x__)
#    include <s390intrin.h>
#  else
/* gcc's altivec.h can have the unwanted consequence to unconditionally
 * #define bool, vector, and pixel keywords,
 * with bad consequences for programs already using these keywords for other purposes.
 * The paragraph defining these macros is skipped when __APPLE_ALTIVEC__ is defined.
 * __APPLE_ALTIVEC__ is _generally_ defined automatically by the compiler,
 * but it seems that, in some cases, it isn't.
 * Force the build macro to be defined, so that keywords are not altered.
 */
#    if defined(__GNUC__) && !defined(__APPLE_ALTIVEC__)
#      define __APPLE_ALTIVEC__
#    endif
#    include <altivec.h>
#  endif

typedef __vector unsigned long long xxh_u64x2;
typedef __vector unsigned char xxh_u8x16;
typedef __vector unsigned xxh_u32x4;

# ifndef XXH_VSX_BE
#  if defined(__BIG_ENDIAN__) \
  || (defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
#    define XXH_VSX_BE 1
#  elif defined(__VEC_ELEMENT_REG_ORDER__) && __VEC_ELEMENT_REG_ORDER__ == __ORDER_BIG_ENDIAN__
#    warning "-maltivec=be is not recommended. Please use native endianness."
#    define XXH_VSX_BE 1
#  else
#    define XXH_VSX_BE 0
#  endif
# endif /* !defined(XXH_VSX_BE) */

# if XXH_VSX_BE
#  if defined(__POWER9_VECTOR__) || (defined(__clang__) && defined(__s390x__))
#    define XXH_vec_revb vec_revb
#  else
/*!
 * A polyfill for POWER9's vec_revb().
 */
XXH_FORCE_INLINE xxh_u64x2 XXH_vec_revb(xxh_u64x2 val)
{
    xxh_u8x16 const vByteSwap = { 0x07, 0x06, 0x05, 0x04, 0x03, 0x02, 0x01, 0x00,
                                  0x0F, 0x0E, 0x0D, 0x0C, 0x0B, 0x0A, 0x09, 0x08 };
    return vec_perm(val, val, vByteSwap);
}
#  endif
# endif /* XXH_VSX_BE */

/*!
 * Performs an unaligned vector load and byte swaps it on big endian.
 */
XXH_FORCE_INLINE xxh_u64x2 XXH_vec_loadu(const void *ptr)
{
    xxh_u64x2 ret;
    memcpy(&ret, ptr, sizeof(xxh_u64x2));
# if XXH_VSX_BE
    ret = XXH_vec_revb(ret);
# endif
    return ret;
}

/*
 * vec_mulo and vec_mule are very problematic intrinsics on PowerPC
 *
 * These intrinsics weren't added until GCC 8, despite existing for a while,
 * and they are endian dependent. Also, their meaning swap depending on version.
 * */
# if defined(__s390x__)
 /* s390x is always big endian, no issue on this platform */
#  define XXH_vec_mulo vec_mulo
#  define XXH_vec_mule vec_mule
# elif defined(__clang__) && XXH_HAS_BUILTIN(__builtin_altivec_vmuleuw)
/* Clang has a better way to control this, we can just use the builtin which doesn't swap. */
#  define XXH_vec_mulo __builtin_altivec_vmulouw
#  define XXH_vec_mule __builtin_altivec_vmuleuw
# else
/* gcc needs inline assembly */
/* Adapted from https://github.com/google/highwayhash/blob/master/highwayhash/hh_vsx.h. */
XXH_FORCE_INLINE xxh_u64x2 XXH_vec_mulo(xxh_u32x4 a, xxh_u32x4 b)
{
    xxh_u64x2 result;
    __asm__("vmulouw %0, %1, %2" : "=v" (result) : "v" (a), "v" (b));
    return result;
}
XXH_FORCE_INLINE xxh_u64x2 XXH_vec_mule(xxh_u32x4 a, xxh_u32x4 b)
{
    xxh_u64x2 result;
    __asm__("vmuleuw %0, %1, %2" : "=v" (result) : "v" (a), "v" (b));
    return result;
}
# endif /* XXH_vec_mulo, XXH_vec_mule */
#endif /* XXH_VECTOR == XXH_VSX */


/* prefetch
 * can be disabled, by declaring XXH_NO_PREFETCH build macro */
#if defined(XXH_NO_PREFETCH)
#  define XXH_PREFETCH(ptr)  (void)(ptr)  /* disabled */
#else
#  if defined(_MSC_VER) && (defined(_M_X64) || defined(_M_IX86))  /* _mm_prefetch() not defined outside of x86/x64 */
#    include <mmintrin.h>   /* https://msdn.microsoft.com/fr-fr/library/84szxsww(v=vs.90).aspx */
#    define XXH_PREFETCH(ptr)  _mm_prefetch((const char*)(ptr), _MM_HINT_T0)
#  elif defined(__GNUC__) && ( (__GNUC__ >= 4) || ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) ) )
#    define XXH_PREFETCH(ptr)  __builtin_prefetch((ptr), 0 /* rw==read */, 3 /* locality */)
#  else
#    define XXH_PREFETCH(ptr) (void)(ptr)  /* disabled */
#  endif
#endif  /* XXH_NO_PREFETCH */


static XXH128_hash_t
XXH_mult64to128(xxh_u64 lhs, xxh_u64 rhs)
{
#if defined(__GNUC__) && !defined(__wasm__) \
    && defined(__SIZEOF_INT128__) \
    || (defined(_INTEGRAL_MAX_BITS) && _INTEGRAL_MAX_BITS >= 128)

    __uint128_t const product = (__uint128_t)lhs * (__uint128_t)rhs;
    XXH128_hash_t r128;
    r128.low64  = (xxh_u64)(product);
    r128.high64 = (xxh_u64)(product >> 64);
    return r128;

    /*
     * MSVC for x64's _umul128 method.
     *
     * xxh_u64 _umul128(xxh_u64 Multiplier, xxh_u64 Multiplicand, xxh_u64 *HighProduct);
     *
     * This compiles to single operand MUL on x64.
     */
#elif defined(_M_X64) || defined(_M_IA64)

#ifndef _MSC_VER
#   pragma intrinsic(_umul128)
#endif
    xxh_u64 product_high;
    xxh_u64 const product_low = _umul128(lhs, rhs, &product_high);
    XXH128_hash_t r128;
    r128.low64  = product_low;
    r128.high64 = product_high;
    return r128;

#else
    /*
     * Portable scalar method. Optimized for 32-bit and 64-bit ALUs.
     */

    /* First calculate all of the cross products. */
    xxh_u64 const lo_lo = XXH_mult32to64(lhs & 0xFFFFFFFF, rhs & 0xFFFFFFFF);
    xxh_u64 const hi_lo = XXH_mult32to64(lhs >> 32,        rhs & 0xFFFFFFFF);
    xxh_u64 const lo_hi = XXH_mult32to64(lhs & 0xFFFFFFFF, rhs >> 32);
    xxh_u64 const hi_hi = XXH_mult32to64(lhs >> 32,        rhs >> 32);

    /* Now add the products together. These will never overflow. */
    xxh_u64 const cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    xxh_u64 const upper = (hi_lo >> 32) + (cross >> 32)        + hi_hi;
    xxh_u64 const lower = (cross << 32) | (lo_lo & 0xFFFFFFFF);

    XXH128_hash_t r128;
    r128.low64  = lower;
    r128.high64 = upper;
    return r128;
#endif
}


static xxh_u64
XXH3_mul128_fold64(xxh_u64 lhs, xxh_u64 rhs)
{
    XXH128_hash_t product = XXH_mult64to128(lhs, rhs);
    return product.low64 ^ product.high64;
}

/*! Seems to produce slightly better code on GCC for some reason. */
static inline xxh_u64 XXH_xorshift64(xxh_u64 v64, int shift)
{
    return v64 ^ (v64 >> shift);
}

static XXH64_hash_t XXH3_avalanche(xxh_u64 h64)
{
    h64 = XXH_xorshift64(h64, 37);
    h64 *= 0x165667919E3779F9ULL;
    h64 = XXH_xorshift64(h64, 32);
    return h64;
}

static xxh_u64 XXH64_avalanche(xxh_u64 h64)
{
    h64 ^= h64 >> 33;
    h64 *= XXH_PRIME64_2;
    h64 ^= h64 >> 29;
    h64 *= XXH_PRIME64_3;
    h64 ^= h64 >> 32;
    return h64;
}

/*
 * This is a stronger avalanche,
 * inspired by Pelle Evensen's rrmxmx
 * preferable when input has not been previously mixed
 */
static XXH64_hash_t XXH3_rrmxmx(xxh_u64 h64, xxh_u64 len)
{
    /* this mix is inspired by Pelle Evensen's rrmxmx */
    h64 ^= XXH_rotl64(h64, 49) ^ XXH_rotl64(h64, 24);
    h64 *= 0x9FB21C651E98DF25ULL;
    h64 ^= (h64 >> 35) + len ;
    h64 *= 0x9FB21C651E98DF25ULL;
    return XXH_xorshift64(h64, 28);
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_len_1to3_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    /*
     * len = 1: combined = { input[0], 0x01, input[0], input[0] }
     * len = 2: combined = { input[1], 0x02, input[0], input[1] }
     * len = 3: combined = { input[2], 0x03, input[0], input[1] }
     */
    {   xxh_u8  const c1 = input[0];
        xxh_u8  const c2 = input[len >> 1];
        xxh_u8  const c3 = input[len - 1];
        xxh_u32 const combined = ((xxh_u32)c1 << 16) | ((xxh_u32)c2  << 24)
                               | ((xxh_u32)c3 <<  0) | ((xxh_u32)len << 8);
        xxh_u64 const bitflip = (XXH_readLE32(secret) ^ XXH_readLE32(secret+4)) + seed;
        xxh_u64 const keyed = (xxh_u64)combined ^ bitflip;
        return XXH64_avalanche(keyed);
    }
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_len_4to8_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    seed ^= (xxh_u64)XXH_swap32((xxh_u32)seed) << 32;
    {   xxh_u32 const input1 = XXH_readLE32(input);
        xxh_u32 const input2 = XXH_readLE32(input + len - 4);
        xxh_u64 const bitflip = (XXH_readLE64(secret+8) ^ XXH_readLE64(secret+16)) - seed;
        xxh_u64 const input64 = input2 + (((xxh_u64)input1) << 32);
        xxh_u64 const keyed = input64 ^ bitflip;
        return XXH3_rrmxmx(keyed, len);
    }
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_len_9to16_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    {   xxh_u64 const bitflip1 = (XXH_readLE64(secret+24) ^ XXH_readLE64(secret+32)) + seed;
        xxh_u64 const bitflip2 = (XXH_readLE64(secret+40) ^ XXH_readLE64(secret+48)) - seed;
        xxh_u64 const input_lo = XXH_readLE64(input)           ^ bitflip1;
        xxh_u64 const input_hi = XXH_readLE64(input + len - 8) ^ bitflip2;
        xxh_u64 const acc = len
                          + XXH_swap64(input_lo) + input_hi
                          + XXH3_mul128_fold64(input_lo, input_hi);
        return XXH3_avalanche(acc);
    }
}

XXH_FORCE_INLINE XXH64_hash_t
XXH3_len_0to16_64b(const xxh_u8* input, size_t len, const xxh_u8* secret, XXH64_hash_t seed)
{
    {   if (XXH_likely(len >  8)) return XXH3_len_9to16_64b(input, len, secret, seed);
        if (XXH_likely(len >= 4)) return XXH3_len_4to8_64b(input, len, secret, seed);
        if (len) return XXH3_len_1to3_64b(input, len, secret, seed);
        return XXH64_avalanche(seed ^ (XXH_readLE64(secret+56) ^ XXH_readLE64(secret+64)));
    }
}

static inline xxh_u64 XXH3_mix16B(const xxh_u8* XXH_RESTRICT input,
                                     const xxh_u8* XXH_RESTRICT secret, xxh_u64 seed64)
{
	xxh_u64 const input_lo = XXH_readLE64(input);
	xxh_u64 const input_hi = XXH_readLE64(input+8);
	return XXH3_mul128_fold64(
	    input_lo ^ (XXH_readLE64(secret)   + seed64),
	    input_hi ^ (XXH_readLE64(secret+8) - seed64)
	);
}

static inline XXH64_hash_t
XXH3_len_17to128_64b(const xxh_u8* XXH_RESTRICT input, size_t len,
                     const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                     XXH64_hash_t seed)
{

    {   xxh_u64 acc = len * XXH_PRIME64_1;
        if (len > 32) {
            if (len > 64) {
                if (len > 96) {
                    acc += XXH3_mix16B(input+48, secret+96, seed);
                    acc += XXH3_mix16B(input+len-64, secret+112, seed);
                }
                acc += XXH3_mix16B(input+32, secret+64, seed);
                acc += XXH3_mix16B(input+len-48, secret+80, seed);
            }
            acc += XXH3_mix16B(input+16, secret+32, seed);
            acc += XXH3_mix16B(input+len-32, secret+48, seed);
        }
        acc += XXH3_mix16B(input+0, secret+0, seed);
        acc += XXH3_mix16B(input+len-16, secret+16, seed);

        return XXH3_avalanche(acc);
    }
}

#define XXH3_MIDSIZE_MAX 240

static XXH64_hash_t
XXH3_len_129to240_64b(const xxh_u8* XXH_RESTRICT input, size_t len,
                      const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                      XXH64_hash_t seed)
{
	if (len > XXH3_MIDSIZE_MAX) {
		len = XXH3_MIDSIZE_MAX;
	}
    #define XXH3_MIDSIZE_STARTOFFSET 3
    #define XXH3_MIDSIZE_LASTOFFSET  17

    {   xxh_u64 acc = len * XXH_PRIME64_1;
        int const nbRounds = (int)len / 16;
        int i;
        for (i=0; i<8; i++) {
            acc += XXH3_mix16B(input+(16*i), secret+(16*i), seed);
        }
        acc = XXH3_avalanche(acc);
#if defined(__clang__)                                /* Clang */ \
    && (defined(__ARM_NEON) || defined(__ARM_NEON__)) /* NEON */ \
    && !defined(XXH_ENABLE_AUTOVECTORIZE)             /* Define to disable */
        #pragma clang loop vectorize(disable)
#endif
        for (i=8 ; i < nbRounds; i++) {
            acc += XXH3_mix16B(input+(16*i), secret+(16*(i-8)) + XXH3_MIDSIZE_STARTOFFSET, seed);
        }
        /* last bytes */
        acc += XXH3_mix16B(input + len - 16, secret + XXH3_SECRET_SIZE_MIN - XXH3_MIDSIZE_LASTOFFSET, seed);
        return XXH3_avalanche(acc);
    }
}

/* =======     Long Keys     ======= */

#define XXH_STRIPE_LEN 64
#define XXH_SECRET_CONSUME_RATE 8   /* nb of secret bytes consumed at each accumulation */
#define XXH_ACC_NB (XXH_STRIPE_LEN / sizeof(xxh_u64))

#ifdef XXH_OLD_NAMES
#  define STRIPE_LEN XXH_STRIPE_LEN
#  define ACC_NB XXH_ACC_NB
#endif

XXH_FORCE_INLINE void XXH_writeLE64(void* dst, xxh_u64 v64)
{
    if (!XXH_CPU_LITTLE_ENDIAN) v64 = XXH_swap64(v64);
    memcpy(dst, &v64, sizeof(v64));
}

/* Several intrinsic functions below are supposed to accept __int64 as argument,
 * as documented in https://software.intel.com/sites/landingpage/IntrinsicsGuide/ .
 * However, several environments do not define __int64 type,
 * requiring a workaround.
 */
#if !defined (__VMS) \
  && (defined (__cplusplus) \
  || (defined (__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L) /* C99 */) )
    typedef int64_t xxh_i64;
#else
    /* the following type must have a width of 64-bit */
    typedef long long xxh_i64;
#endif

/*
 * XXH3_accumulate_512 is the tightest loop for long inputs, and it is the most optimized.
 *
 * It is a hardened version of UMAC, based off of FARSH's implementation.
 *
 * This was chosen because it adapts quite well to 32-bit, 64-bit, and SIMD
 * implementations, and it is ridiculously fast.
 *
 * We harden it by mixing the original input to the accumulators as well as the product.
 *
 * This means that in the (relatively likely) case of a multiply by zero, the
 * original input is preserved.
 *
 * On 128-bit inputs, we swap 64-bit pairs when we add the input to improve
 * cross-pollination, as otherwise the upper and lower halves would be
 * essentially independent.
 *
 * This doesn't matter on 64-bit hashes since they all get merged together in
 * the end, so we skip the extra step.
 *
 * Both XXH3_64bits and XXH3_128bits use this subroutine.
 */

#if (XXH_VECTOR == XXH_AVX512) \
     || (defined(XXH_DISPATCH_AVX512) && XXH_DISPATCH_AVX512 != 0)

#ifndef XXH_TARGET_AVX512
# define XXH_TARGET_AVX512  /* disable attribute target */
#endif

XXH_FORCE_INLINE XXH_TARGET_AVX512 void
XXH3_accumulate_512_avx512(void* XXH_RESTRICT acc,
                     const void* XXH_RESTRICT input,
                     const void* XXH_RESTRICT secret)
{
    XXH_ALIGN(64) __m512i* const xacc = (__m512i *) acc;
    XXH_STATIC_ASSERT(XXH_STRIPE_LEN == sizeof(__m512i));

    {
        /* data_vec    = input[0]; */
        __m512i const data_vec    = _mm512_loadu_si512   (input);
        /* key_vec     = secret[0]; */
        __m512i const key_vec     = _mm512_loadu_si512   (secret);
        /* data_key    = data_vec ^ key_vec; */
        __m512i const data_key    = _mm512_xor_si512     (data_vec, key_vec);
        /* data_key_lo = data_key >> 32; */
        __m512i const data_key_lo = _mm512_shuffle_epi32 (data_key, (_MM_PERM_ENUM)_MM_SHUFFLE(0, 3, 0, 1));
        /* product     = (data_key & 0xffffffff) * (data_key_lo & 0xffffffff); */
        __m512i const product     = _mm512_mul_epu32     (data_key, data_key_lo);
        /* xacc[0] += swap(data_vec); */
        __m512i const data_swap = _mm512_shuffle_epi32(data_vec, (_MM_PERM_ENUM)_MM_SHUFFLE(1, 0, 3, 2));
        __m512i const sum       = _mm512_add_epi64(*xacc, data_swap);
        /* xacc[0] += product; */
        *xacc = _mm512_add_epi64(product, sum);
    }
}

/*
 * XXH3_scrambleAcc: Scrambles the accumulators to improve mixing.
 *
 * Multiplication isn't perfect, as explained by Google in HighwayHash:
 *
 *  // Multiplication mixes/scrambles bytes 0-7 of the 64-bit result to
 *  // varying degrees. In descending order of goodness, bytes
 *  // 3 4 2 5 1 6 0 7 have quality 228 224 164 160 100 96 36 32.
 *  // As expected, the upper and lower bytes are much worse.
 *
 * Source: https://github.com/google/highwayhash/blob/0aaf66b/highwayhash/hh_avx2.h#L291
 *
 * Since our algorithm uses a pseudorandom secret to add some variance into the
 * mix, we don't need to (or want to) mix as often or as much as HighwayHash does.
 *
 * This isn't as tight as XXH3_accumulate, but still written in SIMD to avoid
 * extraction.
 *
 * Both XXH3_64bits and XXH3_128bits use this subroutine.
 */

XXH_FORCE_INLINE XXH_TARGET_AVX512 void
XXH3_scrambleAcc_avx512(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_STATIC_ASSERT(XXH_STRIPE_LEN == sizeof(__m512i));
    {   XXH_ALIGN(64) __m512i* const xacc = (__m512i*) acc;
        const __m512i prime32 = _mm512_set1_epi32((int)XXH_PRIME32_1);

        /* xacc[0] ^= (xacc[0] >> 47) */
        __m512i const acc_vec     = *xacc;
        __m512i const shifted     = _mm512_srli_epi64    (acc_vec, 47);
        __m512i const data_vec    = _mm512_xor_si512     (acc_vec, shifted);
        /* xacc[0] ^= secret; */
        __m512i const key_vec     = _mm512_loadu_si512   (secret);
        __m512i const data_key    = _mm512_xor_si512     (data_vec, key_vec);

        /* xacc[0] *= XXH_PRIME32_1; */
        __m512i const data_key_hi = _mm512_shuffle_epi32 (data_key, (_MM_PERM_ENUM)_MM_SHUFFLE(0, 3, 0, 1));
        __m512i const prod_lo     = _mm512_mul_epu32     (data_key, prime32);
        __m512i const prod_hi     = _mm512_mul_epu32     (data_key_hi, prime32);
        *xacc = _mm512_add_epi64(prod_lo, _mm512_slli_epi64(prod_hi, 32));
    }
}

XXH_FORCE_INLINE XXH_TARGET_AVX512 void
XXH3_initCustomSecret_avx512(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 63) == 0);
    XXH_STATIC_ASSERT(XXH_SEC_ALIGN == 64);
    (void)(&XXH_writeLE64);
    {   int const nbRounds = XXH_SECRET_DEFAULT_SIZE / sizeof(__m512i);
        __m512i const seed = _mm512_mask_set1_epi64(_mm512_set1_epi64((xxh_i64)seed64), 0xAA, -(xxh_i64)seed64);

        XXH_ALIGN(64) const __m512i* const src  = (const __m512i*) XXH3_kSecret;
        XXH_ALIGN(64)       __m512i* const dest = (      __m512i*) customSecret;
        int i;
        for (i=0; i < nbRounds; ++i) {
            /* GCC has a bug, _mm512_stream_load_si512 accepts 'void*', not 'void const*',
             * this will warn "discards ‘const’ qualifier". */
            union {
                XXH_ALIGN(64) const __m512i* cp;
                XXH_ALIGN(64) void* p;
            } remote_const_void;
            remote_const_void.cp = src + i;
            dest[i] = _mm512_add_epi64(_mm512_stream_load_si512(remote_const_void.p), seed);
    }   }
}

#endif

#if (XXH_VECTOR == XXH_AVX2) \
    || (defined(XXH_DISPATCH_AVX2) && XXH_DISPATCH_AVX2 != 0)

#ifndef XXH_TARGET_AVX2
# define XXH_TARGET_AVX2  /* disable attribute target */
#endif

XXH_FORCE_INLINE XXH_TARGET_AVX2 void
XXH3_accumulate_512_avx2( void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{
    {   XXH_ALIGN(32) __m256i* const xacc    =       (__m256i *) acc;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm256_loadu_si256 requires  a const __m256i * pointer for some reason. */
        const         __m256i* const xinput  = (const __m256i *) input;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm256_loadu_si256 requires a const __m256i * pointer for some reason. */
        const         __m256i* const xsecret = (const __m256i *) secret;

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m256i); i++) {
            /* data_vec    = xinput[i]; */
            __m256i const data_vec    = _mm256_loadu_si256    (xinput+i);
            /* key_vec     = xsecret[i]; */
            __m256i const key_vec     = _mm256_loadu_si256   (xsecret+i);
            /* data_key    = data_vec ^ key_vec; */
            __m256i const data_key    = _mm256_xor_si256     (data_vec, key_vec);
            /* data_key_lo = data_key >> 32; */
            __m256i const data_key_lo = _mm256_shuffle_epi32 (data_key, _MM_SHUFFLE(0, 3, 0, 1));
            /* product     = (data_key & 0xffffffff) * (data_key_lo & 0xffffffff); */
            __m256i const product     = _mm256_mul_epu32     (data_key, data_key_lo);
            /* xacc[i] += swap(data_vec); */
            __m256i const data_swap = _mm256_shuffle_epi32(data_vec, _MM_SHUFFLE(1, 0, 3, 2));
            __m256i const sum       = _mm256_add_epi64(xacc[i], data_swap);
            /* xacc[i] += product; */
            xacc[i] = _mm256_add_epi64(product, sum);
    }   }
}

XXH_FORCE_INLINE XXH_TARGET_AVX2 void
XXH3_scrambleAcc_avx2(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    {   XXH_ALIGN(32) __m256i* const xacc = (__m256i*) acc;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm256_loadu_si256 requires a const __m256i * pointer for some reason. */
        const         __m256i* const xsecret = (const __m256i *) secret;
        const __m256i prime32 = _mm256_set1_epi32((int)XXH_PRIME32_1);

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m256i); i++) {
            /* xacc[i] ^= (xacc[i] >> 47) */
            __m256i const acc_vec     = xacc[i];
            __m256i const shifted     = _mm256_srli_epi64    (acc_vec, 47);
            __m256i const data_vec    = _mm256_xor_si256     (acc_vec, shifted);
            /* xacc[i] ^= xsecret; */
            __m256i const key_vec     = _mm256_loadu_si256   (xsecret+i);
            __m256i const data_key    = _mm256_xor_si256     (data_vec, key_vec);

            /* xacc[i] *= XXH_PRIME32_1; */
            __m256i const data_key_hi = _mm256_shuffle_epi32 (data_key, _MM_SHUFFLE(0, 3, 0, 1));
            __m256i const prod_lo     = _mm256_mul_epu32     (data_key, prime32);
            __m256i const prod_hi     = _mm256_mul_epu32     (data_key_hi, prime32);
            xacc[i] = _mm256_add_epi64(prod_lo, _mm256_slli_epi64(prod_hi, 32));
        }
    }
}

XXH_FORCE_INLINE XXH_TARGET_AVX2 void XXH3_initCustomSecret_avx2(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 31) == 0);
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE / sizeof(__m256i)) == 6);
    XXH_STATIC_ASSERT(XXH_SEC_ALIGN <= 64);
    (void)(&XXH_writeLE64);
    XXH_PREFETCH(customSecret);
    {   __m256i const seed = _mm256_set_epi64x(-(xxh_i64)seed64, (xxh_i64)seed64, -(xxh_i64)seed64, (xxh_i64)seed64);

        XXH_ALIGN(64) const __m256i* const src  = (const __m256i*) XXH3_kSecret;
        XXH_ALIGN(64)       __m256i*       dest = (      __m256i*) customSecret;

#       if defined(__GNUC__) || defined(__clang__)
        /*
         * On GCC & Clang, marking 'dest' as modified will cause the compiler:
         *   - do not extract the secret from sse registers in the internal loop
         *   - use less common registers, and avoid pushing these reg into stack
         */
        XXH_COMPILER_GUARD(dest);
#       endif

        /* GCC -O2 need unroll loop manually */
        dest[0] = _mm256_add_epi64(_mm256_stream_load_si256(src+0), seed);
        dest[1] = _mm256_add_epi64(_mm256_stream_load_si256(src+1), seed);
        dest[2] = _mm256_add_epi64(_mm256_stream_load_si256(src+2), seed);
        dest[3] = _mm256_add_epi64(_mm256_stream_load_si256(src+3), seed);
        dest[4] = _mm256_add_epi64(_mm256_stream_load_si256(src+4), seed);
        dest[5] = _mm256_add_epi64(_mm256_stream_load_si256(src+5), seed);
    }
}

#endif

/* x86dispatch always generates SSE2 */
#if (XXH_VECTOR == XXH_SSE2) || defined(XXH_X86DISPATCH)

#ifndef XXH_TARGET_SSE2
# define XXH_TARGET_SSE2  /* disable attribute target */
#endif

XXH_FORCE_INLINE XXH_TARGET_SSE2 void
XXH3_accumulate_512_sse2( void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{
    /* SSE2 is just a half-scale version of the AVX2 version. */
    {   XXH_ALIGN(16) __m128i* const xacc    =       (__m128i *) acc;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm_loadu_si128 requires a const __m128i * pointer for some reason. */
        const         __m128i* const xinput  = (const __m128i *) input;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm_loadu_si128 requires a const __m128i * pointer for some reason. */
        const         __m128i* const xsecret = (const __m128i *) secret;

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m128i); i++) {
            /* data_vec    = xinput[i]; */
            __m128i const data_vec    = _mm_loadu_si128   (xinput+i);
            /* key_vec     = xsecret[i]; */
            __m128i const key_vec     = _mm_loadu_si128   (xsecret+i);
            /* data_key    = data_vec ^ key_vec; */
            __m128i const data_key    = _mm_xor_si128     (data_vec, key_vec);
            /* data_key_lo = data_key >> 32; */
            __m128i const data_key_lo = _mm_shuffle_epi32 (data_key, _MM_SHUFFLE(0, 3, 0, 1));
            /* product     = (data_key & 0xffffffff) * (data_key_lo & 0xffffffff); */
            __m128i const product     = _mm_mul_epu32     (data_key, data_key_lo);
            /* xacc[i] += swap(data_vec); */
            __m128i const data_swap = _mm_shuffle_epi32(data_vec, _MM_SHUFFLE(1,0,3,2));
            __m128i const sum       = _mm_add_epi64(xacc[i], data_swap);
            /* xacc[i] += product; */
            xacc[i] = _mm_add_epi64(product, sum);
    }   }
}

XXH_FORCE_INLINE XXH_TARGET_SSE2 void
XXH3_scrambleAcc_sse2(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    {   XXH_ALIGN(16) __m128i* const xacc = (__m128i*) acc;
        /* Unaligned. This is mainly for pointer arithmetic, and because
         * _mm_loadu_si128 requires a const __m128i * pointer for some reason. */
        const         __m128i* const xsecret = (const __m128i *) secret;
        const __m128i prime32 = _mm_set1_epi32((int)XXH_PRIME32_1);

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(__m128i); i++) {
            /* xacc[i] ^= (xacc[i] >> 47) */
            __m128i const acc_vec     = xacc[i];
            __m128i const shifted     = _mm_srli_epi64    (acc_vec, 47);
            __m128i const data_vec    = _mm_xor_si128     (acc_vec, shifted);
            /* xacc[i] ^= xsecret[i]; */
            __m128i const key_vec     = _mm_loadu_si128   (xsecret+i);
            __m128i const data_key    = _mm_xor_si128     (data_vec, key_vec);

            /* xacc[i] *= XXH_PRIME32_1; */
            __m128i const data_key_hi = _mm_shuffle_epi32 (data_key, _MM_SHUFFLE(0, 3, 0, 1));
            __m128i const prod_lo     = _mm_mul_epu32     (data_key, prime32);
            __m128i const prod_hi     = _mm_mul_epu32     (data_key_hi, prime32);
            xacc[i] = _mm_add_epi64(prod_lo, _mm_slli_epi64(prod_hi, 32));
        }
    }
}

XXH_FORCE_INLINE XXH_TARGET_SSE2 void XXH3_initCustomSecret_sse2(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 15) == 0);
    (void)(&XXH_writeLE64);
    {   int const nbRounds = XXH_SECRET_DEFAULT_SIZE / sizeof(__m128i);

#       if defined(_MSC_VER) && defined(_M_IX86) && _MSC_VER < 1900
        // MSVC 32bit mode does not support _mm_set_epi64x before 2015
        XXH_ALIGN(16) const xxh_i64 seed64x2[2] = { (xxh_i64)seed64, -(xxh_i64)seed64 };
        __m128i const seed = _mm_load_si128((__m128i const*)seed64x2);
#       else
        __m128i const seed = _mm_set_epi64x(-(xxh_i64)seed64, (xxh_i64)seed64);
#       endif
        int i;

        XXH_ALIGN(64)        const float* const src  = (float const*) XXH3_kSecret;
        XXH_ALIGN(XXH_SEC_ALIGN) __m128i*       dest = (__m128i*) customSecret;
#       if defined(__GNUC__) || defined(__clang__)
        /*
         * On GCC & Clang, marking 'dest' as modified will cause the compiler:
         *   - do not extract the secret from sse registers in the internal loop
         *   - use less common registers, and avoid pushing these reg into stack
         */
        XXH_COMPILER_GUARD(dest);
#       endif

        for (i=0; i < nbRounds; ++i) {
            dest[i] = _mm_add_epi64(_mm_castps_si128(_mm_load_ps(src+i*4)), seed);
    }   }
}

#endif

#if (XXH_VECTOR == XXH_NEON)

XXH_FORCE_INLINE void
XXH3_accumulate_512_neon( void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{
    {
        XXH_ALIGN(16) uint64x2_t* const xacc = (uint64x2_t *) acc;
        /* We don't use a uint32x4_t pointer because it causes bus errors on ARMv7. */
        uint8_t const* const xinput = (const uint8_t *) input;
        uint8_t const* const xsecret  = (const uint8_t *) secret;

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN / sizeof(uint64x2_t); i++) {
            /* data_vec = xinput[i]; */
            uint8x16_t data_vec    = vld1q_u8(xinput  + (i * 16));
            /* key_vec  = xsecret[i];  */
            uint8x16_t key_vec     = vld1q_u8(xsecret + (i * 16));
            uint64x2_t data_key;
            uint32x2_t data_key_lo, data_key_hi;
            /* xacc[i] += swap(data_vec); */
            uint64x2_t const data64  = vreinterpretq_u64_u8(data_vec);
            uint64x2_t const swapped = vextq_u64(data64, data64, 1);
            xacc[i] = vaddq_u64 (xacc[i], swapped);
            /* data_key = data_vec ^ key_vec; */
            data_key = vreinterpretq_u64_u8(veorq_u8(data_vec, key_vec));
            /* data_key_lo = (uint32x2_t) (data_key & 0xFFFFFFFF);
             * data_key_hi = (uint32x2_t) (data_key >> 32);
             * data_key = UNDEFINED; */
            XXH_SPLIT_IN_PLACE(data_key, data_key_lo, data_key_hi);
            /* xacc[i] += (uint64x2_t) data_key_lo * (uint64x2_t) data_key_hi; */
            xacc[i] = vmlal_u32 (xacc[i], data_key_lo, data_key_hi);

        }
    }
}

XXH_FORCE_INLINE void
XXH3_scrambleAcc_neon(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{

    {   uint64x2_t* xacc       = (uint64x2_t*) acc;
        uint8_t const* xsecret = (uint8_t const*) secret;
        uint32x2_t prime       = vdup_n_u32 (XXH_PRIME32_1);

        size_t i;
        for (i=0; i < XXH_STRIPE_LEN/sizeof(uint64x2_t); i++) {
            /* xacc[i] ^= (xacc[i] >> 47); */
            uint64x2_t acc_vec  = xacc[i];
            uint64x2_t shifted  = vshrq_n_u64 (acc_vec, 47);
            uint64x2_t data_vec = veorq_u64   (acc_vec, shifted);

            /* xacc[i] ^= xsecret[i]; */
            uint8x16_t key_vec  = vld1q_u8(xsecret + (i * 16));
            uint64x2_t data_key = veorq_u64(data_vec, vreinterpretq_u64_u8(key_vec));

            /* xacc[i] *= XXH_PRIME32_1 */
            uint32x2_t data_key_lo, data_key_hi;
            /* data_key_lo = (uint32x2_t) (xacc[i] & 0xFFFFFFFF);
             * data_key_hi = (uint32x2_t) (xacc[i] >> 32);
             * xacc[i] = UNDEFINED; */
            XXH_SPLIT_IN_PLACE(data_key, data_key_lo, data_key_hi);
            {   /*
                 * prod_hi = (data_key >> 32) * XXH_PRIME32_1;
                 *
                 * Avoid vmul_u32 + vshll_n_u32 since Clang 6 and 7 will
                 * incorrectly "optimize" this:
                 *   tmp     = vmul_u32(vmovn_u64(a), vmovn_u64(b));
                 *   shifted = vshll_n_u32(tmp, 32);
                 * to this:
                 *   tmp     = "vmulq_u64"(a, b); // no such thing!
                 *   shifted = vshlq_n_u64(tmp, 32);
                 *
                 * However, unlike SSE, Clang lacks a 64-bit multiply routine
                 * for NEON, and it scalarizes two 64-bit multiplies instead.
                 *
                 * vmull_u32 has the same timing as vmul_u32, and it avoids
                 * this bug completely.
                 * See https://bugs.llvm.org/show_bug.cgi?id=39967
                 */
                uint64x2_t prod_hi = vmull_u32 (data_key_hi, prime);
                /* xacc[i] = prod_hi << 32; */
                xacc[i] = vshlq_n_u64(prod_hi, 32);
                /* xacc[i] += (prod_hi & 0xFFFFFFFF) * XXH_PRIME32_1; */
                xacc[i] = vmlal_u32(xacc[i], data_key_lo, prime);
            }
    }   }
}

#endif

#if (XXH_VECTOR == XXH_VSX)

XXH_FORCE_INLINE void
XXH3_accumulate_512_vsx(  void* XXH_RESTRICT acc,
                    const void* XXH_RESTRICT input,
                    const void* XXH_RESTRICT secret)
{
          xxh_u64x2* const xacc     =       (xxh_u64x2*) acc;    /* presumed aligned */
    xxh_u64x2 const* const xinput   = (xxh_u64x2 const*) input;   /* no alignment restriction */
    xxh_u64x2 const* const xsecret  = (xxh_u64x2 const*) secret;    /* no alignment restriction */
    xxh_u64x2 const v32 = { 32, 32 };
    size_t i;
    for (i = 0; i < XXH_STRIPE_LEN / sizeof(xxh_u64x2); i++) {
        /* data_vec = xinput[i]; */
        xxh_u64x2 const data_vec = XXH_vec_loadu(xinput + i);
        /* key_vec = xsecret[i]; */
        xxh_u64x2 const key_vec  = XXH_vec_loadu(xsecret + i);
        xxh_u64x2 const data_key = data_vec ^ key_vec;
        /* shuffled = (data_key << 32) | (data_key >> 32); */
        xxh_u32x4 const shuffled = (xxh_u32x4)vec_rl(data_key, v32);
        /* product = ((xxh_u64x2)data_key & 0xFFFFFFFF) * ((xxh_u64x2)shuffled & 0xFFFFFFFF); */
        xxh_u64x2 const product  = XXH_vec_mulo((xxh_u32x4)data_key, shuffled);
        xacc[i] += product;

        /* swap high and low halves */
#ifdef __s390x__
        xacc[i] += vec_permi(data_vec, data_vec, 2);
#else
        xacc[i] += vec_xxpermdi(data_vec, data_vec, 2);
#endif
    }
}

XXH_FORCE_INLINE void
XXH3_scrambleAcc_vsx(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{

    {         xxh_u64x2* const xacc    =       (xxh_u64x2*) acc;
        const xxh_u64x2* const xsecret = (const xxh_u64x2*) secret;
        /* constants */
        xxh_u64x2 const v32  = { 32, 32 };
        xxh_u64x2 const v47 = { 47, 47 };
        xxh_u32x4 const prime = { XXH_PRIME32_1, XXH_PRIME32_1, XXH_PRIME32_1, XXH_PRIME32_1 };
        size_t i;
        for (i = 0; i < XXH_STRIPE_LEN / sizeof(xxh_u64x2); i++) {
            /* xacc[i] ^= (xacc[i] >> 47); */
            xxh_u64x2 const acc_vec  = xacc[i];
            xxh_u64x2 const data_vec = acc_vec ^ (acc_vec >> v47);

            /* xacc[i] ^= xsecret[i]; */
            xxh_u64x2 const key_vec  = XXH_vec_loadu(xsecret + i);
            xxh_u64x2 const data_key = data_vec ^ key_vec;

            /* xacc[i] *= XXH_PRIME32_1 */
            /* prod_lo = ((xxh_u64x2)data_key & 0xFFFFFFFF) * ((xxh_u64x2)prime & 0xFFFFFFFF);  */
            xxh_u64x2 const prod_even  = XXH_vec_mule((xxh_u32x4)data_key, prime);
            /* prod_hi = ((xxh_u64x2)data_key >> 32) * ((xxh_u64x2)prime >> 32);  */
            xxh_u64x2 const prod_odd  = XXH_vec_mulo((xxh_u32x4)data_key, prime);
            xacc[i] = prod_odd + (prod_even << v32);
    }   }
}

#endif

/* scalar variants - universal */

XXH_FORCE_INLINE void
XXH3_accumulate_512_scalar(void* XXH_RESTRICT acc,
                     const void* XXH_RESTRICT input,
                     const void* XXH_RESTRICT secret)
{
    XXH_ALIGN(XXH_ACC_ALIGN) xxh_u64* const xacc = (xxh_u64*) acc; /* presumed aligned */
    const xxh_u8* const xinput  = (const xxh_u8*) input;  /* no alignment restriction */
    const xxh_u8* const xsecret = (const xxh_u8*) secret;   /* no alignment restriction */
    size_t i;
    for (i=0; i < XXH_ACC_NB; i++) {
        xxh_u64 const data_val = XXH_readLE64(xinput + 8*i);
        xxh_u64 const data_key = data_val ^ XXH_readLE64(xsecret + i*8);
        xacc[i ^ 1] += data_val; /* swap adjacent lanes */
        xacc[i] += XXH_mult32to64(data_key & 0xFFFFFFFF, data_key >> 32);
    }
}

XXH_FORCE_INLINE void
XXH3_scrambleAcc_scalar(void* XXH_RESTRICT acc, const void* XXH_RESTRICT secret)
{
    XXH_ALIGN(XXH_ACC_ALIGN) xxh_u64* const xacc = (xxh_u64*) acc;   /* presumed aligned */
    const xxh_u8* const xsecret = (const xxh_u8*) secret;   /* no alignment restriction */
    size_t i;
    for (i=0; i < XXH_ACC_NB; i++) {
        xxh_u64 const key64 = XXH_readLE64(xsecret + 8*i);
        xxh_u64 acc64 = xacc[i];
        acc64 = XXH_xorshift64(acc64, 47);
        acc64 ^= key64;
        acc64 *= XXH_PRIME32_1;
        xacc[i] = acc64;
    }
}

XXH_FORCE_INLINE void
XXH3_initCustomSecret_scalar(void* XXH_RESTRICT customSecret, xxh_u64 seed64)
{
    /*
     * We need a separate pointer for the hack below,
     * which requires a non-const pointer.
     * Any decent compiler will optimize this out otherwise.
     */
    const xxh_u8* kSecretPtr = XXH3_kSecret;
    XXH_STATIC_ASSERT((XXH_SECRET_DEFAULT_SIZE & 15) == 0);

#if defined(__clang__) && defined(__aarch64__)
    /*
     * UGLY HACK:
     * Clang generates a bunch of MOV/MOVK pairs for aarch64, and they are
     * placed sequentially, in order, at the top of the unrolled loop.
     *
     * While MOVK is great for generating constants (2 cycles for a 64-bit
     * constant compared to 4 cycles for LDR), long MOVK chains stall the
     * integer pipelines:
     *   I   L   S
     * MOVK
     * MOVK
     * MOVK
     * MOVK
     * ADD
     * SUB      STR
     *          STR
     * By forcing loads from memory (as the asm line causes Clang to assume
     * that XXH3_kSecretPtr has been changed), the pipelines are used more
     * efficiently:
     *   I   L   S
     *      LDR
     *  ADD LDR
     *  SUB     STR
     *          STR
     * XXH3_64bits_withSeed, len == 256, Snapdragon 835
     *   without hack: 2654.4 MB/s
     *   with hack:    3202.9 MB/s
     */
    XXH_COMPILER_GUARD(kSecretPtr);
#endif
    /*
     * Note: in debug mode, this overrides the asm optimization
     * and Clang will emit MOVK chains again.
     */

    {   int const nbRounds = XXH_SECRET_DEFAULT_SIZE / 16;
        int i;
        for (i=0; i < nbRounds; i++) {
            /*
             * The asm hack causes Clang to assume that kSecretPtr aliases with
             * customSecret, and on aarch64, this prevented LDP from merging two
             * loads together for free. Putting the loads together before the stores
             * properly generates LDP.
             */
            xxh_u64 lo = XXH_readLE64(kSecretPtr + 16*i)     + seed64;
            xxh_u64 hi = XXH_readLE64(kSecretPtr + 16*i + 8) - seed64;
            XXH_writeLE64((xxh_u8*)customSecret + 16*i,     lo);
            XXH_writeLE64((xxh_u8*)customSecret + 16*i + 8, hi);
    }   }
}


typedef void (*XXH3_f_accumulate_512)(void* XXH_RESTRICT, const void*, const void*);
typedef void (*XXH3_f_scrambleAcc)(void* XXH_RESTRICT, const void*);
typedef void (*XXH3_f_initCustomSecret)(void* XXH_RESTRICT, xxh_u64);


#if (XXH_VECTOR == XXH_AVX512)

#define XXH3_accumulate_512 XXH3_accumulate_512_avx512
#define XXH3_scrambleAcc    XXH3_scrambleAcc_avx512
#define XXH3_initCustomSecret XXH3_initCustomSecret_avx512

#elif (XXH_VECTOR == XXH_AVX2)

#define XXH3_accumulate_512 XXH3_accumulate_512_avx2
#define XXH3_scrambleAcc    XXH3_scrambleAcc_avx2
#define XXH3_initCustomSecret XXH3_initCustomSecret_avx2

#elif (XXH_VECTOR == XXH_SSE2)

#define XXH3_accumulate_512 XXH3_accumulate_512_sse2
#define XXH3_scrambleAcc    XXH3_scrambleAcc_sse2
#define XXH3_initCustomSecret XXH3_initCustomSecret_sse2

#elif (XXH_VECTOR == XXH_NEON)

#define XXH3_accumulate_512 XXH3_accumulate_512_neon
#define XXH3_scrambleAcc    XXH3_scrambleAcc_neon
#define XXH3_initCustomSecret XXH3_initCustomSecret_scalar

#elif (XXH_VECTOR == XXH_VSX)

#define XXH3_accumulate_512 XXH3_accumulate_512_vsx
#define XXH3_scrambleAcc    XXH3_scrambleAcc_vsx
#define XXH3_initCustomSecret XXH3_initCustomSecret_scalar

#else /* scalar */

#define XXH3_accumulate_512 XXH3_accumulate_512_scalar
#define XXH3_scrambleAcc    XXH3_scrambleAcc_scalar
#define XXH3_initCustomSecret XXH3_initCustomSecret_scalar

#endif



#ifndef XXH_PREFETCH_DIST
#  ifdef __clang__
#    define XXH_PREFETCH_DIST 320
#  else
#    if (XXH_VECTOR == XXH_AVX512)
#      define XXH_PREFETCH_DIST 512
#    else
#      define XXH_PREFETCH_DIST 384
#    endif
#  endif  /* __clang__ */
#endif  /* XXH_PREFETCH_DIST */

/*
 * XXH3_accumulate()
 * Loops over XXH3_accumulate_512().
 * Assumption: nbStripes will not overflow the secret size
 */
XXH_FORCE_INLINE void
XXH3_accumulate(     xxh_u64* XXH_RESTRICT acc,
                const xxh_u8* XXH_RESTRICT input,
                const xxh_u8* XXH_RESTRICT secret,
                      size_t nbStripes,
                      XXH3_f_accumulate_512 f_acc512)
{
    size_t n;
    for (n = 0; n < nbStripes; n++ ) {
        const xxh_u8* const in = input + n*XXH_STRIPE_LEN;
        XXH_PREFETCH(in + XXH_PREFETCH_DIST);
        f_acc512(acc,
                 in,
                 secret + n*XXH_SECRET_CONSUME_RATE);
    }
}

XXH_FORCE_INLINE void
XXH3_hashLong_internal_loop(xxh_u64* XXH_RESTRICT acc,
                      const xxh_u8* XXH_RESTRICT input, size_t len,
                      const xxh_u8* XXH_RESTRICT secret, size_t secretSize,
                            XXH3_f_accumulate_512 f_acc512,
                            XXH3_f_scrambleAcc f_scramble)
{
    size_t const nbStripesPerBlock = (secretSize - XXH_STRIPE_LEN) / XXH_SECRET_CONSUME_RATE;
    size_t const block_len = XXH_STRIPE_LEN * nbStripesPerBlock;
    size_t const nb_blocks = (len - 1) / block_len;

    size_t n;


    for (n = 0; n < nb_blocks; n++) {
        XXH3_accumulate(acc, input + n*block_len, secret, nbStripesPerBlock, f_acc512);
        f_scramble(acc, secret + secretSize - XXH_STRIPE_LEN);
    }

    /* last partial block */
    {   size_t const nbStripes = ((len - 1) - (block_len * nb_blocks)) / XXH_STRIPE_LEN;
        XXH3_accumulate(acc, input + nb_blocks*block_len, secret, nbStripes, f_acc512);

        /* last stripe */
        {   const xxh_u8* const p = input + len - XXH_STRIPE_LEN;
#define XXH_SECRET_LASTACC_START 7  /* not aligned on 8, last secret is different from acc & scrambler */
            f_acc512(acc, p, secret + secretSize - XXH_STRIPE_LEN - XXH_SECRET_LASTACC_START);
    }   }
}

XXH_FORCE_INLINE xxh_u64
XXH3_mix2Accs(const xxh_u64* XXH_RESTRICT acc, const xxh_u8* XXH_RESTRICT secret)
{
    return XXH3_mul128_fold64(
               acc[0] ^ XXH_readLE64(secret),
               acc[1] ^ XXH_readLE64(secret+8) );
}

static XXH64_hash_t
XXH3_mergeAccs(const xxh_u64* XXH_RESTRICT acc, const xxh_u8* XXH_RESTRICT secret, xxh_u64 start)
{
    xxh_u64 result64 = start;
    size_t i = 0;

    for (i = 0; i < 4; i++) {
        result64 += XXH3_mix2Accs(acc+2*i, secret + 16*i);
#if defined(__clang__)                                /* Clang */ \
    && (defined(__arm__) || defined(__thumb__))       /* ARMv7 */ \
    && (defined(__ARM_NEON) || defined(__ARM_NEON__)) /* NEON */  \
    && !defined(XXH_ENABLE_AUTOVECTORIZE)             /* Define to disable */
        /*
         * UGLY HACK:
         * Prevent autovectorization on Clang ARMv7-a. Exact same problem as
         * the one in XXH3_len_129to240_64b. Speeds up shorter keys > 240b.
         * XXH3_64bits, len == 256, Snapdragon 835:
         *   without hack: 2063.7 MB/s
         *   with hack:    2560.7 MB/s
         */
        XXH_COMPILER_GUARD(result64);
#endif
    }

    return XXH3_avalanche(result64);
}

#define XXH3_INIT_ACC { XXH_PRIME32_3, XXH_PRIME64_1, XXH_PRIME64_2, XXH_PRIME64_3, \
                        XXH_PRIME64_4, XXH_PRIME32_2, XXH_PRIME64_5, XXH_PRIME32_1 }

XXH_FORCE_INLINE XXH64_hash_t
XXH3_hashLong_64b_internal(const void* XXH_RESTRICT input, size_t len,
                           const void* XXH_RESTRICT secret, size_t secretSize,
                           XXH3_f_accumulate_512 f_acc512,
                           XXH3_f_scrambleAcc f_scramble)
{
    XXH_ALIGN(XXH_ACC_ALIGN) xxh_u64 acc[XXH_ACC_NB] = XXH3_INIT_ACC;

    XXH3_hashLong_internal_loop(acc, (const xxh_u8*)input, len, (const xxh_u8*)secret, secretSize, f_acc512, f_scramble);

    /* converge into final hash */
    XXH_STATIC_ASSERT(sizeof(acc) == 64);
    /* do not align on 8, so that the secret is different from the accumulator */
#define XXH_SECRET_MERGEACCS_START 11
    return XXH3_mergeAccs(acc, (const xxh_u8*)secret + XXH_SECRET_MERGEACCS_START, (xxh_u64)len * XXH_PRIME64_1);
}

/*
 * It's important for performance that XXH3_hashLong is not inlined.
 */
XXH_NO_INLINE XXH64_hash_t
XXH3_hashLong_64b_withSecret(const void* XXH_RESTRICT input, size_t len,
                             XXH64_hash_t seed64, const xxh_u8* XXH_RESTRICT secret, size_t secretLen)
{
    (void)seed64;
    return XXH3_hashLong_64b_internal(input, len, secret, secretLen, XXH3_accumulate_512, XXH3_scrambleAcc);
}

/*
 * It's important for performance that XXH3_hashLong is not inlined.
 * Since the function is not inlined, the compiler may not be able to understand that,
 * in some scenarios, its `secret` argument is actually a compile time constant.
 * This variant enforces that the compiler can detect that,
 * and uses this opportunity to streamline the generated code for better performance.
 */
XXH_NO_INLINE XXH64_hash_t
XXH3_hashLong_64b_default(const void* XXH_RESTRICT input, size_t len,
                          XXH64_hash_t seed64, const xxh_u8* XXH_RESTRICT secret, size_t secretLen)
{
    (void)seed64; (void)secret; (void)secretLen;
    return XXH3_hashLong_64b_internal(input, len, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_accumulate_512, XXH3_scrambleAcc);
}

/*
 * XXH3_hashLong_64b_withSeed():
 * Generate a custom key based on alteration of default XXH3_kSecret with the seed,
 * and then use this key for long mode hashing.
 *
 * This operation is decently fast but nonetheless costs a little bit of time.
 * Try to avoid it whenever possible (typically when seed==0).
 *
 * It's important for performance that XXH3_hashLong is not inlined. Not sure
 * why (uop cache maybe?), but the difference is large and easily measurable.
 */
XXH_FORCE_INLINE XXH64_hash_t
XXH3_hashLong_64b_withSeed_internal(const void* input, size_t len,
                                    XXH64_hash_t seed,
                                    XXH3_f_accumulate_512 f_acc512,
                                    XXH3_f_scrambleAcc f_scramble,
                                    XXH3_f_initCustomSecret f_initSec)
{
    if (seed == 0)
        return XXH3_hashLong_64b_internal(input, len,
                                          XXH3_kSecret, sizeof(XXH3_kSecret),
                                          f_acc512, f_scramble);
    {   XXH_ALIGN(XXH_SEC_ALIGN) xxh_u8 secret[XXH_SECRET_DEFAULT_SIZE];
        f_initSec(secret, seed);
        return XXH3_hashLong_64b_internal(input, len, secret, sizeof(secret),
                                          f_acc512, f_scramble);
    }
}

/*
 * It's important for performance that XXH3_hashLong is not inlined.
 */
XXH_NO_INLINE XXH64_hash_t
XXH3_hashLong_64b_withSeed(const void* input, size_t len,
                           XXH64_hash_t seed, const xxh_u8* secret, size_t secretLen)
{
    (void)secret; (void)secretLen;
    return XXH3_hashLong_64b_withSeed_internal(input, len, seed,
                XXH3_accumulate_512, XXH3_scrambleAcc, XXH3_initCustomSecret);
}


typedef XXH64_hash_t (*XXH3_hashLong64_f)(const void* XXH_RESTRICT, size_t,
                                          XXH64_hash_t, const xxh_u8* XXH_RESTRICT, size_t);

XXH_FORCE_INLINE XXH64_hash_t
XXH3_64bits_internal(const void* XXH_RESTRICT input, size_t len,
                     XXH64_hash_t seed64, const void* XXH_RESTRICT secret, size_t secretLen,
                     XXH3_hashLong64_f f_hashLong)
{
    /*
     * If an action is to be taken if `secretLen` condition is not respected,
     * it should be done here.
     * For now, it's a contract pre-condition.
     * Adding a check and a branch here would cost performance at every hash.
     * Also, note that function signature doesn't offer room to return an error.
     */
    if (len <= 16)
        return XXH3_len_0to16_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, seed64);
    if (len <= 128)
        return XXH3_len_17to128_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    if (len <= XXH3_MIDSIZE_MAX)
        return XXH3_len_129to240_64b((const xxh_u8*)input, len, (const xxh_u8*)secret, secretLen, seed64);
    return f_hashLong(input, len, seed64, (const xxh_u8*)secret, secretLen);
}


/* ===   Public entry point   === */

/*! @ingroup xxh3_family */
static inline XXH64_hash_t XXH3_64bits(const void* input, size_t len)
{
    return XXH3_64bits_internal(input, len, 0, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_hashLong_64b_default);
}

/*! @ingroup xxh3_family */
static inline XXH64_hash_t
XXH3_64bits_withSeed(const void* input, size_t len, XXH64_hash_t seed)
{
    return XXH3_64bits_internal(input, len, seed, XXH3_kSecret, sizeof(XXH3_kSecret), XXH3_hashLong_64b_withSeed);
}

}
