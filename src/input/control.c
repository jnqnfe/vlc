/*****************************************************************************
 * control.c
 *****************************************************************************
 * Copyright (C) 1999-2004 the VideoLAN team
 * $Id$
 *
 * Authors: Gildas Bazin <gbazin@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>

#include <stdio.h>
#include <stdlib.h>

#include "input_internal.h"
#include "event.h"


static void UpdateBookmarksOption( input_thread_t * );

/****************************************************************************
 * input_Control
 ****************************************************************************/
/**
 * Control function for inputs.
 * \param p_input input handle
 * \param i_query query type
 * \return VLC_SUCCESS if ok
 */
int input_Control( input_thread_t *p_input, int i_query, ...  )
{
    va_list args;
    int     i_result;

    va_start( args, i_query );
    i_result = input_vaControl( p_input, i_query, args );
    va_end( args );

    return i_result;
}

int input_vaControl( input_thread_t *p_input, int i_query, va_list args )
{
    seekpoint_t *p_bkmk, ***ppp_bkmk;
    int i_bkmk = 0;
    int *pi_bkmk;

    int i_int, *pi_int;
    bool b_bool, *pb_bool;
    double f, *pf;
    int64_t i_64, *pi_64;

    char *psz;
    vlc_value_t val;

    switch( i_query )
    {
        case INPUT_GET_POSITION:
            pf = (double*)va_arg( args, double * );
            *pf = var_GetFloat( p_input, "position" );
            return VLC_SUCCESS;

        case INPUT_SET_POSITION:
            f = (double)va_arg( args, double );
            return var_SetFloat( p_input, "position", f );

        case INPUT_GET_LENGTH:
            pi_64 = (int64_t*)va_arg( args, int64_t * );
            *pi_64 = var_GetTime( p_input, "length" );
            return VLC_SUCCESS;

        case INPUT_GET_TIME:
            pi_64 = (int64_t*)va_arg( args, int64_t * );
            *pi_64 = var_GetTime( p_input, "time" );
            return VLC_SUCCESS;

        case INPUT_SET_TIME:
            i_64 = (int64_t)va_arg( args, int64_t );
            return var_SetTime( p_input, "time", i_64 );

        case INPUT_GET_RATE:
            pi_int = (int*)va_arg( args, int * );
            *pi_int = var_GetInteger( p_input, "rate" );
            return VLC_SUCCESS;

        case INPUT_SET_RATE:
            i_int = (int)va_arg( args, int );
            return var_SetInteger( p_input, "rate", i_int );

        case INPUT_GET_STATE:
            pi_int = (int*)va_arg( args, int * );
            *pi_int = var_GetInteger( p_input, "state" );
            return VLC_SUCCESS;

        case INPUT_SET_STATE:
            i_int = (int)va_arg( args, int );
            return var_SetInteger( p_input, "state", i_int );

        case INPUT_GET_AUDIO_DELAY:
            pi_64 = (int64_t*)va_arg( args, int64_t * );
            *pi_64 = var_GetTime( p_input, "audio-delay" );
            return VLC_SUCCESS;

        case INPUT_GET_SPU_DELAY:
            pi_64 = (int64_t*)va_arg( args, int64_t * );
            *pi_64 = var_GetTime( p_input, "spu-delay" );
            return VLC_SUCCESS;

        case INPUT_SET_AUDIO_DELAY:
            i_64 = (int64_t)va_arg( args, int64_t );
            return var_SetTime( p_input, "audio-delay", i_64 );

        case INPUT_SET_SPU_DELAY:
            i_64 = (int64_t)va_arg( args, int64_t );
            return var_SetTime( p_input, "spu-delay", i_64 );

        case INPUT_ADD_INFO:
        {
            char *psz_cat = (char *)va_arg( args, char * );
            char *psz_name = (char *)va_arg( args, char * );
            char *psz_format = (char *)va_arg( args, char * );

            char *psz_value;
            
            if( vasprintf( &psz_value, psz_format, args ) == -1 )
                return VLC_EGENERIC;

            int i_ret = input_item_AddInfo( p_input->p->p_item,
                                            psz_cat, psz_name, "%s", psz_value );
            free( psz_value );

            if( !p_input->b_preparsing && !i_ret )
                input_SendEventMetaInfo( p_input );
            return i_ret;
        }
        case INPUT_DEL_INFO:
        {
            char *psz_cat = (char *)va_arg( args, char * );
            char *psz_name = (char *)va_arg( args, char * );

            int i_ret = input_item_DelInfo( p_input->p->p_item,
                                            psz_cat, psz_name );

            if( !p_input->b_preparsing && !i_ret )
                input_SendEventMetaInfo( p_input );
            return i_ret;
        }
        case INPUT_GET_INFO:
        {
            char *psz_cat = (char *)va_arg( args, char * );
            char *psz_name = (char *)va_arg( args, char * );
            char **ppsz_value = (char **)va_arg( args, char ** );
            int i_ret = VLC_EGENERIC;
            *ppsz_value = NULL;

            *ppsz_value = input_item_GetInfo( p_input->p->p_item,
                                                  psz_cat, psz_name );
            return i_ret;
        }

        case INPUT_SET_NAME:
        {
            char *psz_name = (char *)va_arg( args, char * );

            if( !psz_name ) return VLC_EGENERIC;

            input_item_SetName( p_input->p->p_item, psz_name );

            if( !p_input->b_preparsing )
                input_SendEventMetaName( p_input, psz_name );
            return VLC_SUCCESS;
        }

        case INPUT_ADD_BOOKMARK:
            p_bkmk = (seekpoint_t *)va_arg( args, seekpoint_t * );
            p_bkmk = vlc_seekpoint_Duplicate( p_bkmk );

            vlc_mutex_lock( &p_input->p->p_item->lock );
            if( !p_bkmk->psz_name )
            {
                 if( asprintf( &p_bkmk->psz_name, _("Bookmark %i"),
                               p_input->p->i_bookmark ) == -1 )
                     p_bkmk->psz_name = NULL;
            }

            TAB_APPEND( p_input->p->i_bookmark, p_input->p->pp_bookmark, p_bkmk );
            vlc_mutex_unlock( &p_input->p->p_item->lock );

            UpdateBookmarksOption( p_input );

            return VLC_SUCCESS;

        case INPUT_CHANGE_BOOKMARK:
            p_bkmk = (seekpoint_t *)va_arg( args, seekpoint_t * );
            i_bkmk = (int)va_arg( args, int );

            vlc_mutex_lock( &p_input->p->p_item->lock );
            if( i_bkmk < p_input->p->i_bookmark )
            {
                vlc_seekpoint_Delete( p_input->p->pp_bookmark[i_bkmk] );
                p_input->p->pp_bookmark[i_bkmk] = vlc_seekpoint_Duplicate( p_bkmk );
            }
            vlc_mutex_unlock( &p_input->p->p_item->lock );

            UpdateBookmarksOption( p_input );

            return VLC_SUCCESS;

        case INPUT_DEL_BOOKMARK:
            i_bkmk = (int)va_arg( args, int );

            vlc_mutex_lock( &p_input->p->p_item->lock );
            if( i_bkmk < p_input->p->i_bookmark )
            {
                p_bkmk = p_input->p->pp_bookmark[i_bkmk];
                TAB_REMOVE( p_input->p->i_bookmark, p_input->p->pp_bookmark, p_bkmk );
                vlc_seekpoint_Delete( p_bkmk );

                vlc_mutex_unlock( &p_input->p->p_item->lock );

                UpdateBookmarksOption( p_input );

                return VLC_SUCCESS;
            }
            vlc_mutex_unlock( &p_input->p->p_item->lock );

            return VLC_EGENERIC;

        case INPUT_GET_BOOKMARKS:
            ppp_bkmk = (seekpoint_t ***)va_arg( args, seekpoint_t *** );
            pi_bkmk = (int *)va_arg( args, int * );

            vlc_mutex_lock( &p_input->p->p_item->lock );
            if( p_input->p->i_bookmark )
            {
                int i;

                *pi_bkmk = p_input->p->i_bookmark;
                *ppp_bkmk = malloc( sizeof(seekpoint_t *) *
                                    p_input->p->i_bookmark );
                for( i = 0; i < p_input->p->i_bookmark; i++ )
                {
                    (*ppp_bkmk)[i] =
                        vlc_seekpoint_Duplicate( p_input->p->pp_bookmark[i] );
                }

                vlc_mutex_unlock( &p_input->p->p_item->lock );
                return VLC_SUCCESS;
            }
            else
            {
                *ppp_bkmk = NULL;
                *pi_bkmk = 0;

                vlc_mutex_unlock( &p_input->p->p_item->lock );
                return VLC_EGENERIC;
            }
            break;

        case INPUT_CLEAR_BOOKMARKS:

            vlc_mutex_lock( &p_input->p->p_item->lock );
            while( p_input->p->i_bookmark > 0 )
            {
                p_bkmk = p_input->p->pp_bookmark[p_input->p->i_bookmark-1];

                TAB_REMOVE( p_input->p->i_bookmark, p_input->p->pp_bookmark,
                            p_bkmk );
                vlc_seekpoint_Delete( p_bkmk );
            }
            vlc_mutex_unlock( &p_input->p->p_item->lock );

            UpdateBookmarksOption( p_input );
            return VLC_SUCCESS;

        case INPUT_SET_BOOKMARK:
            i_bkmk = (int)va_arg( args, int );

            val.i_int = i_bkmk;
            input_ControlPush( p_input, INPUT_CONTROL_SET_BOOKMARK, &val );

            return VLC_SUCCESS;

        case INPUT_GET_BOOKMARK:
            p_bkmk = (seekpoint_t *)va_arg( args, seekpoint_t * );

            vlc_mutex_lock( &p_input->p->p_item->lock );
            *p_bkmk = p_input->p->bookmark;
            vlc_mutex_unlock( &p_input->p->p_item->lock );
            return VLC_SUCCESS;

        case INPUT_ADD_OPTION:
        {
            const char *psz_option = va_arg( args, const char * );
            const char *psz_value = va_arg( args, const char * );
            char *str;
            int i;

            if( asprintf( &str, "%s=%s", psz_option, psz_value ) == -1 )
                return VLC_ENOMEM;

            i = input_item_AddOpt( p_input->p->p_item, str,
                                  VLC_INPUT_OPTION_UNIQUE );
            free( str );
            return i;
        }

        case INPUT_GET_VIDEO_FPS:
            pf = (double*)va_arg( args, double * );

            vlc_mutex_lock( &p_input->p->p_item->lock );
            *pf = p_input->p->f_fps;
            vlc_mutex_unlock( &p_input->p->p_item->lock );
            return VLC_SUCCESS;

        case INPUT_ADD_SLAVE:
            psz = (char*)va_arg( args, char * );
            if( psz && *psz )
            {
                val.psz_string = strdup( psz );
                input_ControlPush( p_input, INPUT_CONTROL_ADD_SLAVE, &val );
            }
            return VLC_SUCCESS;

        case INPUT_GET_ATTACHMENTS: /* arg1=input_attachment_t***, arg2=int*  res=can fail */
        {
            input_attachment_t ***ppp_attachment = (input_attachment_t***)va_arg( args, input_attachment_t *** );
            int *pi_attachment = (int*)va_arg( args, int * );
            int i;

            vlc_mutex_lock( &p_input->p->p_item->lock );
            if( p_input->p->i_attachment <= 0 )
            {
                vlc_mutex_unlock( &p_input->p->p_item->lock );
                *ppp_attachment = NULL;
                *pi_attachment = 0;
                return VLC_EGENERIC;
            }
            *pi_attachment = p_input->p->i_attachment;
            *ppp_attachment = malloc( sizeof(input_attachment_t**) * p_input->p->i_attachment );
            for( i = 0; i < p_input->p->i_attachment; i++ )
                (*ppp_attachment)[i] = vlc_input_attachment_Duplicate( p_input->p->attachment[i] );

            vlc_mutex_unlock( &p_input->p->p_item->lock );
            return VLC_SUCCESS;
        }

        case INPUT_GET_ATTACHMENT:  /* arg1=input_attachment_t**, arg2=char*  res=can fail */
        {
            input_attachment_t **pp_attachment = (input_attachment_t**)va_arg( args, input_attachment_t ** );
            const char *psz_name = (const char*)va_arg( args, const char * );
            int i;

            vlc_mutex_lock( &p_input->p->p_item->lock );
            for( i = 0; i < p_input->p->i_attachment; i++ )
            {
                if( !strcmp( p_input->p->attachment[i]->psz_name, psz_name ) )
                {
                    *pp_attachment = vlc_input_attachment_Duplicate( p_input->p->attachment[i] );
                    vlc_mutex_unlock( &p_input->p->p_item->lock );
                    return VLC_SUCCESS;
                }
            }
            *pp_attachment = NULL;
            vlc_mutex_unlock( &p_input->p->p_item->lock );
            return VLC_EGENERIC;
        }

        case INPUT_SET_RECORD_STATE:
            b_bool = (bool)va_arg( args, int );
            var_SetBool( p_input, "record", b_bool );
            return VLC_SUCCESS;

        case INPUT_GET_RECORD_STATE:
            pb_bool = (bool*)va_arg( args, bool* );
            *pb_bool = var_GetBool( p_input, "record" );
            return VLC_SUCCESS;

        case INPUT_RESTART_ES:
            val.i_int = (int)va_arg( args, int );
            input_ControlPush( p_input, INPUT_CONTROL_RESTART_ES, &val );
            return VLC_SUCCESS;

        default:
            msg_Err( p_input, "unknown query in input_vaControl" );
            return VLC_EGENERIC;
    }
}

