/*****************************************************************************
 * vlc_plugin.h : Macros used from within a module.
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

#ifndef LIBVLC_MODULES_MACROS_H
# define LIBVLC_MODULES_MACROS_H 1

#include <vlc_module_caps.h>

/**
 * \file
 * This file implements plugin (module) macros used to define a vlc module.
 */

enum vlc_module_properties
{
    /* WARNING: MAKING ANY CHANGES, OTHER THAN INSERTING NEW ITEMS AT THE END
     * (OF BLOCKS, WHERE MARKED) IS AN ABI BREAK, REQUIRING A BUMP TO THE BELOW
     * DEFINED ABI NUMBER! */

    VLC_MODULE_CREATE,
    VLC_CONFIG_CREATE,

    VLC_MODULE_SHORTCUT=0x100,
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

    VLC_CONFIG_NAME=0x1000,
    /* command line name (args=const char *) */

    VLC_CONFIG_VALUE,
    /* actual value (args=int64_t/double/const char *) */

    VLC_CONFIG_RANGE,
    /* minimum value (args=int64_t/double/const char * twice) */

    VLC_CONFIG_VOLATILE,
    /* don't write variable to storage (args=none) */

    VLC_CONFIG_PRIVATE,
    /* hide from user (args=none) */

    VLC_CONFIG_REMOVED,
    /* tag as no longer supported (args=none) */

    VLC_CONFIG_CAPABILITY,
    /* capability for a module or list thereof (args=const char*) */

    VLC_CONFIG_SHORTCUT,
    /* one-character (short) command line option name (args=char) */

    VLC_CONFIG_SAFE,
    /* tag as modifiable by untrusted input item "sources" (args=none) */

    VLC_CONFIG_DESC,
    /* description (args=const char *, const char *, const char *) */

    VLC_CONFIG_LIST,
    /* list of suggested values
     * (args=size_t, const <type> *, const char *const *) */

    VLC_CONFIG_LIST_CB,
    /* callback for suggested values
     * (args=const char *, size_t (*)(vlc_object_t *, <type> **, char ***)) */

    /* --> Insert new VLC_CONFIG_* entries here <-- */
};

/* Configuration item class/subtype masks */
#define CONFIG_ITEM_CLASS_MASK   0xFF00
#define CONFIG_ITEM_SUBTYPE_MASK 0x00FF

/* Configuration item classes */
#define CONFIG_ITEM_CLASS_INVALID    0x0000 /* For init */
#define CONFIG_ITEM_CLASS_SPECIAL    0x0100 /* For hint/category items */
#define CONFIG_ITEM_CLASS_INFO       0x0200 /* Info flag option (e.g. --help) */
#define CONFIG_ITEM_CLASS_BOOL       0x0400 /* Boolean flag option */
#define CONFIG_ITEM_CLASS_FLOAT      0x0800 /* Float data-value option */
#define CONFIG_ITEM_CLASS_INTEGER    0x1000 /* Integer data-value option */
#define CONFIG_ITEM_CLASS_STRING     0x2000 /* String data-value option */

/* Configuration hint types */
#define CONFIG_HINT_CATEGORY         (CONFIG_ITEM_CLASS_SPECIAL | 0x01) /* Set category (help output) */
#define CONFIG_CATEGORY              (CONFIG_ITEM_CLASS_SPECIAL | 0x02) /* Set category (GUI) */
#define CONFIG_SUBCATEGORY           (CONFIG_ITEM_CLASS_SPECIAL | 0x03) /* Set subcategory (GUI) */
#define CONFIG_SECTION               (CONFIG_ITEM_CLASS_SPECIAL | 0x04) /* Start of new section */

