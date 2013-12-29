/*
 * Quick FTP Server,  Share folders over FTP without being root.
 *
 * Public domain.  The author disclaims copyright to this source code.
 */

#include "qftps.h"
#ifdef __MINGW32__
#  include "hase.h"
#else
#  include <unistd.h>
#endif
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


/*
 * Logging utility.  Display a printf()-like formatted message (msg, args) with
 * the current PID and a severity label at the beginning of the line.  If
 * show_error is true, append a human readable version of the last system error.
 */
static void message (      int      show_error,
                     const char    *severity,
                     const char    *msg,
                           va_list  args)
{
        int  error_code;

        error_code = errno;

        /*
         * The value in SS.pid prevents calling getpid() for each message,
         * saving the expense of a system call.
         */
        printf("(%5d) [%-7s] ", SS.pid, severity);
        vprintf(msg, args);

        if (show_error && error_code != 0)
                printf(": %s.\n", strerror(error_code));
        else
                puts(".");
}


/*
 * Debug message.  Only enabled for debug compilations.
 */
#ifdef DEBUG
void debug (const char *msg, ...)
{
        va_list  args;

        va_start(args, msg);
        message(0, "DEBUG", msg, args);
        va_end(args);
}
#endif


/*
 * Informational message.
 */
void notice (const char *msg, ...)
{
        va_list  args;

        va_start(args, msg);
        message(0, "NOTICE", msg, args);
        va_end(args);
}


/*
 * Program error message.
 */
void warning (const char *msg, ...)
{
        va_list  args;

        va_start(args, msg);
        message(0, "WARNING", msg, args);
        va_end(args);
}


/*
 * System error message.  Displays system error information when the last error
 * code is not zero.
 */
void error (const char *msg, ...)
{
        va_list  args;

        va_start(args, msg);
        message(1, "ERROR", msg, args);
        va_end(args);
}


/*
 * Fatal error.  Displays a message similar to error() but also terminates the
 * current process.
 */
void fatal (const char *msg, ...)
{
        va_list  args;

        va_start(args, msg);
        message(1, "FATAL", msg, args);
        va_end(args);

        exit(EXIT_FAILURE);
}

