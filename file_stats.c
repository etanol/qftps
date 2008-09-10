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
#  include <time.h>
#endif
#include <stdio.h>


/*
 * File status query.
 *
 * Here we implement some commands (SIZE, MDTM) that share the need of calling
 * stat() over a given file, no matter the fields looked up.  It's handy to
 * merge all of them here so safety checks are not replicated.
 */
void file_stats (int type)
{
        int          l = 0, e;
        struct stat  s;
        struct tm   *t;

        if (SS.arg == NULL)
        {
                warning("Stat type %d without argument", type);
                reply_c("501 Argument required.\r\n");
                return;
        }
        expand_arg();

        e = lstat(SS.arg, &s);
        if (e == -1 || (!S_ISREG(s.st_mode) && !S_ISDIR(s.st_mode)))
        {
                if (e == -1)
                        error("Stating file %s", SS.arg);
                else
                        warning("Path %s is not file nor directory", SS.arg);
                reply_c("550 Could not stat file.\r\n");
                return;
        }

        switch (type)
        {
        case 0: /* MDTM */
                t = gmtime(&s.st_mtime);
                if (t == NULL)
                {
                        error("Converting time of %s", SS.arg);
                        reply_c("550 Error converting time.\r\n");
                        return;
                }

                l = snprintf(SS.aux, LINE_SIZE, "213 %4d%02d%02d%02d%02d%02d\r\n",
                             t->tm_year + 1900, t->tm_mon, t->tm_mday,
                             t->tm_hour, t->tm_min, t->tm_sec);
                break;

        case 1: /* SIZE */
#ifdef __MINGW32__
                l = snprintf(SS.aux, LINE_SIZE, "213 %I64d\r\n", s.st_size);
#else
                l = snprintf(SS.aux, LINE_SIZE, "213 %lld\r\n",
                             (long long) s.st_size);
#endif
        }

        reply(SS.aux, l);
}

