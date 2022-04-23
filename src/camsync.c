
/***************************************************************************
 *  camsync.cpp - Camera Sync via DLNA for EOS 70D
 *
 *  Created: Thu Jan 01 17:13:43 2015
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

#include "config.h"
#include "jobqueue.h"
#include "browse.h"
#include "download.h"
#include "daemon.h"
#include "logging.h"

#include <libgupnp/gupnp-context-manager.h>
#include <libgupnp/gupnp-control-point.h>
#include <libgupnp-av/gupnp-av.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <gmodule.h>
#include <glib/gi18n.h>
#include <glib-unix.h>

#define MEDIA_SERVER     "urn:schemas-upnp-org:device:MediaServer:1"

static struct {
  GMainLoop           *main_loop;
  GUPnPContextManager *context_manager;
  guint                source_download;
  GUPnPServiceProxy   *content_dir;
} G_ =
  {
    .main_loop = NULL,
    .context_manager = NULL,
    .content_dir = NULL
  };

static void
dms_proxy_available_cb(GUPnPControlPoint *cp,
		       GUPnPDeviceProxy  *proxy)
{
  GUPnPDeviceInfo *dev_info = GUPNP_DEVICE_INFO(proxy);

  char *name = g_strdup(gupnp_device_info_get_friendly_name(dev_info));
  g_strstrip(name);

  log_debug("main", "Server '%s' available(mfct %s, model %s, UDN %s)",
	    name,
	    gupnp_device_info_get_manufacturer(dev_info),
	    gupnp_device_info_get_model_name(dev_info),
	    gupnp_device_info_get_udn(dev_info));

  if (g_strcmp0(name, C_.camera_name) == 0) {
    log_info("GUPnP", "Found Camera '%s', browsing files", name);
    // get files
    G_.content_dir = get_content_dir(proxy);
    browse(G_.content_dir, "0", 0, MAX_BROWSE);
  }

  g_free(name);
}

static void
dms_proxy_unavailable_cb(GUPnPControlPoint *cp,
			 GUPnPDeviceProxy  *proxy)
{
  //printf("Server UNavailable\n");
  GUPnPDeviceInfo *dev_info = GUPNP_DEVICE_INFO(proxy);

  char *name = g_strdup(gupnp_device_info_get_friendly_name(dev_info));
  g_strstrip(name);

  if (g_strcmp0(name, C_.camera_name) == 0) {
    log_info("GUPnP", "Camera '%s' has left the network", name);
    rebrowse_abort();
    g_object_unref(G_.content_dir);
    G_.content_dir = NULL;
    jq_flush_undone();
  }
  g_free(name);
}

static void
on_context_available(GUPnPContextManager *context_manager,
		     GUPnPContext        *context,
		     gpointer             user_data)
{
  GUPnPControlPoint *dms_cp;

  dms_cp = gupnp_control_point_new(context, MEDIA_SERVER);

  g_signal_connect(dms_cp,
		   "device-proxy-available",
		   G_CALLBACK(dms_proxy_available_cb),
		   NULL);
  g_signal_connect(dms_cp,
		   "device-proxy-unavailable",
		   G_CALLBACK(dms_proxy_unavailable_cb),
		   NULL);

  gssdp_resource_browser_set_active(GSSDP_RESOURCE_BROWSER(dms_cp), TRUE);

  /* Let context manager take care of the control point life cycle */
  gupnp_context_manager_manage_control_point(context_manager, dms_cp);

  /* We don't need to keep our own references to the control points */
  g_object_unref(dms_cp);
}


static gboolean
init_upnp(int port)
{
  GUPnPWhiteList *white_list;

#if ! GLIB_CHECK_VERSION(2, 35, 0)
  g_type_init();
#endif

  G_.context_manager = gupnp_context_manager_create(port);
  g_assert(G_.context_manager != NULL);

  if (C_.interfaces != NULL) {
    white_list = gupnp_context_manager_get_white_list(G_.context_manager);
    gupnp_white_list_add_entryv(white_list, C_.interfaces);
    gupnp_white_list_set_enabled(white_list, TRUE);
  }

  g_signal_connect(G_.context_manager,
		   "context-available",
		   G_CALLBACK(on_context_available),
		   NULL);

  return TRUE;
}

static void
deinit_upnp(void)
{
  //printf("Tearing down UPnP\n");
  g_object_unref(G_.context_manager);
}

static gboolean
main_loop_quit(gpointer user_data)
{
  GMainLoop *main_loop =(GMainLoop *)user_data;
  g_main_loop_quit(main_loop);
  return FALSE;
}

gint
main(gint   argc, gchar *argv[])
{
  setlocale(LC_ALL, "");
  bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
  bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
  textdomain(GETTEXT_PACKAGE);

  if (! config_init(argc, argv)) {
    return -1;
  }

  if (C_.daemon_kill) {
    daemon_kill(argv[0], C_.daemon_pidfile);
    return 0;
  }
  if (C_.daemonize) {
    //printf("daemonize\n");
    switch (daemonize(argv[0], C_.daemon_pidfile)) {
    case FAIL:
      //printf("FAIL\n");
      return -5;
    case PARENT:
      //printf("PARENT\n");
      return 0;
    case DAEMON:
      //printf("DAEMON\n");
      break;
    }
  }

  log_info("main", "Camera:           %s", C_.camera_name);
  log_info("main", "Output directory: %s", C_.output_dir);
  log_info("main", "UPnP port:        %i", C_.upnp_port);
  log_info("main", "Config file:      %s",
	   C_.config_file ? C_.config_file : "none");

  if (! download_init()) {
    return -2;
  }

  if (!init_upnp(C_.upnp_port)) {
    return -3;
  }

  char *config_db =
    g_strdup_printf("%s/.state.db", C_.output_dir);

  if (! jq_init(config_db)) {
    log_error("main", "Failed to initialize job queue");
    g_free(config_db);
    return -4;
  }
  g_free(config_db);

  // Run GLib main loop
  G_.main_loop = g_main_loop_new(NULL, FALSE);
  g_unix_signal_add(SIGINT, main_loop_quit, G_.main_loop);
  g_unix_signal_add(SIGTERM, main_loop_quit, G_.main_loop);
  g_main_loop_run(G_.main_loop);
  g_main_loop_unref(G_.main_loop);

  deinit_upnp();
  download_finalize();
  jq_finalize();

  return 0;
}
