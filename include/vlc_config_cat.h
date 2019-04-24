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

#include <assert.h>

/* Config category */
enum vlc_config_cat
{
    /* Warning: Keep the order here consistent with the data table below. */

    CAT_INVALID = -1,

    CAT_INTERFACE = 0,
    CAT_AUDIO,
    CAT_VIDEO,
    CAT_INPUT,
    CAT_SOUT,
    CAT_PLAYLIST,
    CAT_ADVANCED,

    /* for table-lookup purposes only! */
    CAT_HIDDEN,

    CAT_MAX,
};

/* Config subcategory */
enum vlc_config_subcat
{
    /* Warning: Keep the order here consistent with the data table below. */

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

#define INTF_GENERAL_HELP N_( "Main interface settings" )

#define INTF_MAIN_HELP N_( "Settings for the main interface" )

#define INTF_CONTROL_HELP N_( "Settings for VLC's control interfaces" )

#define INTF_HOTKEYS_HELP N_( "Hotkey settings" )

#define AUDIO_GENERAL_HELP N_("General audio settings")

#define AFILTER_HELP N_( "Audio filters are used to process the audio stream." )

#define AVISUAL_HELP N_( "Audio visualizations" )

#define AOUT_HELP N_("General settings for audio output modules.")

#define VIDEO_GENERAL_HELP N_( "General video settings" )

#define VOUT_HELP N_("General settings for video output modules.")

#define VFILTER_HELP N_("Video filters are used to process the video stream." )

#define SUBPIC_HELP N_( "Settings related to On-Screen-Display,"\
        " subtitles and \"overlay subpictures\"")

#define SPLITTER_HELP N_("Video splitters separate the stream into multiple videos.")

#define INPUT_GENERAL_HELP N_( "Settings for input, demultiplexing, " \
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

#define PL_GENERAL_HELP N_( "General playlist behaviour")

#define SD_HELP N_("Services discovery modules are facilities "\
        "that automatically add items to playlist.")

#define PL_EXPORT_HELP N_( "Setting relating to exporting playlists" )

#define AADVANCED_HELP N_( "Advanced settings. Use with care...")

#define ANETWORK_HELP N_( "Advanced network settings." )

struct vlc_config_subcat_data
{
    enum vlc_config_cat cat;
    const char *name;
    const char *help;
};

static const enum vlc_config_subcat vlc_cat_to_general_subcat_map[] =
{
    SUBCAT_INTERFACE_GENERAL, /* CAT_INTERFACE */
    SUBCAT_AUDIO_GENERAL,     /* CAT_AUDIO     */
    SUBCAT_VIDEO_GENERAL,     /* CAT_VIDEO     */
    SUBCAT_INPUT_GENERAL,     /* CAT_INPUT     */
    SUBCAT_SOUT_GENERAL,      /* CAT_SOUT      */
    SUBCAT_PLAYLIST_GENERAL,  /* CAT_PLAYLIST  */
    SUBCAT_ADVANCED_MISC,     /* CAT_ADVANCED  */
    SUBCAT_HIDDEN,            /* CAT_HIDDEN    */
};

static_assert(CAT_MAX == (sizeof (vlc_cat_to_general_subcat_map) / sizeof (vlc_cat_to_general_subcat_map[0])), "cat to general subcat map size mismatch");

static const enum vlc_config_cat vlc_cat_preferred_order[] =
{
    CAT_PLAYLIST, CAT_INTERFACE, CAT_AUDIO, CAT_VIDEO, CAT_INPUT, CAT_SOUT, CAT_ADVANCED, CAT_HIDDEN,
};

static const unsigned vlc_cat_preferred_order_count = sizeof (vlc_cat_preferred_order) / sizeof (vlc_cat_preferred_order[0]);

static_assert(CAT_MAX == (sizeof (vlc_cat_preferred_order) / sizeof (vlc_cat_preferred_order[0])), "cat preferred order table size mismatch");

