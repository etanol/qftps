/*
 * User FTP Server
 * Author : C2H5OH
 * License: GPL v2
 *
 * next_command.c - Return the next command (as an integer) available in the
 *                  control channel.
 *
 * Most of this implementation has been inspired by str_netfd_alloc() from
 * netstr.c of VsFTPd.
 */

#include "uftps.h"
#include "dfa.h" /* Command recognizer */

command_t
next_command (void)
{
        int keepon; /* loop exit flag */
        int i;      /* processed and saved information index */
        int j;      /* unprocessed and saved information index */
        int b;      /* read bytes from recv() (auxiliary) */
        int cmd;    /* parsed command value */

        /*
         * First of all, try to read a whole line (consuming up to the CRLF
         * sequence), doing a peek; that is, reading from the socket buffer
         * without really consuming. There is a hard limit of LINE_SIZE up to
         * the LF character.
         *
         * If a LF character is found before reaching the end of the read chunk,
         * only that line will be retired from the socket buffer. Otherwise,
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

        cmd = parse_command(LineBuf);

        /* Check if arguments are present and set 'S_arg' pointer as needed */
        for (; LineBuf[i] != ' ' && LineBuf[i] != '\0'; i++)
                /* ... */;

        if (LineBuf[i] != '\0')
                S_arg = LineBuf + i + 1;
        else
                S_arg = NULL;

        return (command_t) cmd;
}

