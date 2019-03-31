/*****************************************************************************
 * unique_opts.c: test all options are unique
 *****************************************************************************
 * Copyright (C) 2019 VideoLAN and authors
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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <vlc/vlc.h>
#include <vlc_common.h>
#include <vlc_configuration.h>
#include <vlc_modules.h>
#include <vlc_plugin.h>

typedef struct
{
    char *name;
    module_t *mod;
    // An option starting with "no-" is not necessary a negative boolean;
    // it may be useful to know whether or not it is, which this being true
    // indicates!
    bool negative;
    bool obsolete;
    size_t set_idx;
    size_t opt_idx;
} long_t;

typedef struct
{
    char ch;
    module_t *mod;
    size_t set_idx;
    size_t opt_idx;
} short_t;

typedef struct
{
    size_t l_count;
    size_t s_count;
    long_t* l_opts;
    short_t* s_opts;
    module_config_item_t** sets;
    size_t set_count;
} data_t;

static int short_cmp(const void *a, const void *b)
{
    const short_t *ma = a, *mb = b;
    /* Note that qsort() uses _ascending_ order,
       so the smallest module is the one with the biggest score. */
    if (ma->ch == mb->ch)
        return 0;
    if (ma->ch > mb->ch)
        return 1;
    return -1;
}

static int long_cmp(const void *a, const void *b)
{
    const long_t *ma = a, *mb = b;
    return strcmp(ma->name, mb->name);
}

/**
 * Build list of available options
 *
 * This includes negative booleans, private options (those hidden in the GUI,
 * like --help), and obsolete options.
 */
static int build_lists(data_t* data)
{
    /* There is no direct list we can obtain of options, instead we must
       get the list of modules, then get the options from each.
       We will start by looping through to get a count from which to
       allocate the list up front. */

    assert(data != NULL);
    data->l_count = 0;
    data->s_count = 0;
    data->l_opts = NULL;
    data->s_opts = NULL;
    data->sets = NULL;
    data->set_count = 0;

    /* To kick things off, I know there's 1500+ options (excluding negative
       booleans) on my system, so let's alloc a big chunk in expectation */
    #define INIT_COUNT 1500
    #define REALLOC_INCREMENT 100
    data->l_opts = malloc(INIT_COUNT * sizeof(*(data->l_opts)));
    if (data->l_opts == NULL)
    {
        fprintf(stderr, "ERROR: FAILED TO MALLOC\n");
        return -1;
    }

    module_t **mod_list = module_list_get(&data->set_count);
    if (mod_list == NULL)
    {
        fprintf(stderr, "ERROR: FAILED TO GET MODULES LIST\n");
        return -1;
    }

    if (data->set_count == 0)
    {
        fprintf(stderr, "ERROR: NO MODULES!\n");
        return -1;
    }

    data->sets = malloc(data->set_count * sizeof(*(data->sets)));
    if (data->sets == NULL)
    {
        fprintf(stderr, "ERROR: FAILED TO MALLOC\n");
        return -1;
    }

    size_t l_list_max = INIT_COUNT;

    for (size_t i = 0; i < data->set_count; i++)
    {
        unsigned c1 = 0, c2 = 0; //c2 includes boolean negatives
        module_config_item_t* config = module_config_get(mod_list[i], &c1);
        data->sets[i] = config;
        c2 = c1;

        if (config == NULL)
            continue;

        if (c1 == 0)
            continue;

        // Check for booleans, for which we will want an extra entry
        for (unsigned j = 0; j < c1; j++)
        {
            if (config[j].i_type == CONFIG_ITEM_CLASS_BOOL)
                c2++;
        }

        // need more space?
        size_t space_needed = data->l_count + c2;
        if (space_needed > l_list_max)
        {
            // prefer to alloc in chunks of REALLOC_INCREMENT
            l_list_max += REALLOC_INCREMENT;
            if (space_needed > l_list_max)
                l_list_max = space_needed + REALLOC_INCREMENT;
            long_t* temp = realloc(data->l_opts, l_list_max * sizeof(*temp));
            if (temp == NULL)
            {
                fprintf(stderr, "ERROR: FAILED TO REALLOC\n");
                module_config_free(config);
                module_list_free(mod_list);
                return -1;
            }
            data->l_opts = temp;
        }

        for (unsigned j = 0; j < c1; j++)
        {
            // Skip over special cat/subcat/hint type entries
            if (!CONFIG_ITEM(config[j].i_type))
                continue;

            long_t* l_entry = &(data->l_opts)[(data->l_count)++];
            l_entry->name = strdup(config[j].psz_name);
            l_entry->mod = mod_list[i];
            l_entry->negative = false;
            l_entry->obsolete = !!config[j].b_removed;
            l_entry->set_idx = i;
            l_entry->opt_idx = j;

            if (l_entry->name == NULL)
            {
                fprintf(stderr, "ERROR: FAILED TO ALLOCATE STRING\n");
                module_config_free(config);
                module_list_free(mod_list);
                return -1;
            }

            // Add --no-foo
            if (config[j].i_type == CONFIG_ITEM_CLASS_BOOL)
            {
                l_entry = &(data->l_opts)[(data->l_count)++];
                if (asprintf(&l_entry->name, "no-%s", config[j].psz_name) == -1)
                {
                    fprintf(stderr, "ERROR: FAILED TO ALLOCATE STRING\n");
                    module_config_free(config);
                    module_list_free(mod_list);
                    return -1;
                }
                l_entry->mod = mod_list[i];
                l_entry->negative = true;
                l_entry->obsolete = !!config[j].b_removed;
                l_entry->set_idx = i;
                l_entry->opt_idx = j;
            }

            //deal with short option if present
            if (config[j].i_short != '\0')
            {
                //alloc some more space (there are so few we will just do one at a time on demand
                short_t* temp = realloc(data->s_opts, (data->s_count + 1) * sizeof(*temp));
                if (temp == NULL)
                {
                    fprintf(stderr, "ERROR: FAILED TO REALLOC\n");
                    module_config_free(config);
                    module_list_free(mod_list);
                    return -1;
                }
                data->s_opts = temp;

                short_t* s_entry = &(data->s_opts)[(data->s_count)++];
                s_entry->ch = config[j].i_short;
                s_entry->mod = mod_list[i];
                s_entry->set_idx = i;
                s_entry->opt_idx = j;
            }
        }
    }

    module_list_free(mod_list);

    return 0;
}

