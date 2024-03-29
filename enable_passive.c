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
#include <string.h>
#include <stdio.h>


/*
 * PASV command implementation.  Nothing else to mention here, read the FTP RFC
 * if you don't know about data transmission in passive mode.
 */
void enable_passive (void)
{
        int                 bsk, l, e;
        struct sockaddr_in  sai;
        socklen_t           sai_len = sizeof(struct sockaddr_in);
        unsigned char       addr[6];
        char                pasv_reply[64];

        /* Safety check in case there was some error before */
        if (SS.passive_sk != -1)
        {
                closesocket(SS.passive_sk);
                SS.passive_sk = -1;
                SS.mode       = DEFAULT_MODE;
        }

        bsk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (bsk == -1)
        {
                error("Creating passive socket");
                goto out_error;
        }
        e = 1;
        setsockopt(bsk, SOL_SOCKET, SO_REUSEADDR, CCP_CAST &e, sizeof(int));

        /* In case there are various network interfaces, bind only to the one
         * the client is connected to; and use port 0 to let the system choose
         * one for us */
        sai_len = sizeof(struct sockaddr_in);
        memcpy(&sai, &SS.local_address, sai_len);
        sai.sin_port = 0;
        e = bind(bsk, (struct sockaddr *) &sai, sai_len);
        if (e == -1)
        {
                error("Binding to a random port");
                goto out_error;
        }
        e = listen(bsk, 1);
        if (e == -1)
        {
                error("Listening on passive socket");
                goto out_error;
        }

        /* Check what port number we were assigned */
        e = getsockname(bsk, (struct sockaddr *) &sai, &sai_len);
        if (e == -1)
        {
                error("Retrieving passive socket information");
                goto out_error;
        }
        debug("Passive mode listening on port %d", ntohs(sai.sin_port));

        /* String conversion is straightforward in IPv4, provided that all
         * fields in sockaddr_in structures are in network byte order */
        memcpy(&addr[0], &sai.sin_addr, 4);
        memcpy(&addr[4], &sai.sin_port, 2);
        l = snprintf(pasv_reply, 64,
                     "227 Entering Passive Mode (%u,%u,%u,%u,%u,%u).\r\n",
                     addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

        SS.passive_sk = bsk;
        SS.mode       = PASSIVE_MODE;
        reply(pasv_reply, l);
        return;

out_error:
        closesocket(bsk); /* bsk could be -1; never mind, only produces EBADF */
        reply_c("425 No way to open a port.\r\n");
}

