/*****************************************************************************
 * configuration.h management of the modules configuration
 *****************************************************************************
 * Copyright (C) 2007 VLC authors and VideoLAN
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

#ifndef LIBVLC_CONFIGURATION_H
# define LIBVLC_CONFIGURATION_H 1

# ifdef __cplusplus
extern "C" {
# endif

/* Internal configuration prototypes and structures */

int  config_CreateDir( vlc_object_t *, const char * );
int  config_AutoSaveConfigFile( vlc_object_t * );

void config_Free (module_config_item_t *, size_t);

int config_LoadCmdLine   ( vlc_object_t *, int, const char *[], int * );
int config_LoadConfigFile( vlc_object_t * );
#define config_LoadCmdLine(a,b,c,d) config_LoadCmdLine(VLC_OBJECT(a),b,c,d)
#define config_LoadConfigFile(a) config_LoadConfigFile(VLC_OBJECT(a))
bool config_PrintHelp (vlc_object_t *);

int config_SortConfig (void);
void config_UnsortConfig (void);

#define CONFIG_CLASS(x) ((x) & CONFIG_ITEM_CLASS_MASK)

#define IsConfigStringType(type) \
    (CONFIG_CLASS(type) == CONFIG_ITEM_CLASS_STRING)
#define IsConfigIntegerType(type) \
    (CONFIG_CLASS(type) & \
     (CONFIG_ITEM_CLASS_INTEGER | CONFIG_ITEM_CLASS_BOOL))
#define IsConfigFloatType(type) \
    (CONFIG_CLASS(type) == CONFIG_ITEM_CLASS_FLOAT)

extern vlc_rwlock_t config_lock;
extern bool config_dirty;

bool config_IsSafe (const char *);

/**
 * Gets the arch-specific installation directory.
 *
 * This function determines the directory containing the architecture-specific
 * installed asset files (such as executable plugins and compiled byte code).
 *
 * @return a heap-allocated string (use free() to release it), or NULL on error
 */
char *config_GetLibDir(void) VLC_USED VLC_MALLOC;

/* The configuration file */
#define CONFIG_FILE                     "vlcrc"

# ifdef __cplusplus
}
# endif
#endif
