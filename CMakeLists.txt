project(canvas)

cmake_minimum_required(VERSION 3.1)

# Optional image formats
option(HAVE_GIF, "Enable GIF support" OFF)
option(HAVE_SVG, "Enable SVG support" OFF)
option(HAVE_JPEG, "Enable JPEG support" OFF)

# Latest node requires C++11
set(CMAKE_CXX_STANDARD 11) 
set(CMAKE_STANDARD_REQUIRED TRUE)

include_directories(node_modules/nan)
include_directories(/home/caleb/.nvm/versions/node/v10.0.0/include/node)

# Ask pkg-config for libraries (homebrew and most linux distros)
find_package(PkgConfig)

# Find Pango
pkg_check_modules(PC_PANGO pango)
find_library(PANGO_LIBRARIESNAMES pango-1.0 HINTS ${PC_PANGO_LIBDIR} ${PC_PANGO_LIBRARY_DIRS})
find_path(PANGO_INCLUDE_DIRS pango/pango.h HINTS ${PC_PANGO_INCLUDEDIR} ${PC_PANGO_INCLUDE_DIRS})
if (NOT PANGO_LIBRARIESNAMES OR NOT PANGO_INCLUDE_DIRS)
  message(FATAL_ERROR "Couldn't find Pango on the sytem or via pkg-config!")
endif ()
include_directories(${PANGO_INCLUDE_DIRS})
list(APPEND THIRD_PARTY_LIBS ${PANGO_LIBRARIESNAMES})

# Find Cairo
pkg_check_modules(PC_CAIRO cairo)
find_library(CAIRO_LIBRARIESNAMES cairo HINTS ${PC_CAIRO_LIBDIR} ${PC_CAIRO_LIBRARY_DIRS})
find_path(CAIRO_INCLUDE_DIRS cairo.h HINTS ${PC_CAIRO_INCLUDEDIR} ${PC_CAIRO_INCLUDE_DIRS})
if (NOT CAIRO_LIBRARIESNAMES OR NOT CAIRO_INCLUDE_DIRS)
  message(FATAL_ERROR "Couldn't find Cairo on the sytem or via pkg-config!")
endif ()
include_directories(${CAIRO_INCLUDE_DIRS})
list(APPEND THIRD_PARTY_LIBS ${CAIRO_LIBRARIESNAMES})

# Find Glib
pkg_check_modules(PC_GLIB glib-2.0)
find_library(GLIB_LIBRARIESNAMES glib-2.0 HINTS ${PC_GLIB_LIBDIR} ${PC_GLIB_LIBRARY_DIRS})
find_path(GLIB_INCLUDE_DIRS glib.h HINTS ${PC_GLIB_INCLUDEDIR} ${PC_GLIB_INCLUDE_DIRS})
find_path(GLIB_INCLUDE_CONFIG_DIRS glibconfig.h HINTS ${PC_GLIB_INCLUDEDIR} ${PC_GLIB_INCLUDE_DIRS})
if (NOT GLIB_LIBRARIESNAMES OR NOT GLIB_INCLUDE_DIRS OR NOT GLIB_INCLUDE_CONFIG_DIRS)
  message(FATAL_ERROR "Couldn't find GLib on the sytem or via pkg-config!")
endif ()
include_directories(${GLIB_INCLUDE_DIRS})
include_directories(${GLIB_INCLUDE_CONFIG_DIRS})
list(APPEND THIRD_PARTY_LIBS ${GLIB_LIBRARIESNAMES})

# Find pangocairo
pkg_check_modules(PC_PANGOCAIRO pangocairo)
find_library(PANGOCAIRO_LIBRARIESNAMES pangocairo-1.0 HINTS ${PC_PANGOCAIRO_LIBDIR} ${PC_PANGOCAIRO_LIBRARY_DIRS})
find_path(PANGOCAIRO_INCLUDE_DIRS pango/pangocairo.h HINTS ${PC_PANGOCAIRO_INCLUDEDIR} ${PC_PANGOCAIRO_INCLUDE_DIRS})
if (NOT PANGOCAIRO_LIBRARIESNAMES OR NOT PANGOCAIRO_INCLUDE_DIRS)
  message(FATAL_ERROR "Couldn't find pangocairo on the sytem or via pkg-config!")
