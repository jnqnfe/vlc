/*****************************************************************************
 * projectm.cpp: visualization module based on libprojectM
 *****************************************************************************
 * Copyright © 2009-2011 VLC authors and VideoLAN
 *
 * Authors: Rémi Duraffort <ivoire@videolan.org>
 *          Laurent Aimar
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
#include <vlc_aout.h>
#include <vlc_vout_window.h>
#include <vlc_opengl.h>
#include <vlc_filter.h>
#include <vlc_rand.h>

#include <libprojectM/projectM.hpp>

#ifndef _WIN32
# include <locale.h>
#endif
#ifdef HAVE_XLOCALE_H
# include <xlocale.h>
#endif
#include <limits.h>

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
static int  Open         ( filter_t * );
static void Close        ( filter_t * );

#define CONFIG_TEXT N_("projectM configuration file")
#define CONFIG_LONGTEXT N_("File that will be used to configure the projectM " \
                           "module.")

#define PRESET_PATH_TXT N_("projectM preset path")
#define PRESET_PATH_LONGTXT N_("Path to the projectM preset directory")

#define TITLE_FONT_TXT N_("Title font")
#define MENU_FONT_TXT N_("Menu font")

#define WIDTH_TEXT N_("Video width (pixels)")
#define HEIGHT_TEXT N_("Video height (pixels)")

#define MESHX_TEXT N_("Mesh width (pixels)")
#define MESHY_TEXT N_("Mesh height (pixels)")

#define TEXTURE_TEXT N_("Texture size (pixels)")

#ifdef _WIN32
# define FONT_PATH      "C:\\WINDOWS\\Fonts\\arial.ttf"
# define FONT_PATH_MENU "C:\\WINDOWS\\Fonts\\arial.ttf"
# define PRESET_PATH    NULL
#else
# define FONT_PATH      "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSans.ttf"
# define FONT_PATH_MENU "/usr/share/fonts/truetype/ttf-dejavu/DejaVuSansMono.ttf"
# define PRESET_PATH    "/usr/share/projectM/presets"
#endif

#ifdef DEFAULT_FONT_FILE
#undef FONT_PATH
#define FONT_PATH DEFAULT_FONT_FILE
#endif

#ifdef DEFAULT_MONOSPACE_FONT_FILE
#undef FONT_PATH_MENU
#define FONT_PATH_MENU DEFAULT_MONOSPACE_FONT_FILE
#endif

vlc_plugin_begin ()
    set_shortname( N_("projectM"))
    set_description( N_("libprojectM effect") )
    add_shortcut( "projectm" )
    set_capability( VLC_CAP_VISUALIZATION, 0, Open, Close )

    set_subcategory( SUBCAT_AUDIO_VISUAL )
#ifndef HAVE_PROJECTM2
    add_loadfile("projectm-config", "/usr/share/projectM/config.inp",
                 CONFIG_TEXT, CONFIG_LONGTEXT)
#else
    add_directory("projectm-preset-path", PRESET_PATH,
                  PRESET_PATH_TXT, PRESET_PATH_LONGTXT)
    add_loadfile("projectm-title-font", FONT_PATH,
                 TITLE_FONT_TXT, NULL)
    add_loadfile("projectm-menu-font", FONT_PATH_MENU,
                 MENU_FONT_TXT, NULL)
#endif
    add_integer_with_range( "projectm-width", 800, 0, INT_MAX, WIDTH_TEXT, NULL, false )
    add_integer_with_range( "projectm-height", 500, 0, INT_MAX, HEIGHT_TEXT, NULL, false )
    add_integer_with_range( "projectm-meshx", 32, 0, INT_MAX, MESHX_TEXT, NULL, false )
    add_integer_with_range( "projectm-meshy", 24, 0, INT_MAX, MESHY_TEXT, NULL, false )
    add_integer_with_range( "projectm-texture-size", 1024, 0, INT_MAX, TEXTURE_TEXT, NULL, false )
vlc_plugin_end ()


/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
namespace {

struct filter_sys_t
{
    /* */
    vlc_thread_t thread;

    /* Opengl */
    vlc_gl_t  *gl;

    /* audio info */
    int i_channels;

    /* */
    vlc_mutex_t lock;
    bool  b_quit;
    float *p_buffer;
    unsigned i_buffer_size;
    unsigned i_nb_samples;
};

} // namespace

