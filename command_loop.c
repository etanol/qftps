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

/*
 * Main FTP server loop which attends one client.
 *
 * It seems unbelievable but it's possible to implement a FTP server without
 * the need of select() or poll().  In fact, this kind of complexity is present
 * in the client.
 *
 * Commands not implemented are interpreted as unknown.  There used to be
 * dedicated dummy messages for some of them but code simplicity is preferred.
 */

#include "uftps.h"
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


void command_loop (void)
{
        while (1) {
                switch (next_command()) {

                /*
                 * Straightforward implementations.
                 */
                case FTP_NOOP:
                        send_reply(S_cmd_sk, "200 I'm alive, don't worry\r\n");
                        break;

                case FTP_FEAT:
                        send_reply(S_cmd_sk,
                                   "211-Feature list:\r\n"
                                   "211- MDTM\r\n"
                                   "211- REST STREAM\r\n"
                                   "211- SIZE\r\n"
                                   "211- TVFS\r\n"
                                   "211 End.\r\n");
                        break;

                case FTP_PASS:
                case FTP_USER:
                        send_reply(S_cmd_sk, "230 I don't care.\r\n");
                        break;

                case FTP_SYST:
                        send_reply(S_cmd_sk, "215 UNIX Type: L8\r\n");
                        break;

                case FTP_OPTS:
                        send_reply(S_cmd_sk, "501 Option not understood.\r\n");
                        break;

                case FTP_ACCT:
                case FTP_SMNT:
                        send_reply(S_cmd_sk, "202 Unimplemented, ignored.\r\n");
                        break;

                case FTP_REIN:
                        send_reply(S_cmd_sk, "220 Nothing to REIN.\r\n");
                        break;

                /*
                 * A bit more complex commands.
                 */
                case FTP_MODE:
                        if (toupper(S_arg[0]) == 'S')
                                send_reply(S_cmd_sk,
                                           "200 MODE set to stream.\r\n");
                        else
                                send_reply(S_cmd_sk,
                                           "504 Mode not supported.\r\n");
                        break;

                case FTP_STRU:
                        if (toupper(S_arg[0]) == 'F')
                                send_reply(S_cmd_sk,
                                           "200 STRUcture set to file.\r\n");
                        else
                                send_reply(S_cmd_sk,
                                           "504 Structure not supported.\r\n");
                        break;

                case FTP_TYPE:
                        switch (toupper(S_arg[0])) {
                        case 'I':
                        case 'A':
                        case 'L':
                                send_reply(S_cmd_sk, "200 Whatever.\r\n");
                                break;
                        default:
                                send_reply(S_cmd_sk,
                                           "501 Invalid type.\r\n");
                        }
                        break;

                case FTP_QUIT:
                        send_reply(S_cmd_sk, "221 Goodbye.\r\n");
                        close(S_cmd_sk);
                        close(S_passive_bind_sk);
                        exit(EXIT_SUCCESS);
                        break;

                case FTP_PASV:
                        S_passive_mode = 1;
                        send_reply(S_cmd_sk, S_passive_str);
                        break;

                case FTP_PWD:
                        getcwd(AuxBuf, LINE_SIZE);
                        /* Root directory santy check */
                        if (AuxBuf[Basedir_len] == '\0') {
                                AuxBuf[Basedir_len] = '/';
                                AuxBuf[Basedir_len + 1] = '\0';
                        }
                        /* We can overwrite the line buffer as no useful
                         * argument will be present */
                        snprintf(LineBuf, LINE_SIZE, "257 \"%s\"\r\n",
                                 AuxBuf + Basedir_len);
                        send_reply(S_cmd_sk, LineBuf);
                        break;

                case FTP_REST:
                        /* We don't need str_to_ll() as sscanf() does de job */
                        sscanf(S_arg, "%lld", &S_offset);
                        snprintf(AuxBuf, LINE_SIZE, "350 Got it (%lld).\r\n",
                                 S_offset);
                        send_reply(S_cmd_sk, AuxBuf);
                        break;

                /*
                 * Complex commands implemented separately.
                 */
                case FTP_PORT:
                        client_port();
                        break;

                case FTP_NLST:
                        list_dir(0);
                        break;

                case FTP_LIST:
                        list_dir(1);
                        break;

                case FTP_MDTM:
                        file_stats(0);
                        break;

                case FTP_SIZE:
                        file_stats(1);
                        break;

                case FTP_CWD:
                        change_dir();
                        break;

                case FTP_RETR:
                        send_file();
                        break;

                case FTP_NONE:
                default:
                        send_reply(S_cmd_sk, "500 Command unrecognized.\r\n");
                }
        }
}

