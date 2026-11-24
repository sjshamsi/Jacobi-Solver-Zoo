# --- The Project ---

cmake_minimum_required(VERSION 3.27)
project(MyJacobi LANGUAGES C CXX)



# --- COMPILE OPTIONS ---

# Required for GoogleTest
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON) # Supposedly disables GCC and prefers the C++20 compiler? Lies so far.


# Export compile_commands.json for VS Code IntelliSense (optional)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Force -march=native for all targets
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-march=native)
elseif (CMAKE_CXX_COMPILER_ID MATCHES "Intel")
  add_compile_options(-xHost)
endif()


# Adding custom warning/debuf flags for the build types
set(SAN_FLAGS -fsanitize=address,undefined,leak,pointer-compare,pointer-subtract)

# Basic DEV/REL flags
set(MY_DEV_FLAGS -O0 -Werror)
set(MY_REL_FLAGS -O3 -DNDEBUG)

# Warning flags (note minus -Werror !!!)
set(GNU_CLANG_WFLAGS -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wcast-align -Wdouble-promotion
                     -Wformat=2 -Wimplicit-fallthrough -Wnon-virtual-dtor -Wold-style-cast
                     -Woverloaded-virtual -Wnull-dereference)

set(NVHPC_WFLAGS -Mbounds -Minfo=all -Minform=warn -Mchkstk -Ktrap=fp -traceback -Wall)


# Option to point compiler to a specific version of GCC (useful for Leonardo, pass like CUSTOM_GCC=/opt/gcc-13 cmake -S . -B build)
if (DEFINED ENV{CUSTOM_GCC})
  add_compile_options(--gcc-toolchain=$ENV{CUSTOM_GCC})
  add_link_options(--gcc-toolchain=$ENV{CUSTOM_GCC})
  message(STATUS "Using GCC toolchain: $ENV{CUSTOM_GCC}")
endif()


# Link MPI
find_package(MPI REQUIRED COMPONENTS C CXX)
link_libraries(MPI::MPI_C MPI::MPI_CXX)

# Link OpenMP
find_package(OpenMP REQUIRED)
link_libraries(OpenMP::OpenMP_C OpenMP::OpenMP_CXX)


# --- PROJECT DIRS AND FILES ---

# Dirs
set(INC_DIR "${CMAKE_SOURCE_DIR}/inc")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# Make headers in inc/ available to all targets.
include_directories("${INC_DIR}")
include_directories("${CMAKE_SOURCE_DIR}/external/myMatrixPrint/include")
include_directories("${CMAKE_SOURCE_DIR}/external/myMPITimer/include")

# --- Start the bin and test building process ---
add_subdirectory(${SRC_DIR})


# --- Convenience Targets ---
add_custom_target(BINS ALL DEPENDS ${ALL_SOURCES})

add_custom_target(rebuild
  COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --clean-first
  USES_TERMINAL
  COMMENT "Cleaning and rebuilding the whole project..."
)

add_custom_target(distclean
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/*"
    COMMENT "Deleting everything from ${CMAKE_BINARY_DIR}..."
)

message(STATUS "Executables will be placed under: ${BIN_ROOT}/<mirrored-subdirs>")