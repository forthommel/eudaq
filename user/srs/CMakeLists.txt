list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_LIST_DIR}/cmake)
get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)

set(EUDAQ_MODULE module_${EUDAQ_USERNAME})
option(USER_SRS_BUILD "build eudet" OFF)

if(NOT USER_SRS_BUILD)
  message(STATUS "user/srs is NOT to be built (USER_SRS_BUILD=OFF)")
  return()
endif()
message(STATUS "user/srs is to be built (USER_SRS_BUILD=ON)")

include_directories(${EUDAQ_INCLUDE_DIRS})

add_subdirectory(module)
#add_subdirectory(exe)
