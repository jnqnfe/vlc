/*****************************************************************************
 * algo_x.c : "X" algorithm for vlc deinterlacer
 *****************************************************************************
 * Copyright (C) 2000-2011 VLC authors and VideoLAN
 *
 * Author: Laurent Aimar <fenrir@videolan.org>
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

#ifdef HAVE_CONFIG_H
#   include "config.h"
#endif

#include <stdint.h>

#include <vlc_common.h>
#include <vlc_cpu.h>
#include <vlc_picture.h>

#include "deinterlace.h" /* filter_sys_t */

#include "algo_x.h"

/*****************************************************************************
 * Internal functions
 *****************************************************************************/

/* XDeint8x8Detect: detect if a 8x8 block is interlaced.
 * XXX: It need to access to 8x10
 * We use more than 8 lines to help with scrolling (text)
 * (and because XDeint8x8Frame use line 9)
 * XXX: smooth/uniform area with noise detection doesn't works well
 * but it's not really a problem because they don't have much details anyway
 */
static inline int ssd( int a ) { return a*a; }
static inline int XDeint8x8DetectC( uint8_t *src, int i_src )
{
    int y, x;
    int ff, fr;
    int fc;

    /* Detect interlacing */
    fc = 0;
    for( y = 0; y < 7; y += 2 )
    {
        ff = fr = 0;
        for( x = 0; x < 8; x++ )
        {
            fr += ssd(src[      x] - src[1*i_src+x]) +
                  ssd(src[i_src+x] - src[2*i_src+x]);
            ff += ssd(src[      x] - src[2*i_src+x]) +
                  ssd(src[i_src+x] - src[3*i_src+x]);
        }
        if( ff < 6*fr/8 && fr > 32 )
            fc++;

        src += 2*i_src;
    }

    return fc < 1 ? false : true;
}

/* TODO: This is a simple conversion of MMX to using SSE registers,
   without making use of their expanded width. Would that require
   migration to a 16x16 processing model though? */
#ifdef CAN_COMPILE_SSE
VLC_SSE
static inline int XDeint8x8DetectSSE( uint8_t *src, int i_src )
{

    int y, x;
    int32_t ff, fr;
    int fc;

    /* Detect interlacing */
    fc = 0;
    __asm__ volatile ("pxor %%xmm7, %%xmm7" ::: "xmm7");
    for( y = 0; y < 9; y += 2 )
    {
        ff = fr = 0;
        __asm__ volatile (
            "pxor %%xmm5, %%xmm5\n"
            "pxor %%xmm6, %%xmm6\n"
            ::: "xmm5", "xmm6"
        );
        for( x = 0; x < 8; x+=4 )
        {
            __asm__ volatile (
                "movd %0, %%xmm0\n"
                "movd %1, %%xmm1\n"
                "movd %2, %%xmm2\n"
                "movd %3, %%xmm3\n"

                "punpcklbw %%xmm7, %%xmm0\n"
                "punpcklbw %%xmm7, %%xmm1\n"
                "punpcklbw %%xmm7, %%xmm2\n"
                "punpcklbw %%xmm7, %%xmm3\n"

                "movq %%xmm0, %%xmm4\n"

                "psubw %%xmm2, %%xmm4\n"
                "psubw %%xmm1, %%xmm0\n"
                "psubw %%xmm1, %%xmm2\n"
                "psubw %%xmm1, %%xmm3\n"

                "pmaddwd %%xmm0, %%xmm0\n"
                "pmaddwd %%xmm2, %%xmm2\n"
                "pmaddwd %%xmm3, %%xmm3\n"
                "pmaddwd %%xmm4, %%xmm4\n"
                "paddd %%xmm0, %%xmm2\n"
                "paddd %%xmm4, %%xmm3\n"
                "paddd %%xmm2, %%xmm5\n"
                "paddd %%xmm3, %%xmm6\n"

                :: "m" (src[        x]),
                   "m" (src[1*i_src+x]),
                   "m" (src[2*i_src+x]),
                   "m" (src[3*i_src+x])
                : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "xmm6"
            );
        }

        __asm__ volatile (
            "movq %%xmm5, %%xmm0\n"
            "psrlq $32, %%xmm0\n"
            "paddd %%xmm0, %%xmm5\n"
            "movd %%xmm5, %0\n"

            "movq %%xmm6, %%xmm0\n"
            "psrlq $32, %%xmm0\n"
            "paddd %%xmm0, %%xmm6\n"
            "movd %%xmm6, %1\n"

            : "=m" (fr), "=m" (ff)
            :: "xmm0", "xmm5", "xmm6", "memory"
        );

        if( ff < 6*fr/8 && fr > 32 )
            fc++;

        src += 2*i_src;
    }
    return fc;
}
#endif

