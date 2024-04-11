#
# Resource definitions to build the FLTK project using CMake (www.cmake.org)
# Originally written by Michael Surette
#
# Copyright 1998-2024 by Bill Spitzak and others.
#
# This library is free software. Distribution and use rights are outlined in
# the file "COPYING" which should have been included with this file.  If this
# file is missing or damaged, see the license at:
#
#     https://www.fltk.org/COPYING.php
#
# Please see the following page on how to report bugs and issues:
#
#     https://www.fltk.org/bugs.php
#

#######################################################################
# check for headers, libraries and functions
#######################################################################

# If CMAKE_REQUIRED_QUIET is 1 (default) the search is mostly quiet,
# if it is 0 (or not defined) check_include_files() is more verbose
# and the result of the search is logged with fl_debug_var().
# This is useful for debugging.

set(CMAKE_REQUIRED_QUIET 1)

include(CheckIncludeFiles)

macro(fl_find_header VAR HEADER)
  check_include_files("${HEADER}" ${VAR})
  if(NOT CMAKE_REQUIRED_QUIET)
    fl_debug_var(${VAR})
  endif(NOT CMAKE_REQUIRED_QUIET)
endmacro(fl_find_header)

#######################################################################
# Include FindPkgConfig for later use of pkg-config
#######################################################################

include(FindPkgConfig)

# fl_debug_var(PKG_CONFIG_FOUND)
# fl_debug_var(PKG_CONFIG_EXECUTABLE)
# fl_debug_var(PKG_CONFIG_VERSION_STRING)

#######################################################################
# Find header files...
#######################################################################

if(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "FreeBSD")
  list(APPEND CMAKE_REQUIRED_INCLUDES /usr/local/include)
endif(${CMAKE_HOST_SYSTEM_NAME} STREQUAL "FreeBSD")

fl_find_header(HAVE_ALSA_ASOUNDLIB_H alsa/asoundlib.h)
fl_find_header(HAVE_DLFCN_H dlfcn.h)
fl_find_header(HAVE_GL_GLU_H GL/glu.h)
fl_find_header(HAVE_LOCALE_H locale.h)
fl_find_header(HAVE_OPENGL_GLU_H OpenGL/glu.h)
fl_find_header(HAVE_STDIO_H stdio.h)
fl_find_header(HAVE_STRINGS_H strings.h)
fl_find_header(HAVE_SYS_SELECT_H sys/select.h)
fl_find_header(HAVE_SYS_STDTYPES_H sys/stdtypes.h)

fl_find_header(HAVE_X11_XREGION_H "X11/Xlib.h;X11/Xregion.h")

if(WIN32 AND NOT CYGWIN)
  # we don't use pthreads on Windows (except for Cygwin, see options.cmake)
  set(HAVE_PTHREAD_H 0)
else()
  fl_find_header(HAVE_PTHREAD_H pthread.h)
endif(WIN32 AND NOT CYGWIN)

# Do we have PTHREAD_MUTEX_RECURSIVE ?

if(HAVE_PTHREAD_H)
  try_compile(HAVE_PTHREAD_MUTEX_RECURSIVE
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/pthread_mutex_recursive.c
  )
else()
  set(HAVE_PTHREAD_MUTEX_RECURSIVE 0)
endif()

# Special case for Microsoft Visual Studio generator (MSVC):
#
# The header files <GL/glu.h> and <locale.h> are located in the SDK's
# for Visual Studio. If CMake is invoked from a desktop icon or the Windows
# menu it doesn't have the correct paths to find these header files.
# The CMake folks recommend not to search for these files at all, because
# they must always be there, but we do anyway.
# If we don't find them we issue a warning and continue anyway.
#
# Note: these cache variables can only be seen in "advanced" mode.

if(MSVC)

  if(NOT HAVE_GL_GLU_H)
    message(STATUS "Info: Header file GL/glu.h was not found. Continuing...")
    set(HAVE_GL_GLU_H 1)
  endif(NOT HAVE_GL_GLU_H)

  if(NOT HAVE_LOCALE_H)
    message(STATUS "Info: Header file locale.h was not found. Continuing...")
    set(HAVE_LOCALE_H 1)
  endif(NOT HAVE_LOCALE_H)

endif(MSVC)

# Simulate the behavior of autoconf macro AC_HEADER_DIRENT, see:
# https://www.gnu.org/software/autoconf/manual/autoconf-2.69/html_node/Particular-Headers.html
# "Check for the following header files. For the first one that is found
#  and defines 'DIR', define the listed C preprocessor macro ..."
#
# Note: we don't check if it really defines 'DIR', but we stop processing
# once we found the first suitable header file.

fl_find_header(HAVE_DIRENT_H dirent.h)

if(NOT HAVE_DIRENT_H)
  fl_find_header(HAVE_SYS_NDIR_H sys/ndir.h)
  if(NOT HAVE_SYS_NDIR_H)
    fl_find_header(HAVE_SYS_DIR_H sys/dir.h)
    if(NOT HAVE_SYS_DIR_H)
      fl_find_header(HAVE_NDIR_H ndir.h)
    endif(NOT HAVE_SYS_DIR_H)
  endif(NOT HAVE_SYS_NDIR_H)
endif(NOT HAVE_DIRENT_H)

#----------------------------------------------------------------------
# The following code is used to find the include path for freetype
# headers to be able to #include <ft2build.h> in Xft.h.

# where to find freetype headers

find_path(FREETYPE_PATH freetype.h PATH_SUFFIXES freetype2)
find_path(FREETYPE_PATH freetype/freetype.h PATH_SUFFIXES freetype2)

