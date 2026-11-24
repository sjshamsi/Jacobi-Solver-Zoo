### USE THE FOLLOEING TO OVERRIDE DEFAUT CMAKE FLAGS PER CONFIG

# set(PROJ_WARN_GNU "-Wall;-Wextra;-Wpedantic")

# set(PROJ_FLAGS_DEBUG "-O0;-g3;-fno-omit-frame-pointer")
# set(PROJ_FLAGS_RELEASE "-O3;-DNDEBUG")
# set(PROJ_FLAGS_RELWITHDEBINFO "-O2;-g;-DNDEBUG;-fno-omit-frame-pointer")
# set(PROJ_FLAGS_MINSIZEREL "-Os;-DNDEBUG")


# --- Build process ---

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


# Adding error flags sanz -Werror
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(${GNU_CLANG_WFLAGS})
    add_link_options(${GNU_CLANG_WFLAGS})
    message(STATUS "Added GNU/Clang warning flags: ${GNU_CLANG_WFLAGS}")

  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "NVHPC")
    add_compile_options(${NVHPC_WFLAGS})
    add_link_options(${NVHPC_WFLAGS})
    message(STATUS "Added NVHPC warning flags: ${NVHPC_WFLAGS}")

  endif()
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

# Where to place built executables (we'll append mirrored subdirs below)
set(BIN_ROOT "${CMAKE_BINARY_DIR}/bin")

# Collect all .cpp files recursively under src/
file(GLOB_RECURSE ALL_SOURCES CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")

if(NOT ALL_SOURCES)
  message(WARNING "No .cpp files found under ${SRC_DIR}")
endif()

# Build each executable mirrored in BIN_ROOT
foreach(SOURCE_PATH ${ALL_SOURCES})                             # -> e.g. src="/.../src/0/foo.cpp"
  get_filename_component(FILE_NAME "${SOURCE_PATH}" NAME_WE)    # -> "foo"
  get_filename_component(FILE_DIR "${SOURCE_PATH}" DIRECTORY)   # -> "/.../src/0"
  file(RELATIVE_PATH FILE_RELDIR "${SRC_DIR}" "${FILE_DIR}")    # -> "0" (or "" if directly under src)
  set(TARGET_NAME "exe_${FILE_RELDIR}_${FILE_NAME}")
  
  if("${FILE_RELDIR}" STREQUAL "")
    set(OUT_DIR "${BIN_ROOT}")
  else()
    set(OUT_DIR "${BIN_ROOT}/${FILE_RELDIR}")
  endif()
  
  add_executable(${TARGET_NAME} "${SOURCE_PATH}")

  # target_include_directories(${TARGET_NAME} PRIVATE "${INC_DIR}")
  
  set_target_properties(${TARGET_NAME}
    PROPERTIES
    OUTPUT_NAME "${FILE_NAME}.x"
    RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}"
  )
  
  # target_compile_options(${TARGET_NAME} PRIVATE
    # $<$<CXX_COMPILER_ID:GNU,Clang>:${PROJ_WARN_GNU}>
    
    # $<$<CONFIG:Release>:${PROJ_FLAGS_RELEASE}>
  #   $<$<CONFIG:RelWithDebInfo>:${PROJ_FLAGS_RELWITHDEBINFO}>
  #   $<$<CONFIG:MinSizeRel>:${PROJ_FLAGS_MINSIZEREL}>
  #   $<$<CONFIG:Debug>:${PROJ_FLAGS_DEBUG}>
  # )

endforeach()