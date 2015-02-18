
/***************************************************************************
 *  browse.h - Camera Sync - UPnP/DLNA Browse
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

#ifndef __BROWSE_H_
#define __BROWSE_H_

#include <libgupnp/gupnp-control-point.h>
#include <libgupnp-av/gupnp-av.h>

#define MAX_BROWSE       50

GUPnPServiceProxy *
get_content_dir(GUPnPDeviceProxy *proxy);

void
browse(GUPnPServiceProxy *content_dir,
       const char        *container_id,
       guint32            starting_index,
       guint32            requested_count);

void
browse_metadata(GUPnPServiceProxy *content_dir,
		const char        *id);


void
rebrowse_abort();

#endif
