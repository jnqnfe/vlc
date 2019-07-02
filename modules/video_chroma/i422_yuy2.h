/*****************************************************************************
 * i422_yuy2.h : YUV to YUV conversion module for vlc
 *****************************************************************************
 * Copyright (C) 2002, 2019 VLC authors and VideoLAN
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *          Damien Fouilleul <damienf@videolan.org>
 *          Lyndon Brown <jnqnfe@gmail.com>
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

#if defined( PLUGIN_NAME_IS_i422_yuy2_sse2 )

#if defined(CAN_COMPILE_SSE2)

/* SSE2 assembly */

#define SSE2_CALL(SSE2_INSTRUCTIONS)        \
    do {                                    \
    __asm__ __volatile__(                   \
        ".p2align 3 \n\t"                   \
        SSE2_INSTRUCTIONS                   \
        :                                   \
        : "r" (p_line), "r" (p_y),          \
          "r" (p_u), "r" (p_v)              \
        : "xmm0", "xmm1", "xmm2" );         \
        p_line += 32; p_y += 16;            \
        p_u += 8; p_v += 8;                 \
    } while(0)

#define SSE2_END  __asm__ __volatile__ ( "sfence" ::: "memory" )

#define SSE2_YUV422_YUYV_ALIGNED "                                      \n\
movdqa      (%1), %%xmm0  # Load 16 Y           yF yE yD ... y2 y1 y0   \n\
movq        (%2), %%xmm1  # Load 8 Cb           00 00 00 ... u2 u1 u0   \n\
movq        (%3), %%xmm2  # Load 8 Cr           00 00 00 ... v2 v1 v0   \n\
punpcklbw %%xmm2, %%xmm1  #                     v7 u7 ... v1 u1 v0 u0   \n\
movdqa    %%xmm0, %%xmm2  #                     yF yE yD ... y2 y1 y0   \n\
punpcklbw %%xmm1, %%xmm2  #                     v3 y7 ... v0 y1 u0 y0   \n\
movntdq   %%xmm2, (%0)    # Store low YUYV                              \n\
punpckhbw %%xmm1, %%xmm0  #                     v7 yF ... v4 y9 u4 y8   \n\
movntdq   %%xmm0, 16(%0)  # Store high YUYV                             \n\
"

#define SSE2_YUV422_YUYV_UNALIGNED "                                    \n\
movdqu      (%1), %%xmm0  # Load 16 Y           yF yE yD ... y2 y1 y0   \n\
movq        (%2), %%xmm1  # Load 8 Cb           00 00 00 ... u2 u1 u0   \n\
movq        (%3), %%xmm2  # Load 8 Cr           00 00 00 ... v2 v1 v0   \n\
prefetchnta (%0)          # Tell CPU not to cache output YUYV data      \n\
punpcklbw %%xmm2, %%xmm1  #                     v7 u7 ... v1 u1 v0 u0   \n\
movdqa    %%xmm0, %%xmm2  #                     yF yE yD ... y2 y1 y0   \n\
punpcklbw %%xmm1, %%xmm2  #                     v3 y7 ... v0 y1 u0 y0   \n\
movdqu    %%xmm2, (%0)    # Store low YUYV                              \n\
punpckhbw %%xmm1, %%xmm0  #                     v7 yF ... v4 y9 u4 y8   \n\
movdqu    %%xmm0, 16(%0)  # Store high YUYV                             \n\
"

#define SSE2_YUV422_YVYU_ALIGNED "                                      \n\
movdqa      (%1), %%xmm0  # Load 16 Y           yF yE yD ... y2 y1 y0   \n\
movq        (%2), %%xmm2  # Load 8 Cb           00 00 00 ... u2 u1 u0   \n\
movq        (%3), %%xmm1  # Load 8 Cr           00 00 00 ... v2 v1 v0   \n\
punpcklbw %%xmm2, %%xmm1  #                     u7 v7 ... u1 v1 u0 v0   \n\
movdqa    %%xmm0, %%xmm2  #                     yF yE yD ... y2 y1 y0   \n\
punpcklbw %%xmm1, %%xmm2  #                     u3 y7 ... u0 y1 v0 y0   \n\
movntdq   %%xmm2, (%0)    # Store low YUYV                              \n\
punpckhbw %%xmm1, %%xmm0  #                     u7 yF ... u4 y9 v4 y8   \n\
movntdq   %%xmm0, 16(%0)  # Store high YUYV                             \n\
"

