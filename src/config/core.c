/*****************************************************************************
 * core.c management of the modules configuration
 *****************************************************************************
 * Copyright (C) 2001-2007 VLC authors and VideoLAN
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_util.h>
#include <vlc_actions.h>
#include <vlc_modules.h>
#include <vlc_plugin.h>

#include "vlc_configuration.h"

#include <errno.h>
#include <assert.h>

#include "configuration.h"
#include "modules/modules.h"

vlc_rwlock_t config_lock = VLC_STATIC_RWLOCK;
bool config_dirty = false;

/* public lock/unlock */
void vlc_config_GetWriteLock() { config_GetWriteLock(); }
void vlc_config_GetReadLock() { config_GetReadLock(); }
void vlc_config_ReleaseLock() { config_ReleaseLock(); }

static inline char *strdupnull (const char *src)
{
    return src ? strdup (src) : NULL;
}

int vlc_config_GetType(module_config_item_t *p_config)
{
    /* sanity checks */
    if( !p_config )
    {
        return 0;
    }

    switch( CONFIG_CLASS(p_config->i_type) )
    {
        case CONFIG_ITEM_CLASS_FLOAT:
            return VLC_VAR_FLOAT;
        case CONFIG_ITEM_CLASS_INTEGER:
            return VLC_VAR_INTEGER;
        case CONFIG_ITEM_CLASS_BOOL:
        case CONFIG_ITEM_CLASS_INFO:
            return VLC_VAR_BOOL;
        case CONFIG_ITEM_CLASS_STRING:
            return VLC_VAR_STRING;
        default:
            return 0;
    }
}

#undef vlc_config_GetInt
int64_t vlc_config_GetInt(module_config_item_t *p_config, bool locked)
{
    /* sanity checks */
    assert(p_config != NULL);
    assert(IsConfigIntegerBasedType(p_config->i_type));

    int64_t val;

    if (!locked) config_GetReadLock();
    val = p_config->value.i;
    if (!locked) config_ReleaseLock();
    return val;
}

#undef vlc_config_GetFloat
float vlc_config_GetFloat(module_config_item_t *p_config, bool locked)
{
    /* sanity checks */
    assert(p_config != NULL);
    assert(IsConfigFloatType(p_config->i_type));

    float val;

    if (!locked) config_GetReadLock();
    val = p_config->value.f;
    if (!locked) config_ReleaseLock();
    return val;
}

#undef vlc_config_GetPsz
char *vlc_config_GetPsz(module_config_item_t *p_config, bool locked)
{
    /* sanity checks */
    assert(p_config != NULL);
    assert(IsConfigStringType (p_config->i_type));

    /* return a copy of the string */
    if (!locked) config_GetReadLock();
    char *psz_value = strdupnull (p_config->value.psz);
    if (!locked) config_ReleaseLock();

    return psz_value;
}

#undef vlc_config_SetPsz
void vlc_config_SetPsz(module_config_item_t *p_config, const char *psz_value, bool locked)
{
    /* sanity checks */
    assert(p_config != NULL);
    assert(IsConfigStringType(p_config->i_type));

    char *str, *oldstr;
    if ((psz_value != NULL) && *psz_value)
        str = strdup (psz_value);
    else
        str = NULL;

    if (!locked) config_GetWriteLock();
    oldstr = (char *)p_config->value.psz;
    p_config->value.psz = str;
    config_dirty = true;
    if (!locked) config_ReleaseLock();

    free (oldstr);
}

#undef vlc_config_SetInt
void vlc_config_SetInt(module_config_item_t *p_config, int64_t i_value, bool locked)
{
    /* sanity checks */
    assert(p_config != NULL);
    assert(IsConfigIntegerBasedType(p_config->i_type));

    if (i_value < p_config->min.i)
        i_value = p_config->min.i;
    if (i_value > p_config->max.i)
        i_value = p_config->max.i;

    if (!locked) config_GetWriteLock();
    p_config->value.i = i_value;
    config_dirty = true;
    if (!locked) config_ReleaseLock();
}

