/*****************************************************************************
 * entry.c : Callbacks for module entry point
 *****************************************************************************
 * Copyright (C) 2007 VLC authors and VideoLAN
 * Copyright © 2007-2008 Rémi Denis-Courmont
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

#include <vlc_common.h>
#include <vlc_plugin.h>
#include <assert.h>
#include <stdarg.h>
#include <limits.h>
#include <float.h>
#ifdef HAVE_SEARCH_H
# include <search.h>
#endif

#include "modules/modules.h"
#include "config/configuration.h"
#include "libvlc.h"

module_t *vlc_module_create(vlc_plugin_t *plugin)
{
    module_t *module = malloc (sizeof (*module));
    if (module == NULL)
        return NULL;

    /* pre-alloc space for one shortcut (object name is stored in first) */
    const char **shortcuts = malloc (sizeof ( *shortcuts ));
    if (shortcuts == NULL)
    {
        free(module);
        return NULL;
    }
    shortcuts[0] = NULL;

    /* NOTE XXX: For backward compatibility with preferences UIs, the first
     * module must stay first. That defines under which module, the
     * configuration items of the plugin belong. The order of the following
     * entries is irrelevant. */
    module_t *parent = plugin->module;
    if (parent == NULL)
    {
        module->next = NULL;
        plugin->module = module;
    }
    else
    {
        module->next = parent->next;
        parent->next = module;
    }

    plugin->modules_count++;
    module->plugin = plugin;

    module->psz_shortname = NULL;
    module->psz_longname = NULL;
    module->psz_help = NULL;
    module->pp_shortcuts = shortcuts;
    module->i_shortcuts = 0;
    module->i_score = 0;
    module->capability = VLC_CAP_INVALID;
    module->psz_capability = NULL;
    module->activate_name = NULL;
    module->deactivate_name = NULL;
    module->pf_activate = NULL;
    module->pf_deactivate = NULL;
    return module;
}

/**
 * Destroys a module.
 */
void vlc_module_destroy (module_t *module)
{
    while (module != NULL)
    {
        module_t *next = module->next;

        free(module->pp_shortcuts);
        free(module);
        module = next;
    }
}

vlc_plugin_t *vlc_plugin_create(void)
{
    vlc_plugin_t *plugin = malloc(sizeof (*plugin));
    if (unlikely(plugin == NULL))
        return NULL;

    plugin->modules_count = 0;
    plugin->textdomain = NULL;
    plugin->conf.items = NULL;
    plugin->conf.size = 0;
    plugin->conf.count = 0;
    plugin->conf.booleans = 0;
#ifdef HAVE_DYNAMIC_PLUGINS
    plugin->unloadable = true;
    atomic_init(&plugin->handle, 0);
    plugin->abspath = NULL;
    plugin->path = NULL;
#endif
    plugin->module = NULL;

    return plugin;
}

/**
 * Destroys a plug-in.
 * @warning If the plug-in was dynamically loaded in memory, the library handle
 * and associated memory mappings and linker resources will be leaked.
 */
void vlc_plugin_destroy(vlc_plugin_t *plugin)
{
    assert(plugin != NULL);
#ifdef HAVE_DYNAMIC_PLUGINS
    assert(!plugin->unloadable || atomic_load(&plugin->handle) == 0);
#endif

    if (plugin->module != NULL)
        vlc_module_destroy(plugin->module);

    config_Free(plugin->conf.items, plugin->conf.size);
#ifdef HAVE_DYNAMIC_PLUGINS
    free(plugin->abspath);
    free(plugin->path);
#endif
    free(plugin);
}

static module_config_item_t *vlc_config_create(vlc_plugin_t *plugin)
{
    unsigned confsize = plugin->conf.size;
    module_config_item_t *tab = plugin->conf.items;

    if (unlikely(plugin->conf.size == UINT16_MAX))
        return NULL;

    if ((confsize & 0xf) == 0)
    {
        tab = realloc_or_free (tab, (confsize + 17) * sizeof (*tab));
        if (unlikely(tab == NULL))
            return NULL;

        plugin->conf.items = tab;
    }

    memset (tab + confsize, 0, sizeof (tab[confsize]));
    tab += confsize;
    tab->owner = plugin;

    plugin->conf.size++;

    return tab;
}

