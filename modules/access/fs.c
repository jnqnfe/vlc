/*****************************************************************************
 * fs.c: file system access plugin
 *****************************************************************************
 * Copyright (C) 2001-2006 VLC authors and VideoLAN
 * Copyright © 2006-2007 Rémi Denis-Courmont
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
 *          Rémi Denis-Courmont <rem # videolan # org>
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
#include "fs.h"
#include <vlc_plugin.h>

vlc_plugin_begin ()
    set_help( N_("Simple file input") )
    set_description( N_("File") )
    set_shortname( N_("File") )
    add_shortcut( "file", "fd", "stream" )
    set_capability( VLC_CAP_ACCESS, 50, FileOpen, FileClose )

    add_submodule()
#ifndef HAVE_FDOPENDIR
    add_shortcut( "file", "directory", "dir" )
#else
    add_shortcut( "directory", "dir" )
#endif
    set_capability( VLC_CAP_ACCESS, 55, DirOpen, DirClose )

    set_subcategory( SUBCAT_INPUT_ACCESS )
    set_section( N_("Directory" ), NULL )
    add_bool("list-special-files", false, N_("List special files"),
             N_("Include devices and pipes when listing directories"), true)
    add_obsolete_string("directory-sort") /* since 3.0.0 */
vlc_plugin_end ()
