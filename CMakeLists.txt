cmake_minimum_required(VERSION 3.15)
project(WikiRace)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable optimizations
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -Wall")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -Wall")

set(STATIC_RUNTIME_FLAGS -static-libgcc -static-libstdc++)

# Include directories
include_directories(include)

# Source files
file(GLOB CPP_SOURCES src/dbUtil.cpp src/graph.cpp)
file(GLOB MAIN src/main.cpp)
file(GLOB PAGERANK_SEQUENTIAL src/pageRankSequential.cpp)
file(GLOB PAGERANK_SHARED_MEMORY src/pageRankSharedMemory.cpp)
file(GLOB PAGERANK_DISTRIBUTED_MEMORY src/pageRankDistributedMemory.cpp)
file(GLOB BI src/bidirectional_wikiRace.cpp)
file(GLOB MPI src/mpi_wikiRace.cpp)
file(GLOB RANDOM src/random_wikiRace.cpp)
file(GLOB GUI_MAIN src/gui_main.cpp)
set(SQLITE_SOURCE src/sqlite3.c)

# Add executable
add_executable(wikiRace ${MAIN} ${CPP_SOURCES} ${SQLITE_SOURCE})
add_executable(pageRankSequential ${PAGERANK_SEQUENTIAL} ${CPP_SOURCES} ${SQLITE_SOURCE})
add_executable(pageRankSharedMemory ${PAGERANK_SHARED_MEMORY} ${CPP_SOURCES} ${SQLITE_SOURCE})
add_executable(bidirectional_wikiRace ${BI} ${CPP_SOURCES} ${SQLITE_SOURCE} src/bidirectional_wikiRace.cpp)
add_executable(random_wikiRace ${RANDOM} ${CPP_SOURCES} ${SQLITE_SOURCE})

# Compile SQLite with FTS5
set_source_files_properties(${SQLITE_SOURCE} PROPERTIES COMPILE_FLAGS "-DSQLITE_ENABLE_FTS5")

find_package(Threads REQUIRED)
find_package(OpenMP REQUIRED)

set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")

find_package(MPI)
if (MPI_FOUND)
    message(STATUS "MPI found")
    add_executable(pageRankDistributedMemory ${PAGERANK_DISTRIBUTED_MEMORY} ${CPP_SOURCES} ${SQLITE_SOURCE})
    add_executable(mpi_wikiRace ${MPI} ${CPP_SOURCES} ${SQLITE_SOURCE} src/mpi_wikiRace.cpp)
    target_link_libraries(pageRankDistributedMemory MPI::MPI_CXX)
    target_link_libraries(mpi_wikiRace PRIVATE MPI::MPI_CXX)
    target_link_options(pageRankDistributedMemory PRIVATE ${STATIC_RUNTIME_FLAGS})
    target_link_options(mpi_wikiRace PRIVATE ${STATIC_RUNTIME_FLAGS})
else()
    message(WARNING "MPI NOT found. pageRankDistributedMemory and mpi_wikiRace could NOT be compiled")
endif()


    find_package(SFML COMPONENTS Graphics Window System)
    find_package(imgui CONFIG)
    find_package(ImGui-SFML CONFIG)

    add_executable(wikiRace-GUI
        ${GUI_MAIN}
        ${CPP_SOURCES}
        ${SQLITE_SOURCE}
    )

    target_link_libraries(wikiRace-GUI
    PRIVATE
        SFML::Graphics 
        SFML::Window 
        SFML::System
        ImGui-SFML::ImGui-SFML
        Threads::Threads
        OpenMP::OpenMP_CXX
    )

target_link_libraries(wikiRace PRIVATE Threads::Threads dl)
target_link_libraries(pageRankSharedMemory PRIVATE Threads::Threads dl)
target_link_libraries(wikiRace PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(bidirectional_wikiRace PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(random_wikiRace PRIVATE OpenMP::OpenMP_CXX)

target_link_options(wikiRace PRIVATE ${STATIC_RUNTIME_FLAGS})
target_link_options(pageRankSequential PRIVATE ${STATIC_RUNTIME_FLAGS})
target_link_options(pageRankSharedMemory PRIVATE ${STATIC_RUNTIME_FLAGS})
target_link_options(bidirectional_wikiRace PRIVATE ${STATIC_RUNTIME_FLAGS})
target_link_options(random_wikiRace PRIVATE ${STATIC_RUNTIME_FLAGS})
