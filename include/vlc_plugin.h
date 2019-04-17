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
#include <vlc_config_cat.h>
#include <assert.h>

/**
 * \file
 * This file implements plugin (module) macros used to define a vlc plugin.
 */

/* Descriptor callback actions, ignore this! */
enum vlc_plugin_desc_actions
{
    /* WARNING: MAKING ANY CHANGES, OTHER THAN INSERTING NEW ITEMS AT THE END
     * (OF BLOCKS, WHERE MARKED) IS AN ABI BREAK, REQUIRING A BUMP TO THE BELOW
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
#define IsConfigIntegerBasedType(type) \
    (CONFIG_CLASS(type) & \
     (CONFIG_ITEM_CLASS_INTEGER | CONFIG_ITEM_CLASS_BOOL | CONFIG_ITEM_CLASS_INFO))
#define IsConfigIntegerType(type) \
    (CONFIG_CLASS(type) == CONFIG_ITEM_CLASS_INTEGER)
#define IsConfigFloatType(type) \
    (CONFIG_CLASS(type) == CONFIG_ITEM_CLASS_FLOAT)

/**
 * Current plugin ABI version
 */
# define MODULE_SYMBOL 4_0_10
# define MODULE_SUFFIX "__4_0_10"

/*****************************************************************************
 * Plugin descriptor setup, ignore this!
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

typedef struct vlc_plugin_t vlc_plugin_t;

EXTERN_SYMBOL typedef int (*vlc_descriptor_cb) (vlc_plugin_t *, enum vlc_plugin_desc_actions, void *, ...);

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
int CDECL_SYMBOL __VLC_SYMBOL(vlc_entry) (vlc_descriptor_cb, vlc_plugin_t *); \
EXTERN_SYMBOL DLL_SYMBOL \
int CDECL_SYMBOL __VLC_SYMBOL(vlc_entry) (vlc_descriptor_cb desc_cb, vlc_plugin_t *context) \
{ \
    module_t *module; \
    module_config_item_t *config = NULL; \
    config_item_params_t cfg_params; \
    if (vlc_plugin_set_va (VLC_MODULE_CREATE, &module)) \
        goto error; \
    if (vlc_module_set_va (VLC_MODULE_NAME, (MODULE_STRING))) \
        goto error;

#define vlc_plugin_end() \
    (void) config; \
    (void) cfg_params; \
    return 0; \
error: \
    return -1; \
} \
VLC_MODULE_NAME_HIDDEN_SYMBOL \
VLC_METADATA_EXPORTS

/*****************************************************************************
 * Macros for describing modules
 *****************************************************************************/

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

#define set_capability( cap, score ) \
{ \
    enum vlc_module_cap _cap = (cap); \
    int _score = (score); \
    if (vlc_module_set_va (VLC_MODULE_CAPABILITY, _cap) \
     || vlc_module_set_va (VLC_MODULE_SCORE, _score)) \
        goto error; \
}

#define set_custom_capability( cap, score ) \
{ \
    const char * _cap = (cap); \
    int _score = (score); \
    if (vlc_module_set_va (VLC_MODULE_CUSTOM_CAPABILITY, _cap) \
     || vlc_module_set_va (VLC_MODULE_SCORE, _score)) \
        goto error; \
}

#define set_callbacks( activate, deactivate ) \
    if (vlc_module_set_va(VLC_MODULE_CB_OPEN, #activate, (void *)(activate)) \
     || vlc_module_set_va(VLC_MODULE_CB_CLOSE, #deactivate, \
                          (void *)(deactivate))) \
        goto error;

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
        int64_t id; /* cat/subcat ID */
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
        int subcategory; /* option subcategory ID, for a special core-only selection method */
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

#define set_category( _id ) \
    cfg_params = (config_item_params_t) { .special = { \
        .type = CONFIG_CATEGORY, \
        .id = (_id), \
        .text = NULL, \
        .longtext = NULL \
    } }; \
    add_special_type_inner()

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
        .id = 0, \
        .text = (_text), \
        .longtext = (_longtext) \
    } }; \
    add_special_type_inner()

/* private, for alternate category headings in core help output */
#ifndef __PLUGIN__
#define add_category_hint( _text, _longtext ) \
    cfg_params = (config_item_params_t) { .special = { \
        .type = CONFIG_HINT_CATEGORY, \
        .id = 0, \
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
