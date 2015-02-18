
/***************************************************************************
 *  browse.c - Camera Sync - UPnP/DLNA Browse
 *
 *  Created: Sun Jan 04 12:13:29 2015
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

#include "browse.h"
#include "download.h"
#include "jobqueue.h"

#define CONTENT_DIR      "urn:schemas-upnp-org:service:ContentDirectory"
#define RECHECK_INTERVAL 180

static struct {
  guint                timeout_rebrowse;
  guint                browse_active;
} G_ =
  {
    .timeout_rebrowse = 0,
    .browse_active = 0,
  };


typedef struct
{
  GUPnPServiceProxy *content_dir;

  gchar *id;

  guint32 starting_index;
} BrowseData;

typedef struct
{
  gchar *id;
  gchar *title;
} BrowseMetadataData;

static BrowseData *
browse_data_new(GUPnPServiceProxy *content_dir,
		const char        *id,
		guint32            starting_index)
{
  BrowseData *data;

  data = g_slice_new(BrowseData);
  data->content_dir = g_object_ref(content_dir);
  data->id = g_strdup(id);
  data->starting_index = starting_index;

  return data;
}

static void
browse_data_free(BrowseData *data)
{
  g_free(data->id);
  g_object_unref(data->content_dir);
  g_slice_free(BrowseData, data);
}

static BrowseMetadataData *
browse_metadata_data_new(const char  *id)
{
  BrowseMetadataData *data;

  data = g_slice_new(BrowseMetadataData);
  data->id = g_strdup(id);
  data->title = NULL;

  return data;
}

static void
browse_metadata_data_free(BrowseMetadataData *data)
{
  g_free(data->id);
  g_free(data->title);
  g_slice_free(BrowseMetadataData, data);
}

GUPnPServiceProxy *
get_content_dir(GUPnPDeviceProxy *proxy)
{
  GUPnPDeviceInfo  *info;
  GUPnPServiceInfo *content_dir;

  info = GUPNP_DEVICE_INFO(proxy);
  content_dir = gupnp_device_info_get_service(info, CONTENT_DIR);
  return GUPNP_SERVICE_PROXY(content_dir);
}


void
rebrowse_abort()
{
  if (G_.timeout_rebrowse != 0) {
    GSource *timeout_source = g_main_context_find_source_by_id(NULL, G_.timeout_rebrowse);
    if (timeout_source) {
      g_source_destroy(timeout_source);
    }
  }
}

static gboolean
rebrowse(gpointer user_data)
{
  if (G_.browse_active) {
    printf("WARNING: browse active\n");
    return G_SOURCE_CONTINUE;
  }

  GUPnPServiceProxy *content_dir = (GUPnPServiceProxy *)user_data;

  if (content_dir) {
    browse(content_dir, "0", 0, MAX_BROWSE);
  }

  g_object_unref(content_dir);

  return G_SOURCE_REMOVE;
}

static void
browse_completed(GUPnPServiceProxy *content_dir)
{
  if (G_.browse_active == 0) {
    download_start();

    g_object_ref(content_dir);
    G_.timeout_rebrowse =
      g_timeout_add_seconds(RECHECK_INTERVAL, rebrowse, content_dir);
  }
}


static void
browse_didl_object_available(GUPnPDIDLLiteParser *parser,
			     GUPnPDIDLLiteObject *object,
			     gpointer             user_data)
{
  BrowseData   *browse_data;

  browse_data = (BrowseData *) user_data;

  const char *udn   = gupnp_service_info_get_udn
    (GUPNP_SERVICE_INFO(browse_data->content_dir));
  const char *id    = gupnp_didl_lite_object_get_id(object);
  const char *title = gupnp_didl_lite_object_get_title(object);
  gboolean    is_container = GUPNP_IS_DIDL_LITE_CONTAINER (object);

  //printf("ID: %s  Title: %s  UDN: %s  Cont: %s\n", id, title, udn,
  //	 is_container ? "YES" : "NO");

  if (is_container) {
    GUPnPDIDLLiteContainer *container;
    container = GUPNP_DIDL_LITE_CONTAINER(object);
    gint child_count =
      gupnp_didl_lite_container_get_child_count(container);
    //printf("Child Count %i\n", child_count);
    if (child_count != 0) {
      //printf("Recursing into %s\n", title);
      browse(browse_data->content_dir, id, 0, MAX_BROWSE);
      //} else {
      //printf("NOT, recursing into %s, no children\n", title);
    }
  } else {
    if (! jq_has(id)) {
      printf("Get metadata for new item %s\n", title);
      browse_metadata(browse_data->content_dir, id);
    } else {
      printf("Already got item %s\n", title);
    }
  }
}

static void
browse_cb(GUPnPServiceProxy       *content_dir,
	  GUPnPServiceProxyAction *action,
	  gpointer                 user_data)
{
  BrowseData *data;
  char       *didl_xml;
  guint32     number_returned;
  guint32     total_matches;
  GError     *error;

  data =(BrowseData *) user_data;
  didl_xml = NULL;
  error = NULL;

  G_.browse_active -= 1;

  gupnp_service_proxy_end_action
   (content_dir, action, &error,
     /* OUT args */
     "Result",         G_TYPE_STRING, &didl_xml,
     "NumberReturned", G_TYPE_UINT,   &number_returned,
     "TotalMatches",   G_TYPE_UINT,   &total_matches,
     NULL);

  if (didl_xml) {
    GUPnPDIDLLiteParser *parser;
    gint32              remaining;
    gint32              batch_size;
    GError              *error;

    error = NULL;
    parser = gupnp_didl_lite_parser_new();

    g_signal_connect(parser,
		     "object-available",
		     G_CALLBACK(browse_didl_object_available),
		     data);

    // Only try to parse DIDL if server claims that there was a result
    if (number_returned > 0) {
      if (!gupnp_didl_lite_parser_parse_didl(parser, didl_xml, &error)) {
	g_warning("Error while browsing %s: %s",
		  data->id, error->message);
	g_error_free(error);
      }
    }

    g_object_unref(parser);
    g_free(didl_xml);

    data->starting_index += number_returned;

    /* See if we have more objects to get */
    remaining = total_matches - data->starting_index;

    /* Keep browsing till we get each and every object */
    if ((remaining > 0 || total_matches == 0) && number_returned != 0) {
      if (remaining > 0)
	batch_size = MIN(remaining, MAX_BROWSE);
      else
	batch_size = MAX_BROWSE;
      
      browse(data->content_dir, data->id, data->starting_index, batch_size);
    } else {
      // done browsing
      browse_completed(data->content_dir);
    }
  } else if (error) {
    GUPnPServiceInfo *info;

    info = GUPNP_SERVICE_INFO(content_dir);
    g_warning("Failed to browse '%s': %s",
	       gupnp_service_info_get_location(info),
	       error->message);

    g_error_free(error);
  }

  browse_data_free(data);
}


