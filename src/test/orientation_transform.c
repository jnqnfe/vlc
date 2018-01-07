/*****************************************************************************
 * orientation_transform.c: test video orientation transformation logic
 *****************************************************************************
 * Copyright (C) 2019 VLC authors and VideoLAN
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
# include <config.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <vlc_common.h>
#include <vlc_es.h>

#define MAX_OP_NAME_LEN 6
#define MAX_ORIENT_NAME_LEN 5

static const char* op_name(video_transform_t op)
{
    switch (op)
    {
        case TRANSFORM_NONE:       return "T_N";
        case TRANSFORM_HFLIP:      return "T_HF";
        case TRANSFORM_VFLIP:      return "T_VF";
        case TRANSFORM_R180:       return "T_180";
        case TRANSFORM_R270:       return "T_270";
        case TRANSFORM_R90:        return "T_90";
        case TRANSFORM_R90_HFLIP:  return "T_90H";
        case TRANSFORM_R270_HFLIP: return "T_270H";
    }
    return NULL;
}

static const char* orient_name(video_orientation_t orient)
{
    switch (orient)
    {
        case ORIENT_NORMAL:          return "O_N";
        case ORIENT_TRANSPOSED:      return "O_T";
        case ORIENT_ANTI_TRANSPOSED: return "O_AT";
        case ORIENT_HFLIPPED:        return "O_HF";
        case ORIENT_VFLIPPED:        return "O_VF";
        case ORIENT_ROTATED_180:     return "O_180";
        case ORIENT_ROTATED_270:     return "O_270";
        case ORIENT_ROTATED_90:      return "O_90";
    }
    return NULL;
}

typedef struct
{
    video_orientation_t orient_from;
    video_orientation_t orient_to;
    video_transform_t transform;
} mapping_t;

#define ENTRY(o1, o2, t) { o1, o2, t }

// Mappings between orientation pairs, with the applicable transform operation
static const mapping_t mappings[] =
{
    ENTRY( ORIENT_NORMAL,           ORIENT_NORMAL,          TRANSFORM_NONE ),
    ENTRY( ORIENT_NORMAL,           ORIENT_HFLIPPED,        TRANSFORM_HFLIP ),
    ENTRY( ORIENT_NORMAL,           ORIENT_ROTATED_180,     TRANSFORM_R180 ),
    ENTRY( ORIENT_NORMAL,           ORIENT_VFLIPPED,        TRANSFORM_VFLIP ),
    ENTRY( ORIENT_NORMAL,           ORIENT_ROTATED_90,      TRANSFORM_R90 ),
    ENTRY( ORIENT_NORMAL,           ORIENT_TRANSPOSED,      TRANSFORM_R90_HFLIP ),
    ENTRY( ORIENT_NORMAL,           ORIENT_ROTATED_270,     TRANSFORM_R270 ),
    ENTRY( ORIENT_NORMAL,           ORIENT_ANTI_TRANSPOSED, TRANSFORM_R270_HFLIP ),

    ENTRY( ORIENT_HFLIPPED,         ORIENT_NORMAL,          TRANSFORM_HFLIP ),
    ENTRY( ORIENT_HFLIPPED,         ORIENT_HFLIPPED,        TRANSFORM_NONE ),
    ENTRY( ORIENT_HFLIPPED,         ORIENT_ROTATED_180,     TRANSFORM_VFLIP ),
    ENTRY( ORIENT_HFLIPPED,         ORIENT_VFLIPPED,        TRANSFORM_R180 ),
    ENTRY( ORIENT_HFLIPPED,         ORIENT_ROTATED_90,      TRANSFORM_R270_HFLIP ),
    ENTRY( ORIENT_HFLIPPED,         ORIENT_TRANSPOSED,      TRANSFORM_R270 ),
    ENTRY( ORIENT_HFLIPPED,         ORIENT_ROTATED_270,     TRANSFORM_R90_HFLIP ),
    ENTRY( ORIENT_HFLIPPED,         ORIENT_ANTI_TRANSPOSED, TRANSFORM_R90 ),

    ENTRY( ORIENT_VFLIPPED,         ORIENT_NORMAL,          TRANSFORM_VFLIP ),
    ENTRY( ORIENT_VFLIPPED,         ORIENT_HFLIPPED,        TRANSFORM_R180 ),
    ENTRY( ORIENT_VFLIPPED,         ORIENT_ROTATED_180,     TRANSFORM_HFLIP ),
    ENTRY( ORIENT_VFLIPPED,         ORIENT_VFLIPPED,        TRANSFORM_NONE ),
    ENTRY( ORIENT_VFLIPPED,         ORIENT_ROTATED_90,      TRANSFORM_R90_HFLIP ),
    ENTRY( ORIENT_VFLIPPED,         ORIENT_TRANSPOSED,      TRANSFORM_R90 ),
    ENTRY( ORIENT_VFLIPPED,         ORIENT_ROTATED_270,     TRANSFORM_R270_HFLIP ),
    ENTRY( ORIENT_VFLIPPED,         ORIENT_ANTI_TRANSPOSED, TRANSFORM_R270 ),

    ENTRY( ORIENT_ROTATED_180,      ORIENT_NORMAL,          TRANSFORM_R180 ),
    ENTRY( ORIENT_ROTATED_180,      ORIENT_HFLIPPED,        TRANSFORM_VFLIP ),
    ENTRY( ORIENT_ROTATED_180,      ORIENT_ROTATED_180,     TRANSFORM_NONE ),
    ENTRY( ORIENT_ROTATED_180,      ORIENT_VFLIPPED,        TRANSFORM_HFLIP ),
    ENTRY( ORIENT_ROTATED_180,      ORIENT_ROTATED_90,      TRANSFORM_R270 ),
    ENTRY( ORIENT_ROTATED_180,      ORIENT_TRANSPOSED,      TRANSFORM_R270_HFLIP ),
    ENTRY( ORIENT_ROTATED_180,      ORIENT_ROTATED_270,     TRANSFORM_R90 ),
    ENTRY( ORIENT_ROTATED_180,      ORIENT_ANTI_TRANSPOSED, TRANSFORM_R90_HFLIP ),

    ENTRY( ORIENT_TRANSPOSED,       ORIENT_NORMAL,          TRANSFORM_R90_HFLIP ),
    ENTRY( ORIENT_TRANSPOSED,       ORIENT_HFLIPPED,        TRANSFORM_R90 ),
    ENTRY( ORIENT_TRANSPOSED,       ORIENT_ROTATED_180,     TRANSFORM_R270_HFLIP ),
    ENTRY( ORIENT_TRANSPOSED,       ORIENT_VFLIPPED,        TRANSFORM_R270 ),
    ENTRY( ORIENT_TRANSPOSED,       ORIENT_ROTATED_90,      TRANSFORM_HFLIP ),
    ENTRY( ORIENT_TRANSPOSED,       ORIENT_TRANSPOSED,      TRANSFORM_NONE ),
    ENTRY( ORIENT_TRANSPOSED,       ORIENT_ROTATED_270,     TRANSFORM_VFLIP ),
    ENTRY( ORIENT_TRANSPOSED,       ORIENT_ANTI_TRANSPOSED, TRANSFORM_R180 ),

    ENTRY( ORIENT_ROTATED_270,      ORIENT_NORMAL,          TRANSFORM_R90 ),
    ENTRY( ORIENT_ROTATED_270,      ORIENT_HFLIPPED,        TRANSFORM_R90_HFLIP ),
    ENTRY( ORIENT_ROTATED_270,      ORIENT_ROTATED_180,     TRANSFORM_R270 ),
    ENTRY( ORIENT_ROTATED_270,      ORIENT_VFLIPPED,        TRANSFORM_R270_HFLIP ),
    ENTRY( ORIENT_ROTATED_270,      ORIENT_ROTATED_90,      TRANSFORM_R180 ),
    ENTRY( ORIENT_ROTATED_270,      ORIENT_TRANSPOSED,      TRANSFORM_VFLIP ),
    ENTRY( ORIENT_ROTATED_270,      ORIENT_ROTATED_270,     TRANSFORM_NONE ),
    ENTRY( ORIENT_ROTATED_270,      ORIENT_ANTI_TRANSPOSED, TRANSFORM_HFLIP ),

    ENTRY( ORIENT_ROTATED_90,       ORIENT_NORMAL,          TRANSFORM_R270 ),
    ENTRY( ORIENT_ROTATED_90,       ORIENT_HFLIPPED,        TRANSFORM_R270_HFLIP ),
    ENTRY( ORIENT_ROTATED_90,       ORIENT_ROTATED_180,     TRANSFORM_R90 ),
    ENTRY( ORIENT_ROTATED_90,       ORIENT_VFLIPPED,        TRANSFORM_R90_HFLIP ),
    ENTRY( ORIENT_ROTATED_90,       ORIENT_ROTATED_90,      TRANSFORM_NONE ),
    ENTRY( ORIENT_ROTATED_90,       ORIENT_TRANSPOSED,      TRANSFORM_HFLIP ),
    ENTRY( ORIENT_ROTATED_90,       ORIENT_ROTATED_270,     TRANSFORM_R180 ),
    ENTRY( ORIENT_ROTATED_90,       ORIENT_ANTI_TRANSPOSED, TRANSFORM_VFLIP ),

    ENTRY( ORIENT_ANTI_TRANSPOSED,  ORIENT_NORMAL,          TRANSFORM_R270_HFLIP ),
    ENTRY( ORIENT_ANTI_TRANSPOSED,  ORIENT_HFLIPPED,        TRANSFORM_R270 ),
    ENTRY( ORIENT_ANTI_TRANSPOSED,  ORIENT_ROTATED_180,     TRANSFORM_R90_HFLIP ),
    ENTRY( ORIENT_ANTI_TRANSPOSED,  ORIENT_VFLIPPED,        TRANSFORM_R90 ),
    ENTRY( ORIENT_ANTI_TRANSPOSED,  ORIENT_ROTATED_90,      TRANSFORM_VFLIP ),
    ENTRY( ORIENT_ANTI_TRANSPOSED,  ORIENT_TRANSPOSED,      TRANSFORM_R180 ),
    ENTRY( ORIENT_ANTI_TRANSPOSED,  ORIENT_ROTATED_270,     TRANSFORM_HFLIP ),
    ENTRY( ORIENT_ANTI_TRANSPOSED,  ORIENT_ANTI_TRANSPOSED, TRANSFORM_NONE ),
};

static void test_mappings(bool* failed)
{
    int n = sizeof(mappings) / sizeof(mappings[0]);

    video_format_t fmt = {0};

    printf("───────── expected ──────┬───────────────────────── results ────────────────────────┐\n");
    printf("FROM    TO     TRANSFORM │ GetTransform   Transform      TransformBy    TransformTo │\n");
    printf("─────────────────────────┴──────────────────────────────────────────────────────────┘\n");

    for( int i = 0; i < n; i++ )
    {
        video_transform_t transform = video_format_GetTransform(
            mappings[i].orient_from, mappings[i].orient_to);

        video_orientation_t orientation = vlc_video_orient_Transform(
            mappings[i].orient_from, mappings[i].transform);

        fmt.orientation = mappings[i].orient_from;
        video_format_TransformBy(&fmt, mappings[i].transform);
        video_orientation_t new_orientation1 = fmt.orientation;

        fmt.orientation = mappings[i].orient_from;
        video_format_TransformTo(&fmt, mappings[i].orient_to);
        video_orientation_t new_orientation2 = fmt.orientation;

        if ( transform != mappings[i].transform
            || new_orientation1 != mappings[i].orient_to
            || new_orientation2 != mappings[i].orient_to )
        {
            *failed = true;
        }

        const char* name1 = orient_name(mappings[i].orient_from);
        const char* name2 = orient_name(mappings[i].orient_to);
        const char* name3 = op_name(mappings[i].transform);
        const char* name4 = op_name(transform);
        const char* name5 = orient_name(orientation);
        const char* name6 = orient_name(new_orientation1);
        const char* name7 = orient_name(new_orientation2);
        int pad1 = (int)(MAX_OP_NAME_LEN - strlen(name4));
        int pad2 = (int)(MAX_ORIENT_NAME_LEN - strlen(name5));
        int pad3 = (int)(MAX_ORIENT_NAME_LEN - strlen(name6));

        printf("%-6s  %-5s  %-10s  ", name1, name2, name3);

        if (transform != mappings[i].transform)
            printf("FAIL (%s)%*s  ", name4, pad1, "");
        else
            printf("pass           ");

        if (orientation != mappings[i].orient_to)
            printf("FAIL (%s)%*s   ", name5, pad2, "");
        else
            printf("pass           ");

        if (new_orientation1 != mappings[i].orient_to)
            printf("FAIL (%s)%*s   ", name6, pad3, "");
        else
            printf("pass           ");

        if (new_orientation2 != mappings[i].orient_to)
            printf("FAIL (%s)", name7);
        else
            printf("pass");

        printf("\n");
    }
}

typedef struct
{
    video_transform_t to_invert;
    video_transform_t inversion;
} inverted_t;

#define INV(t1, t2) { t1, t2 }

// Mappings between operation and its inverse (to undo)
static const inverted_t inverted_ops[] =
{
    INV( TRANSFORM_NONE,       TRANSFORM_NONE ),
    INV( TRANSFORM_HFLIP,      TRANSFORM_HFLIP ),
    INV( TRANSFORM_VFLIP,      TRANSFORM_VFLIP ),
    INV( TRANSFORM_R180,       TRANSFORM_R180 ),
    INV( TRANSFORM_R270,       TRANSFORM_R90 ), //<--deliberately different!
    INV( TRANSFORM_R90,        TRANSFORM_R270 ), //<--deliberately different!
    INV( TRANSFORM_R90_HFLIP,  TRANSFORM_R90_HFLIP ),
    INV( TRANSFORM_R270_HFLIP, TRANSFORM_R270_HFLIP ),
};

static void test_inversion(bool* failed)
{
    int n = sizeof(inverted_ops) / sizeof(inverted_ops[0]);
    for( int i = 0; i < n; i++ )
    {
        video_transform_t inversion = transform_Inverse(inverted_ops[i].to_invert);
        if( inversion != inverted_ops[i].inversion )
        {
            printf("Output: %-10sExpected: %s\n",
                   op_name(inversion), op_name(inverted_ops[i].inversion));
            *failed = true;
        }
    }
}

typedef struct
{
    int exif;
    video_orientation_t orientation;
} exif_map_t;

static const exif_map_t exif_maps[] =
{
    { 1, ORIENT_NORMAL },
    { 2, ORIENT_HFLIPPED },
    { 3, ORIENT_ROTATED_180 },
    { 4, ORIENT_VFLIPPED },
    { 5, ORIENT_TRANSPOSED },
    { 6, ORIENT_ROTATED_90 },
    { 7, ORIENT_ANTI_TRANSPOSED },
    { 8, ORIENT_ROTATED_270 },
};

static void test_exif_translation(bool* failed)
{
    int n = sizeof(inverted_ops) / sizeof(inverted_ops[0]);
    for( int i = 0; i < n; i++ )
    {
        int exif = ORIENT_TO_EXIF(exif_maps[i].orientation);
        video_orientation_t orientation = ORIENT_FROM_EXIF(exif_maps[i].exif);
        if( exif != exif_maps[i].exif )
        {
            printf("Output: %i\nExpected: %i\n",
                   exif, exif_maps[i].exif);
            *failed = true;
        }
        if( orientation != exif_maps[i].orientation )
        {
            printf("Output: %s\nExpected: %s\n",
                   orient_name(orientation), orient_name(exif_maps[i].orientation));
            *failed = true;
        }
    }
}

typedef struct
{
    video_orientation_t orientation;
    bool is_mirror;
    bool is_swap;
    video_orientation_t hflipped;
    video_orientation_t vflipped;
    video_orientation_t rotated180;
} util_mapping_t;

// Mappings between orientation and expected utility macro results
static const util_mapping_t util_mappings[] =
{
    { ORIENT_NORMAL,          false, false, ORIENT_HFLIPPED,        ORIENT_VFLIPPED,        ORIENT_ROTATED_180 },
    { ORIENT_HFLIPPED,        true,  false, ORIENT_NORMAL,          ORIENT_ROTATED_180,     ORIENT_VFLIPPED },
    { ORIENT_ROTATED_180,     false, false, ORIENT_VFLIPPED,        ORIENT_HFLIPPED,        ORIENT_NORMAL },
    { ORIENT_VFLIPPED,        true,  false, ORIENT_ROTATED_180,     ORIENT_NORMAL,          ORIENT_HFLIPPED },
    { ORIENT_ROTATED_90,      false, true,  ORIENT_ANTI_TRANSPOSED, ORIENT_TRANSPOSED,      ORIENT_ROTATED_270 },
    { ORIENT_TRANSPOSED,      true,  true,  ORIENT_ROTATED_270,     ORIENT_ROTATED_90,      ORIENT_ANTI_TRANSPOSED },
    { ORIENT_ROTATED_270,     false, true,  ORIENT_TRANSPOSED,      ORIENT_ANTI_TRANSPOSED, ORIENT_ROTATED_90 },
    { ORIENT_ANTI_TRANSPOSED, true,  true,  ORIENT_ROTATED_90,      ORIENT_ROTATED_270,     ORIENT_TRANSPOSED },
};

static void test_utils(bool* failed)
{
    int n = sizeof(util_mappings) / sizeof(util_mappings[0]);

    printf("Expected:\n");
    printf("Orient  Mirror?  SwapDim?  HFlip  VFlip  R180\n");
    printf("──────────────────────────────────────────────\n");

    for( int i = 0; i < n; i++ )
    {
        printf("%-6s  %s  %s  %-5s  %-5s  %-5s\n",
               orient_name(util_mappings[i].orientation),
               (util_mappings[i].is_mirror) ? "true   " : "false  ",
               (util_mappings[i].is_swap) ? "true    " : "false   ",
               orient_name(util_mappings[i].hflipped),
               orient_name(util_mappings[i].vflipped),
               orient_name(util_mappings[i].rotated180));
    }

    printf("\n\nResults:\n");
    printf("Orient  Mirror?  SwapDim?  HFlip         VFlip         R180\n");
    printf("───────────────────────────────────────────────────────────────────\n");

    for( int i = 0; i < n; i++ )
    {
        video_orientation_t orientation = util_mappings[i].orientation;
        bool is_mirror = ORIENT_IS_MIRROR(orientation);
        bool is_swap = ORIENT_IS_SWAP(orientation);
        video_orientation_t hflipped = ORIENT_HFLIP(orientation);
        video_orientation_t vflipped = ORIENT_VFLIP(orientation);
        video_orientation_t rotated180 = ORIENT_ROTATE_180(orientation);

        bool fail_is_mirror = is_mirror != util_mappings[i].is_mirror;
        bool fail_is_swap = is_swap != util_mappings[i].is_swap;
        bool fail_hflipped = hflipped != util_mappings[i].hflipped;
        bool fail_vflipped = vflipped != util_mappings[i].vflipped;
        bool fail_rotated180 = rotated180 != util_mappings[i].rotated180;

        if (fail_is_mirror || fail_is_swap || fail_hflipped || fail_vflipped || fail_rotated180)
            *failed = true;

        const char* name1 = orient_name(hflipped);
        const char* name2 = orient_name(vflipped);
        const char* name3 = orient_name(rotated180);
        int pad1 = (int)(MAX_ORIENT_NAME_LEN - strlen(name1));
        int pad2 = (int)(MAX_ORIENT_NAME_LEN - strlen(name2));

        printf("%-6s  %s     %s      ",
               orient_name(util_mappings[i].orientation),
               (fail_is_mirror) ? "FAIL" : "pass",
               (fail_is_swap) ? "FAIL" : "pass");

        if (fail_hflipped)
            printf("FAIL (%s)%*s  ", name1, pad1, "");
        else
            printf("pass          ");

        if (fail_vflipped)
            printf("FAIL (%s)%*s  ", name2, pad2, "");
        else
            printf("pass          ");

        if (fail_rotated180)
            printf("FAIL (%s)", name3);
        else
            printf("pass");

        printf("\n");
    }
}

int main( void )
{
    bool failed = false, failed_cur = false;

    /* Check orientation<->transform mapping
       These need to be so such that we can shortcut things for efficiency
       to grab the operation from "normal" orientation for a given orientation, */
    assert((int)TRANSFORM_NONE       == (int)ORIENT_NORMAL);
    assert((int)TRANSFORM_HFLIP      == (int)ORIENT_HFLIPPED);
    assert((int)TRANSFORM_R180       == (int)ORIENT_ROTATED_180);
    assert((int)TRANSFORM_R180_HFLIP == (int)ORIENT_VFLIPPED);
    assert((int)TRANSFORM_R90        == (int)ORIENT_ROTATED_90);
    assert((int)TRANSFORM_R90_HFLIP  == (int)ORIENT_TRANSPOSED);
    assert((int)TRANSFORM_R270       == (int)ORIENT_ROTATED_270);
    assert((int)TRANSFORM_R270_HFLIP == (int)ORIENT_ANTI_TRANSPOSED);

    /* Check orientation aliases */
    assert(ORIENT_NORMAL      == ORIENT_TOP_LEFT);
    assert(ORIENT_TRANSPOSED  == ORIENT_LEFT_TOP);
    assert(ORIENT_ANTI_TRANSPOSED == ORIENT_RIGHT_BOTTOM);
    assert(ORIENT_HFLIPPED    == ORIENT_TOP_RIGHT);
    assert(ORIENT_VFLIPPED    == ORIENT_BOTTOM_LEFT);
    assert(ORIENT_ROTATED_180 == ORIENT_BOTTOM_RIGHT);
    assert(ORIENT_ROTATED_270 == ORIENT_LEFT_BOTTOM);
    assert(ORIENT_ROTATED_90  == ORIENT_RIGHT_TOP);

    /* Check transform aliases */
    assert(TRANSFORM_IDENTITY       == TRANSFORM_NONE);
    assert(TRANSFORM_VFLIP          == TRANSFORM_R180_HFLIP);
    assert(TRANSFORM_TRANSPOSE      == TRANSFORM_R90_HFLIP);
    assert(TRANSFORM_ANTI_TRANSPOSE == TRANSFORM_R270_HFLIP);

    printf("\n=========================================\n");
    printf("VIDEO ORIENTATION TRANSFORM TEST RESULTS\n");
    printf("\n");

    test_mappings(&failed_cur);
    if (failed_cur)
        failed = true;

    printf("\n=========================================\n");
    printf("INVERSION TRANSFORM TEST RESULTS\n");
    printf("\n");

    failed_cur = false;
    test_inversion(&failed_cur);
    if (!failed_cur)
        printf("all good!\n");
    else
        failed = true;

    printf("\n=========================================\n");
    printf("EXIF TRANSLATION TEST RESULTS\n");
    printf("\n");

    failed_cur = false;
    test_exif_translation(&failed_cur);
    if (!failed_cur)
        printf("all good!\n");
    else
        failed = true;

    printf("\n=========================================\n");
    printf("UTILS TEST RESULTS\n");
    printf("\n");

    failed_cur = false;
    test_utils(&failed_cur);
    if (failed_cur)
        failed = true;

    printf("\n=========================================\n");

    return (failed) ? -1 : 0;
}
