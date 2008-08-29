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
#include <stdlib.h>
#include <string.h>


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
void parse_port_argument (void)
{
        int                 commas, port, i, j, e;
        struct sockaddr_in  sai;

        if (SS.arg == NULL)
        {
                warning("PORT without argument");
                reply_c("501 Argument required.\r\n");
                return;
        }

        /* "h1,h2,h3,h4,p1,p2" ==> "h1.h2.h3.h4"  "p1,p2" */
        i      = 0;
        commas = 0;
        while (commas < 4)
        {
                if (SS.arg[i] == '\0')
                {
                        warning("PORT invalid parameter '%s'", SS.arg);
                        reply_c("501 Invalid PORT parameter.\r\n");
                        return;
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
        e = inet_aton(SS.arg, &sai.sin_addr);
        if (e == 0)
        {
                error("PORT Translating IP '%s'", SS.arg);
                reply_c("501 Invalid PORT parameter.\r\n");
                return;
        }
        /* Check if destination IP is the same as the client's */
        if (sai.sin_addr.s_addr != SS.client_address.sin_addr.s_addr)
        {
                error("PORT IP %s is not the same as the client's", SS.arg);
                reply_c("501 Invalid PORT parameter.\r\n");
                return;
        }

        /* "p1,p2" ==> int (port number) */
        j = i;
        while (SS.arg[j] != ',')
                j++;
        SS.arg[j] = '\0';
        port      = atoi(&SS.arg[i]) * 256 + atoi(&SS.arg[j + 1]);

        /* Save PORT information for later use when opening the data channel */
        memset(&SS.port_destination, 0, sizeof(struct sockaddr_in));
        SS.port_destination.sin_family = AF_INET;
        SS.port_destination.sin_addr   = sai.sin_addr;
        SS.port_destination.sin_port   = htons(port & 0x00FFFF);
        SS.passive_mode                = 0;

        debug("PORT parsing results %s:%d\n", SS.arg, port & 0x00FFFF);
        reply_c("200 PORT Command OK.\r\n");
}

