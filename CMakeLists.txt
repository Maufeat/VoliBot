cmake_minimum_required(VERSION 3.1)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fpermissive")
endif()

add_subdirectory(LeagueAPI)
add_subdirectory(sqlite)
include_directories("${PROJECT_SOURCE_DIR}/sqlite")

add_executable(VoliBot 
src/main.cpp 
src/common.hpp
src/InstanceManager.hpp
src/InstanceManager.cpp
src/VoliServer.hpp
src/VoliServer.cpp
src/message/MessageType.hpp)
target_link_libraries(VoliBot leagueapi sqlite3)