if(FREETYPE_PATH)
  list(APPEND FLTK_BUILD_INCLUDE_DIRECTORIES ${FREETYPE_PATH})
endif(FREETYPE_PATH)

mark_as_advanced(FREETYPE_PATH)

#######################################################################
# libraries
find_library(LIB_dl dl)
if((NOT APPLE) OR FLTK_BACKEND_X11)
  find_library(LIB_fontconfig fontconfig)
endif((NOT APPLE) OR FLTK_BACKEND_X11)
find_library(LIB_freetype freetype)
find_library(LIB_GL GL)
find_library(LIB_MesaGL MesaGL)
find_library(LIB_GLEW NAMES GLEW glew32)
find_library(LIB_jpeg jpeg)
find_library(LIB_png png)
find_library(LIB_zlib z)
find_library(LIB_m m)

mark_as_advanced(LIB_dl LIB_fontconfig LIB_freetype)
mark_as_advanced(LIB_GL LIB_MesaGL LIB_GLEW)
mark_as_advanced(LIB_jpeg LIB_png LIB_zlib)
mark_as_advanced(LIB_m)

#######################################################################
# functions
include(CheckFunctionExists)

# save CMAKE_REQUIRED_LIBRARIES (is this really necessary ?)
if(DEFINED CMAKE_REQUIRED_LIBRARIES)
  set(SAVED_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
else(DEFINED CMAKE_REQUIRED_LIBRARIES)
  unset(SAVED_REQUIRED_LIBRARIES)
endif(DEFINED CMAKE_REQUIRED_LIBRARIES)
set(CMAKE_REQUIRED_LIBRARIES)

if(HAVE_DLFCN_H)
  set(HAVE_DLFCN_H 1)
endif(HAVE_DLFCN_H)

set(CMAKE_REQUIRED_LIBRARIES ${CMAKE_DL_LIBS})
check_function_exists(dlsym                     HAVE_DLSYM)
set(CMAKE_REQUIRED_LIBRARIES)

check_function_exists(localeconv                HAVE_LOCALECONV)

if(LIB_png)
  set(CMAKE_REQUIRED_LIBRARIES ${LIB_png})
  check_function_exists(png_get_valid           HAVE_PNG_GET_VALID)
  check_function_exists(png_set_tRNS_to_alpha   HAVE_PNG_SET_TRNS_TO_ALPHA)
  set(CMAKE_REQUIRED_LIBRARIES)
endif(LIB_png)

check_function_exists(scandir                   HAVE_SCANDIR)
check_function_exists(snprintf                  HAVE_SNPRINTF)

# not really true but we convert strcasecmp calls to _stricmp calls in flstring.h
if(MSVC)
   set(HAVE_STRCASECMP 1)
endif(MSVC)

check_function_exists(strcasecmp                HAVE_STRCASECMP)

check_function_exists(strlcat                   HAVE_STRLCAT)
check_function_exists(strlcpy                   HAVE_STRLCPY)
check_function_exists(vsnprintf                 HAVE_VSNPRINTF)

check_function_exists(setenv                    HAVE_SETENV)

if(LIB_m)
  set(CMAKE_REQUIRED_LIBRARIES ${LIB_m})
  check_function_exists(trunc                   HAVE_TRUNC)
  set(CMAKE_REQUIRED_LIBRARIES)
endif(LIB_m)

if(HAVE_SCANDIR AND NOT HAVE_SCANDIR_POSIX)
   set(MSG "POSIX compatible scandir")
   message(STATUS "Looking for ${MSG}")
   try_compile(V
      ${CMAKE_CURRENT_BINARY_DIR}
      ${CMAKE_CURRENT_SOURCE_DIR}/CMake/posixScandir.cxx
      )
   if(V)
      message(STATUS "${MSG} - found")
      set(HAVE_SCANDIR_POSIX 1 CACHE INTERNAL "")
   else()
      message(STATUS "${MSG} - not found")
      set(HAVE_SCANDIR_POSIX HAVE_SCANDIR_POSIX-NOTFOUND)
   endif(V)
endif(HAVE_SCANDIR AND NOT HAVE_SCANDIR_POSIX)
mark_as_advanced(HAVE_SCANDIR_POSIX)

# restore CMAKE_REQUIRED_LIBRARIES (is this really necessary ?)
if(DEFINED SAVED_REQUIRED_LIBRARIES)
  set(CMAKE_REQUIRED_LIBRARIES ${SAVED_REQUIRED_LIBRARIES})
  unset(SAVED_REQUIRED_LIBRARIES)
else(DEFINED SAVED_REQUIRED_LIBRARIES)
  unset(CMAKE_REQUIRED_LIBRARIES)
endif(DEFINED SAVED_REQUIRED_LIBRARIES)

#######################################################################
# packages

# Doxygen: necessary for HTML and PDF docs
find_package (Doxygen)

# LaTex: necessary for PDF docs (note: FindLATEX doesn't return LATEX_FOUND)

# Note: we only check existence of `latex' (LATEX_COMPILER), hence
# building the pdf docs may still fail because of other missing tools.

set(LATEX_FOUND)
if(DOXYGEN_FOUND)
  find_package (LATEX)
  if(LATEX_COMPILER AND NOT LATEX_FOUND)
    set(LATEX_FOUND YES)
  endif(LATEX_COMPILER AND NOT LATEX_FOUND)
endif(DOXYGEN_FOUND)

# message("Doxygen  found : ${DOXYGEN_FOUND}")
# message("LaTex    found : ${LATEX_FOUND}")
# message("LaTex Compiler : ${LATEX_COMPILER}")

# Cleanup: unset local variables

unset(CMAKE_REQUIRED_QUIET)
