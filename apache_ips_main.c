/*
David Scholberg
Master's Project
Fall 2012
*/

/**********
 * INCLUDES
 **********/

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include "apache_ips_main.h"
#include "apache_ips_regex.h"
#include "reverse_proxy.h"

/*********
 * DEFINES
 *********/

#define BUFFERSIZE 1000000

/*********************
 * STATIC DECLARATIONS
 *********************/

static int process_client_data(
    const char *client_data,
    int data_size
);

/******
 * MAIN
 ******/

int main(int argc, char **argv) {
    // initialize regexes
    compile_regexes();

    // start reverse proxy (function does not return)
    reverse_proxy(process_client_data, NULL);
    //reverse_proxy(NULL, NULL);
    
    return 0;
}

/**********************
 * FUNCTION DEFINITIONS
 **********************/

char *c_stringify(
    const char *buffer,
    const int buffer_length) {

/*    
    takes char buffer and copies it to c-style string (null-terminated)
    be sure to call free on returned pointer
*/
    
    char *c_string = (char *) malloc(buffer_length + 1);
    memcpy(c_string, buffer, buffer_length);
    c_string[buffer_length] = '\0';
    return c_string;
}


static int process_client_data(
    const char *client_data,
    int data_size) {

/*
    This is the client_callback function for the reverse_proxy. It gets called
    every time the reverse proxy gets data from the client.

    This function examines client data for Range header section of an HTTP
    message. If we haven't received the entire HTTP header, return PROXY_BUFFER.
    Else if there's no range header or range header passes inspection, return
    PROXY_ALLOW. Else return PROXY_BLOCK
*/
    int verdict = PROXY_ALLOW;

    char *client_data_string = c_stringify(
        client_data,
        data_size
    );
    
    int match_status = match_regex(
        regexes[REGEX_FULL_HTTP_MSG],
        client_data_string,
        NULL,
        -1
    );
    
    // if client data is not a whole HTTP message, then tell proxy to buffer it
    if(match_status == 0) {
        verdict = PROXY_BUFFER;
    }
    else {
    
        match_status = match_regex(
            regexes[REGEX_HTTP],
            client_data_string,
            NULL,
            -1
        );
        
        // if data is HTTP, continue analysis
        if(match_status == 1) {
            
            char *ranges = NULL;
            match_status = match_regex(
                regexes[REGEX_RANGE_HEADER],
                client_data_string,
                &ranges,
                1
            );
            
            // If we have found a range header, count the number of ranges and
            // set proxy verdict
            if(match_status == 1 && ranges != NULL) {
                int range_count = match_regex_count(
                    regexes[REGEX_RANGE],
                    ranges
                );
                free(ranges);
                fprintf(stderr, "Range count: %d\n", range_count);
                
                // we don't allow more than 50 ranges in range header
                if(range_count > 50) {
                    verdict = PROXY_BLOCK;
                }
            }
        }
    }
    
    free(client_data_string);    
    return verdict;
}