#define SSE2_YUV422_YVYU_UNALIGNED "                                    \n\
movdqu      (%1), %%xmm0  # Load 16 Y           yF yE yD ... y2 y1 y0   \n\
movq        (%2), %%xmm2  # Load 8 Cb           00 00 00 ... u2 u1 u0   \n\
movq        (%3), %%xmm1  # Load 8 Cr           00 00 00 ... v2 v1 v0   \n\
prefetchnta (%0)          # Tell CPU not to cache output YUYV data      \n\
punpcklbw %%xmm2, %%xmm1  #                     u7 v7 ... u1 v1 u0 v0   \n\
movdqa    %%xmm0, %%xmm2  #                     yF yE yD ... y2 y1 y0   \n\
punpcklbw %%xmm1, %%xmm2  #                     u3 y7 ... u0 y1 v0 y0   \n\
movdqu    %%xmm2, (%0)    # Store low YUYV                              \n\
punpckhbw %%xmm1, %%xmm0  #                     u7 yF ... u4 y9 v4 y8   \n\
movdqu    %%xmm0, 16(%0)  # Store high YUYV                             \n\
"

#define SSE2_YUV422_UYVY_ALIGNED "                                      \n\
movdqa      (%1), %%xmm0  # Load 16 Y           yF yE yD ... y2 y1 y0   \n\
movq        (%2), %%xmm1  # Load 8 Cb           00 00 00 ... u2 u1 u0   \n\
movq        (%3), %%xmm2  # Load 8 Cr           00 00 00 ... v2 v1 v0   \n\
punpcklbw %%xmm2, %%xmm1  #                     v7 u7 ... v1 u1 v0 u0   \n\
movdqa    %%xmm1, %%xmm2  #                     v7 u7 ... v1 u1 v0 u0   \n\
punpcklbw %%xmm0, %%xmm2  #                     y7 v3 ... y1 v0 y0 u0   \n\
movntdq   %%xmm2, (%0)    # Store low UYVY                              \n\
punpckhbw %%xmm0, %%xmm1  #                     yF v7 ... y9 v4 y8 u4   \n\
movntdq   %%xmm1, 16(%0)  # Store high UYVY                             \n\
"

#define SSE2_YUV422_UYVY_UNALIGNED "                                    \n\
movdqu      (%1), %%xmm0  # Load 16 Y           yF yE yD ... y2 y1 y0   \n\
movq        (%2), %%xmm1  # Load 8 Cb           00 00 00 ... u2 u1 u0   \n\
movq        (%3), %%xmm2  # Load 8 Cr           00 00 00 ... v2 v1 v0   \n\
prefetchnta (%0)          # Tell CPU not to cache output YUYV data      \n\
punpcklbw %%xmm2, %%xmm1  #                     v7 u7 ... v1 u1 v0 u0   \n\
movdqa    %%xmm1, %%xmm2  #                     v7 u7 ... v1 u1 v0 u0   \n\
punpcklbw %%xmm0, %%xmm2  #                     y7 v3 ... y1 v0 y0 u0   \n\
movdqu    %%xmm2, (%0)    # Store low UYVY                              \n\
punpckhbw %%xmm0, %%xmm1  #                     yF v7 ... y9 v4 y8 u4   \n\
movdqu    %%xmm1, 16(%0)  # Store high UYVY                             \n\
"

#elif defined(HAVE_SSE2_INTRINSICS)

/* SSE2 intrinsics */

#include <emmintrin.h>

#define SSE2_CALL(SSE2_INSTRUCTIONS)    \
    do {                                \
        __m128i xmm0, xmm1, xmm2;        \
        SSE2_INSTRUCTIONS               \
        p_line += 32; p_y += 16;        \
        p_u += 8; p_v += 8;             \
    } while(0)

