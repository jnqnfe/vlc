/*****************************************************************************
 * bank.c : Modules list
 *****************************************************************************
 * Copyright (C) 2001-2011 VLC authors and VideoLAN
 *
 * Authors: Sam Hocevar <sam@zoy.org>
 *          Ethan C. Baldridge <BaldridgeE@cadmus.com>
 *          Hans-Peter Jansen <hpj@urpla.net>
 *          Gildas Bazin <gbazin@videolan.org>
 *          Rémi Denis-Courmont
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#ifdef HAVE_SEARCH_H
# include <search.h>
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <vlc_modules.h>
#include <vlc_fs.h>
#include <vlc_block.h>
#include "libvlc.h"
#include "config/configuration.h"
#include "modules/modules.h"

typedef struct vlc_modcap
{
    module_t **modv;
    size_t modc;
} vlc_modcap_t;

typedef struct vlc_modcap_custom
{
    char *name;
    module_t **modv;
    size_t modc;
} vlc_modcap_custom_t;

static int vlc_modcap_custom_cmp(const void *a, const void *b)
{
    const vlc_modcap_custom_t *capa = a, *capb = b;
    return strcmp(capa->name, capb->name);
}

static void vlc_modcap_custom_free(void *data)
{
    vlc_modcap_custom_t *cap = data;

    free(cap->modv);
    free(cap->name);
    free(cap);
}

static int vlc_module_cmp (const void *a, const void *b)
{
    const module_t *const *ma = a, *const *mb = b;
    /* Note that qsort() uses _ascending_ order,
     * so the smallest module is the one with the biggest score. */
    return (*mb)->i_score - (*ma)->i_score;
}

static void vlc_modcap_sort(vlc_modcap_t *cap)
{
    qsort(cap->modv, cap->modc, sizeof (*cap->modv), vlc_module_cmp);
}

static void vlc_modcap_custom_sort(const void *node, const VISIT which,
                            const int depth)
{
    vlc_modcap_custom_t *const *cp = node, *cap = *cp;

    if (which != postorder && which != leaf)
        return;

    qsort(cap->modv, cap->modc, sizeof (*cap->modv), vlc_module_cmp);
    (void) depth;
}

static struct
{
    vlc_mutex_t lock;
    block_t *caches;
    vlc_modcap_t caps_tree[(size_t)VLC_CAP_MAX];
    void *custom_caps_tree;
    unsigned usage;
} modules = { VLC_STATIC_MUTEX, NULL, {}, NULL, 0 };

vlc_plugin_t *vlc_plugins = NULL;
size_t vlc_plugins_count = 0;

/* reset/init the array */
static void vlc_reset_builtin_cap_tree() {
    for (size_t i = 0; i < (size_t)VLC_CAP_MAX; i++)
    {
        vlc_modcap_t* cap = &modules.caps_tree[i];
        if (cap->modv != NULL)
            free(cap->modv);
        cap->modc = 0;
    }
}

/**
 * Add a module to the pre-organised sets
 */
static int vlc_module_store(module_t *mod)
{
    /* increment counter even if VLC_CAP_INVALID, it should be a complete count */
    vlc_plugins_count++;

    switch (mod->capability)
    {
        case VLC_CAP_INVALID:
            /* Some plugins authors choose to call add_submodule() without
               having actually setup the initial module with a capability and
               callbacks; where a plugin has multiple modules, this is
               sometimes done deliberately in order that the initial module is
               utilised for holding name and help text properties that apply to
               that group of modules as a whole, being used for instance in
               help output against the plugin's option set. We have no interest
               in recording such entries in the capability tree, so we skip it. */
            break;
        case VLC_CAP_CUSTOM:
        {
            const char *name = vlc_module_get_custom_capability(mod);

            vlc_modcap_custom_t *cap = malloc(sizeof (*cap));
            if (unlikely(cap == NULL))
                return -1;

            cap->name = strdup(name);
            cap->modv = NULL;
            cap->modc = 0;

            if (unlikely(cap->name == NULL))
            {
                vlc_modcap_custom_free(cap);
                return -1;
            }

            vlc_modcap_custom_t **cp = tsearch(cap, &modules.custom_caps_tree, vlc_modcap_custom_cmp);
            if (unlikely(cp == NULL))
            {
                vlc_modcap_custom_free(cap);
                return -1;
            }

            if (*cp != cap)
            {
                vlc_modcap_custom_free(cap);
                cap = *cp;
            }

            module_t **modv = realloc(cap->modv, sizeof (*modv) * (cap->modc + 1));
            if (unlikely(modv == NULL))
                return -1;

            cap->modv = modv;
            cap->modv[cap->modc] = mod;
            cap->modc++;
            break;
        }
        default:
        {
            vlc_modcap_t* cap = &modules.caps_tree[(size_t)mod->capability];

            module_t **modv = realloc(cap->modv, sizeof (*modv) * (cap->modc + 1));
            if (unlikely(modv == NULL))
                return -1;

            cap->modv = modv;
            cap->modv[cap->modc] = mod;
            cap->modc++;
            break;
        }
    }
    return 0;
}

