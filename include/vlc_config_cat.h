/*****************************************************************************
 * vlc_config_cat.h : Definition of configuration categories
 *****************************************************************************
 * Copyright (C) 2003 VLC authors and VideoLAN
 *
 * Authors: Clément Stenac <zorglub@videolan.org>
 *          Anil Daoud <anil@videolan.org>
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

#ifndef VLC_CONFIG_CATS_H
#define VLC_CONFIG_CATS_H

/* Config category */
enum vlc_config_cat
{
    CAT_INVALID = -1,

    CAT_INTERFACE = 0,
    CAT_AUDIO,
    CAT_VIDEO,
    CAT_INPUT,
    CAT_SOUT,
    CAT_PLAYLIST,
    CAT_ADVANCED,
};

/* Config subcategory */
enum vlc_config_subcat
{
    SUBCAT_INVALID = -1,

    SUBCAT_INTERFACE_GENERAL = 0,
    SUBCAT_INTERFACE_CONTROL,
    SUBCAT_INTERFACE_HOTKEYS,
    SUBCAT_INTERFACE_MAIN,

    SUBCAT_AUDIO_GENERAL,
    SUBCAT_AUDIO_AFILTER,
    SUBCAT_AUDIO_AOUT,
    SUBCAT_AUDIO_RESAMPLER,
    SUBCAT_AUDIO_VISUAL,

    SUBCAT_VIDEO_GENERAL,
    SUBCAT_VIDEO_VFILTER,
    SUBCAT_VIDEO_VOUT,
    SUBCAT_VIDEO_SPLITTER,
    SUBCAT_VIDEO_SUBPIC,

    SUBCAT_INPUT_GENERAL,
    SUBCAT_INPUT_ACCESS,
    SUBCAT_INPUT_ACODEC,
    SUBCAT_INPUT_DEMUX,
    SUBCAT_INPUT_STREAM_FILTER,
    SUBCAT_INPUT_SCODEC,
    SUBCAT_INPUT_VCODEC,

    SUBCAT_SOUT_GENERAL,
    SUBCAT_SOUT_ACO,
    SUBCAT_SOUT_MUX,
    SUBCAT_SOUT_PACKETIZER,
    SUBCAT_SOUT_RENDERER,
    SUBCAT_SOUT_STREAM,
    SUBCAT_SOUT_VOD,

    SUBCAT_PLAYLIST_GENERAL,
    SUBCAT_PLAYLIST_EXPORT,
    SUBCAT_PLAYLIST_SD,

    SUBCAT_ADVANCED_MISC,
    SUBCAT_ADVANCED_NETWORK,

    /* Hidden subcategory
       Any options under this will be hidden in the GUI preferences, but will
       be listed in cmdline help output. */
    SUBCAT_HIDDEN,

    SUBCAT_MAX,
};

static inline bool vlc_config_IntSubcatIsValid(int i)
{
    return (i >= 0 && i < (int)SUBCAT_MAX);
}

#define MAIN_TITLE N_( "VLC preferences" )
#define MAIN_HELP N_( "Select \"Advanced Options\" to see all options." )

#define INTF_HELP  N_( "Settings for VLC's interfaces" )

#define INTF_GENERAL_HELP N_( "Main interface settings" )

#define INTF_MAIN_HELP N_( "Settings for the main interface" )

#define INTF_CONTROL_HELP N_( "Settings for VLC's control interfaces" )

#define INTF_HOTKEYS_HELP N_( "Hotkey settings" )

#define AUDIO_HELP N_( "Audio settings" )

#define AUDIO_GENERAL_HELP N_("General audio settings")

#define AFILTER_HELP N_( "Audio filters are used to process the audio stream." )

#define AVISUAL_HELP N_( "Audio visualizations" )

#define AOUT_HELP N_("General settings for audio output modules.")

#define VIDEO_HELP N_("Video settings")

#define VIDEO_GENERAL_HELP N_( "General video settings" )

#define VOUT_HELP N_("General settings for video output modules.")

#define VFILTER_HELP N_("Video filters are used to process the video stream." )

#define SUBPIC_HELP N_( "Settings related to On-Screen-Display,"\
        " subtitles and \"overlay subpictures\"")