#define SSE2_END  _mm_sfence()

#define SSE2_YUV422_YUYV_ALIGNED                \
    xmm0 = _mm_load_si128((__m128i *)p_y);      \
    xmm1 = _mm_loadl_epi64((__m128i *)p_u);     \
    xmm2 = _mm_loadl_epi64((__m128i *)p_v);     \
    xmm1 = _mm_unpacklo_epi8(xmm1, xmm2);       \
    xmm2 = xmm0;                                \
    xmm2 = _mm_unpacklo_epi8(xmm2, xmm1);       \
    _mm_stream_si128((__m128i*)(p_line), xmm2); \
    xmm0 = _mm_unpackhi_epi8(xmm0, xmm1);       \
    _mm_stream_si128((__m128i*)(p_line+16), xmm0);
 
#define SSE2_YUV422_YUYV_UNALIGNED              \
    xmm0 = _mm_loadu_si128((__m128i *)p_y);     \
    xmm1 = _mm_loadl_epi64((__m128i *)p_u);     \
    xmm2 = _mm_loadl_epi64((__m128i *)p_v);     \
    xmm1 = _mm_unpacklo_epi8(xmm1, xmm2);       \
    xmm2 = xmm0;                                \
    xmm2 = _mm_unpacklo_epi8(xmm2, xmm1);       \
    _mm_storeu_si128((__m128i*)(p_line), xmm2); \
    xmm0 = _mm_unpackhi_epi8(xmm0, xmm1);       \
    _mm_storeu_si128((__m128i*)(p_line+16), xmm0);
 
#define SSE2_YUV422_YVYU_ALIGNED                \
    xmm0 = _mm_load_si128((__m128i *)p_y);      \
    xmm2 = _mm_loadl_epi64((__m128i *)p_u);     \
    xmm1 = _mm_loadl_epi64((__m128i *)p_v);     \
    xmm1 = _mm_unpacklo_epi8(xmm1, xmm2);       \
    xmm2 = xmm0;                                \
    xmm2 = _mm_unpacklo_epi8(xmm2, xmm1);       \
    _mm_stream_si128((__m128i*)(p_line), xmm2); \
    xmm0 = _mm_unpackhi_epi8(xmm0, xmm1);       \
    _mm_stream_si128((__m128i*)(p_line+16), xmm0);

#define SSE2_YUV422_YVYU_UNALIGNED              \
    xmm0 = _mm_loadu_si128((__m128i *)p_y);     \
    xmm2 = _mm_loadl_epi64((__m128i *)p_u);     \
    xmm1 = _mm_loadl_epi64((__m128i *)p_v);     \
    xmm1 = _mm_unpacklo_epi8(xmm1, xmm2);       \
    xmm2 = xmm0;                                \
    xmm2 = _mm_unpacklo_epi8(xmm2, xmm1);       \
    _mm_storeu_si128((__m128i*)(p_line), xmm2); \
    xmm0 = _mm_unpackhi_epi8(xmm0, xmm1);       \
    _mm_storeu_si128((__m128i*)(p_line+16), xmm0);

#define SSE2_YUV422_UYVY_ALIGNED                \
    xmm0 = _mm_load_si128((__m128i *)p_y);      \
    xmm1 = _mm_loadl_epi64((__m128i *)p_u);     \
    xmm2 = _mm_loadl_epi64((__m128i *)p_v);     \
    xmm1 = _mm_unpacklo_epi8(xmm1, xmm2);       \
    xmm2 = xmm1;                                \
    xmm2 = _mm_unpacklo_epi8(xmm2, xmm0);       \
    _mm_stream_si128((__m128i*)(p_line), xmm2); \
    xmm1 = _mm_unpackhi_epi8(xmm1, xmm0);       \
    _mm_stream_si128((__m128i*)(p_line+16), xmm1);