/**
 * Adds a plugin (and all its modules) to the bank
 */
static void vlc_plugin_store(vlc_plugin_t *lib)
{
    vlc_mutex_assert(&modules.lock);

    /* Add the plugin to the link list */
    lib->next = vlc_plugins;
    vlc_plugins = lib;

    /* Add modules to the pre-organised sets */
    for (module_t *m = lib->module; m != NULL; m = m->next)
        vlc_module_store(m);
}

/**
 * Registers a statically-linked plug-in.
 */
static vlc_plugin_t *module_InitStatic(vlc_plugin_cb *entry)
{
    /* Initializes the statically-linked library */
    vlc_plugin_t *lib = vlc_plugin_describe(entry, "<static>");
    if (unlikely(lib == NULL))
        return NULL;

#ifdef HAVE_DYNAMIC_PLUGINS
    atomic_init(&lib->handle, 1 /* must be non-zero for module_Map() */);
    lib->unloadable = false;
#endif
    return lib;
}

#if defined(__ELF__) || !HAVE_DYNAMIC_PLUGINS
VLC_WEAK
extern vlc_plugin_cb *vlc_static_modules[];

static void module_InitStaticModules(void)
{
    if (!vlc_static_modules)
        return;

    for (unsigned i = 0; vlc_static_modules[i]; i++)
    {
        vlc_plugin_t *lib = module_InitStatic(vlc_static_modules[i]);
        if (likely(lib != NULL))
            vlc_plugin_store(lib);
    }
}
#else
static void module_InitStaticModules(void) { }
#endif

#ifdef HAVE_DYNAMIC_PLUGINS
static const char vlc_entry_name[] = XSTRINGIFY(__VLC_PLUGIN_DESCRIPTOR_SYMBOL);

/**
 * Loads a dynamically-linked plug-in into memory and initialize it.
 *
 * The module can then be handled by vlc_module_need() and module_unneed().
 *
 * \param path file path of the shared object
 * \param fast whether to optimize loading for speed or safety
 *             (fast is used when the plug-in is registered but not used)
 */
static vlc_plugin_t *module_InitDynamic(vlc_object_t *obj, const char *path,
                                        bool fast)
{
    void *handle = vlc_dlopen(path, fast);
    if (handle == NULL)
    {
        char *errmsg = vlc_dlerror();
        msg_Err(obj, "cannot load plug-in %s: %s", path,
                errmsg ? errmsg : "unknown error");
        free(errmsg);
        return NULL;
    }

    /* Try to resolve the symbol */
    vlc_plugin_cb *entry = vlc_dlsym(handle, vlc_entry_name);
    if (entry == NULL)
    {
        msg_Warn (obj, "cannot find plug-in entry point in %s", path);
        goto error;
    }

    /* We can now try to call the symbol */
    vlc_plugin_t *plugin = vlc_plugin_describe(entry, path);
    if (unlikely(plugin == NULL))
    {
        /* With a well-written module we shouldn't have to print an
         * additional error message here, but just make sure. */
        msg_Err (obj, "cannot initialize plug-in %s", path);
        goto error;
    }

    atomic_init(&plugin->handle, (uintptr_t)handle);
    return plugin;
error:
    vlc_dlclose(handle);
    return NULL;
}

typedef enum
{
    CACHE_READ_FILE  = 0x1,
    CACHE_SCAN_DIR   = 0x2,
    CACHE_WRITE_FILE = 0x4,
} cache_mode_t;

typedef struct module_bank
{
    vlc_object_t *obj;
    const char   *base;
    cache_mode_t  mode;

    size_t        size;
    vlc_plugin_t **plugins;
    vlc_plugin_t *cache;
} module_bank_t;

