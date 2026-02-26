cmake_minimum_required(VERSION 3.14)
project(tf2_standalone LANGUAGES CXX)
find_package(ament_cmake REQUIRED)
ament_package_xml()
set(PROJECT_VERSION ${tf2_standalone_VERSION})

# tf2_standalone: A standalone wrapper around tf2::BufferCore
#
# This library wraps tf2's BufferCore to provide a ROS-agnostic API.
# - Public API uses only tf2:: types (from tf2) + std::string
# - Internal geometry_msgs conversions are hidden from users
#
# Build modes (controlled by BUILD_SHARED_LIBS):
# - Shared (default): Links dynamically to libtf2.so, librcutils.so
# - Static: Fully standalone, rcutils logging disabled at compile time

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to shared libraries (standard CMake pattern)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Find dependencies
find_package(tf2 REQUIRED)
# tf2 provides LinearMath headers - we copy them at install time
find_package(geometry_msgs REQUIRED)

# Main library - respects BUILD_SHARED_LIBS automatically
add_library(${PROJECT_NAME} src/transform_buffer.cpp)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

if(NOT BUILD_SHARED_LIBS)
  # Static build: all deps are PRIVATE — they get bundled into a fat archive
  target_link_libraries(${PROJECT_NAME}
    PRIVATE
      tf2::tf2
      ${geometry_msgs_TARGETS}
  )
  # tf2's public headers (LinearMath, time.hpp) are copied at install time,
  # so consumers don't need find_package(tf2).
  # We still need tf2's include dirs at build time for our own headers.
  target_include_directories(${PROJECT_NAME}
    PUBLIC
      $<BUILD_INTERFACE:$<TARGET_PROPERTY:tf2::tf2,INTERFACE_INCLUDE_DIRECTORIES>>
  )

  target_compile_definitions(${PROJECT_NAME} PRIVATE TF2_STANDALONE_STATIC_BUILD)
else()
  # Shared build: tf2 is PUBLIC (consumer links dynamically)
  target_link_libraries(${PROJECT_NAME}
    PUBLIC
      tf2::tf2
    PRIVATE
      ${geometry_msgs_TARGETS}
  )

  # Shared library settings
  set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON
  )
  target_compile_definitions(${PROJECT_NAME} PRIVATE TF2_STANDALONE_BUILDING_DLL)
endif()

# Position independent code (needed for shared libs, harmless for static)
set_target_properties(${PROJECT_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Installation
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(BUILD_SHARED_LIBS)
  install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}-targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
  )
