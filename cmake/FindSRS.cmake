# - Try to find the SRS readout library
# Once done this will define
#  SRS_FOUND - System has its SRS library
#  SRS_INCLUDE_DIR - The SRS library main include directories
#  SRS_LIBRARY - The libraries needed to use SRS readout library

message(STATUS "Looking for SRS readout library.")

set(SRS_DIRS $ENV{SRS_DIR} ${SRS_DIR})
find_path(SRS_INCLUDE_DIR NAMES "srsLib.h" PATHS "${SRS_DIRS}")
message(STATUS "srsLib.h => ${SRS_INCLUDE_DIR}")
if(SRS_INCLUDE_DIR)
   set(SRS_INC_FOUND TRUE)
   message(STATUS "Found SRS library headers: ${SRS_INCLUDE_DIR}")
endif()

find_library(SRS_LIBRARY NAMES "libsrs.so" HINTS "${SRS_DIRS}")
message(STATUS "libsrs.so => ${SRS_LIBRARY}")
if(SRS_LIBRARY)
   set(SRS_LIB_FOUND TRUE)
   message(STATUS "Found SRS readout library: ${SRS_LIBRARY}")
endif()

if(SRS_LIB_FOUND AND SRS_INC_FOUND)
   set(SRS_FOUND TRUE)
   message(STATUS "SRS readout library found")
endif()

mark_as_advanced(SRS_LIBRARY SRS_INCLUDE_DIR)
