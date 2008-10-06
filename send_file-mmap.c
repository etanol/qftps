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
#  include <sys/mman.h>
#  include <unistd.h>
#endif

/*
 * Defining MAP_CHUNK_MEGS can control the maximum slice of file that can be
 * mapped at a time.  Recommended values are powers of two below 512.
 */
#if !defined(MAP_CHUNK_MEGS) || MAP_CHUNK_MEGS < 16
#  define MAP_CHUNK_MEGS  16
#endif

#define MAP_MAX_BYTES  (MAP_CHUNK_MEGS * 1024 * 1024)


/*
 * Cached values: system page size and some masks to extract bits.  Make sure
 * they are initialized to zero.  Does Hasefroch have BSS-like section in its
 * executables?
 */
static off_t  Page_Size = 0;
static off_t  File_Mask = 0;
static off_t  Page_Mask = 0;


/*
 * RETR implementation using file mappings to avoid some memory copies when
 * reading from a file.  This version used to be UNIX specific, but recent
 * Hasefroch support has been added.
 *
 * As the files can be large, it is unwise to map the whole file at once or the
 * virtual memory could be exhausted.  Even if that doesn't happen, the system
 * could become swappy.
 *
 * Therefore, only a pice of the file is mapped at once; not too large and not
 * too small to take advantage of the I/O benefits offered by the file mappings.
 * At each iteration of the send loop a chunk of the file is mapped, sent over
 * the data channel, and then unmapped to release the memory for the next
 * iteration.
 */
void send_file (void)
{
        int     f, e;
        void   *map;
        size_t  map_bytes;
        off_t   size, completed, file_offset, page_offset;
#ifdef __MINGW32__
        BOOL    ok;
        HANDLE  fm;  /* File Mapping */
#endif

        /* Retrieve page size if necessary, assuming a power of two value */
        if (Page_Size == 0)
        {
#ifdef __MINGW32__
                SYSTEM_INFO  si;

                GetSystemInfo(&si);
                Page_Size = si.dwPageSize;
#else
                Page_Size = sysconf(_SC_PAGESIZE);
#endif
                Page_Mask = Page_Size - 1;
                File_Mask = ~Page_Mask;
                debug("System page size is %d", (int) Page_Size);
        }

        completed      = SS.rest_offset;
        SS.rest_offset = 0;

        f = open_file(&size);
        if (f == -1)
                return;

#ifdef __MINGW32__
        /*
         * As always, in Hasefroch everyting is more complicated than necessary.
         * So to map files two steps are necessary: first create a mapping
         * object and then create a view, which is more like the UNIX mmap.
         */
        fm = CreateFileMapping((HANDLE) _get_osfhandle(f), NULL, PAGE_READONLY,
                               0, 0, NULL);
        if (fm == NULL)
        {
                error("Creating file mapping for %s", SS.aux);
                reply_c("451 Problems mapping file.\r\n");
                close(f);
                return;
        }
#endif

        e = open_data_channel();
        if (e == -1)
        {
                close(f);
                return;
        }

        reply_c("150 Sending file content.\r\n");
        debug("Initial offset is %lld", (long long) completed);

        /* Main transfer loop */
        while (completed < size)
        {
                /*
                 * File can only be mapped from offsets that are multiples of
                 * Page_Size.  Therefore, we need to step back a bit in the case
                 * of misalignment.
                 *
                 * And, obviously, we need to take care of how much amount of
                 * file we map; because we could run out of virtual memory.  So
                 * we have a hard limit on how much file can be mapped at a
                 * time.
                 */
                file_offset = completed & File_Mask;
                page_offset = completed & Page_Mask;
                map_bytes   = size - file_offset;
                if (map_bytes > MAP_MAX_BYTES)
                        map_bytes = MAP_MAX_BYTES;

#ifdef __MINGW32__
                map = MapViewOfFile(fm, FILE_MAP_READ,
                                    (file_offset >> 32) & 0x00FFFFFFFFLL,
                                     file_offset        & 0x00FFFFFFFFLL,
                                    map_bytes);
                if (map == NULL)
                {
                        e = -1;
                        break;
                }
#else
                map = mmap(NULL, map_bytes, PROT_READ, MAP_SHARED, f, file_offset);
                if (map == (void *) -1)
                {
                        e = -1;
                        break;
                }
                madvise(map, map_bytes, MADV_SEQUENTIAL);
#endif

                e = data_reply(map + page_offset, map_bytes - page_offset);
                if (e == -1)
                        break;

#ifdef __MINGW32__
                ok = UnmapViewOfFile(map);
                if (!ok)
                {
                        e = -1;
                        break;
                }
#else
                e = munmap(map, map_bytes);
                if (e == -1)
                        break;
#endif

                /*
                  * In the next iteration, "completed" will be properly aligned
                 * to a page boundary; provided that 1 << 20 (one megabyte
                 * offset) is aligned too.
                 */
                completed += map_bytes - page_offset;
        }

        if (e != -1)
                reply_c("226 File content sent.\r\n");
        else
        {
                error("Sending file %s with mmap", SS.arg);
                reply_c("426 Something happened, transfer aborted.\r\n");
        }

        close(f);
        closesocket(SS.data_sk);
        SS.data_sk = -1;
}

