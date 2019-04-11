/*****************************************************************************
 * vlc_common.h: common definitions
 * Collection of useful common types and macros definitions
 *****************************************************************************
 * Copyright (C) 1998-2011 VLC authors and VideoLAN
 *
 * Authors: Samuel Hocevar <sam@via.ecp.fr>
 *          Vincent Seguin <seguin@via.ecp.fr>
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

#ifndef VLC_COMMON_H
# define VLC_COMMON_H 1

/**
 * \defgroup vlc VLC plug-in programming interface
 * \file
 * \ingroup vlc
 * This file is a collection of common definitions and types
 */

/*****************************************************************************
 * Required vlc headers
 *****************************************************************************/
#include "vlc_config.h"

/*****************************************************************************
 * Required system headers
 *****************************************************************************/
#include <stdlib.h>
#include <stdarg.h>

#include <string.h>
#include <stdio.h>
#include <inttypes.h>
#include <stddef.h>

#ifndef __cplusplus
# include <stdbool.h>
#endif

/**
 * \defgroup cext C programming language extensions
 * \ingroup vlc
 *
 * This section defines a number of macros and inline functions extending the
 * C language. Most extensions are implemented by GCC and LLVM/Clang, and have
 * unoptimized fallbacks for other C11/C++11 conforming compilers.
 * @{
 */
#ifdef __GNUC__
# define VLC_GCC_VERSION(maj,min) \
    ((__GNUC__ > (maj)) || (__GNUC__ == (maj) && __GNUC_MINOR__ >= (min)))
#else
/** GCC version check */
# define VLC_GCC_VERSION(maj,min) (0)
#endif

/* Try to fix format strings for all versions of mingw and mingw64 */
#if defined( _WIN32 ) && defined( __USE_MINGW_ANSI_STDIO )
 #undef PRId64
 #define PRId64 "lld"
 #undef PRIi64
 #define PRIi64 "lli"
 #undef PRIu64
 #define PRIu64 "llu"
 #undef PRIo64
 #define PRIo64 "llo"
 #undef PRIx64
 #define PRIx64 "llx"
 #define snprintf __mingw_snprintf
 #define vsnprintf __mingw_vsnprintf
 #define swprintf _snwprintf
#endif

/* Function attributes for compiler warnings */
#ifdef __GNUC__
# define VLC_DEPRECATED __attribute__((deprecated))
# if VLC_GCC_VERSION(6,0)
#  define VLC_DEPRECATED_ENUM __attribute__((deprecated))
# else
#  define VLC_DEPRECATED_ENUM
# endif

# if defined( _WIN32 ) && !defined( __clang__ )
#  define VLC_FORMAT(x,y) __attribute__ ((format(gnu_printf,x,y)))
# else
#  define VLC_FORMAT(x,y) __attribute__ ((format(printf,x,y)))
# endif
# define VLC_FORMAT_ARG(x) __attribute__ ((format_arg(x)))
# define VLC_MALLOC __attribute__ ((malloc))
# define VLC_USED __attribute__ ((warn_unused_result))

#else
/**
 * Deprecated functions or compound members annotation
 *
 * Use this macro in front of a function declaration or compound member
 * within a compound type declaration.
 * The compiler may emit a warning every time the function or member is used.
 *
 * Use \ref VLC_DEPRECATED_ENUM instead for enumeration members.
 */
# define VLC_DEPRECATED

/**
 * Deprecated enum member annotation
 *
 * Use this macro after an enumerated type member declaration.
 * The compiler may emit a warning every time the enumeration member is used.
 *
 * See also \ref VLC_DEPRECATED.
 */
# define VLC_DEPRECATED_ENUM

/**
 * String format function annotation
 *
 * Use this macro after a function prototype/declaration if the function
 * expects a standard C format string. This helps compiler diagnostics.
 *
 * @param x the position (starting from 1) of the format string argument
 * @param y the first position (also starting from 1) of the variable arguments
 *          following the format string (usually but not always \p x+1).
 */
# define VLC_FORMAT(x,y)