static void UpdateBookmarksOption( input_thread_t *p_input )
{
    vlc_mutex_lock( &p_input->p->p_item->lock );

    /* Update the "bookmark" list */
    var_Change( p_input, "bookmark", VLC_VAR_CLEARCHOICES, 0, 0 );
    for( int i = 0; i < p_input->p->i_bookmark; i++ )
    {
        vlc_value_t val, text;

        val.i_int = i;
        text.psz_string = p_input->p->pp_bookmark[i]->psz_name;
        var_Change( p_input, "bookmark", VLC_VAR_ADDCHOICE,
                    &val, &text );
    }

    /* Create the "bookmarks" option value */
    const char *psz_format = "{name=%s,bytes=%"PRId64",time=%"PRId64"}";
    int i_len = strlen( "bookmarks=" );
    for( int i = 0; i < p_input->p->i_bookmark; i++ )
    {
        const seekpoint_t *p_bookmark = p_input->p->pp_bookmark[i];

        i_len += snprintf( NULL, 0, psz_format,
                           p_bookmark->psz_name,
                           p_bookmark->i_byte_offset,
                           p_bookmark->i_time_offset/1000000 );
    }

    char *psz_value = malloc( i_len + p_input->p->i_bookmark + 1 );
    char *psz_next = psz_value;

    psz_next += sprintf( psz_next, "bookmarks=" );
    for( int i = 0; i < p_input->p->i_bookmark && psz_value != NULL; i++ )
    {
        const seekpoint_t *p_bookmark = p_input->p->pp_bookmark[i];

        psz_next += sprintf( psz_next, psz_format,
                             p_bookmark->psz_name,
                             p_bookmark->i_byte_offset,
                             p_bookmark->i_time_offset/1000000 );

        if( i < p_input->p->i_bookmark - 1)
            *psz_next++ = ',';
    }
    vlc_mutex_unlock( &p_input->p->p_item->lock );

    if( psz_value )
        input_item_AddOpt( p_input->p->p_item, psz_value, VLC_INPUT_OPTION_UNIQUE );

    free( psz_value );

    input_SendEventBookmark( p_input );
}

