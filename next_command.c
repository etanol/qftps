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
 * Return the next command (as an integer) available in the control channel.
 *
 * Most of this implementation has been inspired by str_netfd_alloc() from
 * netstr.c of VsFTPd.
 */

#include "command_parser.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


enum command next_command (void)
{
        const struct Cmd *cmd;    /* parsed command value */
        int keepon; /* loop exit flag */
        int i;      /* processed and saved information index */
        int j;      /* unprocessed and saved information index */
        int b;      /* read bytes from recv() (auxiliary) */

        /*
         * First of all, try to read a whole line (consuming up to the CRLF
         * sequence), doing a peek; that is, reading from the socket buffer
         * without really consuming.  There is a hard limit of LINE_SIZE up to
         * the LF character.
         *
         * If a LF character is found before reaching the end of the read chunk,
         * only that line will be retired from the socket buffer.  Otherwise,
         * consume everything to let more network data to come.
         *
         * The management scheme of the 'Line' buffer is as follows:
         *
         *                              ____________ b ___________
         *                             /                          \
         *       +--------------------+----------------+----------+-------+
         * Line: |  Processed data    |  Last call to recv()      |  ...  |
         *       +--------------------+----------------+----------+-------+
         *                            ^                ^          ^
         *                            i                j        (i+b)
         */
        i = 0;
        keepon = 1;
        while (keepon) {
                b = recv(Session.cmd_sk, Session.LineBuf + i, LINE_SIZE - i, MSG_PEEK);
                if (b == 0) {
                        if (i == LINE_SIZE)
                                printf("(%d) * Overflow attempt, exiting.\n",
                                        getpid());
                        else
                                printf("(%d) * Close request without QUIT command, exiting.\n",
                                        getpid());
                        close(Session.cmd_sk);
                        exit(EXIT_FAILURE);
                }

                /* Locate LF character, j will point to it */
                for (j = i; j < (b + i) && Session.LineBuf[j] != '\n'; j++)
                        /* ... */;

                /* Check limits */
                if (j == (b + i)) {
                        /* LF not found yet, clean the socket buffer */
                        i += b;
                        j  = b;
                        do {
                                b  = recv(Session.cmd_sk, Session.AuxBuf, j, 0);
                                j -= b;
                        } while (j > 0);
                } else {
                        /* LF found, consume only that line and get out */
                        Session.LineBuf[j-1] = '\0';
                        i = j + 1;
                        do {
                                b  = recv(Session.cmd_sk, Session.AuxBuf, i, 0);
                                i -= b;
                        } while (i > 0);
                        keepon = 0;
                }
        }

        debug("Request '%s'", Session.LineBuf);

        /* Check if arguments are present and set 'Session.arg' pointer as needed */
        for (; Session.LineBuf[i] != ' ' && Session.LineBuf[i] != '\0'; i++)
                Session.LineBuf[i] = (char) (toupper(Session.LineBuf[i]) & 0x07F);

        if (Session.LineBuf[i] != '\0')
                Session.arg = Session.LineBuf + i + 1;
        else
                Session.arg = NULL;

        cmd = parse_command(Session.LineBuf, i);
        if (cmd == NULL)
            return FTP_NONE;

        return cmd->value;
}

