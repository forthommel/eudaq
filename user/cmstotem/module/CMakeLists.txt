include_directories(include)
aux_source_directory(src MODULE_SRC)

if(NOT EUDAQ_TTREE_LIBRARY)
  list(REMOVE_ITEM MODULE_SRC src/SampicRawEvent2TTreeEventConverter.cc)
endif()

add_library(${EUDAQ_MODULE} SHARED ${MODULE_SRC})
target_link_libraries(${EUDAQ_MODULE} ${EUDAQ_CORE_LIBRARY} ${EUDAQ_TTREE_LIBRARY})

# specific linking for the Sampic manipulation
find_library(SAMPIC_DAQ_LIB SampicDAQ HINTS $ENV{SAMPIC_DAQ})
if(NOT SAMPIC_DAQ_LIB)
  message(FATAL_ERROR "Failed to retrieve the Sampic DAQ library!")
endif()
target_link_libraries(${EUDAQ_MODULE} ${SAMPIC_DAQ_LIB})
find_path(SAMPIC_DAQ_PATH sampicdaq HINTS $ENV{SAMPIC_DAQ} PATH_SUFFIXES include)
find_path(SAMPIC_DAQ_EXT_PATH BBmenu HINTS $ENV{SAMPIC_DAQ} PATH_SUFFIXES external)
if(NOT SAMPIC_DAQ_PATH OR NOT SAMPIC_DAQ_EXT_PATH)
  message(FATAL_ERROR "Failed to retrieve the Sampic DAQ library headers!")
endif()
target_include_directories(${EUDAQ_MODULE} PRIVATE ${SAMPIC_DAQ_PATH} ${SAMPIC_DAQ_EXT_PATH})

install(TARGETS
  ${EUDAQ_MODULE}
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
  ARCHIVE DESTINATION lib)
