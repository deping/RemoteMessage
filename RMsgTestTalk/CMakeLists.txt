# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)
# Instruct CMake to run moc automatically when needed
set(CMAKE_AUTOMOC ON)
# Create code from a list of Qt designer ui files
set(CMAKE_AUTOUIC ON)

# Find the QtWidgets library
find_package(Qt5Widgets CONFIG REQUIRED)

set(SRC
    RMsgTestTalk.cpp
    UpdateUI.h
    UpdateUI.cpp
    )

include_directories(${RMSG_INCLUDE_DIR})

add_definitions(-D_WIN32_WINNT=0x0501)
add_executable(RMsgTestTalk WIN32 
    ${SRC})
add_dependencies(RMsgTestTalk RemoteMessage)
target_link_libraries(RMsgTestTalk ${RMSG_LIBRARY} Qt5::Widgets optimized libprotobuf-lite.lib debug libprotobuf-lited.lib)
