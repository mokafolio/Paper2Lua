cmake_minimum_required (VERSION 2.8.11)
project (Paper2Lua)
set(CMAKE_CXX_FLAGS "-std=c++11 -fno-exceptions")

find_package(Lua 5.2 REQUIRED)
include_directories (${CMAKE_CURRENT_SOURCE_DIR} /usr/local/include ${LUA_INCLUDE_DIR})
link_directories(/usr/local/lib)

set (PAPERLUADEPS Stick Paper2 pthread ${LUA_LIBRARIES})

set (PAPERLUAINC
Paper2Lua/Paper2Lua.hpp
)

install (FILES ${PAPERLUAINC} DESTINATION /usr/local/include/Paper2Lua)
add_subdirectory (Tests)
