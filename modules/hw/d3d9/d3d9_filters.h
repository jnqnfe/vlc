/*****************************************************************************
 * d3d9_filters.h : D3D9 filters module callbacks
 *****************************************************************************
 * Copyright © 2017 VLC authors, VideoLAN and VideoLabs
 *
 * Authors: Steve Lhomme <robux4@gmail.com>
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

#ifndef VLC_D3D9_FILTERS_H
#define VLC_D3D9_FILTERS_H

#include <vlc_common.h>

#include "../../video_chroma/d3d9_fmt.h"

int  D3D9OpenDeinterlace(filter_t *);
void D3D9CloseDeinterlace(filter_t *);
int  D3D9OpenConverter(filter_t *);
void D3D9CloseConverter(filter_t *);
int  D3D9OpenCPUConverter(filter_t *);
void D3D9CloseCPUConverter(filter_t *);

void D3D9_FilterHoldInstance(filter_t *, d3d9_device_t *, D3DSURFACE_DESC *);
void D3D9_FilterReleaseInstance(d3d9_device_t *);

#endif /* VLC_D3D9_FILTERS_H */
