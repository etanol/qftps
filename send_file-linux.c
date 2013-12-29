/*
 * Quick FTP Server,  Share folders over FTP without being root.
 *
 * Public domain.  The author disclaims copyright to this source code.
 */

#include "qftps.h"
#include <limits.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>


/*
 * RETR command implementation usin Linux sendfile() to minimize memory copies.
 * Due to the nature of the system call, it is not even necessary to perform the
 * initial seek in case a REST command was issued.
 */
void send_file (void)
{
        int    f, b, e;
        off_t  size, completed;

        completed      = SS.rest_offset;
        SS.rest_offset = 0;

        f = open_file(&size);
        if (f == -1)
                return;

        e = open_data_channel();
        if (e == -1)
        {
                close(f);
                return;
        }

        reply_c("150 Sending file content.\r\n");
        debug("Initial offset is %lld", (long long) completed);

        /* Main transfer loop */
        while (completed < size && e != -1)
        {
                b = (size - completed > INT_MAX ? INT_MAX : size - completed);
                e = sendfile(SS.data_sk, f, &completed, b);
        }

        if (e != -1)
                reply_c("226 File content sent.\r\n");
        else
        {
                error("Sending file %s", SS.aux);
                reply_c("426 Connection closed, transfer aborted.\r\n");
        }

        close(f);
        closesocket(SS.data_sk);
        SS.data_sk = -1;
}

