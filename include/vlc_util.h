/*****************************************************************************
 * vlc_util.h: utility functions/macros
 * Collection of useful common functions and macros definitions
 *****************************************************************************
 * Copyright (C) 1998-2011 VLC authors and VideoLAN
 *
 * Authors: Samuel Hocevar <sam@via.ecp.fr>
 *          Vincent Seguin <seguin@via.ecp.fr>
 *          Gildas Bazin <gbazin@videolan.org>
 *          Rémi Denis-Courmont
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifndef VLC_COMMON_UTILS_H
# define VLC_COMMON_UTILS_H 1

/**
 * \file
 * This file defines common useful utility functions and macros
 */

/** Greatest common divisor */
VLC_USED
static inline int64_t GCD ( int64_t a, int64_t b )
{
    while( b )
    {
        int64_t c = a % b;
        a = b;
        b = c;
    }
    return a;
}

/* function imported from libavutil/common.h */
VLC_USED
static inline uint8_t clip_uint8_vlc( int32_t a )
{
    if( a&(~255) ) return (-a)>>31;
    else           return a;
}

/**
 * \defgroup bitops Bit operations
 * @{
 */

#define VLC_INT_FUNC(basename) \
        VLC_INT_FUNC_TYPE(basename, unsigned, ) \
        VLC_INT_FUNC_TYPE(basename, unsigned long, l) \
        VLC_INT_FUNC_TYPE(basename, unsigned long long, ll)

#if defined (__GNUC__) || defined (__clang__)
# define VLC_INT_FUNC_TYPE(basename,type,suffix) \
VLC_USED static inline int vlc_##basename##suffix(type x) \
{ \
    return __builtin_##basename##suffix(x); \
}

VLC_INT_FUNC(clz)
#else
VLC_USED static inline int vlc_clzll(unsigned long long x)
{
    int i = sizeof (x) * 8;

    while (x)
    {
        x >>= 1;
        i--;
    }
    return i;
}

VLC_USED static inline int vlc_clzl(unsigned long x)
{
    return vlc_clzll(x) - ((sizeof (long long) - sizeof (long)) * 8);
}

VLC_USED static inline int vlc_clz(unsigned x)
{
    return vlc_clzll(x) - ((sizeof (long long) - sizeof (int)) * 8);
}

VLC_USED static inline int vlc_ctz_generic(unsigned long long x)
{
    unsigned i = sizeof (x) * 8;

    while (x)
    {
        x <<= 1;
        i--;
    }
    return i;
}

VLC_USED static inline int vlc_parity_generic(unsigned long long x)
{
    for (unsigned i = 4 * sizeof (x); i > 0; i /= 2)
        x ^= x >> i;
    return x & 1;
}

VLC_USED static inline int vlc_popcount_generic(unsigned long long x)
{
    int count = 0;
    while (x)
    {
        count += x & 1;
        x = x >> 1;
    }
    return count;
}

# define VLC_INT_FUNC_TYPE(basename,type,suffix) \
VLC_USED static inline int vlc_##basename##suffix(type x) \
{ \
    return vlc_##basename##_generic(x); \
}
#endif

VLC_INT_FUNC(ctz)
VLC_INT_FUNC(parity)
VLC_INT_FUNC(popcount)

