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
#include <windows.h>
#include <process.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

struct _SessionScope  SS;  /* SS --> Session State*/
static char          *Program_Name;


static unsigned __stdcall create_child_process (void *x)
{
        int                  dup_sk, sk = (int) x;
        STARTUPINFO          si;
        PROCESS_INFORMATION  pi;
        char                 argbuf[16];

        memset(&si, 0, sizeof(si));

        debug("In monitor thread");
        // Duplicate the socket OrigSock to create an inheritable copy.
        if (!DuplicateHandle(GetCurrentProcess(), (HANDLE)    sk,
                             GetCurrentProcess(), (HANDLE *) &dup_sk, 0, TRUE,
                             DUPLICATE_SAME_ACCESS))
        {
                error("Duplicating handle %d", sk);
                return 1;
        }

        // Spawn the child process.
        // The first command line argument (argv[1]) is the socket handle.

        snprintf(argbuf, 16, "@ %d", dup_sk);
        debug("Socket %d duplicated to %d", sk, dup_sk);

        if (!CreateProcess(Program_Name, argbuf, NULL, NULL, TRUE, 0, NULL,
                           NULL, &si, &pi))
        {
                error("Creating child process");
                return 1;
        }


        // The duplicated socket handle must be closed by the owner
        // process--the parent. Otherwise, socket handle leakage
        // occurs. On the other hand, closing the handle prematurely
        // would make the duplicated handle invalid in the child. In this
        // sample, we use WaitForSingleObject(pi.hProcess, INFINITE) to
        // wait for the child.
        debug("And waiting for process to terminate");
        WaitForSingleObject(pi.hProcess, INFINITE);
        closesocket(sk);
        closesocket(dup_sk);
        debug("Child done, finishing monitor thread");

        return 0;
}


/*
 * Main program.  Opens the command port until a client requests a connection.
 * Then the server is forked and the child will manage all that client's
 * requests.
 */
int main (int argc, char **argv)
{
        int                 bind_sk, cmd_sk, e, yes;
        int                 port = DEFAULT_PORT;
        unsigned            th;
        unsigned long       thread_handle;
        struct sockaddr_in  sai;
        socklen_t           sai_len = sizeof(struct sockaddr_in);
        WSADATA             wd;

        if (WSAStartup(MAKEWORD(2, 2), &wd))
                fatal("Starting WinSock");
        atexit((void (*)()) WSACleanup);

        /* Am I a child? */
        if (argv[0][0] == '@' && argv[0][1] == '\0' && argc > 1)
        {
                cmd_sk = atoi(argv[1]);
                debug("Inherited socket is %d", cmd_sk);
                init_session(cmd_sk);
                command_loop();
        }
        Program_Name = argv[0];

        /* Then I must be the server */
        if (argc > 1)
        {
                port = atoi(argv[1]) & 0x00FFFF;
                if (port <= 1024)
                        fatal("This port number is restricted");
        }

        /* Connection handling */
        sai.sin_family      = AF_INET;
        sai.sin_port        = htons(port);
        sai.sin_addr.s_addr = INADDR_ANY;

        bind_sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (bind_sk == -1)
                fatal("Creating main server socket");

        yes = 1;
        setsockopt(bind_sk, SOL_SOCKET, SO_REUSEADDR, (const char *) &yes,
                   sizeof(yes));
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
                        error("Accepting incoming connection");
                        continue;
                }

                thread_handle = _beginthreadex(NULL, 0, create_child_process,
                                               (void *) cmd_sk, 0, &th);
                if (thread_handle == 0)
                        error("Launching monitor thread");
        } while (1);

        return EXIT_SUCCESS;
}

