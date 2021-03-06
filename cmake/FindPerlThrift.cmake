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

# - Find PYTHON5
# This module defines
#  PERLTHRIFT_FOUND, If false, do not try to use ant

exec_program(env ARGS perl -MThrift -e 0 OUTPUT_VARIABLE PERLTHRIFT_OUT
             RETURN_VALUE PERLTHRIFT_RETURN)

if (PERLTHRIFT_RETURN STREQUAL "0")
  set(PERLTHRIFT_FOUND TRUE)
else ()
  set(PERLTHRIFT_FOUND FALSE)
endif ()

if (PERLTHRIFT_FOUND)
  if (NOT PERLTHRIFT_FIND_QUIETLY)
    message(STATUS "Found thrift for perl")
  endif ()
else ()
  message(STATUS "Thrift for perl not found. "
                 "ThriftBroker support for perl will be disabled")
endif ()

