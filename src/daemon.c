
/***************************************************************************
 *  daemon.cpp - Camera Sync via DLNA - daemon support
 *
 *  Created: Fri Jan 09 23:57:32 2015
 *  Copyright  2015  Tim Niemueller [www.niemueller.de]
 ****************************************************************************/

/*  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version. A runtime exception applies to
 *  this software (see LICENSE.GPL file mentioned below for details).
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  Read the full text in the LICENSE.GPL file in the root directory.
 */

#include "daemon.h"
#include "config.h"

#include <libdaemon/daemon.h>
#include <glib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

static const char *
camsync_daemon_pid_file_proc()
{
  return C_.daemon_pidfile;
}


daemon_proc_type_t
daemonize(char *argv0, const char *pidfile)
{
#ifdef DAEMON_RESET_SIGS_AVAILABLE
  // Reset signal handlers
  if (daemon_reset_sigs(-1) < 0) {
    daemon_log(LOG_ERR, "Failed to reset all signal handlers: %s", strerror(errno));
    return FAIL;
  }
#endif

#ifdef DAEMON_UNBLOCK_SIGS_AVAILABLE
  /* Unblock signals */
  if (daemon_unblock_sigs(-1) < 0) {
    daemon_log(LOG_ERR, "Failed to unblock all signals: %s", strerror(errno));
    return FAIL;
  }
#endif

  /* Set indetification string for the daemon for both syslog and PID file */
  daemon_log_ident = daemon_ident_from_argv0(argv0);

  if (C_.daemon_pidfile) {
    daemon_pid_file_proc = camsync_daemon_pid_file_proc;

    if (g_file_test(C_.daemon_pidfile, G_FILE_TEST_EXISTS)) {
      daemon_log(LOG_ERR, "PID file already exists");
      return FAIL;
    }
  } else {
    daemon_pid_file_ident = daemon_log_ident;
  }

  pid_t pid;

  // Check that the daemon is not rung twice a the same time
  if ((pid = daemon_pid_file_is_running()) >= 0) {
    daemon_log(LOG_ERR, "Daemon already running on PID file %u", pid);
    return FAIL;
  }

  // Prepare for return value passing from the initialization
  // procedure of the daemon process
  if (daemon_retval_init() < 0) {
    daemon_log(LOG_ERR, "Failed to create pipe.");
    return FAIL;
  }

  // Do the fork
  if ((pid = daemon_fork()) < 0) {

    /* Exit on error */
    daemon_retval_done();
    return FAIL;
      
  } else if (pid) { // The parent
    int ret;

    /* Wait for 20 seconds for the return value passed from the daemon process */
    if ((ret = daemon_retval_wait(20)) < 0) {
      daemon_log(LOG_ERR, "Could not recieve return value "
		 "from daemon process: %s", strerror(errno));
      return FAIL;
    }

    return ret == 0 ? PARENT : FAIL;

    } else { /* The daemon */

#ifdef DAEMON_CLOSE_ALL_AVAILABLE
    /* Close FDs */
    if (daemon_close_all(-1) < 0) {
      daemon_log(LOG_ERR, "Failed to close all file descriptors: %s", strerror(errno));
      
      /* Send the error condition to the parent process */
      daemon_retval_send(1);
      goto finish;
    }
#endif

    /* Create the PID file */
    if (daemon_pid_file_create() < 0) {
      daemon_log(LOG_ERR, "Could not create PID file (%s).", strerror(errno));
      daemon_retval_send(2);
      goto finish;
    }

    /* Initialize signal handling */
    if (daemon_signal_init(SIGINT, SIGTERM, SIGQUIT, SIGHUP, 0) < 0) {
      daemon_log(LOG_ERR, "Could not register signal handlers (%s).", strerror(errno));
      daemon_retval_send(3);
      goto finish;
    }

    /* Send OK to parent process */
    daemon_retval_send(0);

    return DAEMON;

  finish:
    daemon_retval_send(255);
    daemon_signal_done();
    daemon_pid_file_remove();
    return FAIL;
  }
}


void
daemon_kill(char *argv0, const char *pidfile)
{
#ifdef DAEMON_RESET_SIGS_AVAILABLE
  // Reset signal handlers
  if (daemon_reset_sigs(-1) < 0) {
    daemon_log(LOG_ERR, "Failed to reset all signal handlers: %s", strerror(errno));
    return;
  }
#endif

#ifdef DAEMON_UNBLOCK_SIGS_AVAILABLE
  /* Unblock signals */
  if (daemon_unblock_sigs(-1) < 0) {
    daemon_log(LOG_ERR, "Failed to unblock all signals: %s", strerror(errno));
    return;
  }
#endif

  /* Set indetification string for the daemon for both syslog and PID file */
  daemon_log_ident = daemon_ident_from_argv0(argv0);

  if (C_.daemon_pidfile) {
    daemon_pid_file_proc = camsync_daemon_pid_file_proc;

    if (! g_file_test(C_.daemon_pidfile, G_FILE_TEST_EXISTS)) {
      daemon_log(LOG_ERR, "PID file does not exist");
      return;
    }
  } else {
    daemon_pid_file_ident = daemon_log_ident;
  }

#ifdef DAEMON_PID_FILE_KILL_WAIT_AVAILABLE
  if (daemon_pid_file_kill_wait(SIGINT, 5) < 0)
    daemon_log(LOG_WARNING, "Failed to kill daemon: %s", strerror(errno));
#endif

}
