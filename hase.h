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
 * Hasefroch compatiblity adaptations.
 */

#define __MSVCRT_VERSION__ 0x0601
#include <sys/stat.h>
#include <process.h>
#include <time.h>
#include <io.h>
#include <fcntl.h>


struct stat
{
        struct __stat64  x;
};


#define st_size   x.st_size
#define st_mode   x.st_mode
#define st_mtime  x.st_mtime

#define S_ISDIR  _S_ISDIR
#define S_ISREG  _S_ISREG

#define O_RDONLY  _O_RDONLY

#define gmtime(t)  _gmtime64(t)

static inline int getpid (void)
{
        return _getpid();
}

static inline int lstat (const char *path, struct stat *st)
{
        return _stat64(path, &st->x);
}

static inline off_t lseek (int fd, off_t offset, int whence)
{
        return _lseeki64(fd, offset, whence);
}

static inline int read (int fd, void *buf, size_t count)
{
        return _read(fd, buf, count);
}

static inline int open (const char *pathname, int flags)
{
        return _open(pathname, flags | _O_BINARY | _O_SEQUENTIAL);
}

static inline int close (int fd)
{
        return _close(fd);
}