static block_t *DoWork( filter_t *, block_t * );
static void *Thread( void * );

/**
 * Open the module
 * @param p_filter: the filter object
 * @return VLC_SUCCESS or vlc error codes
 */
static int Open( filter_t *p_filter )
{
    filter_sys_t *p_sys;

    p_filter->p_sys = p_sys = (filter_sys_t*)malloc( sizeof( *p_sys ) );
    if( !p_sys )
        return VLC_ENOMEM;

    /* Create the object for the thread */
    p_sys->b_quit        = false;
    p_sys->i_channels    = aout_FormatNbChannels( &p_filter->fmt_in.audio );
    vlc_mutex_init( &p_sys->lock );
    p_sys->p_buffer      = NULL;
    p_sys->i_buffer_size = 0;
    p_sys->i_nb_samples  = 0;

    /* Create the OpenGL context */
    vout_window_cfg_t cfg;

    memset(&cfg, 0, sizeof (cfg));
    cfg.width = var_CreateGetInteger( p_filter, "projectm-width" );
    cfg.height = var_CreateGetInteger( p_filter, "projectm-height" );

    p_sys->gl = vlc_gl_surface_Create( VLC_OBJECT(p_filter), &cfg, NULL );
    if( p_sys->gl == NULL )
        goto error;

    /* Create the thread */
    if( vlc_clone( &p_sys->thread, Thread, p_filter,
                   VLC_THREAD_PRIORITY_LOW ) )
    {
        vlc_gl_surface_Destroy( p_sys->gl );
        goto error;
    }

    p_filter->fmt_in.audio.i_format = VLC_CODEC_FL32;
    p_filter->fmt_out.audio = p_filter->fmt_in.audio;
    p_filter->pf_audio_filter = DoWork;
    return VLC_SUCCESS;

error:
    vlc_mutex_destroy( &p_sys->lock );
    free (p_sys );
    return VLC_EGENERIC;
}


/**
 * Close the module
 * @param p_filter: the filter object
 */
static void Close( filter_t *p_filter )
{
    filter_sys_t *p_sys = reinterpret_cast<filter_sys_t *>( p_filter->p_sys );

    /* Stop the thread
     * XXX vlc_cleanup_push does not seems to work with C++ so no
     * vlc_cancel()... */
    vlc_mutex_lock( &p_sys->lock );
    p_sys->b_quit = true;
    vlc_mutex_unlock( &p_sys->lock );

    vlc_join( p_sys->thread, NULL );

    /* Free the ressources */
    vlc_gl_surface_Destroy( p_sys->gl );
    vlc_mutex_destroy( &p_sys->lock );
    free( p_sys->p_buffer );
    free( p_sys );
}


/**
 * Do the actual work with the new sample
 * @param p_aout: audio output object
 * @param p_filter: filter object
 * @param p_in_buf: input buffer
 * @param p_out_buf: output buffer
 */
static block_t *DoWork( filter_t *p_filter, block_t *p_in_buf )
{
    filter_sys_t *p_sys = reinterpret_cast<filter_sys_t *>( p_filter->p_sys );

    vlc_mutex_lock( &p_sys->lock );
    if( p_sys->i_buffer_size > 0 )
    {
        p_sys->i_nb_samples = __MIN( p_sys->i_buffer_size,
                                     p_in_buf->i_nb_samples );

        const float *p_src = (float*)p_in_buf->p_buffer;
        for( unsigned i = 0; i < p_sys->i_nb_samples; i++ )
        {
            float v = 0;
            for( int j = 0; j < p_sys->i_channels; j++ )
                v += p_src[p_sys->i_channels * i + j];
            p_sys->p_buffer[i] = v / p_sys->i_channels;
        }
    }
    vlc_mutex_unlock( &p_sys->lock );

    return p_in_buf;
}

/**
 * ProjectM update thread which do the rendering
 * @param p_this: the p_thread object
 */
