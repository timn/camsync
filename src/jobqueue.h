
/***************************************************************************
 *  jobqueue.h - Camera Sync Download Job Queue
 *
 *  Created: Sat Jan 03 14:52:59 2015
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

#ifndef __JOBQUEUE_H_
#define __JOBQUEUE_H_

#include <stdbool.h>

typedef struct {
  char *id;
  char *name;
  char *url;
  bool  done;
} JobQueueEntry;

JobQueueEntry *   jqe_new(const char *id, const char *name, const char *url);
void              jqe_destroy(JobQueueEntry *jqe);

bool              jq_init(const char *db_file);
void              jq_finalize();

bool              jq_append(const char *id, const char *name, const char *url);
bool              jq_has(const char *id);
void              jq_expire();
bool              jq_has_next();
JobQueueEntry *   jq_get_next();
bool              jq_is_done(const char *id);
void              jq_mark_done(const char *id);
bool              jq_refresh(const char *id);
void              jq_remove(const char *id);
void              jq_flush_undone();
void              jq_defer(const char *id);

void              jq_refresh_or_append(const char *id, const char *name, const char *url);

#endif
