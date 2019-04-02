/*****************************************************************************
 * i422_yuy2.c : Planar YUV 4:2:2 to Packed YUV conversion module for vlc
 *****************************************************************************
 * Copyright (C) 2000, 2001, 2019 VLC authors and VideoLAN
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *          Damien Fouilleul <damienf@videolan.org>
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

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_filter.h>
#include <vlc_picture.h>
#include <vlc_cpu.h>

#include "i422_yuy2.h"

#define SRC_FOURCC  "I422"
#if defined (PLUGIN_NAME_IS_i422_yuy2)
#    define DEST_FOURCC "YUY2,YUNV,YVYU,UYVY,UYNV,Y422,IUYV,Y211"
#else
#    define DEST_FOURCC "YUY2,YUNV,YVYU,UYVY,UYNV,Y422,IUYV"
#endif

/*****************************************************************************
 * Local and extern prototypes.
 *****************************************************************************/
static int  Activate ( filter_t * );

static void I422_YUY2               ( filter_t *, picture_t *, picture_t * );
static void I422_YVYU               ( filter_t *, picture_t *, picture_t * );
static void I422_UYVY               ( filter_t *, picture_t *, picture_t * );
static void I422_IUYV               ( filter_t *, picture_t *, picture_t * );
static picture_t *I422_YUY2_Filter  ( filter_t *, picture_t * );
static picture_t *I422_YVYU_Filter  ( filter_t *, picture_t * );
static picture_t *I422_UYVY_Filter  ( filter_t *, picture_t * );
static picture_t *I422_IUYV_Filter  ( filter_t *, picture_t * );
#if defined (PLUGIN_NAME_IS_i422_yuy2)
static void I422_Y211               ( filter_t *, picture_t *, picture_t * );
static picture_t *I422_Y211_Filter  ( filter_t *, picture_t * );
#endif

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
vlc_plugin_begin ()
#if defined (PLUGIN_NAME_IS_i422_yuy2)
    set_description( N_("Conversions from " SRC_FOURCC " to " DEST_FOURCC) )
    set_capability( VLC_CAP_VIDEO_CONVERTER, 80, Activate, NULL )
# define vlc_CPU_capable() (true)
# define VLC_TARGET
#elif defined (PLUGIN_NAME_IS_i422_yuy2_sse2)
    set_description( N_("SSE2 conversions from " SRC_FOURCC " to " DEST_FOURCC) )
    set_capability( VLC_CAP_VIDEO_CONVERTER, 120, Activate, NULL )
# define vlc_CPU_capable() vlc_CPU_SSE2()
# define VLC_TARGET VLC_SSE
#elif defined (PLUGIN_NAME_IS_i422_yuy2_avx2)
    set_description( N_("AVX2 conversions from " SRC_FOURCC " to " DEST_FOURCC) )
    set_capability( VLC_CAP_VIDEO_CONVERTER, 130, Activate, NULL )
# define vlc_CPU_capable() vlc_CPU_AVX2()
# define VLC_TARGET VLC_AVX
#endif
vlc_plugin_end ()

/*****************************************************************************
 * Activate: allocate a chroma function
 *****************************************************************************
 * This function allocates and initializes a chroma function
 *****************************************************************************/
static int Activate( filter_t *p_filter )
{
    if( !vlc_CPU_capable() )
        return VLC_EGENERIC;
    if( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) & 1
     || (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) & 1 )
    {
        return -1;
    }

    if( p_filter->fmt_in.video.orientation != p_filter->fmt_out.video.orientation )
    {
        return VLC_EGENERIC;
    }

    switch( p_filter->fmt_in.video.i_chroma )
    {
        case VLC_CODEC_I422:
            switch( p_filter->fmt_out.video.i_chroma )
            {
                case VLC_CODEC_YUYV:
                    p_filter->pf_video_filter = I422_YUY2_Filter;
                    break;

                case VLC_CODEC_YVYU:
                    p_filter->pf_video_filter = I422_YVYU_Filter;
                    break;

                case VLC_CODEC_UYVY:
                    p_filter->pf_video_filter = I422_UYVY_Filter;
                    break;

                case VLC_FOURCC('I','U','Y','V'):
                    p_filter->pf_video_filter = I422_IUYV_Filter;
                    break;

#if defined (PLUGIN_NAME_IS_i422_yuy2)
                case VLC_CODEC_Y211:
                    p_filter->pf_video_filter = I422_Y211_Filter;
                    break;
#endif

                default:
                    return -1;
            }
            break;

        default:
            return -1;
    }
    return 0;
}

