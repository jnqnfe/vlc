/*****************************************************************************
 * h26x.c : raw H264 and HEVC Video demuxers
 *****************************************************************************
 * Copyright (C) 2002-2004, 2014-2015 VLC authors and VideoLAN
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Denis Charmet <typx@videolan.org>
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
#include <vlc_demux.h>
#include <vlc_codec.h>
#include <float.h>
#ifdef PLUGIN_NAME_IS_hevc
#include "../packetizer/hevc_nal.h" /* definitions, inline helpers */
#else
#include "../packetizer/h264_nal.h" /* definitions, inline helpers */
#endif

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Open ( demux_t * );
static void Close( demux_t * );

#define FPS_TEXT N_("Frames per second")
#define FPS_LONGTEXT N_("Desired frame rate for the stream. (Use 0.0 to request default).")

#ifdef PLUGIN_NAME_IS_hevc
# define MOD_NAME "hevc"
#else
# define MOD_NAME "h264"
#endif

#define MOD_CFG_PREFIX MOD_NAME"-"

vlc_plugin_begin ()
#ifdef PLUGIN_NAME_IS_hevc
    set_shortname( "HEVC" )
    set_description( "HEVC/H.265" )
    add_shortcut( "h265" )
#else
    set_shortname( "H264")
#endif
    set_capability( VLC_CAP_DEMUX, 6, Open, Close )

    set_subcategory( SUBCAT_INPUT_DEMUX )
    add_float_with_range( MOD_CFG_PREFIX "fps", 0.0, 0.0, FLT_MAX, FPS_TEXT, FPS_LONGTEXT, true )
vlc_plugin_end ()

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
typedef struct
{
    es_out_id_t *p_es;

    date_t      dts;
    unsigned    frame_rate_num;
    unsigned    frame_rate_den;

    decoder_t *p_packetizer;
} demux_sys_t;

static int Demux( demux_t * );
static int Control( demux_t *, int, va_list );

#define H26X_PACKET_SIZE 2048
#define H26X_PEEK_CHUNK  (H26X_PACKET_SIZE * 4)
#define H26X_MIN_PEEK    (4 + 7 + 10)
#define H26X_MAX_PEEK    (H26X_PEEK_CHUNK * 8) /* max data to check */
#define H26X_NAL_COUNT   8 /* max # or NAL to check */

/*****************************************************************************
 * Probing
 *****************************************************************************/
typedef struct
{
    bool b_sps;
    bool b_pps;
#ifdef PLUGIN_NAME_IS_hevc
    bool b_vps;
#endif
} probe_ctx_t;

#ifdef PLUGIN_NAME_IS_hevc
static int Probe( const uint8_t *p_peek, size_t i_peek, void *p_priv )
{
    probe_ctx_t *p_ctx = (probe_ctx_t *) p_priv;

    if( i_peek < 2 )
        return -1;

    if( p_peek[0] & 0x80 )
        return -1;

    const uint8_t i_type = hevc_getNALType( p_peek );
    const uint8_t i_layer = hevc_getNALLayer( p_peek );

   if ( i_type == HEVC_NAL_VPS ) /* VPS */
   {
       if( i_layer != 0 || i_peek < 6 ||
           p_peek[4] != 0xFF || p_peek[5] != 0xFF ) /* Check reserved bits */
           return -1;
       p_ctx->b_vps = true;
       return 0;
   }
   else if( i_type == HEVC_NAL_SPS )  /* SPS */
   {
       if( i_layer != 0 )
           return -1;
       p_ctx->b_sps = true;
       return 0;
   }
   else if( i_type == HEVC_NAL_PPS )  /* PPS */
   {
       if( i_layer != 0 )
           return -1;
       p_ctx->b_pps = true;
       return 0;
   }
   else if( i_type >= HEVC_NAL_BLA_W_LP && i_type <= HEVC_NAL_CRA ) /* Key Frame */
   {
        if( p_ctx->b_vps && p_ctx->b_sps && p_ctx->b_pps && i_layer == 0 )
            return 1;
   }
   else if( i_type == HEVC_NAL_AUD ) /* AU */
   {
        if( i_peek < H26X_MIN_PEEK ||
            p_peek[4] != 0 || p_peek[5] != 0 ) /* Must prefix another NAL */
            return -1;
   }
   else if( i_type == HEVC_NAL_PREF_SEI ) /* Prefix SEI */
   {
       if( p_peek[2] == 0xFF ) /* empty SEI */
           return -1;
   }
   else
   {
       return -1; /* See 7.4.2.4.4 for sequence order */
   }

    return 0; /* Probe more */
}
#endif

