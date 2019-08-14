get_filename_component(EUDAQ_USERNAME ${CMAKE_CURRENT_LIST_DIR} NAME)
set(EUDAQ_MODULE module_${EUDAQ_USERNAME})

option(USER_CMSTOTEM_BUILD "build user/cmstotem" ON)

if(NOT USER_CMSTOTEM_BUILD)
    message(STATUS "user/cmstotem is NOT to be built (USER_CMSTOTEM_BUILD=OFF)")
  return()
endif()

message(STATUS "user/cmstotem is to be built (USER_CMSTOTEM_BUILD=ON)")

add_subdirectory(module)
