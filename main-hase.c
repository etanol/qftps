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

struct _SessionScope  SS;  /* SS --> Session State */
static char          *Program_Name;

/* This is quite ugly, but Win64 type sizes are screwed */
#ifdef __WIN64__
#  define INT_FROM_HANDLE(h)  ((int) (long long) (h))
#else
#  define INT_FROM_HANDLE(h)  ((int) (h))
#endif


/*
 * Create a child process to attend the recently connected client.  As a
 * parameter, the socket descriptor of such connection is given.
 *
 * This function blocks, so is executed in its own thread to keep accepting new
 * incoming client connections.  This fact also forces the function prototype.
 *
 * The code here has been shamelessly taken and adapted from the Hasefroch
 * support page:
 *
 *   http://support.microsoft.com/default.aspx?scid=kb;en-us;150523
 */
static unsigned __stdcall create_child_process (void *x)
{
        HANDLE               dup_sk, sk = x;
        BOOL                 ok;
        STARTUPINFO          si;
        PROCESS_INFORMATION  pi;
        char                 argbuf[16];

        memset(&si, 0, sizeof(si));
        debug("In monitor thread");

        /*
         * Duplicate the socket descriptor to create an inheritable copy.
         */
        ok = DuplicateHandle(GetCurrentProcess(), sk, GetCurrentProcess(),
                             &dup_sk, 0, TRUE, DUPLICATE_SAME_ACCESS);
        if (!ok)
        {
                error("Duplicating handle %d", INT_FROM_HANDLE(sk));
                return 1;
        }
        debug("Socket %d duplicated to %d", INT_FROM_HANDLE(sk),
              INT_FROM_HANDLE(dup_sk));

        /*
         * Spawn the child process providing the inheritable socket descriptor
         * as a command line argument.
         */
        snprintf(argbuf, 16, "@ %d", INT_FROM_HANDLE(dup_sk));
        ok = CreateProcess(Program_Name, argbuf, NULL, NULL, TRUE, 0, NULL,
                           NULL, &si, &pi);
        if (!ok)
        {
                error("Creating child process");
                return 1;
        }

        /*
         * Now the child process can use the given socket descriptor directly.
         * But the parent process cannot close the original socket or something
         * weird may occur.
         *
         * However, we neither can forget about these to socket descriptors or
         * leakage will pay back in the long run.  Therefore, we need to wait
         * for the child process to finish before closing the descriptors.
         *
         * Because this operation would block the parent process (preventing
         * new clients from connecting), we need to perform it in a different
         * thread.  That's why each client connected generates a thread in the
         * parent process to monitor the child process that does all the work.
         */
        debug("And waiting for process to terminate");
        WaitForSingleObject(pi.hProcess, INFINITE);
        closesocket(INT_FROM_HANDLE(sk));
        closesocket(INT_FROM_HANDLE(dup_sk));
        debug("Child done, finishing monitor thread");

        return 0;
}


/*
 * Main program.
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

        printf("User FTP Server, version %s\n\n", UFTPS_VERSION);

        SS.pid = (int) _getpid();

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
                {
                        errno = 0;
                        fatal("Invalid port number");
                }
        }

        /* Preparing to serve */
        memset(&sai, 0, sizeof(struct sockaddr_in));
        sai.sin_family      = AF_INET;
        sai.sin_port        = htons(port);
        sai.sin_addr.s_addr = INADDR_ANY;

        bind_sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (bind_sk == -1)
                fatal("Creating main server socket");

        yes = 1;
        setsockopt(bind_sk, SOL_SOCKET, SO_REUSEADDR, (const char *) &yes,
                   sizeof(int));
        e = bind(bind_sk, (struct sockaddr *) &sai, sai_len);
        if (e == -1)
                fatal("Binding main server socket");

        e = listen(bind_sk, 5);
        if (e == -1)
                fatal("Listening at main server socket");

        notice("Listening on port %d (TCP)", port);
        notice("Use CTRL + C to finish");

        /* Main server loop (accepting connections) */
        do {
                sai_len = sizeof(struct sockaddr_in);
                cmd_sk  = accept(bind_sk, (struct sockaddr *) &sai, &sai_len);
                if (cmd_sk == -1)
                {
                        error("Accepting incoming connection");
                        continue;
                }

                /* Create a monitor thread for the client (read more above) */
                thread_handle = _beginthreadex(NULL, 0, create_child_process,
#ifdef __WIN64__
                                               (void *) (long long) cmd_sk,
#else
                                               (void *) cmd_sk,
#endif
                                               0, &th);
                if (thread_handle == 0)
                        error("Launching monitor thread");
        } while (1);

        return EXIT_SUCCESS;
}

