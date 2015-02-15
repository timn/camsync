
/***************************************************************************
 *  download.c - Camera Sync - Download Images
 *
 *  Created: Sun Jan 04 12:44:49 2015
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

#include "download.h"
#include "jobqueue.h"
#include "config.h"

#include <glib.h>
#include <glib/gi18n.h>
#include <stdio.h>
#include <errno.h>
#include <libsoup/soup.h>


static struct {
  SoupSession         *soup_session;
  guint                source_download;
  guint                active_downloads;
} G_ =
  {
    .soup_session = NULL,
    .source_download = 0,
    .active_downloads = 0,
  };

typedef struct
{
  gchar *id;
  gchar *outfile_dir;
  gchar *outfile_name;
  gchar *url;
} DownloadData;


static DownloadData *
download_data_new(const char *id,
		  const char *outfile_dir,
		  const char *outfile_name,
		  const char *url)
{
  DownloadData *data;

  data = g_slice_new(DownloadData);
  data->id = g_strdup(id);
  data->outfile_dir = g_strdup(outfile_dir);
  data->outfile_name = g_strdup(outfile_name);
  data->url = g_strdup(url);

  return data;
}

static bool download_next();


static void
download_data_free(DownloadData *data)
{
  g_free(data->id);
  g_free(data->outfile_dir);
  g_free(data->outfile_name);
  g_free(data->url);
  g_slice_free(DownloadData, data);
}


bool
download_init()
{
  G_.soup_session =
    soup_session_new_with_options(SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_CONTENT_DECODER,
				  SOUP_SESSION_ADD_FEATURE_BY_TYPE, SOUP_TYPE_COOKIE_JAR,
				  SOUP_SESSION_MAX_CONNS, C_.conc_downloads,
				  SOUP_SESSION_MAX_CONNS_PER_HOST, C_.conc_downloads,
				  SOUP_SESSION_USER_AGENT, "camsync ",
				  SOUP_SESSION_ACCEPT_LANGUAGE_AUTO, TRUE,
				  NULL);

  /*
  SoupLogger *logger;
  logger = soup_logger_new(SOUP_LOGGER_LOG_HEADERS, -1);
  soup_session_add_feature(G_.soup_session, SOUP_SESSION_FEATURE(logger));
  g_object_unref(logger);
  */

  return (G_.soup_session != NULL);
}

void
download_finalize()
{
  g_object_unref(G_.soup_session);
}

static gboolean download(gpointer user_data);

void
download_start()
{
  if (G_.source_download != 0)  return;

  if (G_.active_downloads < C_.conc_downloads) {
    printf("Starting download\n");
    G_.source_download = g_idle_add(download, NULL);
  }
}


static void
get_url_finished (SoupSession *session, SoupMessage *msg, gpointer user_data)
{
  DownloadData *data = (DownloadData *)user_data;

  //name = soup_message_get_uri(msg)->path;

  char *tmpfile_name, *file_name;

  printf("Download of %s completed\n", data->outfile_name);

  G_.active_downloads -= 1;

  tmpfile_name = g_strdup_printf("%s/.%s.part", data->outfile_dir, data->outfile_name);
  file_name    = g_strdup_printf("%s/%s", data->outfile_dir, data->outfile_name);

  g_assert(tmpfile_name != NULL);
  g_assert(file_name != NULL);

  if (SOUP_STATUS_IS_SUCCESSFUL(msg->status_code)) {
    FILE *output_file = fopen(tmpfile_name, "w");
    if (!output_file) {
      g_printerr ("Error trying to create file %s.\n", tmpfile_name);
    } else {
      fwrite (msg->response_body->data,
	      1, msg->response_body->length,
	      output_file);

      fclose(output_file);

      printf("Renaming %s -> %s\n", tmpfile_name, file_name);
      int rename_status = rename(tmpfile_name, file_name);
      if (rename_status != 0) {
	g_printerr("Failed to move temporary file: %s\n", strerror(errno));
      }

      jq_mark_done(data->id);
    }
  } else {
    // the file will be deferred for a few minutes to retry later
    g_printerr("Failed to get %s: %s\n", data->url, msg->reason_phrase);
  }

  g_free(tmpfile_name);
  g_free(file_name);

  download_next();
}


static void
get_url(const char *id, const char *url, const char *outfile_dir, const char *outfile_name)
{
  SoupMessage *msg;
  const char *header;
  FILE *output_file = NULL;

  G_.active_downloads += 1;

  msg = soup_message_new("GET", url);

  DownloadData *data;
  data = download_data_new(id, outfile_dir, outfile_name, url);

  g_object_ref(msg);
  printf("Queueing SOUP message\n");
  soup_session_queue_message(G_.soup_session, msg, get_url_finished, data);
}


static bool
download_next()
{
  if (G_.active_downloads < C_.conc_downloads) {
    JobQueueEntry *jqe = jq_get_next();
    if (jqe) {
      printf("Fetching %s (%s)\n", jqe->name, jqe->url);
      get_url(jqe->id, jqe->url, C_.output_dir, jqe->name);
      jqe_destroy(jqe);
    }
    return true;
  } else {
    return false;
  }
}

static gboolean
download(gpointer user_data)
{
  if (download_next() && jq_has_next()) {
    printf("Continuing\n");
    return G_SOURCE_CONTINUE;
  } else {
    printf("Stopping download\n");
    G_.source_download = 0;
    return G_SOURCE_REMOVE;
  }
}

