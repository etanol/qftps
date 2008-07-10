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

/*
 * Return the next command (as an integer) available in the control channel.
 *
 * Most of this implementation has been inspired by str_netfd_alloc() from
 * netstr.c of VsFTPd.
 */

#include "uftps.h"
#include "command_list.h" /* Command recognizer */
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


command_t next_command (void)
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
                b = recv(S_cmd_sk, LineBuf + i, LINE_SIZE - i, MSG_PEEK);
                if (b == 0) {
                        if (i == LINE_SIZE)
                                printf("(%d) * Overflow attempt, exiting.\n",
                                        getpid());
                        else
                                printf("(%d) * Close request without QUIT command, exiting.\n",
                                        getpid());
                        close(S_cmd_sk);
                        exit(EXIT_FAILURE);
                }

                /* Locate LF character, j will point to it */
                for (j = i; j < (b + i) && LineBuf[j] != '\n'; j++)
                        /* ... */;

                /* Check limits */
                if (j == (b + i)) {
                        /* LF not found yet, clean the socket buffer */
                        i += b;
                        j  = b;
                        do {
                                b  = recv(S_cmd_sk, AuxBuf, j, 0);
                                j -= b;
                        } while (j > 0);
                } else {
                        /* LF found, consume only that line and get out */
                        LineBuf[j-1] = '\0';
                        i = j + 1;
                        do {
                                b  = recv(S_cmd_sk, AuxBuf, i, 0);
                                i -= b;
                        } while (i > 0);
                        keepon = 0;
                }
        }

        debug_msg(">>> %s\n", LineBuf);

        /* Check if arguments are present and set 'S_arg' pointer as needed */
        for (; LineBuf[i] != ' ' && LineBuf[i] != '\0'; i++)
                LineBuf[i] = (char) (toupper(LineBuf[i]) & 0x07F);

        if (LineBuf[i] != '\0')
                S_arg = LineBuf + i + 1;
        else
                S_arg = NULL;

        cmd = command_lookup(LineBuf, i);
        if (cmd == NULL)
            return FTP_NONE;

        return cmd->value;
}

