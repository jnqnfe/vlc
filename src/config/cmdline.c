/*****************************************************************************
 * cmdline.c: command line parsing
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
#include "../libvlc.h"
#include <vlc_actions.h>
#include <vlc_charset.h>
#include <vlc_modules.h>
#include <vlc_plugin.h>

#include "vlc_getopt.h"
#include "vlc_jaro_winkler.h"

#include "configuration.h"
#include "console.h"
#include "modules/modules.h"

#include <assert.h>

#define RED TS_RED_BOLD
#define YELLOW TS_YELLOW_BOLD

#undef config_LoadCmdLine
/**
 * Parse command line for configuration options.
 *
 * Now that the module_bank has been initialized, we can dynamically
 * generate the longopts structure used by getops. We have to do it this way
 * because we don't know (and don't want to know) in advance the configuration
 * options used (ie. exported) by each module.
 *
 * @param p_this object to write command line options as variables to
 * @param i_argc number of command line arguments
 * @param ppsz_args commandl ine arguments [IN/OUT]
 * @param pindex NULL to ignore unknown options,
 *               otherwise index of the first non-option argument [OUT]
 * @return 0 on success, -1 on error.
 */
int config_LoadCmdLine( vlc_object_t *p_this, int i_argc,
                        const char *ppsz_argv[], int *pindex )
{
    int i_cmd, i_index, i_opts, i_shortopts = 0, flag, i_verbose = 0;
    struct vlc_option *p_longopts;
    const char **argv_copy = NULL;
    static bool printed_obs_neg_bool_warn = false;
#define b_ignore_errors (pindex == NULL)

    /* Short options */
    const module_config_item_t *pp_shortopts[256];
    char *psz_shortopts;

    /*
     * Generate the longopts and shortopts structures used by getopt_long
     */

    i_opts = 0;
    for (const vlc_plugin_t *p = vlc_plugins; p != NULL; p = p->next)
        /* count the number of exported configuration options (to allocate
         * longopts). We also need to allocate space for two options when
         * dealing with boolean to allow for --foo and --no-foo */
        i_opts += p->conf.count + 2 * p->conf.booleans;

    p_longopts = vlc_alloc( i_opts + 1, sizeof(*p_longopts)  );
    if( p_longopts == NULL )
        return -1;

    psz_shortopts = malloc( 2 * i_opts + 1 );
    if( psz_shortopts == NULL )
    {
        free( p_longopts );
        return -1;
    }

    /* If we are requested to ignore errors, then we must work on a copy
     * of the ppsz_argv array, otherwise getopt_long will reorder it for
     * us, ignoring the arity of the options */
    if( b_ignore_errors )
    {
        argv_copy = vlc_alloc( i_argc, sizeof(char *) );
        if( argv_copy == NULL )
        {
            free( psz_shortopts );
            free( p_longopts );
            return -1;
        }
        memcpy( argv_copy, ppsz_argv, i_argc * sizeof(char *) );
        ppsz_argv = argv_copy;
    }

    memset(&pp_shortopts, 0, 256 * sizeof(pp_shortopts[0]));

    /* Indicate that we want to know the difference between unknown opt and
       missing data issues */
    psz_shortopts[0] = ':';
    i_shortopts = 1;

    /* Fill the p_longopts and psz_shortopts structures */
    i_index = 0;
    for (const vlc_plugin_t *p = vlc_plugins; p != NULL; p = p->next)
    {
        for (const module_config_item_t *p_item = p->conf.items,
                                   *p_end = p_item + p->conf.size;
             p_item < p_end;
             p_item++)
        {
            /* Ignore hints */
            if( !CONFIG_ITEM(p_item->i_type) )
                continue;

            /* Add item to long options */
            p_longopts[i_index].name = strdup( p_item->psz_name );
            if( p_longopts[i_index].name == NULL ) continue;
            p_longopts[i_index].flag = &flag;
            p_longopts[i_index].val = 0;
            p_longopts[i_index].is_obsolete = p_item->b_removed;

            if( CONFIG_CLASS(p_item->i_type) == CONFIG_ITEM_CLASS_INFO )
                p_longopts[i_index].has_arg = false;
            else if( CONFIG_CLASS(p_item->i_type) != CONFIG_ITEM_CLASS_BOOL )
                p_longopts[i_index].has_arg = true;
            else
            /* Booleans also need --no-foo and --nofoo options */
            {
                char *psz_name;

                p_longopts[i_index].has_arg = false;
                i_index++;

                if( asprintf( &psz_name, "no%s", p_item->psz_name ) == -1 )
                    continue;
                p_longopts[i_index].name = psz_name;
                p_longopts[i_index].has_arg = false;
                p_longopts[i_index].is_obsolete = true; /* this form is now obsolete */
                p_longopts[i_index].flag = &flag;
                p_longopts[i_index].val = 1;
                i_index++;

                if( asprintf( &psz_name, "no-%s", p_item->psz_name ) == -1 )
                    continue;
                p_longopts[i_index].name = psz_name;
                p_longopts[i_index].has_arg = false;
                p_longopts[i_index].is_obsolete = p_item->b_removed;
                p_longopts[i_index].flag = &flag;
                p_longopts[i_index].val = 1;
            }
            i_index++;

            /* If item also has a short option, add it */
            if( p_item->i_short )
            {
                pp_shortopts[(int)p_item->i_short] = p_item;
                psz_shortopts[i_shortopts] = p_item->i_short;
                i_shortopts++;
                if( CONFIG_CLASS(p_item->i_type) != CONFIG_ITEM_CLASS_BOOL
                 && CONFIG_CLASS(p_item->i_type) != CONFIG_ITEM_CLASS_INFO
                 && p_item->i_short != 'v' )
                {
                    psz_shortopts[i_shortopts] = ':';
                    i_shortopts++;
                }
            }
        }
    }

    /* Close the longopts and shortopts structures */
    memset( &p_longopts[i_index], 0, sizeof(*p_longopts) );
    psz_shortopts[i_shortopts] = '\0';

    int ret = -1;
    bool color = false;
#ifndef _WIN32
    color = (isatty(STDERR_FILENO));
#endif

    /*
     * Parse the command line options
     */
    vlc_getopt_t state;
    state.ind = 0 ; /* set to 0 to tell GNU getopt to reinitialize */
    while( ( i_cmd = vlc_getopt_long( i_argc, (char **)ppsz_argv,
                                      psz_shortopts,
                                      p_longopts, &i_index, &state ) ) != -1 )
    {
        /* A long option has been recognized */
        if( i_cmd == 0 )
        {
            module_config_item_t *p_conf;
            const char *psz_full_name = p_longopts[i_index].name;
            const char *psz_name = psz_full_name;

            bool old_style_neg_bool = flag && psz_name[2] != '-';

            /* Check if we deal with a --nofoo or --no-foo long option */
            if( flag ) psz_name += old_style_neg_bool ? 2 : 3;

            /* Store the configuration option */
            p_conf = config_FindConfig( psz_name );
            if( p_conf )
            {
                /* Warn deprecation of --nofoo style (allow them to still work however) */
                if (old_style_neg_bool)
                {
                    if (!printed_obs_neg_bool_warn)
                    {
                        fprintf(stderr, color ?
                                        YELLOW "%s:" TS_RESET " %s\n" :
                                               "%s:"          " %s\n",
                                _( "Warning" ),
                                _( "Negative boolean flags of the form "
                                   "`--nofoo' are now obsolete and will not "
                                   "be supported in future. Use only the "
                                   "`--no-foo' form now." ));
                        printed_obs_neg_bool_warn = true;
                    }
                    /* don't bother printing this if obsolete anyway */
                    if( !p_conf->b_removed )
                    {
                        fprintf(stderr, color ?
                                        YELLOW "%s:" TS_RESET " --no%s %s --no-%s.\n" :
                                               "%s:"          " --no%s %s --no-%s.\n",
                                _( "Warning" ), psz_name,
                                _( "is to become obsolete, in future use" ),
                                psz_name);
                    }
                }

                /* Check if the option is deprecated */
                if( p_conf->b_removed )
                {
                    fprintf(stderr, color ?
                                    YELLOW "%s:" TS_RESET " %s --%s %s.\n" :
                                           "%s:"          " %s --%s %s.\n",
                            _( "Warning" ), _( "option" ), psz_full_name,
                            _( "no longer exists" ));
                    continue;
                }

                switch( CONFIG_CLASS(p_conf->i_type) )
                {
                    case CONFIG_ITEM_CLASS_STRING:
                        var_Create( p_this, psz_name, VLC_VAR_STRING );
                        var_SetString( p_this, psz_name, state.arg );
                        break;
                    case CONFIG_ITEM_CLASS_INTEGER:
                        var_Create( p_this, psz_name, VLC_VAR_INTEGER );
                        var_Change( p_this, psz_name, VLC_VAR_SETMINMAX,
                            (vlc_value_t){ .i_int = p_conf->min.i },
                            (vlc_value_t){ .i_int = p_conf->max.i } );
                        var_SetInteger( p_this, psz_name,
                                        strtoll(state.arg, NULL, 0));
                        break;
                    case CONFIG_ITEM_CLASS_FLOAT:
                        var_Create( p_this, psz_name, VLC_VAR_FLOAT );
                        var_Change( p_this, psz_name, VLC_VAR_SETMINMAX,
                            (vlc_value_t){ .f_float = p_conf->min.f },
                            (vlc_value_t){ .f_float = p_conf->max.f } );
                        var_SetFloat( p_this, psz_name, us_atof(state.arg) );
                        break;
                    case CONFIG_ITEM_CLASS_BOOL:
                        var_Create( p_this, psz_name, VLC_VAR_BOOL );
                        var_SetBool( p_this, psz_name, !flag );
                        break;
                    case CONFIG_ITEM_CLASS_INFO:
                        var_Create( p_this, psz_name, VLC_VAR_BOOL );
                        var_SetBool( p_this, psz_name, true );
                        break;
                }
                continue;
            }
        }

        /* A short option has been recognized */
        if( i_cmd != '?' && i_cmd != ':' && pp_shortopts[i_cmd] != NULL )
        {
            const char *name = pp_shortopts[i_cmd]->psz_name;
            switch( CONFIG_CLASS(pp_shortopts[i_cmd]->i_type) )
            {
                case CONFIG_ITEM_CLASS_STRING:
                    var_Create( p_this, name, VLC_VAR_STRING );
                    var_SetString( p_this, name, state.arg );
                    break;
                case CONFIG_ITEM_CLASS_INTEGER:
                    var_Create( p_this, name, VLC_VAR_INTEGER );
                    if( i_cmd == 'v' )
                    {
                        i_verbose++; /* -v */
                        var_SetInteger( p_this, name, i_verbose );
                    }
                    else
                    {
                        var_SetInteger( p_this, name,
                                        strtoll(state.arg, NULL, 0) );
                    }
                    break;
                /* none currently?
                case CONFIG_ITEM_CLASS_FLOAT:
                    var_Create( p_this, name, VLC_VAR_FLOAT );
                    var_SetFloat( p_this, name, us_atof(state.arg) );
                    break;*/
                case CONFIG_ITEM_CLASS_BOOL:
                case CONFIG_ITEM_CLASS_INFO:
                    var_Create( p_this, name, VLC_VAR_BOOL );
                    var_SetBool( p_this, name, true );
                    break;
            }

            continue;
        }

        /* Internal error: unknown option */
        if( !b_ignore_errors )
        {
            if (i_cmd == ':')
                fprintf(stderr, color ?
                                RED "%s:" TS_RESET " %s " :
                                    "%s:"          " %s ",
                        _( "Error" ), _( "missing mandatory data value for" ));
            else
                fprintf(stderr, color ?
                                RED "%s:" TS_RESET " %s " :
                                    "%s:"          " %s ",
                        _( "Error" ), _( "unknown option" ));

            if( state.opt )
                fprintf( stderr, "`-%c'\n", state.opt );
            else if ( i_cmd == ':' )
                fprintf( stderr, "`%s'\n", ppsz_argv[state.ind-1] );
            else
            {
                fprintf( stderr, "`%s'", ppsz_argv[state.ind-1] );

                /* suggestion matching */
                double jw_filter = 0.8, best_metric = jw_filter, metric;
                const char *best = NULL;
                const char *jw_a = ppsz_argv[state.ind-1] + 2;
                for (size_t i = 0; i < (size_t)i_opts; i++) {
                    if (p_longopts[i].is_obsolete)
                        continue;
                    const char *jw_b = p_longopts[i].name;
                    if (jaro_winkler(jw_a, jw_b, &metric) == 0) { //ignore malloc failed calls
                        if (metric > best_metric || (!best && metric >= jw_filter)) {
                            best = jw_b;
                            best_metric = metric;
                        }
                    }
                }
                if (best) {
                    fprintf( stderr, "; %s `--%s'?\n", _( "did you mean" ), best );
                }
                else
                    fprintf( stderr, "\n" );
            }
            fprintf( stderr, "%s\n", _( "Try `vlc --help' for more information." ) );
            goto out;
        }
    }

    ret = 0;
    if( pindex != NULL )
        *pindex = state.ind;
out:
    /* Free allocated resources */
    for( i_index = 0; p_longopts[i_index].name; i_index++ )
        free( (char *)p_longopts[i_index].name );
    free( p_longopts );
    free( psz_shortopts );
    free( argv_copy );
    return ret;
}