/**
 * Format string translation function annotation
 *
 * Use this macro after a function prototype/declaration if the function
 * expects a format string as input and returns another format string as output
 * to another function.
 *
 * This is primarily intended for localization functions such as gettext().
 */
# define VLC_FORMAT_ARG(x)

/**
 * Heap allocated result function annotation
 *
 * Use this macro to annotate a function that returns a pointer to memory that
 * cannot alias any other valid pointer.
 *
 * This is primarily used for functions that return a pointer to heap-allocated
 * memory, but it can be used for other applicable purposes.
 *
 * \warning Do not use this annotation if the returned pointer can in any way
 * alias a valid pointer at the time the function exits. This could lead to
 * very weird memory corruption bugs.
 */
# define VLC_MALLOC

/**
 * Used result function annotation
 *
 * Use this macro to annotate a function whose result must be used.
 *
 * There are several cases where this is useful:
 * - If a function has no side effects (or no useful side effects), such that
 *   the only useful purpose of calling said function is to obtain its
 *   return value.
 * - If ignoring the function return value would lead to a resource leak
 *   (including but not limited to a memory leak).
 * - If a function cannot be used correctly without checking its return value.
 *   For instance, if the function can fail at any time.
 *
 * The compiler may warn if the return value of a function call is ignored.
 */
# define VLC_USED
#endif

#if defined (__ELF__) || defined (__MACH__)
# define VLC_WEAK __attribute__((weak))
#else
/**
 * Weak symbol annotation
 *
 * Use this macro before an external identifier \b definition to mark it as a
 * weak symbol. A weak symbol can be overriden by another symbol of the same
 * name at the link time.
 */
# define VLC_WEAK
#endif

/* Branch prediction */
#if defined (__GNUC__) || defined (__clang__)
# define likely(p)     __builtin_expect(!!(p), 1)
# define unlikely(p)   __builtin_expect(!!(p), 0)
# define unreachable() __builtin_unreachable()
#else
/**
 * Predicted true condition
 *
 * This macro indicates that the condition is expected most often true.
 * The compiler may optimize the code assuming that this condition is usually
 * met.
 */
# define likely(p)     (!!(p))

/**
 * Predicted false condition
 *
 * This macro indicates that the condition is expected most often false.
 * The compiler may optimize the code assuming that this condition is rarely
 * met.
 */
# define unlikely(p)   (!!(p))

/**
 * Impossible branch
 *
 * This macro indicates that the branch cannot be reached at run-time, and
 * represents undefined behaviour.
 * The compiler may optimize the code assuming that the call flow will never
 * logically reach the point where this macro is expanded.
 *
 * See also \ref vlc_assert_unreachable.
 */
# define unreachable() ((void)0)
#endif

/**
 * Impossible branch assertion
 *
 * This macro asserts that the branch cannot be reached at run-time.
 *
 * If the branch is reached in a debug build, it will trigger an assertion
 * failure and abnormal program termination.
 *
 * If the branch is reached in a non-debug build, this macro is equivalent to
 * \ref unreachable and the behaviour is undefined.
 */
#define vlc_assert_unreachable() (vlc_assert(!"unreachable"), unreachable())

/**
 * Run-time assertion
 *
 * This macro performs a run-time assertion if C assertions are enabled
 * and the following preprocessor symbol is defined:
 * @verbatim __LIBVLC__ @endverbatim
 * That restriction ensures that assertions in public header files are not
 * unwittingly <i>leaked</i> to externally-compiled plug-ins
 * including those header files.
 *
 * Within the LibVLC code base, this is exactly the same as assert(), which can
 * and probably should be used directly instead.
 */
#ifdef __LIBVLC__
# define vlc_assert(pred) assert(pred)
#else
# define vlc_assert(pred) ((void)0)
#endif

/* Linkage */
#ifdef __cplusplus
# define VLC_EXTERN extern "C"
#else
# define VLC_EXTERN
#endif

#if defined (_WIN32) && defined (DLL_EXPORT)
# define VLC_EXPORT __declspec(dllexport)
#elif defined (__GNUC__)
# define VLC_EXPORT __attribute__((visibility("default")))
#else
# define VLC_EXPORT
#endif

