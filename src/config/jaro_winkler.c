/*****************************************************************************
 * jaro_winkler.c: jaro winkler string similarity algorithm implementation
 *****************************************************************************
 * Copyright 2015 Danny Guo
 * Copyright 2018 Lyndon Brown
 *
 * Authors: Danny Guo <dguo@users.noreply.github.com>
 * Authors: Lyndon Brown <jnqnfe@gmail.com>
 *
 * Licensed under the MIT license. You may not copy, modify, or distribute this
 * file except in compliance with said license. You can find a copy of this
 * license either in the LICENSE file, or alternatively at
 * <http://opensource.org/licenses/MIT>.
 *****************************************************************************
 * This file is based upon the Jaro Winkler implementation of the `strsim`
 * Rust crate, authored by Danny Guo, available at
 * <https://github.com/dguo/strsim-rs>; more specifically the (as yet unmerged)
 * optimised copy authored by myself (Lyndon Brown), available at
 * <https://github.com/dguo/strsim-rs/pull/31>. The code is available under the
 * MIT license.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#ifdef JW_TEST
# include <stdio.h>
# include <math.h> /* fabs() */
#endif

#include "vlc_jaro_winkler.h"

#define MAX(a, b) ( ((a) > (b)) ? (a) : (b) )
#define MIN(a, b) ( ((a) < (b)) ? (a) : (b) )

/**
 * Checks both strings for a common prefix, returning the number of matching
 * bytes.
 */
static inline size_t split_on_common_prefix(const char *a, const char *b) {
    size_t len = 0;
    while (*(a) && *(b) && *(a++) == *(b++)) len++;
    return len;
}

/**
 * This is the inner Jaro algorithm, with a parameter for passing back the
 * length of the prefix common to both strings, used for efficiency of the
 * Jaro-Winkler implementation.
 */
static inline int jaro_inner(const char *a, const char *b, size_t *ret_prefix_cc, double* res) {
    assert(a && b && ret_prefix_cc && res);

    if ((a[0] == '\0') ^ (b[0] == '\0')) {
        *res = 0.0;
        return 0.0;
    }

    size_t prefix_char_count = split_on_common_prefix(a, b);
    const char *a_suffix = a + prefix_char_count;
    const char *b_suffix = b + prefix_char_count;

    if (a_suffix[0] == '\0' && b_suffix[0] == '\0') {
        *res = 1.0;
        return 0;
    }

    *ret_prefix_cc = prefix_char_count;

    size_t a_numchars = strlen(a_suffix) + prefix_char_count;
    size_t b_numchars = strlen(b_suffix) + prefix_char_count;

    // The check for lengths of one here is to prevent integer overflow when
    // calculating the search range.
    if (a_numchars == 1 && b_numchars == 1) {
        *res = 0.0;
        return 0;
    }

    size_t search_range = (MAX(a_numchars, b_numchars) / 2) - 1;

    /* catch overflow */
    assert(a_numchars <= INT_MAX);
    assert(search_range <= INT_MAX);

    bool *b_consumed = calloc(b_numchars, sizeof(*b_consumed));
    if (!b_consumed) {
        *res = 0.0;
        return -1;
    }

    size_t matches = prefix_char_count;
    size_t transpositions = 0;
    size_t b_match_index = 0;

    const char *a_char = a_suffix;
    for (size_t i = 0; *a_char; i++) {
        ssize_t tmp = (ssize_t)i - (ssize_t)search_range;
        size_t bound_start = (tmp >= 0) ? tmp : 0;
        size_t bound_end = MIN(b_numchars, i + search_range + 1);

        if (bound_start >= bound_end) {
            a_char++;
            continue;
        }

        const char *b_char = b_suffix + bound_start;
        for (size_t j = bound_start; *b_char && j < bound_end; j++) {
            if (*a_char == *b_char && !b_consumed[j]) {
                b_consumed[j] = true;
                matches++;

                if (j < b_match_index) {
                    transpositions++;
                }
                b_match_index = j;

                break;
            }
            b_char++;
        }
        a_char++;
    }

    if (matches == 0) {
        *res = 0.0;
        return 0;
    }

    *res = (1.0 / 3.0) *
           (((double)matches / (double)a_numchars) +
            ((double)matches / (double)b_numchars) +
            (((double)matches - (double)transpositions) / (double)matches));
    return 0;
}

/**
 * Calculate a “Jaro Winkler” metric.
 *
 * Algorithm: <http://en.wikipedia.org/wiki/Jaro%E2%80%93Winkler_distance>
 *
 * Like “Jaro” but gives a boost to strings that have a common prefix.
 *
 * \note: This implementation does not place a limit the common prefix length
 * adjusted for.
 *
 * \param a string A
 * \param b string B
 * \param res [OUT] a pointer to a double to receive the result
 * \return -1 on memory allocation failure, otherwise 0
 */
int jaro_winkler(const char *a, const char *b, double* res) {
    size_t prefix_char_count = 0;
    double jaro_distance;
    if (jaro_inner(a, b, &prefix_char_count, &jaro_distance) != 0) {
        return -1;
    }

    double jaro_winkler_distance =
        jaro_distance + (0.1 * (double)prefix_char_count * (1.0 - jaro_distance));

    *res = (jaro_winkler_distance <= 1.0) ? jaro_winkler_distance : 1.0;
    return 0;
}

#ifdef JW_TEST

# define test1( expected, a, b ) \
    assert(jaro_winkler(a, b, &actual) == 0); \
    failed = (actual != expected); \
    problems |= failed; \
    printf("[TEST] expected: %f, actual: %f, accuracy: n/a, result: %s, (a: %s), (b: %s)\n", \
        expected, actual, (failed) ? "FAIL" : "pass", a, b);

# define test2( expected, a, b, accuracy ) \
    assert(jaro_winkler(a, b, &actual) == 0); \
    failed = (fabs(expected - actual) >= accuracy); \
    problems |= failed; \
    printf("[TEST] expected: %f, actual: %f, accuracy: %f, result: %s, (a: %s), (b: %s)\n", \
        expected, actual, accuracy, (failed) ? "FAIL": "pass", a, b);

int main( void )
{
    bool problems = false, failed = false;
    double actual;

    // both_empty
    test1(1.0, "", "");

    // first_empty
    test1(0.0, "", "jaro-winkler");

    // second_empty
    test1(0.0, "distance", "");

    // same
    test1(1.0, "Jaro-Winkler", "Jaro-Winkler");

    // diff_short
    test2(0.813, "dixon", "dicksonx", 0.001);
    test2(0.813, "dicksonx", "dixon", 0.001);

    // same_one_character
    test1(1.0, "a", "a");

    // diff_one_character
    test1(0.0, "a", "b");

    // diff_no_transposition
    test2(0.840, "dwayne", "duane", 0.001);

    // diff_with_transposition
    test2(0.961, "martha", "marhta", 0.001);

    // names
    test2(0.562, "Friedrich Nietzsche", "Fran-Paul Sartre", 0.001);

    // long_prefix
    test2(0.911, "cheeseburger", "cheese fries", 0.001);

    // more_names
    test2(0.868, "Thorkel", "Thorgier", 0.001);

    // length_of_one
    test2(0.738, "Dinsdale", "D", 0.001);

    // very_long_prefix
    test2(1.0, "thequickbrownfoxjumpedoverx", "thequickbrownfoxjumpedovery", 0.001);

    return (problems) ? -1 : 0;
}
#endif
