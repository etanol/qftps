/*
 * User FTP Server,  Share folders over FTP without being root.
 *
 * Public domain.  The author disclaims copyright to this source code.
 */

#include "uftps.h"
#ifdef __MINGW32__
#  include "hase.h"
#else
#  include <sys/socket.h>
#  include <unistd.h>
#endif
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "command_parser.h"


/*
 * Read data from the control channel until a complete request (delimited by
 * CRLF) is found.
 *
 * Control channel processing is implemented in a pipelined fashion.  Trailing
 * bytes belonging to the following request are left in the buffer; to be read
 * in the next call to this function.
 */
static void read_request (void)
{
        int  l, i, b;

        i = 0;
        l = SS.input_len;
        /* Shift trailing data from previous call */
        if (SS.input_offset > 0)
                memmove(SS.input, &SS.input[SS.input_offset], l);

        do {
                while (i < l && SS.input[i] != '\n')
                        i++;
                if (SS.input[i] == '\n')
                {
                        if (i > 0 && SS.input[i - 1] == '\r')
                                break;
                        else
                                continue;
                }

                /* Buffer data exhausted, get more from the network */
                b = recv(SS.control_sk, &SS.input[l], LINE_SIZE - l, 0);
                if (b <= 0)
                {
                        if (b == -1)
                                fatal("Control channel input");
                        else if (l == LINE_SIZE)
                        {
                                errno = 0;
                                fatal("Input buffer overflow");
                        }
                        else
                        {
                                notice("Peer closed control connection");
                                closesocket(SS.control_sk);
                                exit(EXIT_SUCCESS);
                        }
                }

                l += b;
        } while (1);

        /* Mark residual (trailing) bytes for the next call */
        SS.input[i - 1] = '\0';
        SS.input[i]     = '\0';
        i++;
        SS.input_len    =  l - i;
        SS.input_offset = (l - i > 0 ? i : 0);

        debug("Request : %s", SS.input);
}


/*
 * Parse the current request from the control channel and return the
 * corresponding command number.  The command argument (SS.arg) is filled
 * accordingly.
 */
enum command next_command (void)
{
        const struct Cmd  *cmd;
        int                i;

        read_request();

        i = 0;
        while (SS.input[i] != ' ' && SS.input[i] != '\0')
        {
                SS.input[i] = toupper(SS.input[i] & 0x07F);
                i++;
        }

        SS.arg      = (SS.input[i] == ' ' ? &SS.input[i + 1] : NULL);
        SS.input[i] = '\0';

        cmd = parse_command(SS.input, i);
        return (cmd != NULL ? cmd->value : FTP_NONE);
}

