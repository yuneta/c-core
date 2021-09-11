# Install script for directory: /yuneta/development/yuneta/^yuneta/c-core

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/yuneta/development/output")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/yuneta/development/output/include/yuneta_version.h;/yuneta/development/output/include/yuneta.h;/yuneta/development/output/include/yuneta_environment.h;/yuneta/development/output/include/yuneta_register.h;/yuneta/development/output/include/msglog_yuneta.h;/yuneta/development/output/include/dbsimple.h;/yuneta/development/output/include/dbsimple2.h;/yuneta/development/output/include/entry_point.h;/yuneta/development/output/include/c_yuno.h;/yuneta/development/output/include/c_treedb.h;/yuneta/development/output/include/c_node.h;/yuneta/development/output/include/c_tranger.h;/yuneta/development/output/include/c_resource.h;/yuneta/development/output/include/c_ievent_srv.h;/yuneta/development/output/include/c_ievent_cli.h;/yuneta/development/output/include/c_mqiogate.h;/yuneta/development/output/include/c_qiogate.h;/yuneta/development/output/include/c_iogate.h;/yuneta/development/output/include/c_channel.h;/yuneta/development/output/include/c_counter.h;/yuneta/development/output/include/c_task.h;/yuneta/development/output/include/c_dynrule.h;/yuneta/development/output/include/c_timetransition.h;/yuneta/development/output/include/c_rstats.h;/yuneta/development/output/include/c_connex.h;/yuneta/development/output/include/c_websocket.h;/yuneta/development/output/include/c_prot_header4.h;/yuneta/development/output/include/c_prot_raw.h;/yuneta/development/output/include/c_prot_http.h;/yuneta/development/output/include/c_prot_http_srv.h;/yuneta/development/output/include/c_prot_http_cli.h;/yuneta/development/output/include/c_gss_udp_s0.h;/yuneta/development/output/include/c_tcp0.h;/yuneta/development/output/include/c_tcp_s0.h;/yuneta/development/output/include/c_udp0.h;/yuneta/development/output/include/c_udp_s0.h;/yuneta/development/output/include/c_timer.h;/yuneta/development/output/include/c_fs.h;/yuneta/development/output/include/ghttp_parser.h")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/yuneta/development/output/include" TYPE FILE FILES
    "/yuneta/development/yuneta/^yuneta/c-core/src/yuneta_version.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/yuneta.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/yuneta_environment.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/yuneta_register.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/msglog_yuneta.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/dbsimple.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/dbsimple2.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/entry_point.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_yuno.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_treedb.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_node.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_tranger.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_resource.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_ievent_srv.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_ievent_cli.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_mqiogate.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_qiogate.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_iogate.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_channel.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_counter.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_task.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_dynrule.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_timetransition.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_rstats.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_connex.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_websocket.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_prot_header4.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_prot_raw.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_prot_http.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_prot_http_srv.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_prot_http_cli.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_gss_udp_s0.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_tcp0.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_tcp_s0.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_udp0.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_udp_s0.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_timer.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/c_fs.h"
    "/yuneta/development/yuneta/^yuneta/c-core/src/ghttp_parser.h"
    )
endif()

if("x${CMAKE_INSTALL_COMPONENT}x" STREQUAL "xUnspecifiedx" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/yuneta/development/output/lib/libyuneta-core.a")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
file(INSTALL DESTINATION "/yuneta/development/output/lib" TYPE STATIC_LIBRARY PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE WORLD_READ FILES "/yuneta/development/yuneta/^yuneta/c-core/cmake-build-debug/libyuneta-core.a")
endif()

if(CMAKE_INSTALL_COMPONENT)
  set(CMAKE_INSTALL_MANIFEST "install_manifest_${CMAKE_INSTALL_COMPONENT}.txt")
else()
  set(CMAKE_INSTALL_MANIFEST "install_manifest.txt")
endif()

string(REPLACE ";" "\n" CMAKE_INSTALL_MANIFEST_CONTENT
       "${CMAKE_INSTALL_MANIFEST_FILES}")
file(WRITE "/yuneta/development/yuneta/^yuneta/c-core/cmake-build-debug/${CMAKE_INSTALL_MANIFEST}"
     "${CMAKE_INSTALL_MANIFEST_CONTENT}")