else()
  # Static: install individual archives, then combine into a fat archive.
  # We don't use EXPORT here — we write a custom targets file below.
  install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
  )

  # Locate libtf2.a from the tf2 package
  get_target_property(_tf2_type tf2::tf2 TYPE)
  if(_tf2_type STREQUAL "STATIC_LIBRARY")
    set(_tf2_archive "$<TARGET_FILE:tf2::tf2>")
  else()
    # tf2 was built as shared+static — look for the .a alongside the .so
    get_target_property(_tf2_loc tf2::tf2 LOCATION)
    get_filename_component(_tf2_libdir "${_tf2_loc}" DIRECTORY)
    set(_tf2_archive "${_tf2_libdir}/libtf2.a")
  endif()

  # Create fat archive at install time: combine tf2_standalone + tf2
  install(CODE "
    set(_workdir \"\${CMAKE_INSTALL_PREFIX}/lib/_fat_archive_tmp\")
    file(MAKE_DIRECTORY \${_workdir})

    # Extract objects from each archive into separate subdirs to avoid name collisions
    set(_idx 0)
    foreach(_lib
      \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/lib${PROJECT_NAME}.a\"
      \"${_tf2_archive}\")
      if(EXISTS \${_lib})
        set(_subdir \"\${_workdir}/\${_idx}\")
        file(MAKE_DIRECTORY \${_subdir})
        execute_process(COMMAND \${CMAKE_COMMAND} -E chdir \${_subdir}
          ar x \${_lib} COMMAND_ERROR_IS_FATAL ANY)
        math(EXPR _idx \"\${_idx} + 1\")
      else()
        message(WARNING \"Fat archive: skipping missing \${_lib}\")
      endif()
    endforeach()

    # Combine all objects into the final fat archive
    file(GLOB_RECURSE _objects \"\${_workdir}/*.o\" \"\${_workdir}/*.obj\")
    execute_process(COMMAND ar rcs
      \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/lib${PROJECT_NAME}.a\"
      \${_objects} COMMAND_ERROR_IS_FATAL ANY)

    # Cleanup
    file(REMOVE_RECURSE \${_workdir})

    message(STATUS \"Created fat archive: lib${PROJECT_NAME}.a\")

    # Guard: verify that libtf2.a was compiled with RCUTILS_LOG_MIN_SEVERITY=5
    # so the fat archive has zero rcutils symbol references.
    execute_process(
      COMMAND nm -u \"\${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/lib${PROJECT_NAME}.a\"
      OUTPUT_VARIABLE _nm_output
      OUTPUT_STRIP_TRAILING_WHITESPACE)
    string(FIND \"\${_nm_output}\" \"rcutils\" _rcutils_pos)
    if(NOT \${_rcutils_pos} EQUAL -1)
      message(FATAL_ERROR
        \"Fat archive lib${PROJECT_NAME}.a still contains rcutils symbols.\\n\"
        \"libtf2.a must be compiled with -DRCUTILS_LOG_MIN_SEVERITY=5 to eliminate them.\\n\"
        \"Undefined symbols found:\\n\${_nm_output}\")
    endif()
    message(STATUS \"Verified: no rcutils symbols in lib${PROJECT_NAME}.a\")
  ")
endif()

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

# Copy tf2's LinearMath headers at install time (avoids duplicating them in source)
get_target_property(TF2_INCLUDE_DIRS tf2::tf2 INTERFACE_INCLUDE_DIRECTORIES)
# Handle generator expressions - extract the actual path (BUILD_INTERFACE or INSTALL_INTERFACE)
set(TF2_INCLUDE_DIR "")
foreach(_dir ${TF2_INCLUDE_DIRS})
  if(_dir MATCHES "\\$<BUILD_INTERFACE:(.+)>")
    set(TF2_INCLUDE_DIR "${CMAKE_MATCH_1}")
    break()
  elseif(_dir MATCHES "\\$<INSTALL_INTERFACE:(.+)>")
    # Skip install interface, prefer build interface
  elseif(IS_DIRECTORY "${_dir}/tf2/LinearMath")
    # Plain path (from installed package)
    set(TF2_INCLUDE_DIR "${_dir}")
    break()
  endif()
endforeach()

if(TF2_INCLUDE_DIR STREQUAL "")
  message(FATAL_ERROR "Could not find tf2 include directory")
endif()

# Install LinearMath from tf2
install(DIRECTORY ${TF2_INCLUDE_DIR}/tf2/LinearMath
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/tf2)

# Install visibility_control.h from tf2
install(FILES ${TF2_INCLUDE_DIR}/tf2/visibility_control.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/tf2)

# Install time.hpp from tf2 (needed by public API)
install(FILES ${TF2_INCLUDE_DIR}/tf2/time.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/tf2)

# Install exceptions.hpp from tf2 (needed by public API)
install(FILES ${TF2_INCLUDE_DIR}/tf2/exceptions.hpp
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}/tf2)

if(BUILD_SHARED_LIBS)
  install(EXPORT ${PROJECT_NAME}-targets
    FILE ${PROJECT_NAME}-targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )
else()
  # Static: generate a custom targets file for the self-contained fat archive
  configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-targets-static.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake
    @ONLY
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-targets.cmake
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
  )
endif()

set(TF2_STANDALONE_SHARED ${BUILD_SHARED_LIBS})
configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

# Testing
if(BUILD_TESTING)
  find_package(ament_cmake_gtest QUIET)
  if(ament_cmake_gtest_FOUND)
    ament_add_gtest(test_transform_buffer test/test_transform_buffer.cpp)
    target_link_libraries(test_transform_buffer ${PROJECT_NAME})
  else()
    find_package(GTest QUIET)
    if(GTest_FOUND)
      enable_testing()
      add_executable(test_transform_buffer test/test_transform_buffer.cpp)
      target_link_libraries(test_transform_buffer ${PROJECT_NAME} GTest::gtest_main)
      add_test(NAME test_transform_buffer COMMAND test_transform_buffer)
    endif()
  endif()
endif()

# Ament is the build system, but this package is not intended for running in a ROS environment, so we don't declare any ROS dependencies or exports.
ament_package()
