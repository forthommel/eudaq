include_directories(include)
aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_TTREE_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/SampicRawEvent2TTreeEventConverter.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY})

# specific linking for the Sampic manipulation
find_library(SAMPICDAQ_LIB SampicDAQ HINTS ENV SAMPICDAQ_DIR)
if(NOT SAMPICDAQ_LIB)
  message(FATAL_ERROR "Failed to find the Sampic DAQ library!")
endif()
message(STATUS "Sampic DAQ library found in ${SAMPICDAQ_LIB}")
target_link_libraries(${EUDAQ_MODULE} ${SAMPICDAQ_LIB})

find_path(SAMPICDAQ_PATH sampicdaq HINTS $ENV{SAMPICDAQ_DIR} PATH_SUFFIXES include)
find_path(SAMPICDAQ_EXT_PATH BBmenu HINTS $ENV{SAMPICDAQ_DIR} PATH_SUFFIXES external)
if(NOT SAMPICDAQ_PATH OR NOT SAMPICDAQ_EXT_PATH)
  message(FATAL_ERROR "Failed to find the Sampic DAQ library headers!")
endif()
target_include_directories(${EUDAQ_MODULE} PRIVATE ${SAMPICDAQ_PATH} ${SAMPICDAQ_EXT_PATH})

# QuickUSB library linking
find_library(QUICKUSB_LIB quickusb HINTS ENV QUICKUSB_LIB)
if(NOT QUICKUSB_LIB)
  message(FATAL_ERROR "Failed to find the QuickUSB library!")
endif()
message(STATUS "QuickUSB library found in ${QUICKUSB_LIB}")
target_link_libraries(${EUDAQ_MODULE} ${QUICKUSB_LIB})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