static void *Thread( void *p_data )
{
    filter_t  *p_filter = (filter_t*)p_data;
    filter_sys_t *p_sys = reinterpret_cast<filter_sys_t *>( p_filter->p_sys );
    vlc_gl_t *gl = p_sys->gl;
    locale_t loc;
    locale_t oldloc;

    projectM *p_projectm;
#ifndef HAVE_PROJECTM2
    char *psz_config;
#else
    char *psz_preset_path;
    char *psz_title_font;
    char *psz_menu_font;
    projectM::Settings settings;
#endif

    if( vlc_gl_MakeCurrent( gl ) != VLC_SUCCESS )
    {
        msg_Err( p_filter, "Can't attach gl context" );
        return NULL;
    }

    /* Work-around the projectM locale bug */
    loc = newlocale (LC_NUMERIC_MASK, "C", NULL);
    oldloc = uselocale (loc);

    /* Create the projectM object */
#ifndef HAVE_PROJECTM2
    psz_config = var_InheritString( p_filter, "projectm-config" );
    p_projectm = new projectM( psz_config );
    free( psz_config );
#else
    psz_preset_path = var_InheritString( p_filter, "projectm-preset-path" );
#ifdef _WIN32
    if ( psz_preset_path == NULL )
        psz_preset_path = config_GetSysPath(VLC_PKG_DATA_DIR, "visualization");
#endif

    psz_title_font                = var_InheritString( p_filter, "projectm-title-font" );
    psz_menu_font                 = var_InheritString( p_filter, "projectm-menu-font" );

    settings.meshX                = var_InheritInteger( p_filter, "projectm-meshx" );
    settings.meshY                = var_InheritInteger( p_filter, "projectm-meshy" );
    settings.fps                  = 35;
    settings.textureSize          = var_InheritInteger( p_filter, "projectm-texture-size" );
    settings.windowWidth          = var_InheritInteger( p_filter, "projectm-width" );
    settings.windowHeight         = var_CreateGetInteger( p_filter, "projectm-height" );
    settings.presetURL            = psz_preset_path;
    settings.titleFontURL         = psz_title_font;
    settings.menuFontURL          = psz_menu_font;
    settings.smoothPresetDuration = 5;
    settings.presetDuration       = 30;
    settings.beatSensitivity      = 10;
    settings.aspectCorrection     = 1;
    settings.easterEgg            = 1;
    settings.shuffleEnabled       = 1;
    settings.softCutRatingsEnabled= false;

    p_projectm = new projectM( settings );

    free( psz_menu_font );
    free( psz_title_font );
    free( psz_preset_path );
#endif /* HAVE_PROJECTM2 */

    p_sys->i_buffer_size = p_projectm->pcm()->maxsamples;
    p_sys->p_buffer = (float*)calloc( p_sys->i_buffer_size,
                                      sizeof( float ) );

    /* Choose a preset randomly or projectM will always show the first one */
    if ( p_projectm->getPlaylistSize() > 0 )
        p_projectm->selectPreset( (unsigned)vlc_mrand48() % p_projectm->getPlaylistSize() );

    /* */
    for( ;; )
    {
        const vlc_tick_t i_deadline = vlc_tick_now() + VLC_TICK_FROM_MS(20); /* 50 fps max */

        /* Manage the events */
        unsigned width, height;
        bool quit;

        if( vlc_gl_surface_CheckSize( gl, &width, &height ) )
            p_projectm->projectM_resetGL( width, height );

        /* Render the image and swap the buffers */
        vlc_mutex_lock( &p_sys->lock );
        if( p_sys->i_nb_samples > 0 )
        {
            p_projectm->pcm()->addPCMfloat( p_sys->p_buffer,
                                            p_sys->i_nb_samples );
            p_sys->i_nb_samples = 0;
        }
        quit = p_sys->b_quit;
        vlc_mutex_unlock( &p_sys->lock );

        if( quit )
            break;

        p_projectm->renderFrame();

        /* */
        vlc_tick_wait( i_deadline );

        vlc_gl_Swap( gl );
    }

    delete p_projectm;

    if (loc != (locale_t)0)
    {
        uselocale (oldloc);
        freelocale (loc);
    }

    vlc_gl_ReleaseCurrent( gl );
    return NULL;
}