#ifdef PLUGIN_NAME_IS_h264
static int Probe( const uint8_t *p_peek, size_t i_peek, void *p_priv )
{
    probe_ctx_t *p_ctx = (probe_ctx_t *) p_priv;

    if( i_peek < 1 )
        return -1;
    const uint8_t i_nal_type = p_peek[0] & 0x1F;
    const uint8_t i_ref_idc = p_peek[0] & 0x60;

    if( (p_peek[0] & 0x80) ) /* reserved 0 */
        return -1;

    /* ( !i_ref_idc && (i_nal_type < 6 || i_nal_type == 7 || i_nal_type == 8) ) ||
       (  i_ref_idc && (i_nal_type == 6 || i_nal_type >= 9) ) */

    if( i_nal_type == H264_NAL_SPS )
    {
        if( i_ref_idc == 0 || i_peek < 3 ||
           (p_peek[2] & 0x03) /* reserved 0 bits */ )
            return -1;
        p_ctx->b_sps = true;
    }
    else if( i_nal_type == H264_NAL_PPS )
    {
        if( i_ref_idc == 0 )
            return -1;
        p_ctx->b_pps = true;
    }
    else if( i_nal_type == H264_NAL_SLICE_IDR )
    {
        if( i_ref_idc == 0 || ! p_ctx->b_pps || ! p_ctx->b_sps )
            return -1;
        else
            return 1;
    }
    else if( i_nal_type == H264_NAL_AU_DELIMITER )
    {
        if( i_ref_idc || p_ctx->b_pps || p_ctx->b_sps )
            return -1;
    }
    else if ( i_nal_type == H264_NAL_SEI )
    {
        if( i_ref_idc )
            return -1;
    }
     /* 7.4.1.1 */
    else if ( i_nal_type == H264_NAL_SPS_EXT ||
              i_nal_type == H264_NAL_SUBSET_SPS )
    {
        if( i_ref_idc == 0 || !p_ctx->b_sps )
            return -1;
    }
    else if( i_nal_type == H264_NAL_PREFIX )
    {
        if( i_ref_idc == 0 || !p_ctx->b_pps || !p_ctx->b_sps )
            return -1;
    }
    else return -1; /* see 7.4.1.2.3 for sequence */

    return 0;
}
#endif

/*****************************************************************************
 * Shared Open code
 *****************************************************************************/
static inline bool check_Property( demux_t *p_demux, const char **pp_psz,
                                   bool(*pf_check)(demux_t *, const char *) )
{
    while( *pp_psz )
    {
        if( pf_check( p_demux, *pp_psz ) )
            return true;
        pp_psz++;
    }
    return false;
}