#undef vlc_config_SetFloat
void vlc_config_SetFloat(module_config_item_t *p_config, float f_value, bool locked)
{
    /* sanity checks */
    assert(p_config != NULL);
    assert(IsConfigFloatType(p_config->i_type));

    /* if f_min == f_max == 0, then do not use them */
    if ((p_config->min.f == 0.f) && (p_config->max.f == 0.f))
        ;
    else if (f_value < p_config->min.f)
        f_value = p_config->min.f;
    else if (f_value > p_config->max.f)
        f_value = p_config->max.f;

    if (!locked) config_GetWriteLock();
    p_config->value.f = f_value;
    config_dirty = true;
    if (!locked) config_ReleaseLock();
}

ssize_t vlc_config_GetIntChoices(module_config_item_t *cfg,
                             int64_t **restrict values, char ***restrict texts)
{
    *values = NULL;
    *texts = NULL;

    assert(cfg != NULL);

    size_t count = cfg->list_count;
    if (count == 0)
    {
        if (cfg->list_cb_name == NULL)
            return 0;

        if (module_Map(NULL, cfg->owner))
        {
            errno = EIO;
            return -1;
        }

        if (cfg->list.i_cb == NULL)
            return 0;
        return cfg->list.i_cb(cfg->psz_name, values, texts);
    }

    int64_t *vals = vlc_alloc (count, sizeof (*vals));
    char **txts = vlc_alloc (count, sizeof (*txts));
    if (vals == NULL || txts == NULL)
    {
        errno = ENOMEM;
        goto error;
    }

    for (size_t i = 0; i < count; i++)
    {
        vals[i] = cfg->list.i[i];
        /* FIXME: use module_gettext() instead */
        txts[i] = strdup ((cfg->list_text[i] != NULL)
                                       ? vlc_gettext (cfg->list_text[i]) : "");
        if (unlikely(txts[i] == NULL))
        {
            for (int j = i - 1; j >= 0; --j)
                free(txts[j]);
            errno = ENOMEM;
            goto error;
        }
    }

    *values = vals;
    *texts = txts;
    return count;
error:

    free(vals);
    free(txts);
    return -1;
}


static ssize_t config_ListModules (const char *cap, char ***restrict values,
                                   char ***restrict texts)
{
    module_t **list;
    ssize_t n = vlc_module_list_cap_ext (&list, vlc_module_cap_from_textid(cap), cap);
    if (unlikely(n < 0))
    {
        *values = *texts = NULL;
        return n;
    }

    char **vals = malloc ((n + 2) * sizeof (*vals));
    char **txts = malloc ((n + 2) * sizeof (*txts));
    if (!vals || !txts)
    {
        free (vals);
        free (txts);
        *values = *texts = NULL;
        return -1;
    }

    ssize_t i = 0;

    vals[i] = NULL;
    txts[i] = strdup (_("Automatic"));
    if (!vals[i] || !txts[i])
        goto error;

    ++i;
    for (; i <= n; i++)
    {
        vals[i] = strdup (module_get_object (list[i - 1]));
        txts[i] = strdup (module_gettext (list[i - 1],
                               vlc_module_GetLongName (list[i - 1])));
        if( !vals[i] || !txts[i])
            goto error;
    }
    vals[i] = strdup ("none");
    txts[i] = strdup (_("Disable"));
    if( !vals[i] || !txts[i])
        goto error;

    *values = vals;
    *texts = txts;
    module_list_free (list);
    return i + 1;

error:
    for (ssize_t j = 0; j <= i; ++j)
    {
        free (vals[j]);
        free (txts[j]);
    }
    free(vals);
    free(txts);
    module_list_free (list);
    *values = *texts = NULL;
    return -1;
}

