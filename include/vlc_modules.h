/*****************************************************************************
 * vlc_modules.h : Module descriptor and load functions
 *****************************************************************************
 * Copyright (C) 2001-2011 VLC authors and VideoLAN
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

#ifndef VLC_MODULES_H
#define VLC_MODULES_H 1

#include <assert.h>

#include <vlc_module_caps.h>

/**
 * \file
 * This file defines functions for modules in vlc
 */

typedef int (*vlc_activate_t)(void *func, bool forced, va_list args);
typedef void (*vlc_deactivate_t)(void *func, va_list args);
struct vlc_logger;

/*****************************************************************************
 * Exported functions.
 *****************************************************************************/

/**
 * Finds and instantiates the best module of a certain type.
 *
 * All candidates modules having the specified capability and name will be
 * sorted in decreasing order of priority. Then the probe callback will be
 * invoked for each module, until it succeeds (returns 0), or all candidate
 * module failed to initialize.
 *
 * The probe callback first parameter is the address of the module entry point.
 * Further parameters are passed as an argument list; it corresponds to the
 * variable arguments passed to this function. This scheme is meant to
 * support arbitrary prototypes for the module entry point.
 *
 * Typically you will want to use the friendlier vlc_module_load()
 * and vlc_module_load_custom() calls, which are specific to built-in and
 * custom capabilities respectively; This function takes both (set cap to
 * VLC_CAP_CUSTOM and cap_custom to a string capability name if wanting to
 * request a custom capability.
 *
 * \param log logger for debugging (or NULL to ignore)
 * \param cap capability to load (set to VLC_CAP_CUSTOM for a custom string)
 * \param custom_cap the string form of a custom capability to check
 * \param name name of the module asked, if any
 * \param strict if true, do not fallback to plugin with a different name
 *                 but the same capability
 * \param probe module probe callback
 * \return the module or NULL in case of a failure
 */
VLC_API module_t *vlc_module_load_ext(struct vlc_logger *log,
                                  enum vlc_module_cap cap,
                                  const char *cap_custom,
                                  const char *name, bool strict,
                                  vlc_activate_t probe, ...) VLC_USED;
#ifndef __cplusplus
#define vlc_module_load_ext(ctx, cap, custom_cap, name, strict, ...) \
    _Generic ((ctx), \
        struct vlc_logger *: \
            vlc_module_load_ext((void *)(ctx), cap, custom_cap, name, strict, __VA_ARGS__), \
        void *: \
            vlc_module_load_ext((void *)(ctx), cap, custom_cap, name, strict, __VA_ARGS__), \
        default: \
            vlc_module_load_ext(vlc_object_logger((vlc_object_t *)(ctx)), \
                                cap, custom_cap, name, strict, __VA_ARGS__))
#endif

/**
 * Finds and instantiates the best module of a certain type.
 *
 * This is a convenience macro for use where the capability of interest is a
 * common ('built-in') type.
 */
#define vlc_module_load2( log, cap, name, strict, probe, ... ) \
    vlc_module_load_ext( log, cap, NULL, name, strict, probe, __VA_ARGS__ )

/**
 * Finds and instantiates the best module of a certain type.
 *
 * This is a convenience macro for use where the capability of interest is a
 * custom (string-based) type.
 */
#define vlc_module_load_custom( log, cap, name, strict, probe, ... ) \
    vlc_module_load_ext( log, VLC_CAP_CUSTOM, cap, name, strict, probe, __VA_ARGS__ )

/* deprecated */
VLC_DEPRECATED static inline
module_t *vlc_module_load(struct vlc_logger *log, const char *cap,
                          const char *name, bool strict,
                          vlc_activate_t probe, ...)
{
    va_list ap;
    va_start(ap, probe);
    enum vlc_module_cap _cap = vlc_module_cap_from_textid(cap);
    module_t *m = (vlc_module_load_ext)(log, _cap, cap, name, strict, probe, ap);
    va_end(ap);
    return m;
}
#ifndef __cplusplus
#define vlc_module_load(ctx, cap, name, strict, ...) \
    _Generic ((ctx), \
        struct vlc_logger *: \
            vlc_module_load((void *)(ctx), cap, name, strict, __VA_ARGS__), \
        void *: \
            vlc_module_load((void *)(ctx), cap, name, strict, __VA_ARGS__), \
        default: \
            vlc_module_load(vlc_object_logger((vlc_object_t *)(ctx)), cap, \
                            name, strict, __VA_ARGS__))
#endif

