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