/**
 * Scans a plug-in from a file.
 */
static int AllocatePluginFile (module_bank_t *bank, const char *abspath,
                               const char *relpath, const struct stat *st)
{
    vlc_plugin_t *plugin = NULL;

    /* Check our plugins cache first then load plugin if needed */
    if (bank->mode & CACHE_READ_FILE)
    {
        plugin = vlc_cache_lookup(&bank->cache, relpath);

        if (plugin != NULL
         && (plugin->mtime != (int64_t)st->st_mtime
          || plugin->size != (uint64_t)st->st_size))
        {
            msg_Err(bank->obj, "stale plugins cache: modified %s",
                    plugin->abspath);
            vlc_plugin_destroy(plugin);
            plugin = NULL;
        }
    }

    if (plugin == NULL)
    {
        plugin = module_InitDynamic(bank->obj, abspath, true);

        if (plugin != NULL)
        {
            plugin->path = xstrdup(relpath);
            plugin->mtime = st->st_mtime;
            plugin->size = st->st_size;
        }
    }

    if (plugin == NULL)
        return -1;

    vlc_plugin_store(plugin);

    if (bank->mode & CACHE_WRITE_FILE) /* Add entry to to-be-saved cache */
    {
        bank->plugins = xrealloc(bank->plugins,
                                 (bank->size + 1) * sizeof (vlc_plugin_t *));
        bank->plugins[bank->size] = plugin;
        bank->size++;
    }

    /* TODO: deal with errors */
    return  0;
}

/**
 * Recursively browses a directory to look for plug-ins.
 */
static void AllocatePluginDir (module_bank_t *bank, unsigned maxdepth,
                               const char *absdir, const char *reldir)
{
    if (maxdepth == 0)
        return;
    maxdepth--;

    DIR *dh = vlc_opendir (absdir);
    if (dh == NULL)
        return;

    /* Parse the directory and try to load all files it contains. */
    for (;;)
    {
        char *relpath = NULL, *abspath = NULL;
        const char *file = vlc_readdir (dh);
        if (file == NULL)
            break;

        /* Skip ".", ".." */
        if (!strcmp (file, ".") || !strcmp (file, ".."))
            continue;

        /* Compute path relative to plug-in base directory */
        if (reldir != NULL)
        {
            if (asprintf (&relpath, "%s"DIR_SEP"%s", reldir, file) == -1)
                relpath = NULL;
        }
        else
            relpath = strdup (file);
        if (unlikely(relpath == NULL))
            continue;

        /* Compute absolute path */
        if (asprintf (&abspath, "%s"DIR_SEP"%s", bank->base, relpath) == -1)
        {
            abspath = NULL;
            goto skip;
        }

        struct stat st;
        if (vlc_stat (abspath, &st) == -1)
            goto skip;

        if (S_ISREG (st.st_mode))
        {
            static const char prefix[] = "lib";
            static const char suffix[] = "_plugin"LIBEXT;
            size_t len = strlen (file);

#ifndef __OS2__
            /* Check that file matches the "lib*_plugin"LIBEXT pattern */
            if (len > strlen (suffix)
             && !strncmp (file, prefix, strlen (prefix))
             && !strcmp (file + len - strlen (suffix), suffix))
#else
            /* We load all the files ending with LIBEXT on OS/2,
             * because OS/2 has a 8.3 length limitation for DLL name */
            if (len > strlen (LIBEXT)
             && !strcasecmp (file + len - strlen (LIBEXT), LIBEXT))
#endif
                AllocatePluginFile (bank, abspath, relpath, &st);
        }
        else if (S_ISDIR (st.st_mode))
            /* Recurse into another directory */
            AllocatePluginDir (bank, maxdepth, abspath, relpath);
    skip:
        free (relpath);
        free (abspath);
    }
    closedir (dh);
}

/**
 * Scans for plug-ins within a file system hierarchy.
 * \param path base directory to browse
 */
