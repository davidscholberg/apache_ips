/*
David Scholberg
Master's Project
Fall 2012
*/

// adapted from pcredemo.c, which is provided by the PCRE developers

#include <stdio.h>
#include <string.h>
#include "apache_ips_main.h"
#include "apache_ips_regex.h"

const pcre *regexes[REGEX_NUM] = {
    NULL,
    NULL,
    NULL
};

const char *regex_strings[REGEX_NUM] = {
    "\r\n\r\n$",
    "HTTP\/[0-9]+?\.[0-9]+?[^0-9]",
    "Range:.+?=(.*?)\r",
    ",?[0-9]*?\-[0-9]*"
};


void compile_regexes() {
    const char *error;
    int erroffset;
    
    int i;
    for(i = 0; i < REGEX_NUM; i++) {
        regexes[i] = pcre_compile(
                        regex_strings[i],              /* the pattern */
                        0,                    /* default options */
                        &error,                  /* for error message */
                        &erroffset,           /* for error offset */
                        NULL);                /* use default character tables */

        /* Compilation failed: print the error message and exit */
        if (regexes[i] == NULL) {
            printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
            exit(1);
        }
    }
}

int match_regex(const pcre *regex,
                            const char *subject,
                            char **sub_expr,
                            int sub_expr_index) {
//return 1 regex matches subject,
// 0 otherwise
// place pointer to ovector[sub_expr_index] into *sub_expr if sub_expr_index>0
// make sure to free pointer returned
    int ovector[OVECCOUNT];
    int subject_length = (int) strlen(subject);
    int rc = pcre_exec(
                regex,                /* the compiled pattern */
                NULL,                 /* no extra data - we didn't study the pattern */
                subject,              /* the subject string */
                subject_length,       /* the length of the subject */
                0,                    /* start at offset 0 in the subject */
                0,                    /* default options */
                ovector,              /* output vector for substring information */
                OVECCOUNT);           /* number of elements in the output vector */

    /* Matching failed: handle error cases */

    if(rc < 0) {
        return 0;
    }

    /* Match succeded */
    /*
    printf("\nMatch succeeded at offset %d\n", ovector[0]);

    char *substring_start = subject + ovector[0];
    int substring_length = ovector[1] - ovector[0];
    printf("%2d: %.*s\n", 0, substring_length, substring_start);
    */
    if(sub_expr_index >= 0) {
        int sub_expr_size = ovector[(2*sub_expr_index)+1] - ovector[2*sub_expr_index];
        *sub_expr = c_stringify(subject + ovector[2*sub_expr_index], sub_expr_size);
    }
    
    return 1;
}

int match_regex_count(const pcre *regex, const char *subject) {
//return number of times regex matches subject, 0 if no match

    int ovector[OVECCOUNT];
    int subject_length = (int) strlen(subject);
    int rc = pcre_exec(
                regex,                /* the compiled pattern */
                NULL,                 /* no extra data - we didn't study the pattern */
                subject,              /* the subject string */
                subject_length,       /* the length of the subject */
                0,                    /* start at offset 0 in the subject */
                0,                    /* default options */
                ovector,              /* output vector for substring information */
                OVECCOUNT);           /* number of elements in the output vector */

    /* Matching failed: handle error cases */

    if(rc < 0) {
        return 0;
    }
    
    //printf("%.*s:", ovector[1] - ovector[0], subject + ovector[0]);
    
    int match_count = 1;
    for (;;){
        int options = 0;                 /* Normally no options */
        int start_offset = ovector[1];   /* Start at end of previous match */

        /* If the previous match was for an empty string, we are finished if we are
        at the end of the subject. Otherwise, arrange to run another match at the
        same point to see if a non-empty match can be found. */

        if (ovector[0] == ovector[1]) {
            if (ovector[0] == subject_length) break;
            options = PCRE_NOTEMPTY | PCRE_ANCHORED;
        }

        /* Run the next matching operation */

        rc = pcre_exec(
              regex,                   /* the compiled pattern */
              NULL,                 /* no extra data - we didn't study the pattern */
              subject,              /* the subject string */
              subject_length,       /* the length of the subject */
              start_offset,         /* starting offset in the subject */
              options,              /* options */
              ovector,              /* output vector for substring information */
              OVECCOUNT);           /* number of elements in the output vector */

        /* This time, a result of NOMATCH isn't an error. If the value in "options"
        is zero, it just means we have found all possible matches, so the loop ends.
        Otherwise, it means we have failed to find a non-empty-string match at a
        point where there was a previous empty-string match. In this case, we do what
        Perl does: advance the matching position by one, and continue. We do this by
        setting the "end of previous match" offset, because that is picked up at the
        top of the loop as the point at which to start again. */

        if (rc == PCRE_ERROR_NOMATCH) {
            if (options == 0) break;
            ovector[1] = start_offset + 1;
            continue;    /* Go round the loop again */
        }

        /* Other matching errors are not recoverable. */

        if (rc < 0) {
            break;
        }

        /* Match succeded */

        //printf("%.*s:", ovector[1] - ovector[0], subject + ovector[0]);

        match_count++;
    }

    //printf("\n");
    return match_count;
}

