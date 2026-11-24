# --- The Project ---

cmake_minimum_required(VERSION 3.24)


# The following detects NVHPC and informs us about it. Neesd to be done before the "project(...)" declaration
if(DEFINED ENV{CXX} AND "$ENV{CXX}" STREQUAL "nvc++")
  find_program(NVCXX_PATH NAMES nvc++ HINTS ENV PATH)
  if(NOT NVCXX_PATH)
    message(FATAL_ERROR
      "Environment requested NVHPC compiler (CXX=nvc++), "
      "but 'nvc++' was not found in PATH.\n"
      "→ Please load the NVHPC module or add its bin directory to PATH.")
  else()
    message(STATUS "Using NVHPC compiler: ${NVCXX_PATH}")
  endif()
endif()


project(ParallelJacobi LANGUAGES CXX)



# --- COMPILE OPTIONS ---

# Required for GoogleTest
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON) # Supposedly disables GCC and prefers the C++20 compiler? Lies so far.


# Export compile_commands.json for VS Code IntelliSense (optional)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# If no preset, Release is the default build type
if(NOT DEFINED CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
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


# Basic compile flags
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  add_compile_options(${MY_DEV_FLAGS})
  add_link_options(${MY_DEV_FLAGS})
  message(STATUS "Added Debug flags: ${MY_DEV_FLAGS}")

elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
  add_compile_options(${MY_REL_FLAGS})
  add_link_options(${MY_REL_FLAGS})
  message(STATUS "Added Release flags: ${MY_REL_FLAGS}")
  
endif()


# Adding error flags sanz -Werror (yes, for the release build too)
if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(${GNU_CLANG_WFLAGS})
  add_link_options(${GNU_CLANG_WFLAGS})
  message(STATUS "Added GNU/Clang warning flags: ${GNU_CLANG_WFLAGS}")

elseif (CMAKE_CXX_COMPILER_ID STREQUAL "NVHPC")
  add_compile_options(${NVHPC_WFLAGS})
  add_link_options(${NVHPC_WFLAGS})
  message(STATUS "Added NVHPC warning flags: ${NVHPC_WFLAGS}")

endif()


# Option to enable sanitation flags (set like -DSANF=ON)
option(SANF "Enable sanitizers (GCC/Clang only)" OFF)

if (SANF)
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(${SAN_FLAGS})
    add_link_options(${SAN_FLAGS})
    message(STATUS "Sanitizers enabled: ${SANITIZERS}")

  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "NVHPC")
    message(WARNING "SANF=ON: NVHPC (nvc++) does not support -fsanitize. Ignoring.")
  
  else()
    message(WARNING "SANF=ON: Unknown compiler; skipping -fsanitize flags.")
  
  endif()
endif()


# Option to point compiler to a specific version of GCC (useful for Leonardo, pass like CUSTOM_GCC=/opt/gcc-13 cmake -S . -B build)
if (DEFINED ENV{CUSTOM_GCC})
  add_compile_options(--gcc-toolchain=$ENV{CUSTOM_GCC})
  add_link_options(--gcc-toolchain=$ENV{CUSTOM_GCC})
  message(STATUS "Using GCC toolchain: $ENV{CUSTOM_GCC}")
endif()


# Link to a chosen multithread library (chosen through presets)
set(BACKEND "None" CACHE STRING "Backend: TBB, OpenMP, NVCXX, or None")
set_property(CACHE BACKEND PROPERTY STRINGS None TBB OpenMP NVCXX)

option(NVCXX_GPU_COMPILE "Enable GPU compilation with NVHPC" OFF)


if (BACKEND STREQUAL "TBB")
  find_package(TBB REQUIRED COMPONENTS tbb)
  link_libraries(TBB::tbb)
  add_compile_definitions(USE_TBB=1)
  message(STATUS "Using TBB backend")

elseif (BACKEND STREQUAL "OpenMP")
  find_package(OpenMP REQUIRED COMPONENTS CXX)
  link_libraries(OpenMP::OpenMP_CXX)
  add_compile_definitions(USE_OPENMP=1)
  message(STATUS "Using OpenMP backend")

elseif (BACKEND STREQUAL "NVCXX")
  add_compile_definitions(USE_NVCXX=1)

  if(NVCXX_GPU_COMPILE)
    add_compile_options(-stdpar=gpu -gpu=cc80)
    message(STATUS "Compiling NVC++ code for GPU (-stdpar=gpu -gpu=cc80)")
  else()
    add_compile_options(-stdpar=multicore)
    message(STATUS "Using NVCXX backend with -stdpar=multicore")
  endif()

elseif (BACKEND STREQUAL "None")
  message(STATUS "Building without parallel backend")

endif()



# --- PROJECT DIRS AND FILES ---

# Dirs
set(INC_DIR "${CMAKE_SOURCE_DIR}/inc")
set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")

# Make headers in inc/ available to all targets.
include_directories("${INC_DIR}")

set(TEST_DIR "${CMAKE_SOURCE_DIR}/test")
if (BACKEND STREQUAL "TBB")
  add_subdirectory(${TEST_DIR})
endif()

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