
/***************************************************************************
 *  jobqueue.cpp - Camera Sync Download Job Queue
 *
 *  Created: Sat Jan 03 14:43:05 2015
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

#include "jobqueue.h"

#include <sqlite3.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


#define SQL_CREATE_TABLE						\
  "CREATE TABLE IF NOT EXISTS queue (\n"				\
  "  id     TEXT NOT NULL,\n"						\
  "  name   TEXT NOT NULL,\n"						\
  "  url    TEXT NOT NULL,\n"						\
  "  added  DATETIME DEFAULT CURRENT_TIMESTAMP,\n"			\
  "  tried  DATETIME DEFAULT (datetime('now', '-10 minutes')),\n"	\
  "  done   DATETIME DEFAULT NULL,\n"					\
  "  PRIMARY KEY (id)\n"						\
  ")"

#define SQL_INSERT_JOB							\
  "INSERT OR REPLACE INTO queue (id, name, url) "			\
  " VALUES (?, ?, ?)"

#define SQL_IS_JOB_DONE							\
  "SELECT id FROM queue WHERE id=? AND done IS NOT NULL"

#define SQL_HAS_JOB							\
  "SELECT id FROM queue WHERE id=?"

#define SQL_GET_NEXT_JOB						\
  "SELECT id, name, url FROM queue WHERE done IS NULL "			\
  "AND tried < datetime('now', '-5 minutes')"

#define SQL_REMOVE_JOB							\
  "DELETE FROM queue WHERE id=?"

#define SQL_MARK_JOB_DONE						\
  "UPDATE queue SET done=datetime('now') WHERE id=?"

#define SQL_REFRESH_JOB							\
  "UPDATE queue SET added=datetime('now') WHERE id=?"

#define SQL_DEFER_JOB							\
  "UPDATE queue SET tried=datetime('now') WHERE id=?"

#define SQL_EXPIRE_JOBS							\
  "DELETE FROM queue WHERE added < datetime('now', '-30 days')"

#define SQL_FLUSH_UNDONE_JOBS						\
  "DELETE FROM queue WHERE done IS NULL"


struct {
  sqlite3 *db;
} G_ =
  {
    .db = NULL
  };


JobQueueEntry *
jqe_new(const char *id, const char *name, const char *url)
{
  JobQueueEntry *jqe = (JobQueueEntry *)malloc(sizeof(JobQueueEntry));
  jqe->id   = strdup(id);
  jqe->name = strdup(name);
  jqe->url  = strdup(url);
  jqe->done = false;
  return jqe;
}

void
jqe_destroy(JobQueueEntry *jqe)
{
  free(jqe->id);
  free(jqe->name);
  free(jqe->url);
  free(jqe);
}


bool
jq_init(const char *db_file)
{
  if (sqlite3_open(db_file, &G_.db) != SQLITE_OK) {
    return false;
  }

  char *errmsg;
  if (sqlite3_exec(G_.db, SQL_CREATE_TABLE, NULL, NULL, &errmsg) != SQLITE_OK) {
    fprintf(stderr, "Failed to create tables: %s\n", sqlite3_errmsg(G_.db));
    sqlite3_close(G_.db);
    return false;
  }

  return true;
}


void
jq_finalize()
{
  sqlite3_close(G_.db);
  G_.db = NULL;
}

bool
jq_append(const char *id, const char *name, const char *url)
{
  sqlite3_stmt *stmt;
  const char   *tail;

  if ( sqlite3_prepare(G_.db, SQL_INSERT_JOB, -1, &stmt, &tail) != SQLITE_OK ) {
    fprintf(stderr, "Failed to prepare SQL_INSERT_JOB: %s\n", sqlite3_errmsg(G_.db));
    return false;
  }
  if ( (sqlite3_bind_text(stmt, 1, id, -1, NULL) != SQLITE_OK) ||
       (sqlite3_bind_text(stmt, 2, name, -1, NULL) != SQLITE_OK) ||
       (sqlite3_bind_text(stmt, 3, url, -1, NULL) != SQLITE_OK) )
  {
    fprintf(stderr, "Failed to bind for SQL_INSERT_JOB: %s\n", sqlite3_errmsg(G_.db));
    sqlite3_finalize(stmt);
    return false;
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    fprintf(stderr, "Failed to execute SQL_INSERT_JOB: %s\n", sqlite3_errmsg(G_.db));
    sqlite3_finalize(stmt);
    return false;
  }

  sqlite3_finalize(stmt);
  return true;
}

void
jq_expire()
{
  char *errmsg;
  sqlite3_exec(G_.db, SQL_EXPIRE_JOBS, NULL, NULL, &errmsg);
}

void
jq_flush_undone()
{
  char *errmsg;
  sqlite3_exec(G_.db, SQL_FLUSH_UNDONE_JOBS, NULL, NULL, &errmsg);
}


JobQueueEntry *
jq_get_next()
{
  sqlite3_stmt *stmt;
  if ( (sqlite3_prepare(G_.db, SQL_GET_NEXT_JOB, -1, &stmt, 0) != SQLITE_OK) || ! stmt ) {
    fprintf(stderr, "Failed to prepare SQL_GET_NEXT_JOB: %s\n", sqlite3_errmsg(G_.db));
    return NULL;
  }

  if (sqlite3_step(stmt) == SQLITE_ROW ) {
    JobQueueEntry *jeq =
      jqe_new(sqlite3_column_text(stmt, 0),
	      sqlite3_column_text(stmt, 1),
	      sqlite3_column_text(stmt, 2));

    jq_defer(jeq->id);

    sqlite3_finalize(stmt);
    return jeq;
  } else {
    printf("No job\n");
  }

  sqlite3_finalize(stmt);
  return NULL;
}

bool
jq_has_next()
{
  sqlite3_stmt *stmt;
  if ( (sqlite3_prepare(G_.db, SQL_GET_NEXT_JOB, -1, &stmt, 0) != SQLITE_OK) || ! stmt ) {
    fprintf(stderr, "Failed to prepare SQL_GET_NEXT_JOB (has_next): %s\n",
	    sqlite3_errmsg(G_.db));
    return false;
  }

  if (sqlite3_step(stmt) == SQLITE_ROW ) {
    sqlite3_finalize(stmt);
    return true;
  }

  sqlite3_finalize(stmt);
  return false;
}

void
jq_mark_done(const char *id)
{
  sqlite3_stmt *stmt;

  if ( sqlite3_prepare(G_.db, SQL_MARK_JOB_DONE, -1, &stmt, NULL) == SQLITE_OK ) {
    if (sqlite3_bind_text(stmt, 1, id, -1, NULL) == SQLITE_OK) {
      sqlite3_step(stmt);
    }
    sqlite3_finalize(stmt);  
  }
}

bool
jq_is_done(const char *id)
{
  sqlite3_stmt *stmt;

  if ( sqlite3_prepare(G_.db, SQL_IS_JOB_DONE, -1, &stmt, NULL) == SQLITE_OK ) {
    if (sqlite3_bind_text(stmt, 1, id, -1, NULL) == SQLITE_OK) {
      int rv = sqlite3_step(stmt);
      if (rv == SQLITE_ROW) {
	sqlite3_finalize(stmt);  
	return true;
      } else if (rv != SQLITE_DONE) { 
	fprintf(stderr, "Failed to execute SQL_IS_JOB_DONE: %s\n", sqlite3_errmsg(G_.db));
      }
    } else {
      fprintf(stderr, "Failed to bind for SQL_IS_JOB_DONE: %s\n", sqlite3_errmsg(G_.db));
    }
  } else {
    fprintf(stderr, "Failed to prepare SQL_IS_JOB_DONE: %s\n", sqlite3_errmsg(G_.db));
  }
  sqlite3_finalize(stmt);  
  return false;
}

bool
jq_has(const char *id)
{
  sqlite3_stmt *stmt;

  if ( sqlite3_prepare(G_.db, SQL_HAS_JOB, -1, &stmt, NULL) == SQLITE_OK ) {
    if (sqlite3_bind_text(stmt, 1, id, -1, NULL) == SQLITE_OK) {
      int rv = sqlite3_step(stmt);
      if (rv == SQLITE_ROW) {
	sqlite3_finalize(stmt);  
	return true;
      } else if (rv != SQLITE_DONE) { 
	fprintf(stderr, "Failed to execute SQL_HAS_JOB: %s\n", sqlite3_errmsg(G_.db));
      }
    } else {
      fprintf(stderr, "Failed to bind for SQL_HAS_JOB: %s\n", sqlite3_errmsg(G_.db));
    }
  } else {
    fprintf(stderr, "Failed to prepare SQL_HAS_JOB: %s\n", sqlite3_errmsg(G_.db));
  }
  sqlite3_finalize(stmt);  
  return false;
}

void
jq_remove(const char *id)
{
  sqlite3_stmt *stmt;

  if ( sqlite3_prepare(G_.db, SQL_REMOVE_JOB, -1, &stmt, NULL) == SQLITE_OK ) {
    if (sqlite3_bind_text(stmt, 1, id, -1, NULL) == SQLITE_OK) {
      if (sqlite3_step(stmt) != SQLITE_DONE) {
	fprintf(stderr, "Failed to execute SQL_REMOVE_JOB: %s\n", sqlite3_errmsg(G_.db));
      }
    } else {
      fprintf(stderr, "Failed to bind for SQL_REMOVE_JOB: %s\n", sqlite3_errmsg(G_.db));
    }
    sqlite3_finalize(stmt);  
  } else {
    fprintf(stderr, "Failed to prepare SQL_REMOVE_JOB: %s\n", sqlite3_errmsg(G_.db));
  }

}

void
jq_defer(const char *id)
{
  sqlite3_stmt *stmt;

  if ( sqlite3_prepare(G_.db, SQL_DEFER_JOB, -1, &stmt, NULL) == SQLITE_OK ) {
    if (sqlite3_bind_text(stmt, 1, id, -1, NULL) == SQLITE_OK) {
      if (sqlite3_step(stmt) != SQLITE_DONE) {
	fprintf(stderr, "Failed to execute SQL_DEFER_JOB: %s\n", sqlite3_errmsg(G_.db));
      }
    } else {
      fprintf(stderr, "Failed to bind for SQL_DEFER_JOB: %s\n", sqlite3_errmsg(G_.db));
    }
    sqlite3_finalize(stmt);  
  } else {
    fprintf(stderr, "Failed to prepare SQL_DEFER_JOB: %s\n", sqlite3_errmsg(G_.db));
  }

}

bool
jq_refresh(const char *id)
{
  sqlite3_stmt *stmt;

  if ( sqlite3_prepare(G_.db, SQL_REFRESH_JOB, -1, &stmt, NULL) == SQLITE_OK ) {
    if (sqlite3_bind_text(stmt, 1, id, -1, NULL) == SQLITE_OK) {
      if (sqlite3_step(stmt) == SQLITE_DONE) {
	int num_rows = sqlite3_changes(G_.db);
	sqlite3_finalize(stmt);  
	return (num_rows > 0);
      } else {
	fprintf(stderr, "Failed to execute SQL_REFRESH_JOB: %s\n", sqlite3_errmsg(G_.db));
      }
    } else {
      fprintf(stderr, "Failed to bind for SQL_REFRESH_JOB: %s\n", sqlite3_errmsg(G_.db));
    }
  } else {
    fprintf(stderr, "Failed to prepare SQL_REFRESH_JOB: %s\n", sqlite3_errmsg(G_.db));
  }
  sqlite3_finalize(stmt);  
  return false;
}


void
jq_refresh_or_append(const char *id, const char *name, const char *url)
{
  if (! jq_refresh(id)) {
    jq_append(id, name, url);
  }
}
