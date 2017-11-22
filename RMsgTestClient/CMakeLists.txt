set(SRC
    RMsgTestClient.cpp
    ${PROTO_OUTPUT}
    )
set_source_files_properties(
    ${PROTO_OUTPUT}
    PROPERTIES
    GENERATED TRUE)

include_directories(${RMSG_INCLUDE_DIR})

add_definitions(-D_WIN32_WINNT=0x0501)
add_executable(RMsgTestClient
    ${SRC})
add_dependencies(RMsgTestClient RemoteMessage CompileProto)
target_link_libraries(RMsgTestClient ${RMSG_LIBRARY} optimized libprotobuf-lite.lib debug libprotobuf-lited.lib)
