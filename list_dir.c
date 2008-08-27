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
#include <unistd.h>
#include <dirent.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

/* Month names */
static char month[12][4] = {
        "Jan\0", "Feb\0", "Mar\0", "Apr\0", "May\0", "Jun\0", "Jul\0", "Aug\0",
        "Sep\0", "Oct\0", "Nov\0", "Dec\0"
};


/*
 * Temporary workaround
 */
static void send_data (int sk, const char *str, int len)
{
        int  b;

        do {
                b = write(sk, str, len);
                if (b <= 0)
                        return;

                str += b;
                len -= b;
        } while (len > 0);
}


/*
 * LIST and NLST commands implementation.  The boolean argument enables LIST
 * mode, otherwise NSLT listing is sent.
 *
 * To make Mozilla Firefox list the directory contents correctly, some research
 * work had to be done.  That concluded in:
 *
 *     http://cr.yp.to/ftp.html
 *
 * In particular:
 *
 *     http://cr.yp.to/ftp/list/binls.html
 *
 * Only files or subdirectories are shown, the rest of items (symlinks, named
 * pipes, sockets...) are ignored.  The following line is sent when a file is
 * found:
 *
 * -rw-r--r-- 1 ftp ftp       999 Mon  88 7777 filename
 *
 * In case of a subdirectory, the line is:
 *
 * drwxr-xr-x 1 ftp ftp       999 Mon  88 7777 dirname
 *
 * Where '999' is the item size, 'Mon  88 7777' is the month, day and year,
 * respectively, of the last modification date.  Some servers display the hour
 * and minute when the distance between current time and last modification time
 * is less than six months.  We don't do that as it is considered irrelevant.
 * To obtain a more precise value for the last modification time, let the client
 * send an MDTM command.
 *
 * Note that most clients will appreciate listings where all its items are
 * stat()-able.
 */
void list_dir (int full_list)
{
        int             len, l, e;
        DIR            *dir;
        struct dirent  *dentry;
        struct stat     st;
        struct tm       t;
        char            item[512];

        /* Workaround for Konqueror and Nautilus */
        if (SS.arg != NULL && SS.arg[0] == '-')
                SS.arg = NULL;

        len = expand_arg();
        if (len > 3)
        {
                SS.arg[len - 1] = '/';
                len++;
        }

        dir = opendir(SS.arg);
        if (dir == NULL)
        {
                error("Opening directory '%s'", SS.arg);
                reply_c("550 Could not open directory.\r\n");
                return;
        }

        e = open_data_channel();
        if (e == -1)
                return;

        reply_c("150 Sending directory list.\r\n");

        do {
                dentry = readdir(dir);
                if (dentry == NULL)
                        break;

                l = strlen(dentry->d_name);
                if (len + l >= LINE_SIZE)
                        fatal("Path overflow in LIST/NLST");
                strcpy(&SS.arg[len - 1], dentry->d_name);

                debug("Stating '%s'", SS.arg);
                e = stat(SS.arg, &st);
                if (e == -1 || (!S_ISDIR(st.st_mode) && !S_ISREG(st.st_mode)))
                {
                        debug("Dentry '%s' skipped", SS.arg);
                        continue;
                }

                if (full_list)
                {
                        /* LIST */
                        gmtime_r(&(st.st_mtime), &t);
                        l = snprintf(item, 512,
                                     "%s 1 ftp ftp %13lld %s %3d %4d %s\r\n",
                                     (S_ISDIR(st.st_mode) ? "dr-xr-xr-x"
                                      : "-r--r--r--"), (long long) st.st_size,
                                     month[t.tm_mon], t.tm_mday, t.tm_year + 1900,
                                     dentry->d_name);
                }
                else
                {
                        /* NLST */
                        l = snprintf(item, 512, "%s%s", dentry->d_name,
                                     (S_ISDIR(st.st_mode) ? "/\r\n" : "\r\n"));
                }

                send_data(SS.data_sk, item, l);
        } while (1);

        reply_c("226 Directory list sent.\r\n");

        closedir(dir);
        e = close(SS.data_sk);
        if (e == -1)
                error("Closing data channel");
        SS.data_sk      = -1;
        SS.passive_mode = 0;
}