/* Configuration item types */
#define CONFIG_ITEM_INVALID          (CONFIG_ITEM_CLASS_INVALID | 0x00)
#define CONFIG_ITEM_INFO             (CONFIG_ITEM_CLASS_INFO    | 0x00) /* Info request option */
#define CONFIG_ITEM_BOOL             (CONFIG_ITEM_CLASS_BOOL    | 0x00) /* Bool option */
#define CONFIG_ITEM_FLOAT            (CONFIG_ITEM_CLASS_FLOAT   | 0x00) /* Float option */
#define CONFIG_ITEM_INTEGER          (CONFIG_ITEM_CLASS_INTEGER | 0x00) /* Integer option */
#define CONFIG_ITEM_RGB              (CONFIG_ITEM_CLASS_INTEGER | 0x01) /* RGB color option */
#define CONFIG_ITEM_RGBA             (CONFIG_ITEM_CLASS_INTEGER | 0x02) /* RGBA color option */
#define CONFIG_ITEM_STRING           (CONFIG_ITEM_CLASS_STRING  | 0x00) /* String option */
#define CONFIG_ITEM_PASSWORD         (CONFIG_ITEM_CLASS_STRING  | 0x01) /* Password option (*) */
#define CONFIG_ITEM_KEY              (CONFIG_ITEM_CLASS_STRING  | 0x02) /* Hot key option */
#define CONFIG_ITEM_MODULE           (CONFIG_ITEM_CLASS_STRING  | 0x03) /* Module option */
#define CONFIG_ITEM_MODULE_CAT       (CONFIG_ITEM_CLASS_STRING  | 0x04) /* Module option */
#define CONFIG_ITEM_MODULE_LIST      (CONFIG_ITEM_CLASS_STRING  | 0x05) /* Module option */
#define CONFIG_ITEM_MODULE_LIST_CAT  (CONFIG_ITEM_CLASS_STRING  | 0x06) /* Module option */
#define CONFIG_ITEM_LOADFILE         (CONFIG_ITEM_CLASS_STRING  | 0x07) /* Read file option */
#define CONFIG_ITEM_SAVEFILE         (CONFIG_ITEM_CLASS_STRING  | 0x08) /* Written file option */
#define CONFIG_ITEM_DIRECTORY        (CONFIG_ITEM_CLASS_STRING  | 0x09) /* Directory option */
#define CONFIG_ITEM_FONT             (CONFIG_ITEM_CLASS_STRING  | 0x0A) /* Font option */

#define CONFIG_CLASS(x) ((x) & CONFIG_ITEM_CLASS_MASK)

/* is proper option, not a special hint type? */
#define CONFIG_ITEM(x) (((x) & CONFIG_ITEM_CLASS_MASK) != CONFIG_ITEM_CLASS_SPECIAL)

#define IsConfigStringType(type) \
    (CONFIG_CLASS(type) == CONFIG_ITEM_CLASS_STRING)
#define IsConfigIntegerType(type) \
    (CONFIG_CLASS(type) & \
     (CONFIG_ITEM_CLASS_INTEGER | CONFIG_ITEM_CLASS_BOOL | CONFIG_ITEM_CLASS_INFO))
#define IsConfigFloatType(type) \
    (CONFIG_CLASS(type) == CONFIG_ITEM_CLASS_FLOAT)

/* Hidden categories and subcategories */
/* Any options under this will be hidden in the GUI preferences, but will be
   listed in cmdline help output. */
#define CAT_HIDDEN -1
#define SUBCAT_HIDDEN -1

/* Categories and subcategories */
#define CAT_INTERFACE 1
#define SUBCAT_INTERFACE_GENERAL 101
#define SUBCAT_INTERFACE_MAIN 102
#define SUBCAT_INTERFACE_CONTROL 103
#define SUBCAT_INTERFACE_HOTKEYS 104

#define CAT_AUDIO 2
#define SUBCAT_AUDIO_GENERAL 201
#define SUBCAT_AUDIO_AOUT 202
#define SUBCAT_AUDIO_AFILTER 203
#define SUBCAT_AUDIO_VISUAL 204
#define SUBCAT_AUDIO_RESAMPLER 206