/* Following functions are local */

VIDEO_FILTER_WRAPPER( I422_YUY2 )
VIDEO_FILTER_WRAPPER( I422_YVYU )
VIDEO_FILTER_WRAPPER( I422_UYVY )
VIDEO_FILTER_WRAPPER( I422_IUYV )
#if defined (PLUGIN_NAME_IS_i422_yuy2)
VIDEO_FILTER_WRAPPER( I422_Y211 )
#endif

/*****************************************************************************
 * I422_YUY2: planar YUV 4:2:2 to packed YUY2 4:2:2
 *****************************************************************************/
VLC_TARGET
static void I422_YUY2( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line = p_dest->p->p_pixels;
    uint8_t *p_y = p_source->Y_PIXELS;
    uint8_t *p_u = p_source->U_PIXELS;
    uint8_t *p_v = p_source->V_PIXELS;

    int i_x, i_y;

    const int i_source_margin = p_source->p[0].i_pitch
                                 - p_source->p[0].i_visible_pitch
                                 - p_filter->fmt_in.video.i_x_offset;
    const int i_source_margin_c = p_source->p[1].i_pitch
                                 - p_source->p[1].i_visible_pitch
                                 - ( p_filter->fmt_in.video.i_x_offset );
    const int i_dest_margin = p_dest->p->i_pitch
                               - p_dest->p->i_visible_pitch
                               - ( p_filter->fmt_out.video.i_x_offset * 2 );

#if defined (PLUGIN_NAME_IS_i422_yuy2_avx2)

    /* AVX2 aligned store/load can require 32-byte alignment */

    if( 0 == (31 & (p_source->p[Y_PLANE].i_pitch|p_dest->p->i_pitch|
        ((intptr_t)p_line|(intptr_t)p_y))) )
    {
        /* use AVX aligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 32 ; i_x-- ; )
            {
                AVX2_CALL(
                    AVX2_INIT_ALIGNED
                    AVX2_YUV422_YUYV_ALIGNED
                );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 32 ) / 2; i_x-- ; )
            {
                C_YUV422_YUYV( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    else {
        /* use AVX2 unaligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 32 ; i_x-- ; )
            {
                AVX2_CALL(
                    AVX2_INIT_UNALIGNED
                    AVX2_YUV422_YUYV_UNALIGNED
                );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 32 ) / 2; i_x-- ; )
            {
                C_YUV422_YUYV( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    AVX2_END;

#elif defined (PLUGIN_NAME_IS_i422_yuy2_sse2)

    /* SSE2 aligned store/load is faster, requires 16-byte alignment */

    if( 0 == (15 & (p_source->p[Y_PLANE].i_pitch|p_dest->p->i_pitch|
        ((intptr_t)p_line|(intptr_t)p_y))) )
    {
        /* use faster SSE2 aligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV422_YUYV_ALIGNED );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 16 ) / 2; i_x-- ; )
            {
                C_YUV422_YUYV( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    else {
        /* use slower SSE2 unaligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV422_YUYV_UNALIGNED );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 16 ) / 2; i_x-- ; )
            {
                C_YUV422_YUYV( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    SSE2_END;

#else

    for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
    {
        for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 8 ; i_x-- ; )
        {
            C_YUV422_YUYV( p_line, p_y, p_u, p_v );
            C_YUV422_YUYV( p_line, p_y, p_u, p_v );
            C_YUV422_YUYV( p_line, p_y, p_u, p_v );
            C_YUV422_YUYV( p_line, p_y, p_u, p_v );
        }
        for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 8 ) / 2; i_x-- ; )
        {
            C_YUV422_YUYV( p_line, p_y, p_u, p_v );
        }
        p_y += i_source_margin;
        p_u += i_source_margin_c;
        p_v += i_source_margin_c;
        p_line += i_dest_margin;
    }

#endif
}

/*****************************************************************************
 * I422_YVYU: planar YUV 4:2:2 to packed YVYU 4:2:2
 *****************************************************************************/
VLC_TARGET
static void I422_YVYU( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line = p_dest->p->p_pixels;
    uint8_t *p_y = p_source->Y_PIXELS;
    uint8_t *p_u = p_source->U_PIXELS;
    uint8_t *p_v = p_source->V_PIXELS;

    int i_x, i_y;

    const int i_source_margin = p_source->p[0].i_pitch
                                 - p_source->p[0].i_visible_pitch
                                 - p_filter->fmt_in.video.i_x_offset;
    const int i_source_margin_c = p_source->p[1].i_pitch
                                 - p_source->p[1].i_visible_pitch
                                 - ( p_filter->fmt_in.video.i_x_offset );
    const int i_dest_margin = p_dest->p->i_pitch
                               - p_dest->p->i_visible_pitch
                               - ( p_filter->fmt_out.video.i_x_offset * 2 );

#if defined (PLUGIN_NAME_IS_i422_yuy2_avx2)

    /* AVX2 aligned store/load can require 32-byte alignment */

    if( 0 == (31 & (p_source->p[Y_PLANE].i_pitch|p_dest->p->i_pitch|
        ((intptr_t)p_line|(intptr_t)p_y))) )
    {
        /* use AVX2 aligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 32 ; i_x-- ; )
            {
                AVX2_CALL(
                    AVX2_INIT_ALIGNED
                    AVX2_YUV422_YVYU_ALIGNED
                );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 32 ) / 2; i_x-- ; )
            {
                C_YUV422_YVYU( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    else {
        /* use AVX2 unaligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 32 ; i_x-- ; )
            {
                AVX2_CALL(
                    AVX2_INIT_UNALIGNED
                    AVX2_YUV422_YVYU_UNALIGNED
                );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 32 ) / 2; i_x-- ; )
            {
                C_YUV422_YVYU( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    AVX2_END;

#elif defined (PLUGIN_NAME_IS_i422_yuy2_sse2)

    /* SSE2 aligned store/load is faster, requires 16-byte alignment */

    if( 0 == (15 & (p_source->p[Y_PLANE].i_pitch|p_dest->p->i_pitch|
        ((intptr_t)p_line|(intptr_t)p_y))) )
    {
        /* use faster SSE2 aligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV422_YVYU_ALIGNED );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 16 ) / 2; i_x-- ; )
            {
                C_YUV422_YVYU( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    else {
        /* use slower SSE2 unaligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV422_YVYU_UNALIGNED );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 16 ) / 2; i_x-- ; )
            {
                C_YUV422_YVYU( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    SSE2_END;

#else

    for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
    {
        for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 8 ; i_x-- ; )
        {
            C_YUV422_YVYU( p_line, p_y, p_u, p_v );
            C_YUV422_YVYU( p_line, p_y, p_u, p_v );
            C_YUV422_YVYU( p_line, p_y, p_u, p_v );
            C_YUV422_YVYU( p_line, p_y, p_u, p_v );
        }
        for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 8 ) / 2; i_x-- ; )
        {
            C_YUV422_YVYU( p_line, p_y, p_u, p_v );
        }
        p_y += i_source_margin;
        p_u += i_source_margin_c;
        p_v += i_source_margin_c;
        p_line += i_dest_margin;
    }

#endif
}

/*****************************************************************************
 * I422_UYVY: planar YUV 4:2:2 to packed UYVY 4:2:2
 *****************************************************************************/
VLC_TARGET
static void I422_UYVY( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line = p_dest->p->p_pixels;
    uint8_t *p_y = p_source->Y_PIXELS;
    uint8_t *p_u = p_source->U_PIXELS;
    uint8_t *p_v = p_source->V_PIXELS;

    int i_x, i_y;

    const int i_source_margin = p_source->p[0].i_pitch
                                 - p_source->p[0].i_visible_pitch
                                 - p_filter->fmt_in.video.i_x_offset;
    const int i_source_margin_c = p_source->p[1].i_pitch
                                 - p_source->p[1].i_visible_pitch
                                 - ( p_filter->fmt_in.video.i_x_offset );
    const int i_dest_margin = p_dest->p->i_pitch
                               - p_dest->p->i_visible_pitch
                               - ( p_filter->fmt_out.video.i_x_offset * 2 );

#if defined (PLUGIN_NAME_IS_i422_yuy2_avx2)

    /* AVX2 aligned store/load can require 32-byte alignment */

    if( 0 == (31 & (p_source->p[Y_PLANE].i_pitch|p_dest->p->i_pitch|
        ((intptr_t)p_line|(intptr_t)p_y))) )
    {
        /* use AVX2 aligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 32 ; i_x-- ; )
            {
                AVX2_CALL(
                    AVX2_INIT_ALIGNED
                    AVX2_YUV422_UYVY_ALIGNED
                );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 32 ) / 2; i_x-- ; )
            {
                C_YUV422_UYVY( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    else {
        /* use AVX2 unaligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 32 ; i_x-- ; )
            {
                AVX2_CALL(
                    AVX2_INIT_UNALIGNED
                    AVX2_YUV422_UYVY_UNALIGNED
                );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 32 ) / 2; i_x-- ; )
            {
                C_YUV422_UYVY( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    AVX2_END;

#elif defined (PLUGIN_NAME_IS_i422_yuy2_sse2)

    /* SSE2 aligned store/load is faster, requires 16-byte alignment */

    if( 0 == (15 & (p_source->p[Y_PLANE].i_pitch|p_dest->p->i_pitch|
        ((intptr_t)p_line|(intptr_t)p_y))) )
    {
        /* use faster SSE2 aligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV422_UYVY_ALIGNED );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 16 ) / 2; i_x-- ; )
            {
                C_YUV422_UYVY( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    else {
        /* use slower SSE2 unaligned fetch and store */
        for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
        {
            for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 16 ; i_x-- ; )
            {
                SSE2_CALL( SSE2_YUV422_UYVY_UNALIGNED );
            }
            for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 16 ) / 2; i_x-- ; )
            {
                C_YUV422_UYVY( p_line, p_y, p_u, p_v );
            }
            p_y += i_source_margin;
            p_u += i_source_margin_c;
            p_v += i_source_margin_c;
            p_line += i_dest_margin;
        }
    }
    SSE2_END;

#else

    for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
    {
        for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 8 ; i_x-- ; )
        {
            C_YUV422_UYVY( p_line, p_y, p_u, p_v );
            C_YUV422_UYVY( p_line, p_y, p_u, p_v );
            C_YUV422_UYVY( p_line, p_y, p_u, p_v );
            C_YUV422_UYVY( p_line, p_y, p_u, p_v );
        }
        for( i_x = ( (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) % 8 ) / 2; i_x-- ; )
        {
            C_YUV422_UYVY( p_line, p_y, p_u, p_v );
        }
        p_y += i_source_margin;
        p_u += i_source_margin_c;
        p_v += i_source_margin_c;
        p_line += i_dest_margin;
    }

#endif
}

/*****************************************************************************
 * I422_IUYV: planar YUV 4:2:2 to interleaved packed IUYV 4:2:2
 *****************************************************************************/
static void I422_IUYV( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    VLC_UNUSED(p_source); VLC_UNUSED(p_dest);
    /* FIXME: TODO ! */
    msg_Err( p_filter, "I422_IUYV unimplemented, please harass <sam@zoy.org>" );
}

/*****************************************************************************
 * I422_Y211: planar YUV 4:2:2 to packed YUYV 2:1:1
 *****************************************************************************/
#if defined (PLUGIN_NAME_IS_i422_yuy2)
static void I422_Y211( filter_t *p_filter, picture_t *p_source,
                                           picture_t *p_dest )
{
    uint8_t *p_line = p_dest->p->p_pixels + p_dest->p->i_visible_lines * p_dest->p->i_pitch;
    uint8_t *p_y = p_source->Y_PIXELS;
    uint8_t *p_u = p_source->U_PIXELS;
    uint8_t *p_v = p_source->V_PIXELS;

    int i_x, i_y;

    for( i_y = (p_filter->fmt_in.video.i_y_offset + p_filter->fmt_in.video.i_visible_height) ; i_y-- ; )
    {
        for( i_x = (p_filter->fmt_in.video.i_x_offset + p_filter->fmt_in.video.i_visible_width) / 8 ; i_x-- ; )
        {
            C_YUV422_Y211( p_line, p_y, p_u, p_v );
            C_YUV422_Y211( p_line, p_y, p_u, p_v );
        }
    }
}
#endif
