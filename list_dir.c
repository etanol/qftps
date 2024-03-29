/*
 * Quick FTP Server,  Share folders over FTP without being root.
 *
 * Public domain.  The author disclaims copyright to this source code.
 */

#include "qftps.h"
#ifdef __MINGW32__
#  include "hase.h"
#else
#  include <sys/stat.h>
#  include <unistd.h>
#  include <time.h>
#endif
#include <dirent.h>
#include <string.h>
#include <stdio.h>

/* Month names */
static const char const month[12][4] = {
        "Jan\0", "Feb\0", "Mar\0", "Apr\0", "May\0", "Jun\0", "Jul\0", "Aug\0",
        "Sep\0", "Oct\0", "Nov\0", "Dec\0"
};


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
        struct stat     s;
        struct tm      *t;
        char            item[512];

        /* Workaround for Konqueror and Nautilus */
        if (SS.arg != NULL && SS.arg[0] == '-')
                SS.arg = NULL;
        len = expand_arg();

        dir = opendir(SS.arg);
        if (dir == NULL)
        {
                error("Opening directory '%s'", SS.arg);
                reply_c("550 Could not open directory.\r\n");
                return;
        }

        /* Prepare the argument for the listing, see below */
        if (len > 3)
        {
                SS.arg[len - 1] = '/';
                len++;
        }

        e = open_data_channel();
        if (e == -1)
        {
                closedir(dir);
                return;
        }

        reply_c("150 Sending directory list.\r\n");

        do {
                dentry = readdir(dir);
                if (dentry == NULL)
                        break;

                /*
                 * Due to the chroot emulation, each dentry needs to be
                 * prepended by the expanded argument (stored in the auxiliary
                 * buffer) after checking bounds.  Otherwise, the stat() call
                 * would fail.
                 */
                l = strlen(dentry->d_name);
                if (len + l >= LINE_SIZE)
                        fatal("Path overflow in LIST/NLST");
                strcpy(&SS.arg[len - 1], dentry->d_name);

                e = lstat(SS.arg, &s);
                if (e == -1 || (!S_ISDIR(s.st_mode) && !S_ISREG(s.st_mode)))
                {
                        debug("Dentry %s skipped", SS.arg);
                        continue;
                }

                if (full_list)
                {
                        /* LIST */
                        t = gmtime(&s.st_mtime);
                        if (t == NULL)
                        {
                                error("Converting time of %s", SS.arg);
                                continue;
                        }

                        l = snprintf(item, 512,
#ifdef __MINGW32__
                                     "%s 1 ftp ftp %13I64d %s %3d %4d %s\r\n",
#else
                                     "%s 1 ftp ftp %13lld %s %3d %4d %s\r\n",
#endif
                                     (S_ISDIR(s.st_mode) ? "dr-xr-xr-x"
                                      : "-r--r--r--"), (long long) s.st_size,
                                     month[t->tm_mon], t->tm_mday,
                                     t->tm_year + 1900, dentry->d_name);
                }
                else
                {
                        /* NLST */
                        l = snprintf(item, 512, "%s%s", dentry->d_name,
                                     (S_ISDIR(s.st_mode) ? "/\r\n" : "\r\n"));
                }

                e = data_reply(item, l);
        } while (e == 0);

        if (e == 0)
                reply_c("226 Directory list sent.\r\n");
        else
                reply_c("426 Connection closed, transfer aborted.\r\n");

        closedir(dir);
        closesocket(SS.data_sk);
        SS.data_sk = -1;
}

