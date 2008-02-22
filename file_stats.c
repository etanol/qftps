/*
 * User FTP Server
 * Author : C2H5OH
 * License: GPL v2
 *
 * file_stats.c - File status query.
 *
 * Here we implement some commands (SIZE, MDTM) that share the need of calling
 * stat() over a given file, no matter the fields looked up. It's handy to merge
 * all of them here so safety checks are not replicated.
 */

#include "uftps.h"

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

