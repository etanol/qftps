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
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>


/*
 * clid_finish
 *
 * Reaper function.  For "zombies".
 */
static void child_finish (int sig)
{
        while (waitpid(-1, NULL, WNOHANG) > 0)
                debug("Collecting children");
}


/*
 * end
 *
 * End. What where you expecting?
 */
static void end (int sig)
{
        notice("Signal caught, exiting");
        exit(EXIT_SUCCESS);
}


/*
 * Main program.  Opens the command port until a client requests a connection.
 * Then the server is forked the child will manage all that client's requests.
 *
 * Attending multiple clients is necessary to allow some clients (like lftp(1))
 * perform multiple concurrent jops.  Like moving current transfer to de
 * background and then browse through directories.
 *
 * Also note that here the root directory is fixed.  As we can't protect with
 * chroot(), due to de lack of privilege, we must do a series of safety checks
 * to simulate that behaviour.  Because of this, some strong restrictions have
 * arised; which could reduce de number of FTP clients compatible with this
 * server.
 */

int main (int argc, char **argv)
{
        int                 bind_sk, cmd_sk, e, yes = 1;
        unsigned short      port = 0;
        struct sockaddr_in  saddr;
        struct sigaction    my_sa;

        setlinebuf(stdout);

        if (argc > 1)
                port = (unsigned short) atoi(argv[1]);

        /* Signal handling */
        sigfillset(&my_sa.sa_mask);
        my_sa.sa_flags   = SA_RESTART;
        my_sa.sa_handler = child_finish;
        sigaction(SIGCHLD, &my_sa, NULL);

        my_sa.sa_flags   = SA_NOMASK;
        my_sa.sa_handler = end;
        sigaction(SIGINT, &my_sa, NULL);

        /* Connection handling */
        saddr.sin_family = AF_INET;
        saddr.sin_port   = htons(port > 1024 ? port : DEFAULT_PORT);
        saddr.sin_addr.s_addr = INADDR_ANY;

        bind_sk = socket(PF_INET, SOCK_STREAM, 0);
        if (bind_sk == -1)
                fatal("Could not create socket");

        setsockopt(bind_sk, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        e = bind(bind_sk, (struct sockaddr *) &saddr, sizeof(saddr));
        if (e == -1)
                fatal("Could not bind socket");

        e = listen(bind_sk, 5);
        if (e == -1)
                fatal("Could not listen at socket");

        printf("UFTPS listening on port %d (TCP). Use CTRL+C to finish.\n\n",
               (port > 1024 ? port : DEFAULT_PORT));
        printf("If you want to use a different port use:\n"
               "\t%s <port>\n\n"
               "Where port must be between 1025 and 65535.\n", argv[0]);

        /* Main server loop (accepting connections) */
        do {
                cmd_sk = accept(bind_sk, (struct sockaddr *) &saddr,
                                (socklen_t *) &yes);
                if (cmd_sk == -1)
                {
                        if (errno == EINTR)
                                continue;
                        else
                                fatal("Could not open command connection");
                }

                e = fork();
                if (e == 0)
                {
                        /* Child */
                        e = close(bind_sk);
                        if (e == -1)
                                error("Closing server socket from child");
                        init_session(cmd_sk);
                        command_loop();
                }
                else
                {
                        /* Parent */
                        if (e == -1)
                                error("Could not create a child process");

                        e = close(cmd_sk);
                        if (e == -1)
                                error("Closing control channel %d from parent",
                                      cmd_sk);
                }
        } while (1);

        return EXIT_SUCCESS;
}