#define SSE2_YUV422_UYVY_UNALIGNED              \
    xmm0 = _mm_loadu_si128((__m128i *)p_y);     \
    xmm1 = _mm_loadl_epi64((__m128i *)p_u);     \
    xmm2 = _mm_loadl_epi64((__m128i *)p_v);     \
    xmm1 = _mm_unpacklo_epi8(xmm1, xmm2);       \
    xmm2 = xmm1;                                \
    xmm2 = _mm_unpacklo_epi8(xmm2, xmm0);       \
    _mm_storeu_si128((__m128i*)(p_line), xmm2); \
    xmm1 = _mm_unpackhi_epi8(xmm1, xmm0);       \
    _mm_storeu_si128((__m128i*)(p_line+16), xmm1);

#endif

#elif defined( PLUGIN_NAME_IS_i422_yuy2_avx2 )

#if defined(CAN_COMPILE_AVX2)

/* AVX2 assembly */

#define AVX2_CALL(AVX2_INSTRUCTIONS)        \
    do {                                    \
    __asm__ __volatile__(                   \
        ".p2align 3 \n\t"                   \
        AVX2_INSTRUCTIONS                   \
        :                                   \
        : [l]"r"(p_line), [y]"r"(p_y),      \
          [u]"r"(p_u), [v]"r"(p_v)          \
        : "ymm0", "ymm1", "ymm2" );         \
        p_line += 64; p_y += 32;            \
        p_u += 16; p_v += 16;               \
    } while(0)

#define AVX2_END __asm__ __volatile__ ( " \
    sfence                              \n\
    vzeroupper                          \n\
    " ::: "memory" )

#define AVX2_INIT_ALIGNED "                                                    \n\
vmovdqa      (%[y]), %%ymm0  # Load 32 Y                      ...  y2  y1  y0  \n\
vmovdqa      (%[u]), %%xmm1  # Load 16 Cb into lower half     ...  u2  u1  u0  \n\
vmovdqa      (%[v]), %%xmm2  # Load 16 Cr into lower half     ...  v2  v1  v0  \n\
"

#define AVX2_INIT_UNALIGNED "                                                  \n\
vmovdqu      (%[y]), %%ymm0  # Load 32 Y                      ...  y2  y1  y0  \n\
vmovdqu      (%[u]), %%xmm1  # Load 16 Cb into lower half     ...  u2  u1  u0  \n\
vmovdqu      (%[v]), %%xmm2  # Load 16 Cr into lower half     ...  v2  v1  v0  \n\
prefetchnta  (%[l])          # Tell CPU not to cache output data               \n\
"

#define AVX2_YUV422_YUYV_ALIGNED "                                                   \n\
vpunpcklbw %%ymm2, %%ymm1, %%ymm1  # Interleave u,v              ... v1  u1  v0  u0  \n\
vpunpcklbw %%ymm1, %%ymm0, %%ymm2  # Interleave (low) y,uv       ... v0  y1  u0  y0  \n\
vmovntdq   %%ymm2, (%[l])          # Store low YUYV                                  \n\
vpunpckhbw %%ymm1, %%ymm0, %%ymm1  # Interleave (high) y,uv      ... v8 y17  u8 y16  \n\
vmovntdq   %%ymm1, 32(%[l])        # Store high YUYV                                 \n\
"

#define AVX2_YUV422_YUYV_UNALIGNED "                                                 \n\
vpunpcklbw %%ymm2, %%ymm1, %%ymm1  # Interleave u,v              ... v1  u1  v0  u0  \n\
vpunpcklbw %%ymm1, %%ymm0, %%ymm2  # Interleave (low) y,uv       ... v0  y1  u0  y0  \n\
vmovdqu    %%ymm2, (%[l])          # Store low YUYV                                  \n\
vpunpckhbw %%ymm1, %%ymm0, %%ymm1  # Interleave (high) y,uv      ... v8 y17  u8 y16  \n\
vmovdqu    %%ymm1, 32(%[l])        # Store high YUYV                                 \n\
"

