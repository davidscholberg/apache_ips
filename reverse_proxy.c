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

/**********
 * INCLUDES
 **********/

#include <stdio.h>      // for printf() and fprintf()
#include <sys/socket.h> // for socket(), bind(), and connect()
#include <arpa/inet.h>  // for sockaddr_in and inet_ntoa()
#include <stdlib.h>     // for atoi() and exit()
#include <string.h>     // for memset()
#include <unistd.h>     // for close()
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include "reverse_proxy.h"

/*********
 * DEFINES
 *********/

#define MAXPENDING 5    // Maximum outstanding connection requests 
#define BUFFERSIZE 1000000

/*********************
 * STATIC DECLARATIONS
 *********************/

/*
    client_callback and server_callback are function pointers that are called
    every time data is receieved from the client or server, respectively.
    The data is passed to the function, and the function returns one of
    PROXY_ALLOW, PROXY_BUFFER, or PROXY_BLOCK.
*/
int (*client_callback)(const char *, int);
int (*server_callback)(const char *, int);


static void die(char *error_message);

static void construct_sockaddr_in(
    struct sockaddr_in *sock_addr,
    uint32_t ip_address,
    uint16_t port
);

static int init_local_server_socket();

static int init_remote_server_socket();

static void handle_client_socket(
    int remote_client_socket,
    char *client_addr
);

/**********************
 * FUNCTION DEFINITIONS
 **********************/

void reverse_proxy(
    int (*client_callback_arg)(const char *, int),
    int (*server_callback_arg)(const char *, int)) {
    
    // The two arguments are callback functions that determine if the data from
    // the client or the server respectively should be proxied or not.
    
    client_callback = client_callback_arg;
    server_callback = server_callback_arg;
    
    // ignore child process return values to prevent zombie processes
    struct sigaction sig_action;
    sig_action.sa_handler = SIG_IGN;
    sigemptyset(&sig_action.sa_mask);
    sig_action.sa_flags = 0;
    
    if(sigaction(SIGCHLD, &sig_action, NULL) < 0) {
        die("sigaction() failed");
    }
    
    // set up logging
    openlog("reverse_proxy", LOG_PID, LOG_USER);
    
    int local_server_socket = init_local_server_socket();

    // init variables for client_socket
    int client_socket;
    struct sockaddr_in client_addr;
    unsigned int client_len;  // Length of client address data structure

    for (;;) {
        // Set the size of the in-out parameter
        client_len = sizeof(client_addr);

        // Wait for a client to connect
        client_socket = accept(
            local_server_socket,
            (struct sockaddr *) &client_addr, 
            &client_len
        );
        
        if (client_socket < 0) {
            die("accept() failed");
        }

        printf("Handling client %s\n", inet_ntoa(client_addr.sin_addr));

        // Create child process to handle client connection
        int pid = fork();
        if(pid == 0) {
            printf("I'm the child, yo\n");
            handle_client_socket(
                client_socket,
                inet_ntoa(client_addr.sin_addr)
            );
        }
        else if (pid < 0) {
            die("fork() failed");
        }
        else {
            printf("I'm the parent, yo\nChild PID: %d\n", pid);
            close(client_socket);
        }
    }
}


