/*****************************************************************************
 * vlc_module_caps.h : Module capability handling.
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
 *
 * Authors: Lyndon Brown <jnqnfe@gmail.com>
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

#ifndef LIBVLC_MODULE_CAPS_H
# define LIBVLC_MODULE_CAPS_H 1

/**
 * Module capabilties
 *
 * Note: It is *NOT* safe to assume that modules with a capability of
 * VLC_CAP_INVALID would never exist in a bug-free running instance; some
 * plugins with multiple modules use the "hack" of leaving their first module
 * without a capability set (nor callbacks), with its purpose being only to
 * hold a "group" name representing all of that plugin's modules, for display
 * purposes (e.g. for help output - plugins own options, not modules, but
 * (currently) do not themselves hold specific properties for this, properties
 * are used from a plugin's first module).
 */
enum vlc_module_cap
{
    /* WARNING: MAKING ANY CHANGES, OTHER THAN INSERTING NEW ITEMS AT THE END,
     * IS AN ABI BREAK, REQUIRING A BUMP TO THE PLUGIN ABI NUMBER DEFINED IN
     * PLUGIN.H!
     *
     * Also, if this is changed, do remember to also update the descriptions
     * table at the end of src/modules/bank.c!
     */

    /* Invalid/unset default */
    VLC_CAP_INVALID = -1,

    /* Custom is special, allowing modules to define custom capabilities via a string name */
    VLC_CAP_CUSTOM = 0,

    /* Built-in capabilities */
    VLC_CAP_CORE = 1, /* core program */
    VLC_CAP_ACCESS,
    VLC_CAP_ADDONS_FINDER,
    VLC_CAP_ADDONS_STORAGE,
    VLC_CAP_AOUT_STREAM,
    VLC_CAP_ART_FINDER,
    VLC_CAP_AUDIO_CONVERTER,
    VLC_CAP_AUDIO_FILTER,
    VLC_CAP_AUDIO_OUTPUT,
    VLC_CAP_AUDIO_RESAMPLER,
    VLC_CAP_AUDIO_RENDERER,
    VLC_CAP_AUDIO_VOLUME,
    VLC_CAP_AUDIO_DECODER,
    VLC_CAP_VIDEO_DECODER,
    VLC_CAP_SPU_DECODER,
    VLC_CAP_DEMUX,
    VLC_CAP_DEMUX_FILTER,
    VLC_CAP_DIALOGS_PROVIDER,
    VLC_CAP_ENCODER,
    VLC_CAP_EXTENSION,
    VLC_CAP_FINGERPRINTER,
    VLC_CAP_GLCONV,
    VLC_CAP_HW_DECODER,
    VLC_CAP_HW_DECODER_DEVICE,
    VLC_CAP_INHIBIT,
    VLC_CAP_INTERFACE,
    VLC_CAP_KEYSTORE,
    VLC_CAP_LOGGER,
    VLC_CAP_MEDIALIBRARY,
    VLC_CAP_META_FETCHER,
    VLC_CAP_META_READER,
    VLC_CAP_META_WRITER,
    VLC_CAP_OPENGL,
    VLC_CAP_PACKETIZER,
    VLC_CAP_PLAYLIST_EXPORT,
    VLC_CAP_RENDERER_DISCOVERY,
    VLC_CAP_RENDERER_PROBE,
    VLC_CAP_SERVICES_DISCOVERY,
    VLC_CAP_SERVICES_PROBE,
    VLC_CAP_SOUT_ACCESS,
    VLC_CAP_SOUT_MUX,
    VLC_CAP_SOUT_STREAM,
    VLC_CAP_STREAM_DIRECTORY,
    VLC_CAP_STREAM_EXTRACTOR,
    VLC_CAP_STREAM_FILTER,
    VLC_CAP_SUB_FILTER,
    VLC_CAP_SUB_SOURCE,
    VLC_CAP_TEXT_RENDERER,
    VLC_CAP_TLS_CLIENT,
    VLC_CAP_TLS_SERVER,
    VLC_CAP_VIDEO_BLENDING,
    VLC_CAP_VIDEO_CONVERTER,
    VLC_CAP_VIDEO_FILTER,
    VLC_CAP_VIDEO_SPLITTER,
    VLC_CAP_VISUALIZATION,
    VLC_CAP_VOD_SERVER,
    VLC_CAP_VOUT_DISPLAY,
    VLC_CAP_VOUT_WINDOW,
    VLC_CAP_VULKAN,
    VLC_CAP_XML,
    VLC_CAP_XML_READER,

    /* --> Insert new entries here <-- */

    /* Max valid ID */
    /* This Must always come last, add new entries above this! */
    VLC_CAP_MAX,
};

/* String form IDs of capabilities, useful for instance in defining options
 * which relate to a capability where the string name is needed, or for
 * variable names. */
