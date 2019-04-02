/*****************************************************************************
 * vlc_plugin.h : Plugin descriptor stuff.
 *****************************************************************************
 * Copyright (C) 2001-2006 VLC authors and VideoLAN
 * Copyright © 2007-2009 Rémi Denis-Courmont
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
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

#ifndef LIBVLC_PLUGIN_MACROS_H
# define LIBVLC_PLUGIN_MACROS_H 1

#include <vlc_module_caps.h>
#include <vlc_configuration.h>
#include <vlc_config_cat.h>
#include <assert.h>

/**
 * \file
 * This file implements plugin (module) macros used to define a vlc plugin.
 */

/**
 * Current plugin ABI version
 */
#define PLUGIN_ABI_VERSION 4_0_11

/* Descriptor callback actions, ignore this! */
enum vlc_plugin_desc_actions
{
    /* WARNING: MAKING ANY CHANGES, OTHER THAN INSERTING NEW ITEMS AT THE END
     * (OF BLOCKS, WHERE MARKED) IS AN ABI BREAK, REQUIRING A BUMP TO THE
     * DEFINED ABI NUMBER! */

    VLC_MODULE_CREATE,
    VLC_CONFIG_CREATE_SPECIAL,
    VLC_CONFIG_CREATE_COMMON,
    VLC_CONFIG_CREATE_OBSOLETE,
    VLC_CONFIG_CREATE_MOD_SELECT,

    VLC_MODULE_SHORTCUT = 0x100,
    VLC_MODULE_CAPABILITY,
    VLC_MODULE_CUSTOM_CAPABILITY,
    VLC_MODULE_SCORE,
    VLC_MODULE_CB_OPEN,
    VLC_MODULE_CB_CLOSE,
    VLC_MODULE_NO_UNLOAD,
    VLC_MODULE_NAME,
    VLC_MODULE_SHORTNAME,
    VLC_MODULE_DESCRIPTION,
    VLC_MODULE_HELP,
    VLC_MODULE_TEXTDOMAIN,
    /* --> Insert new VLC_MODULE_* entries here <-- */

    VLC_CONFIG_NAME = 0x1000,
    /* command line name (args=const char *) */

    VLC_CONFIG_VOLATILE, /* don't write variable to storage */
    VLC_CONFIG_PRIVATE,  /* hide from user in GUI */
    VLC_CONFIG_REMOVED,  /* tag as no longer supported */
    VLC_CONFIG_SAFE,     /* tag as modifiable by untrusted input item "sources" */

    VLC_CONFIG_SHORT, /* one-character (short) command line option character */

    VLC_CONFIG_INT_RANGE,
    VLC_CONFIG_FLOAT_RANGE,

    VLC_CONFIG_STRING_LIST, /* list of suggested string values */
    VLC_CONFIG_INT_LIST, /* list of suggested integer values */
    VLC_CONFIG_STRING_LIST_CB, /* callback for suggested values */
    VLC_CONFIG_INT_LIST_CB, /* callback for suggested values */

    VLC_CONFIG_CAPABILITY, /* capability for a module or list thereof */

    /* --> Insert new VLC_CONFIG_* entries here <-- */
};

/*****************************************************************************
 * Plugin descriptor setup, ignore this!
 *****************************************************************************/

#define __VLC_SYMBOL_PREFIX vlc_entry

#define __VLC_DESCRIPTOR_SYMBOL( y, z ) __VLC_SYMBOL( y, z )
#define __VLC_EXTRA_SYMBOL( x, y, z ) __VLC_SYMBOL( x##_##y, z )
#define __VLC_SYMBOL( y, z )  y##__##z

#define XSTRINGIFY( s ) STRINGIFY( s )
#define STRINGIFY( s ) #s

#define __VLC_PLUGIN_DESCRIPTOR_SYMBOL __VLC_DESCRIPTOR_SYMBOL( __VLC_SYMBOL_PREFIX, PLUGIN_ABI_VERSION )

/* If the module is built-in, then we need to define vlc_entry_FOO instead
 * of vlc_entry_ABIVERSION. */
#ifdef __PLUGIN__
# define __VLC_MY_DESCRIPTOR_SYMBOL __VLC_PLUGIN_DESCRIPTOR_SYMBOL
# define __VLC_MY_EXTRA_SYMBOL( symbol  ) __VLC_EXTRA_SYMBOL( __VLC_SYMBOL_PREFIX, symbol, PLUGIN_ABI_VERSION )
# define VLC_PLUGIN_NAME_HIDDEN_SYMBOL \
    const char vlc_plugin_name[] = PLUGIN_STRING;
