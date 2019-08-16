include_directories(include)
aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_TTREE_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/SampicRawEvent2TTreeEventConverter.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY})

# specific linking for the Sampic manipulation
find_library(SAMPICDAQ_LIB SampicDAQ HINTS ENV SAMPICDAQ_DIR)
find_path(SAMPICDAQ_PATH sampicdaq HINTS ENV SAMPICDAQ_DIR PATH_SUFFIXES include)
find_path(SAMPICDAQ_EXT_PATH BBmenu HINTS ENV SAMPICDAQ_DIR PATH_SUFFIXES external)
if(NOT SAMPICDAQ_LIB OR NOT SAMPICDAQ_PATH OR NOT SAMPICDAQ_EXT_PATH)
  message(FATAL_ERROR "Failed to find the Sampic DAQ library and/or headers!\n"
                      "Did you properly set the SAMPICDAQ_DIR environment variable?")
endif()
message(STATUS "Sampic DAQ library found in ${SAMPICDAQ_LIB}")
target_link_libraries(${EUDAQ_MODULE} ${SAMPICDAQ_LIB})
target_include_directories(${EUDAQ_MODULE} PRIVATE ${SAMPICDAQ_PATH} ${SAMPICDAQ_EXT_PATH})

# QuickUSB library linking
find_library(QUICKUSB_LIB quickusb HINTS ENV QUICKUSB_DIR PATH_SUFFIXES lib lib64)
find_path(QUICKUSB_PATH QuickUSB.h HINTS ENV QUICKUSB_DIR PATH_SUFFIXES include)
if(NOT QUICKUSB_LIB OR NOT QUICKUSB_PATH)
  message(FATAL_ERROR "Failed to find the QuickUSB library and/or header!\n"
                      "Did you properly set the QUICKUSB_DIR environment variable?")
endif()
message(STATUS "QuickUSB library found in ${QUICKUSB_LIB}")
target_link_libraries(${EUDAQ_MODULE} ${QUICKUSB_LIB})
target_include_directories(${EUDAQ_MODULE} PRIVATE ${QUICKUSB_PATH})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
