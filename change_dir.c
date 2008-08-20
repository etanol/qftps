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
#include <stddef.h>


/*
 * Change the current working directory.  In practice, only the "virtual" path
 * is modified; without any chdir() call.  This way chroot emulation is
 * achieved: by explicitly controlling the path.
 *
 * Client tries to traverse the root by issuing ".." will be silently ignored,
 * as apply_path() swallows them.
 */
void change_dir (void)
{
        int  len;

        if (SS.arg == NULL)
        {
                reply_c("501 Argument required.\r\n");
                return;
        }

        len = apply_path(SS.arg, SS.cwd, SS.cwd_len);
        if (len == -1)
        {
                reply_c("552 Path overflow.\r\n");
                fatal("Path overflow in CWD");
        }

        SS.cwd_len = len;
        reply_c("250 Directory changed.\r\n");
        debug("Directory changed to '%s'", SS.cwd);
}

