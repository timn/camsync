
/***************************************************************************
 *  daemon.h - Camera Sync - daemon support
 *
 *  Created: Fri Jan 09 23:58:03 2015
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

#ifndef __DAEMON_H_
#define __DAEMON_H_

typedef enum {
  FAIL,
  PARENT,
  DAEMON
} daemon_proc_type_t;

daemon_proc_type_t daemonize(char *argv0, const char *pidfile);
void               daemon_kill(char *argv0, const char *pidfile);

#endif