endif ()
include_directories(${PANGOCAIRO_INCLUDE_DIRS})
list(APPEND THIRD_PARTY_LIBS ${PANGOCAIRO_LIBRARIESNAMES})

# Find freetype2
pkg_check_modules(PC_FREETYPE freetype2)
find_library(FREETYPE_LIBRARIESNAMES freetype HINTS ${PC_FREETYPE_LIBDIR} ${PC_FREETYPE_LIBRARY_DIRS})
find_path(FREETYPE_INCLUDE_DIRS freetype/freetype.h HINTS ${PC_FREETYPE_INCLUDEDIR} ${PC_FREETYPE_INCLUDE_DIRS})
if (NOT FREETYPE_LIBRARIESNAMES OR NOT FREETYPE_INCLUDE_DIRS)
  message(FATAL_ERROR "Couldn't find freetype on the sytem or via pkg-config!")
endif ()
include_directories(${FREETYPE_INCLUDE_DIRS})
list(APPEND THIRD_PARTY_LIBS ${FREETYPE_LIBRARIESNAMES})

# Find libpng
pkg_check_modules(PC_PNG libpng)
find_library(PNG_LIBRARIESNAMES png HINTS ${PC_PNG_LIBDIR} ${PC_PNG_LIBRARY_DIRS})
find_path(PNG_INCLUDE_DIRS png.h HINTS ${PC_PNG_INCLUDEDIR} ${PC_PNG_INCLUDE_DIRS})
if (NOT PNG_LIBRARIESNAMES OR NOT PNG_INCLUDE_DIRS)
  message(FATAL_ERROR "Couldn't find libpng on the sytem or via pkg-config!")
endif ()
include_directories(${PNG_INCLUDE_DIRS})
list(APPEND THIRD_PARTY_LIBS ${PNG_LIBRARIESNAMES})

message(${CAIRO_INCLUDE_DIRS})

# Begin optional

# pkg_check_modules(SVG librsvg-2.0)
# find_library(gif)
# pkg_check_modules(JPEG libjpeg)

# Seems to be necessary to build a binary with hanging symbols (a "plugin") on macOS
if (APPLE)
  set(CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS "${CMAKE_SHARED_LIBRARY_CREATE_CXX_FLAGS} -undefined dynamic_lookup")
endif ()

# Main target
file(GLOB_RECURSE SOURCE_FILES "src/*.cc")
add_library(canvas SHARED ${SOURCE_FILES})
target_link_libraries(canvas ${THIRD_PARTY_LIBS})

# register_font.c
if (APPLE)
  target_link_libraries(canvas "-framework CoreFoundation -framework CoreText")
endif ()

# Add optional JPEG dependencies
#find_library(JPEG_LIBRARIES jpeg)
#find_path(JPEG_INCLUDE_DIRS jpeglib.h)
#if (JPEG_LIBRARIES AND JPEG_INCLUDE_DIRS)
#  target_link_libraries(canvas ${JPEG_LIBRARIES})
#  target_include_directories(canvas PUBLIC ${JPEG_INCLUDE_DIRS})
#  add_definitions(-DHAVE_JPEG)
#  message("Using JPEG")
#endif ()
#
#find_library(GIF_LIBRARIES jpeg)
#find_path(GIF_INCLUDE_DIRS gif_lib.h)
#if (GIF_LIBRARIES AND GIF_INCLUDE_DIRS)
#  target_link_libraries(canvas ${GIF_LIBRARIES})
#  target_include_directories(canvas PUBLIC ${GIF_INCLUDE_DIRS})
#  add_definitions(-DHAVE_GIF)
#  message("Using GIF")
#endif ()
