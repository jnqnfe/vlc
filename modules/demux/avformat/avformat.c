/*****************************************************************************
 * avformat.c: demuxer and muxer using libavformat library
 *****************************************************************************
 * Copyright (C) 1999-2008 VLC authors and VideoLAN
 *
 * Authors: Laurent Aimar <fenrir@via.ecp.fr>
 *          Gildas Bazin <gbazin@videolan.org>
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

#ifndef MERGE_FFMPEG
#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>

#include "avformat.h"
#include "../../codec/avcodec/avcommon.h"

vlc_plugin_begin ()
#endif /* MERGE_FFMPEG */
    add_shortcut( "ffmpeg" )
    set_shortname( "Avformat" )
    set_capability( VLC_CAP_DEMUX, 2, avformat_OpenDemux, avformat_CloseDemux )

#ifdef ENABLE_SOUT
    /* mux submodule */
    add_submodule ()
    add_shortcut( "ffmpeg" )
    set_capability( VLC_CAP_SOUT_MUX, 2, avformat_OpenMux, avformat_CloseMux )
#endif

    set_subcategory( SUBCAT_INPUT_DEMUX )
    set_section( N_("Demuxer"), NULL )
    add_string( "avformat-format", NULL, FORMAT_TEXT, FORMAT_LONGTEXT, true )
    add_string( "avformat-options", NULL, AV_OPTIONS_TEXT, AV_OPTIONS_LONGTEXT, true )
#ifdef ENABLE_SOUT
    set_section( N_("Muxer"), NULL )
    add_string( "sout-avformat-mux", NULL, MUX_TEXT, MUX_LONGTEXT, true )
    add_string( "sout-avformat-options", NULL, AV_OPTIONS_TEXT, AV_OPTIONS_LONGTEXT, true )
    add_bool( "sout-avformat-reset-ts", false, AV_RESET_TS_TEXT, AV_RESET_TS_LONGTEXT, true )
#endif

#ifndef MERGE_FFMPEG
vlc_plugin_end ()
#endif
