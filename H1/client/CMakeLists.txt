cmake_minimum_required(VERSION 3.16)
project(HelloWorld)

set(CMAKE_CXX_STANDARD 17)

include_directories(./include)
set(my_cpps "./src/client.cpp")
add_executable(client_exec ${my_includes} ${my_cpps})