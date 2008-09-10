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
#include <string.h>


/*
 * Apply new path components to an existing working directory.  The working
 * directory is contained (full path) in "wd", which is a buffer with a capacity
 * of LINE_SIZE bytes.  Parameter "len" specifies the current length of the
 * working directory.
 *
 * This function returns the length of "wd" after applying "path" on it, which
 * means that the working directory is modified in-place.  In case the working
 * directory would exceed LINE_SIZE bytes (at any stage), -1 is returned.
 *
 * NOTE: Path lengths always include the terminating null byte.  This means that
 *       the last useful character of the path is at length - 2.
 */
static int apply_path (const char *path, char *wd, int len)
{
        int  i;

        if (*path == '/')
        {
                /* Absolute path, truncate wd */
                wd[0] = '.';
                wd[1] = '/';
                wd[2] = '\0';
                len   = 3;
        }

        do {
                /* Combine delimiters */
                while (*path == '/')
                        path++;

                if (*path == '\0')
                        /* No more components to apply */
                        break;

                /* Isolate next component */
                i = 0;
                while (path[i] != '/' && path[i] != '\0')
                        i++;

                if (i == 2 && path[0] == '.' && path[1] == '.')
                {
                        /* Return to parent directory, found ".." */
                        while (wd[len] != '/')
                                len--;
                        if (len == 1)
                                len++;  /* Root reached, fix */
                        wd[len] = '\0';
                        len++;
                }
                else if (i != 1 || path[0] != '.')
                {
                        /* Bounds check */
                        if (len + i >= LINE_SIZE)
                                return -1;

                        /* Apply component, because it is different than "." */
                        if (len > 3)
                                wd[len - 1] = '/';
                        else
                                len--;   /* Skip delimiter at root */

                        memcpy(wd + len, path, i);
                        len        += i + 1;
                        wd[len - 1] = '\0';
                }

                /* Now skip to the next component */
                path += i;
        } while (1);

        return len;
}


/*
 * Expand the command argument to its corresponding full path.  This is
 * necessary because chdir() is never performed so the correct path needs to be
 * resolved for each file or directory to be accessed.
 *
 * Returns the length of the expanded argument, including the trailing null
 * byte (just as apply_path() does).  Also note that the expanded argument is
 * saved in the auxiliary buffer and the argument pointer is redirected
 * accordingly.
 */
int expand_arg (void)
{
        int  len = SS.cwd_len;

        /* Even if the argument is empty, list_dir() requires the current
         * working directory to be copied */
        memcpy(SS.aux, SS.cwd, len);

        if (SS.arg != NULL)
        {
                len = apply_path(SS.arg, SS.aux, len);
                if (len == -1)
                {
                        reply_c("552 Path overflow.\r\n");
                        fatal("Path overflow expanding argument");
                }

#ifdef __MINGW32__
                /*
                 * Hasefroch is so stupid that does not accept "./" as directory
                 * path.  Instead, we have to convert that to ".".
                 */
                if (len == 3)
                {
                        len--;
                        SS.aux[len - 1] = '\0';
                }
#endif
        }

        SS.arg = SS.aux;
        debug("Argument expanded to %s", SS.arg);
        return len;
}