static int Open( demux_t *p_demux )
{
#ifdef PLUGIN_NAME_IS_hevc
    vlc_fourcc_t i_codec = VLC_CODEC_HEVC;
    probe_ctx_t ctx = { 0, 0, 0 };
    const char *rgi_psz_ext[] = { ".h265", ".265", ".hevc", ".bin", ".bit", ".raw", NULL };
    const char *rgi_psz_mime[] = { "video/h265", "video/hevc", "video/HEVC", NULL };
#else
    vlc_fourcc_t i_codec = VLC_CODEC_H264;
    probe_ctx_t ctx = { 0, 0 };
    const char *rgi_psz_ext[] = { ".h264", ".264", ".bin", ".bit", ".raw", NULL };
    const char *rgi_psz_mime[] = { "video/H264", "video/h264", "video/avc", NULL };
#endif

    demux_sys_t *p_sys;
    const uint8_t *p_peek;
    es_format_t fmt;
    uint8_t annexb_startcode[] = {0,0,0,1};
    int i_ret = 0;

    /* Restrict by type first */
    if( !p_demux->obj.force &&
        !check_Property( p_demux, rgi_psz_ext, demux_IsPathExtension ) &&
        !check_Property( p_demux, rgi_psz_mime, demux_IsContentType ) )
    {
        return VLC_EGENERIC;
    }

    /* First check for first AnnexB header */
    if( vlc_stream_Peek( p_demux->s, &p_peek, H26X_MIN_PEEK ) == H26X_MIN_PEEK &&
       !memcmp( p_peek, annexb_startcode, 4 ) )
    {
        size_t i_peek = H26X_MIN_PEEK;
        size_t i_peek_target = H26X_MIN_PEEK;
        size_t i_probe_offset = 4;
        const uint8_t *p_probe = p_peek;
        bool b_synced = true;
        unsigned i_bitflow = 0;

        for( unsigned i=0; i<H26X_NAL_COUNT; i++ )
        {
            while( !b_synced )
            {
                if( i_probe_offset + H26X_MIN_PEEK >= i_peek &&
                    i_peek_target + H26X_PEEK_CHUNK <= H26X_MAX_PEEK )
                {
                    i_peek_target += H26X_PEEK_CHUNK;
                    i_peek = vlc_stream_Peek( p_demux->s, &p_peek, i_peek_target );
                }

                if( i_probe_offset + H26X_MIN_PEEK >= i_peek )
                    break;

                p_probe = &p_peek[i_probe_offset];
                i_bitflow = (i_bitflow << 1) | (!p_probe[0]);
                /* Check for annexB */
                if( p_probe[0] == 0x01 && ((i_bitflow & 0x06) == 0x06) )
                    b_synced = true;

                i_probe_offset++;
            }

            if( b_synced )
            {
                p_probe = &p_peek[i_probe_offset];
                i_ret = Probe( p_probe, i_peek - i_probe_offset, &ctx );
            }

            if( i_ret != 0 )
                break;

            i_probe_offset += 4;
            b_synced = false;
        }
    }

    if( i_ret < 1 )
    {
        if( !p_demux->obj.force )
        {
            msg_Warn( p_demux, MOD_NAME " module discarded (no startcode)" );
            return VLC_EGENERIC;
        }

        msg_Err( p_demux, "this doesn't look like a "MOD_NAME" ES stream, "
                 "continuing anyway" );
    }

    p_demux->pf_demux  = Demux;
    p_demux->pf_control= Control;
    p_demux->p_sys     = p_sys = malloc( sizeof( demux_sys_t ) );
    p_sys->p_es        = NULL;
    p_sys->frame_rate_num = 0;
    p_sys->frame_rate_den = 0;

    float f_fps = var_CreateGetFloat( p_demux, MOD_CFG_PREFIX "fps" );
    if( f_fps )
    {
        if ( f_fps < 0.001f ) f_fps = 0.001f;
        p_sys->frame_rate_den = 1000;
        p_sys->frame_rate_num = 1000 * f_fps;
        date_Init( &p_sys->dts, p_sys->frame_rate_num, p_sys->frame_rate_den );
    }
    else
        date_Init( &p_sys->dts, 25000, 1000 );
    date_Set( &p_sys->dts, VLC_TICK_0 );

    /* Load the mpegvideo packetizer */
    es_format_Init( &fmt, VIDEO_ES, i_codec );
    if( f_fps )
    {
        fmt.video.i_frame_rate = p_sys->dts.i_divider_num;
        fmt.video.i_frame_rate_base = p_sys->dts.i_divider_den;
    }
    p_sys->p_packetizer = demux_PacketizerNew( p_demux, &fmt, MOD_NAME );
    if( !p_sys->p_packetizer )
    {
        free( p_sys );
        return VLC_EGENERIC;
    }

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Close: frees unused data
 *****************************************************************************/
static void Close( demux_t *p_demux )
{
    demux_sys_t *p_sys = p_demux->p_sys;

    demux_PacketizerDestroy( p_sys->p_packetizer );
    free( p_sys );
}

/*****************************************************************************
 * Demux: reads and demuxes data packets
 *****************************************************************************
 * Returns -1 in case of error, 0 in case of EOF, 1 otherwise
 *****************************************************************************/
static int Demux( demux_t *p_demux)
{
    demux_sys_t *p_sys = p_demux->p_sys;
    block_t *p_block_in, *p_block_out;
    bool b_eof = false;

    p_block_in = vlc_stream_Block( p_demux->s, H26X_PACKET_SIZE );
    if( p_block_in == NULL )
    {
        b_eof = true;
    }
    else
    {
        p_block_in->i_dts = date_Get( &p_sys->dts );
    }

    while( (p_block_out = p_sys->p_packetizer->pf_packetize( p_sys->p_packetizer,
                                                             p_block_in ? &p_block_in : NULL )) )
    {
        while( p_block_out )
        {
            block_t *p_next = p_block_out->p_next;

            p_block_out->p_next = NULL;

            if( p_block_in )
            {
                p_block_in->i_dts = date_Get( &p_sys->dts );
                p_block_in->i_pts = VLC_TICK_INVALID;
            }

            if( p_sys->p_es == NULL )
            {
                p_sys->p_packetizer->fmt_out.b_packetized = true;
                p_sys->p_es = es_out_Add( p_demux->out, &p_sys->p_packetizer->fmt_out );
                if( !p_sys->p_es )
                {
                    block_ChainRelease( p_block_out );
                    return VLC_DEMUXER_EOF;
                }
            }

            /* h264 packetizer does merge multiple NAL into AU, but slice flag persists */
            bool frame = p_block_out->i_flags & BLOCK_FLAG_TYPE_MASK;
            const vlc_tick_t i_frame_dts = p_block_out->i_dts;
            const vlc_tick_t i_frame_length = p_block_out->i_length;
            es_out_Send( p_demux->out, p_sys->p_es, p_block_out );
            if( frame )
            {
                if( !p_sys->frame_rate_den )
                {
                    /* Use packetizer's one */
                    if( p_sys->p_packetizer->fmt_out.video.i_frame_rate_base &&
                        p_sys->p_packetizer->fmt_out.video.i_frame_rate )
                    {
                        p_sys->frame_rate_num = p_sys->p_packetizer->fmt_out.video.i_frame_rate;
                        p_sys->frame_rate_den = p_sys->p_packetizer->fmt_out.video.i_frame_rate_base;
                    }
                    else
                    {
                        p_sys->frame_rate_num = 25000;
                        p_sys->frame_rate_den = 1000;
                    }
                    date_Init( &p_sys->dts, 2 * p_sys->frame_rate_num, p_sys->frame_rate_den );
                    date_Set( &p_sys->dts, VLC_TICK_0 );
                    msg_Dbg( p_demux, "using %.2f fps", (double) p_sys->frame_rate_num / p_sys->frame_rate_den );
                }

                es_out_SetPCR( p_demux->out, date_Get( &p_sys->dts ) );
                unsigned i_nb_fields;
                if( i_frame_length > 0 )
                {
                    i_nb_fields = round( (double)i_frame_length * 2 * p_sys->frame_rate_num /
                                  ( p_sys->frame_rate_den * CLOCK_FREQ ) );
                }
                else i_nb_fields = 2;
                if( i_nb_fields <= 6 ) /* in the legit range */
                    date_Increment( &p_sys->dts, i_nb_fields );
                else /* Somehow some discontinuity */
                    date_Set( &p_sys->dts, i_frame_dts );
            }

            p_block_out = p_next;
        }
    }
    return (b_eof) ? VLC_DEMUXER_EOF : VLC_DEMUXER_SUCCESS;
}

/*****************************************************************************
 * Control:
 *****************************************************************************/
static int Control( demux_t *p_demux, int i_query, va_list args )
{
    /* demux_sys_t *p_sys  = p_demux->p_sys; */
    /* FIXME calculate the bitrate */
    if( i_query == DEMUX_SET_TIME )
        return VLC_EGENERIC;
    else
        return demux_vaControlHelper( p_demux->s,
                                       0, -1,
                                       0, 1, i_query, args );
}

