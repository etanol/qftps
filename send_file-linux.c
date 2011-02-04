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

