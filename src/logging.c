
/***************************************************************************
 *  logging.c - Camera Sync - Logging
 *
 *  Created: Sun Feb 22 19:58:55 2015
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

#include "logging.h"

#include <sys/time.h>
#include <stdio.h>
#include <time.h>
#include <stdarg.h>

static const char *c_normal = "\033[0;39m";
static const char *c_lightgray = "\033[2;37m";
static const char *c_brown = "\033[0;33m";
static const char *c_red = "\033[0;31m";

static LogLevel log_level = LL_DEBUG;

void
log_debug(const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  if (log_level <= LL_DEBUG ) {
    struct timeval now;
    struct tm  now_s;
    gettimeofday(&now, NULL);
    localtime_r(&now.tv_sec, &now_s);
    fprintf(stderr, "%s%02d:%02d:%02d.%06ld %s: ", c_lightgray, now_s.tm_hour,
	    now_s.tm_min, now_s.tm_sec, (long)now.tv_usec, component);
    vfprintf(stderr, format, arg);
    fprintf(stderr, "%s\n", c_normal);
  }
}


void
log_info(const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  if (log_level <= LL_INFO ) {
    struct timeval now;
    struct tm  now_s;
    gettimeofday(&now, NULL);
    localtime_r(&now.tv_sec, &now_s);
    fprintf(stderr, "%02d:%02d:%02d.%06ld %s: ", now_s.tm_hour, now_s.tm_min,
	    now_s.tm_sec, (long)now.tv_usec, component);
    vfprintf(stderr, format, arg);
    fprintf(stderr, "\n");
  }
}


void
log_warn(const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  if ( log_level <= LL_WARN ) {
    struct timeval now;
    struct tm  now_s;
    gettimeofday(&now, NULL);
    localtime_r(&now.tv_sec, &now_s);
    fprintf(stderr, "%s%02d:%02d:%02d.%06ld %s: ", c_brown, now_s.tm_hour,
	    now_s.tm_min, now_s.tm_sec, (long)now.tv_usec, component);
    vfprintf(stderr, format, arg);
    fprintf(stderr, "%s\n", c_normal);
  }
}


void
log_error(const char *component, const char *format, ...)
{
  va_list arg;
  va_start(arg, format);
  if ( log_level <= LL_ERROR ) {
    struct timeval now;
    struct tm  now_s;
    gettimeofday(&now, NULL);
    localtime_r(&now.tv_sec, &now_s);
    fprintf(stderr, "%s%02d:%02d:%02d.%06ld %s: ", c_red, now_s.tm_hour,
	    now_s.tm_min, now_s.tm_sec, (long)now.tv_usec, component);
    vfprintf(stderr, format, arg);
    fprintf(stderr, "%s\n", c_normal);
  }
}