/**
 * Deinstantiates a module.
 *
 * This is an inconvenience wrapper for deactivating a module if the module
 * capability/type expects it. In that fashion, it is paired with
 * vlc_module_load(). It is rather inconvenient however, as it requires
 * variable arguments for no good reasons, and inhibits type safety.
 *
 * In practice, it is easier to treat vlc_module_load() as an object "factory",
 * and define a type-safe custom callback for object deletion.
 *
 * \param module the module pointer as returned by vlc_module_load()
 * \param deinit deactivation callback
 */
VLC_API void vlc_module_unload( module_t *, vlc_deactivate_t deinit, ... );

VLC_API module_t * vlc_module_need_ext( vlc_object_t *obj,
                                        enum vlc_module_cap cap,
                                        const char *custom_cap,
                                        const char *name,
                                        bool strict ) VLC_USED;
#define vlc_module_need_ext(a,b,c,d,e) vlc_module_need_ext(VLC_OBJECT(a),b,c,d,e)

#define vlc_module_need(a,b,c,d)        vlc_module_need_ext(a, b, NULL, c, d)
#define vlc_module_need_custom(a,b,c,d) vlc_module_need_ext(a, VLC_CAP_CUSTOM, b, c, d)

/* deprecated */
VLC_DEPRECATED static inline module_t *module_need( vlc_object_t *obj,
                                                    const char *cap,
                                                    const char *name,
                                                    bool strict )
{
    enum vlc_module_cap _cap = vlc_module_cap_from_textid(cap);
    return (_cap != VLC_CAP_CUSTOM) ? (vlc_module_need_ext)(obj, _cap, NULL, name, strict)
                                    : (vlc_module_need_ext)(obj, VLC_CAP_CUSTOM, cap, name, strict);
}
#define module_need(a,b,c) module_need(VLC_OBJECT(a),b,c)

VLC_USED
static inline module_t *vlc_module_need_var_ext(vlc_object_t *obj,
                                                enum vlc_module_cap cap,
                                                const char *custom_cap,
                                                const char *varname)
{
    assert(cap != VLC_CAP_INVALID);

    char *list = var_InheritString(obj, varname);
    module_t *m = (vlc_module_need_ext)(obj, cap, custom_cap, list, false);

    free(list);
    return m;
}
#define vlc_module_need_var_ext(a,b,c,d) vlc_module_need_var_ext(VLC_OBJECT(a),b,c,d)

#define vlc_module_need_var(a, b, c)        vlc_module_need_var_ext(a, b, NULL, c)
#define vlc_module_need_custom_var(a, b, c) vlc_module_need_var_ext(a, VLC_CAP_CUSTOM, b, c)

/* deprecated */
VLC_DEPRECATED static inline module_t *module_need_var(vlc_object_t *obj, const char *cap,
                                        const char *varname)
{
    enum vlc_module_cap _cap = vlc_module_cap_from_textid(cap);
    return (_cap != VLC_CAP_CUSTOM) ? (vlc_module_need_var_ext)(obj, _cap, NULL, varname)
                                    : (vlc_module_need_var_ext)(obj, VLC_CAP_CUSTOM, cap, varname);
}
#define module_need_var(a,b,c) module_need_var(VLC_OBJECT(a),b,c)

VLC_API void module_unneed( vlc_object_t *, module_t * );
#define module_unneed(a,b) module_unneed(VLC_OBJECT(a),b)

/**
 * Get a pointer to a module_t given it's name.
 *
 * The search can be targetted towards modules of a particular capability:
 *  - For built-in capabilities, simply pass the capability in cap, and NULL
 *    for custom_cap;
 *  - For custom capabilities, pass VLC_CAP_CUSTOM in cap and the name of the
 *    capability in custom_cap.
 *  - If a search of the entire collection of modules is wanted, pass
 *    VLC_CAP_INVALID as the cap and NULL for custom_cap. (Note, modules that
 *    actually have VLC_CAP_INVALID as their capability are filtered out, so
 *    cannot be found via this function).
 *
 * \param name the name of the module
 * \param cap a capability with which to filter the search to
 * \param custom_cap the name of a custom capability
 * \return a pointer to the module or NULL in case of a failure
 */
VLC_API module_t *vlc_module_find_ext(const char *name, enum vlc_module_cap cap, const char *custom_cap) VLC_USED;
#define vlc_module_find(n)               vlc_module_find_ext(n, VLC_CAP_INVALID, NULL)
#define vlc_module_find_with(n,c)        vlc_module_find_ext(n, c, NULL)
#define vlc_module_find_with_custom(n,c) vlc_module_find_ext(n, VLC_CAP_CUSTOM, c)

/* deprecated */
VLC_DEPRECATED static inline module_t *module_find(const char *name)
{
    return vlc_module_find(name);
}