/**
 * Plug-in descriptor callback.
 *
 * This callback populates modules, configuration items and properties of a
 * plug-in from the plug-in descriptor.
 */
static int vlc_plugin_desc_cb(vlc_plugin_t *plugin, enum vlc_plugin_desc_actions action, void *target, ...)
{
    module_t *module = target;
    module_config_item_t **item = target;
    va_list ap;
    int ret = 0;

#ifdef HAVE_DYNAMIC_PLUGINS
# define print_err_plugin_path \
        const char* err_plugin_path = (plugin->abspath != NULL) ? plugin->abspath : "NULL"; \
        fprintf(stderr, "    %s: %s\n", _("plugin path"), err_plugin_path);
#else
# define print_err_plugin_path
#endif

#define print_module_error(msg) \
    do { \
        const char* err_mod_name = (module && module->i_shortcuts != 0 \
            && module->pp_shortcuts[0] != NULL) ? module->pp_shortcuts[0] : "NULL"; \
        fprintf(stderr, "%s: %s\n", _("LibVLC module error"), msg); \
        print_err_plugin_path \
        fprintf(stderr, "    %s: %s\n", _("module name"), err_mod_name); \
    } while (0)

#define print_config_error(msg) \
    do { \
        const char* err_cfg_name = (item && *item && (*item)->psz_name != NULL) ? (*item)->psz_name : "NULL"; \
        fprintf(stderr, "%s: %s\n", _("LibVLC plugin config error"), msg); \
        print_err_plugin_path \
        fprintf(stderr, "    %s: %s\n", _("option name"), err_cfg_name); \
    } while (0)

    va_start (ap, target);
    switch (action)
    {
        case VLC_MODULE_CREATE:
        {
            /* Note, for some plugins an add_submodule() call is made without
               first using the implicitly declared initial module. In some
               cases this is a mistake, however in others it is deliberate in
               order to get a generic plugin title for help output where the
               plugin has multiple modules. We thus do *NOT* want to reject the
               call here for a new module when capability is not be set (as an
               optimisation or for hygiene) as that would prevent this. */

            module_t *super = plugin->module;
            module_t *submodule = vlc_module_create(plugin);
            if (unlikely(submodule == NULL))
            {
                ret = -1;
                break;
            }

            *(va_arg (ap, module_t **)) = submodule;
            if (super == NULL)
                break;

            /* Inheritance. Ugly!! */
            submodule->pp_shortcuts[0] = super->pp_shortcuts[0];
            submodule->i_shortcuts = 1; /* object name */

            submodule->psz_shortname = super->psz_shortname;
            submodule->psz_longname = super->psz_longname;
            break;
        }

        case VLC_CONFIG_CREATE_SPECIAL:
        case VLC_CONFIG_CREATE_OBSOLETE:
        case VLC_CONFIG_CREATE_COMMON:
        case VLC_CONFIG_CREATE_MOD_SELECT:
        {
            config_item_params_t *params = va_arg (ap, config_item_params_t *);

            module_config_item_t *new_item = vlc_config_create(plugin);
            if (unlikely(new_item == NULL))
            {
                print_config_error(_("too many config items, or malloc failure"));
                ret = -1;
                break;
            }
            *item = new_item;

            uint16_t type;
            switch (action)
            {
                case VLC_CONFIG_CREATE_SPECIAL:
                    if (unlikely(params->special.type == CONFIG_SUBCATEGORY &&
                                 !vlc_config_IntSubcatIsValid((int)params->special.id)))
                    {
                        print_config_error(_("invalid subcategory"));
                        ret = -1;
                    }
                    type =
                    new_item->i_type = params->special.type;
                    new_item->orig.i = /* FIXME: old code put it in both of these */
                    new_item->value.i = (int)params->special.id;
                    new_item->psz_text = params->special.text;
                    new_item->psz_longtext = params->special.longtext;
                    break;
                case VLC_CONFIG_CREATE_OBSOLETE:
                    type =
                    new_item->i_type = params->obsolete.type;
                    new_item->psz_name = params->obsolete.name;
                    new_item->b_removed = true;
                    break;
                case VLC_CONFIG_CREATE_COMMON:
                    type =
                    new_item->i_type = params->basic_item.type;
                    new_item->psz_name = params->basic_item.name;
                    new_item->orig = params->basic_item.default_val;
                    new_item->value = params->basic_item.default_val;
                    new_item->psz_text = params->basic_item.text;
                    new_item->psz_longtext = params->basic_item.longtext;

                    if (IsConfigIntegerBasedType (type))
                    {
                        new_item->max.i = INT64_MAX;
                        new_item->min.i = INT64_MIN;
                    }
                    else if (IsConfigFloatType (type))
                    {
                        new_item->max.f = FLT_MAX;
                        new_item->min.f = -FLT_MAX;
                    }
                    if (IsConfigStringType (type))
                    {
                        new_item->orig.psz = (new_item->orig.psz) ? strdup(new_item->orig.psz) : NULL;
                        new_item->value.psz = (new_item->value.psz) ? strdup(new_item->value.psz) : NULL;
                    }
                    break;
                case VLC_CONFIG_CREATE_MOD_SELECT:
                    type =
                    new_item->i_type = params->mod_select_item.type;
                    new_item->psz_name = params->mod_select_item.name;
                    new_item->psz_type = params->mod_select_item.cap;
                    new_item->min.i = params->mod_select_item.subcategory;
                    new_item->max.i = 0;
                    new_item->psz_text = params->mod_select_item.text;
                    new_item->psz_longtext = params->mod_select_item.longtext;
                    break;
                default:
                    unreachable();
                    break;
            }
            if (CONFIG_ITEM(type))
            {
                plugin->conf.count++;
                if (CONFIG_CLASS(type) == CONFIG_ITEM_CLASS_BOOL)
                    plugin->conf.booleans++;
            }
            if (unlikely(action != VLC_CONFIG_CREATE_SPECIAL && new_item->psz_name == NULL))
            {
                print_config_error(_("name cannot be null"));
                ret = -1;
            }
            break;
        }
        case VLC_MODULE_SHORTCUT:
        {
            unsigned i_shortcuts = va_arg (ap, unsigned);
            unsigned index = module->i_shortcuts;
            /* The cache loader accept only a small number of shortcuts */
            if (unlikely(i_shortcuts + index > MODULE_SHORTCUT_MAX))
            {
                print_module_error(_("too many module shortcuts"));
                ret = -1;
                break;
            }

            const char *const *tab = va_arg (ap, const char *const *);
            const char **pp = realloc (module->pp_shortcuts,
                                       sizeof (pp[0]) * (index + i_shortcuts));
            if (unlikely(pp == NULL))
            {
                ret = -1;
                break;
            }
            module->pp_shortcuts = pp;
            module->i_shortcuts = index + i_shortcuts;
            pp += index;
            for (unsigned i = 0; i < i_shortcuts; i++)
                pp[i] = tab[i];
            break;
        }
        case VLC_MODULE_CAPABILITY:
            module->capability = va_arg (ap, enum vlc_module_cap);
            if (unlikely(!vlc_module_int_is_valid_cap((int)module->capability)))
            {
                if (module->capability == VLC_CAP_CUSTOM)
                    print_module_error(_("invalid capability, for a custom " \
                                         "capability use set_custom_capability()"));
                else
                    print_module_error(_("invalid capability"));
                ret = -1;
            }
            break;

        case VLC_MODULE_CUSTOM_CAPABILITY:
            module->capability = VLC_CAP_CUSTOM;
            module->psz_capability = va_arg (ap, const char *);
            break;

        case VLC_MODULE_SCORE:
            module->i_score = va_arg (ap, int);
            break;

        case VLC_MODULE_CB_OPEN:
        {
            const char * cb_name = va_arg (ap, const char *);
            vlc_activate_cb cb = va_arg (ap, vlc_activate_cb);

            if (cb != NULL && (cb_name == NULL || strlen(cb_name) == 0))
            {
                print_module_error(_("callback name cannot be null or empty"));
                ret = -1;
                break;
            }
            module->pf_activate = cb;
            module->activate_name = (cb != NULL) ? cb_name : NULL;
            break;
        }
        case VLC_MODULE_CB_CLOSE:
        {
            const char * cb_name = va_arg (ap, const char *);
            vlc_deactivate_cb cb = va_arg (ap, vlc_deactivate_cb);

            if (cb != NULL && (cb_name == NULL || strlen(cb_name) == 0))
            {
                print_module_error(_("callback name cannot be null or empty"));
                ret = -1;
                break;
            }
            module->pf_deactivate = cb;
            module->deactivate_name = (cb != NULL) ? cb_name : NULL;
            break;
        }
        case VLC_MODULE_NO_UNLOAD:
#ifdef HAVE_DYNAMIC_PLUGINS
            plugin->unloadable = false;
#endif
            break;

        case VLC_MODULE_NAME:
        {
            const char *name = va_arg (ap, const char *);
            if (unlikely(name == NULL || name[0] == '\0'))
            {
                print_module_error(_("object name cannot be null or empty"));
                ret = -1;
                break;
            }

            module->pp_shortcuts[0] = name;
            module->i_shortcuts = 1;
            break;
        }
        case VLC_MODULE_SHORTNAME:
            module->psz_shortname = va_arg (ap, const char *);
            break;

        case VLC_MODULE_DESCRIPTION:
            module->psz_longname = va_arg (ap, const char *);
            break;

        case VLC_MODULE_HELP:
            module->psz_help = va_arg (ap, const char *);
            break;

        case VLC_MODULE_TEXTDOMAIN:
            plugin->textdomain = va_arg(ap, const char *);
            break;

        case VLC_CONFIG_VOLATILE:
            if (CONFIG_ITEM((*item)->i_type))
                (*item)->b_unsaveable = true;
            break;

        case VLC_CONFIG_PRIVATE:
            if (CONFIG_ITEM((*item)->i_type))
                (*item)->b_internal = true;
            break;

        case VLC_CONFIG_REMOVED:
            if (CONFIG_ITEM((*item)->i_type))
                (*item)->b_removed = true;
            break;

        case VLC_CONFIG_SAFE:
            if (CONFIG_ITEM((*item)->i_type))
                (*item)->b_safe = true;
            break;

        case VLC_CONFIG_SHORT:
        {
            config_item_params_t *params = va_arg (ap, config_item_params_t *);
            if (unlikely(!CONFIG_ITEM((*item)->i_type)))
                break;

            char c = params->short_char.ch;
            if (unlikely(c == '\0' || c == '?' || c == ':'))
            {
                print_config_error(_("invalid short option"));
                ret = -1;
            }
            else
                (*item)->i_short = c;
            break;
        }
        case VLC_CONFIG_INT_RANGE:
        {
            config_item_params_t *params = va_arg (ap, config_item_params_t *);
            if (unlikely(!IsConfigIntegerType((*item)->i_type)))
            {
                print_config_error(_("int range only applies to int items"));
                ret = -1;
                break;
            }
            (*item)->min.i = params->integer_range.min;
            (*item)->max.i = params->integer_range.max;
            break;
        }
        case VLC_CONFIG_FLOAT_RANGE:
        {
            config_item_params_t *params = va_arg (ap, config_item_params_t *);
            if (unlikely(!IsConfigFloatType((*item)->i_type)))
            {
                print_config_error(_("float range only applies to float items"));
                ret = -1;
                break;
            }
            (*item)->min.f = params->float_range.min;
            (*item)->max.f = params->float_range.max;
            break;
        }
        case VLC_CONFIG_STRING_LIST:
        {
            config_item_params_t *params = va_arg (ap, config_item_params_t *);
            if (unlikely(!IsConfigStringType((*item)->i_type)))
            {
                print_config_error(_("string list only applies to string items"));
                ret = -1;
                break;
            }
            if (unlikely((*item)->list_count != 0 || (*item)->list_cb_name != NULL))
            {
                print_config_error(_("list properties already set"));
                ret = -1;
                break;
            }

            size_t len = params->string_list.count;

            /* ignore null terminator entries, as may originate from 3rd-party arrays */
            if (unlikely(len > 0 && !params->string_list.list[len-1] && !params->string_list.text[len-1]))
                --len;

            if (unlikely(len == 0))
                break; /* nothing to do */

            /* Copy values */
            const char *const *src = params->string_list.list;
            const char **dst = xmalloc (sizeof (const char *) * len);

            memcpy(dst, src, sizeof (const char *) * len);
            (*item)->list.psz = dst;

            /* Copy textual descriptions */
            const char *const *text = params->string_list.text;
            const char **dtext = xmalloc (sizeof (const char *) * len);

            memcpy(dtext, text, sizeof (const char *) * len);
            (*item)->list_text = dtext;
            (*item)->list_count = len;
            break;
        }
        case VLC_CONFIG_INT_LIST:
        {
            config_item_params_t *params = va_arg (ap, config_item_params_t *);
            if (unlikely(!IsConfigIntegerType((*item)->i_type)))
            {
                print_config_error(_("int list only applies to int items"));
                ret = -1;
                break;
            }
            if (unlikely((*item)->list_count != 0 || (*item)->list_cb_name != NULL))
            {
                print_config_error(_("list properties already set"));
                ret = -1;
                break;
            }

            size_t len = params->int_list.count;
            if (unlikely(len == 0))
                break; /* nothing to do */

            /* Copy values */
            (*item)->list.i = params->int_list.list;

            /* Copy textual descriptions */
            /* XXX: item->list_text[len + 1] is probably useless. */
            const char *const *text = params->int_list.text;
            const char **dtext = xmalloc (sizeof (const char *) * (len + 1));

            memcpy(dtext, text, sizeof (const char *) * len);
            (*item)->list_text = dtext;
            (*item)->list_count = len;
            break;
        }
        case VLC_CONFIG_STRING_LIST_CB:
        {
            config_item_params_t *params = va_arg (ap, config_item_params_t *);
            if (unlikely(!IsConfigStringType((*item)->i_type)))
            {
                print_config_error(_("string list callback only applies to string items"));
                ret = -1;
                break;
            }
            if (unlikely((*item)->list_count != 0 || (*item)->list_cb_name != NULL))
            {
                print_config_error(_("list properties already set"));
                ret = -1;
                break;
            }
            (*item)->list_cb_name = params->string_list_cb.name;
            (*item)->list.psz_cb = params->string_list_cb.cb;
            break;
        }
        case VLC_CONFIG_INT_LIST_CB:
        {
            config_item_params_t *params = va_arg (ap, config_item_params_t *);
            if (unlikely(!IsConfigIntegerType((*item)->i_type)))
            {
                print_config_error(_("int list callback only applies to int items"));
                ret = -1;
                break;
            }
            if (unlikely((*item)->list_count != 0 || (*item)->list_cb_name != NULL))
            {
                print_config_error(_("list properties already set"));
                ret = -1;
                break;
            }
            (*item)->list_cb_name = params->int_list_cb.name;
            (*item)->list.i_cb = params->int_list_cb.cb;
            break;
        }
        default:
            fprintf(stderr, "%s (%d)\n", _("LibVLC plugin error: unknown descriptor action"), (int)action);
            ret = -1;
            break;
    }

#undef print_err_plugin_path
#undef print_module_error
#undef print_config_error

    va_end (ap);
    return ret;
}