static void AllocatePluginPath(vlc_object_t *obj, const char *path,
                               cache_mode_t mode)
{
    module_bank_t bank =
    {
        .obj = obj,
        .base = path,
        .mode = mode,
    };

    if (mode & CACHE_READ_FILE)
        bank.cache = vlc_cache_load(obj, path, &modules.caches);
    else
        msg_Dbg(bank.obj, "ignoring plugins cache file");

    if (mode & CACHE_SCAN_DIR)
    {
        msg_Dbg(obj, "recursively browsing `%s'", bank.base);

        /* Don't go deeper than 5 subdirectories */
        AllocatePluginDir(&bank, 5, path, NULL);
    }

    /* Deal with unmatched cache entries from cache file */
    while (bank.cache != NULL)
    {
        vlc_plugin_t *plugin = bank.cache;

        bank.cache = plugin->next;
        if (mode & CACHE_SCAN_DIR)
            vlc_plugin_destroy(plugin);
        else
            vlc_plugin_store(plugin);
    }

    if (mode & CACHE_WRITE_FILE)
        CacheSave(obj, path, bank.plugins, bank.size);

    free(bank.plugins);
}

/**
 * Enumerates all dynamic plug-ins that can be found.
 *
 * This function will recursively browse the default plug-ins directory and any
 * directory listed in the VLC_PLUGIN_PATH environment variable.
 * For performance reasons, a cache is normally used so that plug-in shared
 * objects do not need to loaded and linked into the process.
 */
static void AllocateAllPlugins (vlc_object_t *p_this)
{
    char *paths;
    cache_mode_t mode = 0;

    if (var_InheritBool(p_this, "plugins-cache"))
        mode |= CACHE_READ_FILE;
    if (var_InheritBool(p_this, "plugins-scan"))
        mode |= CACHE_SCAN_DIR;
    if (var_InheritBool(p_this, "reset-plugins-cache"))
        mode = (mode | CACHE_WRITE_FILE) & ~CACHE_READ_FILE;

#if VLC_WINSTORE_APP
    /* Windows Store Apps can not load external plugins with absolute paths. */
    AllocatePluginPath (p_this, "plugins", mode);
#else
    /* Contruct the special search path for system that have a relocatable
     * executable. Set it to <vlc path>/plugins. */
    char *vlcpath = config_GetSysPath(VLC_PKG_LIB_DIR, "plugins");
    if (likely(vlcpath != NULL))
    {
        AllocatePluginPath(p_this, vlcpath, mode);
        free(vlcpath);
    }
#endif /* VLC_WINSTORE_APP */

    /* If the user provided a plugin path, we add it to the list */
    paths = getenv( "VLC_PLUGIN_PATH" );
    if( paths == NULL )
        return;

#ifdef _WIN32
    paths = realpath( paths, NULL );
#else
    paths = strdup( paths ); /* don't harm the environment ! :) */
#endif
    if( unlikely(paths == NULL) )
        return;

    for( char *buf, *path = strtok_r( paths, PATH_SEP, &buf );
         path != NULL;
         path = strtok_r( NULL, PATH_SEP, &buf ) )
        AllocatePluginPath (p_this, path, mode);

    free( paths );
}

/**
 * Ensures that a plug-in is loaded.
 *
 * \note This function is thread-safe but not re-entrant.
 *
 * \return 0 on success, -1 on failure
 */
int module_Map(struct vlc_logger *log, vlc_plugin_t *plugin)
{
    static vlc_mutex_t lock = VLC_STATIC_MUTEX;

    if (atomic_load_explicit(&plugin->handle, memory_order_acquire))
        return 0; /* fast path: already loaded */

    /* Try to load the plug-in (without locks, so read-only) */
    assert(plugin->abspath != NULL);

    void *handle = vlc_dlopen(plugin->abspath, false);
    if (handle == NULL)
    {
        char *errmsg = vlc_dlerror();
        vlc_error(log, "cannot load plug-in %s: %s",
                  plugin->abspath, errmsg ? errmsg : "unknown error");
        free(errmsg);
        return -1;
    }

    vlc_plugin_cb *entry = vlc_dlsym(handle, vlc_entry_name);
    if (entry == NULL)
    {
        vlc_error(log, "cannot find plug-in entry point in %s",
                  plugin->abspath);
        goto error;
    }

    vlc_mutex_lock(&lock);
    if (atomic_load_explicit(&plugin->handle, memory_order_relaxed) == 0)
    {   /* Lock is held, update the plug-in structure */
        if (vlc_plugin_resolve(plugin, entry))
        {
            vlc_mutex_unlock(&lock);
            goto error;
        }

        atomic_store_explicit(&plugin->handle, (uintptr_t)handle,
                              memory_order_release);
    }
    else /* Another thread won the race to load the plugin */
        vlc_dlclose(handle);
    vlc_mutex_unlock(&lock);

    return 0;
error:
    vlc_dlclose(handle);
    return -1;
}