static inline module_config_item_t *get_option_long(data_t *data, size_t index)
{
    return &data->sets[data->l_opts[index].set_idx][data->l_opts[index].opt_idx];
}

static inline module_config_item_t *get_option_short(data_t *data, size_t index)
{
    return &data->sets[data->s_opts[index].set_idx][data->s_opts[index].opt_idx];
}

/* returns true if problem identified worthy of test failure (error worthy, not just warn condition) */
static bool analyse_long_dups(data_t *data, size_t index, size_t dups)
{
    module_config_item_t *config1 = get_option_long(data, index);
    module_config_item_t *config2 = NULL;

    bool fail = false;

    /* basic details */
    printf("\n--%s\n", data->l_opts[index].name);
    printf("    dups: %lu\n", dups);
    printf("    mods: %s", module_get_object(data->l_opts[index].mod));
    for (size_t i = 1; i <= dups; i++)
        printf(", %s", module_get_object(data->l_opts[index + i].mod));
    printf("\n");

    /* obsolete/non-obsolete clash */
    for (size_t i = 1; i <= dups; i++)
        if (data->l_opts[index].obsolete != data->l_opts[index + i].obsolete)
        {
            printf("\n    CLASH BETWEEN OBSOLETE AND NON-OBSOLETE FOUND");
            fail = true;
            break;
        }

    /* type clash */
    bool type_issue = false;
    for (size_t i = 1; i <= dups; i++)
    {
        config2 = get_option_long(data, index + i);
        if (config1->i_type != config2->i_type)
        {
            printf("\n    DIFFERENT TYPES FOUND!");
            fail = type_issue = true;
            break;
        }
    }

    /* same type, different range clash */
    if (!type_issue)
    {
        bool range_done = false;
        for (size_t i = 1; !range_done && i <= dups; i++)
        {
            config2 = get_option_long(data, index + i);
            switch (CONFIG_CLASS(config1->i_type))
            {
                case CONFIG_ITEM_CLASS_INTEGER:
                {
                    if (config1->min.i != config2->min.i || config1->max.i != config2->max.i)
                    {
                        printf("\n    SAME TYPES BUT DIFFERENT RANGES FOUND!");
                        fail = range_done = true;
                    }
                    break;
                }
                case CONFIG_ITEM_CLASS_FLOAT:
                {
                    if (config1->min.f != config2->min.f || config1->max.f != config2->max.f)
                    {
                        printf("\n    SAME TYPES BUT DIFFERENT RANGES FOUND!");
                        fail = range_done = true;
                    }
                    break;
                }
            }
        }
    }

    /* safety clash */
    for (size_t i = 1; i <= dups; i++)
    {
        config2 = get_option_long(data, index + i);
        if (config1->b_safe != config2->b_safe)
        {
            printf("\n    SAFETY FLAG DIFFERENCE FOUND!");
            fail = true;
            break;
        }
    }

    /* note, value lists are just suggestions, so neglecting to check */

    if (fail)
        printf("\n");

    return fail;
}