#else
# define __VLC_MY_DESCRIPTOR_SYMBOL __VLC_DESCRIPTOR_SYMBOL( __VLC_SYMBOL_PREFIX, PLUGIN_NAME )
# define __VLC_MY_EXTRA_SYMBOL( symbol  ) __VLC_EXTRA_SYMBOL( __VLC_SYMBOL_PREFIX, symbol, PLUGIN_NAME )
# define VLC_PLUGIN_NAME_HIDDEN_SYMBOL
#endif

#define CDECL_SYMBOL
#if defined (__PLUGIN__)
# if defined (_WIN32)
#   define DLL_SYMBOL              __declspec(dllexport)
#   undef CDECL_SYMBOL
#   define CDECL_SYMBOL            __cdecl
# elif defined (__GNUC__)
#   define DLL_SYMBOL              __attribute__((visibility("default")))
# else
#  define DLL_SYMBOL
# endif
#else
# define DLL_SYMBOL
#endif

#if defined( __cplusplus )
#   define EXTERN_SYMBOL           extern "C"
#else
#   define EXTERN_SYMBOL
#endif

typedef struct vlc_plugin_t vlc_plugin_t;

EXTERN_SYMBOL typedef int (*vlc_descriptor_cb) (vlc_plugin_t *, enum vlc_plugin_desc_actions, void *, ...);

typedef int (*vlc_activate_cb)(vlc_object_t*);
typedef void (*vlc_deactivate_cb)(vlc_object_t*);

#define vlc_plugin_set_va(action, ...) desc_cb (context, action,   NULL, __VA_ARGS__)
#define vlc_module_set_va(action, ...) desc_cb (context, action, module, __VA_ARGS__)
#define vlc_module_set(action)         desc_cb (context, action, module)
#define vlc_config_set(action)         desc_cb (context, action, &config, &cfg_params)

/*
 * Plugin descriptor: this function is called once and only once, when the
 * plugin is looked at for the first time. Its purpose is to describe the
 * plugin, providing details on module, submodules, and config. A copy is
 * taken of data provided, since plugins are dynamically loaded/unloaded.
 */
#define vlc_plugin_begin() \
EXTERN_SYMBOL DLL_SYMBOL \
int CDECL_SYMBOL __VLC_MY_DESCRIPTOR_SYMBOL (vlc_descriptor_cb, vlc_plugin_t *); \
EXTERN_SYMBOL DLL_SYMBOL \
int CDECL_SYMBOL __VLC_MY_DESCRIPTOR_SYMBOL (vlc_descriptor_cb desc_cb, vlc_plugin_t *context) \
{ \
    module_t *module = NULL; \
    module_config_item_t *config = NULL; \
    config_item_params_t cfg_params; \
    if (vlc_plugin_set_va (VLC_MODULE_CREATE, &module)) \
        goto error; \
    if (vlc_module_set_va (VLC_MODULE_NAME, (PLUGIN_STRING))) \
        goto error;

#define vlc_plugin_end() \
    (void) config; \
    (void) cfg_params; \
    return 0; \
error: \
    return -1; \
} \
VLC_PLUGIN_NAME_HIDDEN_SYMBOL \
VLC_METADATA_EXPORTS

/*****************************************************************************
 * Macros for describing modules
 *****************************************************************************/

/* Inner helpers, do not use directly in plugins */

#define set_capability_inner( cap, score ) \
{ \
    enum vlc_module_cap _cap = (cap); \
    int _score = (score); \
    if (vlc_module_set_va (VLC_MODULE_CAPABILITY, _cap) \
     || vlc_module_set_va (VLC_MODULE_SCORE, _score)) \
        goto error; \
}

#define set_custom_capability_inner( cap, score ) \
{ \
    const char * _cap = (cap); \
    int _score = (score); \
    if (vlc_module_set_va (VLC_MODULE_CUSTOM_CAPABILITY, _cap) \
     || vlc_module_set_va (VLC_MODULE_SCORE, _score)) \
        goto error; \
}

#define set_callbacks_inner( a_name, d_name, a_cb, d_cb ) \
    if (vlc_module_set_va(VLC_MODULE_CB_OPEN, (a_name), (vlc_activate_cb)(void*)(a_cb)) \
     || vlc_module_set_va(VLC_MODULE_CB_CLOSE, (d_name), (vlc_deactivate_cb)(void*)(d_cb))) \
        goto error;