ssize_t vlc_config_GetPszChoices(module_config_item_t *cfg,
                             char ***restrict values, char ***restrict texts)
{
    *values = *texts = NULL;

    if (cfg == NULL)
    {
        errno = ENOENT;
        return -1;
    }

    switch (cfg->i_type)
    {
        case CONFIG_ITEM_MODULE:
            return config_ListModules (cfg->min.psz, values, texts);
        default:
            if (!IsConfigStringType (cfg->i_type))
            {
                errno = EINVAL;
                return -1;
            }
            break;
    }

    size_t count = cfg->list_count;
    if (count == 0)
    {
        if (cfg->list_cb_name == NULL)
            return 0;

        if (module_Map(NULL, cfg->owner))
        {
            errno = EIO;
            return -1;
        }

        if (cfg->list.psz_cb == NULL)
            return 0;
        return cfg->list.psz_cb(cfg->psz_name, values, texts);
    }

    char **vals = malloc (sizeof (*vals) * count);
    char **txts = malloc (sizeof (*txts) * count);
    if (!vals || !txts)
    {
        free (vals);
        free (txts);
        errno = ENOMEM;
        return -1;
    }

    size_t i;
    for (i = 0; i < count; i++)
    {
        vals[i] = strdup ((cfg->list.psz[i] != NULL) ? cfg->list.psz[i] : "");
        /* FIXME: use module_gettext() instead */
        txts[i] = strdup ((cfg->list_text[i] != NULL)
                                       ? vlc_gettext (cfg->list_text[i]) : "");
        if (!vals[i] || !txts[i])
            goto error;
    }

    *values = vals;
    *texts = txts;
    return count;

error:
    for (size_t j = 0; j <= i; ++j)
    {
        free (vals[j]);
        free (txts[j]);
    }
    free(vals);
    free(txts);
    errno = ENOMEM;
    return -1;
}

static int confcmp (const void *a, const void *b)
{
    const module_config_item_t *const *ca = a, *const *cb = b;

    return strcmp ((*ca)->psz_name, (*cb)->psz_name);
}

static int confnamecmp (const void *key, const void *elem)
{
    const module_config_item_t *const *conf = elem;

    return strcmp (key, (*conf)->psz_name);
}

static struct
{
    module_config_item_t **list;
    size_t count;
} config = { NULL, 0 };

/**
 * Index the configuration items by name for faster lookups.
 */
int config_SortConfig (void)
{
    vlc_plugin_t *p;
    size_t nconf = 0;

    for (p = vlc_plugins; p != NULL; p = p->next)
         nconf += p->conf.count;

    module_config_item_t **clist = vlc_alloc (nconf, sizeof (*clist));
    if (unlikely(clist == NULL))
        return VLC_ENOMEM;

    nconf = 0;
    for (p = vlc_plugins; p != NULL; p = p->next)
    {
        module_config_item_t *item, *end;

        for (item = p->conf.items, end = item + p->conf.size;
             item < end;
             item++)
        {
            if (!CONFIG_ITEM(item->i_type))
                continue; /* ignore hints */
            clist[nconf++] = item;
        }
    }

    qsort (clist, nconf, sizeof (*clist), confcmp);

    config.list = clist;
    config.count = nconf;
    return VLC_SUCCESS;
}

void config_UnsortConfig (void)
{
    module_config_item_t **clist;

    clist = config.list;
    config.list = NULL;
    config.count = 0;

    free (clist);
}

module_config_item_t *vlc_config_FindItem(const char *name)
{
    if (unlikely(name == NULL))
        return NULL;

    module_config_item_t *const *p;
    p = bsearch (name, config.list, config.count, sizeof (*p), confnamecmp);
    return p ? *p : NULL;
}

/**
 * Destroys an array of configuration items.
 * \param config start of array of items
 * \param confsize number of items in the array
 */
void config_Free (module_config_item_t *tab, size_t confsize)
{
    for (size_t j = 0; j < confsize; j++)
    {
        module_config_item_t *p_item = &tab[j];

        if (IsConfigStringType (p_item->i_type))
        {
            free (p_item->value.psz);
            if (p_item->list_count)
                free (p_item->list.psz);
        }

        free (p_item->list_text);
    }

    free (tab);
}

void config_ResetAll(void)
{
    config_GetWriteLock();
    for (vlc_plugin_t *p = vlc_plugins; p != NULL; p = p->next)
    {
        for (size_t i = 0; i < p->conf.size; i++ )
        {
            module_config_item_t *p_config = p->conf.items + i;

            if (IsConfigIntegerBasedType (p_config->i_type))
                p_config->value.i = p_config->orig.i;
            else
            if (IsConfigFloatType (p_config->i_type))
                p_config->value.f = p_config->orig.f;
            else
            if (IsConfigStringType (p_config->i_type))
            {
                free ((char *)p_config->value.psz);
                p_config->value.psz =
                        strdupnull (p_config->orig.psz);
            }
        }
    }
    config_dirty = true;
    config_ReleaseLock();
}
