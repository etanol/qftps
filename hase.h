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
 * Hasefroch compatiblity.  This header contains the rest of the junk (the
 * initial junk is at uftps.h) to compile the application with MinGW while
 * maintaining the UNIX look of the code.  Most of the wrappers are trivial, but
 * still was desirable to avoid having aliases for the underscore prefixed
 * names.
 *
 * However, the Hasefroch support is not complete.  The way WINVER is defined in
 * uftps.h prevents this program to be compiled or run in Hasefroch versions
 * prior to XP SP-1.
 *
 * NOTE: Inline functions are preferred over macros as some type checking is
 * allowed.  When this type checking is not appreciated, macros are used
 * instead.
 */

#define __MSVCRT_VERSION__ 0x0601
#include <sys/stat.h>
#include <process.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>


/*
 * File stat wrappers, with large file support.  Structures, fields and macros
 * and functions.  Hasefroch does not have symbolic links, so we do not care
 * about them.
 */
struct stat
{
        struct __stat64  x;
};

#define st_size   x.st_size
#define st_mode   x.st_mode
#define st_mtime  x.st_mtime

#define S_ISDIR   _S_ISDIR
#define S_ISREG   _S_ISREG

static inline int lstat (const char *path, struct stat *st)
{
        return _stat64(path, &st->x);
}


/*
 * File I/O wrappers.  No writing support as the server is not supposed to
 * modify the filesystem.
 */
#define O_RDONLY  _O_RDONLY

static inline int open (const char *pathname, int flags)
{
        return _open(pathname, flags | _O_BINARY | _O_SEQUENTIAL);
}


static inline off_t lseek (int fd, off_t offset, int whence)
{
        return _lseeki64(fd, offset, whence);
}


static inline int read (int fd, void *buf, size_t count)
{
        return _read(fd, buf, count);
}


static inline int close (int fd)
{
        return _close(fd);
}


/*
 * Miscellany wrappers.  In the case of gmtime, dealing with parameter types
 * (which obviously differ) is avoided by directly issuing the st_mtime field as
 * the time parameter.
 */
#define gmtime(t)  _gmtime64(t)

static inline int getpid (void)
{
        return _getpid();
}

