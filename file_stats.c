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
 * File status query.
 *
 * Here we implement some commands (SIZE, MDTM) that share the need of calling
 * stat() over a given file, no matter the fields looked up.  It's handy to
 * merge all of them here so safety checks are not replicated.
 */

#include "uftps.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <stdio.h>


void file_stats (int type)
{
        int         err;
        struct stat st;
        struct tm   t;

        if (!path_is_secure(S_arg)) {
                send_reply(S_cmd_sk, "550 Path is insecure.\r\n");
                return;
        }

        err = stat(expanded_arg(), &st);

        if (err == -1) {
                send_reply(S_cmd_sk, "550 Could not stat file.\r\n");
                return;
        }

        switch (type) {
        case 0: /* MDTM */
                gmtime_r(&(st.st_mtime), &t);
                snprintf(AuxBuf, LINE_SIZE,
                         "213 %4d%02d%02d%02d%02d%02d\r\n", t.tm_year + 1900,
                         t.tm_mon, t.tm_mday, t.tm_hour, t.tm_min, t.tm_sec);
                break;

        case 1: /* SIZE */
                snprintf(AuxBuf, LINE_SIZE, "213 %lld\r\n",
                         (long long) st.st_size);
        }

        send_reply(S_cmd_sk, AuxBuf);
}

