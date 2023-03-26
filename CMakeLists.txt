cmake_minimum_required(VERSION 3.8)
project(robots)

set(CMAKE_CXX_STANDARD "20")
set(CMAKE_CXX_FLAGS "-g -std=gnu++2a -Wall -Wextra -Wconversion -Werror -O2")

find_package(Boost COMPONENTS program_options system REQUIRED)
include_directories(${Boost_INCLUDE_DIR})

add_executable(robots-client client/robots-client.cpp common/types.h common/program_options.h
               common/game.h common/buffer.cpp common/buffer.h common/events.cpp common/events.h
               common/messages.cpp common/messages.h)

add_executable(robots-server server/robots-server.cpp common/types.h common/program_options.h
               common/game.h common/buffer.cpp common/buffer.h common/events.cpp common/events.h
               common/messages.cpp common/messages.h server/blocking_queue.h
               server/game_manager.cpp server/game_manager.h server/message_sender.h
               server/message_receiver.h server/client_connection.h server/server.cpp
               server/server.h)

target_link_libraries(robots-client ${Boost_LIBRARIES} pthread)
target_link_libraries(robots-server ${Boost_LIBRARIES} pthread)

install(TARGETS DESTINATION .)