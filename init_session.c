/*
 * User FTP Server
 * Autor  : C2H5OH
 * License: GPL v2
 *
 * init_session.c - Preparing to serve one client.
 */

#include "uftps.h"

#define ERR_FINISH_MSG "421 Cannot attend you now, sorry.\r\n"

/*
 * get_random_port
 *
 * Returns a random non-privileged port, range 1025..65536
 */
static inline unsigned short get_random_port (void)
{
        unsigned char  byte;
        unsigned short port;

        port   = 0x0000U;
        byte   = (unsigned char) rand();
        port  |= byte;
        port <<= 8;
        byte   = (unsigned char) rand();
        port  |= byte;
        if (port < 1024)
                port += 1024;
        return port;
}


void init_session (int cmd_sk)
{
        unsigned short     port;
        int                err, i, sk;
        struct sockaddr_in saddr;
        socklen_t          saddr_len;
        char               address[16];

        /* Get my own IP. As the server is listening to all network interfaces,
         * we won't know the real IP until we are connected to someone */
        saddr_len = (socklen_t) sizeof(saddr);
        err = getsockname(cmd_sk, (struct sockaddr *) &saddr, &saddr_len);
        if (err == -1) {
                send_reply(cmd_sk, ERR_FINISH_MSG);
                fatal("trying to get my own address");
        }

        debug_msg("* Local address: %s\n", inet_ntoa(saddr.sin_addr));

        /* Create a data socket to use in passive mode. This socket is cached
         * and only one of these sockets is created per client. Thus, the
         * response to PASV is easier (and, maybe, quicker) */
        sk = socket(PF_INET, SOCK_STREAM, 0);
        if (sk == -1) {
                send_reply(cmd_sk, ERR_FINISH_MSG);
                fatal("Could not create data socket");
        }

        /* 9 random tries to get an available port */
        i   = 1;
        err = -1;
        setsockopt(sk, SOL_SOCKET, SO_REUSEADDR, &i, sizeof (i));
        srand((unsigned int) times (NULL));
        for (i = 9; i > 0 && err == -1; i--) {
                port = get_random_port();
                saddr.sin_port = htons(port);
                err = bind(sk, (struct sockaddr *) &saddr, saddr_len);
        }
        if (i <= 0) {
                send_reply(cmd_sk, ERR_FINISH_MSG);
                fatal("Could not find any bindable port");
        }

        debug_msg("* Number of bind() tries for PASV port: %d\n", 9 - i);

        err = listen(sk, 1);
        if (err == -1) {
                send_reply(cmd_sk, ERR_FINISH_MSG);
                fatal("cannot listen on port");
        }

        /* As de passive socket is cached, we can do the same for the PASV
         * response string */
        strncpy(address, inet_ntoa(saddr.sin_addr), 16);
        for (i = 0; i < 16; i++) {
                if (address[i] == '.')
                        address[i] = ',';
        }
        snprintf(S_passive_str, 64,
                 "227 Entering Passive Mode (%s,%d,%d).\r\n", address,
                 port >> 8, port & 0x00FF);

        debug_msg("* Passive data port: %d\n",  port);
        debug_msg("* Passive string reply: %s", S_passive_str);

        /* Set up the rest of the session state variables, except for Basedir
         * which is inherited from uftps.c */
        S_cmd_sk          = cmd_sk;
        S_data_sk         = -1;
        S_offset          = 0;
        S_passive_bind_sk = sk;
        S_passive_mode    = 0;

        send_reply(cmd_sk, "220 User FTP Server ready.\r\n");
}

