/*****************************************************************************
 * vlc_configuration.h : configuration management module
 * This file describes the programming interface for the configuration module.
 * It includes functions allowing to declare, get or set configuration options.
 *****************************************************************************
 * Copyright (C) 1999-2006 VLC authors and VideoLAN
 *
 * Authors: Gildas Bazin <gbazin@videolan.org>
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

#ifndef VLC_CONFIGURATION_H
#define VLC_CONFIGURATION_H 1

/**
 * \defgroup config User settings
 * \ingroup interface
 * VLC provides a simple name-value dictionary for user settings.
 *
 * Those settings are per-user per-system - they are shared by all LibVLC
 * instances in a single process, and potentially other processes as well.
 *
 * Each name-value pair is called a configuration item.
 * @{
 */

/**
 * \file
 * This file describes the programming interface for the configuration module.
 * It includes functions allowing to declare, get or set configuration options.
 */

#include <sys/types.h>  /* for ssize_t */

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
#define CONFIG_SUBCATEGORY           (CONFIG_ITEM_CLASS_SPECIAL | 0x01) /* Set subcategory (GUI) */
#define CONFIG_SECTION               (CONFIG_ITEM_CLASS_SPECIAL | 0x02) /* Start of new section */

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
#define CONFIG_ITEM_FOURCC           (CONFIG_ITEM_CLASS_STRING  | 0x0B) /* FourCC option */

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