#ifndef __cplusplus
# define VLC_INT_GENERIC(func,x) \
    _Generic((x), \
        unsigned char:      func(x), \
          signed char:      func(x), \
        unsigned short:     func(x), \
          signed short:     func(x), \
        unsigned int:       func(x), \
          signed int:       func(x), \
        unsigned long:      func##l(x), \
          signed long:      func##l(x), \
        unsigned long long: func##ll(x), \
          signed long long: func##ll(x))

/**
 * Count leading zeroes
 *
 * This function counts the number of consecutive zero (clear) bits
 * down from the highest order bit in an unsigned integer.
 *
 * \param x a non-zero integer
 * \note This macro assumes that CHAR_BIT equals 8.
 * \warning By definition, the result depends on the (width of the) type of x.
 * \return The number of leading zero bits in x.
 */
# define clz(x) \
    _Generic((x), \
        unsigned char: (vlc_clz(x) - (sizeof (unsigned) - 1) * 8), \
        unsigned short: (vlc_clz(x) \
        - (sizeof (unsigned) - sizeof (unsigned short)) * 8), \
        unsigned: vlc_clz(x), \
        unsigned long: vlc_clzl(x), \
        unsigned long long: vlc_clzll(x))

/**
 * Count trailing zeroes
 *
 * This function counts the number of consecutive zero bits
 * up from the lowest order bit in an unsigned integer.
 *
 * \param x a non-zero integer
 * \note This function assumes that CHAR_BIT equals 8.
 * \return The number of trailing zero bits in x.
 */
# define ctz(x) VLC_INT_GENERIC(vlc_ctz, x)

/**
 * Parity
 *
 * This function determines the parity of an integer.
 * \retval 0 if x has an even number of set bits.
 * \retval 1 if x has an odd number of set bits.
 */
# define parity(x) VLC_INT_GENERIC(vlc_parity, x)

/**
 * Bit weight / population count
 *
 * This function counts the number of non-zero bits in an integer.
 *
 * \return The count of non-zero bits.
 */
# define vlc_popcount(x) \
    _Generic((x), \
        signed char:  vlc_popcount((unsigned char)(x)), \
        signed short: vlc_popcount((unsigned short)(x)), \
        default: VLC_INT_GENERIC(vlc_popcount ,x))
#else
VLC_USED static inline int vlc_popcount(unsigned char x)
{
    return vlc_popcount((unsigned)x);
}

VLC_USED static inline int vlc_popcount(unsigned short x)
{
    return vlc_popcount((unsigned)x);
}

VLC_USED static inline int vlc_popcount(unsigned long x)
{
    return vlc_popcountl(x);
}

VLC_USED static inline int vlc_popcount(unsigned long long x)
{
    return vlc_popcountll(x);
}
#endif

/** Byte swap (16 bits) */
VLC_USED
static inline uint16_t vlc_bswap16(uint16_t x)
{
    return (x << 8) | (x >> 8);
}

/** Byte swap (32 bits) */
VLC_USED
static inline uint32_t vlc_bswap32(uint32_t x)
{
#if defined (__GNUC__) || defined(__clang__)
    return __builtin_bswap32 (x);
#else
    return ((x & 0x000000FF) << 24)
         | ((x & 0x0000FF00) <<  8)
         | ((x & 0x00FF0000) >>  8)
         | ((x & 0xFF000000) >> 24);
#endif
}

/** Byte swap (64 bits) */
VLC_USED
static inline uint64_t vlc_bswap64(uint64_t x)
{
#if defined (__GNUC__) || defined(__clang__)
    return __builtin_bswap64 (x);
#elif !defined (__cplusplus)
    return ((x & 0x00000000000000FF) << 56)
         | ((x & 0x000000000000FF00) << 40)
         | ((x & 0x0000000000FF0000) << 24)
         | ((x & 0x00000000FF000000) <<  8)
         | ((x & 0x000000FF00000000) >>  8)
         | ((x & 0x0000FF0000000000) >> 24)
         | ((x & 0x00FF000000000000) >> 40)
         | ((x & 0xFF00000000000000) >> 56);
#else
    return ((x & 0x00000000000000FFULL) << 56)
         | ((x & 0x000000000000FF00ULL) << 40)
         | ((x & 0x0000000000FF0000ULL) << 24)
         | ((x & 0x00000000FF000000ULL) <<  8)
         | ((x & 0x000000FF00000000ULL) >>  8)
         | ((x & 0x0000FF0000000000ULL) >> 24)
         | ((x & 0x00FF000000000000ULL) >> 40)
         | ((x & 0xFF00000000000000ULL) >> 56);
#endif
}
/** @} */

/**
 * \defgroup overflow Overflowing arithmetic
 * @{
 */
static inline bool uadd_overflow(unsigned a, unsigned b, unsigned *res)
{
#if VLC_GCC_VERSION(5,0) || defined(__clang__)
     return __builtin_uadd_overflow(a, b, res);
#else
     *res = a + b;
     return (a + b) < a;
#endif
}

static inline bool uaddl_overflow(unsigned long a, unsigned long b,
                                  unsigned long *res)
{
#if VLC_GCC_VERSION(5,0) || defined(__clang__)
     return __builtin_uaddl_overflow(a, b, res);
#else
     *res = a + b;
     return (a + b) < a;
#endif
}

static inline bool uaddll_overflow(unsigned long long a, unsigned long long b,
                                   unsigned long long *res)
{
#if VLC_GCC_VERSION(5,0) || defined(__clang__)
     return __builtin_uaddll_overflow(a, b, res);
#else
     *res = a + b;
     return (a + b) < a;
#endif
}

#ifndef __cplusplus
/**
 * Overflowing addition
 *
 * Converts \p a and \p b to the type of \p *r.
 * Then computes the sum of both conversions while checking for overflow.
 * Finally stores the result in \p *r.
 *
 * \param a an integer
 * \param b an integer
 * \param r a pointer to an integer [OUT]
 * \retval false The sum did not overflow.
 * \retval true The sum overflowed.
 */
# define add_overflow(a,b,r) \
    _Generic(*(r), \
        unsigned: uadd_overflow(a, b, (unsigned *)(r)), \
        unsigned long: uaddl_overflow(a, b, (unsigned long *)(r)), \
        unsigned long long: uaddll_overflow(a, b, (unsigned long long *)(r)))
#else
static inline bool add_overflow(unsigned a, unsigned b, unsigned *res)
{
    return uadd_overflow(a, b, res);
}

static inline bool add_overflow(unsigned long a, unsigned long b,
                                unsigned long *res)
{
    return uaddl_overflow(a, b, res);
}

static inline bool add_overflow(unsigned long long a, unsigned long long b,
                                unsigned long long *res)
{
    return uaddll_overflow(a, b, res);
}
#endif

#if !(VLC_GCC_VERSION(5,0) || defined(__clang__))
# include <limits.h>
#endif

static inline bool umul_overflow(unsigned a, unsigned b, unsigned *res)
{
#if VLC_GCC_VERSION(5,0) || defined(__clang__)
     return __builtin_umul_overflow(a, b, res);
#else
     *res = a * b;
     return b > 0 && a > (UINT_MAX / b);
#endif
}

static inline bool umull_overflow(unsigned long a, unsigned long b,
                                  unsigned long *res)
{
#if VLC_GCC_VERSION(5,0) || defined(__clang__)
     return __builtin_umull_overflow(a, b, res);
#else
     *res = a * b;
     return b > 0 && a > (ULONG_MAX / b);
#endif
}

static inline bool umulll_overflow(unsigned long long a, unsigned long long b,
                                   unsigned long long *res)
{
#if VLC_GCC_VERSION(5,0) || defined(__clang__)
     return __builtin_umulll_overflow(a, b, res);
#else
     *res = a * b;
     return b > 0 && a > (ULLONG_MAX / b);
#endif
}

#ifndef __cplusplus
/**
 * Overflowing multiplication
 *
 * Converts \p a and \p b to the type of \p *r.
 * Then computes the product of both conversions while checking for overflow.
 * Finally stores the result in \p *r.
 *
 * \param a an integer
 * \param b an integer
 * \param r a pointer to an integer [OUT]
 * \retval false The product did not overflow.
 * \retval true The product overflowed.
 */
#define mul_overflow(a,b,r) \
    _Generic(*(r), \
        unsigned: umul_overflow(a, b, (unsigned *)(r)), \
        unsigned long: umull_overflow(a, b, (unsigned long *)(r)), \
        unsigned long long: umulll_overflow(a, b, (unsigned long long *)(r)))
#else
static inline bool mul_overflow(unsigned a, unsigned b, unsigned *res)
{
    return umul_overflow(a, b, res);
}

static inline bool mul_overflow(unsigned long a, unsigned long b,
                                unsigned long *res)
{
    return umull_overflow(a, b, res);
}

static inline bool mul_overflow(unsigned long long a, unsigned long long b,
                                unsigned long long *res)
{
    return umulll_overflow(a, b, res);
}
#endif
/** @} */
/** @} */

/* MSB (big endian)/LSB (little endian) conversions - network order is always
 * MSB, and should be used for both network communications and files. */

#ifdef WORDS_BIGENDIAN
# define hton16(i) ((uint16_t)(i))
# define hton32(i) ((uint32_t)(i))
# define hton64(i) ((uint64_t)(i))
#else
# define hton16(i) vlc_bswap16(i)
# define hton32(i) vlc_bswap32(i)
# define hton64(i) vlc_bswap64(i)
#endif
#define ntoh16(i) hton16(i)
#define ntoh32(i) hton32(i)
#define ntoh64(i) hton64(i)

/** Reads 16 bits in network byte order */
VLC_USED
static inline uint16_t U16_AT (const void *p)
{
    uint16_t x;

    memcpy (&x, p, sizeof (x));
    return ntoh16 (x);
}

/** Reads 32 bits in network byte order */
VLC_USED
static inline uint32_t U32_AT (const void *p)
{
    uint32_t x;

    memcpy (&x, p, sizeof (x));
    return ntoh32 (x);
}

/** Reads 64 bits in network byte order */
VLC_USED
static inline uint64_t U64_AT (const void *p)
{
    uint64_t x;

    memcpy (&x, p, sizeof (x));
    return ntoh64 (x);
}

#define GetWBE(p)  U16_AT(p)
#define GetDWBE(p) U32_AT(p)
#define GetQWBE(p) U64_AT(p)

/** Reads 16 bits in little-endian order */
VLC_USED
static inline uint16_t GetWLE (const void *p)
{
    uint16_t x;

    memcpy (&x, p, sizeof (x));
#ifdef WORDS_BIGENDIAN
    x = vlc_bswap16 (x);
#endif
    return x;
}

/** Reads 32 bits in little-endian order */
VLC_USED
static inline uint32_t GetDWLE (const void *p)
{
    uint32_t x;

    memcpy (&x, p, sizeof (x));
#ifdef WORDS_BIGENDIAN
    x = vlc_bswap32 (x);
#endif
    return x;
}

/** Reads 64 bits in little-endian order */
VLC_USED
static inline uint64_t GetQWLE (const void *p)
{
    uint64_t x;

    memcpy (&x, p, sizeof (x));
#ifdef WORDS_BIGENDIAN
    x = vlc_bswap64 (x);
#endif
    return x;
}

/** Writes 16 bits in network byte order */
static inline void SetWBE (void *p, uint16_t w)
{
    w = hton16 (w);
    memcpy (p, &w, sizeof (w));
}

/** Writes 32 bits in network byte order */
static inline void SetDWBE (void *p, uint32_t dw)
{
    dw = hton32 (dw);
    memcpy (p, &dw, sizeof (dw));
}

/** Writes 64 bits in network byte order */
static inline void SetQWBE (void *p, uint64_t qw)
{
    qw = hton64 (qw);
    memcpy (p, &qw, sizeof (qw));
}

/** Writes 16 bits in little endian order */
static inline void SetWLE (void *p, uint16_t w)
{
#ifdef WORDS_BIGENDIAN
    w = vlc_bswap16 (w);
#endif
    memcpy (p, &w, sizeof (w));
}

/** Writes 32 bits in little endian order */
static inline void SetDWLE (void *p, uint32_t dw)
{
#ifdef WORDS_BIGENDIAN
    dw = vlc_bswap32 (dw);
#endif
    memcpy (p, &dw, sizeof (dw));
}

/** Writes 64 bits in little endian order */
static inline void SetQWLE (void *p, uint64_t qw)
{
#ifdef WORDS_BIGENDIAN
    qw = vlc_bswap64 (qw);
#endif
    memcpy (p, &qw, sizeof (qw));
}

VLC_API bool vlc_ureduce( unsigned *, unsigned *, uint64_t, uint64_t, uint64_t );

VLC_USED VLC_MALLOC
static inline void *vlc_alloc(size_t count, size_t size)
{
    return mul_overflow(count, size, &size) ? NULL : malloc(size);
}

VLC_USED
static inline void *vlc_reallocarray(void *ptr, size_t count, size_t size)
{
    return mul_overflow(count, size, &size) ? NULL : realloc(ptr, size);
}

#endif