/**
 * Exported API call annotation
 *
 * This macro is placed before a function declaration to indicate that the
 * function is an API call of the LibVLC plugin API.
 */
#define VLC_API VLC_EXTERN VLC_EXPORT

/** @} */

/*****************************************************************************
 * Basic types definitions
 *****************************************************************************/
/**
 * The vlc_fourcc_t type.
 *
 * See http://www.webartz.com/fourcc/ for a very detailed list.
 */
typedef uint32_t vlc_fourcc_t;

#ifdef WORDS_BIGENDIAN
#   define VLC_FOURCC( a, b, c, d ) \
        ( ((uint32_t)d) | ( ((uint32_t)c) << 8 ) \
           | ( ((uint32_t)b) << 16 ) | ( ((uint32_t)a) << 24 ) )
#   define VLC_TWOCC( a, b ) \
        ( (uint16_t)(b) | ( (uint16_t)(a) << 8 ) )

#else
#   define VLC_FOURCC( a, b, c, d ) \
        ( ((uint32_t)a) | ( ((uint32_t)b) << 8 ) \
           | ( ((uint32_t)c) << 16 ) | ( ((uint32_t)d) << 24 ) )
#   define VLC_TWOCC( a, b ) \
        ( (uint16_t)(a) | ( (uint16_t)(b) << 8 ) )

#endif

/**
 * Translate a vlc_fourcc into its string representation. This function
 * assumes there is enough room in psz_fourcc to store 4 characters in.
 *
 * \param fcc a vlc_fourcc_t
 * \param psz_fourcc string to store string representation of vlc_fourcc in
 */
static inline void vlc_fourcc_to_char( vlc_fourcc_t fcc, char *psz_fourcc )
{
    memcpy( psz_fourcc, &fcc, 4 );
}

/*****************************************************************************
 * Classes declaration
 *****************************************************************************/

/* Internal types */
typedef struct vlc_object_t vlc_object_t;
typedef struct libvlc_int_t libvlc_int_t;
typedef struct date_t date_t;

/* Playlist */

typedef struct playlist_t playlist_t;
typedef struct playlist_item_t playlist_item_t;
typedef struct services_discovery_t services_discovery_t;
typedef struct vlc_renderer_discovery_t vlc_renderer_discovery_t;
typedef struct vlc_renderer_item_t vlc_renderer_item_t;
typedef struct vlc_probe_t vlc_probe_t;
typedef struct vlc_playlist_export vlc_playlist_export;

/* Modules */
typedef struct module_t module_t;
typedef struct module_config_item_t module_config_item_t;

typedef struct config_category_t config_category_t;

/* Input */
typedef struct input_item_t input_item_t;
typedef struct input_item_node_t input_item_node_t;
typedef struct stream_t     stream_t;
typedef struct stream_t demux_t;
typedef struct es_out_t     es_out_t;
typedef struct es_out_id_t  es_out_id_t;
typedef struct seekpoint_t seekpoint_t;
typedef struct info_t info_t;
typedef struct info_category_t info_category_t;
typedef struct input_attachment_t input_attachment_t;

/* Format */
typedef struct audio_format_t audio_format_t;
typedef struct video_format_t video_format_t;
typedef struct subs_format_t subs_format_t;
typedef struct es_format_t es_format_t;
typedef struct video_palette_t video_palette_t;
typedef struct vlc_es_id_t vlc_es_id_t;

/* Audio */
typedef struct audio_output audio_output_t;
typedef audio_format_t audio_sample_format_t;

/* Video */
typedef struct vout_thread_t vout_thread_t;
typedef struct vlc_viewpoint_t vlc_viewpoint_t;

typedef video_format_t video_frame_format_t;
typedef struct picture_t picture_t;

/* Subpictures */
typedef struct spu_t spu_t;
typedef struct subpicture_t subpicture_t;
typedef struct subpicture_region_t subpicture_region_t;

typedef struct image_handler_t image_handler_t;

/* Stream output */
typedef struct sout_instance_t sout_instance_t;

