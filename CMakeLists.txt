cmake_minimum_required(VERSION 3.1)

project(RemoteMessage)
set (CMAKE_CXX_STANDARD 11)

set(BOOST_INCLUDE_DIR "" CACHE PATH "boost inc directory")
set(BOOST_LIBRARY_DIR "" CACHE PATH "boost lib directory")
include_directories(${BOOST_INCLUDE_DIR})
link_directories(${BOOST_LIBRARY_DIR})

# set(CMAKE_PREFIX_PATH "C:/Program Files (x86)/protobuf")
# find_package(protobuf CONFIG REQUIRED)
# if (NOT PROTOBUF_FOUND)
#     message(FATAL_ERROR "protobuf not found")
#     return()
# endif()

set(PROTOBUF_DIR "" CACHE PATH "protobuf directory")
include_directories(${PROTOBUF_DIR}/include)
link_directories(${PROTOBUF_DIR}/lib)

add_subdirectory(RemoteMessage)
add_subdirectory(RMsgTestClient)
add_subdirectory(RMsgTestServer)