/**
 * Runs a plug-in descriptor.
 *
 * This loads the plug-in meta-data in memory.
 */
vlc_plugin_t *vlc_plugin_describe(vlc_plugin_cb *entry, const char *path)
{
    vlc_plugin_t *plugin = vlc_plugin_create();
    if (unlikely(plugin == NULL))
        return NULL;

    if (path) /* provide for descriptor callback error message use */
        plugin->abspath = xstrdup(path);

    if (entry(vlc_plugin_desc_cb, plugin) != 0)
    {
        vlc_plugin_destroy(plugin); /* partially initialized plug-in... */
        plugin = NULL;
    }

    /* guard against potential plugin modification */
    free(plugin->abspath);
    plugin->abspath = NULL;

    return plugin;
}

struct vlc_plugin_symbol
{
    const char *name;
    void *addr;
};

static int vlc_plugin_symbol_compare(const void *a, const void *b)
{
    const struct vlc_plugin_symbol *sa = a , *sb = b;

    return strcmp(sa->name, sb->name);
}

/**
 * Plug-in symbols callback.
 *
 * This callback generates a mapping of plugin symbol names to symbol
 * addresses.
 */
static int vlc_plugin_gpa_cb(vlc_plugin_t *plugin, enum vlc_plugin_desc_actions action, void *target, ...)
{
    void **rootp = (void **) plugin;
    va_list ap;
    config_item_params_t *cfg_params;

    (void) target;

    switch (action)
    {
        case VLC_MODULE_CB_OPEN:
        case VLC_MODULE_CB_CLOSE:
        case VLC_CONFIG_STRING_LIST_CB:
        case VLC_CONFIG_INT_LIST_CB:
            break;
        default:
            return 0;
    }

    va_start(ap, target);

    struct vlc_plugin_symbol *sym = malloc(sizeof (*sym));
    if (unlikely(!sym)) return -1;

    switch (action)
    {
        case VLC_MODULE_CB_OPEN:
            sym->name = va_arg(ap, const char *);
            sym->addr = (void *) va_arg(ap, vlc_activate_cb);
            break;
        case VLC_MODULE_CB_CLOSE:
            sym->name = va_arg(ap, const char *);
            sym->addr = (void *) va_arg(ap, vlc_deactivate_cb);
            break;
        case VLC_CONFIG_STRING_LIST_CB:
            cfg_params = va_arg (ap, config_item_params_t *);
            sym->name = cfg_params->string_list_cb.name;
            sym->addr = (void *) cfg_params->string_list_cb.cb;
            break;
        case VLC_CONFIG_INT_LIST_CB:
            cfg_params = va_arg (ap, config_item_params_t *);
            sym->name = cfg_params->int_list_cb.name;
            sym->addr = (void *) cfg_params->int_list_cb.cb;
            break;
        default:
            unreachable();
            break;
    }

    va_end (ap);

    struct vlc_plugin_symbol **symp = tsearch(sym, rootp,
                                              vlc_plugin_symbol_compare);
    if (unlikely(symp == NULL))
    {   /* Memory error */
        free(sym);
        return -1;
    }

    if (*symp != sym)
    {   /* Duplicate symbol */
        assert((*symp)->addr == sym->addr);
        free(sym);
    }
    return 0;
}

