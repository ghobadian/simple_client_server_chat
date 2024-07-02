cmake_minimum_required(VERSION 3.22)
project(client_server C)

set(CMAKE_C_STANDARD 99)

add_executable(server server.c)
add_executable(client client.c)