typedef struct sout_input_t sout_input_t;
typedef struct sout_packetizer_input_t sout_packetizer_input_t;

typedef struct sout_access_out_t sout_access_out_t;

typedef struct sout_mux_t sout_mux_t;

typedef struct sout_stream_t    sout_stream_t;

typedef struct config_chain_t       config_chain_t;
typedef struct session_descriptor_t session_descriptor_t;

/* Decoders */
typedef struct decoder_t         decoder_t;
typedef struct decoder_synchro_t decoder_synchro_t;

/* Encoders */
typedef struct encoder_t      encoder_t;

/* Filters */
typedef struct filter_t filter_t;

/* Network */
typedef struct vlc_url_t vlc_url_t;

/* Misc */
typedef struct iso639_lang_t iso639_lang_t;

/* block */
typedef struct block_t      block_t;
typedef struct block_fifo_t block_fifo_t;

/* Hashing */
typedef struct md5_s md5_t;

/* XML */
typedef struct xml_t xml_t;
typedef struct xml_reader_t xml_reader_t;

/* vod server */
typedef struct vod_t     vod_t;
typedef struct vod_media_t vod_media_t;

/* VLM */
typedef struct vlm_t         vlm_t;
typedef struct vlm_message_t vlm_message_t;

/* misc */
typedef struct vlc_meta_t    vlc_meta_t;
typedef struct input_stats_t input_stats_t;
typedef struct addon_entry_t addon_entry_t;
typedef struct vlc_tls_server_t vlc_tls_server_t;
typedef struct vlc_tls_client_t vlc_tls_client_t;

/* Update */
typedef struct update_t update_t;

/**
 * VLC value structure
 */
typedef union
{
    int64_t         i_int;
    bool            b_bool;
    float           f_float;
    char *          psz_string;
    void *          p_address;
    struct { int32_t x; int32_t y; } coords;

} vlc_value_t;

/*****************************************************************************
 * Error values (shouldn't be exposed)
 *****************************************************************************/
/** No error */
#define VLC_SUCCESS        (-0)
/** Unspecified error */
#define VLC_EGENERIC       (-1)
/** Not enough memory */
#define VLC_ENOMEM         (-2)
/** Timeout */
#define VLC_ETIMEOUT       (-3)
/** Module not found */
#define VLC_ENOMOD         (-4)
/** Object not found */
#define VLC_ENOOBJ         (-5)
/** Variable not found */
#define VLC_ENOVAR         (-6)
/** Bad variable value */
#define VLC_EBADVAR        (-7)
/** Item not found */
#define VLC_ENOITEM        (-8)

/*****************************************************************************
 * Variable callbacks: called when the value is modified
 *****************************************************************************/
typedef int ( * vlc_callback_t ) ( vlc_object_t *,      /* variable's object */
                                   char const *,            /* variable name */
                                   vlc_value_t,                 /* old value */
                                   vlc_value_t,                 /* new value */
                                   void * );                /* callback data */

/*****************************************************************************
 * List callbacks: called when elements are added/removed from the list
 *****************************************************************************/
typedef int ( * vlc_list_callback_t ) ( vlc_object_t *,      /* variable's object */
                                        char const *,            /* variable name */
                                        int,                  /* VLC_VAR_* action */
                                        vlc_value_t *,      /* new/deleted value  */
                                        void *);                 /* callback data */

/*****************************************************************************
 * OS-specific headers and thread types
 *****************************************************************************/
#if defined( _WIN32 )
#   include <malloc.h>
#   ifndef PATH_MAX
#       define PATH_MAX MAX_PATH
#   endif
#   include <windows.h>
#endif

#ifdef __APPLE__
#include <sys/syslimits.h>
#include <AvailabilityMacros.h>
#endif

#ifdef __OS2__
#   define OS2EMX_PLAIN_CHAR
#   define INCL_BASE
#   define INCL_PM
#   include <os2safe.h>
#   include <os2.h>
#endif

#include "vlc_tick.h"
#include "vlc_threads.h"

/**
 * \defgroup intops Integer operations
 * \ingroup cext
 *
 * Common integer functions.
 * @{
 */