#define CAT_VIDEO 3
#define SUBCAT_VIDEO_GENERAL 301
#define SUBCAT_VIDEO_VOUT 302
#define SUBCAT_VIDEO_VFILTER 303
#define SUBCAT_VIDEO_SUBPIC 305
#define SUBCAT_VIDEO_SPLITTER 306

#define CAT_INPUT 4
#define SUBCAT_INPUT_GENERAL 401
#define SUBCAT_INPUT_ACCESS 402
#define SUBCAT_INPUT_DEMUX 403
#define SUBCAT_INPUT_VCODEC 404
#define SUBCAT_INPUT_ACODEC 405
#define SUBCAT_INPUT_SCODEC 406
#define SUBCAT_INPUT_STREAM_FILTER 407

#define CAT_SOUT 5
#define SUBCAT_SOUT_GENERAL 501
#define SUBCAT_SOUT_STREAM 502
#define SUBCAT_SOUT_MUX 503
#define SUBCAT_SOUT_ACO 504
#define SUBCAT_SOUT_PACKETIZER 505
#define SUBCAT_SOUT_VOD 507
#define SUBCAT_SOUT_RENDERER 508

#define CAT_ADVANCED 6
#define SUBCAT_ADVANCED_MISC 602
#define SUBCAT_ADVANCED_NETWORK 603

#define CAT_PLAYLIST 7
#define SUBCAT_PLAYLIST_GENERAL 701
#define SUBCAT_PLAYLIST_SD 702
#define SUBCAT_PLAYLIST_EXPORT 703


/**
 * Current plugin ABI version
 */
# define MODULE_SYMBOL 4_0_9
# define MODULE_SUFFIX "__4_0_9"

/*****************************************************************************
 * Add a few defines. You do not want to read this section. Really.
 *****************************************************************************/

/* Explanation:
 *
 * if linking a module statically, we will need:
 * #define MODULE_FUNC( zog ) module_foo_zog
 *
 * this can't easily be done with the C preprocessor, thus a few ugly hacks.
 */

/* I need to do _this_ to change « foo bar » to « module_foo_bar » ! */
#define CONCATENATE( y, z ) CRUDE_HACK( y, z )
#define CRUDE_HACK( y, z )  y##__##z

/* If the module is built-in, then we need to define foo_InitModule instead
 * of InitModule. Same for Activate- and DeactivateModule. */
#ifdef __PLUGIN__
# define __VLC_SYMBOL( symbol  ) CONCATENATE( symbol, MODULE_SYMBOL )
# define VLC_MODULE_NAME_HIDDEN_SYMBOL \
    const char vlc_module_name[] = MODULE_STRING;
#else
# define __VLC_SYMBOL( symbol )  CONCATENATE( symbol, MODULE_NAME )
# define VLC_MODULE_NAME_HIDDEN_SYMBOL
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

EXTERN_SYMBOL typedef int (*vlc_set_cb) (void *, void *, int, ...);

#define vlc_plugin_set(...) vlc_set (opaque,   NULL, __VA_ARGS__)
#define vlc_module_set(...) vlc_set (opaque, module, __VA_ARGS__)
#define vlc_config_set(...) vlc_set (opaque, config, __VA_ARGS__)

/*
 * InitModule: this function is called once and only once, when the module
 * is looked at for the first time. We get the useful data from it, for
 * instance the module name, its shortcuts, its capabilities... we also create
 * a copy of its config because the module can be unloaded at any time.
 */
#define vlc_module_begin() \
EXTERN_SYMBOL DLL_SYMBOL \
int CDECL_SYMBOL __VLC_SYMBOL(vlc_entry) (vlc_set_cb, void *); \
EXTERN_SYMBOL DLL_SYMBOL \
int CDECL_SYMBOL __VLC_SYMBOL(vlc_entry) (vlc_set_cb vlc_set, void *opaque) \
{ \
    module_t *module; \
    module_config_item_t *config = NULL; \
    if (vlc_plugin_set (VLC_MODULE_CREATE, &module)) \
        goto error; \
    if (vlc_module_set (VLC_MODULE_NAME, (MODULE_STRING))) \
        goto error;

#define vlc_module_end() \
    (void) config; \
    return 0; \
error: \
    return -1; \
} \
VLC_MODULE_NAME_HIDDEN_SYMBOL \
VLC_METADATA_EXPORTS

