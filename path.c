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
 * FUTURE IDEAS:
 *
 *    - Check every component to see if it is a symbolic link in order to
 *      protect the shared hierarchy even more.  This may come at a cost in
 *      performance though.
 */

#include <string.h>

/*
 * Apply new path components to an existing working directory.  The working
 * directory is contained (full path) in wd, which is a buffer with a capacity
 * of size bytes.  This working directory path length is len bytes (counting the
 * null ending).
 *
 * The function returns the length of the resulting path after walking the path
 * argument from the working directory.  In case the working directory would
 * exceed size bytes (at any stage), -1 is returning.
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


void change_dir (void)
{
        int  len;

        if (Session.arg == NULL)
        {
                reply_c("501 Argument required.\r\n");
                return;
        }

        len = apply_path(Session.arg, Session.cwd, Session.cwd_len);
        if (len == -1)
        {
                reply_c("552 Path overflow.\r\n");
                fatal("Path overflow in CWD");
        }

        Session.cwd_len = len;
        reply_c("250 Directory changed.\r\n");
        debug("Directory changed to '%s'", Session.cwd);
}


int expand_arg (void)
{
        int  len = Session.cwd_len;

        strncpy(Session.AuxBuf, Session.cwd, len);

        if (Session.arg != NULL)
        {
                len = apply_path(Session.arg, Session.AuxBuf, Session.cwd_len);
                if (len == -1)
                {
                        reply_c("552 Path overflow.\r\n");
                        fatal("Path overflow");
                }
        }

        Session.arg = Session.AuxBuf;
        debug("Argument expanded to '%s'", Session.arg);
        return len;
}