#define set_callbacks_common_inner( a_name, d_name, a_cb, d_cb, ty ) \
{ \
    int (*_activate_cb)(ty *) = (a_cb); \
    void (*_deactivate_cb)(ty *) = (d_cb); \
    set_callbacks_inner((a_name), (d_name), _activate_cb, _deactivate_cb) \
} \

/* module types following the common activate/deactivate signature of:
   activation: int(*)(<type>*)
   deactivation: void(*)(<type>*)
 */
#define set_callbacks_VLC_CAP_ACCESS(            an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, demux_t )
#define set_callbacks_VLC_CAP_ADDONS_FINDER(     an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, addons_finder_t )
#define set_callbacks_VLC_CAP_ADDONS_STORAGE(    an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, addons_storage_t )
#define set_callbacks_VLC_CAP_ART_FINDER(        an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, meta_fetcher_t )
#define set_callbacks_VLC_CAP_AUDIO_CONVERTER(   an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, filter_t )
#define set_callbacks_VLC_CAP_AUDIO_DECODER(     an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, decoder_t )
#define set_callbacks_VLC_CAP_AUDIO_FILTER(      an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, filter_t )
#define set_callbacks_VLC_CAP_AUDIO_OUTPUT(      an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, audio_output_t )
#define set_callbacks_VLC_CAP_AUDIO_RENDERER(    an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, filter_t )
#define set_callbacks_VLC_CAP_AUDIO_RESAMPLER(   an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, filter_t )
#define set_callbacks_VLC_CAP_AUDIO_VOLUME(      an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, audio_volume_t )
#define set_callbacks_VLC_CAP_DEMUX(             an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, demux_t )
#define set_callbacks_VLC_CAP_DEMUX_FILTER(      an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, demux_t )
#define set_callbacks_VLC_CAP_DIALOGS_PROVIDER(  an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, intf_thread_t )
#define set_callbacks_VLC_CAP_ENCODER(           an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, encoder_t )
#define set_callbacks_VLC_CAP_EXTENSION(         an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, extensions_manager_t )
#define set_callbacks_VLC_CAP_FINGERPRINTER(     an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, fingerprinter_thread_t )
#define set_callbacks_VLC_CAP_GLCONV(            an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, opengl_tex_converter_t )
#define set_callbacks_VLC_CAP_INHIBIT(           an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, vlc_inhibit_t )
#define set_callbacks_VLC_CAP_INTERFACE(         an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, intf_thread_t )
#define set_callbacks_VLC_CAP_KEYSTORE(          an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, vlc_keystore )
#define set_callbacks_VLC_CAP_MEDIALIBRARY(      an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, vlc_medialibrary_module_t )
#define set_callbacks_VLC_CAP_META_FETCHER(      an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, meta_fetcher_t )
#define set_callbacks_VLC_CAP_META_READER(       an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, demux_meta_t )
#define set_callbacks_VLC_CAP_META_WRITER(       an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, meta_export_t )
#define set_callbacks_VLC_CAP_PACKETIZER(        an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, decoder_t )
#define set_callbacks_VLC_CAP_PLAYLIST_EXPORT(   an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, vlc_playlist_export )
#define set_callbacks_VLC_CAP_RENDERER_DISCOVERY(an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, vlc_renderer_discovery_t )
#define set_callbacks_VLC_CAP_RENDERER_PROBE(    an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, vlc_probe_t )
#define set_callbacks_VLC_CAP_SERVICES_DISCOVERY(an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, services_discovery_t )
#define set_callbacks_VLC_CAP_SERVICES_PROBE(    an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, vlc_probe_t )
#define set_callbacks_VLC_CAP_SOUT_ACCESS(       an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, sout_access_out_t )
#define set_callbacks_VLC_CAP_SOUT_MUX(          an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, sout_mux_t )
#define set_callbacks_VLC_CAP_SOUT_STREAM(       an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, sout_stream_t )
#define set_callbacks_VLC_CAP_SPU_DECODER(       an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, decoder_t )
#define set_callbacks_VLC_CAP_STREAM_DIRECTORY(  an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, stream_directory_t )
#define set_callbacks_VLC_CAP_STREAM_EXTRACTOR(  an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, stream_extractor_t )
#define set_callbacks_VLC_CAP_STREAM_FILTER(     an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, stream_t )
#define set_callbacks_VLC_CAP_SUB_FILTER(        an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, filter_t )
#define set_callbacks_VLC_CAP_SUB_SOURCE(        an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, filter_t )
#define set_callbacks_VLC_CAP_TEXT_RENDERER(     an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, filter_t )
#define set_callbacks_VLC_CAP_TLS_CLIENT(        an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, vlc_tls_client_t )
#define set_callbacks_VLC_CAP_VIDEO_BLENDING(    an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, filter_t )
#define set_callbacks_VLC_CAP_VIDEO_CONVERTER(   an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, filter_t )
#define set_callbacks_VLC_CAP_VIDEO_DECODER(     an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, decoder_t )
#define set_callbacks_VLC_CAP_VIDEO_FILTER(      an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, filter_t )
#define set_callbacks_VLC_CAP_VIDEO_SPLITTER(    an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, video_splitter_t )
#define set_callbacks_VLC_CAP_VISUALIZATION(     an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, filter_t )
#define set_callbacks_VLC_CAP_VOD_SERVER(        an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, vod_t )
#define set_callbacks_VLC_CAP_VOUT_WINDOW(       an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, vout_window_t )
#define set_callbacks_VLC_CAP_VULKAN(            an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, vlc_vk_t )
#define set_callbacks_VLC_CAP_XML(               an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, xml_t )
#define set_callbacks_VLC_CAP_XML_READER(        an, dn, ac, dc) set_callbacks_common_inner( an, dn, ac, dc, xml_reader_t )


