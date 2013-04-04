/*
David Scholberg
Master's Project
Fall 2012
*/

#ifndef REVERSE_PROXY_H_
#define REVERSE_PROXY_H_


#define PROXY_ALLOW     0
#define PROXY_BUFFER    1
#define PROXY_BLOCK     2


void reverse_proxy(
    int (*client_callback_arg)(const char *, int),
    int (*server_callback_arg)(const char *, int)
);


#endif // REVERSE_PROXY_H_