#define add_submodule( ) \
    if (vlc_plugin_set (VLC_MODULE_CREATE, &module)) \
        goto error;

#define add_shortcut( ... ) \
{ \
    const char *shortcuts[] = { __VA_ARGS__ }; \
    if (vlc_module_set (VLC_MODULE_SHORTCUT, \
                        sizeof(shortcuts)/sizeof(shortcuts[0]), shortcuts)) \
        goto error; \
}

#define set_shortname( shortname ) \
{ \
    const char *_shortname = shortname; \
    if (vlc_module_set (VLC_MODULE_SHORTNAME, _shortname)) \
        goto error; \
}

#define set_description( desc ) \
{ \
    const char *_desc = desc; \
    if (vlc_module_set (VLC_MODULE_DESCRIPTION, _desc)) \
        goto error; \
}

#define set_help( help ) \
{ \
    const char *_help = help; \
    if (vlc_module_set (VLC_MODULE_HELP, _help)) \
        goto error; \
}

#define set_capability( cap, score ) \
{ \
    enum vlc_module_cap _cap = (cap); \
    int _score = (score); \
    if (vlc_module_set (VLC_MODULE_CAPABILITY, _cap) \
     || vlc_module_set (VLC_MODULE_SCORE, _score)) \
        goto error; \
}

#define set_custom_capability( cap, score ) \
{ \
    const char * _cap = (cap); \
    int _score = (score); \
    if (vlc_module_set (VLC_MODULE_CUSTOM_CAPABILITY, _cap) \
     || vlc_module_set (VLC_MODULE_SCORE, _score)) \
        goto error; \
}

