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
 * CWD command implementation.
 *
 * As we don't have root privileges, we can't execute the server within a
 * chroot() environment.  Therefore, we must impose a couple of restrictions
 * when the client tries to change the current working directory:
 *
 * 1. The command argument (i.e. the path we are being requested for) cannot
 *    contain any reference to the self directory "." nor to the parent
 *    directory ".."
 *
 * 2. If the argument begins with '/', it will be understood as an absolute path
 *    and then will be concatenated with the base directory (not visible to the
 *    client).
 *
 * 3. In any other case the parameter will be interpreted as a relative path, so
 *    it will simply be appended to the current working path.
 *
 * The complete working directory is retrieved via getcwd().  Given that we want
 * to hide part of it (the base path), BdLen contains the number of characters
 * we have to skip.  All this just to emulate chroot().
 *
 */

#include "uftps.h"

void change_dir (void)
{
        int err;

        if (!path_is_secure(S_arg)) {
                send_reply(S_cmd_sk, "550 Path is insecure.\r\n");
                return;
        }

        err = chdir(expanded_arg());

        if (err == -1) {
                send_reply(S_cmd_sk, "550 Could not change dir.\r\n");
                return;
        }

        send_reply(S_cmd_sk, "250 Directory changed.\r\n");
}