static inline void XDeint8x8MergeC( uint8_t *dst,  int i_dst,
                                    uint8_t *src1, int i_src1,
                                    uint8_t *src2, int i_src2 )
{
    int y, x;

    /* Progressive */
    for( y = 0; y < 8; y += 2 )
    {
        memcpy( dst, src1, 8 );
        dst  += i_dst;

        for( x = 0; x < 8; x++ )
            dst[x] = (src1[x] + 6*src2[x] + src1[i_src1+x] + 4 ) >> 3;
        dst += i_dst;

        src1 += i_src1;
        src2 += i_src2;
    }
}

/* TODO: This is a simple conversion of MMX to using SSE registers,
   without making use of their expanded width. Would that require
   migration to a 16x16 processing model though? */
#ifdef CAN_COMPILE_SSE
VLC_SSE
static inline void XDeint8x8MergeSSE( uint8_t *dst,  int i_dst,
                                         uint8_t *src1, int i_src1,
                                         uint8_t *src2, int i_src2 )
{
    static const uint64_t m_4 = INT64_C(0x0004000400040004);
    int y, x;

    /* Progressive */
    __asm__ volatile (
        "pxor %%xmm7, %%xmm7\n"
        "movq %0, %%xmm6\n"
        :: "m" (m_4) : "xmm6", "xmm7"
    );
    for( y = 0; y < 8; y += 2 )
    {
        for( x = 0; x < 8; x +=4 )
        {
            __asm__ volatile (
                "movd %2, %%xmm0\n"
                "movd %%xmm0, %0\n"

                "movd %3, %%xmm1\n"
                "movd %4, %%xmm2\n"

                "punpcklbw %%xmm7, %%xmm0\n"
                "punpcklbw %%xmm7, %%xmm1\n"
                "punpcklbw %%xmm7, %%xmm2\n"
                "paddw %%xmm1, %%xmm1\n"
                "movq %%xmm1, %%xmm3\n"
                "paddw %%xmm3, %%xmm3\n"
                "paddw %%xmm2, %%xmm0\n"
                "paddw %%xmm3, %%xmm1\n"
                "paddw %%xmm6, %%xmm1\n"
                "paddw %%xmm1, %%xmm0\n"
                "psraw $3, %%xmm0\n"
                "packuswb %%xmm7, %%xmm0\n"
                "movd %%xmm0, %1\n"

                : "=m" (dst[x]), "=m" (dst[i_dst+x])
                : "m" (src1[x]), "m" (src2[x]), "m" (src1[i_src1+x])
                : "xmm0", "xmm1", "xmm2", "xmm3",
                  "memory"
            );
        }
        dst += 2*i_dst;
        src1 += i_src1;
        src2 += i_src2;
    }
}
#endif

/* XDeint8x8FieldE: Stupid deinterlacing (1,0,1) for block that miss a
 * neighbour
 * (Use 8x9 pixels)
 * TODO: a better one for the inner part.
 */
static inline void XDeint8x8FieldEC( uint8_t *dst, int i_dst,
                                     uint8_t *src, int i_src )
{
    int y, x;

    /* Interlaced */
    for( y = 0; y < 8; y += 2 )
    {
        memcpy( dst, src, 8 );
        dst += i_dst;

        for( x = 0; x < 8; x++ )
            dst[x] = (src[x] + src[2*i_src+x] ) >> 1;
        dst += 1*i_dst;
        src += 2*i_src;
    }
}