#define SPLITTER_HELP N_("Video splitters separate the stream into multiple videos.")

#define INPUT_HELP N_( "Settings for input, demultiplexing, " \
         "decoding and encoding")

#define ACCESS_HELP N_( \
    "Settings related to the various access methods. " \
    "Common settings you may want to alter are HTTP proxy or " \
    "caching settings." )

#define STREAM_FILTER_HELP N_( \
    "Stream filters are special modules that allow advanced operations on " \
    "the input side of VLC. Use with care..." )

#define DEMUX_HELP N_( "Demuxers are used to separate audio and video streams." )

#define VDEC_HELP N_( "Settings for the video, images or video+audio decoders and encoders." )

#define ADEC_HELP N_( "Settings for the audio-only decoders and encoders." )

#define SDEC_HELP N_( "Settings for subtitle, teletext and CC decoders and encoders." )

#define ADVANCED_HELP N_( "General input settings. Use with care..." )

#define SOUT_HELP N_( \
      "Stream output settings are used when acting as a streaming server " \
      "or when saving incoming streams.\n" \
      "Streams are first muxed and then sent through an \"access output\" "\
      "module that can either save the stream to a file, or stream " \
      "it (UDP, HTTP, RTP/RTSP).\n" \
      "Sout streams modules allow advanced stream processing (transcoding, "\
      "duplicating...).")

#define SOUT_GENERAL_HELP N_( "General stream output settings")

#define SOUT_MUX_HELP N_( \
       "Muxers create the encapsulation formats that are used to " \
       "put all the elementary streams (video, audio, ...) " \
       "together. This setting allows you to always force a specific muxer. " \
       "You should probably not do that.\n" \
       "You can also set default parameters for each muxer." )

#define SOUT_ACO_HELP N_( \
   "Access output modules control the ways the muxed streams are sent. " \
   "This setting allows you to always force a specific access output method. " \
   "You should probably not do that.\n" \
   "You can also set default parameters for each access output.")

#define SOUT_PACKET_HELP N_( \
        "Packetizers are used to \"preprocess\" the elementary "\
        "streams before muxing. " \
        "This setting allows you to always force a packetizer. " \
        "You should probably not do that.\n" \
        "You can also set default parameters for each packetizer." )

#define SOUT_RENDER_HELP N_( "External renderer discovery related settings." )

#define SOUT_STREAM_HELP N_( "Sout stream modules allow to build a sout " \
                "processing chain. Please refer to the Streaming Howto for " \
                "more information. You can configure default options for " \
                "each sout stream module here.")

#define SOUT_VOD_HELP N_( "VLC's implementation of Video On Demand" )

#define PLAYLIST_HELP N_( "Settings related to playlist behaviour " \
        "(e.g. playback mode) and to modules that automatically add "\
        "items to the playlist (\"service discovery\" modules).")

#define PGENERAL_HELP N_( "General playlist behaviour")

#define SD_HELP N_("Services discovery modules are facilities "\
        "that automatically add items to playlist.")

#define PEXPORT_HELP N_( "Setting relating to exporting playlists" )

#define AADVANCED_HELP N_( "Advanced settings. Use with care...")

#define ANETWORK_HELP N_( "Advanced network settings." )

struct config_category_t
{
    enum vlc_config_cat cat;
    const char *psz_name;
    const char *psz_help;
};

struct config_subcategory_t
{
    enum vlc_config_subcat subcat;
    const char *psz_name;
    const char *psz_help;
};

static const struct config_category_t categories_array[] =
{
    { CAT_INTERFACE,               N_("Interface"),           INTF_HELP          },
    { CAT_AUDIO,                   N_("Audio"),               AUDIO_HELP         },
    { CAT_VIDEO,                   N_("Video"),               VIDEO_HELP         },
    { CAT_INPUT,                   N_("Input / Codecs"),      INPUT_HELP         },
    { CAT_SOUT,                    N_("Stream output"),       SOUT_HELP          },
    { CAT_PLAYLIST,                N_("Playlist") ,           PLAYLIST_HELP      },
    { CAT_ADVANCED,                N_("Advanced"),            AADVANCED_HELP     },

    { CAT_INVALID, NULL, NULL }
};