/**
 * Ensures that a module is not loaded.
 *
 * \note This function is not thread-safe. The caller must ensure that the
 * plug-in is no longer used before calling this function.
 */
static void module_Unmap(vlc_plugin_t *plugin)
{
    if (!plugin->unloadable)
        return;

    void *handle = (void *)atomic_exchange_explicit(&plugin->handle, 0,
                                                    memory_order_acquire);
    if (handle != NULL)
        vlc_dlclose(handle);
}
#else
int module_Map(struct vlc_logger *log, vlc_plugin_t *plugin)
{
    (void) log; (void) plugin;
    return 0;
}

static void module_Unmap(vlc_plugin_t *plugin)
{
    (void) plugin;
}
#endif /* HAVE_DYNAMIC_PLUGINS */

/**
 * Init bank
 *
 * Creates a module bank structure which will be filled later
 * on with all the modules found.
 */
void module_InitBank (void)
{
    vlc_mutex_lock (&modules.lock);

    if (modules.usage == 0)
    {
        /* Fills the module bank structure with the core module infos.
         * This is very useful as it will allow us to consider the core
         * library just as another module, and for instance the configuration
         * options of core will be available in the module bank structure just
         * as for every other module. */
        memset(&modules.caps_tree, 0, sizeof(modules.caps_tree));
        vlc_plugin_t *plugin = module_InitStatic(vlc_entry__core);
        if (likely(plugin != NULL))
            vlc_plugin_store(plugin);
        config_SortConfig ();
    }
    modules.usage++;

    /* We do retain the module bank lock until the plugins are loaded as well.
     * This is ugly, this staged loading approach is needed: LibVLC gets
     * some configuration parameters relevant to loading the plugins from
     * the core (builtin) module. The module bank becomes shared read-only data
     * once it is ready, so we need to fully serialize initialization.
     * DO NOT UNCOMMENT the following line unless you managed to squeeze
     * module_LoadPlugins() before you unlock the mutex. */
    /*vlc_mutex_unlock (&modules.lock);*/
}

/**
 * Unloads all unused plugin modules and empties the module
 * bank in case of success.
 */
void module_EndBank (bool b_plugins)
{
    vlc_plugin_t *libs = NULL;
    block_t *caches = NULL;
    void *custom_caps_tree = NULL;

    /* If plugins were _not_ loaded, then the caller still has the bank lock
     * from module_InitBank(). */
    if( b_plugins )
        vlc_mutex_lock (&modules.lock);
    else
        vlc_mutex_assert(&modules.lock);

    assert (modules.usage > 0);
    if (--modules.usage == 0)
    {
        config_UnsortConfig ();
        libs = vlc_plugins;
        caches = modules.caches;
        custom_caps_tree = modules.custom_caps_tree;
        vlc_plugins = NULL;
        vlc_plugins_count = 0;
        modules.caches = NULL;
        modules.custom_caps_tree = NULL;
        vlc_reset_builtin_cap_tree();
    }
    vlc_mutex_unlock (&modules.lock);

    tdestroy(custom_caps_tree, vlc_modcap_custom_free);

    while (libs != NULL)
    {
        vlc_plugin_t *lib = libs;

        libs = lib->next;
        module_Unmap(lib);
        vlc_plugin_destroy(lib);
    }

    block_ChainRelease(caches);
}

#undef module_LoadPlugins
/**
 * Loads module descriptions for all available plugins.
 * Fills the module bank structure with the plugin modules.
 *
 * \param p_this vlc object structure
 */
void module_LoadPlugins(vlc_object_t *obj)
{
    /*vlc_mutex_assert (&modules.lock); not for static mutexes :( */

    if (modules.usage == 1)
    {
        module_InitStaticModules ();
#ifdef HAVE_DYNAMIC_PLUGINS
        msg_Dbg (obj, "searching plug-in modules");
        AllocateAllPlugins (obj);
#endif
        config_UnsortConfig ();
        config_SortConfig ();

        for (size_t i = 0; i < (size_t)VLC_CAP_MAX; i++)
            vlc_modcap_sort(&modules.caps_tree[(size_t)i]);
        twalk(modules.custom_caps_tree, vlc_modcap_custom_sort);
    }
    vlc_mutex_unlock (&modules.lock);

    msg_Dbg (obj, "plug-ins loaded: %zu modules", vlc_plugins_count);
}

