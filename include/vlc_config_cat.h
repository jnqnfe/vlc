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

# include <vlc_plugin.h>

#define MAIN_TITLE N_( "VLC preferences" )
#define MAIN_HELP N_( "Select \"Advanced Options\" to see all options." )

#define INTF_HELP  N_( "Settings for VLC's interfaces" )

#define INTF_GENERAL_HELP N_( "Main interfaces settings" )

#define INTF_MAIN_HELP N_( "Settings for the main interface" )

#define INTF_CONTROL_HELP N_( "Settings for VLC's control interfaces" )

#define INTF_HOTKEYS_HELP N_( "Hotkeys settings" )

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
    int         i_id;
    const char *psz_name;
    const char *psz_help;
};

/* This function is deprecated and is kept only for compatibility */
static const struct config_category_t categories_array[] =
{
    { CAT_INTERFACE,               N_("Interface"),           INTF_HELP          },
    { SUBCAT_INTERFACE_GENERAL,    N_("Interface"),           INTF_GENERAL_HELP  },
    { SUBCAT_INTERFACE_CONTROL,    N_("Control interfaces"),  INTF_CONTROL_HELP  },
    { SUBCAT_INTERFACE_HOTKEYS,    N_("Hotkeys settings"),    INTF_HOTKEYS_HELP  },
    { SUBCAT_INTERFACE_MAIN,       N_("Main interfaces"),     INTF_MAIN_HELP     },

    { CAT_AUDIO,                   N_("Audio"),               AUDIO_HELP         },
    { SUBCAT_AUDIO_GENERAL,        N_("Audio"),               AUDIO_GENERAL_HELP },
    { SUBCAT_AUDIO_RESAMPLER,      N_("Audio resampler"),     AFILTER_HELP       },
    { SUBCAT_AUDIO_AFILTER,        N_("Filters"),             AFILTER_HELP       },
    { SUBCAT_AUDIO_AOUT,           N_("Output modules"),      AOUT_HELP          },
    { SUBCAT_AUDIO_VISUAL,         N_("Visualizations"),      AVISUAL_HELP       },

    { CAT_VIDEO,                   N_("Video"),               VIDEO_HELP         },
    { SUBCAT_VIDEO_GENERAL,        N_("Video"),               VIDEO_GENERAL_HELP },
    { SUBCAT_VIDEO_VFILTER,        N_("Filters"),             VFILTER_HELP       },
    { SUBCAT_VIDEO_VOUT,           N_("Output modules"),      VOUT_HELP          },
    { SUBCAT_VIDEO_SPLITTER,       N_("Splitters"),           SPLITTER_HELP      },
    { SUBCAT_VIDEO_SUBPIC,         N_("Subtitles / OSD"),     SUBPIC_HELP        },

    { CAT_INPUT,                   N_("Input / Codecs"),      INPUT_HELP         },
    { SUBCAT_INPUT_GENERAL,        N_("Input / Codecs"),      INPUT_HELP         },
    { SUBCAT_INPUT_ACCESS,         N_("Access modules"),      ACCESS_HELP        },
    { SUBCAT_INPUT_ACODEC,         N_("Audio codecs"),        ADEC_HELP          },
    { SUBCAT_INPUT_DEMUX,          N_("Demuxers"),            DEMUX_HELP         },
    { SUBCAT_INPUT_STREAM_FILTER,  N_("Stream filters"),      STREAM_FILTER_HELP },
    { SUBCAT_INPUT_SCODEC,         N_("Subtitle codecs"),     SDEC_HELP          },
    { SUBCAT_INPUT_VCODEC,         N_("Video codecs"),        VDEC_HELP          },

    { CAT_SOUT,                    N_("Stream output"),       SOUT_HELP          },
    { SUBCAT_SOUT_GENERAL,         N_("Stream output"),       SOUT_GENERAL_HELP  },
    { SUBCAT_SOUT_ACO,             N_("Access output"),       SOUT_ACO_HELP      },
    { SUBCAT_SOUT_MUX,             N_("Muxers"),              SOUT_MUX_HELP      },
    { SUBCAT_SOUT_PACKETIZER,      N_("Packetizers"),         SOUT_PACKET_HELP   },
    { SUBCAT_SOUT_RENDERER,        N_("Renderers"),           SOUT_RENDER_HELP   },
    { SUBCAT_SOUT_STREAM,          N_("Sout stream"),         SOUT_STREAM_HELP   },
    { SUBCAT_SOUT_VOD,             N_("VOD"),                 SOUT_VOD_HELP      },

    { CAT_PLAYLIST,                N_("Playlist") ,           PLAYLIST_HELP      },
    { SUBCAT_PLAYLIST_GENERAL,     N_("Playlist"),            PGENERAL_HELP      },
    { SUBCAT_PLAYLIST_EXPORT,      N_("Export"),              PEXPORT_HELP       },
    { SUBCAT_PLAYLIST_SD,          N_("Services discovery"),  SD_HELP            },

    { CAT_ADVANCED,                N_("Advanced"),            AADVANCED_HELP     },
    { SUBCAT_ADVANCED_MISC,        N_("Advanced settings"),   AADVANCED_HELP     },
    { SUBCAT_ADVANCED_NETWORK,     N_("Network"),             ANETWORK_HELP      },

    { -1, NULL, NULL }
};

VLC_USED
static inline const char *config_CategoryNameGet( int i_value )
{
    int i = 0;
    while( categories_array[i].psz_name != NULL )
    {
        if( categories_array[i].i_id == i_value )
        {
            return vlc_gettext(categories_array[i].psz_name);
        }
        i++;
    }
    return NULL;
}

VLC_USED
static inline const char *config_CategoryHelpGet( int i_value )
{
    int i = 0;
    while( categories_array[i].psz_help != NULL )
    {
        if( categories_array[i].i_id == i_value )
        {
            return vlc_gettext(categories_array[i].psz_help);
        }
        i++;
    }
    return NULL;
}

#endif /* VLC_CONFIG_CATS_H */