/**
 * Checks if a module exists.
  *
 * Note that modules with capability of VLC_CAP_INVALID are ignored.
 *
 * \param name name of the module
 * \retval true if the module exists
 * \retval false if the module does not exist (in the running installation)
 */
VLC_USED static inline bool vlc_module_exists (const char * name)
{
    return vlc_module_find(name) != NULL;
}

/**
 * Gets the table of module configuration items.
 *
 * \note Most users of this function will not be interested in 'private'
 * options (those not displayed in the GUI, like --help), nor obsolete items.
 * You can filter those out using true for both of the latter parameters, or
 * use the vlc_module_config_get() convenience macro.
 *
 * \note Use module_config_free() to release the allocated memory.
 *
 * \param module the module
 * \param psize the size of the configuration returned
 * \param fpriv whether or not to filter private options (true = filtered out)
 * \param fobs whether or not to filter obsolete options (true = filtered out)
 * \return the configuration as an array, or NULL if the module has no config items
 */
VLC_API module_config_item_t *vlc_module_config_get_ext(const module_t *module,
                                                        unsigned *restrict psize,
                                                        bool fpriv, bool fobs) VLC_USED;
#define vlc_module_config_get(m, s) vlc_module_config_get_ext(m, s, true, true)

/* deprecated */
VLC_DEPRECATED static inline
module_config_item_t *module_config_get(const module_t *module, unsigned *restrict psize)
{
    return vlc_module_config_get(module, psize);
}

/**
 * Releases a configuration items table.
 *
 * \param tab base address of a table returned by vlc_module_config_get_ext()
 */
VLC_API void module_config_free( module_config_item_t *tab);

/**
 * Frees a list of VLC modules.
 *
 * \param list list obtained by module_list_get(), vlc_module_list_cap_ext()
 *             or vlc_module_list_have_config()
 */
VLC_API void module_list_free(module_t **);

/**
 * Gets the flat list of VLC modules.
 *
 * Note that this is not quite the complete list, it deliberately
 * excludes modules with capability of VLC_CAP_INVALID.
 *
 * \param n [OUT] pointer to the number of modules
 * \return table of module pointers (release with module_list_free()),
 *         or NULL in case of error (in that case, *n is zeroed).
 */
VLC_API module_t ** module_list_get(size_t *n) VLC_USED;

/**
 * Gets the list of VLC modules that have a config set.
 *
 * \note: Only those with at least one proper config item are included; so
 *        called "special"/"hint" items (like those selecting category) do not
 *        count.
 *
 * \param n [OUT] pointer to the number of modules
 * \return table of module pointers (release with module_list_free()),
 *         or NULL in case of error (in that case, *n is zeroed).
 */
VLC_API module_t ** vlc_module_list_have_config(size_t *n) VLC_USED;

/**
 * Gets a sorted list of all VLC modules with a given capability.
 *
 * The list is sorted from the highest module score to the lowest.
 *
 * \note *list must be freed with module_list_free().
 * \note macros vlc_module_list_cap() and vlc_module_list_cap_custom() are
 * provided for specifically 'built-in' and 'custom' capabilities respectively.
 *
 * \param list pointer to the table of modules [OUT]
 * \param id capability of modules to look for; use VLC_CAP_CUSTOM for those
 *           with a custom capability
 * \param name custom capability name (use NULL where non-custom)
 * \return the number of matching found, or -1 on error (*list is then NULL).
 */
VLC_API ssize_t vlc_module_list_cap_ext (module_t ***list, enum vlc_module_cap id, const char *name) VLC_USED;

#define vlc_module_list_cap(l, c)        vlc_module_list_cap_ext(l, c, NULL)
#define vlc_module_list_cap_custom(l, c) vlc_module_list_cap_ext(l, VLC_CAP_CUSTOM, c)

/**
 * Gets the internal name of a module.
 *
 * \param m the module
 * \return the module name
 */
VLC_API const char * module_get_object(const module_t *m) VLC_USED;

/**
 * Gets the human-friendly name of a module.
 *
 * \param m the module
 * \param longname TRUE to have the long name of the module
 * \return the short or long name of the module
 */
VLC_API const char *vlc_module_get_name(const module_t *m, bool longname) VLC_USED;
#define vlc_module_GetShortName( m ) vlc_module_get_name( m, false )
#define vlc_module_GetLongName( m ) vlc_module_get_name( m, true )

/**
 * Gets the help text for a module.
 *
 * \param m the module
 * \return the help
 */
VLC_API const char *module_get_help(const module_t *m) VLC_USED;