static const struct vlc_config_subcat_data vlc_subcategory_data[] =
{
    /* SUBCAT_INTERFACE_GENERAL   */ { CAT_INTERFACE,  N_("Interface"),           INTF_GENERAL_HELP  },
    /* SUBCAT_INTERFACE_CONTROL   */ { CAT_INTERFACE,  N_("Control interfaces"),  INTF_CONTROL_HELP  },
    /* SUBCAT_INTERFACE_HOTKEYS   */ { CAT_INTERFACE,  N_("Hotkey settings"),     INTF_HOTKEYS_HELP  },
    /* SUBCAT_INTERFACE_MAIN      */ { CAT_INTERFACE,  N_("Main interfaces"),     INTF_MAIN_HELP     },

    /* SUBCAT_AUDIO_GENERAL       */ { CAT_AUDIO,      N_("Audio"),               AUDIO_GENERAL_HELP },
    /* SUBCAT_AUDIO_AFILTER       */ { CAT_AUDIO,      N_("Filters"),             AFILTER_HELP       },
    /* SUBCAT_AUDIO_AOUT          */ { CAT_AUDIO,      N_("Output modules"),      AOUT_HELP          },
    /* SUBCAT_AUDIO_RESAMPLER     */ { CAT_AUDIO,      N_("Resampler"),           AFILTER_HELP       },
    /* SUBCAT_AUDIO_VISUAL        */ { CAT_AUDIO,      N_("Visualizations"),      AVISUAL_HELP       },

    /* SUBCAT_VIDEO_GENERAL       */ { CAT_VIDEO,      N_("Video"),               VIDEO_GENERAL_HELP },
    /* SUBCAT_VIDEO_VFILTER       */ { CAT_VIDEO,      N_("Filters"),             VFILTER_HELP       },
    /* SUBCAT_VIDEO_VOUT          */ { CAT_VIDEO,      N_("Output modules"),      VOUT_HELP          },
    /* SUBCAT_VIDEO_SPLITTER      */ { CAT_VIDEO,      N_("Splitters"),           SPLITTER_HELP      },
    /* SUBCAT_VIDEO_SUBPIC        */ { CAT_VIDEO,      N_("Subtitles / OSD"),     SUBPIC_HELP        },

    /* SUBCAT_INPUT_GENERAL       */ { CAT_INPUT,      N_("Input / Codecs"),      INPUT_GENERAL_HELP },
    /* SUBCAT_INPUT_ACCESS        */ { CAT_INPUT,      N_("Access modules"),      ACCESS_HELP        },
    /* SUBCAT_INPUT_ACODEC        */ { CAT_INPUT,      N_("Audio codecs"),        ADEC_HELP          },
    /* SUBCAT_INPUT_DEMUX         */ { CAT_INPUT,      N_("Demuxers"),            DEMUX_HELP         },
    /* SUBCAT_INPUT_STREAM_FILTER */ { CAT_INPUT,      N_("Stream filters"),      STREAM_FILTER_HELP },
    /* SUBCAT_INPUT_SCODEC        */ { CAT_INPUT,      N_("Subtitle codecs"),     SDEC_HELP          },
    /* SUBCAT_INPUT_VCODEC        */ { CAT_INPUT,      N_("Video codecs"),        VDEC_HELP          },

    /* SUBCAT_SOUT_GENERAL        */ { CAT_SOUT,       N_("Stream output"),       SOUT_GENERAL_HELP  },
    /* SUBCAT_SOUT_ACO            */ { CAT_SOUT,       N_("Access output"),       SOUT_ACO_HELP      },
    /* SUBCAT_SOUT_MUX            */ { CAT_SOUT,       N_("Muxers"),              SOUT_MUX_HELP      },
    /* SUBCAT_SOUT_PACKETIZER     */ { CAT_SOUT,       N_("Packetizers"),         SOUT_PACKET_HELP   },
    /* SUBCAT_SOUT_RENDERER       */ { CAT_SOUT,       N_("Renderers"),           SOUT_RENDER_HELP   },
    /* SUBCAT_SOUT_STREAM         */ { CAT_SOUT,       N_("Sout stream"),         SOUT_STREAM_HELP   },
    /* SUBCAT_SOUT_VOD            */ { CAT_SOUT,       N_("VoD"),                 SOUT_VOD_HELP      },

    /* SUBCAT_PLAYLIST_GENERAL    */ { CAT_PLAYLIST,   N_("Playlist"),            PL_GENERAL_HELP    },
    /* SUBCAT_PLAYLIST_EXPORT     */ { CAT_PLAYLIST,   N_("Export"),              PL_EXPORT_HELP     },
    /* SUBCAT_PLAYLIST_SD         */ { CAT_PLAYLIST,   N_("Services discovery"),  SD_HELP            },

    /* SUBCAT_ADVANCED_MISC       */ { CAT_ADVANCED,   N_("Advanced"),            AADVANCED_HELP     },
    /* SUBCAT_ADVANCED_NETWORK    */ { CAT_ADVANCED,   N_("Network"),             ANETWORK_HELP      },

    /* SUBCAT_HIDDEN              */ { CAT_HIDDEN,     NULL,                      NULL               },
};

static_assert(SUBCAT_MAX == (sizeof (vlc_subcategory_data) / sizeof (vlc_subcategory_data[0])), "subcategory data table size mismatch");

/** Get the parent category for a given subcategory */
VLC_USED
static inline enum vlc_config_cat vlc_config_CategoryFromSubcategory( enum vlc_config_subcat subcat )
{
    return vlc_subcategory_data[(int)subcat].cat;
}

/** Get the name for a subcategory */
VLC_USED
static inline const char *vlc_config_SubcategoryNameGet( enum vlc_config_subcat subcat )
{
    return vlc_gettext(vlc_subcategory_data[(int)subcat].name);
}

/** Get the name for a category */
VLC_USED
static inline const char *vlc_config_CategoryNameGet( enum vlc_config_cat cat )
{
    return vlc_config_SubcategoryNameGet(vlc_cat_to_general_subcat_map[(int)cat]);
}

/** Get the help text for a subcategory */
VLC_USED
static inline const char *vlc_config_SubcategoryHelpGet( enum vlc_config_subcat subcat )
{
    return vlc_gettext(vlc_subcategory_data[(int)subcat].help);
}

/** Get the help text for a category */
VLC_USED
static inline const char *vlc_config_CategoryHelpGet( enum vlc_config_cat cat )
{
    return vlc_config_SubcategoryHelpGet(vlc_cat_to_general_subcat_map[(int)cat]);
}

/** Check if the given subcategory is a "general" one
 *
 * A "general" subcategory may be displayed when the category node itself in a
 * cat/subcat tree is selected, rather than appearing as a child node under the
 * category, as with other subcategories.
 */
VLC_USED
static inline bool vlc_config_SubcategoryIsGeneral( enum vlc_config_subcat subcat )
{
    enum vlc_config_cat cat = vlc_config_CategoryFromSubcategory( subcat );
    return (subcat == vlc_cat_to_general_subcat_map[(int)cat]);
}

/** Get the "general" subcategory of a given category */
VLC_USED
static inline enum vlc_config_subcat vlc_config_CategoryGeneralSubcatGet( enum vlc_config_cat cat )
{
    return vlc_cat_to_general_subcat_map[(int)cat];
}

#endif /* VLC_CONFIG_CATS_H */
