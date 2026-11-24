# --- Download GTest ---
include(FetchContent)

FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
  DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

# Import the 'module'
FetchContent_MakeAvailable(googletest)
include(GoogleTest)


# --- Setting test files and dirs ---

# Where to place built executables (we'll append mirrored subdirs below)
set(TEST_BIN_ROOT "${CMAKE_BINARY_DIR}/test")

# Collect all test .cpp files recursively under test/
file(GLOB_RECURSE ALL_TESTS CONFIGURE_DEPENDS "${TEST_DIR}/*.cpp")

if(NOT ALL_TESTS)
  message(WARNING "No .cpp files found under ${TEST_DIR}")
endif()

# Have to do it before making test targets
enable_testing()


# --- Build process ---

foreach(TEST_PATH ${ALL_TESTS})                               # -> e.g. test="/.../test/0/foo.cpp"
  get_filename_component(FILE_NAME "${TEST_PATH}" NAME_WE)    # -> "foo"
  get_filename_component(FILE_DIR "${TEST_PATH}" DIRECTORY)   # -> "/.../test/0"
  file(RELATIVE_PATH FILE_RELDIR "${TEST_DIR}" "${FILE_DIR}") # -> "0" (or "" if directly under test)
  
  set(TARGET_NAME "${FILE_RELDIR}_${FILE_NAME}")

  if("${FILE_RELDIR}" STREQUAL "")
    set(OUT_DIR "${TEST_BIN_ROOT}")
  else()
    set(OUT_DIR "${TEST_BIN_ROOT}/${FILE_RELDIR}")
  endif()
  
  add_executable(${TARGET_NAME} "${TEST_PATH}")
  
  # target_include_directories(${TARGET_NAME} PRIVATE "${INC_DIR}")

  target_link_libraries(${TARGET_NAME} PRIVATE GTest::gmock GTest::gtest_main)
  
  set_target_properties(${TARGET_NAME}
    PROPERTIES
    OUTPUT_NAME "${FILE_NAME}.x"
    RUNTIME_OUTPUT_DIRECTORY "${OUT_DIR}"
  )

  gtest_discover_tests(
    ${TARGET_NAME}
  )

endforeach()