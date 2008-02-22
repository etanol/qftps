/*
 * User FTP Server
 * Author : C2H5OH
 * License: GPL v2
 *
 * uftps.c - main
 *
 * Main program. Opens the command port until a client requests a connection.
 * Then the server is forked the child will manage all that client's requests.
 *
 * Attending multiple clients is necessary to allow some clients (like lftp(1))
 * perform multiple concurrent jops. Like moving current transfer to de
 * background and then browse through directories.
 *
 * Also note that here the root directory is fixed. As we can't protect with
 * chroot(), due to de lack of privilege, we must do a series of safety checks
 * to simulate that behaviour. Because of this, some strong restrictions have
 * arised; which could reduce de number of FTP clients compatible with this
 * server.
 */

#include "uftps.h"

/*
 * clid_finish
 *
 * Reaper function. For "zombies".
 */
static void
child_finish (int sig)
{
        while (waitpid(-1, NULL, WNOHANG) > 0)
                debug_msg("- Collecting children.\n");
}


/*
 * end
 *
 * End. What where you expecting?
 */
static void
end (int sig)
{
        printf("(%d) Signal caught, exiting...\n", getpid());
        exit(EXIT_SUCCESS);
}


/***   MAIN   ***/
int
main (int argc, char **argv)
{
        int                bind_sk, cmd_sk, err, yes = 1;
        unsigned short     port = 0;
        struct sockaddr_in saddr;
        struct sigaction   my_sa;

        setlinebuf(stderr);
        setlinebuf(stdout);

        if (argc > 1)
                port = (unsigned short) atoi(argv[1]);

        /* Fixate root directory */
        Basedir = getcwd(NULL, 0);
        if (Basedir == NULL)
                fatal("Could not retrieve working directory");
        Basedir_len = strlen(Basedir);
        debug_msg("Working directory is: %s (strlen = %d)\n", Basedir,
                  Basedir_len);

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
        err = bind(bind_sk, (struct sockaddr *) &saddr, sizeof(saddr));
        if (err == -1)
                fatal("Could not bind socket");

        err = listen(bind_sk, 5);
        if (err == -1)
                fatal("Could not listen at socket");

        printf("UFTPS listening on port %d (TCP). Use CTRL+C to finish.\n\n",
               (port > 1024 ? port : DEFAULT_PORT));
        printf("If you want to use a different port use:\n"
               "\t%s <port>\n\n"
               "Where port must be between 1025 and 65535.\n", argv[0]);

#if 0
        /* It's nearly impossible, to my knowledge, debug a program that forks.
         * First try to find the error using the debug_msg() facility. Then, if
         * you really need to use a debugger, then replace the 0 with a 1 to
         * compile this code. But remember you won't be able to serve multiple
         * connections */

        cmd_sk = accept(bind_sk, (struct sockaddr *) &saddr,
                          (socklen_t *) &yes);
        if (cmd_sk == -1)
                fatal("could not open command connection");

        init_session(cmd_sk);
        command_loop();
        return EXIT_SUCCESS;
#endif

        /* Main loop (accepting connections) */
        while (1) {
                cmd_sk = accept(bind_sk, (struct sockaddr *) &saddr,
                                  (socklen_t *) &yes);
                if (cmd_sk == -1) {
                        if (errno == EINTR)
                                continue;
                        else
                                fatal("Could not open command connection");
                }
                if (fork() == 0) {
                        /* Child */
                        close(bind_sk);
                        init_session(cmd_sk);
                        command_loop();
                } else {
                        /* Parent */
                        close(cmd_sk);
                }
        }
        return EXIT_SUCCESS;
}

