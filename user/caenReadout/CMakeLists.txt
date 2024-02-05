get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE ${EUDAQ_PREFIX}module_${EUDAQ_USERNAME})

option(USER_CAENREADOUT_BUILD "build CAEN readout" ON)

if(NOT USER_CAENREADOUT_BUILD)
    message(STATUS "user/example is NOT to be built (USER_CAENREADOUT_BUILD=OFF)")
  return()
endif()

message(STATUS "user/example is to be built (USER_CAENREADOUT_BUILD=ON)")

add_subdirectory(module)
