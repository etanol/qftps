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
 * Send a reply to the client over the control connection.  Not a complex
 * function at all, but necessary almost everywhere else.
 */
void reply (const char *str, int len)
{
        int  b;

        debug("Reply   : %.*s", len - 2, str);

        do {
                b = send(SS.control_sk, str, len, 0);
                if (b == -1)
                        fatal("Control channel output");

                str += b;
                len -= b;
        } while (len > 0);
}


/*
 * Send reply over the current data connection.  It succeeds (return value 0)
 * when all data has been transferred.  It fails otherwise (return value -1),
 * that is, either by an incomplete transfer or a system error.
 */
int data_reply (const char *data, int len)
{
        int  b, l = 0;

        while (l < len)
        {
                b = send(SS.data_sk, &data[l], len - l, 0);
                if (b <= 0)
                {
                        error("Sending listing data");
                        return -1;
                }

                l += b;
        }

        return 0;
}

