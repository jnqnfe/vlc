/*****************************************************************************
 * playlist.h:  Playlist import module common functions
 *****************************************************************************
 * Copyright (C) 2004 VLC authors and VideoLAN
 *
 * Authors: Sigmund Augdal Helberg <dnumgis@videolan.org>
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

#include <vlc_input.h>

char *ProcessMRL( const char *, const char * );

int Import_M3U ( stream_t * );

int Import_RAM ( stream_t * );

int Import_PLS ( stream_t * );

int Import_B4S ( stream_t * );

int Import_DVB ( stream_t * );

int Import_podcast ( stream_t * );

int Import_xspf ( stream_t * );
void Close_xspf ( stream_t * );

int Import_Shoutcast ( stream_t * );

int Import_ASX ( stream_t * );

int Import_SGIMB ( stream_t * );
void Close_SGIMB ( stream_t * );

int Import_QTL ( stream_t * );

int Import_IFO ( stream_t * );
void Close_IFO ( stream_t * );

int Import_BDMV ( stream_t * );
void Close_BDMV ( stream_t * );

int Import_iTML ( stream_t * );

int Import_WMS( stream_t * );

int Import_WPL ( stream_t * );
void Close_WPL ( stream_t * );

#define GetCurrentItem(obj) ((obj)->p_input_item)
#define GetSource(obj) ((obj)->s)

#define CHECK_FILE(obj) \
do { \
    if( GetSource(obj)->pf_readdir != NULL ) \
        return VLC_EGENERIC; \
} while(0)
