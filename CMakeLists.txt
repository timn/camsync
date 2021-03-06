cmake_minimum_required(VERSION 2.4.5)
include(FindPkgConfig)
include(FindGettext)
#include(GNUInstallDirs)
#include(CheckIncludeFileCXX)

if(POLICY CMP0005)
  cmake_policy(SET CMP0005 OLD)
endif()

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

set(CMAKE_C_FLAGS "-std=gnu99 -g")

add_definitions(-DRESDIR='"${PROJECT_SOURCE_DIR}/res"' -DGETTEXT_PACKAGE='"camsync"' -DLOCALEDIR='"${CMAKE_INSTALL_FULL_LOCALEDIR}"')
pkg_check_modules(GLIB REQUIRED glib-2.0 gobject-2.0)
pkg_check_modules(GUPNP REQUIRED gupnp-1.0 gssdp-1.0 gupnp-av-1.0)
pkg_search_module(LIBSOUP REQUIRED libsoup-2.4 libsoup-2.2)
pkg_check_modules(SQLITE REQUIRED sqlite3)
pkg_check_modules(LIBDAEMON REQUIRED libdaemon)


include_directories(${GLIB_INCLUDE_DIRS} ${GUPNP_INCLUDE_DIRS} ${LIBSOUP_INCLUDE_DIRS} ${SQLITE_INCLUDE_DIRS} ${LIBDAEMON_INCLUDE_DIRS})
link_libraries(${GLIB_LIBRARIES} ${GUPNP_LIBRARIES} ${LIBSOUP_LIBRARIES} ${SQLITE_LIBRARIES} ${LIBDAEMON_LIBRARIES})
link_directories(${GLIB_LIBRARY_DIRS} ${GUPNP_LIBRARY_DIRS} ${LIBSOUP_LIBRARY_DIRS} ${SQLITE_LIBRARY_DIRS} ${LIBDAEMON_LIBRARY_DIRS})

add_executable(camsync src/camsync.c src/browse.c src/download.c src/jobqueue.c src/config.c src/daemon.c src/logging.c)

