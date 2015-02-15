
/***************************************************************************
 *  config.c - Camera Sync - Configuration
 *
 *  Created: Sun Jan 04 21:55:36 2015
 *  Copyright  2015  Tim Niemueller [www.niemueller.de]
 ***************************************************************************/

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

#include "config.h"

#include <stdlib.h>
#include <glib.h>
#include <glib/gi18n.h>

CamSyncConfig C_ =
  {
    .upnp_port      = 0,
    .interfaces     = NULL,
    .output_dir     = NULL,
    .camera_name    = NULL,
    .config_file    = NULL,
    .daemonize      = false,
    .daemon_kill    = false,
    .daemon_pidfile = NULL,
    .conc_downloads = 0
  };


static gboolean
config_daemon_cb(const gchar *option_name, const gchar *value,
			 gpointer data, GError **error)
{
  if (strcmp(option_name, "--daemon") == 0 || strcmp(option_name, "-d") == 0) {
    C_.daemonize = true;
  } else if (strcmp(option_name, "--daemon-kill") == 0 || strcmp(option_name, "-k") == 0) {
    C_.daemon_kill = true;
  }
  if (value) {
    C_.daemon_pidfile = g_strdup(value);
  }
  return TRUE;
}

static GOptionEntry config_entries[] =
{
  { "config", 'C', 0, G_OPTION_ARG_STRING, &C_.config_file,
    N_("Configuration file"), "FILE" },
  { "port", 'p', 0, G_OPTION_ARG_INT, &C_.upnp_port,
    N_("Network PORT to use for UPnP"), "PORT" },
  { "interface", 'i', 0, G_OPTION_ARG_STRING_ARRAY, &C_.interfaces,
    N_("Network interfaces to use for UPnP communication"), "INTERFACE" },
  { "camera", 'c', 0, G_OPTION_ARG_STRING, &C_.camera_name,
    N_("Name of camera to query (default Canon EOS 70D)"), "CAMERA" },
  { "outdir", 'o', 0, G_OPTION_ARG_STRING, &C_.output_dir,
    N_("Directory where to store downloaded images (must already exist)"), "DIR" },
  { "daemon", 'd', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
    config_daemon_cb, N_("Run in background as a daemon"), "[PIDFILE]" },
  { "daemon-kill", 'k', G_OPTION_FLAG_OPTIONAL_ARG, G_OPTION_ARG_CALLBACK,
    config_daemon_cb, N_("Kill camsync daemon running in the background"), "[PIDFILE]" },
  { "conc-downloads", 'D', 0, G_OPTION_ARG_INT, &C_.conc_downloads,
    N_("Maximum number of concurrent downloads"), "N" },
  { NULL }
};


bool
config_init(int argc, char **argv)
{
  GError *error = NULL;

  GOptionContext *context =
    g_option_context_new(_("- Download images from a camera via DLNA"));
  g_option_context_add_main_entries(context, config_entries, GETTEXT_PACKAGE);

  if (!g_option_context_parse(context, &argc, &argv, &error)) {
    g_print(_("Could not parse options: %s\n"), error->message);
    
    g_option_context_free(context);
    return false;
  }

  if (C_.config_file != NULL) {
    // read config file
    GKeyFile *kf = g_key_file_new();
    GError *error;
    if (g_key_file_load_from_file(kf, C_.config_file, G_KEY_FILE_NONE, &error)) {
      if (C_.interfaces == NULL) {
	gsize length;
	C_.interfaces =
	  g_key_file_get_string_list(kf, "network", "interfaces", &length, NULL);
      }
      if (C_.upnp_port == 0) {
	C_.upnp_port = g_key_file_get_integer(kf, "network", "upnp-port", NULL);
      }
      if (C_.output_dir == NULL) {
	C_.output_dir = g_key_file_get_string(kf, "general", "output-directory", NULL);
      }
      if (C_.camera_name == NULL) {
	C_.camera_name = g_key_file_get_string(kf, "general", "camera", NULL);
      }
      if (! C_.daemonize) {
	C_.daemonize = g_key_file_get_boolean(kf, "general", "daemonize", NULL);
	C_.daemon_pidfile = g_key_file_get_string(kf, "general", "pidfile", NULL);
      }
      // daemon_kill cannot be set in config file
      if (C_.conc_downloads == 0) {
	C_.conc_downloads = g_key_file_get_integer(kf, "general",
						   "concurrent-downloads", NULL);
      }

    } else {
      g_print(_("Could not read config file: %s\n"), error->message);
    }
  }

  if (C_.output_dir == NULL)   C_.output_dir = g_get_current_dir();
  if (C_.camera_name == NULL)  C_.camera_name = g_strdup("Canon EOS 70D");

  g_option_context_free(context);
  return true;
}

void config_finalize()
{
  g_strfreev(C_.interfaces);
  g_free(C_.output_dir);
  g_free(C_.camera_name);
  g_free(C_.config_file);
}

