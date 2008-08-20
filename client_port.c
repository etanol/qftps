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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>


/*
 * Parse a PORT argument and convert it to a internet address structure.
 * Returns -1 when a parsing error occurs.
 *
 * The argument has the syntax "h1,h2,h3,h4,p1,p2".  First, we need to split IP
 * and port information "h1.h2.h3.h4\0p1,p2" (two strings).  The IP string is
 * finally parsed by inet_aton() into a 'struct in_addr' value.  Finally, the
 * port number is obtained as p1 * 256 + p2 and translated to network byte
 * ordering with htons().
 */
static int parse_port_argument (struct sockaddr_in *sai)
{
        int  i, j, e, commas;
        int  port;

        /* "h1,h2,h3,h4,p1,p2" ==> "h1.h2.h3.h4"  "p1,p2" */
        i      = 0;
        commas = 0;
        while (commas < 4)
        {
                if (SS.arg[i] == '\0')
                {
                        warning("PORT invalid parameter '%s'", SS.arg);
                        return -1;
                }

                if (SS.arg[i] == ',')
                {
                        commas++;
                        SS.arg[i] = '.';
                }
                i++;
        }
        SS.arg[i - 1] = '\0';

        /* "h1.h2.h3.h4" ==> struct in_addr */
        e = inet_aton(SS.arg, &sai->sin_addr);
        if (e == 0)
        {
                error("PORT Translating IP '%s'", SS.arg);
                return -1;
        }

        /* "p1,p2" ==> int (port number) */
        j = i;
        while (SS.arg[j] != ',')
                j++;
        SS.arg[j] = '\0';
        port      = atoi(&SS.arg[i]) * 256 + atoi(&SS.arg[j + 1]);

        sai->sin_family = AF_INET;
        sai->sin_port   = htons(port);

        debug("PORT parsing results %s:%d\n", SS.arg, port);
        return 0;
}


/*
 * PORT command implementation.
 */
void client_port (void)
{
        int                 sk, e;
        struct sockaddr_in  sai;

        if (SS.arg == NULL)
        {
                warning("PORT without argument");
                reply_c("501 Argument required.\r\n");
                return;
        }

        debug("PORT initial argument: %s\n", SS.arg);

        e = parse_port_argument(&sai);
        if (e == -1)
        {
                reply_c("501 Invalid PORT parameter.\r\n");
                return;
        }

        /* Try to connect to the given address:port */
        sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sk == -1)
        {
                error("PORT creating socket");
                reply_c("425 Could not create a socket.\r\n");
                return;
        }

        e = connect(sk, (struct sockaddr *) &sai, sizeof(struct sockaddr_in));
        if (e == -1)
        {
                error("PORT connecting to %s", SS.arg);
                reply_c("425 Could not open data connection.\r\n");
                e = close(sk);
                if (e == -1)
                        error("PORT closing data socket");
                return;
        }

        SS.data_sk      = sk;
        SS.passive_mode = 0;
        reply_c("200 PORT Command OK.\r\n");
}