void module_list_free (module_t **list)
{
    free (list);
}

module_t **module_list_get (size_t *n)
{
    assert (n != NULL);

    module_t **tab = malloc(vlc_plugins_count * sizeof (*tab));
    if (unlikely(tab == NULL))
    {
        *n = 0;
        return NULL;
    }

    size_t i = 0;
    for (vlc_plugin_t *lib = vlc_plugins; lib != NULL; lib = lib->next)
    {
        for (module_t *m = lib->module; m != NULL; m = m->next)
        {
            if (unlikely(m->capability == VLC_CAP_INVALID))
                continue;
            assert(i < vlc_plugins_count);
            tab[i++] = m;
        }
    }
    *n = i;
    return tab;
}

module_t **vlc_module_list_have_config (size_t *n)
{
    assert (n != NULL);

    module_t **tab = malloc(vlc_plugins_count * sizeof (*tab));
    if (unlikely(tab == NULL))
    {
        *n = 0;
        return NULL;
    }

    size_t i = 0;
    for (vlc_plugin_t *lib = vlc_plugins; lib != NULL; lib = lib->next)
    {
        /* first module's attributes are used to represent plugin and thus its
           option set! */
        if (lib->conf.count > 0)
            tab[i++] = lib->module;
    }
    *n = i;
    return tab;
}

ssize_t vlc_module_list_cap_ext (module_t ***restrict list, enum vlc_module_cap id, const char *name)
{
    assert(id != VLC_CAP_INVALID);

    module_t **caps = NULL;
    size_t n;

    switch (id)
    {
        case VLC_CAP_CUSTOM:
        {
            assert(name != NULL);
            const vlc_modcap_custom_t **set = tfind(&name,
                &modules.custom_caps_tree, vlc_modcap_custom_cmp);
            if (set == NULL)
            {
                *list = NULL;
                return 0;
            }
            caps = (*set)->modv;
            n = (*set)->modc;
            break;
        }
        default:
        {
            const vlc_modcap_t* set = &modules.caps_tree[(size_t)id];
            caps = set->modv;
            n = set->modc;
            break;
        }
    }

    module_t **tab = vlc_alloc (n, sizeof (*tab));
    *list = tab;
    if (unlikely(tab == NULL))
        return -1;

    memcpy(tab, caps, sizeof (*tab) * n);
    return n;
}

typedef struct {
    enum vlc_module_cap cap; /**< Capability */
    char textid[20];         /**< String form */
    char name[20];           /**< Name (for interface output) */
} cap_description_t;

#define CAP(cap, textid, name) { cap, textid, name }

/**
 * Capability lookup data
 *
 * Used for translation between enum ID and older string-based ID form,
 * and for getting a description for interface output.
 */
