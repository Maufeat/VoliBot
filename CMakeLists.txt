cmake_minimum_required(VERSION 3.1)

if(MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /std:c++17")
else()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -fpermissive")
endif()

add_subdirectory(LeagueAPI)
add_executable(VoliBot main.cpp InstanceManager.hpp)
target_link_libraries(VoliBot leagueapi)