/* module types with more exotic activate/deactivate signatures */
#ifndef __PLUGIN__
#define set_callbacks_VLC_CAP_CORE( an, dn, ac, dc ) \
    /* do nothing, core does not use a callback */
#endif
#define set_callbacks_VLC_CAP_AOUT_STREAM( an, dn, ac, dc ) \
{ \
    HRESULT (*_activate_cb)(aout_stream_t *, audio_sample_format_t *, const GUID *) = (ac); \
    void (*_deactivate_cb)(aout_stream_t *) = (dc); \
    set_callbacks_inner((an), (dn), _activate_cb, _deactivate_cb) \
}
#define set_callbacks_VLC_CAP_HW_DECODER( an, dn, ac, dc ) \
{ \
    int (*_activate_cb)(vlc_va_t *, AVCodecContext *, enum PixelFormat, const es_format_t *, void *) = (ac); \
    void (*_deactivate_cb)(vlc_va_t *, void **) = (dc); \
    set_callbacks_inner((an), (dn), _activate_cb, _deactivate_cb) \
}
#define set_callbacks_VLC_CAP_HW_DECODER_DEVICE( an, dn, ac, dc ) \
{ \
    int (*_activate_cb)(vlc_decoder_device *, vout_window_t *) = (ac); \
    void (*_deactivate_cb)(vlc_decoder_device *) = (dc); \
    set_callbacks_inner((an), (dn), _activate_cb, _deactivate_cb) \
}
#define set_callbacks_VLC_CAP_LOGGER( an, dn, ac, dc ) \
{ \
    const struct vlc_logger_operations* (*_activate_cb)(vlc_object_t *, void **) = (ac); \
    void (*_deactivate_cb)(vlc_object_t *) = (dc); \
    set_callbacks_inner((an), (dn), _activate_cb, _deactivate_cb) \
}
#define set_callbacks_VLC_CAP_OPENGL( an, dn, ac, dc ) \
{ \
    int (*_activate_cb)(vlc_gl_t *, unsigned, unsigned) = (ac); \
    void (*_deactivate_cb)(vlc_gl_t *) = (dc); \
    set_callbacks_inner((an), (dn), _activate_cb, _deactivate_cb) \
}
#define set_callbacks_VLC_CAP_TLS_SERVER( an, dn, ac, dc ) \
{ \
    int (*_activate_cb)(vlc_tls_server_t *, const char *, const char *) = (ac); \
    void (*_deactivate_cb)(vlc_tls_server_t *) = (dc); \
    set_callbacks_inner((an), (dn), _activate_cb, _deactivate_cb) \
}
#define set_callbacks_VLC_CAP_VOUT_DISPLAY( an, dn, ac, dc ) \
{ \
    int (*_activate_cb)(vout_display_t *, const vout_display_cfg_t *, video_format_t *, vlc_video_context *) = (ac); \
    void (*_deactivate_cb)(vout_display_t *) = (dc); \
    set_callbacks_inner((an), (dn), _activate_cb, _deactivate_cb) \
}

