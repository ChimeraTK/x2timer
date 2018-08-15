PROJECT(chimeratk-x2timer)
cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)

set(${PROJECT_NAME}_MAJOR_VERSION 01)
set(${PROJECT_NAME}_MINOR_VERSION 00)
set(${PROJECT_NAME}_PATCH_VERSION 00)
include(cmake/set_version_numbers.cmake)

# Moderate version of the compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -Wextra -ansi -Wuninitialized -O3 -fomit-frame-pointer")

# add constants for HOST and USER
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DUSER=\\\"`whoami`\\\" -DHOST=\\\"`hostname`\\\"")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0")

# linker flags
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,--export-dynamic -Wl,--no-as-needed")

include(cmake/add_dependency.cmake)
add_dependency(ChimeraTK-ApplicationCore 00.14 REQUIRED)

add_dependency(X2TIMER_SDK 18.11 REQUIRED)
message("X2TIMER_SDK_INCLUDE_DIRS = ${X2TIMER_SDK_INCLUDE_DIRS}")
include_directories(${X2TIMER_SDK_INCLUDE_DIRS})

IF(ADAPTER STREQUAL "OPCUA")
  message("Building against the OPC UA ControlSystemAdater")
  add_dependency(ChimeraTK-ControlSystemAdapter-OPCUAAdapter REQUIRED)
  set(Adapter_LINK_FLAGS ${ChimeraTK-ControlSystemAdapter-OPCUAAdapter_LINK_FLAGS})
  set(Adapter_LIBRARIES ${ChimeraTK-ControlSystemAdapter-OPCUAAdapter_LIBRARIES})
ELSEIF(ADAPTER STREQUAL "DOOCS")
  message("Building against the DOOCS ControlSystemAdater")
  add_dependency(ChimeraTK-ControlSystemAdapter-DoocsAdapter REQUIRED)
  set(Adapter_LINK_FLAGS ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_LINK_FLAGS})
  set(Adapter_LIBRARIES ${ChimeraTK-ControlSystemAdapter-DoocsAdapter_LIBRARIES})
ELSEIF(ADAPTER STREQUAL "EPICSIOC")
  message("Building against the EPICS IOC ControlSystemAdater")
  add_dependency(ChimeraTK-ControlSystemAdapter-EPICS-IOC-Adapter REQUIRED)
  set(Adapter_LINK_FLAGS ${ChimeraTK-ControlSystemAdapter-EPICS-IOC-Adapter_LINK_FLAGS})
  set(Adapter_LIBRARIES ${ChimeraTK-ControlSystemAdapter-EPICS-IOC-Adapter_LIBRARIES})
ELSE()
  message(FATAL_ERROR "Please select your ControlSystemAdapter to use by passing to the cmake command line:\n"
                      "   -DADAPTER=DOOCS to build a DOOCS server\n"
                      "   -DADAPTER=OPCUA to build an OPC UA server\n"
                      "   -DADAPTER=EPICSIOC to build an EPICS IOC")
ENDIF()

set(CMAKE_LINK_FLAGS "-Wl,--no-as-needed ${Adapter_LINK_FLAGS} ${ChimeraTK-ApplicationCore_LINK_FLAGS}")

# set include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
set(${PROJECT_NAME}_INCLUDE_DIRS ${${PROJECT_NAME}_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR}/include/)

# create lists with source files
aux_source_directory(${CMAKE_SOURCE_DIR}/src library_sources)

# keep the rpath in the installed executable and library to correctly find the dependencies
if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/export/doocs/server")
  SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/llrfCtrl_server/lib")
else()
  SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")
endif()
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# server library (to avoid compiling everyting multiple times)
add_library(${PROJECT_NAME}lib SHARED ${library_sources} )
set_target_properties(${PROJECT_NAME}lib PROPERTIES VERSION ${${PROJECT_NAME}_FULL_LIBRARY_VERSION}
                                                    SOVERSION ${${PROJECT_NAME}_SOVERSION})
target_link_libraries(${PROJECT_NAME}lib ${X2TIMER_SDK_LIBRARIES} ${ChimeraTK-ApplicationCore_LIBRARIES} )

# main server executable
add_executable(${PROJECT_NAME} src/dummy.cc)
set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "${CMAKE_LINK_FLAGS}")
target_link_libraries(${PROJECT_NAME} ${PROJECT_NAME}lib ${Adapter_LIBRARIES})

# XML file generation
add_executable(${PROJECT_NAME}-xmlGenerator src/dummy.cc)
set_target_properties(${PROJECT_NAME}-xmlGenerator PROPERTIES COMPILE_FLAGS "-DGENERATE_XML")
set_target_properties(${PROJECT_NAME}-xmlGenerator PROPERTIES LINK_FLAGS "${CMAKE_LINK_FLAGS}")
target_link_libraries(${PROJECT_NAME}-xmlGenerator ${PROJECT_NAME}lib)

add_custom_target(${PROJECT_NAME}-xml ALL DEPENDS ${PROJECT_NAME}-xmlGenerator COMMAND ${PROJECT_BINARY_DIR}/${PROJECT_NAME}-xmlGenerator WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

# Install the executable and the RPC_LIBNO file
if("${CMAKE_INSTALL_PREFIX}" STREQUAL "/export/doocs/server")
  install(TARGETS ${PROJECT_NAME}lib LIBRARY DESTINATION x2timer_server)
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION x2timer_server)
  install(TARGETS ${PROJECT_NAME}-xmlGenerator RUNTIME DESTINATION x2timer_server)
else()
  install(TARGETS ${PROJECT_NAME}lib LIBRARY DESTINATION lib)
  install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)
  install(TARGETS ${PROJECT_NAME}-xmlGenerator RUNTIME DESTINATION bin)
endif()

