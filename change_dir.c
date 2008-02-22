/*
 * User FTP Server
 * Author : C2H5OH
 * License: GPL v2
 *
 * change_dir.c - CWD command implementation.
 *
 * As we don't have root privileges, we can't execute the server within a
 * chroot() environment. Therefore, we must impose a couple of restrictions when
 * the client tries to change the current working directory:
 *
 * 1) The command argument (i.e. the path we are being requested for) cannot
 *    contain any reference to the self directory "." nor to the parent
 *    directory ".."
 *
 * 2) If the argument begins with '/', it will be understood as an absolute path
 *    and then will be concatenated with the base directory (not visible to the
 *    client).
 *
 * 3) In any other case the parameter will be interpreted as a relative path, so
 *    it will simply be appended to the current working path.
 *
 * The complete working directory is retrieved via getcwd(). Given that we want
 * to hide part of it (the base path), BdLen contains the number of characters
 * we have to skip. All this just to emulate chroot().
 *
 */

#include "uftps.h"

void change_dir (void)
{
        int err;

        if (!path_is_secure(S_arg)) {
                send_reply(S_cmd_sk, "550 Path is insecure.\r\n");
                return;
        }

        err = chdir(expanded_arg());

        if (err == -1) {
                send_reply(S_cmd_sk, "550 Could not change dir.\r\n");
                return;
        }

        send_reply(S_cmd_sk, "250 Directory changed.\r\n");
}

