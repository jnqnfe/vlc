/*****************************************************************************
 * avio.h: access using libavformat library
 *****************************************************************************
 * Copyright (C) 2009 Laurent Aimar
 *
 * Authors: Laurent Aimar <fenrir _AT_ videolan _DOT_ org>
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

#include <libavformat/avformat.h>
#include <libavformat/avio.h>

int  OpenAvio (stream_t *);
void CloseAvio(stream_t *);
int  OutOpenAvio (sout_access_out_t *);
void OutCloseAvio(sout_access_out_t *);

#define AVIO_MODULE \
    set_shortname(N_("AVIO"))                                                    \
    set_description(N_("libavformat AVIO access") )                              \
    if (strcmp("avio", PLUGIN_STRING)) {                                         \
        add_shortcut("avio")                                                     \
    }                                                                            \
    add_shortcut("rtmp", "rtmpe", "rtmps", "rtmpt", "rtmpte", "rtmpts")          \
    set_capability(VLC_CAP_ACCESS, -1, OpenAvio, CloseAvio)                      \
                                                                                 \
    add_submodule ()                                                             \
        set_shortname( "AVIO" )                                                  \
        set_description( N_("libavformat AVIO access output") )                  \
        if (strcmp("avio", PLUGIN_STRING)) {                                     \
            add_shortcut("avio")                                                 \
        }                                                                        \
        add_shortcut( "rtmp" )                                                   \
        set_capability( VLC_CAP_SOUT_ACCESS, -1, OutOpenAvio, OutCloseAvio )     \
                                                                                 \
    set_subcategory(SUBCAT_INPUT_ACCESS)                                         \
    set_section(N_("Input"), NULL )                                              \
    add_string("avio-options", NULL, AV_OPTIONS_TEXT, AV_OPTIONS_LONGTEXT, true) \
                                                                                 \
    set_subcategory( SUBCAT_SOUT_ACO )                                           \
    set_section(N_("Stream output"), NULL )                                      \
    add_string("sout-avio-options", NULL, AV_OPTIONS_TEXT, AV_OPTIONS_LONGTEXT, true)
