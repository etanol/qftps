/*
 * User FTP Server,  Share folders over FTP without being root.
 * Copyright (C) 2008  Isaac Jurado
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "uftps.h"
#include <arpa/inet.h>


/*
 * Initialize the session environment for a new client.
 */
void init_session (int control_sk)
{
        int        e;
        socklen_t  sai_len;

        /* Basic initializations */
        SS.control_sk   = control_sk;
        SS.passive_sk   = -1;
        SS.data_sk      = -1;
        SS.input_offset = 0;
        SS.input_len    = 0;
        SS.passive_mode = 0;
        SS.file_offset  = 0;
        SS.arg          = NULL;

        /* Set CWD to root */
        SS.cwd[0]  = '.';
        SS.cwd[1]  = '/';
        SS.cwd[2]  = '\0';
        SS.cwd_len = 3;

        /* Get local address so we can bind passive listening sockets only to
         * the network interface the client is connected to; instead of binding
         * to all interfaces */
        sai_len = sizeof(struct sockaddr_in);
        e = getsockname(control_sk, (struct sockaddr *) &SS.local_address,
                        &sai_len);
        if (e == -1)
                fatal("Getting local socket address");

        /* Get remote address in order to verify that we only establish data
         * connections with the same client (at least the same IP) */
        sai_len = sizeof(struct sockaddr_in);
        e = getpeername(control_sk, (struct sockaddr *) &SS.client_address,
                        &sai_len);
        if (e == -1)
                fatal("Getting remote socket address");

        notice("Attending new client from %s",
               inet_ntoa(SS.client_address.sin_addr));
        reply_c("220 User FTP Server ready.\r\n");
}