/* TODO: This is a simple conversion of MMX to using SSE registers,
   without making use of their expanded width. Would that require
   migration to a 16x16 processing model though? */
#ifdef CAN_COMPILE_SSE
VLC_SSE
static inline void XDeint8x8FieldESSE( uint8_t *dst, int i_dst,
                                          uint8_t *src, int i_src )
{
    int y;

    /* Interlaced */
    for( y = 0; y < 8; y += 2 )
    {
        __asm__ volatile (
            "movq %1, %%xmm0\n"
            "movq %%xmm0, %0\n"
            : "=m" (dst[0]) : "m" (src[0])
            : "xmm0", "memory"
        );
        dst += i_dst;

        __asm__ volatile (
            "movq %1, %%xmm1\n"
            "pavgb %%xmm1, %%xmm0\n"
            "movq %%xmm0, %0\n"
            : "=m" (dst[0]) : "m" (src[2*i_src])
            : "xmm0", "xmm1", "memory"
        );

        dst += 1*i_dst;
        src += 2*i_src;
    }
}
#endif

/* XDeint8x8Field: Edge oriented interpolation
 * (Need -4 and +5 pixels H, +1 line)
 */
static inline void XDeint8x8FieldC( uint8_t *dst, int i_dst,
                                    uint8_t *src, int i_src )
{
    int y, x;

    /* Interlaced */
    for( y = 0; y < 8; y += 2 )
    {
        memcpy( dst, src, 8 );
        dst += i_dst;

        for( x = 0; x < 8; x++ )
        {
            uint8_t *src2 = &src[2*i_src];
            /* I use 8 pixels just to match the MMX version, but it's overkill
             * 5 would be enough (less isn't good) */
            const int c0 = abs(src[x-4]-src2[x-2]) + abs(src[x-3]-src2[x-1]) +
                           abs(src[x-2]-src2[x+0]) + abs(src[x-1]-src2[x+1]) +
                           abs(src[x+0]-src2[x+2]) + abs(src[x+1]-src2[x+3]) +
                           abs(src[x+2]-src2[x+4]) + abs(src[x+3]-src2[x+5]);

            const int c1 = abs(src[x-3]-src2[x-3]) + abs(src[x-2]-src2[x-2]) +
                           abs(src[x-1]-src2[x-1]) + abs(src[x+0]-src2[x+0]) +
                           abs(src[x+1]-src2[x+1]) + abs(src[x+2]-src2[x+2]) +
                           abs(src[x+3]-src2[x+3]) + abs(src[x+4]-src2[x+4]);

            const int c2 = abs(src[x-2]-src2[x-4]) + abs(src[x-1]-src2[x-3]) +
                           abs(src[x+0]-src2[x-2]) + abs(src[x+1]-src2[x-1]) +
                           abs(src[x+2]-src2[x+0]) + abs(src[x+3]-src2[x+1]) +
                           abs(src[x+4]-src2[x+2]) + abs(src[x+5]-src2[x+3]);

            if( c0 < c1 && c1 <= c2 )
                dst[x] = (src[x-1] + src2[x+1]) >> 1;
            else if( c2 < c1 && c1 <= c0 )
                dst[x] = (src[x+1] + src2[x-1]) >> 1;
            else
                dst[x] = (src[x+0] + src2[x+0]) >> 1;
        }

        dst += 1*i_dst;
        src += 2*i_src;
    }
}

/* TODO: This is a simple conversion of MMX to using SSE registers,
   without making use of their expanded width. Would that require
   migration to a 16x16 processing model though? */
