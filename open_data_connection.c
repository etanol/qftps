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
#include <unistd.h>


static int active_connection (void)
{
        int  sk, e;

        sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sk == -1)
                return -1;

        e = connect(sk, (struct sockaddr *) &SS.port_destination,
                    sizeof(struct sockaddr_in));

        return e == -1 ? -1 : sk;
}


int open_data_connection (void)
{
        int                 sk, e;
        struct sockaddr_in  sai;
        socklen_t           sai_len;

        if (SS.passive_mode)
                sk = accept(SS.passive_sk, (struct sockaddr *) &sai, &sai_len);
        else
                sk = active_connection();

        if (sk == -1)
        {
                error("Opening data connection");
                reply_c("425 Can't open data connection.\r\n");
                e = close(sk);
                if (e == -1)
                        error("Closing data socket");
                return -1;
        }

        SS.data_sk = sk;
        return 0;
}

