/*****************************************************************************
 * sdp.c: Fake input for sdp:// scheme
 *****************************************************************************
 * Copyright (C) 2010 Rémi Denis-Courmont
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
# include "config.h"
#endif

#include <limits.h>
#include <string.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_access.h>

static int Open (stream_t *);

vlc_plugin_begin ()
    set_shortname ("SDP")
    set_description (N_("Session Description Protocol"))
    set_capability (VLC_CAP_ACCESS, 0, Open, NULL)
    //set_subcategory (SUBCAT_INPUT_ACCESS)
vlc_plugin_end()

static ssize_t Read (stream_t *, void *, size_t);
static int Seek (stream_t *, uint64_t);
static int Control (stream_t *, int, va_list);

static int Open (stream_t *access)
{
    access->pf_read = Read;
    access->pf_block = NULL;
    access->pf_seek = Seek;
    access->pf_control = Control;
    access->p_sys = (char *)access->psz_location;

    return VLC_SUCCESS;
}

static ssize_t Read (stream_t *access, void *buf, size_t len)
{
    char *in = access->p_sys, *out = buf;
    size_t i;

    for (i = 0; i < len && *in != '\0'; i++)
        *(out++) = *(in++);

    access->p_sys = in;
    return i;
}

static int Seek (stream_t *access, uint64_t position)
{
#if (UINT64_MAX > SIZE_MAX)
    if (unlikely(position > SIZE_MAX))
        position = SIZE_MAX;
#endif
    access->p_sys = (char *)access->psz_location
                    + strnlen(access->psz_location, position);
    return VLC_SUCCESS;
}

static int Control (stream_t *access, int query, va_list args)
{
    switch (query)
    {
        case STREAM_CAN_SEEK:
        case STREAM_CAN_FASTSEEK:
        case STREAM_CAN_PAUSE:
        case STREAM_CAN_CONTROL_PACE:
        {
            bool *b = va_arg(args, bool *);
            *b = true;
            return VLC_SUCCESS;
        }

        case STREAM_GET_SIZE:
            *va_arg(args, uint64_t *) = strlen(access->psz_location);
            return VLC_SUCCESS;

        case STREAM_GET_PTS_DELAY:
            *va_arg(args, vlc_tick_t *) =  DEFAULT_PTS_DELAY;
            return VLC_SUCCESS;
    
        case STREAM_SET_PAUSE_STATE:
            return VLC_SUCCESS;
    }
    return VLC_EGENERIC;
}
