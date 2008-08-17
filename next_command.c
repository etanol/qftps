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
 * Return the next command (as an integer) available in the control channel.
 *
 * Most of this implementation has been inspired by str_netfd_alloc() from
 * netstr.c of VsFTPd.
 */

#include "command_parser.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


enum command next_command (void)
{
        const struct Cmd  *cmd;
        int                i;

        read_request();

        i = 0;
        while (Session.input[i] != ' ' && Session.input[i] != '\0')
        {
                Session.input[i] = toupper(Session.input[i] & 0x07F);
                i++;
        }

        Session.arg      = (Session.input[i] == ' ' ? Session.input + i + 1 : NULL);
        Session.input[i] = '\0';

        cmd = parse_command(Session.input, i);
        if (cmd == NULL)
                return FTP_NONE;
        return cmd->value;
}

