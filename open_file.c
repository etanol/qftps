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
#ifdef __MINGW32__
#  include "hase.h"
#else
#  include <sys/stat.h>
#  include <unistd.h>
#  include <fcntl.h>
#endif


/*
 * Open the file specified in the argument after performing various checks.
 * Returns the opened file descriptor when successful, -1 on error.  The total
 * size of the file is stored in "file_size".
 *
 * This function is the common code between all send_file() implementations.
 * Note that, on error, everybody is notified so the invocation to this function
 * is as simple as possible.
 */
int open_file (off_t *file_size)
{
        int          f, e;
        struct stat  s;

        if (SS.arg == NULL)
        {
                reply_c("501 Argument required.\r\n");
                return -1;
        }
        expand_arg();

        /* Avoid following symlinks, as usual */
        e = lstat(SS.arg, &s);
        if (e == -1 || !S_ISREG(s.st_mode))
        {
                if (e == -1)
                        error("Stating file %s", SS.arg);
                else
                        warning("%s is not a regular file", SS.arg);
                reply_c("550 Not a file.\r\n");
                return -1;
        }
        *file_size = s.st_size;

        f = open(SS.arg, O_RDONLY);
        if (f == -1)
        {
                error("Opening file %s", SS.arg);
                reply_c("550 Could not open file.\r\n");
        }

        return f;
}

