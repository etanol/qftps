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
#include <unistd.h>
#include <string.h>
#include <stdio.h>


void enable_passive (void)
{
        int                 bsk, l, e;
        struct sockaddr_in  sai;
        socklen_t           sai_len = sizeof(struct sockaddr_in);
        unsigned char       addr[6];
        char                pasv_reply[32];

        /* Safety check in case there was some error before */
        if (SS.passive_sk != -1)
        {
                e = close(SS.passive_sk);
                if (e == -1)
                        error("Closing unused passive socket");
                SS.passive_sk   = -1;
                SS.passive_mode = 0;
        }

        bsk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (bsk == -1)
        {
                error("Creating passive socket");
                goto error;
        }
        e = 1;
        setsockopt(bsk, SOL_SOCKET, SO_REUSEADDR, &e, sizeof(int));

        sai_len = sizeof(struct sockaddr_in);
        memcpy(&sai, &SS.local_address, sizeof(struct sockaddr_in));
        sai.sin_port = 0;
        e = bind(bsk, (struct sockaddr *) &sai, sai_len);
        if (e == -1)
        {
                error("Binding to a random port");
                goto error_close;
        }
        e = listen(bsk, 1);
        if (e == -1)
        {
                error("Listening on passive socket");
                goto error_close;
        }

        e = getsockname(bsk, (struct sockaddr *) &sai, &sai_len);
        if (e == -1)
        {
                error("Retrieving passive socket information");
                goto error_close;
        }
        memcpy(&addr[0], &sai.sin_addr, 4);
        memcpy(&addr[4], &sai.sin_port, 2);

        debug("Passive mode listening on port %d", ntohs(sai.sin_port));

        SS.passive_mode = 1;
        SS.passive_sk   = bsk;
        l = snprintf(pasv_reply, 32, "227 =%u,%u,%u,%u,%u,%u\r\n", addr[0],
                     addr[1], addr[2], addr[3], addr[4], addr[5]);

        reply(pasv_reply, l);
        return;

error_close:
        e = close(bsk);
        if (e == -1)
                error("Closing passive socket");
error:
        reply_c("425 No way to open a port for you.\r\n");
}