static const cap_description_t cap_descriptions[] = {
    /* Stored in approximate most used order to optimise */
    CAP(VLC_CAP_ACCESS,             VLC_CAP_STR_ACCESS,             N_("Access")),
    CAP(VLC_CAP_DEMUX,              VLC_CAP_STR_DEMUX,              N_("Demux")),
    CAP(VLC_CAP_VIDEO_FILTER,       VLC_CAP_STR_VIDEO_FILTER,       N_("Video filter")),
    CAP(VLC_CAP_ENCODER,            VLC_CAP_STR_ENCODER,            N_("Encoder")),
    CAP(VLC_CAP_VIDEO_CONVERTER,    VLC_CAP_STR_VIDEO_CONVERTER,    N_("Video converter")),
    CAP(VLC_CAP_AUDIO_DECODER,      VLC_CAP_STR_AUDIO_DECODER,      N_("Audio decoder")),
    CAP(VLC_CAP_VIDEO_DECODER,      VLC_CAP_STR_VIDEO_DECODER,      N_("Video decoder")),
    CAP(VLC_CAP_VOUT_DISPLAY,       VLC_CAP_STR_VOUT_DISPLAY,       N_("Vout display")),
    CAP(VLC_CAP_PACKETIZER,         VLC_CAP_STR_PACKETIZER,         N_("Packetizer")),
    CAP(VLC_CAP_STREAM_FILTER,      VLC_CAP_STR_STREAM_FILTER,      N_("Stream filter")),
    CAP(VLC_CAP_SOUT_STREAM,        VLC_CAP_STR_SOUT_STREAM,        N_("Sout stream")),
    CAP(VLC_CAP_SPU_DECODER,        VLC_CAP_STR_SPU_DECODER,        N_("SPU decoder")),
    CAP(VLC_CAP_INTERFACE,          VLC_CAP_STR_INTERFACE,          N_("Interface")),
    CAP(VLC_CAP_SERVICES_DISCOVERY, VLC_CAP_STR_SERVICES_DISCOVERY, N_("Services discovery")),
    CAP(VLC_CAP_AUDIO_OUTPUT,       VLC_CAP_STR_AUDIO_OUTPUT,       N_("Audio output")),
    CAP(VLC_CAP_AUDIO_FILTER,       VLC_CAP_STR_AUDIO_FILTER,       N_("Audio filter")),
    CAP(VLC_CAP_SOUT_ACCESS,        VLC_CAP_STR_SOUT_ACCESS,        N_("Sout access")),
    CAP(VLC_CAP_SERVICES_PROBE,     VLC_CAP_STR_SERVICES_PROBE,     N_("Services probe")),
    CAP(VLC_CAP_SOUT_MUX,           VLC_CAP_STR_SOUT_MUX,           N_("Sout mux")),
    CAP(VLC_CAP_VOUT_WINDOW,        VLC_CAP_STR_VOUT_WINDOW,        N_("Vout window")),
    CAP(VLC_CAP_AUDIO_CONVERTER,    VLC_CAP_STR_AUDIO_CONVERTER,    N_("Audio converter")),
    CAP(VLC_CAP_SUB_SOURCE,         VLC_CAP_STR_SUB_SOURCE,         N_("Sub source")),
    CAP(VLC_CAP_KEYSTORE,           VLC_CAP_STR_KEYSTORE,           N_("Keystore")),
    CAP(VLC_CAP_AUDIO_RESAMPLER,    VLC_CAP_STR_AUDIO_RESAMPLER,    N_("Audio resampler")),
    CAP(VLC_CAP_GLCONV,             VLC_CAP_STR_GLCONV,             N_("Glconv")),
    CAP(VLC_CAP_INHIBIT,            VLC_CAP_STR_INHIBIT,            N_("Inhibit")),
    CAP(VLC_CAP_LOGGER,             VLC_CAP_STR_LOGGER,             N_("Logger")),
    CAP(VLC_CAP_HW_DECODER,         VLC_CAP_STR_HW_DECODER,         N_("HW decoder")),
    CAP(VLC_CAP_HW_DECODER_DEVICE,  VLC_CAP_STR_HW_DECODER_DEVICE,  N_("Decoder device")),
    CAP(VLC_CAP_TEXT_RENDERER,      VLC_CAP_STR_TEXT_RENDERER,      N_("Text renderer")),
    CAP(VLC_CAP_VISUALIZATION,      VLC_CAP_STR_VISUALIZATION,      N_("Visualization")),
    CAP(VLC_CAP_OPENGL,             VLC_CAP_STR_OPENGL,             N_("OpenGL")),
    CAP(VLC_CAP_PLAYLIST_EXPORT,    VLC_CAP_STR_PLAYLIST_EXPORT,    N_("Playlist export")),
    CAP(VLC_CAP_RENDERER_DISCOVERY, VLC_CAP_STR_RENDERER_DISCOVERY, N_("Renderer discovery")),
    CAP(VLC_CAP_RENDERER_PROBE,     VLC_CAP_STR_RENDERER_PROBE,     N_("Renderer probe")),
    CAP(VLC_CAP_AUDIO_VOLUME,       VLC_CAP_STR_AUDIO_VOLUME,       N_("Audio volume")),
    CAP(VLC_CAP_VIDEO_SPLITTER,     VLC_CAP_STR_VIDEO_SPLITTER,     N_("Video splitter")),
    CAP(VLC_CAP_DEMUX_FILTER,       VLC_CAP_STR_DEMUX_FILTER,       N_("Demux filter")),
    CAP(VLC_CAP_ADDONS_FINDER,      VLC_CAP_STR_ADDONS_FINDER,      N_("Addons finder")),
    CAP(VLC_CAP_ADDONS_STORAGE,     VLC_CAP_STR_ADDONS_STORAGE,     N_("Addons storage")),
    CAP(VLC_CAP_AOUT_STREAM,        VLC_CAP_STR_AOUT_STREAM,        N_("Aout stream")),
    CAP(VLC_CAP_ART_FINDER,         VLC_CAP_STR_ART_FINDER,         N_("Art finder")),
    CAP(VLC_CAP_TLS_CLIENT,         VLC_CAP_STR_TLS_CLIENT,         N_("TLS client")),
    CAP(VLC_CAP_TLS_SERVER,         VLC_CAP_STR_TLS_SERVER,         N_("TLS server")),
    CAP(VLC_CAP_AUDIO_RENDERER,     VLC_CAP_STR_AUDIO_RENDERER,     N_("Audio renderer")),
    CAP(VLC_CAP_DIALOGS_PROVIDER,   VLC_CAP_STR_DIALOGS_PROVIDER,   N_("Dialogs provider")),
    CAP(VLC_CAP_EXTENSION,          VLC_CAP_STR_EXTENSION,          N_("Extension")),
    CAP(VLC_CAP_FINGERPRINTER,      VLC_CAP_STR_FINGERPRINTER,      N_("Fingerprinter")),
    CAP(VLC_CAP_MEDIALIBRARY,       VLC_CAP_STR_MEDIALIBRARY,       N_("Medialibrary")),
    CAP(VLC_CAP_META_FETCHER,       VLC_CAP_STR_META_FETCHER,       N_("Meta fetcher")),
    CAP(VLC_CAP_META_READER,        VLC_CAP_STR_META_READER,        N_("Meta reader")),
    CAP(VLC_CAP_META_WRITER,        VLC_CAP_STR_META_WRITER,        N_("Meta writer")),
    CAP(VLC_CAP_STREAM_DIRECTORY,   VLC_CAP_STR_STREAM_DIRECTORY,   N_("Stream directory")),
    CAP(VLC_CAP_STREAM_EXTRACTOR,   VLC_CAP_STR_STREAM_EXTRACTOR,   N_("Stream extractor")),
    CAP(VLC_CAP_SUB_FILTER,         VLC_CAP_STR_SUB_FILTER,         N_("Sub filter")),
    CAP(VLC_CAP_VIDEO_BLENDING,     VLC_CAP_STR_VIDEO_BLENDING,     N_("Video blending")),
    CAP(VLC_CAP_VOD_SERVER,         VLC_CAP_STR_VOD_SERVER,         N_("VoD server")),
    CAP(VLC_CAP_VULKAN,             VLC_CAP_STR_VULKAN,             N_("Vulkan")),
    CAP(VLC_CAP_XML,                VLC_CAP_STR_XML,                N_("XML")),
    CAP(VLC_CAP_XML_READER,         VLC_CAP_STR_XML_READER,         N_("XML reader")),
    CAP(VLC_CAP_CORE,               "core",                         N_("Core program")),
};

