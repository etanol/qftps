/*
 * Quick FTP Server,  Share folders over FTP without being root.
 *
 * Public domain.  The author disclaims copyright to this source code.
 */

#include "qftps.h"
#ifndef __MINGW32__
#  include <sys/socket.h>
#  include <arpa/inet.h>
#  include <unistd.h>
#endif


/*
 * Open a data channel in passive mode.  Return the socket of the data channel
 * if successful, -1 on any failure.
 */
static int passive_connection (void)
{
        int                 sk, e;
        struct sockaddr_in  sai;
        socklen_t           sai_len = sizeof(struct sockaddr_in);

        sk = accept(SS.passive_sk, NULL, NULL);

        /* Passive socket not needed anymore */
        closesocket(SS.passive_sk);
        SS.passive_sk = -1;

        if (sk == -1)
                return -1;

        /* Check if this new connection comes from the same IP as the client */
        e = getpeername(sk, (struct sockaddr *) &sai, &sai_len);
        if (e == -1 || sai.sin_addr.s_addr != SS.client_address.sin_addr.s_addr)
        {
                if (e == -1)
                        error("Getting remote data socket address");
                else
                        warning("A different client (%s) connected to a passive socket",
                                inet_ntoa(sai.sin_addr));
                closesocket(sk);
                return -1;
        }

        return sk;
}


/*
 * Open a data channel in active mode.  Return the socket of the data channel if
 * successful, -1 on any failure.
 *
 * Note that peer IP check is not needed here because it was performed directly
 * on the PORT request.
 */
static int active_connection (void)
{
        int  sk, e;

        sk = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sk == -1)
                return -1;

        e = connect(sk, (struct sockaddr *) &SS.port_destination,
                    sizeof(struct sockaddr_in));
        if (e == -1)
        {
                error("Initiating active data connection");
                closesocket(sk);
                return -1;
        }

        return sk;
}


/*
 * Open the data channel.  When successful, the "data_sk" session field is set
 * to the data channel socket and zero is returned.  If the connection
 * establishment fails, -1 is returned and the client is notified.
 */
int open_data_channel (void)
{
        int  sk;

        switch (SS.mode)
        {
        case ACTIVE_MODE:
                sk = active_connection();
                break;
        case PASSIVE_MODE:
                sk = passive_connection();
                break;
        case DEFAULT_MODE:
        default:
                warning("No data transfer mode selected");
                reply_c("425 Default data transfer mode unsupported.\r\n");
                return -1;
        }

        SS.mode = DEFAULT_MODE;

        if (sk == -1)
        {
                error("Opening data connection");
                reply_c("425 Can't open data connection.\r\n");
                return -1;
        }

        SS.data_sk = sk;
        return 0;
}

