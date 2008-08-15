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

#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


static void message (      int      show_error,
                     const char    *severity,
                     const char    *msg,
                           va_list  args)
{
        int  error_code;

        error_code = errno;

        printf("(%5d) [%-7s] ", getpid(), severity);
        vprintf(msg, args);

        if (show_error && error_code != 0)
                printf(": %s.\n", strerror(error_code));
        else
                puts(".");
}


#ifdef DEBUG
void debug (const char *msg, ...)
{
        va_list  args;

        va_start(args, msg);
        message(0, "DEBUG", msg, args);
        va_end(args);
}
#endif


void notice (const char *msg, ...)
{
        va_list  args;

        va_start(args, msg);
        message(0, "DEBUG", msg, args);
        va_end(args);
}


void warning (const char *msg, ...)
{
        va_list  args;

        va_start(args, msg);
        message(0, "WARNING", msg, args);
        va_end(args);
}



void error (const char *msg, ...)
{
        va_list  args;

        va_start(args, msg);
        message(1, "ERROR", msg, args);
        va_end(args);
}


void fatal (const char *msg, ...)
{
        va_list  args;

        va_start(args, msg);
        message(1, "FATAL", msg, args);
        va_end(args);

        exit(EXIT_FAILURE);
}

