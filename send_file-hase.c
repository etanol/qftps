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
#include <limits.h>
#include <mswsock.h>
#include <fcntl.h>


/*
 * Hasefroch specific RETR command implementation.  It uses TransmitFile to
 * minimize memory copies and take more profit of the filesystem cache.  It is
 * equivalent to the Linux send_file/splice implementations.
 */
void send_file (void)
{
        int         f, e;
        off_t       size, completed;
        HANDLE      fh;
        DWORD       b, xx;
        OVERLAPPED  ovlp;
        BOOL        ok = TRUE;

        completed      = SS.rest_offset;
        SS.rest_offset = 0;

        f = open_file(&size);
        if (f == -1)
                return;
        fh = (HANDLE) _get_osfhandle(f);

        e = open_data_channel();
        if (e == -1)
        {
                _close(f);
                return;
        }

        reply_c("150 Sending file content.\r\n");
        debug("Initial offset is %I64d", completed);

        while (completed < size)
        {
                /*
                 * Instead of seeking the file, the file offset is specified
                 * using an OVERLAPPED structure.  This forces TrasnmitFile to
                 * behave in overlapped mode (non-blocking).
                 *
                 * As blocking behaviour is desired and TransmitFile with an
                 * offset returns immediately, an explicit wait is required.
                 * The wait function to use is WSAGetOverlappedResult with the
                 * wait flag enabled.
                 */
                memset(&ovlp, 0, sizeof(OVERLAPPED));
                ovlp.OffsetHigh = (completed >> 32) & 0x0000FFFFFFFFLL;
                ovlp.Offset     =  completed        & 0x0000FFFFFFFFLL;
                b  = (size - completed > INT_MAX ? INT_MAX : size - completed);

                ok = TransmitFile(SS.data_sk, fh, b, 0, &ovlp, NULL, 0);
                if (!ok && WSAGetLastError() != WSA_IO_PENDING)
                        break;

                ok = WSAGetOverlappedResult(SS.data_sk, &ovlp, &b, TRUE, &xx);
                if (!ok)
                        break;

                completed += b;
        }

        if (ok)
                reply_c("226 File content sent.\r\n");
        else
        {
                error("Sending file %s", SS.aux);
                reply_c("426 Something happened, transfer aborted.\r\n");
        }

        _close(f);
        closesocket(SS.data_sk);
        SS.data_sk = -1;
}