static const struct config_subcategory_t subcategories_array[] =
{
    { SUBCAT_INTERFACE_GENERAL,    N_("Interface"),           INTF_GENERAL_HELP  },
    { SUBCAT_INTERFACE_CONTROL,    N_("Control interfaces"),  INTF_CONTROL_HELP  },
    { SUBCAT_INTERFACE_HOTKEYS,    N_("Hotkey settings"),     INTF_HOTKEYS_HELP  },
    { SUBCAT_INTERFACE_MAIN,       N_("Main interfaces"),     INTF_MAIN_HELP     },

    { SUBCAT_AUDIO_GENERAL,        N_("Audio"),               AUDIO_GENERAL_HELP },
    { SUBCAT_AUDIO_AFILTER,        N_("Filters"),             AFILTER_HELP       },
    { SUBCAT_AUDIO_AOUT,           N_("Output modules"),      AOUT_HELP          },
    { SUBCAT_AUDIO_RESAMPLER,      N_("Resampler"),           AFILTER_HELP       },
    { SUBCAT_AUDIO_VISUAL,         N_("Visualizations"),      AVISUAL_HELP       },

    { SUBCAT_VIDEO_GENERAL,        N_("Video"),               VIDEO_GENERAL_HELP },
    { SUBCAT_VIDEO_VFILTER,        N_("Filters"),             VFILTER_HELP       },
    { SUBCAT_VIDEO_VOUT,           N_("Output modules"),      VOUT_HELP          },
    { SUBCAT_VIDEO_SPLITTER,       N_("Splitters"),           SPLITTER_HELP      },
    { SUBCAT_VIDEO_SUBPIC,         N_("Subtitles / OSD"),     SUBPIC_HELP        },

    { SUBCAT_INPUT_GENERAL,        N_("Input / Codecs"),      INPUT_HELP         },
    { SUBCAT_INPUT_ACCESS,         N_("Access modules"),      ACCESS_HELP        },
    { SUBCAT_INPUT_ACODEC,         N_("Audio codecs"),        ADEC_HELP          },
    { SUBCAT_INPUT_DEMUX,          N_("Demuxers"),            DEMUX_HELP         },
    { SUBCAT_INPUT_STREAM_FILTER,  N_("Stream filters"),      STREAM_FILTER_HELP },
    { SUBCAT_INPUT_SCODEC,         N_("Subtitle codecs"),     SDEC_HELP          },
    { SUBCAT_INPUT_VCODEC,         N_("Video codecs"),        VDEC_HELP          },

    { SUBCAT_SOUT_GENERAL,         N_("Stream output"),       SOUT_GENERAL_HELP  },
    { SUBCAT_SOUT_ACO,             N_("Access output"),       SOUT_ACO_HELP      },
    { SUBCAT_SOUT_MUX,             N_("Muxers"),              SOUT_MUX_HELP      },
    { SUBCAT_SOUT_PACKETIZER,      N_("Packetizers"),         SOUT_PACKET_HELP   },
    { SUBCAT_SOUT_RENDERER,        N_("Renderers"),           SOUT_RENDER_HELP   },
    { SUBCAT_SOUT_STREAM,          N_("Sout stream"),         SOUT_STREAM_HELP   },
    { SUBCAT_SOUT_VOD,             N_("VoD"),                 SOUT_VOD_HELP      },

    { SUBCAT_PLAYLIST_GENERAL,     N_("Playlist"),            PGENERAL_HELP      },
    { SUBCAT_PLAYLIST_EXPORT,      N_("Export"),              PEXPORT_HELP       },
    { SUBCAT_PLAYLIST_SD,          N_("Services discovery"),  SD_HELP            },

    { SUBCAT_ADVANCED_MISC,        N_("Advanced settings"),   AADVANCED_HELP     },
    { SUBCAT_ADVANCED_NETWORK,     N_("Network"),             ANETWORK_HELP      },

    { SUBCAT_INVALID, NULL, NULL }
};

VLC_USED
static inline const char *vlc_config_SubcategoryNameGet( enum vlc_config_subcat subcat )
{
    int i = 0;
    while( subcategories_array[i].psz_name != NULL )
    {
        if( subcategories_array[i].subcat == subcat )
        {
            return vlc_gettext(subcategories_array[i].psz_name);
        }
        i++;
    }
    return NULL;
}