#define AVX2_YUV422_YVYU_ALIGNED "                                                   \n\
vpunpcklbw %%ymm1, %%ymm2, %%ymm1  # Interleave v,u              ... u1  v1  u0  v0  \n\
vpunpcklbw %%ymm1, %%ymm0, %%ymm2  # Interleave (low) y,vu       ... u0  y1  v0  y0  \n\
vmovntdq   %%ymm2, (%[l])          # Store low YUYV                                  \n\
vpunpckhbw %%ymm1, %%ymm0, %%ymm1  # Interleave (high) y,vu      ... u8 y17  v8 y16  \n\
vmovntdq   %%ymm1, 32(%[l])        # Store high YUYV                                 \n\
"

#define AVX2_YUV422_YVYU_UNALIGNED "                                                 \n\
vpunpcklbw %%ymm1, %%ymm2, %%ymm1  # Interleave v,u              ... u1  v1  u0  v0  \n\
vpunpcklbw %%ymm1, %%ymm0, %%ymm2  # Interleave (low) y,vu       ... u0  y1  v0  y0  \n\
vmovdqu    %%ymm2, (%[l])          # Store low YUYV                                  \n\
vpunpckhbw %%ymm1, %%ymm0, %%ymm1  # Interleave (high) y,vu      ... u8 y17  v8 y16  \n\
vmovdqu    %%ymm1, 32(%[l])        # Store high YUYV                                 \n\
"

#define AVX2_YUV422_UYVY_ALIGNED "                                                   \n\
vpunpcklbw %%ymm2, %%ymm1, %%ymm1  # Interleave u,v              ... v1  u1  v0  u0  \n\
vpunpcklbw %%ymm0, %%ymm1, %%ymm2  # Interleave (low) uv,y       ... y1  v0  y0  u0  \n\
vmovntdq   %%ymm2, (%[l])          # Store low UYVY                                  \n\
vpunpckhbw %%ymm0, %%ymm1, %%ymm1  # Interleave (high) uv,y     ... y17  v8 y16  u8  \n\
vmovntdq   %%ymm1, 32(%[l])        # Store high UYVY                                 \n\
"

#define AVX2_YUV422_UYVY_UNALIGNED "                                                 \n\
vpunpcklbw %%ymm2, %%ymm1, %%ymm1  # Interleave u,v              ... v1  u1  v0  u0  \n\
vpunpcklbw %%ymm0, %%ymm1, %%ymm2  # Interleave (low) uv,y       ... y1  v0  y0  u0  \n\
vmovdqu    %%ymm2, (%[l])          # Store low UYVY                                  \n\
vpunpckhbw %%ymm0, %%ymm1, %%ymm1  # Interleave (high) uv,y     ... y17  v8 y16  u8  \n\
vmovdqu    %%ymm1, 32(%[l])        # Store high UYVY                                 \n\
"

#elif defined(HAVE_AVX2_INTRINSICS)

/* AVX2 intrinsics */

#include <immintrin.h>

#define AVX2_CALL(AVX2_INSTRUCTIONS)    \
    do {                                \
        __m256i ymm0, ymm1, ymm2;       \
        AVX2_INSTRUCTIONS               \
        p_line += 64; p_y += 32;        \
        p_u += 16; p_v += 16;           \
    } while(0)

#define AVX2_END  \
    _mm_sfence(); \
    _mm256_zeroupper();

#define AVX2_INIT_ALIGNED                      \
    ymm0 = _mm256_load_si256((__m256i *)p_y);  \
    ymm1 = _mm256_inserti128_si256(ymm1, *((__m128i*)p_u), 0); \
    ymm2 = _mm256_inserti128_si256(ymm2, *((__m128i*)p_v), 0);

#define AVX2_INIT_UNALIGNED                    \
    ymm0 = _mm256_loadu_si256((__m256i *)p_y); \
    ymm1 = _mm256_inserti128_si256(ymm1, *((__m128i*)p_u), 0); \
    ymm2 = _mm256_inserti128_si256(ymm2, *((__m128i*)p_v), 0); \
    _mm_prefetch(p_line, _MM_HINT_NTA);

#define AVX2_YUV422_YUYV_ALIGNED                     \
    ymm1 = _mm256_unpacklo_epi8(ymm1, ymm2);         \
    ymm2 = _mm256_unpacklo_epi8(ymm0, ymm1);         \
    _mm256_stream_si256((__m256i*)(p_line), ymm2);   \
    ymm1 = _mm256_unpackhi_epi8(ymm0, ymm1);         \
    _mm256_stream_si256((__m256i*)(p_line+32), ymm1);