#define VLC_CAP_STR_ACCESS             "access"
#define VLC_CAP_STR_ADDONS_FINDER      "addons finder"
#define VLC_CAP_STR_ADDONS_STORAGE     "addons storage"
#define VLC_CAP_STR_AOUT_STREAM        "aout stream"
#define VLC_CAP_STR_ART_FINDER         "art finder"
#define VLC_CAP_STR_AUDIO_CONVERTER    "audio converter"
#define VLC_CAP_STR_AUDIO_DECODER      "audio decoder"
#define VLC_CAP_STR_AUDIO_FILTER       "audio filter"
#define VLC_CAP_STR_AUDIO_OUTPUT       "audio output"
#define VLC_CAP_STR_AUDIO_RENDERER     "audio renderer"
#define VLC_CAP_STR_AUDIO_RESAMPLER    "audio resampler"
#define VLC_CAP_STR_AUDIO_VOLUME       "audio volume"
#define VLC_CAP_STR_DEMUX              "demux"
#define VLC_CAP_STR_DEMUX_FILTER       "demux_filter"
#define VLC_CAP_STR_DIALOGS_PROVIDER   "dialogs provider"
#define VLC_CAP_STR_ENCODER            "encoder"
#define VLC_CAP_STR_EXTENSION          "extension"
#define VLC_CAP_STR_FINGERPRINTER      "fingerprinter"
#define VLC_CAP_STR_GLCONV             "glconv"
#define VLC_CAP_STR_HW_DECODER_DEVICE  "decoder device"
#define VLC_CAP_STR_HW_DECODER         "hw decoder"
#define VLC_CAP_STR_INHIBIT            "inhibit"
#define VLC_CAP_STR_INTERFACE          "interface"
#define VLC_CAP_STR_KEYSTORE           "keystore"
#define VLC_CAP_STR_LOGGER             "logger"
#define VLC_CAP_STR_MEDIALIBRARY       "medialibrary"
#define VLC_CAP_STR_META_FETCHER       "meta fetcher"
#define VLC_CAP_STR_META_READER        "meta reader"
#define VLC_CAP_STR_META_WRITER        "meta writer"
#define VLC_CAP_STR_OPENGL             "opengl"
#define VLC_CAP_STR_PACKETIZER         "packetizer"
#define VLC_CAP_STR_PLAYLIST_EXPORT    "playlist export"
#define VLC_CAP_STR_RENDERER_DISCOVERY "renderer_discovery"
#define VLC_CAP_STR_RENDERER_PROBE     "renderer probe"
#define VLC_CAP_STR_SERVICES_DISCOVERY "services_discovery"
#define VLC_CAP_STR_SERVICES_PROBE     "services probe"
#define VLC_CAP_STR_SOUT_ACCESS        "sout access"
#define VLC_CAP_STR_SOUT_MUX           "sout mux"
#define VLC_CAP_STR_SOUT_STREAM        "sout stream"
#define VLC_CAP_STR_SPU_DECODER        "spu decoder"
#define VLC_CAP_STR_STREAM_DIRECTORY   "stream_directory"
#define VLC_CAP_STR_STREAM_EXTRACTOR   "stream_extractor"
#define VLC_CAP_STR_STREAM_FILTER      "stream_filter"
#define VLC_CAP_STR_SUB_FILTER         "sub filter"
#define VLC_CAP_STR_SUB_SOURCE         "sub source"
#define VLC_CAP_STR_TEXT_RENDERER      "text renderer"
#define VLC_CAP_STR_TLS_CLIENT         "tls client"
#define VLC_CAP_STR_TLS_SERVER         "tls server"
#define VLC_CAP_STR_VIDEO_BLENDING     "video blending"
#define VLC_CAP_STR_VIDEO_CONVERTER    "video converter"
#define VLC_CAP_STR_VIDEO_DECODER      "video decoder"
#define VLC_CAP_STR_VIDEO_FILTER       "video filter"
#define VLC_CAP_STR_VIDEO_SPLITTER     "video splitter"
#define VLC_CAP_STR_VISUALIZATION      "visualization"
#define VLC_CAP_STR_VOD_SERVER         "vod server"
#define VLC_CAP_STR_VOUT_DISPLAY       "vout display"
#define VLC_CAP_STR_VOUT_WINDOW        "vout window"
#define VLC_CAP_STR_VULKAN             "vulkan"
#define VLC_CAP_STR_XML_READER         "xml reader"
#define VLC_CAP_STR_XML                "xml"

/**
 * Converts the string ID form of a capability to its enum form
 *
 * If no conversion is possible, returns VLC_CAP_CUSTOM
 */
VLC_API enum vlc_module_cap vlc_module_cap_from_textid(const char * textid) VLC_USED;

/**
 * Gives the string ID form of a given capability
 */
VLC_API const char *vlc_module_cap_get_textid(enum vlc_module_cap cap) VLC_USED;

/**
 * Gives a text description for a given capability
 */
VLC_API const char *vlc_module_cap_get_desc(enum vlc_module_cap cap) VLC_USED;

/**
 * Checks whether int maps to a valid built-in capability
 */
static inline bool vlc_module_int_is_valid_cap(int i)
{
    /* Note that this deliberately rejects VLC_CAP_CUSTOM! */
    return (i >= 1 && i < (int)VLC_CAP_MAX);
}

#endif
