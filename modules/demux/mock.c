/*****************************************************************************
 * mock.c : mock demux module for vlc
 *****************************************************************************
 * Copyright (C) 2018 VLC authors and VideoLAN
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

#include <ctype.h>
#include <limits.h>

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_aout.h>
#include <vlc_picture.h>
#include <vlc_demux.h>
#include <vlc_input.h>
#include <vlc_vector.h>


struct mock_track
{
    es_format_t fmt;
    es_out_id_t *id;
};
typedef struct VLC_VECTOR(struct mock_track *) mock_track_vector;

static vlc_fourcc_t
var_InheritFourcc(vlc_object_t *obj, const char *name)
{
    char *var_value = var_InheritString(obj, name);
    if (!var_value)
        return 0;

    size_t var_len = strlen(var_value);
    if (var_len > 4)
    {
        free(var_value);
        return 0;
    }

    /* Pad with spaces if the string len is less than 4 */
    char value[] = "    ";
    strcpy(value, var_value);
    if (var_len != 4)
        value[var_len] = ' ';
    free(var_value);

    vlc_fourcc_t fourcc;
    memcpy(&fourcc, value, 4);
    return fourcc;
}
#define var_InheritFourcc(o, n) var_InheritFourcc(VLC_OBJECT(o), n)

/* X:  var_name, default_value,           type, module_header_type, getter */
/* XR: var_name, default_value, min, max, type, module_header_type, getter */
#define LIST_OPTIONS \
    XR( length,                VLC_TICK_FROM_MS(5000), 0, INT_MAX, vlc_tick_t, add_integer_with_range, var_InheritInteger ) \
    XR( audio_track_count,     0, 0, INT_MAX,    ssize_t,      add_integer_with_range, var_InheritInteger ) \
    XR( audio_channels,        2, 1, AOUT_CHAN_MAX, unsigned,  add_integer_with_range, (unsigned)var_InheritInteger ) \
    X(  audio_format,          "u8",             vlc_fourcc_t, add_string,             var_InheritFourcc ) \
    XR( audio_rate,            44100, 1, INT_MAX-1, unsigned,  add_integer_with_range, (unsigned)var_InheritInteger ) \
    X(  audio_packetized,      true,             bool,         add_bool,               var_InheritBool ) \
    XR( video_track_count,     0, 0, INT_MAX,    ssize_t,      add_integer_with_range, var_InheritInteger ) \
    X(  video_chroma,          "I420",           vlc_fourcc_t, add_string,             var_InheritFourcc ) \
    XR( video_width,           640, 0, INT_MAX,  unsigned,     add_integer_with_range, (unsigned)var_InheritInteger ) \
    XR( video_height,          480, 0, INT_MAX,  unsigned,     add_integer_with_range, (unsigned)var_InheritInteger ) \
    XR( video_frame_rate,      25, 0, INT_MAX-1, unsigned,     add_integer_with_range, (unsigned)var_InheritInteger ) \
    XR( video_frame_rate_base, 1, 0, INT_MAX-1,  unsigned,     add_integer_with_range, (unsigned)var_InheritInteger ) \
    X(  video_packetized,      true,             bool,         add_bool,               var_InheritBool ) \
    XR( sub_track_count,       0, 0, INT_MAX,    ssize_t,      add_integer_with_range, var_InheritInteger ) \
    X(  sub_packetized,        true,             bool,         add_bool,               var_InheritBool ) \
    XR( title_count,           0, 0, INT_MAX,    ssize_t,      add_integer_with_range, var_InheritInteger ) \
    XR( chapter_count,         0, 0, INT_MAX,    ssize_t,      add_integer_with_range, var_InheritInteger ) \
    X(  null_names,            false,            bool,         add_bool,               var_InheritBool ) \
    XR( program_count,         0, 0, INT_MAX,    ssize_t,      add_integer_with_range, var_InheritInteger ) \
    X(  can_seek,              true,             bool,         add_bool,               var_InheritBool ) \
    X(  can_pause,             true,             bool,         add_bool,               var_InheritBool ) \
    X(  can_control_pace,      true,             bool,         add_bool,               var_InheritBool ) \
    X(  can_control_rate,      true,             bool,         add_bool,               var_InheritBool ) \
    X(  can_record,            true,             bool,         add_bool,               var_InheritBool ) \
    X(  error,                 false,            bool,         add_bool,               var_InheritBool ) \
    X(  add_video_track_at,    VLC_TICK_INVALID, vlc_tick_t,   add_integer,            var_InheritInteger ) \
    X(  add_audio_track_at,    VLC_TICK_INVALID, vlc_tick_t,   add_integer,            var_InheritInteger ) \
    X(  add_spu_track_at,      VLC_TICK_INVALID, vlc_tick_t,   add_integer,            var_InheritInteger )

