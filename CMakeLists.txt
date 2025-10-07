cmake_minimum_required(VERSION 3.15)
project(WikiRace)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable optimizations
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")


# Include directories
include_directories(include)

# Source files
file(GLOB CPP_SOURCES "src/*.cpp")
set(SQLITE_SOURCE "src/sqlite3.c")

# Add executable
add_executable(wikiRace ${CPP_SOURCES} ${SQLITE_SOURCE})

# Compile SQLite with FTS5
set_source_files_properties(${SQLITE_SOURCE} PROPERTIES COMPILE_FLAGS "-DSQLITE_ENABLE_FTS5")

find_package(Threads REQUIRED)
target_link_libraries(wikiRace PRIVATE Threads::Threads dl)