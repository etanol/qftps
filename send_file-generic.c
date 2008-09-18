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
         * chunks of file.
         */
        while (completed < size)
        {
                b = read(f, SS.aux, LINE_SIZE);
                if (b == -1)
                        break;  /* Cannot show any useful error message here */

                e = data_reply(SS.aux, b);
                if (e == -1)
                        break;

                completed += b;
        }

        if (b != -1 && e != -1)
                reply_c("226 File content sent.\r\n");
        else
                reply_c("426 Something happened, transfer aborted.\r\n");

        close(f);
        closesocket(SS.data_sk);
        SS.data_sk = -1;
}

