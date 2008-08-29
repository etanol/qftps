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
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>


/*
 * RETR command implementation.  It uses sendfile() to minimize memory copies.
 */
void send_file (void)
{
        int    f, e;
        off_t  size, seek;

        f = open_file(&size);
        if (f == -1)
                return;

        /* Apply a possible previous REST command */
        if (SS.file_offset > 0)
        {
                seek = lseek(f, SS.file_offset, SEEK_SET);
                if (seek == -1)
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

        e = 0;
        while (SS.file_offset < size && e != -1)
        {
                debug("Offset step: %lld", (long long) SS.file_offset);

                e = sendfile(SS.data_sk, f, &SS.file_offset, INT_MAX);
                if (e == -1)
                        error("Sending file");
        }

        debug("Offset end: %lld", (long long) SS.file_offset);

        if (e != -1)
                reply_c("226 File content sent.\r\n");
        else
                reply_c("426 Connection closed, transfer aborted.\r\n");

        close(f);
        close(SS.data_sk);
        SS.data_sk     = -1;
        SS.file_offset = 0;
}

