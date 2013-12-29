/*
 * Quick FTP Server,  Share folders over FTP without being root.
 *
 * Public domain.  The author disclaims copyright to this source code.
 */

#include "qftps.h"
#ifndef __MINGW32__
#  include <sys/socket.h>
#endif


/*
 * Send a reply to the client over the control connection.  Not a complex
 * function at all, but necessary almost everywhere else.
 */
void reply (const char *str, int len)
{
        int  b;

        debug("Reply   : %.*s", len - 2, str);

        do {
                b = send(SS.control_sk, str, len, 0);
                if (b == -1)
                        fatal("Control channel output");

                str += b;
                len -= b;
        } while (len > 0);
}


/*
 * Send reply over the current data connection.  It succeeds (return value 0)
 * when all data has been transferred.  It fails otherwise (return value -1),
 * that is, either by an incomplete transfer or a system error.
 */
int data_reply (const char *data, int len)
{
        int  b, l = 0;

        while (l < len)
        {
                b = send(SS.data_sk, &data[l], len - l, 0);
                if (b <= 0)
                {
                        error("Sending listing data");
                        return -1;
                }

                l += b;
        }

        return 0;
}

