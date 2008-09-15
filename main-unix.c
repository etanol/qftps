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
#include <sys/socket.h>
#include <sys/wait.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

struct _SessionScope  SS;  /* SS --> Session State*/


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
 * Main program.
 */
int main (int argc, char **argv)
{
        int                 bind_sk, cmd_sk, e, yes;
        int                 port = DEFAULT_PORT;
        struct sigaction    my_sa;
        struct sockaddr_in  sai;
        socklen_t           sai_len = sizeof(struct sockaddr_in);

        if (argc > 1)
        {
                port = atoi(argv[1]) & 0x00FFFF;
                if (port <= 1024)
                {
                        errno = 0;
                        fatal("Invalid port number");
                }
        }

        /* Signal handling */
        sigfillset(&my_sa.sa_mask);
        my_sa.sa_flags   = SA_RESTART;
        my_sa.sa_handler = child_finish;
        sigaction(SIGCHLD, &my_sa, NULL);

        sigemptyset(&my_sa.sa_mask);
        my_sa.sa_flags   = 0;
        my_sa.sa_handler = end;
        sigaction(SIGINT,  &my_sa, NULL);
        sigaction(SIGTERM, &my_sa, NULL);

        /* Connection handling, preparing to serve */
        sai.sin_family      = AF_INET;
        sai.sin_port        = htons(port);
        sai.sin_addr.s_addr = INADDR_ANY;

        bind_sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (bind_sk == -1)
                fatal("Creating main server socket");

        yes = 1;
        setsockopt(bind_sk, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
        e = bind(bind_sk, (struct sockaddr *) &sai, sai_len);
        if (e == -1)
                fatal("Binding main server socket");

        e = listen(bind_sk, 5);
        if (e == -1)
                fatal("Listening at main server socket");

        notice("UFTPS listening on port %d (TCP)", port);
        notice("Use CTRL + C to finish");
        notice("If you want to use a different port, specify it as the only argument in the command line");

        /* Main server loop (accepting connections) */
        do {
                sai_len = sizeof(struct sockaddr_in);
                cmd_sk  = accept(bind_sk, (struct sockaddr *) &sai, &sai_len);
                if (cmd_sk == -1)
                {
                        if (errno != EINTR)
                                error("Accepting incoming connection");
                        continue;
                }

                e = fork();
                if (e == 0)
                {
                        /***  CHILD  ***/
                        close(bind_sk);
                        init_session(cmd_sk);
                        command_loop();
                }
                else
                {
                        /***  PARENT  ***/
                        if (e == -1)
                                error("Could not create a child process");
                        close(cmd_sk);
                }
        } while (1);

        return EXIT_SUCCESS;
}

