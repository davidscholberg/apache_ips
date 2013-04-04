/*
David Scholberg
Master's Project
Fall 2012
*/

#ifndef APACHE_IPS_REGEX_H_
#define APACHE_IPS_REGEX_H_

#include <pcre.h>


#define REGEX_NUM 4
#define REGEX_FULL_HTTP_MSG 0
#define REGEX_HTTP 1
#define REGEX_RANGE_HEADER 2
#define REGEX_RANGE 3
#define OVECCOUNT 30    /* should be a multiple of 3 */


extern const pcre *regexes[REGEX_NUM];
extern const char *regex_strings[REGEX_NUM];


void compile_regexes();

int match_regex(
    const pcre *regex,
    const char *subject,
    char **sub_expr,
    int sub_expr_index
);

int match_regex_count(
    const pcre *regex,
    const char *subject
);

#endif // APACHE_IPS_REGEX_H_

