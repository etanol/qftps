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


int expand_arg (void)
{
        int  len = SS.cwd_len;

        strncpy(SS.aux, SS.cwd, len);

        if (SS.arg != NULL)
        {
                len = apply_path(SS.arg, SS.aux, SS.cwd_len);
                if (len == -1)
                {
                        reply_c("552 Path overflow.\r\n");
                        fatal("Path overflow");
                }
        }

        SS.arg = SS.aux;
        debug("Argument expanded to '%s'", SS.arg);
        return len;
}

