/*
 * Quick FTP Server,  Share folders over FTP without being root.
 *
 * Public domain.  The author disclaims copyright to this source code.
 */

#include "qftps.h"
#ifdef __MINGW32__
#  include "hase.h"
#else
#  include <sys/socket.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#endif


/*
 * Initialize the session environment for a new client.
 */
void init_session (int control_sk)
{
        int        e;
        socklen_t  sai_len;

        /* Basic initializations */
        SS.pid          = (int) getpid();
        SS.control_sk   = control_sk;
        SS.passive_sk   = -1;
        SS.data_sk      = -1;
        SS.input_offset = 0;
        SS.input_len    = 0;
        SS.mode         = DEFAULT_MODE;
        SS.rest_offset  = 0;
        SS.arg          = NULL;

        /* Set CWD to root */
        SS.cwd[0]  = '.';
        SS.cwd[1]  = '/';
        SS.cwd[2]  = '\0';
        SS.cwd_len = 3;

        /* Get local address so we can bind passive listening sockets only to
         * the network interface the client is connected to; instead of binding
         * to all interfaces */
        sai_len = sizeof(struct sockaddr_in);
        e = getsockname(control_sk, (struct sockaddr *) &SS.local_address,
                        &sai_len);
        if (e == -1)
                fatal("Getting local socket address");

        /* Get remote address in order to verify that we only establish data
         * connections with the same client (at least the same IP) */
        sai_len = sizeof(struct sockaddr_in);
        e = getpeername(control_sk, (struct sockaddr *) &SS.client_address,
                        &sai_len);
        if (e == -1)
                fatal("Getting remote socket address");

        notice("Attending new client from %s",
               inet_ntoa(SS.client_address.sin_addr));
        reply_c("220-Quick FTP Server ready.\r\n"
                "220 Features: a p .\r\n");
}

