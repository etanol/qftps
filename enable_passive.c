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
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


/*
 * Return a random non-privileged port, range 1025..65536
 */
static inline int get_random_port (void)
{
        int port;

        port = rand() & 0x00FFFF;
        if (port < 1024)
                port += 1024;

        return port;
}


void enable_passive (void)
{
        int                 bsk, port, i, e;
        char                ip[16];
        struct sockaddr_in  sai;
        socklen_t           sai_len = sizeof(struct sockaddr_in);

        bsk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (bsk == -1)
        {
                error("Creating passive socket");
                goto error;
        }

        /* XXX: This is temporary */
        getsockname(SS.control_sk, (struct sockaddr *) &sai, &sai_len);
        sai_len = sizeof(struct sockaddr_in);

        e = -1;
        i =  1;
        setsockopt(bsk, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(int));
        for (i = 0;  i < 9 && e == -1;  i++)
        {
                port = get_random_port();
                sai.sin_port = htons(port);
                e = bind(bsk, (struct sockaddr *) &sai, sai_len);
        }
        if  (e == -1)
        {
                error("Could not bind to any port");
                e = close(bsk);
                if (e == -1)
                        error("Closing passive socket");
                goto error;
        }

        debug("%d tries to end up using port %d", i, port);

        e = listen(bsk, 1);
        if (e == -1)
        {
                error("Listening on passive socket");
                e = close(bsk);
                if (e == -1)
                        error("Closing passive socket");
                goto error;
        }

        strncpy(ip, inet_ntoa(sai.sin_addr), 16);
        for (i = 0;  i < 16;  i++)
                if (ip[i] == '.')
                        ip[i] = ',';
        ip[15] = '\0';

        SS.passive_mode = 1;
        SS.passive_sk   = bsk;
        SS.passive_len  = snprintf(SS.passive_str, 32, "227 =%s,%d,%d\r\n",
                                  ip, port / 256, port % 256); return;

        debug("Passive string reply: %s", SS.passive_str);
        return;
error:
        SS.passive_mode = 0;
        SS.passive_sk   = -1;
        SS.passive_len  = snprintf(SS.passive_str, 32,
                                   "425 No way to open a port.\r\n");
}

