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

/*
 * PORT command implementation.
 *
 * The argument has the syntax "h1,h2,h3,h4,p1,p2".  First, we need to split IP
 * and port information "h1.h2.h3.h4\0p1,p2" (two strings).  Then obtain the
 * port number from bytes p1 and p2. inet_aton() function converts the first
 * string "h1.h2.h3.h4" into a 'struct addr_in' value.
 *
 * Port number is interpreted as it comes "p1,p2".  That is, no matter the
 * machine endianness, p1 is the most significant byte and p2 the least
 * significant byte.  But before assigning this value to the sockaddr_in
 * structure, we must encode it BIG ENDIAN (htons()).
 *
 * Once all this is done (all data processed), we are able to create a socket
 * and try to connect to it.  If everything is allright, passive mode is
 * disabled for the next command.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

#define BAD_PARAMETER  "501 Invalid PORT parameter.\r\n"
#define BAD_CONNECTION "425 Cannot open data connection.\r\n"


void client_port (void)
{
        int                sk, commas, e, i;
        unsigned short     port;
        char              *str;
        struct sockaddr_in saddr;

        debug("PORT initial argument: %s\n", SS.arg);

        str    = SS.arg;
        commas = 0;
        while (commas < 4)
        {
                if (*str == '\0')
                {
                        warning("PORT invalid parameter '%s'", SS.arg);
                        reply_c(BAD_PARAMETER);
                        return;
                }

                if (*str == ',')
                {
                        commas++;
                        *str = '.';
                }
                str++;
        }

        str[-1] = '\0'; /* *--str++ = '\0'; */

        /* "h1.h2.h3.h4" ==> struct in_addr */
        e = inet_aton(SS.arg, &(saddr.sin_addr));
        if (e == 0)
        {
                error("Translating PORT IP '%s'", SS.arg);
                reply_c(BAD_PARAMETER);
                return;
        }

        /* "p1,p2" ==> unsigned short */
        for (i = 0; str[i] != ','; i++)
                /* ... */;
        str[i] = '\0';
        port   = atoi(str);
        port <<= 8;
        port  |= atoi(str + i + 1);

        debug("PORT parsing results %s:%d\n", SS.arg, port);

        saddr.sin_family = AF_INET;
        saddr.sin_port   = htons(port);

        sk = socket(PF_INET, SOCK_STREAM, 0);
        if (sk == -1)
        {
                error("Creating socket for PORT request %s:%d", SS.arg, port);
                reply_c(BAD_CONNECTION);
                return;
        }

        e = connect(sk, (struct sockaddr *) &saddr, sizeof(saddr));
        if (e == -1)
        {
                error("Connecting actively to %s:%d", SS.arg, port);
                reply_c(BAD_CONNECTION);
                e = close(sk);
                if (e == -1)
                        error("Closing active data socket");
                return;
        }

        SS.data_sk      = sk;
        SS.passive_mode = 0;
        reply_c("200 PORT Command OK.\r\n");
}

