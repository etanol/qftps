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

/*
 * RETR command implementation.  It uses sendfile() because seems quite optimal,
 * just as VsFTPd does.
 *
 * Same restrictions, as in change_dir.c, apply for the argument.  See that file
 * for more details.
 *
 * Usually, a client sends a PASV command then immediately connects to the port
 * indicated by the server reply.  If we are continuously reusing the same port
 * for passive data connections, we have to open whether theres is error or not
 * to shift one position in the connection wait queue.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>


void send_file (void)
{
        int                fd, err;
        struct sockaddr_in saddr;
        struct stat        st;
        socklen_t          slen = sizeof(saddr);

        if (S_passive_mode)
                S_data_sk = accept(S_passive_bind_sk,
                                   (struct sockaddr *) &saddr, &slen);

        if (!path_is_secure(S_arg)) {
                send_reply(S_cmd_sk, "550 Path to file is insecure.\r\n");
                goto finish;
        }

        fd = open(expanded_arg(), O_RDONLY, 0);

        if (fd == -1) {
                send_reply(S_cmd_sk, "550 Could not open file.\r\n");
                goto finish;
        }

        err = fstat(fd, &st);
        if (err == -1 || !S_ISREG(st.st_mode)) {
                send_reply(S_cmd_sk, "550 Could not stat file.\r\n");
                goto finish;
        }

        send_reply(S_cmd_sk, "150 Sending file content.\r\n");

        /* Apply a possible previous REST command.  Ignore errors, is it allowd
         * by the RFC? */
        if (S_offset > 0)
                (void) lseek(fd, (off_t) S_offset, SEEK_SET);

        while (S_offset < st.st_size) {
                debug("Offset step: %lld", S_offset);

                err = sendfile(S_data_sk, fd, (off_t *) &S_offset, INT_MAX);
                if (err == -1)
                        fatal("Could not send file");
        }

        debug("Offset end: %lld", S_offset);

        send_reply(S_cmd_sk, "226 File content sent.\r\n");

finish:
        close(S_data_sk);
        S_passive_mode = 0;
        S_offset       = 0;
        S_data_sk      = -1;
}

