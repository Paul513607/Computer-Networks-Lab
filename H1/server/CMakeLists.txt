cmake_minimum_required(VERSION 3.16)
project(HelloWorld)

set(CMAKE_CXX_STANDARD 17)

include_directories(./include)
set(my_includes "./include/UserProfile.hpp")
set(my_cpps "./src/UserProfile.cpp" "./src/server.cpp")
add_executable(server_exec ${my_includes} ${my_cpps})