### USE THE FOLLOEING TO OVERRIDE DEFAUT CMAKE FLAGS PER CONFIG

# set(PROJ_WARN_GNU "-Wall;-Wextra;-Wpedantic")

# set(PROJ_FLAGS_DEBUG "-O0;-g3;-fno-omit-frame-pointer")
# set(PROJ_FLAGS_RELEASE "-O3;-DNDEBUG")
# set(PROJ_FLAGS_RELWITHDEBINFO "-O2;-g;-DNDEBUG;-fno-omit-frame-pointer")
# set(PROJ_FLAGS_MINSIZEREL "-Os;-DNDEBUG")


# --- Build process ---

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