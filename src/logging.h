
/***************************************************************************
 *  logging.h - Camera Sync - Logging
 *
 *  Created: Sun Feb 22 19:57:17 2015
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

#ifndef __LOGGING_H_
#define __LOGGING_H_

/** Log level.
 * Defines a level that can be used to determine the amount of output to
 * generate in loggers. The log levels are strictly ordered
 * (debug < info < warn < error < none) so loggers shall implement a
 * facility to set a minimum logging level. Messages below that minimum
 * log level shall be omitted.
 */
typedef enum {
  LL_DEBUG  = 0,	/**< debug output, relevant only when tracking down problems */
  LL_INFO   = 1,	/**< informational output about normal procedures */
  LL_WARN   = 2,	/**< warning, should be investigated but software still functions,
			 * an example is that something was requested that is not
			 * available and thus it is more likely a user error */
  LL_ERROR  = 4,	/**< error, may be recoverable (software still running) or not
			 * (software has to terminate). This shall be used if the error
			 * is a rare situation that should be investigated. */
  LL_NONE   = 8	/**< use this to disable log output */
} LogLevel;

void log_debug(const char *component, const char *format, ...);
void log_info(const char *component, const char *format, ...);
void log_warn(const char *component, const char *format, ...);
void log_error(const char *component, const char *format, ...);

#endif