static const size_t n_caps_descs =
    sizeof (cap_descriptions) / sizeof (cap_descriptions[0]);

static_assert(VLC_CAP_MAX == (sizeof (cap_descriptions) / sizeof (cap_descriptions[0])) + 1, "capability description table size mismatch");

enum vlc_module_cap vlc_module_cap_from_textid(const char * textid)
{
    if (textid != NULL)
    {
        for (size_t i = 0; i < n_caps_descs; i++)
        {
            if (!strcmp((const char *)&cap_descriptions[i].textid, textid))
                return cap_descriptions[i].cap;
        }
    }
    return VLC_CAP_CUSTOM;
}

const char *vlc_module_cap_get_textid(enum vlc_module_cap cap)
{
    assert(cap != VLC_CAP_CUSTOM && cap != VLC_CAP_INVALID);
    for (size_t i = 0; i < n_caps_descs; i++)
    {
        if (cap_descriptions[i].cap == cap)
            return (const char *)&cap_descriptions[i].textid;
    }
    vlc_assert_unreachable(); /* if reached, table is missing an entry! */
}

const char *vlc_module_cap_get_desc(enum vlc_module_cap cap)
{
    assert(cap != VLC_CAP_CUSTOM && cap != VLC_CAP_INVALID);
    for (size_t i = 0; i < n_caps_descs; i++)
    {
        if (cap_descriptions[i].cap == cap)
            return (const char *)&cap_descriptions[i].name;
    }
    vlc_assert_unreachable(); /* if reached, table is missing an entry! */
}
