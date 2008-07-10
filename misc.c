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
 * Miscellaneous helper functions.
 */

#include "uftps.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


/*
 * fatal
 *
 * Show 'msg' followed by the system error and exit badly.
 */
void fatal (char *msg)
{
        fputs("FATAL ERROR: ", stderr);
        perror(msg);
        exit(EXIT_FAILURE);
}


/*
 * send_reply
 *
 * Send 'msg' through socket 'sk'.  Because a single send() call doesn't ensure
 * that all data is transferred (or maybe the call is interrupted), we need to
 * place it in a small loop to wait until all 'msg' contents, at least, gets
 * copied onto the internal TCP/IP stack buffers.
 */
void send_reply (int sk, char *msg)
{
        int msglen, b;

        debug_msg("<<< %s", msg);

        msglen = strlen(msg);
        do {
                b = send(sk, msg, msglen, 0);
                if (b == -1)
                        fatal("Could not send reply");
                msg    += b;
                msglen -= b;
        } while (msglen > 0);

}


#if 0
/*
 * str_to_ll
 *
 * Converts a string to its numeric value in 64 bit representation.  Quote from
 * sysutil.c file of VsFTPd:
 *
 *   ``atoll() is C99 standard - but even modern FreeBSD, OpenBSD don't
 *     haveit, so we'll supply our own''
 *
 * This function is almost a literal copy of vsf_sysutil_a_to_filesize_t(),
 * present in that file.  The only difference is that the string is processed
 * from left to right, in contrast with the original.  Therefore, here, negative
 * numbers are directly converted to 0.
 */
long long str_to_ll (char *str)
{
        long long value = 0;

        if (strlen(str) <= 15) {
                while (*str) {
                        if (*str < '0' || *str > '9')
                                break;
                        value *= 10;
                        value += *str - '0';
                        str++;
                }
        }
        if (value < 0)
                value = 0;
        return value;
}
#endif


/*
 * path_is_secure
 *
 * Check if 'path' is trying to access beyond the Basedir.  We shouldn't allow
 * that because we try to emulate chroot().
 *
 * This implementation is basically a small DFA (Deterministic Finite Automata)
 * which parses the path looking for any of the substrings "./" or "../" at the
 * beginning, "/./" or "/../" within, or else "/." or "/.." at the end.
 */
int path_is_secure (char *path)
{
        /*
         *       Current state ____      ____ Input value
         *                         \    /                       */
        const static int next_state[5][3] = {
                /* State 0 */ { 0, 1, 4 },     /* Input values:          */
                /* State 1 */ { 3, 2, 4 },     /*                        */
                /* State 2 */ { 3, 4, 4 },     /*     '/' = 0            */
                /* State 3 */ { 3, 3, 3 },     /*     '.' = 1            */
                /* State 4 */ { 0, 4, 4 }      /*   other = 2            */
        };
        int state = 0;  /* Initial state */
        int input;

        while (*path != '\0') {
                switch (*path) {
                        case '/': input = 0; break;
                        case '.': input = 1; break;
                        default : input = 2;
                }
                state = next_state[state][input];
                path++;
        }

        /* Accepting states (safe path) are: */
        return state == 0 || state == 4;
}


#ifdef DEBUG
/*
 * debug_msg
 *
 * Only implemented when debug flags are enabled.  Display an information
 * message to the stderr.  Useful to follow the progress of the command-reply
 * exchange without the need of a debugger.
 */
void debug_msg (const char *format, ...)
{
        va_list params;

        fprintf(stderr, "(%d) ", getpid());
        va_start(params, format);
        vfprintf(stderr, format, params);
        va_end(params);
}
#endif

