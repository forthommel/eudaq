set(EXT_DIR ../extern)
set(MON_DIR ${PROJECT_SOURCE_DIR}/monitors/onlinemon)

include_directories(include)
aux_source_directory(src MODULE_SRC)
aux_source_directory(${EXT_DIR} EXTERN_SRC)

list(REMOVE_ITEM MODULE_SRC src/SampicMonitor.cxx)
if(NOT EUDAQ_TTREE_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/SampicRawEvent2TTreeEventConverter.cc)
  list(REMOVE_ITEM MODULE_SRC src/SampicMonitor.cc)
endif()

include_directories(${EXT_DIR})
add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC} ${EXTERN_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY})

if(EUDAQ_TTREE_LIBRARY)
  target_include_directories(${EUDAQ_MODULE} PRIVATE ${MON_DIR})
endif()

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

if(EUDAQ_TTREE_LIBRARY)
  aux_source_directory(${MON_DIR}/src MON_SRC)
  list(APPEND CMAKE_PREFIX_PATH $ENV{ROOTSYS})
  find_package(ROOT REQUIRED COMPONENTS Gui)
  include_directories(${MON_DIR} ${MON_DIR}/include)
  set(THISMON SampicMonitor)
  set(THISMON_DICT_CXX ${CMAKE_CURRENT_BINARY_DIR}/${THISMON}_ROOT.cxx)
  root_generate_dictionary(${THISMON}_ROOT ${MON_DIR}/include/OnlineMonWindow.hh)
  aux_source_directory(${MON_DIR}/src THISMON_SRC)
  list(APPEND THISMON_SRC src/SampicMonitor.cxx)
  add_executable(${THISMON} ${THISMON_SRC} ${THISMON_DICT_CXX})
  target_link_libraries(${THISMON} ${ROOT_LIBRARIES} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_MODULE})
  if(ROOT_VERSION_MAJOR GREATER 5)
    target_compile_definitions(${THISMON} PRIVATE -DEUDAQ_LIB_ROOT6)
    #set(ROOTProducer_PCM ${CMAKE_CURRENT_BINARY_DIR}/${THISMON}_ROOT_rdict.pcm)
    #set(ROOTProducer_MAP ${CMAKE_CURRENT_BINARY_DIR}/${THISMON}_ROOT.rootmap)
    #install(FILES
    #  ${ROOTProducer_PCM}
    #  ${ROOTProducer_MAP}
    #  DESTINATION lib)
  endif()
  install(TARGETS
    ${THISMON}
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
endif()


install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
