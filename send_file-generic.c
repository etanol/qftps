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
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * Generic RETR command implementation.
 */
void send_file (void)
{
        int          f, b, l, e;
        off_t        progress = 0;
        struct stat  s;

        if (SS.arg == NULL)
        {
                reply_c("501 Argument required.\r\n");
                return;
        }
        expand_arg();

        e = lstat(SS.arg, &s);
        if (e == -1 || !S_ISREG(s.st_mode))
        {
                if (e == -1)
                        error("Stating %s", SS.arg);
                reply_c("550 Not a file.\r\n");
                return;
        }

        f = open(SS.arg, O_RDONLY, 0);
        if (f == -1)
        {
                error("Opening %s", SS.arg);
                reply_c("550 Could not open file.\r\n");
                return;
        }

        /* Apply a possible previous REST command */
        if (SS.file_offset > 0)
        {
                progress = lseek(f, SS.file_offset, SEEK_SET);
                if (progress == -1)
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

        /*
         * Main transfer loop.  We use the auxiliary buffer to temporarily store
         * chunks of file.
         */
        while (progress < s.st_size)
        {
                b = read(f, SS.aux, LINE_SIZE);
                if (b == -1)
                        break;

                e = data_reply(SS.aux, b);
                if (e == -1)
                        break;

                progress += b;
        }

        if (b != -1 && e != -1)
                reply_c("226 File content sent.\r\n");
        else
                reply_c("426 Something happened, transfer aborted.\r\n");

        close(f);
        close(SS.data_sk);
        SS.data_sk     = -1;
        SS.file_offset = 0;
}

