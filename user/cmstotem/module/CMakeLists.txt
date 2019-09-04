include_directories(include)
aux_source_directory(src MODULE_SRC)
aux_source_directory(../extern EXTERN_SRC)

if(NOT EUDAQ_TTREE_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/SampicRawEvent2TTreeEventConverter.cc)
else()
  list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
  find_package(ROOT REQUIRED COMPONENTS Gui)
  set(THISMON MonitorWindow)
  set(THISMON_DICT_CXX ${CMAKE_CURRENT_BINARY_DIR}/${THISMON}_ROOT.cxx)
  root_generate_dictionary(${THISMON}_ROOT ../extern/MonitorWindow.hh)
  list(APPEND EXTERN_SRC ${THISMON_DICT_CXX})
  if(ROOT_VERSION_MAJOR GREATER 5)
    set(ROOTProducer_PCM ${CMAKE_CURRENT_BINARY_DIR}/${THISMON}_ROOT_rdict.pcm)
    set(ROOTProducer_MAP ${CMAKE_CURRENT_BINARY_DIR}/${THISMON}_ROOT.rootmap)
    install(FILES
      ${ROOTProducer_PCM}
      ${ROOTProducer_MAP}
      DESTINATION lib)
  endif()
endif()

set(EXT_LIBS)
set(EXT_INCS)

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
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY} ${ROOT_LIBRARIES} ${EXT_LIBS})
target_include_directories(${EUDAQ_MODULE} PRIVATE ${EXT_INCS})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