/* Plugin authors, use this stuff! */

#define add_submodule( ) \
    if (vlc_plugin_set_va (VLC_MODULE_CREATE, &module)) \
        goto error;

#define add_shortcut( ... ) \
{ \
    const char *shortcuts[] = { __VA_ARGS__ }; \
    if (vlc_module_set_va (VLC_MODULE_SHORTCUT, \
                           sizeof(shortcuts)/sizeof(shortcuts[0]), shortcuts)) \
        goto error; \
}

#define set_shortname( shortname ) \
{ \
    const char *_shortname = shortname; \
    if (vlc_module_set_va (VLC_MODULE_SHORTNAME, _shortname)) \
        goto error; \
}

#define set_description( desc ) \
{ \
    const char *_desc = desc; \
    if (vlc_module_set_va (VLC_MODULE_DESCRIPTION, _desc)) \
        goto error; \
}

#define set_help( help ) \
{ \
    const char *_help = help; \
    if (vlc_module_set_va (VLC_MODULE_HELP, _help)) \
        goto error; \
}

#define set_capability( cap, score, activate, deactivate ) \
    set_capability_inner( cap, score ) \
    set_callbacks_##cap( #activate, #deactivate, activate, deactivate ) \

#define set_capability_custom( cap, score, activate, deactivate ) \
    set_custom_capability_inner( cap, score ) \
    set_callbacks_inner( #activate, #deactivate, activate, deactivate ) \

#define cannot_unload_broken_library( ) \
    if (vlc_module_set(VLC_MODULE_NO_UNLOAD)) \
        goto error;

#define set_text_domain( dom ) \
{ \
    const char *_dom = (dom); \
    if (vlc_plugin_set_va (VLC_MODULE_TEXTDOMAIN, _dom)) \
        goto error; \
}

/*****************************************************************************
 * Macros used to build the configuration structure.
 *
 * Note that internally we support only 3 types of config data: int, float
 *   and string.
 *   The other types declared here just map to one of these 3 basic types but
 *   have the advantage of also providing very good hints to a configuration
 *   interface so as to make it more user friendly.
 * The configuration structure also includes category hints. These hints can
 *   provide a configuration interface with some very useful data and again
 *   allow for a more user friendly interface.
 *****************************************************************************/

/* Param block, for passing data to the descriptor callback */
typedef union
{
    struct {
        uint16_t type;
        enum vlc_config_subcat id; /* subcat ID */
        const char *text; /* section/hint text */
        const char *longtext; /* section/hint longtext */
    } special;
    struct {
        uint16_t type;
        const char *name;
    } obsolete;
    struct {
        uint16_t type;
        const char *name;
        module_value_t default_val;
        const char *text;
        const char *longtext;
    } basic_item;
    struct {
        uint16_t type;
        const char *name;
        const char *cap; /* string form of capability it relates to */
        enum vlc_config_subcat subcategory; /* for a special core-only selection method */
        module_value_t default_val;
        const char *text;
        const char *longtext;
    } mod_select_item;
    struct {
        char ch;
    } short_char;
    struct {
        int64_t min;
        int64_t max;
    } integer_range;
    struct {
        float min;
        float max;
    } float_range;
    struct {
        const char *const *list; /* values */
        const char *const *text; /* corresponding labels */
        uint16_t count;
    } string_list;
    struct {
        const int *list; /* values */
        const char *const *text; /* corresponding labels */
        uint16_t count;
    } int_list;
    struct {
        const char *name;
        vlc_string_list_cb cb;
    } string_list_cb;
    struct {
        const char *name;
        vlc_integer_list_cb cb;
    } int_list_cb;
} config_item_params_t;

/* Inner helpers, do not use directly in plugins */

#define add_common_type_inner() \
    vlc_config_set( VLC_CONFIG_CREATE_COMMON );

#define add_special_type_inner() \
    vlc_config_set( VLC_CONFIG_CREATE_SPECIAL );

#define add_module_type_inner() \
    vlc_config_set( VLC_CONFIG_CREATE_MOD_SELECT );

#define add_obsolete_type_inner( _name, _type ) \
    cfg_params = (config_item_params_t) { .obsolete = { \
        .type = (_type), \
        .name = (_name) \
    } }; \
    vlc_config_set( VLC_CONFIG_CREATE_OBSOLETE );

#define add_string_inner( _name, subtype, default, _text, _longtext ) \
{ \
    const char *_str = (default); \
    cfg_params = (config_item_params_t) { .basic_item = { \
        .type = (subtype), \
        .name = (_name), \
        .default_val = { .psz = (char*)_str }, \
        .text = (_text), \
        .longtext = (_longtext), \
    } }; \
    add_common_type_inner() \
}

#define add_integer_inner( _name, subtype, default, _text, _longtext ) \
    cfg_params = (config_item_params_t) { .basic_item = { \
        .type = (subtype), \
        .name = (_name), \
        .default_val = { .i = (default) }, \
        .text = (_text), \
        .longtext = (_longtext) \
    } }; \
    add_common_type_inner()

#define add_float_inner( _name, default, _text, _longtext ) \
    cfg_params = (config_item_params_t) { .basic_item = { \
        .type = CONFIG_ITEM_FLOAT, \
        .name = (_name), \
        .default_val = { .f = (default) }, \
        .text = (_text), \
        .longtext = (_longtext) \
    } }; \
    add_common_type_inner()

#define add_module_inner( _name, subtype, _cap, _subcategory, default, _text, _longtext ) \
{ \
    const char *_str = (default); \
    cfg_params = (config_item_params_t) { .mod_select_item = { \
        .type = (subtype), \
        .name = (_name), \
        .cap = (_cap), \
        .subcategory = (_subcategory), \
        .default_val = { .psz = (char*)_str }, \
        .text = (_text), \
        .longtext = (_longtext) \
    } }; \
    add_module_type_inner() \
}

/* "Special" (hint) type entries */

#define set_subcategory( _id ) \
    cfg_params = (config_item_params_t) { .special = { \
        .type = CONFIG_SUBCATEGORY, \
        .id = (_id), \
        .text = NULL, \
        .longtext = NULL \
    } }; \
    add_special_type_inner()

#define set_section( _text, _longtext ) \
    cfg_params = (config_item_params_t) { .special = { \
        .type = CONFIG_SECTION, \
        .id = SUBCAT_INVALID, \
        .text = (_text), \
        .longtext = (_longtext) \
    } }; \
    add_special_type_inner()

/* private, for alternate category headings in core help output */
#ifndef __PLUGIN__
#define add_category_hint( _text, _longtext ) \
    cfg_params = (config_item_params_t) { .special = { \
        .type = CONFIG_HINT_CATEGORY, \
        .id = SUBCAT_INVALID, \
        .text = (_text), \
        .longtext = (_longtext) \
    } }; \
    add_special_type_inner()
#endif

/* Basic option items */

/* those like --help */
#define add_info( _name, _text, _longtext ) \
    cfg_params = (config_item_params_t) { .basic_item = { \
        .type = CONFIG_ITEM_INFO, \
        .name = (_name), \
        .default_val = { .b = false }, \
        .text = (_text), \
        .longtext = (_longtext) \
    } }; \
    add_common_type_inner() \
    change_volatile()

/* creates --foo and --no-foo flag options */
#define add_bool( _name, default, _text, _longtext, advanced ) \
    cfg_params = (config_item_params_t) { .basic_item = { \
        .type = CONFIG_ITEM_BOOL, \
        .name = (_name), \
        .default_val = { .b = (default) }, \
        .text = (_text), \
        .longtext = (_longtext) \
    } }; \
    add_common_type_inner()

/* Basic string option items */

#define add_string( name, default, text, longtext, advanced ) \
    add_string_inner( name, CONFIG_ITEM_STRING, default, text, longtext )

#define add_password( name, default, text, longtext ) \
    add_string_inner( name, CONFIG_ITEM_PASSWORD, default, text, longtext )

#define add_loadfile( name, default, text, longtext ) \
    add_string_inner( name, CONFIG_ITEM_LOADFILE, default, text, longtext )

#define add_savefile( name, default, text, longtext ) \
    add_string_inner( name, CONFIG_ITEM_SAVEFILE, default, text, longtext )

#define add_directory( name, default, text, longtext ) \
    add_string_inner( name, CONFIG_ITEM_DIRECTORY, default, text, longtext )

#define add_font( name, default, text, longtext ) \
    add_string_inner( name, CONFIG_ITEM_FONT, default, text, longtext )

/* add --foo and --global-foo options */
#define add_key( name, default, text, longtext ) \
    add_string_inner( "global-" name, CONFIG_ITEM_KEY, KEY_UNSET, text, longtext ) \
    add_string_inner( name, CONFIG_ITEM_KEY, default, text, longtext )

/* Basic integer option items */

#define add_integer( name, default, text, longtext, advanced ) \
    add_integer_inner( name, CONFIG_ITEM_INTEGER, default, text, longtext )

#define add_rgb( name, default, text, longtext ) \
    add_integer_inner( name, CONFIG_ITEM_RGB, default, text, longtext ) \
    change_integer_range( 0, 0xFFFFFF )

#define add_rgba( name, default, text, longtext ) \
    add_integer_inner( name, CONFIG_ITEM_RGBA, default, text, longtext ) \
    change_integer_range( 0, 0xFFFFFFFF )

#define add_integer_with_range( name, default, i_min, i_max, text, longtext, advanced ) \
    add_integer_inner( name, CONFIG_ITEM_INTEGER, default, text, longtext ) \
    change_integer_range( i_min, i_max )

/* Basic float option items */

#define add_float( name, default, text, longtext, advanced ) \
    add_float_inner( name, default, text, longtext )

#define add_float_with_range( name, default, f_min, f_max, text, longtext, advanced ) \
    add_float_inner( name, default, text, longtext ) \
    change_float_range( f_min, f_max )

/* Module selection option items */

#define add_module( name, cap, default, text, longtext ) \
    add_module_inner( name, CONFIG_ITEM_MODULE, cap, 0, default, text, longtext )

#define add_module_list( name, cap, default, text, longtext ) \
    add_module_inner( name, CONFIG_ITEM_MODULE_LIST, cap, 0, default, text, longtext )

/* private to core, for special selection via 'category' use */
#ifndef __PLUGIN__
#define add_module_cat( name, subcategory, default, text, longtext ) \
    add_module_inner( name, CONFIG_ITEM_MODULE_CAT, NULL, subcategory, default, text, longtext )

#define add_module_list_cat( name, subcategory, default, text, longtext ) \
    add_module_inner( name, CONFIG_ITEM_MODULE_LIST_CAT, NULL, subcategory, default, text, longtext )
#endif

/* For removed options */

#define add_obsolete_info( name ) \
    add_obsolete_type_inner( name, CONFIG_ITEM_INFO )

#define add_obsolete_bool( name ) \
    add_obsolete_type_inner( name, CONFIG_ITEM_BOOL )

#define add_obsolete_string( name ) \
    add_obsolete_type_inner( name, CONFIG_ITEM_STRING )

#define add_obsolete_integer( name ) \
    add_obsolete_type_inner( name, CONFIG_ITEM_INTEGER )

#define add_obsolete_float( name ) \
    add_obsolete_type_inner( name, CONFIG_ITEM_FLOAT )

/* Modifiers (used for fine tuning) */

#define change_short( _ch ) \
    cfg_params = (config_item_params_t) { .short_char = { \
        .ch = (_ch) \
    } }; \
    vlc_config_set( VLC_CONFIG_SHORT );

#define change_integer_range( _min, _max ) \
    cfg_params = (config_item_params_t) { .integer_range = { \
        .min = (_min), .max = (_max) \
    } }; \
    vlc_config_set( VLC_CONFIG_INT_RANGE );

#define change_float_range( _min, _max ) \
    cfg_params = (config_item_params_t) { .float_range = { \
        .min = (_min), .max = (_max) \
    } }; \
    vlc_config_set( VLC_CONFIG_FLOAT_RANGE );

#define change_string_list( _list, list_text ) \
    static_assert((sizeof(_list) / sizeof(*_list)) == (sizeof(list_text) / sizeof(*list_text)), "array count mismatch"); \
    cfg_params = (config_item_params_t) { .string_list = { \
        .list = (const char *const *) &(_list), \
        .text = (const char *const *) &(list_text), \
        .count = sizeof(_list) / sizeof(*_list) \
    } }; \
    vlc_config_set( VLC_CONFIG_STRING_LIST );

#define change_integer_list( _list, list_text ) \
    static_assert((sizeof(_list) / sizeof(*_list)) == (sizeof(list_text) / sizeof(*list_text)), "array count mismatch"); \
    cfg_params = (config_item_params_t) { .int_list = { \
        .list = (const int *) &(_list), \
        .text = (const char *const *) &(list_text), \
        .count = sizeof(_list) / sizeof(*_list) \
    } }; \
    vlc_config_set( VLC_CONFIG_INT_LIST );

#define change_string_cb( _cb ) \
    cfg_params = (config_item_params_t) { .string_list_cb = { \
        .name = #_cb, .cb = (_cb) \
    } }; \
    vlc_config_set( VLC_CONFIG_STRING_LIST_CB );

#define change_integer_cb( _cb ) \
    cfg_params = (config_item_params_t) { .int_list_cb = { \
        .name = #_cb, .cb = (_cb) \
    } }; \
    vlc_config_set( VLC_CONFIG_INT_LIST_CB );

/* For options that are saved but hidden from the preferences panel */
#define change_private() \
    vlc_config_set( VLC_CONFIG_PRIVATE );

/* For options that cannot be saved in the configuration */
#define change_volatile() \
    change_private() \
    vlc_config_set( VLC_CONFIG_VOLATILE );

#define change_safe() \
    vlc_config_set( VLC_CONFIG_SAFE );

/*****************************************************************************
 * Misc.
 *****************************************************************************/

/* Meta data plugin exports */
#define VLC_META_EXPORT( name, value ) \
    EXTERN_SYMBOL DLL_SYMBOL const char * CDECL_SYMBOL \
    __VLC_MY_EXTRA_SYMBOL(name) (void); \
    EXTERN_SYMBOL DLL_SYMBOL const char * CDECL_SYMBOL \
    __VLC_MY_EXTRA_SYMBOL(name) (void) \
    { \
         return value; \
    }

#define VLC_COPYRIGHT_VIDEOLAN \
    "\x43\x6f\x70\x79\x72\x69\x67\x68\x74\x20\x28\x43\x29\x20\x74\x68" \
    "\x65\x20\x56\x69\x64\x65\x6f\x4c\x41\x4e\x20\x56\x4c\x43\x20\x6d" \
    "\x65\x64\x69\x61\x20\x70\x6c\x61\x79\x65\x72\x20\x64\x65\x76\x65" \
    "\x6c\x6f\x70\x65\x72\x73"
#define VLC_LICENSE_LGPL_2_1_PLUS \
    "\x4c\x69\x63\x65\x6e\x73\x65\x64\x20\x75\x6e\x64\x65\x72\x20\x74" \
    "\x68\x65\x20\x74\x65\x72\x6d\x73\x20\x6f\x66\x20\x74\x68\x65\x20" \
    "\x47\x4e\x55\x20\x4c\x65\x73\x73\x65\x72\x20\x47\x65\x6e\x65\x72" \
    "\x61\x6c\x20\x50\x75\x62\x6c\x69\x63\x20\x4c\x69\x63\x65\x6e\x73" \
    "\x65\x2c\x20\x76\x65\x72\x73\x69\x6f\x6e\x20\x32\x2e\x31\x20\x6f" \
    "\x72\x20\x6c\x61\x74\x65\x72\x2e"
#define VLC_LICENSE_GPL_2_PLUS \
    "\x4c\x69\x63\x65\x6e\x73\x65\x64\x20\x75\x6e\x64\x65\x72\x20\x74" \
    "\x68\x65\x20\x74\x65\x72\x6d\x73\x20\x6f\x66\x20\x74\x68\x65\x20" \
    "\x47\x4e\x55\x20\x47\x65\x6e\x65\x72\x61\x6c\x20\x50\x75\x62\x6c" \
    "\x69\x63\x20\x4c\x69\x63\x65\x6e\x73\x65\x2c\x20\x76\x65\x72\x73" \
    "\x69\x6f\x6e\x20\x32\x20\x6f\x72\x20\x6c\x61\x74\x65\x72\x2e"
#if defined (__LIBVLC__)
# define VLC_MODULE_COPYRIGHT VLC_COPYRIGHT_VIDEOLAN
# ifndef VLC_MODULE_LICENSE
#  define VLC_MODULE_LICENSE VLC_LICENSE_LGPL_2_1_PLUS
# endif
#endif

#ifdef VLC_MODULE_COPYRIGHT
# define VLC_COPYRIGHT_EXPORT VLC_META_EXPORT(copyright, VLC_MODULE_COPYRIGHT)
#else
# define VLC_COPYRIGHT_EXPORT
#endif
#ifdef VLC_MODULE_LICENSE
# define VLC_LICENSE_EXPORT VLC_META_EXPORT(license, VLC_MODULE_LICENSE)
#else
# define VLC_LICENSE_EXPORT
#endif

#define VLC_METADATA_EXPORTS \
    VLC_COPYRIGHT_EXPORT \
    VLC_LICENSE_EXPORT

#endif