/* returns true if problem identified worthy of test failure (error worthy, not just warn condition) */
static bool analyse_short_dups(data_t *data, size_t index, size_t dups)
{
    module_config_item_t *config1 = get_option_short(data, index);
    module_config_item_t *config2 = NULL;

    /* basic details */
    printf("\n-%c\n", data->s_opts[index].ch);
    printf("    dups: %lu\n", dups);
    printf("    mods: %s", module_get_object(data->s_opts[index].mod));
    for (size_t i = 1; i <= dups; i++)
        printf(", %s", module_get_object(data->s_opts[index + i].mod));
    printf("\n");

    /* check whether or not assigned to long options with different names;
       note that short options are always associated with a long option, VLC
       does not allow them to be options on their own; and thus we only need
       to report a problem with them if attached to long options of different
       names, otherwise probelsm are covered by the long option analysis! */
    for (size_t i = 1; i <= dups; i++)
    {
        config2 = get_option_short(data, index + i);
        if (config1->psz_name != config2->psz_name)
        {
            printf("\n    ATTACHED TO DIFFERENT LONG OPTIONS!\n");
            return true;
        }
    }

    return false;
}

static int test_unique()
{
    int ret = -1;
    data_t data = { 0, 0, NULL, NULL, NULL, 0 };
    if (build_lists(&data) == -1)
    {
        fprintf(stderr, "ERROR: FAILED TO BUILD OPTION LISTS!\n");
        goto destroy;
    }

    // Sort them
    qsort(data.l_opts, data.l_count, sizeof(*data.l_opts), long_cmp);
    qsort(data.s_opts, data.s_count, sizeof(*data.s_opts), short_cmp);

    printf("\nOPTIONS:\n");
    printf("========\n\n");

    printf("*NB = negative boolean, Ob = obsolete\n\n");
    printf("Option                                                   NB  Ob  Module\n");
    printf("──────────────────────────────────────────────────────────────────────────────────────────\n");
    for (size_t i = 0; i < data.l_count; i++)
    {
        printf("--%-55s%-4s%-4s%s\n",
            data.l_opts[i].name,
            data.l_opts[i].negative ? "x" : " ",
            data.l_opts[i].obsolete ? "x" : " ",
            module_get_object(data.l_opts[i].mod));
    }
    if (data.l_count == 0)
        printf("\nnone!\n");

    printf("\nOption  Module\n");
    printf("─────────────────────────────\n");
    for (size_t i = 0; i < data.s_count; i++)
    {
        printf("-%c      %s\n",
            data.s_opts[i].ch,
            module_get_object(data.s_opts[i].mod));
    }
    if (data.s_count == 0)
        printf("\nnone!\n");

    printf("\nPROBLEMS:\n");
    printf("=========\n");

    bool long_fail = false;
    bool long_dups = false;
    printf("\nlong option problems:\n");
    for (size_t i = 0; i < (data.l_count - 1); i++)
    {
        size_t dups = 0;
        while ((i + dups) < (data.l_count - 1)
            && strcmp(data.l_opts[i].name, data.l_opts[i+1+dups].name) == 0)
        {
            dups++;
        }
        if (dups > 0)
        {
            long_dups = true;
            long_fail |= analyse_long_dups(&data, i, dups);
        }
        i += dups;
    }
    if (!long_dups)
        printf("\nnone!\n");

    bool short_fail = false;
    bool short_dups = false;
    printf("\nshort option problems:\n");
    for (size_t i = 0; i < (data.s_count - 1); i++)
    {
        size_t dups = 0;
        while ((i + dups) < (data.s_count - 1)
            && data.s_opts[i].ch == data.s_opts[i+1+dups].ch)
        {
            dups++;
        }
        if (dups > 0)
        {
            short_dups = true;
            short_fail |= analyse_short_dups(&data, i, dups);
        }
        i += dups;
    }
    if (!short_dups)
        printf("\nnone!\n");

    printf("\n");

    ret = (long_fail || short_fail) ? -1 : 0;

destroy:
    if (data.l_opts != NULL)
    {
        for (size_t i = 0; i < data.l_count; i++)
        {
            char* name = data.l_opts[i].name;
            if (name != NULL)
                free(name);
        }
        free(data.l_opts);
        data.l_opts = NULL;
    }
    if (data.s_opts != NULL)
    {
        free(data.s_opts);
        data.s_opts = NULL;
    }
    if (data.sets != NULL)
    {
        for (size_t i = 0; i < data.set_count; i++)
        {
            module_config_free(data.sets[i]);
        }
        free(data.sets);
        data.sets = NULL;
    }

    return ret;
}

int main( void )
{
    int ret = -1;

    setenv("VLC_PLUGIN_PATH", "../modules", 1);

    libvlc_instance_t *p_libvlc = libvlc_new(0, NULL);
    assert(p_libvlc != NULL);

    printf("Testing option uniqueness\n");
    if (test_unique() != 0)
        goto error;

    ret = 0;

error:
    libvlc_release(p_libvlc);

    return ret;
}