struct demux_sys
{
    mock_track_vector tracks;
    vlc_tick_t pts;
    vlc_tick_t step_length;

    int current_title;
    size_t chapter_gap;

#define XR(var_name, default_value, min, max, type, module_header_type, getter) \
    X(var_name, default_value, type, module_header_type, getter)
#define X(var_name, default_value, type, module_header_type, getter) \
    type var_name;
    LIST_OPTIONS
#undef XR
#undef X
};

static input_title_t *
CreateTitle(demux_t *demux, size_t idx)
{
    struct demux_sys *sys = demux->p_sys;

    input_title_t *t = vlc_input_title_New();
    if (!t)
        return NULL;

    t->i_length = sys->length;
    if (!sys->null_names
     && asprintf(&t->psz_name, "Mock Title %zu", idx) == -1)
    {
        t->psz_name = NULL;
        vlc_input_title_Delete(t);
        return NULL;
    }
    t->seekpoint = vlc_alloc(sys->chapter_count, sizeof(*t->seekpoint));
    if (!t->seekpoint)
    {
        vlc_input_title_Delete(t);
        return NULL;
    }

    for (ssize_t i = 0; i < sys->chapter_count; ++i)
    {
        t->seekpoint[i] = vlc_seekpoint_New();
        if (!t->seekpoint[i])
        {
            vlc_input_title_Delete(t);
            return NULL;
        }
        t->i_seekpoint++;
        if (!sys->null_names
         && asprintf(&t->seekpoint[i]->psz_name, "Mock Chapter %zu-%zu", idx, i)
            == -1)
        {
            vlc_input_title_Delete(t);
            return NULL;
        }
        t->seekpoint[i]->i_time_offset = i * sys->chapter_gap;
    }
    return t;
}

