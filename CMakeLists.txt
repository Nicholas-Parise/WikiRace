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
file(GLOB CPP_SOURCES src/dbUtil.cpp src/graph.cpp)
file(GLOB MAIN src/main.cpp)
file(GLOB PAGERANK_SEQUENTIAL src/pageRankSequential.cpp)
file(GLOB PAGERANK_SHARED_MEMORY src/pageRankSharedMemory.cpp)
file(GLOB PAGERANK_DISTRIBUTED_MEMORY src/pageRankDistributedMemory.cpp)
file(GLOB BI src/bidirectional_wikiRace.cpp)
set(SQLITE_SOURCE src/sqlite3.c)

# Add executable
add_executable(wikiRace ${MAIN} ${CPP_SOURCES} ${SQLITE_SOURCE})
add_executable(pageRankSequential ${PAGERANK_SEQUENTIAL} ${CPP_SOURCES} ${SQLITE_SOURCE})
add_executable(pageRankSharedMemory ${PAGERANK_SHARED_MEMORY} ${CPP_SOURCES} ${SQLITE_SOURCE})

add_executable(bidirectional_wikiRace ${BI} ${CPP_SOURCES} ${SQLITE_SOURCE}
        src/bidirectional_wikiRace.cpp)

# Compile SQLite with FTS5
set_source_files_properties(${SQLITE_SOURCE} PROPERTIES COMPILE_FLAGS "-DSQLITE_ENABLE_FTS5")


find_package(Threads REQUIRED)
target_link_libraries(wikiRace PRIVATE Threads::Threads dl)
target_link_libraries(pageRankSharedMemory PRIVATE Threads::Threads dl)


find_package(MPI)
if (MPI_FOUND)
    message(STATUS "MPI found")
    add_executable(pageRankDistributedMemory ${PAGERANK_DISTRIBUTED_MEMORY} ${CPP_SOURCES} ${SQLITE_SOURCE})
    target_link_libraries(pageRankDistributedMemory MPI::MPI_CXX)
else()
    message(WARNING "MPI NOT found")
endif()

find_package(OpenMP REQUIRED)
if (OPENMP_FOUND)
    message(STATUS "MP found")
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
    set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
else()
    message(WARNING "MP NOT found, REQUIRED DEPENCENCY")
endif()

target_link_libraries(wikiRace PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(bidirectional_wikiRace PRIVATE OpenMP::OpenMP_CXX)

