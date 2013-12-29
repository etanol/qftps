/*
 * Quick FTP Server,  Share folders over FTP without being root.
 *
 * Public domain.  The author disclaims copyright to this source code.
 */

#include "qftps.h"
#ifndef __MINGW32__
#  include <sys/socket.h>
#  include <arpa/inet.h>
#endif
#include <stdlib.h>
#include <string.h>

/* This is needed by Solaris */
#ifndef INADDR_NONE
#  define INADDR_NONE  -1
#endif


/*
 * Parse a PORT argument and convert it to a internet address structure.
 * Returns -1 when a parsing error occurs.
 *
 * The argument has the syntax "h1,h2,h3,h4,p1,p2".  First, we need to split IP
 * and port information "h1.h2.h3.h4\0p1,p2" (two strings).  The IP string is
 * finally parsed by inet_addr() into a 'in_addr_t' value.  Finally, the port
 * number is obtained as p1 * 256 + p2 and translated to network byte ordering
 * with htons().
 */
void parse_port_argument (void)
{
        int                 commas, port, i, j;
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
        sai.sin_addr.s_addr = inet_addr(SS.arg);
        if (sai.sin_addr.s_addr == INADDR_NONE)
        {
                error("PORT Translating IP '%s'", SS.arg);
                reply_c("501 Invalid PORT parameter.\r\n");
                return;
        }
        /* Check if destination IP is the same as the client's */
        if (sai.sin_addr.s_addr != SS.client_address.sin_addr.s_addr)
        {
                warning("PORT IP %s is not the same as the client's", SS.arg);
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

        SS.mode = ACTIVE_MODE;
        debug("PORT parsing results %s:%d\n", SS.arg, port & 0x00FFFF);
        reply_c("200 PORT Command OK.\r\n");
}