VLC_USED
static inline const char *vlc_config_CategoryNameGet( enum vlc_config_cat cat )
{
    int i = 0;
    while( categories_array[i].psz_name != NULL )
    {
        if( categories_array[i].cat == cat )
        {
            return vlc_gettext(categories_array[i].psz_name);
        }
        i++;
    }
    return NULL;
}

VLC_USED
static inline const char *vlc_config_SubcategoryHelpGet( enum vlc_config_subcat subcat )
{
    int i = 0;
    while( subcategories_array[i].psz_help != NULL )
    {
        if( subcategories_array[i].subcat == subcat )
        {
            return vlc_gettext(subcategories_array[i].psz_help);
        }
        i++;
    }
    return NULL;
}

VLC_USED
static inline const char *vlc_config_CategoryHelpGet( enum vlc_config_cat cat )
{
    int i = 0;
    while( categories_array[i].psz_help != NULL )
    {
        if( categories_array[i].cat == cat )
        {
            return vlc_gettext(categories_array[i].psz_help);
        }
        i++;
    }
    return NULL;
}

VLC_USED
static inline enum vlc_config_cat vlc_config_CategoryFromSubcategory( enum vlc_config_subcat subcat )
{
    switch (subcat)
    {
        case SUBCAT_INTERFACE_GENERAL:
        case SUBCAT_INTERFACE_MAIN:
        case SUBCAT_INTERFACE_CONTROL:
        case SUBCAT_INTERFACE_HOTKEYS:
            return CAT_INTERFACE;
        case SUBCAT_AUDIO_GENERAL:
        case SUBCAT_AUDIO_AOUT:
        case SUBCAT_AUDIO_AFILTER:
        case SUBCAT_AUDIO_VISUAL:
        case SUBCAT_AUDIO_RESAMPLER:
            return CAT_AUDIO;
        case SUBCAT_VIDEO_GENERAL:
        case SUBCAT_VIDEO_VOUT:
        case SUBCAT_VIDEO_VFILTER:
        case SUBCAT_VIDEO_SUBPIC:
        case SUBCAT_VIDEO_SPLITTER:
            return CAT_VIDEO;
        case SUBCAT_INPUT_GENERAL:
        case SUBCAT_INPUT_ACCESS:
        case SUBCAT_INPUT_DEMUX:
        case SUBCAT_INPUT_VCODEC:
        case SUBCAT_INPUT_ACODEC:
        case SUBCAT_INPUT_SCODEC:
        case SUBCAT_INPUT_STREAM_FILTER:
            return CAT_INPUT;
        case SUBCAT_SOUT_GENERAL:
        case SUBCAT_SOUT_STREAM:
        case SUBCAT_SOUT_MUX:
        case SUBCAT_SOUT_ACO:
        case SUBCAT_SOUT_PACKETIZER:
        case SUBCAT_SOUT_VOD:
        case SUBCAT_SOUT_RENDERER:
            return CAT_SOUT;
        case SUBCAT_ADVANCED_MISC:
        case SUBCAT_ADVANCED_NETWORK:
            return CAT_ADVANCED;
        case SUBCAT_PLAYLIST_GENERAL:
        case SUBCAT_PLAYLIST_SD:
        case SUBCAT_PLAYLIST_EXPORT:
            return CAT_PLAYLIST;
        case SUBCAT_HIDDEN:
            return CAT_INVALID;
        default:
            unreachable();
    }
}

VLC_USED
static inline bool vlc_config_SubcategoryIsGeneral( enum vlc_config_subcat subcat )
{
    if (subcat == SUBCAT_VIDEO_GENERAL ||
        subcat == SUBCAT_INPUT_GENERAL ||
        subcat == SUBCAT_INTERFACE_GENERAL ||
        subcat == SUBCAT_SOUT_GENERAL||
        subcat == SUBCAT_PLAYLIST_GENERAL||
        subcat == SUBCAT_AUDIO_GENERAL||
        subcat == SUBCAT_ADVANCED_MISC)
    {
        return true;
    }
    return false;
}

#endif /* VLC_CONFIG_CATS_H */
