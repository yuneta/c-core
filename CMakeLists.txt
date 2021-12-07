##############################################
#   CMake
##############################################
project(yuneta-core C)
cmake_minimum_required(VERSION 2.8)
include(CheckIncludeFiles)
include(CheckSymbolExists)

set(CMAKE_INSTALL_PREFIX /yuneta/development/output)

set(INC_DEST_DIR ${CMAKE_INSTALL_PREFIX}/include)
set(LIB_DEST_DIR ${CMAKE_INSTALL_PREFIX}/lib)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -std=c99")

if(CMAKE_BUILD_TYPE MATCHES Debug)
  add_definitions(-DDEBUG)
  option(SHOWNOTES "Show preprocessor notes" OFF)

  if(CMAKE_COMPILER_IS_GNUCC)
    # GCC specific debug options
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g3 -ggdb3 -gdwarf-2")
    set(AVOID_VERSION -avoid-version)
  endif(CMAKE_COMPILER_IS_GNUCC)
endif(CMAKE_BUILD_TYPE MATCHES Debug)

add_definitions(-D_GNU_SOURCE)
add_definitions(-D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64)

check_include_files(regex.h HAVE_REGEX_H)
if(HAVE_REGEX_H)
    add_definitions(-DHAVE_REGEX_H)
else(HAVE_REGEX_H)
    message("***  WARNING: GNU C regex library not found.")
endif(HAVE_REGEX_H)

include_directories(/yuneta/development/output/include)

##############################################
#   Source
##############################################

set (SRCS
    src/yuneta_environment.c
    src/yuneta_register.c
    src/dbsimple.c
    src/dbsimple2.c

    # Daemon/CLI builders
    src/entry_point.c

    # GClass core
    src/c_yuno.c            # the grandmother

    # Services          # service, gotter or spider or gateway.
    src/c_treedb.c
    src/c_node.c
    src/c_tranger.c
    src/c_resource.c
    src/c_ievent_srv.c
    src/c_ievent_cli.c
#     src/c_store.c
#     src/c_snmp.c
#     src/c_dynagent.c

    # Gadgets
    src/c_mqiogate.c
    src/c_qiogate.c
    src/c_iogate.c
    src/c_channel.c
    src/c_counter.c
    src/c_task.c
    src/c_dynrule.c
    src/c_timetransition.c
    src/c_rstats.c

    # Protocols
    src/c_connex.c
#     src/c_serverlink.c
#     src/c_tlv.c
    src/c_websocket.c
    src/c_prot_header4.c
    src/c_prot_raw.c
    src/c_prot_http.c
    src/c_prot_http_srv.c
    src/c_prot_http_cli.c
    src/c_prot_modbus_master.c
    src/c_prot_canopen.c
    src/c_serial.c

    # gossamer
    src/c_gss_udp_s0.c

    # Mixin uv-gobj
    src/c_tcp0.c
    src/c_tcp_s0.c
    src/c_udp0.c
    src/c_udp_s0.c
    src/c_timer.c
    src/c_fs.c
    src/c_canbus0.c

    # Protocols, decoders
    src/ghttp_parser.c
)

set (HDRS
    src/yuneta_version.h
    src/yuneta.h
    src/yuneta_environment.h
    src/yuneta_register.h
    src/msglog_yuneta.h
    src/dbsimple.h
    src/dbsimple2.h

    # Daemon/CLI builders
    src/entry_point.h

    # GClass core
    src/c_yuno.h            # the grandmother

    # Services          # service, gotter or spider or gateway.
    src/c_treedb.h
    src/c_node.h
    src/c_tranger.h
    src/c_resource.h
    src/c_ievent_srv.h
    src/c_ievent_cli.h

#     src/c_store.h
#     src/c_snmp.h
#     src/c_dynagent.h

    # Gadgets
    src/c_mqiogate.h
    src/c_qiogate.h
    src/c_iogate.h
    src/c_channel.h
    src/c_counter.h
    src/c_task.h
    src/c_dynrule.h
    src/c_timetransition.h
    src/c_rstats.h

    # Protocols
    src/c_connex.h
#     src/c_serverlink.h
#     src/c_tlv.h
    src/c_websocket.h
    src/c_prot_header4.h
    src/c_prot_raw.h
    src/c_prot_http.h
    src/c_prot_http_srv.h
    src/c_prot_http_cli.h
    src/c_prot_modbus_master.h
    src/c_prot_canopen.h
    src/c_serial.h

    # gossamer
    src/c_gss_udp_s0.h

    # Mixin uv-gobj
    src/c_tcp0.h
    src/c_tcp_s0.h
    src/c_udp0.h
    src/c_udp_s0.h
    src/c_timer.h
    src/c_fs.h
    src/c_canbus0.h

    # Protocols, decoders
    src/ghttp_parser.h
)


##############################################
#   Lib
##############################################
add_library(yuneta-core ${SRCS} ${HDRS})

##############################################
#   System install
##############################################
install(FILES ${HDRS} DESTINATION ${INC_DEST_DIR})

install(
    TARGETS yuneta-core
    PERMISSIONS
    OWNER_READ OWNER_WRITE
    GROUP_READ GROUP_WRITE
    WORLD_READ
    DESTINATION ${LIB_DEST_DIR}
)

##############################################
#   sub-projects
##############################################
# TODO mueve los test fuera del core
# add_subdirectory(tests) # WARNING Si falta alguna funcion, el core no compila, por la dependencia de tests


# compile in Release mode optimized but adding debug symbols, useful for profiling :
#
#     cmake -DCMAKE_BUILD_TYPE=RelWithDebInfo ..
#
# or compile with NO optimization and adding debug symbols :
#
#     cmake -DCMAKE_BUILD_TYPE=Debug ..
#
# Compilando para ARM poner cmake -DNOT_INCLUDE_LIBUNWIND
#
