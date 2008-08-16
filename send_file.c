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

        if (Session.passive_mode)
                Session.data_sk = accept(Session.passive_bind_sk,
                                   (struct sockaddr *) &saddr, &slen);

        if (Session.arg == NULL)
        {
                send_reply(Session.cmd_sk, "501 Argument required.\r\n");
                goto finish;
        }

        expand_arg();
        fd = open(Session.arg, O_RDONLY, 0);

        if (fd == -1) {
                send_reply(Session.cmd_sk, "550 Could not open file.\r\n");
                goto finish;
        }

        err = fstat(fd, &st);
        if (err == -1 || !S_ISREG(st.st_mode)) {
                send_reply(Session.cmd_sk, "550 Could not stat file.\r\n");
                goto finish;
        }

        send_reply(Session.cmd_sk, "150 Sending file content.\r\n");

        /* Apply a possible previous REST command.  Ignore errors, is it allowd
         * by the RFC? */
        if (Session.offset > 0)
                (void) lseek(fd, (off_t) Session.offset, SEEK_SET);

        while (Session.offset < st.st_size) {
                debug("Offset step: %lld", Session.offset);

                err = sendfile(Session.data_sk, fd, (off_t *) &Session.offset, INT_MAX);
                if (err == -1)
                        fatal("Could not send file");
        }

        debug("Offset end: %lld", Session.offset);

        send_reply(Session.cmd_sk, "226 File content sent.\r\n");

finish:
        close(Session.data_sk);
        Session.passive_mode = 0;
        Session.offset       = 0;
        Session.data_sk      = -1;
}