#ifdef CAN_COMPILE_SSE
VLC_SSE
static inline void XDeint8x8FieldSSE( uint8_t *dst, int i_dst,
                                         uint8_t *src, int i_src )
{
    int y, x;

    /* Interlaced */
    for( y = 0; y < 8; y += 2 )
    {
        memcpy( dst, src, 8 );
        dst += i_dst;

        for( x = 0; x < 8; x++ )
        {
            uint8_t *src2 = &src[2*i_src];
            int32_t c0, c1, c2;

            __asm__ volatile (
                "movq %3, %%xmm0\n"
                "movq %4, %%xmm1\n"
                "movq %5, %%xmm2\n"
                "movq %6, %%xmm3\n"
                "movq %7, %%xmm4\n"
                "movq %8, %%xmm5\n"
                "psadbw %%xmm3, %%xmm0\n"
                "psadbw %%xmm4, %%xmm1\n"
                "psadbw %%xmm5, %%xmm2\n"
                "movd %%xmm0, %2\n"
                "movd %%xmm1, %1\n"
                "movd %%xmm2, %0\n"
                : "=m" (c0), "=m" (c1), "=m" (c2)
                : "m" (src[x-2]), "m" (src[x-3]), "m" (src[x-4]),
                  "m" (src2[x-4]), "m" (src2[x-3]), "m" (src2[x-2])
                : "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "xmm5", "memory"
            );

            if( c0 < c1 && c1 <= c2 )
                dst[x] = (src[x-1] + src2[x+1]) >> 1;
            else if( c2 < c1 && c1 <= c0 )
                dst[x] = (src[x+1] + src2[x-1]) >> 1;
            else
                dst[x] = (src[x+0] + src2[x+0]) >> 1;
        }

        dst += 1*i_dst;
        src += 2*i_src;
    }
}
#endif

/* NxN arbitray size (and then only use pixel in the NxN block)
 */
static inline int XDeintNxNDetect( uint8_t *src, int i_src,
                                   int i_height, int i_width )
{
    int y, x;
    int ff, fr;
    int fc;


    /* Detect interlacing */
    /* FIXME way too simple, need to be more like XDeint8x8Detect */
    ff = fr = 0;
    fc = 0;
    for( y = 0; y < i_height - 2; y += 2 )
    {
        const uint8_t *s = &src[y*i_src];
        for( x = 0; x < i_width; x++ )
        {
            fr += ssd(s[      x] - s[1*i_src+x]);
            ff += ssd(s[      x] - s[2*i_src+x]);
        }
        if( ff < fr && fr > i_width / 2 )
            fc++;
    }

    return fc < 2 ? false : true;
}

static inline void XDeintNxNFrame( uint8_t *dst, int i_dst,
                                   uint8_t *src, int i_src,
                                   int i_width, int i_height )
{
    int y, x;

    /* Progressive */
    for( y = 0; y < i_height; y += 2 )
    {
        memcpy( dst, src, i_width );
        dst += i_dst;

        if( y < i_height - 2 )
        {
            for( x = 0; x < i_width; x++ )
                dst[x] = (src[x] + 2*src[1*i_src+x] + src[2*i_src+x] + 2 ) >> 2;
        }
        else
        {
            /* Blend last line */
            for( x = 0; x < i_width; x++ )
                dst[x] = (src[x] + src[1*i_src+x] ) >> 1;
        }
        dst += 1*i_dst;
        src += 2*i_src;
    }
}

static inline void XDeintNxNField( uint8_t *dst, int i_dst,
                                   uint8_t *src, int i_src,
                                   int i_width, int i_height )
{
    int y, x;

    /* Interlaced */
    for( y = 0; y < i_height; y += 2 )
    {
        memcpy( dst, src, i_width );
        dst += i_dst;

        if( y < i_height - 2 )
        {
            for( x = 0; x < i_width; x++ )
                dst[x] = (src[x] + src[2*i_src+x] ) >> 1;
        }
        else
        {
            /* Blend last line */
            for( x = 0; x < i_width; x++ )
                dst[x] = (src[x] + src[i_src+x]) >> 1;
        }
        dst += 1*i_dst;
        src += 2*i_src;
    }
}

static inline void XDeintNxN( uint8_t *dst, int i_dst, uint8_t *src, int i_src,
                              int i_width, int i_height )
{
    if( XDeintNxNDetect( src, i_src, i_width, i_height ) )
        XDeintNxNField( dst, i_dst, src, i_src, i_width, i_height );
    else
        XDeintNxNFrame( dst, i_dst, src, i_src, i_width, i_height );
}

/* XDeintBand8x8:
 */
