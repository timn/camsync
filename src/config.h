
/***************************************************************************
 *  config.h - Camera Sync - Configuration
 *
 *  Created: Sun Jan 04 13:14:11 2015
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

#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <stdbool.h>

typedef struct {
  int                  upnp_port;
  char               **interfaces;
  char                *output_dir;
  char                *camera_name;
  char                *config_file;
  bool                 daemonize;
  bool                 daemon_kill;
  char                *daemon_pidfile;
  unsigned int         conc_downloads;
  unsigned int         rebrowse_interval;
} CamSyncConfig;

extern CamSyncConfig C_;


bool config_init(int argc, char **argv);
void config_finalize();

#endif