/**
 * Gets the capability of a module.
 *
 * The returned capability is the raw `enum vlc_module_cap` property. Of the
 * possible variants:
 *
 *  - Normally this will either be a variant representing a proper capability,
 *    or VLC_CAP_CUSTOM where the module uses a custom string-form capability
 *    (in which case use vlc_module_get_custom_capability() to get the custom
 *    capability name if you are interested in it).
 *  - Modules with VLC_CAP_INVALID are rare but they do exist so you may in
 *    places want to check for this. Such modules occur where a plugin has a
 *    module for which a capability has not been assigned; which could occur
 *    through mistake (e.g. plugin has an unneccessary initial call to
 *    add_submodule()), or a deliberate use of this as a "hack" regarding the
 *    name used in help output against a plugin's options (the name of the
 *    plugin's first module is used there). In practice, except in unexpected
 *    error, you're only going to encounter it if you process the raw list of
 *    all modules (they will never be returned in a capability-specific list).
 *  - VLC_CAP_MAX will never be encountered in practice.
 *
 * \param m the module
 * \return the capability
 */
VLC_API enum vlc_module_cap vlc_module_get_capability(const module_t *m) VLC_USED;

/**
 * Gets the custom capability string of a module.
 *
 * \param m the module
 * \return the capability, or "none" if unspecified
 */
VLC_API const char *vlc_module_get_custom_capability(const module_t *m) VLC_USED;

/**
 * Gets the string form for the capability of a module.
 *
 * \param m the module
 * \return for a module with a built-in capability, the string form of the ID
 * will be returned; for those with a custom capability, that custom capability
 * string will be returned, or "none" if unspecified (an error).
 */
VLC_USED
static inline const char *vlc_module_get_capability_str (const module_t *m)
{
    enum vlc_module_cap cap_id = vlc_module_get_capability(m);
    if (cap_id == VLC_CAP_CUSTOM)
        return vlc_module_get_custom_capability(m);
    else
    {
        assert(cap_id != VLC_CAP_INVALID);
        return vlc_module_cap_get_textid(cap_id);
    }
}

/* deprecated */
VLC_DEPRECATED static inline const char *module_get_capability(const module_t *m)
{
    return vlc_module_get_capability_str(m);
}

/**
 * Gets a "display" name for the capability of a module.
 *
 * \param m the module
 * \return for a module with a built-in capability, a descriptive display name
 * of the capability will be returned (not the same as the string form of the
 * ID); for those with a custom capability, that custom capability string will
 * simply be returned, or "none" if unspecified (an error).
 */
VLC_USED
static inline const char *vlc_module_get_capability_name (const module_t *m)
{
    enum vlc_module_cap cap_id = vlc_module_get_capability(m);
    if (cap_id == VLC_CAP_CUSTOM)
        return vlc_module_get_custom_capability(m);
    else
    {
        assert(cap_id != VLC_CAP_INVALID);
        return vlc_module_cap_get_desc(cap_id);
    }
}

/**
 * Gets the precedence of a module.
 *
 * \param m the module
 * return the score for the capability
 */
VLC_API int module_get_score(const module_t *m) VLC_USED;

/**
 * Checks whether a module implements a capability.
 *
 * \param m the module
 * \param cap the capability to check (set to VLC_CAP_CUSTOM to check a custom string)
 * \param custom_cap the string form of a custom capability to check
 * \retval true if the module has the capability
 * \retval false if the module has another capability
 */
static inline bool vlc_module_provides(const module_t *m,
                                       enum vlc_module_cap cap,
                                       const char *custom_cap)
{
    assert(cap != VLC_CAP_INVALID);
    if (cap != VLC_CAP_CUSTOM)
        return vlc_module_get_capability(m) == cap;
    if (custom_cap == NULL)
        return false;
    return !strcmp(vlc_module_get_custom_capability(m), custom_cap);
}

/* deprecated */
VLC_DEPRECATED static inline bool module_provides(const module_t *m, const char *cap)
{
    return vlc_module_provides(m, vlc_module_cap_from_textid(cap), cap);
}

/**
 * Translates a string using the module's text domain
 *
 * \param m the module
 * \param s the American English ASCII string to localize
 * \return the gettext-translated string
 */
VLC_API const char *module_gettext(const module_t *m, const char *s) VLC_USED;

VLC_USED static inline module_t *module_get_main (void)
{
    module_t **module_list = NULL;
    ssize_t count = vlc_module_list_cap( &module_list, VLC_CAP_CORE );
    assert(count == 1);
    return module_list[0];
}

VLC_USED static inline bool module_is_main( const module_t * p_module )
{
    return !strcmp( module_get_object( p_module ), "core" );
}

#endif /* VLC_MODULES_H */