#define set_callbacks( activate, deactivate ) \
    if (vlc_module_set(VLC_MODULE_CB_OPEN, #activate, (void *)(activate)) \
     || vlc_module_set(VLC_MODULE_CB_CLOSE, #deactivate, \
                       (void *)(deactivate))) \
        goto error;

#define cannot_unload_broken_library( ) \
    if (vlc_module_set (VLC_MODULE_NO_UNLOAD)) \
        goto error;

#define set_text_domain( dom ) \
{ \
    const char *_dom = (dom); \
    if (vlc_plugin_set (VLC_MODULE_TEXTDOMAIN, _dom)) \
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

#define add_type_inner( type ) \
    vlc_plugin_set (VLC_CONFIG_CREATE, (type), &config);

#define add_typedesc_inner( type, text, longtext ) \
    add_type_inner( type ) \
    vlc_config_set (VLC_CONFIG_DESC, (text), (longtext));

#define add_typename_inner(type, name, text, longtext) \
{ \
    const char *_name = (name); \
    add_typedesc_inner(type, text, longtext) \
    vlc_config_set (VLC_CONFIG_NAME, _name); \
}

#define add_string_inner(type, name, text, longtext, v) \
{ \
    const char *_v = (v); \
    add_typename_inner(type, name, text, longtext) \
    vlc_config_set (VLC_CONFIG_VALUE, _v); \
}

#define add_int_inner(type, name, text, longtext, v) \
{ \
    int64_t _v = (v); \
    add_typename_inner(type, name, text, longtext) \
    vlc_config_set (VLC_CONFIG_VALUE, _v); \
}


#define set_category( i_id ) \
{ \
    int64_t _i_id = (i_id); \
    add_type_inner( CONFIG_CATEGORY ) \
    vlc_config_set (VLC_CONFIG_VALUE, _i_id); \
}

#define set_subcategory( i_id ) \
{ \
    int64_t _i_id = (i_id); \
    add_type_inner( CONFIG_SUBCATEGORY ) \
    vlc_config_set (VLC_CONFIG_VALUE, _i_id); \
}

#define set_section( text, longtext ) \
{ \
    const char * _text = (text); \
    const char * _longtext = (longtext); \
    add_typedesc_inner( CONFIG_SECTION, _text, _longtext ) \
}

#ifndef __PLUGIN__
#define add_category_hint( text, longtext ) \
{ \
    const char * _text = (text); \
    const char * _longtext = (longtext); \
    add_typedesc_inner( CONFIG_HINT_CATEGORY, _text, _longtext ) \
}
#endif

#define add_string( name, value, text, longtext, advc ) \
    add_string_inner(CONFIG_ITEM_STRING, name, text, longtext, value)

#define add_password(name, value, text, longtext) \
    add_string_inner(CONFIG_ITEM_PASSWORD, name, text, longtext, value)

#define add_loadfile(name, value, text, longtext) \
    add_string_inner(CONFIG_ITEM_LOADFILE, name, text, longtext, value)

#define add_savefile(name, value, text, longtext) \
    add_string_inner(CONFIG_ITEM_SAVEFILE, name, text, longtext, value)

#define add_directory(name, value, text, longtext) \
    add_string_inner(CONFIG_ITEM_DIRECTORY, name, text, longtext, value)

#define add_font(name, value, text, longtext) \
    add_string_inner(CONFIG_ITEM_FONT, name, text, longtext, value)

#define add_module(name, psz_caps, value, text, longtext) \
{ \
    const char *_psz_caps = (psz_caps); \
    add_string_inner(CONFIG_ITEM_MODULE, name, text, longtext, value) \
    vlc_config_set (VLC_CONFIG_CAPABILITY, _psz_caps); \
}

#define add_module_list(name, psz_caps, value, text, longtext) \
{ \
    const char *_psz_caps = (psz_caps); \
    add_string_inner(CONFIG_ITEM_MODULE_LIST, name, text, longtext, value) \
    vlc_config_set (VLC_CONFIG_CAPABILITY, _psz_caps); \
}

#ifndef __PLUGIN__
#define add_module_cat(name, i_subcategory, value, text, longtext) \
    add_string_inner(CONFIG_ITEM_MODULE_CAT, name, text, longtext, value) \
    change_integer_range (i_subcategory /* gruik */, 0);

#define add_module_list_cat(name, i_subcategory, value, text, longtext) \
    add_string_inner(CONFIG_ITEM_MODULE_LIST_CAT, name, text, longtext, \
                     value) \
    change_integer_range (i_subcategory /* gruik */, 0);
#endif

#define add_integer( name, value, text, longtext, advc ) \
    add_int_inner(CONFIG_ITEM_INTEGER, name, text, longtext, value)

#define add_rgb(name, value, text, longtext) \
    add_int_inner(CONFIG_ITEM_RGB, name, text, longtext, value) \
    change_integer_range( 0, 0xFFFFFF )

#define add_rgba(name, value, text, longtext) \
    add_int_inner(CONFIG_ITEM_RGBA, name, text, longtext, value) \
    change_integer_range( 0, 0xFFFFFFFF )

#define add_key(name, value, text, longtext) \
    add_string_inner(CONFIG_ITEM_KEY, "global-" name, text, longtext, \
                     KEY_UNSET) \
    add_string_inner(CONFIG_ITEM_KEY, name, text, longtext, value)

#define add_integer_with_range( name, value, i_min, i_max, text, longtext, advc ) \
    add_integer( name, value, text, longtext, advc ) \
    change_integer_range( i_min, i_max )

#define add_float( name, v, text, longtext, advc ) \
{ \
    /* note, module_value_t uses float, but floats are converted to double \
       when passed variadically */ \
    float _v = (v); \
    add_typename_inner(CONFIG_ITEM_FLOAT, name, text, longtext) \
    vlc_config_set (VLC_CONFIG_VALUE, (double)_v); \
}

#define add_float_with_range( name, value, f_min, f_max, text, longtext, advc ) \
    add_float( name, value, text, longtext, advc ) \
    change_float_range( f_min, f_max )

#define add_bool( name, v, text, longtext, advc ) \
    /* note, the value actually gets stored in an int! */ \
    add_typename_inner(CONFIG_ITEM_BOOL, name, text, longtext) \
    if (v) vlc_config_set (VLC_CONFIG_VALUE, (int64_t)true);

#define add_info( name, text, longtext ) \
    add_typename_inner(CONFIG_ITEM_INFO, name, text, longtext) \
    change_volatile();

/* For removed option */
#define add_obsolete_inner( name, type ) \
{ \
    const char *_name = (name); \
    add_type_inner( type ) \
    vlc_config_set (VLC_CONFIG_NAME, _name); \
    vlc_config_set (VLC_CONFIG_REMOVED); \
}

#define add_obsolete_info( name ) \
        add_obsolete_inner( name, CONFIG_ITEM_INFO )

#define add_obsolete_bool( name ) \
        add_obsolete_inner( name, CONFIG_ITEM_BOOL )

#define add_obsolete_integer( name ) \
        add_obsolete_inner( name, CONFIG_ITEM_INTEGER )

#define add_obsolete_float( name ) \
        add_obsolete_inner( name, CONFIG_ITEM_FLOAT )

#define add_obsolete_string( name ) \
        add_obsolete_inner( name, CONFIG_ITEM_STRING )

/* Modifier macros for the config options (used for fine tuning) */

#define change_short( ch ) \
{ \
    /* note, char is expanded to int when passed variadically */ \
    char _ch = (ch); \
    vlc_config_set (VLC_CONFIG_SHORTCUT, (int)_ch); \
}

#define change_string_list( list, list_text ) \
{ \
    const char *const *_list = (list); \
    const char *const *_list_text = (list_text); \
    vlc_config_set (VLC_CONFIG_LIST, \
        (size_t)(sizeof (list) / sizeof (char *)), \
        _list, _list_text); \
}

#define change_string_cb( cb ) \
{ \
    void *_cb = (cb); \
    vlc_config_set (VLC_CONFIG_LIST_CB, #cb, _cb); \
}

#define change_integer_list( list, list_text ) \
{ \
    const int *_list = (list); \
    const char *const *_list_text = (list_text); \
    vlc_config_set (VLC_CONFIG_LIST, \
        (size_t)(sizeof (list) / sizeof (int)), \
        _list, _list_text); \
}

#define change_integer_cb( cb ) \
{ \
    void *_cb = (cb); \
    vlc_config_set (VLC_CONFIG_LIST_CB, #cb, _cb); \
}

#define change_integer_range( minv, maxv ) \
{ \
    int64_t _minv = (minv), _maxv = (maxv); \
    vlc_config_set (VLC_CONFIG_RANGE, _minv, _maxv); \
}

#define change_float_range( minv, maxv ) \
{ \
    /* note, module_value_t uses float, but floats are converted to double \
       when passed variadically */ \
    float _minv = (minv), _maxv = (maxv); \
    vlc_config_set (VLC_CONFIG_RANGE, (double)_minv, (double)_maxv); \
}

/* For options that are saved but hidden from the preferences panel */
#define change_private() \
    vlc_config_set (VLC_CONFIG_PRIVATE);

/* For options that cannot be saved in the configuration */
#define change_volatile() \
    change_private() \
    vlc_config_set (VLC_CONFIG_VOLATILE);

#define change_safe() \
    vlc_config_set (VLC_CONFIG_SAFE);

/* Meta data plugin exports */
#define VLC_META_EXPORT( name, value ) \
    EXTERN_SYMBOL DLL_SYMBOL const char * CDECL_SYMBOL \
    __VLC_SYMBOL(vlc_entry_ ## name) (void); \
    EXTERN_SYMBOL DLL_SYMBOL const char * CDECL_SYMBOL \
    __VLC_SYMBOL(vlc_entry_ ## name) (void) \
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
