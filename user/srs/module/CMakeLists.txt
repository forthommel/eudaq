include_directories(${EUDAQ_INCLUDE_DIRS})
include_directories(include)
aux_source_directory(src MODULE_SRC)
include_directories(../extern)
aux_source_directory(../extern EXTERN_SRC)

find_package(SRS)
if(NOT SRS_FOUND)
  message(FATAL_ERROR "SRS readout library not found")
endif()

if(NOT EUDAQ_LCIO_LIBRARY)
#  list(REMOVE_ITEM MODULE_SRC src/AHCalRawEvent2LCEventConverter.cc)
endif()

if(NOT EUDAQ_TTREE_LIBRARY)
  #list(REMOVE_ITEM MODULE_SRC src/AHCALDesyRawEvent2TTreeEventConverter.cc)
  #list(REMOVE_ITEM MODULE_SRC src/AHCALRawEvent2TTreeEventConverter.cc)
endif()

if(NOT EUDAQ_BUILD_ONLINE_ROOT_MONITOR)
  list(REMOVE_ITEM MODULE_SRC src/SrsMonitor.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC} ${EXTERN_SRC})
target_include_directories(${EUDAQ_MODULE} PRIVATE ${SRS_INCLUDE_DIR})
target_link_libraries(${EUDAQ_MODULE} ${SRS_LIBRARY})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_LCIO_LIBRARY} ${LCIO_LIBRARIES} ${EUDAQ_ROOT_MONITOR_LIBRARY})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
