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
 * Session state variables.
 *
 * As each child process only attends to one client code doesn't need to be
 * thread safe.
 */

#include "uftps.h"

/*
 * Buffers
 */
char LineBuf[LINE_SIZE]; /* Incoming commands buffer */
char AuxBuf[LINE_SIZE];  /* Auxiliary buffer */

/*
 * chroot() emulation
 */
char *Basedir;
int   Basedir_len;        /* Size of the hidden working directory path */

/*
 * Session information (prefixed by "S_")
 */
int       S_cmd_sk;           /* Control channel */
int       S_data_sk;          /* Data channel */
int       S_passive_bind_sk;  /* Cached data socket for passive mode */
int       S_passive_mode;     /* Passive mode flag */
long long S_offset;           /* Last REST offset accepted */
char      S_passive_str[64];  /* Cached reply for PASV */
char     *S_arg;              /* Current command's argument */

