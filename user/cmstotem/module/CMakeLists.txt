include_directories(${EUDAQ_INCLUDE_DIRS})
include_directories(include)
aux_source_directory(src MODULE_SRC)
aux_source_directory(../extern EXTERN_SRC)

set(EXT_LIBS)
set(EXT_INCS)

if(NOT EUDAQ_TTREE_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/SampicRawEvent2TTreeEventConverter.cc)
else()
  list(APPEND EXT_LIBS ${ROOT_LIBRARIES} ${EUDAQ_TTREE_LIBRARY})
endif()

if(NOT EUDAQ_ROOT_MONITOR)
  list(REMOVE_ITEM MODULE_SRC src/SampicMonitor.cc)
else()
  list(APPEND EXT_LIBS ${ROOT_LIBRARIES} ${EUDAQ_ROOT_MONITOR})
endif()

# specific linking for the Sampic manipulation
find_library(SAMPICDAQ_LIB SampicDAQ HINTS ENV SAMPICDAQ_DIR)
find_path(SAMPICDAQ_PATH sampicdaq HINTS ENV SAMPICDAQ_DIR PATH_SUFFIXES include)
find_path(SAMPICDAQ_EXT_PATH BBmenu HINTS ENV SAMPICDAQ_DIR PATH_SUFFIXES external)
if(NOT SAMPICDAQ_LIB OR NOT SAMPICDAQ_PATH OR NOT SAMPICDAQ_EXT_PATH)
  list(REMOVE_ITEM MODULE_SRC src/SampicProducer.cc)
  message(STATUS "Failed to find the Sampic DAQ library and/or headers!\n"
                 "Disabling the producer for the time being.\n"
                 "If you intend to use it anyway, you should set the SAMPICDAQ_DIR environment variable.")
else()
  message(STATUS "Sampic DAQ library found in ${SAMPICDAQ_LIB}")
  list(APPEND EXT_LIBS ${SAMPICDAQ_LIB})
  list(APPEND EXT_INCS ${SAMPICDAQ_PATH} ${SAMPICDAQ_EXT_PATH})
endif()

# QuickUSB library linking
find_library(QUICKUSB_LIB quickusb HINTS ENV QUICKUSB_DIR PATH_SUFFIXES lib lib64)
find_path(QUICKUSB_PATH QuickUSB.h HINTS ENV QUICKUSB_DIR PATH_SUFFIXES include)
if(NOT QUICKUSB_LIB OR NOT QUICKUSB_PATH)
  list(REMOVE_ITEM MODULE_SRC src/SampicProducer.cc)
  message(STATUS "Failed to find the QuickUSB library and/or header!\n"
                 "Disabling the producer for the time being.\n"
                 "If you intend to use it anyway, you should set the QUICKUSB_DIR environment variable.")
else()
  message(STATUS "QuickUSB library found in ${QUICKUSB_LIB}")
  list(APPEND EXT_LIBS ${QUICKUSB_LIB})
  list(APPEND EXT_INCS ${QUICKUSB_PATH})
endif()

include_directories(../extern)
add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC} ${EXTERN_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EXT_LIBS})
target_include_directories(${EUDAQ_MODULE} PRIVATE ${EXT_INCS})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
