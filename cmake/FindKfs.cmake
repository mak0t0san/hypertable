# Copyright (C) 2007-2012 Hypertable, Inc.
#
# This file is part of Hypertable.
#
# Hypertable is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 3
# of the License, or any later version.
#
# Hypertable is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Hypertable. If not, see <http://www.gnu.org/licenses/>
#

# - Find Kfs
# Find the native Kfs includes and library
#
#  Kfs_INCLUDE_DIR - where to find Kfs.h, etc.
#  Kfs_LIBRARIES   - List of libraries when using Kfs.
#  Kfs_FOUND       - True if Kfs found.


if (Kfs_INCLUDE_DIR)
  # Already in cache, be silent
  set(Kfs_FIND_QUIETLY TRUE)
endif ()

find_path(Kfs_INCLUDE_DIR kfs/KfsClient.h
  /opt/kfs/include
  /opt/local/include
  /usr/local/include
  $ENV{HOME}/src/kosmosfs/build/include
)

macro(FIND_KFS_LIB lib)
  find_library(${lib}_LIB NAMES ${lib}
    PATHS /opt/kfs/lib/static /opt/kfs/lib /opt/local/lib /usr/local/lib
          $ENV{HOME}/src/kosmosfs/build/lib/static
  )
  mark_as_advanced(${lib}_LIB)
endmacro(FIND_KFS_LIB lib libname)

FIND_KFS_LIB(kfsClient)
FIND_KFS_LIB(kfsIO)
FIND_KFS_LIB(kfsCommon)
FIND_KFS_LIB(qcdio)

find_library(Crypto_LIB NAMES crypto PATHS /opt/local/lib /usr/local/lib)

if (Kfs_INCLUDE_DIR AND kfsClient_LIB)
  set(Kfs_FOUND TRUE)
  set( Kfs_LIBRARIES ${kfsClient_LIB} ${kfsIO_LIB} ${kfsCommon_LIB}
                     ${qcdio_LIB} ${Crypto_LIB})
else ()
   set(Kfs_FOUND FALSE)
   set( Kfs_LIBRARIES)
endif ()

if (Kfs_FOUND)
   if (NOT Kfs_FIND_QUIETLY)
      message(STATUS "Found KFS: ${Kfs_LIBRARIES}")
   endif ()
else ()
   if (Kfs_FIND_REQUIRED)
      message(FATAL_ERROR "Could NOT find KFS libraries")
   endif ()
endif ()

mark_as_advanced(
  Kfs_INCLUDE_DIR
)
