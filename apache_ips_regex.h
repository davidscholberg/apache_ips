/*
    Copyright 2013 David Scholberg <recombinant.vector@gmail.com>

    This file is part of apache_ips.

    apache_ips is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    apache_ips is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with apache_ips.  If not, see <http://www.gnu.org/licenses/>.
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