/**
 * Gets the symbols of a plugin.
 *
 * This function generates a list of symbol names and addresses for a given
 * plugin descriptor. The result can be used with vlc_plugin_get_symbol()
 * to resolve a symbol name to an address.
 *
 * The result must be freed with vlc_plugin_free_symbols(). The result is only
 * meaningful until the plugin is unloaded.
 */
static void *vlc_plugin_get_symbols(vlc_plugin_cb *entry)
{
    vlc_plugin_t *root = NULL;

    if (entry(vlc_plugin_gpa_cb, root))
    {
        tdestroy(root, free);
        return NULL;
    }

    return root;
}

static void vlc_plugin_free_symbols(void *root)
{
    tdestroy(root, free);
}

static int vlc_plugin_get_symbol(void *root, const char *name,
                                 void **restrict addrp)
{
    if (name == NULL)
    {
        *addrp = NULL;
        return 0;
    }

    const struct vlc_plugin_symbol **symp = tfind(&name, &root,
                                                  vlc_plugin_symbol_compare);

    if (symp == NULL)
        return -1;

    *addrp = (*symp)->addr;
    return 0;
}

int vlc_plugin_resolve(vlc_plugin_t *plugin, vlc_plugin_cb *entry)
{
    void *syms = vlc_plugin_get_symbols(entry);
    int ret = -1;

    /* Resolve modules activate/deactivate callbacks */
    for (module_t *module = plugin->module;
         module != NULL;
         module = module->next)
    {
        if (vlc_plugin_get_symbol(syms, module->activate_name,
                                  &module->pf_activate)
         || vlc_plugin_get_symbol(syms, module->deactivate_name,
                                  &module->pf_deactivate))
            goto error;
    }

    /* Resolve configuration callbacks */
    for (size_t i = 0; i < plugin->conf.size; i++)
    {
        module_config_item_t *item = plugin->conf.items + i;
        void *cb;

        if (item->list_cb_name == NULL)
            continue;
        if (vlc_plugin_get_symbol(syms, item->list_cb_name, &cb))
            goto error;

        if (IsConfigIntegerBasedType (item->i_type))
            item->list.i_cb = cb;
        else
        if (IsConfigStringType (item->i_type))
            item->list.psz_cb = cb;
    }

    ret = 0;
error:
    vlc_plugin_free_symbols(syms);
    return ret;
}