static void handle_client_socket(
    int remote_client_socket,
    char *client_addr) {

/*
    This is the function that proxies the connection between the connected
    client and the predefined server. This function is only called by child
    processes and does not return. It calls exit() after either the client or
    the server closes the connection, or one of the callback functions returns
    PROXY_BLOCK.
*/
    
    char client_buffer[BUFFERSIZE];
    char server_buffer[BUFFERSIZE];
    
    int remote_server_socket = init_remote_server_socket();
    int bytes_read;
    int bytes_sent;
    int client_bytes = 0;
    int server_bytes = 0;
    int client_verdict = PROXY_ALLOW;
    int server_verdict = PROXY_ALLOW;
    
    // proxy loop
    // read from client, write to server, read from server, write to client
    while(1) {
        
        // write client data to server socket
        if(client_verdict == PROXY_ALLOW && client_bytes > 0) {
            bytes_sent = send(
                remote_server_socket,
                client_buffer,
                client_bytes,
                0
            );
            
            // if error returned, break out of proxy loop
            if(bytes_sent < 0) {
                break;
            }
            
            // subract # of bytes sent from current # of bytes to send
            client_bytes -= bytes_sent;
            
            // if there's still data to send, move it to front of the buffer
            if(client_bytes > 0) {
                memmove(
                    client_buffer,
                    client_buffer + bytes_sent,
                    client_bytes
                );
            }
        }
        
        // write server data to client socket
        if(server_verdict == PROXY_ALLOW && server_bytes > 0) {
            bytes_sent = send(
                remote_client_socket,
                server_buffer,
                server_bytes,
                0
            );
            
            // if error returned, break out of proxy loop
            if(bytes_sent < 0) {
                break;
            }
            
            // subract # of bytes sent from current # of bytes to send
            server_bytes -= bytes_sent;
            
            // if there's still data to send, move it to front of the buffer
            if(server_bytes > 0) {
                memmove(
                    server_buffer,
                    server_buffer + bytes_sent,
                    server_bytes
                );
            }
        }
        
        // read from client socket
        bytes_read = recv(
            remote_client_socket,
            client_buffer + client_bytes,
            BUFFERSIZE - client_bytes,
            MSG_DONTWAIT
        );
        
        // if any error but a blocking error is returned, or if the socket was 
        // closed on the other end, break out of proxy loop
        if((bytes_read < 0 && (errno != EWOULDBLOCK || errno != EAGAIN))
            || bytes_read == 0) {
            
            break;
        }
        
        // if some data was recieved, add # of received bytes to current # of
        // bytes in buffer and call client_callback
        if(bytes_read > 0) {
            client_bytes += bytes_read;
            
            // if client_callback rejects client data, break out of proxy loop
            if(client_callback != NULL) {
                client_verdict = client_callback(client_buffer, client_bytes);
            
                if(client_verdict == PROXY_BLOCK) {
                    syslog(
                        LOG_WARNING,
                        "data from %s was rejected\n",
                        client_addr
                    );
                    break;
                }
                
                if(client_verdict == PROXY_BUFFER) {
                    syslog(
                        LOG_INFO,
                        "data from %s was buffered\n",
                        client_addr
                    );
                }
            }
        }
        
        // read from server socket
        bytes_read = recv(
            remote_server_socket,
            server_buffer + server_bytes,
            BUFFERSIZE - server_bytes,
            MSG_DONTWAIT
        );
        
        // if any error but a blocking error is returned, or if the socket was 
        //closed on the other end, break out of proxy loop
        if((bytes_read < 0 && (errno != EWOULDBLOCK || errno != EAGAIN))
            || bytes_read == 0) {
            
            break;
        }
        
        // if some data was recieved, add # of received bytes to current # of
        // bytes in buffer and call server_callback
        if(bytes_read > 0) {
            server_bytes += bytes_read;
            
            // if server_callback rejects server data, break out of proxy loop
            if(server_callback != NULL) {
                server_verdict = server_callback(server_buffer, server_bytes);
                
                if(server_verdict == PROXY_BLOCK) {
                    break;
                }
            }
        }
    }
    
    // cleanly close both sockets (FIN, FIN ACK)
    close(remote_server_socket);
    close(remote_client_socket);
    
    exit(0); // exit child process
    
    // make sure to set SA_NOCLDWAIT in parent process to prevent child process
    // zombification
}

static void die(char *error_message) {
    perror(error_message);
    exit(1);
}

static void construct_sockaddr_in(
    struct sockaddr_in *sock_addr,
    uint32_t ip_address,
    uint16_t port) {

    // ip_address and port must be in network byte order

    memset(sock_addr, 0, sizeof(*sock_addr));   // Zero out structure
    sock_addr->sin_family = AF_INET;                // Internet address family
    sock_addr->sin_addr.s_addr = ip_address; // 
    sock_addr->sin_port = port;
}

static int init_local_server_socket() {
    int local_server_socket;
    struct sockaddr_in local_server_addr;
    unsigned short local_server_port = 80;
    
    // Create socket for incoming connections
    if ((local_server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        die("socket() failed");
    }
    
    // Construct local address structure
    construct_sockaddr_in(
        &local_server_addr,
        htonl(INADDR_ANY),
        htons(local_server_port)
    );
    
    // Bind to the local address
    if (bind(local_server_socket, (struct sockaddr *) &local_server_addr, sizeof(local_server_addr)) < 0) {
        die("bind() failed");
    }
    // Mark the socket so it will listen for incoming connections
    if (listen(local_server_socket, MAXPENDING) < 0)
        die("listen() failed");
        
    //setsockopt reuse addr
    
    return local_server_socket;
}

static int init_remote_server_socket() {
    int remote_server_socket;
    struct sockaddr_in remote_server_addr;
    unsigned short remote_server_port = 80;
    
    // Create socket for remote connection
    if ((remote_server_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        die("socket() failed");
    
    // Construct remote address structure
    construct_sockaddr_in(
        &remote_server_addr,
        inet_addr("10.0.0.2"),
        htons(remote_server_port)
    );
    
    // connect to the remote address
    if (connect(remote_server_socket, (struct sockaddr *) &remote_server_addr, sizeof(remote_server_addr)) < 0)
        die("connect() failed");

    return remote_server_socket;
}