# ifdef __cplusplus
extern "C" {
# endif

typedef union
{
    char        *psz;
    int64_t     i;
    bool        b;
    float       f;
} module_value_t;

typedef int (*vlc_string_list_cb)(const char *, char ***, char ***);
typedef int (*vlc_integer_list_cb)(const char *, int64_t **, char ***);

/**
 * Configuration item
 *
 * This is the internal representation of a configuration item.
 * See also vlc_config_FindItem().
 */
struct module_config_item_t
{
    uint16_t    i_type;       /**< Item type */
    char        i_short;      /**< Optional short option name */
    bool        b_internal;   /**< Hidden from GUI preferences but not help */
    bool        b_unsaveable; /**< Not stored in configuration */
    bool        b_safe;       /**< Safe for web plugins and playlist files */
    bool        b_removed;    /**< Obsolete */

    const char *psz_type; /**< Configuration subtype */
    const char *psz_name; /**< Option name */
    const char *psz_text; /**< Short comment on the configuration option */
    const char *psz_longtext; /**< Long comment on the configuration option */

    module_value_t value; /**< Current value */
    module_value_t orig; /**< Default value */
    module_value_t min; /**< Minimum value (for scalars only) */
    module_value_t max; /**< Maximum value (for scalars only) */

    /* Values list */
    uint16_t list_count; /**< Choices count */
    union
    {
        const char **psz; /**< Table of possible string choices */
        const int  *i; /**< Table of possible integer choices */
        vlc_string_list_cb psz_cb; /**< Callback to enumerate string choices */
        vlc_integer_list_cb i_cb; /**< Callback to enumerate integer choices */
    } list; /**< Possible choices */
    const char **list_text; /**< Human-readable names for list values */
    const char *list_cb_name; /**< Symbol name of the enumeration callback */
    void *owner; /**< Origin run-time linker module handle */
};

/**
 * Locks the config for writing
 *
 * Release with vlc_config_ReleaseLock().
 *
 * This is only necessary for attributes that can change during runtime, i.e.
 * the value, (and where not already done internally by a function).
 */
VLC_API void vlc_config_GetWriteLock(void);

/**
 * Locks the config for reading
 *
 * Release with vlc_config_ReleaseLock().
 *
 * This is only necessary for attributes that can change during runtime, i.e.
 * the value, (and where not already done internally by a function).
 */
VLC_API void vlc_config_GetReadLock(void);

/**
 * Releases the config read/write lock
 *
 * I.e. release the lock taken with vlc_config_GetWriteLock() or vlc_config_GetReadLock().
 */
VLC_API void vlc_config_ReleaseLock(void);

/**
 * Gets a configuration item type in VLC_VAR_* form
 *
 * This function checks the type of configuration item by name.
 * \param name Configuration item name
 * \return The configuration item type (VLC_VAR_*) or 0 if not found.
 */
VLC_API int config_GetType(const char *name) VLC_USED;

/**
 * Gets an integer configuration item's value.
 *
 * This function retrieves the current value of a configuration item of
 * integral type (\ref CONFIG_ITEM_CLASS_INTEGER and \ref CONFIG_ITEM_CLASS_BOOL).
 *
 * \warning The behaviour is undefined if the configuration item exists but is
 * not of integer or boolean type.
 *
 * \param name Configuration item name
 * \param locked If true, signals that you already hold the config read lock
 * \return The configuration item value or -1 if not found.
 * \bug A legitimate integer value of -1 cannot be distinguished from an error.
 */
VLC_API int64_t vlc_config_GetInt(const char *name, bool locked) VLC_USED;
#define config_GetInt(n)        vlc_config_GetInt(n, false)
#define config_GetInt_locked(n) vlc_config_GetInt(n, true)

/**
 * Sets an integer configuration item's value.
 *
 * This function changes the current value of a configuration item of
 * integral type (\ref CONFIG_ITEM_CLASS_INTEGER and \ref CONFIG_ITEM_CLASS_BOOL).
 *
 * \warning The behaviour is undefined if the configuration item exists but is
 * not of integer or boolean type.
 *
 * \note If no configuration item by the specified exist, the function has no
 * effects.
 *
 * \param name Configuration item name
 * \param val New value
 * \param locked If true, signals that you already hold the config write lock
 */
VLC_API void vlc_config_SetInt(const char *name, int64_t val, bool locked);
#define config_PutInt(n, v)        vlc_config_SetInt(n, v, false)
#define config_PutInt_locked(n, v) vlc_config_SetInt(n, v, true)

/**
 * Gets a floating point configuration item's value.
 *
 * This function retrieves the current value of a configuration item of
 * floating point type (\ref CONFIG_ITEM_CLASS_FLOAT).
 *
 * \warning The behaviour is undefined if the configuration item exists but is
 * not of floating point type.
 *
 * \param name Configuration item name
 * \param locked If true, signals that you already hold the config read lock
 * \return The configuration item value or -1 if not found.
 * \bug A legitimate floating point value of -1 cannot be distinguished from an
 * error.
 */
VLC_API float vlc_config_GetFloat(const char *name, bool locked) VLC_USED;
#define config_GetFloat(n)        vlc_config_GetFloat(n, false)
#define config_GetFloat_locked(n) vlc_config_GetFloat(n, true)

/**
 * Sets a floating point configuration item's value.
 *
 * This function changes the current value of a configuration item of
 * floating point type (\ref CONFIG_ITEM_CLASS_FLOAT).
 *
 * \warning The behaviour is undefined if the configuration item exists but is
 * not of floating point type.
 *
 * \note If no configuration item by the specified exist, the function has no
 * effects.
 *
 * \param name Configuration item name
 * \param val New value
 * \param locked If true, signals that you already hold the config write lock
 */
VLC_API void vlc_config_SetFloat(const char *name, float val, bool locked);
#define config_PutFloat(n, v)        vlc_config_SetFloat(n, v, false)
#define config_PutFloat_locked(n, v) vlc_config_SetFloat(n, v, true)

/**
 * Gets a string configuration item's value.
 *
 * This function retrieves the current value of a configuration item of
 * string type (\ref CONFIG_ITEM_CLASS_STRING).
 *
 * \note The caller must free() the returned pointer (if non-NULL), which is a
 * duplicate of the current value. It is not safe to return a pointer to the
 * current value internally as it can be modified at any time by any other
 * thread.
 *
 * \warning The behaviour is undefined if the configuration item exists but is
 * not of string type.
 *
 * \param name Configuration item name
 * \param locked If true, signals that you already hold the config read lock
 * \return Normally, a heap-allocated copy of the configuration item value.
 * If the value is the empty string, if the configuration does not exist,
 * or if an error occurs, NULL is returned.
 * \bug The empty string value cannot be distinguished from an error.
 */
VLC_API char *vlc_config_GetPsz(const char *name, bool locked) VLC_USED VLC_MALLOC;
#define config_GetPsz(n)        vlc_config_GetPsz(n, false)
#define config_GetPsz_locked(n) vlc_config_GetPsz(n, true)

/**
 * Sets a string configuration item's value.
 *
 * This function changes the current value of a configuration item of
 * string type (e.g. \ref CONFIG_ITEM_CLASS_STRING).
 *
 * \warning The behaviour is undefined if the configuration item exists but is
 * not of a string type.
 *
 * \note If no configuration item by the specified exist, the function has no
 * effects.
 *
 * \param name Configuration item name
 * \param val New value (will be copied)
 * \param locked If true, signals that you already hold the config write lock
 * \bug This function allocates memory but errors cannot be detected.
 */
VLC_API void vlc_config_SetPsz(const char *name, const char *val, bool locked);
#define config_PutPsz(n, v)        vlc_config_SetPsz(n, v, false)
#define config_PutPsz_locked(n, v) vlc_config_SetPsz(n, v, true)

/**
 * Enumerates integer configuration choices.
 *
 * Determines a list of suggested values for an integer configuration item.
 * \param values pointer to a table of integer values [OUT]
 * \param texts pointer to a table of descriptions strings [OUT]
 * \return number of choices, or -1 on error
 * \note the caller is responsible for calling free() on all descriptions and
 * on both tables. In case of error, both pointers are set to NULL.
 */
VLC_API ssize_t config_GetIntChoices(const char *, int64_t **values,
                                     char ***texts) VLC_USED;

/**
 * Determines a list of suggested values for a string configuration item.
 * \param values pointer to a table of value strings [OUT]
 * \param texts pointer to a table of descriptions strings [OUT]
 * \return number of choices, or -1 on error
 * \note the caller is responsible for calling free() on all values, on all
 * descriptions and on both tables.
 * In case of error, both pointers are set to NULL.
 */
VLC_API ssize_t config_GetPszChoices(const char *,
                                     char ***values, char ***texts) VLC_USED;

/**
 * Saves the config to the saved-settings file
 *
 * Note, locking is already done internally.
 */
VLC_API int config_SaveConfigFile( vlc_object_t * );
#define config_SaveConfigFile(a) config_SaveConfigFile(VLC_OBJECT(a))

/**
 * Resets the configuration.
 *
 * This function resets all configuration items to their respective
 * compile-time default value.
 *
 * Note, locking is already done internally.
 */
VLC_API void config_ResetAll(void);

/**
 * Looks up a configuration item.
 *
 * This function looks for the internal representation of a configuration item.
 * Where possible, this should be avoided in favor of more specific function
 * calls.
 *
 * \param name Configuration item name
 * \return The internal structure, or NULL if not found.
 */
VLC_API module_config_item_t *vlc_config_FindItem(const char *name) VLC_USED;

VLC_DEPRECATED static inline module_config_item_t *config_FindConfig(const char *name)
{
    return vlc_config_FindItem(name);
}

/**
 * Check whether or not the config item is in a modified (non-default) state
 *
 * \param item Configuration item
 * \return True if modified, False if unmodified (default).
 */
VLC_USED static inline bool vlc_config_ItemIsModified(const module_config_item_t *item)
{
    bool is_modified;
    switch (CONFIG_CLASS(item->i_type))
    {
        case CONFIG_ITEM_CLASS_BOOL:
            is_modified = (item->value.b != item->orig.b);
            break;
        case CONFIG_ITEM_CLASS_FLOAT:
            is_modified = (item->value.f != item->orig.f);
            break;
        case CONFIG_ITEM_CLASS_INTEGER:
            is_modified = (item->value.i != item->orig.i);
            break;
        case CONFIG_ITEM_CLASS_STRING: {
            bool orig_is_empty = (item->orig.psz == NULL || item->orig.psz[0] == '\0');
            bool curr_is_empty = (item->value.psz == NULL || item->value.psz[0] == '\0');
            is_modified = (orig_is_empty) ? !curr_is_empty :
                ( (curr_is_empty) ? true : (strcmp(item->value.psz, item->orig.psz) != 0) );
            break;
        }
        case CONFIG_ITEM_CLASS_INFO:
        case CONFIG_ITEM_CLASS_SPECIAL:
        default:
            is_modified = false;
            break;
    }
    return is_modified;
}

/**
 * System directory identifiers
 */
typedef enum vlc_system_dir
{
    VLC_PKG_DATA_DIR, /**< Package-specific architecture-independent read-only
                           data directory (e.g. /usr/local/data/vlc). */
    VLC_PKG_LIB_DIR, /**< Package-specific architecture-dependent read-only
                          data directory (e.g. /usr/local/lib/vlc). */
    VLC_PKG_LIBEXEC_DIR, /**< Package-specific executable read-only directory
                              (e.g. /usr/local/libexec/vlc). */
    VLC_PKG_INCLUDE_DIR_RESERVED,
    VLC_SYSDATA_DIR, /**< Global architecture-independent read-only
                          data directory (e.g. /usr/local/data).
                          Available only on some platforms. */
    VLC_LIB_DIR, /**< Global architecture-dependent read-only directory
                      (e.g. /usr/local/lib). */
    VLC_LIBEXEC_DIR, /**< Global executable read-only directory
                          (e.g. /usr/local/libexec). */
    VLC_INCLUDE_DIR_RESERVED,
    VLC_LOCALE_DIR, /**< Base directory for package read-only locale data. */
} vlc_sysdir_t;

/**
 * Gets an installation directory.
 *
 * This function determines one of the installation directory.
 *
 * @param dir identifier of the directory (see \ref vlc_sysdir_t)
 * @param filename name of a file or other object within the directory
 *                 (or NULL to obtain the plain directory)
 *
 * @return a heap-allocated string (use free() to release it), or NULL on error
 */
VLC_API char *config_GetSysPath(vlc_sysdir_t dir, const char *filename)
VLC_USED VLC_MALLOC;

typedef enum vlc_user_dir
{
    VLC_HOME_DIR, /* User's home */
    VLC_CONFIG_DIR, /* VLC-specific configuration directory */
    VLC_USERDATA_DIR, /* VLC-specific data directory */
    VLC_CACHE_DIR, /* VLC-specific user cached data directory */
    /* Generic directories (same as XDG) */
    VLC_DESKTOP_DIR=0x80,
    VLC_DOWNLOAD_DIR,
    VLC_TEMPLATES_DIR,
    VLC_PUBLICSHARE_DIR,
    VLC_DOCUMENTS_DIR,
    VLC_MUSIC_DIR,
    VLC_PICTURES_DIR,
    VLC_VIDEOS_DIR,
} vlc_userdir_t;

VLC_API char * config_GetUserDir( vlc_userdir_t ) VLC_USED VLC_MALLOC;

VLC_API void config_AddIntf(const char *);
VLC_API void config_RemoveIntf(const char *);
VLC_API bool config_ExistIntf(const char *) VLC_USED;

/****************************************************************************
 * config_chain_t:
 ****************************************************************************/
struct config_chain_t
{
    config_chain_t *p_next;     /**< Pointer on the next config_chain_t element */

    char        *psz_name;      /**< Option name */
    char        *psz_value;     /**< Option value */
};

/**
 * This function will
 * - create all options in the array ppsz_options (var_Create).
 * - parse the given linked list of config_chain_t and set the value (var_Set).
 *
 * The option names will be created by adding the psz_prefix prefix.
 */
VLC_API void config_ChainParse( vlc_object_t *, const char *psz_prefix, const char *const *ppsz_options, config_chain_t * );
#define config_ChainParse( a, b, c, d ) config_ChainParse( VLC_OBJECT(a), b, c, d )

/**
 * This function will parse a configuration string (psz_opts) and
 * - set all options for this module in a chained list (*pp_cfg)
 * - returns a pointer on the next module if any.
 *
 * The string format is
 *   module{option=*,option=*}
 *
 * The options values are unescaped using config_StringUnescape.
 */
VLC_API const char *config_ChainParseOptions( config_chain_t **pp_cfg, const char *ppsz_opts );

/**
 * This function will parse a configuration string (psz_string) and
 * - set the module name (*ppsz_name)
 * - set all options for this module in a chained list (*pp_cfg)
 * - returns a pointer on the next module if any.
 *
 * The string format is
 *   module{option=*,option=*}[:modulenext{option=*,...}]
 *
 * The options values are unescaped using config_StringUnescape.
 */
VLC_API char *config_ChainCreate( char **ppsz_name, config_chain_t **pp_cfg, const char *psz_string ) VLC_USED VLC_MALLOC;

/**
 * This function will release a linked list of config_chain_t
 * (Including the head)
 */
VLC_API void config_ChainDestroy( config_chain_t * );

/**
 * This function will duplicate a linked list of config_chain_t
 */
VLC_API config_chain_t * config_ChainDuplicate( const config_chain_t * ) VLC_USED VLC_MALLOC;

/**
 * This function will unescape a string in place and will return a pointer on
 * the given string.
 * No memory is allocated by it (unlike config_StringEscape).
 * If NULL is given as parameter nothing will be done (NULL will be returned).
 *
 * The following sequences will be unescaped (only one time):
 * \\ \' and \"
 */
VLC_API char * config_StringUnescape( char *psz_string );

/**
 * This function will escape a string that can be unescaped by
 * config_StringUnescape.
 * The returned value is allocated by it. You have to free it once you
 * do not need it anymore (unlike config_StringUnescape).
 * If NULL is given as parameter nothing will be done (NULL will be returned).
 *
 * The escaped characters are ' " and \
 */
VLC_API char * config_StringEscape( const char *psz_string ) VLC_USED VLC_MALLOC;

# ifdef __cplusplus
}
# endif

/** @} */

#endif /* _VLC_CONFIGURATION_H */