static inline void XDeintBand8x8C( uint8_t *dst, int i_dst,
                                   uint8_t *src, int i_src,
                                   const int i_mbx, int i_modx )
{
    int x;

    for( x = 0; x < i_mbx; x++ )
    {
        int s;
        if( ( s = XDeint8x8DetectC( src, i_src ) ) )
        {
            if( x == 0 || x == i_mbx - 1 )
                XDeint8x8FieldEC( dst, i_dst, src, i_src );
            else
                XDeint8x8FieldC( dst, i_dst, src, i_src );
        }
        else
        {
            XDeint8x8MergeC( dst, i_dst,
                             &src[0*i_src], 2*i_src,
                             &src[1*i_src], 2*i_src );
        }

        dst += 8;
        src += 8;
    }

    if( i_modx )
        XDeintNxN( dst, i_dst, src, i_src, i_modx, 8 );
}

#ifdef CAN_COMPILE_SSE
VLC_SSE
static inline void XDeintBand8x8SSE( uint8_t *dst, int i_dst,
                                        uint8_t *src, int i_src,
                                        const int i_mbx, int i_modx )
{
    int x;

    /* Reset current line */
    for( x = 0; x < i_mbx; x++ )
    {
        int s;
        if( ( s = XDeint8x8DetectSSE( src, i_src ) ) )
        {
            if( x == 0 || x == i_mbx - 1 )
                XDeint8x8FieldESSE( dst, i_dst, src, i_src );
            else
                XDeint8x8FieldSSE( dst, i_dst, src, i_src );
        }
        else
        {
            XDeint8x8MergeSSE( dst, i_dst,
                                  &src[0*i_src], 2*i_src,
                                  &src[1*i_src], 2*i_src );
        }

        dst += 8;
        src += 8;
    }

    if( i_modx )
        XDeintNxN( dst, i_dst, src, i_src, i_modx, 8 );
}
#endif

/*****************************************************************************
 * Public functions
 *****************************************************************************/

int RenderX( filter_t *p_filter, picture_t *p_outpic, picture_t *p_pic )
{
    VLC_UNUSED(p_filter);
    int i_plane;
#if defined (CAN_COMPILE_SSE)
    const bool sse = vlc_CPU_SSE2();
#endif

    /* Copy image and skip lines */
    for( i_plane = 0 ; i_plane < p_pic->i_planes ; i_plane++ )
    {
        const int i_mby = ( p_outpic->p[i_plane].i_visible_lines + 7 )/8 - 1;
        const int i_mbx = p_outpic->p[i_plane].i_visible_pitch/8;

        const int i_mody = p_outpic->p[i_plane].i_visible_lines - 8*i_mby;
        const int i_modx = p_outpic->p[i_plane].i_visible_pitch - 8*i_mbx;

        const int i_dst = p_outpic->p[i_plane].i_pitch;
        const int i_src = p_pic->p[i_plane].i_pitch;

        int y, x;

        for( y = 0; y < i_mby; y++ )
        {
            uint8_t *dst = &p_outpic->p[i_plane].p_pixels[8*y*i_dst];
            uint8_t *src = &p_pic->p[i_plane].p_pixels[8*y*i_src];

#ifdef CAN_COMPILE_SSE
            if( sse )
                XDeintBand8x8SSE( dst, i_dst, src, i_src, i_mbx, i_modx );
            else
#endif
                XDeintBand8x8C( dst, i_dst, src, i_src, i_mbx, i_modx );
        }

        /* Last line (C only)*/
        if( i_mody )
        {
            uint8_t *dst = &p_outpic->p[i_plane].p_pixels[8*y*i_dst];
            uint8_t *src = &p_pic->p[i_plane].p_pixels[8*y*i_src];

            for( x = 0; x < i_mbx; x++ )
            {
                XDeintNxN( dst, i_dst, src, i_src, 8, i_mody );

                dst += 8;
                src += 8;
            }

            if( i_modx )
                XDeintNxN( dst, i_dst, src, i_src, i_modx, i_mody );
        }
    }

    return VLC_SUCCESS;
}