#define AVX2_YUV422_YUYV_UNALIGNED                   \
    ymm1 = _mm256_unpacklo_epi8(ymm1, ymm2);         \
    ymm2 = _mm256_unpacklo_epi8(ymm0, ymm1);         \
    _mm256_storeu_si256((__m256i*)(p_line), ymm2);   \
    ymm1 = _mm256_unpackhi_epi8(ymm0, ymm1);         \
    _mm256_storeu_si256((__m256i*)(p_line+32), ymm1);

#define AVX2_YUV422_YVYU_ALIGNED                     \
    ymm1 = _mm256_unpacklo_epi8(ymm2, ymm1);         \
    ymm2 = _mm256_unpacklo_epi8(ymm0, ymm1);         \
    _mm256_stream_si256((__m256i*)(p_line), ymm2);   \
    ymm1 = _mm256_unpackhi_epi8(ymm0, ymm1);         \
    _mm256_stream_si256((__m256i*)(p_line+32), ymm1);

#define AVX2_YUV422_YVYU_UNALIGNED                   \
    ymm1 = _mm256_unpacklo_epi8(ymm2, ymm1);         \
    ymm2 = _mm256_unpacklo_epi8(ymm0, ymm1);         \
    _mm256_storeu_si256((__m256i*)(p_line), ymm2);   \
    ymm1 = _mm256_unpackhi_epi8(ymm0, ymm1);         \
    _mm256_storeu_si256((__m256i*)(p_line+32), ymm1);

#define AVX2_YUV422_UYVY_ALIGNED                     \
    ymm1 = _mm256_unpacklo_epi8(ymm1, ymm2);         \
    ymm2 = _mm256_unpacklo_epi8(ymm1, ymm0);         \
    _mm256_stream_si256((__m256i*)(p_line), ymm2);   \
    ymm1 = _mm256_unpackhi_epi8(ymm1, ymm0);         \
    _mm256_stream_si256((__m256i*)(p_line+32), ymm1);

#define AVX2_YUV422_UYVY_UNALIGNED                   \
    ymm1 = _mm256_unpacklo_epi8(ymm1, ymm2);         \
    ymm2 = _mm256_unpacklo_epi8(ymm1, ymm0);         \
    _mm256_storeu_si256((__m256i*)(p_line), ymm2);   \
    ymm1 = _mm256_unpackhi_epi8(ymm1, ymm0);         \
    _mm256_storeu_si256((__m256i*)(p_line+32), ymm1);

#endif

#endif

#define C_YUV422_YUYV( p_line, p_y, p_u, p_v )                              \
    *(p_line)++ = *(p_y)++;                                                 \
    *(p_line)++ = *(p_u)++;                                                 \
    *(p_line)++ = *(p_y)++;                                                 \
    *(p_line)++ = *(p_v)++;                                                 \

#define C_YUV422_YVYU( p_line, p_y, p_u, p_v )                              \
    *(p_line)++ = *(p_y)++;                                                 \
    *(p_line)++ = *(p_v)++;                                                 \
    *(p_line)++ = *(p_y)++;                                                 \
    *(p_line)++ = *(p_u)++;                                                 \

#define C_YUV422_UYVY( p_line, p_y, p_u, p_v )                              \
    *(p_line)++ = *(p_u)++;                                                 \
    *(p_line)++ = *(p_y)++;                                                 \
    *(p_line)++ = *(p_v)++;                                                 \
    *(p_line)++ = *(p_y)++;                                                 \

#define C_YUV422_Y211( p_line, p_y, p_u, p_v )                              \
    *(p_line)++ = *(p_y); p_y += 2;                                         \
    *(p_line)++ = *(p_u) - 0x80; p_u += 2;                                  \
    *(p_line)++ = *(p_y); p_y += 2;                                         \
    *(p_line)++ = *(p_v) - 0x80; p_v += 2;                                  \