static void
browse_metadata_didl_item_available (GUPnPDIDLLiteParser *parser,
				     GUPnPDIDLLiteObject *object,
				     gpointer             user_data)
{
  BrowseMetadataData *data = (BrowseMetadataData *) user_data;

  GList *resources = gupnp_didl_lite_object_get_resources(object);
  data->title = g_strdup(gupnp_didl_lite_object_get_title(object));

  GList *r;
  for (r = resources; r; r = r->next) {
    GUPnPDIDLLiteResource *res = (GUPnPDIDLLiteResource *)r->data;

    GUPnPProtocolInfo *protinf = gupnp_didl_lite_resource_get_protocol_info(res);
    if (protinf) {
      const char *dlna_profile =
	gupnp_protocol_info_get_dlna_profile(protinf);
      if (dlna_profile == NULL) {
	printf("Resource %s (%s): %s\n", data->title, data->id,
	       gupnp_didl_lite_resource_get_uri(res));

	// jq_append would overwrite potentially existing entries,
	// but this would also clear the completion flag and we would
	// re-download files. Hence first try to refresh, and only then add.
	jq_refresh_or_append(data->id, data->title, gupnp_didl_lite_resource_get_uri(res));
	//get_url(data->id, gupnp_didl_lite_resource_get_uri(res), ".", data->title);
      }	
    }
  }

  g_list_foreach(resources, (GFunc)g_object_unref, NULL);
  g_list_free(resources);
}


static void
browse_metadata_cb(GUPnPServiceProxy       *content_dir,
		   GUPnPServiceProxyAction *action,
		   gpointer                 user_data)
{
  BrowseMetadataData *data;
  char               *metadata;
  GError             *error;

  data = (BrowseMetadataData *) user_data;

  metadata = NULL;
  error = NULL;

  gupnp_service_proxy_end_action(content_dir,
				 action, &error,
				  // OUT args
				  "Result", G_TYPE_STRING, &metadata,
				  NULL);

  if (metadata) {
    // process meta data

    GUPnPDIDLLiteParser   *parser;
    GError                *error;

    parser = gupnp_didl_lite_parser_new();
    error = NULL;

    g_signal_connect(parser,
		     "item-available",
		     G_CALLBACK(browse_metadata_didl_item_available), user_data);

    gupnp_didl_lite_parser_parse_didl(parser, metadata, &error);
    if (error) {
      g_warning("Failed to parse metadata: %s\n", error->message);
      g_error_free(error);
    }

    g_object_unref(parser);
    g_free(metadata);
  } else if (error) {
    g_warning("Failed to get metadata for '%s': %s",
	      data->id, error->message);
    g_error_free(error);
  }

  download_start();
  
  browse_metadata_data_free(data);
  g_object_unref(content_dir);
}


void
browse(GUPnPServiceProxy *content_dir,
       const char        *container_id,
       guint32            starting_index,
       guint32            requested_count)
{
  BrowseData *data;
  data = browse_data_new(content_dir, container_id, starting_index);

  G_.browse_active += 1;

  gupnp_service_proxy_begin_action
   (content_dir, "Browse", browse_cb, data,
     /* IN args */
     "ObjectID",       G_TYPE_STRING, container_id,
     "BrowseFlag",     G_TYPE_STRING, "BrowseDirectChildren",
     "Filter",         G_TYPE_STRING, "@childCount",
     "StartingIndex",  G_TYPE_UINT,   starting_index,
     "RequestedCount", G_TYPE_UINT,   requested_count,
     "SortCriteria",   G_TYPE_STRING, "",
     NULL);
}

void
browse_metadata(GUPnPServiceProxy *content_dir,
		const char        *id)
{
  BrowseMetadataData *data;

  data = browse_metadata_data_new(id);

  gupnp_service_proxy_begin_action
    (g_object_ref (content_dir),
     "Browse", browse_metadata_cb, data,
     // IN args
     "ObjectID",       G_TYPE_STRING, data->id,
     "BrowseFlag",     G_TYPE_STRING, "BrowseMetadata",
     "Filter",         G_TYPE_STRING, "*",
     "StartingIndex",  G_TYPE_UINT,   0,
     "RequestedCount", G_TYPE_UINT,   0,
     "SortCriteria",   G_TYPE_STRING, "",
     NULL);
}