/* __MAX and __MIN: self explanatory */
#ifndef __MAX
#   define __MAX(a, b)   ( ((a) > (b)) ? (a) : (b) )
#endif
#ifndef __MIN
#   define __MIN(a, b)   ( ((a) < (b)) ? (a) : (b) )
#endif

/* clip v in [min, max] */
#define VLC_CLIP(v, min, max)    __MIN(__MAX((v), (min)), (max))

/**
 * \defgroup bitops Bit operations
 * @{
 */

/* Free and set set the variable to NULL */
#define FREENULL(a) do { free( a ); a = NULL; } while(0)

#define EMPTY_STR(str) (!str || !*str)

#include <vlc_arrays.h>

/* */
#define VLC_UNUSED(x) (void)(x)

/* Stuff defined in src/extras/libc.c */

#if defined(_WIN32)
/* several type definitions */
#   if defined( __MINGW32__ )
#       if !defined( _OFF_T_ )
            typedef long long _off_t;
            typedef _off_t off_t;
#           define _OFF_T_
#       else
#           ifdef off_t
#               undef off_t
#           endif
#           define off_t long long
#       endif
#   endif

#   ifndef O_NONBLOCK
#       define O_NONBLOCK 0
#   endif
#endif /* _WIN32 */

typedef struct {
    unsigned num, den;
} vlc_rational_t;

#define container_of(ptr, type, member) \
    ((type *)(((char *)(ptr)) - offsetof(type, member)))

/*****************************************************************************
 * I18n stuff
 *****************************************************************************/
VLC_API const char *vlc_gettext(const char *msgid) VLC_FORMAT_ARG(1);
VLC_API const char *vlc_ngettext(const char *s, const char *p, unsigned long n)
VLC_FORMAT_ARG(1) VLC_FORMAT_ARG(2);

#define vlc_pgettext( ctx, id ) \
        vlc_pgettext_aux( ctx "\004" id, id )

VLC_FORMAT_ARG(2)
static inline const char *vlc_pgettext_aux( const char *ctx, const char *id )
{
    const char *tr = vlc_gettext( ctx );
    return (tr == ctx) ? id : tr;
}

/*****************************************************************************
 * Loosy memory allocation functions. Do not use in new code.
 *****************************************************************************/
static inline void *xmalloc(size_t len)
{
    void *ptr = malloc(len);
    if (unlikely(ptr == NULL && len > 0))
        abort();
    return ptr;
}

static inline void *xrealloc(void *ptr, size_t len)
{
    void *nptr = realloc(ptr, len);
    if (unlikely(nptr == NULL && len > 0))
        abort();
    return nptr;
}

static inline char *xstrdup (const char *str)
{
    char *ptr = strdup (str);
    if (unlikely(ptr == NULL))
        abort ();
    return ptr;
}

/*****************************************************************************
 * libvlc features
 *****************************************************************************/
VLC_API const char * VLC_CompileBy( void ) VLC_USED;
VLC_API const char * VLC_CompileHost( void ) VLC_USED;
VLC_API const char * VLC_Compiler( void ) VLC_USED;

/*****************************************************************************
 * Additional vlc stuff
 *****************************************************************************/
#include "vlc_messages.h"
#include "vlc_objects.h"
#include "vlc_variables.h"
#include "vlc_configuration.h"
#include "vlc_util.h"

#if defined( _WIN32 ) || defined( __OS2__ )
#   define DIR_SEP_CHAR '\\'
#   define DIR_SEP "\\"
#   define PATH_SEP_CHAR ';'
#   define PATH_SEP ";"
#else
#   define DIR_SEP_CHAR '/'
#   define DIR_SEP "/"
#   define PATH_SEP_CHAR ':'
#   define PATH_SEP ":"
#endif

#define LICENSE_MSG \
  _("This program comes with NO WARRANTY, to the extent permitted by " \
    "law.\nYou may redistribute it under the terms of the GNU General " \
    "Public License;\nsee the file named COPYING for details.\n" \
    "Written by the VideoLAN team; see the AUTHORS file.\n")

#endif /* !VLC_COMMON_H */