static int
Control(demux_t *demux, int query, va_list args)
{
    struct demux_sys *sys = demux->p_sys;

    switch (query)
    {
        case DEMUX_CAN_SEEK:
            *va_arg(args, bool *) = sys->can_seek;
            return VLC_SUCCESS;
        case DEMUX_CAN_PAUSE:
            *va_arg(args, bool *) = sys->can_pause;
            return VLC_SUCCESS;
        case DEMUX_CAN_CONTROL_PACE:
            *va_arg(args, bool *) = sys->can_control_pace;
            return VLC_SUCCESS;
        case DEMUX_GET_PTS_DELAY:
            *va_arg(args, vlc_tick_t *) = 0;
            return VLC_SUCCESS;
        case DEMUX_GET_META:
            return VLC_EGENERIC;
        case DEMUX_GET_SIGNAL:
            return VLC_EGENERIC;
        case DEMUX_SET_PAUSE_STATE:
            return sys->can_pause ? VLC_SUCCESS : VLC_EGENERIC;
        case DEMUX_SET_TITLE:
            if (sys->title_count > 0)
            {
                int new_title = va_arg(args, int);
                if (new_title >= sys->title_count)
                    return VLC_EGENERIC;
                sys->current_title = new_title;
                sys->pts = VLC_TICK_0;
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;
        case DEMUX_SET_SEEKPOINT:
            if (sys->chapter_gap > 0)
            {
                const int seekpoint_idx = va_arg(args, int);
                if (seekpoint_idx < sys->chapter_count)
                {
                    sys->pts = seekpoint_idx * sys->chapter_gap;
                    return VLC_SUCCESS;
                }
            }
            return VLC_EGENERIC;
        case DEMUX_TEST_AND_CLEAR_FLAGS:
            return VLC_EGENERIC;
        case DEMUX_GET_TITLE:
            if (sys->title_count > 0)
            {
                *va_arg(args, int *) = sys->current_title;
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;
        case DEMUX_GET_SEEKPOINT:
            if (sys->chapter_gap > 0)
            {
                *va_arg(args, int *) = sys->pts / sys->chapter_gap;
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;
        case DEMUX_GET_POSITION:
            *va_arg(args, double *) = sys->pts / (double) sys->length;
            return VLC_SUCCESS;
        case DEMUX_SET_POSITION:
            if (!sys->can_seek)
                return VLC_EGENERIC;
            sys->pts = va_arg(args, double) * sys->length;
            return VLC_SUCCESS;
        case DEMUX_GET_LENGTH:
            *va_arg(args, vlc_tick_t *) = sys->length;
            return VLC_SUCCESS;
        case DEMUX_GET_TIME:
            *va_arg(args, vlc_tick_t *) = sys->pts;
            return VLC_SUCCESS;
        case DEMUX_SET_TIME:
            if (!sys->can_seek)
                return VLC_EGENERIC;
            sys->pts = va_arg(args, vlc_tick_t);
            return VLC_SUCCESS;
        case DEMUX_GET_TITLE_INFO:
            if (sys->title_count > 0)
            {
                input_title_t ***titles = va_arg(args, input_title_t ***);
                *titles = vlc_alloc(sys->title_count, sizeof(*titles));
                if (!*titles)
                    return VLC_ENOMEM;
                for (ssize_t i = 0; i < sys->title_count; ++i)
                {
                    (*titles)[i] = CreateTitle(demux, i);
                    if (!(*titles)[i])
                    {
                        while (i--)
                            vlc_input_title_Delete((*titles)[i - 1]);
                        free(*titles);
                        *titles = NULL;
                        return VLC_ENOMEM;
                    }
                }
                *va_arg(args, int *) = (int)sys->title_count;
                *va_arg(args, int *) = 0;
                *va_arg(args, int *) = 0;
                return VLC_SUCCESS;
            }
            return VLC_EGENERIC;
        case DEMUX_SET_GROUP_DEFAULT:
            return VLC_EGENERIC;
        case DEMUX_SET_GROUP_ALL:
            return VLC_EGENERIC;
        case DEMUX_SET_GROUP_LIST:
            return VLC_EGENERIC;
        case DEMUX_SET_ES:
            return VLC_EGENERIC;
        case DEMUX_SET_ES_LIST:
            return VLC_EGENERIC;
        case DEMUX_SET_NEXT_DEMUX_TIME:
            return VLC_EGENERIC;
        case DEMUX_GET_FPS:
            return VLC_EGENERIC;
        case DEMUX_HAS_UNSUPPORTED_META:
            return VLC_EGENERIC;
        case DEMUX_GET_ATTACHMENTS:
            return VLC_EGENERIC;
        case DEMUX_CAN_RECORD:
            *va_arg(args, bool *) = sys->can_record;
            return VLC_SUCCESS;
        case DEMUX_SET_RECORD_STATE:
            return sys->can_record ? VLC_SUCCESS : VLC_EGENERIC;
        case DEMUX_CAN_CONTROL_RATE:
            *va_arg(args, bool *) = sys->can_control_rate;
            return VLC_SUCCESS;
        case DEMUX_SET_RATE:
            return sys->can_control_rate ? VLC_SUCCESS : VLC_EGENERIC;
        case DEMUX_IS_PLAYLIST:
            return VLC_EGENERIC;
        case DEMUX_NAV_ACTIVATE:
            return VLC_EGENERIC;
        case DEMUX_NAV_UP:
            return VLC_EGENERIC;
        case DEMUX_NAV_DOWN:
            return VLC_EGENERIC;
        case DEMUX_NAV_LEFT:
            return VLC_EGENERIC;
        case DEMUX_NAV_RIGHT:
            return VLC_EGENERIC;
        case DEMUX_NAV_POPUP:
            return VLC_EGENERIC;
        case DEMUX_NAV_MENU:
            return VLC_EGENERIC;
        default:
            return VLC_EGENERIC;
    }
}

static block_t *
CreateAudioBlock(demux_t *demux, struct mock_track *track)
{
    struct demux_sys *sys = demux->p_sys;
    const int64_t samples =
        samples_from_vlc_tick(sys->step_length, track->fmt.audio.i_rate);
    const int64_t bytes = samples / track->fmt.audio.i_frame_length
                        * track->fmt.audio.i_bytes_per_frame;
    block_t *b = block_Alloc(bytes);
    if (!b)
        return NULL;
    memset(b->p_buffer, 0, b->i_buffer);
    return b;
}

struct video_block
{
    block_t b;
    picture_t *pic;
};

static void
video_block_free_cb(block_t *b)
{
    struct video_block *video = container_of(b, struct video_block, b);
    picture_Release(video->pic);
    free(video);
}

static block_t *
CreateVideoBlock(demux_t *demux, struct mock_track *track)
{
    struct demux_sys *sys = demux->p_sys;
    picture_t *pic = picture_NewFromFormat(&track->fmt.video);
    if (!pic)
        return NULL;

    struct video_block *video = malloc(sizeof(*video));
    if (!video)
    {
        picture_Release(pic);
        return NULL;
    }
    video->pic = pic;

    static const struct vlc_block_callbacks cbs =
    {
        .free = video_block_free_cb
    };

    size_t block_len = 0;
    for (int i = 0; i < pic->i_planes; ++i)
        block_len += pic->p[i].i_lines * pic->p[i].i_pitch;
    memset(pic->p[0].p_pixels, (sys->pts / VLC_TICK_FROM_MS(10)) % 255,
           block_len);
    return block_Init(&video->b, &cbs, pic->p[0].p_pixels, block_len);
    (void) demux;
}

static block_t *
CreateSubBlock(demux_t *demux, struct mock_track *track)
{
    struct demux_sys *sys = demux->p_sys;
    char *text;
    if (asprintf(&text, "subtitle @ %"PRId64, sys->pts) == -1)
        return NULL;
    size_t len = strlen(text) + 1;

    block_t *b = block_Alloc(len);
    if (!b)
    {
        free(text);
        return NULL;
    }

    memcpy(b->p_buffer, text, len);
    b->i_buffer = len;

    free(text);
    return b;
    (void) track;
}

static int
AppendMockTrack(demux_t *demux, const es_format_t *fmt, int group,
                bool packetized)
{
    struct demux_sys *sys = demux->p_sys;
    struct mock_track *mock_track = malloc(sizeof(*mock_track));
    if (!mock_track)
        return VLC_EGENERIC;
    mock_track->fmt = *fmt;
    mock_track->fmt.i_group = group;
    mock_track->fmt.b_packetized = packetized;
    mock_track->id = es_out_Add(demux->out, & mock_track->fmt);
    if (!mock_track->id)
    {
        free(mock_track);
        return VLC_ENOMEM;
    }
    bool success = vlc_vector_push(&sys->tracks, mock_track);
    assert(success); (void) success; /* checked by reserve() */
    return VLC_SUCCESS;
}

static int
InitVideoTracks(demux_t *demux, int group, size_t count)
{
    struct demux_sys *sys = demux->p_sys;

    if (count == 0)
        return VLC_SUCCESS;

    for (size_t i = 0; i < count; ++i)
    {
        es_format_t fmt;
        es_format_Init(&fmt, VIDEO_ES, sys->video_chroma);
        fmt.video.i_chroma = fmt.i_codec;
        fmt.video.i_width = fmt.video.i_visible_width = sys->video_width;
        fmt.video.i_height = fmt.video.i_visible_height = sys->video_height;
        fmt.video.i_frame_rate = sys->video_frame_rate;
        fmt.video.i_frame_rate_base = sys->video_frame_rate_base;

        if (AppendMockTrack(demux, &fmt, group, sys->video_packetized))
            return VLC_ENOMEM;
    }
    return VLC_SUCCESS;
}

static int
InitAudioTracks(demux_t *demux, int group, size_t count)
{
    struct demux_sys *sys = demux->p_sys;

    if (count == 0)
        return VLC_SUCCESS;

    uint16_t physical_channels = 0;
    switch (sys->audio_channels)
    {
        case 1: physical_channels = AOUT_CHAN_CENTER; break;
        case 2: physical_channels = AOUT_CHANS_2_0; break;
        case 3: physical_channels = AOUT_CHANS_2_1; break;
        case 4: physical_channels = AOUT_CHANS_4_0; break;
        case 5: physical_channels = AOUT_CHANS_4_1; break;
        case 6: physical_channels = AOUT_CHANS_6_0; break;
        case 7: physical_channels = AOUT_CHANS_7_0; break;
        case 8: physical_channels = AOUT_CHANS_7_1; break;
        case 9: physical_channels = AOUT_CHANS_8_1; break;
        default: vlc_assert_unreachable();
    }

    for (size_t i = 0; i < count; ++i)
    {
        es_format_t fmt;
        es_format_Init(&fmt, AUDIO_ES, sys->audio_format);
        fmt.audio.i_format = fmt.i_codec;
        fmt.audio.i_rate = sys->audio_rate;
        fmt.audio.i_physical_channels = physical_channels;
        aout_FormatPrepare(&fmt.audio);

        if (AppendMockTrack(demux, &fmt, group, sys->audio_packetized))
            return VLC_ENOMEM;
    }

    return VLC_SUCCESS;
}

static int
InitSubTracks(demux_t *demux, int group, size_t count)
{
    struct demux_sys *sys = demux->p_sys;

    if (count == 0)
        return VLC_SUCCESS;

    for (size_t i = 0; i < count; ++i)
    {
        es_format_t fmt;
        es_format_Init(&fmt, SPU_ES, VLC_CODEC_SUBT);

        if (AppendMockTrack(demux, &fmt, group, sys->sub_packetized))
            return VLC_ENOMEM;
    }

    return VLC_SUCCESS;
}

static int
Demux(demux_t *demux)
{
    struct demux_sys *sys = demux->p_sys;

    if (sys->error)
        return VLC_DEMUXER_EGENERIC;

    struct mock_track *track;
    vlc_vector_foreach(track, &sys->tracks)
    {
        block_t *block;
        switch (track->fmt.i_cat)
        {
            case AUDIO_ES:
                block = CreateAudioBlock(demux, track);
                break;
            case VIDEO_ES:
                block = CreateVideoBlock(demux, track);
                break;
            case SPU_ES:
                block = CreateSubBlock(demux, track);
                break;
            default:
                vlc_assert_unreachable();
        }
        if (!block)
            return VLC_DEMUXER_EGENERIC;
        block->i_length = sys->step_length;
        block->i_pts = block->i_dts = sys->pts;
        int ret = es_out_Send(demux->out, track->id, block);
        if (ret != VLC_SUCCESS)
            return VLC_DEMUXER_EGENERIC;
    }
    es_out_SetPCR(demux->out, sys->pts);
    sys->pts += sys->step_length;
    if (sys->pts > sys->length)
        sys->pts = sys->length;

    if (sys->add_video_track_at != VLC_TICK_INVALID &&
        sys->add_video_track_at <= sys->pts)
    {
        const bool frame_rate_ok =
            sys->video_frame_rate != 0 && sys->video_frame_rate < INT_MAX &&
            sys->video_frame_rate_base != 0 && sys->video_frame_rate_base < INT_MAX;
        const bool chroma_ok = sys->video_chroma != 0;
        const bool size_ok = sys->video_width < INT_MAX &&
                             sys->video_height < INT_MAX;

        if (sys->video_frame_rate == 0 || sys->video_frame_rate_base == 0
         || sys->video_chroma == 0)
        {
            if (!frame_rate_ok || !chroma_ok || !size_ok)
            {
                if (!frame_rate_ok)
                    msg_Err(demux, "Invalid video frame rate");
                if (!chroma_ok)
                    msg_Err(demux, "Invalid video chroma");
                if (!size_ok)
                    msg_Err(demux, "Invalid video size");
                return VLC_EGENERIC;
            }
        }
        else
        {
            InitVideoTracks(demux, 0, 1);
            sys->add_video_track_at = VLC_TICK_INVALID;
        }
    }
    if (sys->add_audio_track_at != VLC_TICK_INVALID &&
        sys->add_audio_track_at <= sys->pts)
    {
        const bool rate_ok = sys->audio_rate > 0 && sys->audio_rate < INT_MAX;
        const bool format_ok = aout_BitsPerSample(sys->audio_format) != 0;
        const bool channels_ok = sys->audio_channels > 0 &&
                                 sys->audio_channels <= AOUT_CHAN_MAX;

        if (!rate_ok)
            msg_Err(demux, "Invalid audio rate");
        else if (!format_ok)
            msg_Err(demux, "Invalid audio format");
        else if (!channels_ok)
            msg_Err(demux, "Invalid audio channels");
        else
        {
            InitAudioTracks(demux, 0, 1);
            sys->add_audio_track_at = VLC_TICK_INVALID;
        }
    }
    if (sys->add_spu_track_at != VLC_TICK_INVALID &&
        sys->add_spu_track_at <= sys->pts)
    {
        InitSubTracks(demux, 0, 1);
        sys->add_spu_track_at = VLC_TICK_INVALID;
    }

    return sys->pts == sys->length ? VLC_DEMUXER_EOF : VLC_DEMUXER_SUCCESS;
}

static void
Close(demux_t *demux)
{
    struct demux_sys *sys = demux->p_sys;

    struct mock_track *track;
    vlc_vector_foreach(track, &sys->tracks)
    {
        es_out_Del(demux->out, track->id);
        free(track);
    }
    vlc_vector_clear(&sys->tracks);
}

static int
Open(demux_t *demux)
{
    if (demux->out == NULL)
        return VLC_EGENERIC;
    struct demux_sys *sys = vlc_obj_malloc(VLC_OBJECT(demux), sizeof(*sys));
    if (!sys)
        return VLC_ENOMEM;

    demux->p_sys = sys;

    if (var_LocationParse(demux, demux->psz_location, "mock-") != VLC_SUCCESS)
        return VLC_ENOMEM;

#define XR(var_name, default_value, min, max, type, module_header_type, getter) \
    X(var_name, default_value, type, module_header_type, getter)
#define X(var_name, default_value, type, module_header_type, getter) \
    sys->var_name = getter(demux, "mock-"#var_name);
    LIST_OPTIONS
#undef XR
#undef X

    if (aout_BitsPerSample(sys->audio_format) == 0)
    {
        msg_Err(demux, "Invalid audio format");
        return VLC_EGENERIC;
    }
    if (sys->video_chroma == 0)
    {
        msg_Err(demux, "Invalid video chroma");
        return VLC_EGENERIC;
    }

    if (sys->chapter_count > 0 && sys->title_count == 0)
        sys->title_count = 1;

    /* note: tests currently expect to be able to give value zero, so don't
       change lower bound of option to 1 to get rid of this! */
    if (sys->program_count == 0)
        sys->program_count = 1;
    size_t track_count = (sys->video_track_count + sys->audio_track_count +
                          sys->sub_track_count) * sys->program_count;
    vlc_vector_init(&sys->tracks);
    if (track_count > 0)
    {
        bool success = vlc_vector_reserve(&sys->tracks, track_count);
        if (!success)
            return VLC_ENOMEM;
    }

    int ret;
    for (ssize_t i = 0; i < sys->program_count; ++i)
    {
        ret = InitVideoTracks(demux, i, sys->video_track_count);
        if (ret != VLC_SUCCESS)
            goto error;

        ret = InitAudioTracks(demux, i, sys->audio_track_count);
        if (ret != VLC_SUCCESS)
            goto error;

        ret = InitSubTracks(demux, i, sys->sub_track_count);
        if (ret != VLC_SUCCESS)
            goto error;
    }

    if (sys->video_track_count > 0)
        sys->step_length = VLC_TICK_FROM_SEC(1) * sys->video_frame_rate_base
                         / sys->video_frame_rate;
    else
        sys->step_length = VLC_TICK_FROM_MS(100);

    sys->pts = VLC_TICK_0;
    sys->current_title = 0;
    sys->chapter_gap = sys->chapter_count > 0 ?
                       (sys->length / sys->chapter_count) : 0;

    demux->pf_control = Control;
    demux->pf_demux = Demux;

    return VLC_SUCCESS;
error:
    Close(demux);
    demux->p_sys = NULL;
    return ret;
}

#define XR(var_name, default_value, min, max, type, module_header_type, getter) \
    module_header_type("mock-"#var_name, default_value, min, max, NULL, NULL, true) \
    change_volatile() \
    change_safe()
#define X(var_name, default_value, type, module_header_type, getter) \
    module_header_type("mock-"#var_name, default_value, NULL, NULL, true) \
    change_volatile() \
    change_safe()

vlc_plugin_begin()
    set_description("mock")
    set_help("mock access-demux")
    add_shortcut("mock")
    set_capability(VLC_CAP_ACCESS, 0, Open, Close)

    set_subcategory(SUBCAT_INPUT_ACCESS)
    LIST_OPTIONS
vlc_plugin_end()

#undef XR
#undef X
