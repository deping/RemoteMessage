if (WIN32)
    message(STATUS "Using Win32")
    set(SRC
        ImpExpMacro.h
        stdafx.h
        stdafx.cpp
        dllmain.cpp)
elseif (APPLE)
    message(STATUS "Using Win32")
elseif (UNIX)
    message(STATUS "Using UNIX")
else()
    message(FATAL_ERROR "No supported platform was detected")
endif()

list(APPEND SRC
    Message.h
    Message.cpp
    Session.h
    Session.cpp
    Server.h
    Server.cpp
    utility.h
    utility.cpp
    )

add_definitions(-DRMSG_LIB -D_WIN32_WINNT=0x0501)
add_library(RemoteMessage SHARED
    ${SRC})
set(RMSG_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR} PARENT_SCOPE)
set(RMSG_LIBRARY $<TARGET_LINKER_FILE:RemoteMessage> PARENT_SCOPE)
string(REPLACE "built/RemoteMessage" "built/RMsgTestServer"
    RMsgTestServer_TARGET_FILE_DIR
    ${CMAKE_CURRENT_BINARY_DIR})
string(REPLACE "built/RemoteMessage"
    "built/RMsgTestClient" RMsgTestClient_TARGET_FILE_DIR
    ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(TARGET RemoteMessage POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RMsgTestServer_TARGET_FILE_DIR}/$<CONFIG>)
add_custom_command(TARGET RemoteMessage POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${RMsgTestClient_TARGET_FILE_DIR}/$<CONFIG>)
add_custom_command(TARGET RemoteMessage POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:RemoteMessage> $<$<CONFIG:Debug>:$<TARGET_PDB_FILE:RemoteMessage>> ${RMsgTestServer_TARGET_FILE_DIR}/$<CONFIG>)
add_custom_command(TARGET RemoteMessage POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:RemoteMessage> $<$<CONFIG:Debug>:$<TARGET_PDB_FILE:RemoteMessage>> ${RMsgTestClient_TARGET_FILE_DIR}/$<CONFIG>)

set(PROTO_OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/PbSample.pb.h ${CMAKE_CURRENT_SOURCE_DIR}/PbSample.pb.cc)
add_custom_command(
    OUTPUT ${PROTO_OUTPUT}
    COMMAND "${PROTOBUF_DIR}/bin/protoc.exe" -I=${CMAKE_CURRENT_SOURCE_DIR} --cpp_out=${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/PbSample.proto
    DEPENDS PbSample.proto
    VERBATIM
    )
add_custom_target(CompileProto
    DEPENDS ${PROTO_OUTPUT}
    )
set(PROTO_OUTPUT ${PROTO_OUTPUT} PARENT_SCOPE)
