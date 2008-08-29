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
#include <sys/stat.h>
#include <string.h>


/*
 * Change the current working directory.  In practice, only the FTP path is
 * modified; without any chdir() call.  This way chroot emulation is achieved:
 * by explicitly controlling all paths.
 *
 * Even though the process working directory never changes, the issued path is
 * checked for existance in order to succeed.  Also, any client trie to traverse
 * the root by issuing ".." will be silently ignored, as expand_arg() swallows
 * every "." and ".." component.
 */
void change_dir (void)
{
        int          l, e;
        struct stat  s;

        if (SS.arg == NULL)
        {
                reply_c("501 Argument required.\r\n");
                return;
        }
        l = expand_arg();

        e = lstat(SS.arg, &s);
        if (e == -1 || !S_ISDIR(s.st_mode))
        {
                if (e == -1)
                        error("Stating directory %s", SS.arg);
                else
                        warning("Path %s is not a directory", SS.arg);
                reply_c("550 Could not change directory.\r\n");
        }
        else
        {
                memcpy(SS.cwd, SS.arg, l);
                SS.cwd_len = l;
                debug("Directory changed to %s", SS.cwd);
                reply_c("250 Directory changed.\r\n");
        }
}

