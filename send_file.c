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
#include <sys/sendfile.h>
#include <unistd.h>
#include <fcntl.h>


/*
 * RETR command implementation.  It uses sendfile() because seems quite optimal,
 * just as vsftpd does.
 */
void send_file (void)
{
        int          fd, e;
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
                reply_c("550 Argument is not a file or does not exist.\r\n");
                return;
        }

        fd = open(SS.arg, O_RDONLY, 0);
        if (fd == -1)
        {
                reply_c("550 Could not open file.\r\n");
                return;
        }

        e = open_data_channel();
        if (e == -1)
                return;

        reply_c("150 Sending file content.\r\n");

        /* Apply a possible previous REST command.  Ignore errors, is it allowd
         * by the RFC? */
        if (SS.file_offset > 0)
                lseek(fd, SS.file_offset, SEEK_SET);

        while (SS.file_offset < s.st_size)
        {
                debug("Offset step: %lld", (long long) SS.file_offset);

                e = sendfile(SS.data_sk, fd, &SS.file_offset, INT_MAX);
                if (e == -1)
                        fatal("Could not send file");
        }

        debug("Offset end: %lld", (long long) SS.file_offset);

        reply_c("226 File content sent.\r\n");

        close(SS.data_sk);
        SS.file_offset  = 0;
        SS.data_sk      = -1;
}

