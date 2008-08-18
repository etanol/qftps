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

#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>


void command_loop (void)
{
        int  l;

        while (1) {
                switch (next_command()) {

                /*
                 * Straightforward implementations.
                 */
                case FTP_NOOP:
                        reply_c("200 I'm alive, don't worry\r\n");
                        break;

                case FTP_FEAT:
                        reply_c("211-Feature list:\r\n"
                                "211- MDTM\r\n"
                                "211- REST STREAM\r\n"
                                "211- SIZE\r\n"
                                "211- TVFS\r\n"
                                "211 End.\r\n");
                        break;

                case FTP_PASS:
                case FTP_USER:
                        reply_c("230 I don't care.\r\n");
                        break;

                case FTP_SYST:
                        reply_c("215 UNIX Type: L8\r\n");
                        break;

                case FTP_OPTS:
                        reply_c("501 Option not understood.\r\n");
                        break;

                case FTP_ACCT:
                case FTP_SMNT:
                        reply_c("202 Unimplemented, ignored.\r\n");
                        break;

                case FTP_REIN:
                        reply_c("220 Nothing to REIN.\r\n");
                        break;

                /*
                 * A bit more complex commands.
                 */
                case FTP_MODE:
                        if (toupper(SS.arg[0]) == 'S')
                                reply_c("200 MODE set to stream.\r\n");
                        else
                                reply_c("504 Mode not supported.\r\n");
                        break;

                case FTP_STRU:
                        if (toupper(SS.arg[0]) == 'F')
                                reply_c("200 STRUcture set to file.\r\n");
                        else
                                reply_c("504 Structure not supported.\r\n");
                        break;

                case FTP_TYPE:
                        switch (toupper(SS.arg[0])) {
                        case 'I':
                        case 'A':
                        case 'L':
                                reply_c("200 Whatever.\r\n");
                                break;
                        default:
                                reply_c("501 Invalid type.\r\n");
                        }
                        break;

                case FTP_QUIT:
                        reply_c("221 Goodbye.\r\n");
                        close(SS.control_sk);
                        close(SS.passive_sk);
                        exit(EXIT_SUCCESS);
                        break;

                case FTP_PASV:
                        SS.passive_mode = 1;
                        reply(SS.passive_str, SS.passive_len);
                        break;

                case FTP_PWD:
                        l = snprintf(SS.aux, LINE_SIZE, "257 \"%s\"\r\n", &SS.cwd[1]);
                        reply(SS.aux, l);
                        break;

                case FTP_REST:
                        /* We don't need str_to_ll() as sscanf() does de job */
                        sscanf(SS.arg, "%lld", &SS.file_offset);
                        l = snprintf(SS.aux, LINE_SIZE, "350 Got it (%lld).\r\n",
                                     SS.file_offset);
                        reply(SS.aux, l);
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
                        reply_c("500 Command unrecognized.\r\n");
                        break;

                default:
                        reply_c("500 Command not implemented.\r\n");
                }
        }
}

