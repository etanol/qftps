/*
 * User FTP Server,  Share folders over FTP without being root.
 *
 * Public domain.  The author disclaims copyright to this source code.
 */

#include "uftps.h"
#ifdef __MINGW32__
#  include "hase.h"
#else
#  include <unistd.h>
#  include <fcntl.h>
#endif


/*
 * Generic RETR command implementation.
 */
void send_file (void)
{
        int    f, e, b = 0;
        off_t  size, completed;

        completed      = SS.rest_offset;
        SS.rest_offset = 0;

        f = open_file(&size);
        if (f == -1)
                return;

        /* Apply a possible previous REST command */
        if (completed > 0)
        {
                completed = lseek(f, completed, SEEK_SET);
                if (completed == -1)
                {
                        error("Seeking file %s", SS.arg);
                        reply_c("450 Could not restart transfer.\r\n");
                        close(f);
                        return;
                }
        }

        e = open_data_channel();
        if (e == -1)
        {
                close(f);
                return;
        }

        reply_c("150 Sending file content.\r\n");
#ifdef __MINGW32__
        debug("Initial offset is %I64d", completed);
#else
        debug("Initial offset is %lld", (long long) completed);
#endif

        /*
         * Main transfer loop.  We use the auxiliary buffer to temporarily store
         * chunks of file.  Because of that, the filename is lost and cannot be
         * shown in error messages.
         */
        while (completed < size)
        {
                b = read(f, SS.aux, LINE_SIZE);
                if (b == -1)
                        break;

                e = data_reply(SS.aux, b);
                if (e == -1)
                        break;

                completed += b;
        }

        if (b != -1 && e != -1)
                reply_c("226 File content sent.\r\n");
        else
        {
                error("Sending a file");
                reply_c("426 Something happened, transfer aborted.\r\n");
        }

        close(f);
        closesocket(SS.data_sk);
        SS.data_sk = -1;
}

