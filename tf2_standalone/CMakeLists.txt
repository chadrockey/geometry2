cmake_minimum_required(VERSION 3.14)
project(tf2_standalone VERSION 1.0.0 LANGUAGES C CXX)

# tf2_standalone: A standalone wrapper around tf2::BufferCore
#
# This library wraps tf2's BufferCore to provide a ROS-agnostic API.
# - Public API uses only tf2:: types (from tf2_types) + std::string
# - Internal geometry_msgs conversions are hidden from users
#
# Build modes (controlled by BUILD_SHARED_LIBS):
# - Shared (default): Links dynamically to libtf2.so, librcutils.so
# - Static: Fully standalone, uses rcutils stubs

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Default to shared libraries (standard CMake pattern)
option(BUILD_SHARED_LIBS "Build shared libraries" ON)

# Find dependencies
find_package(ament_cmake QUIET)
find_package(tf2 REQUIRED)
find_package(tf2_types REQUIRED)
find_package(geometry_msgs REQUIRED)

# Main library - respects BUILD_SHARED_LIBS automatically
add_library(${PROJECT_NAME} src/transform_buffer.cpp)
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include/${PROJECT_NAME}>
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    tf2_types::tf2_types
    tf2::tf2
  PRIVATE
    ${geometry_msgs_TARGETS}
)

# For static builds, add rcutils stubs to avoid runtime dependencies
if(NOT BUILD_SHARED_LIBS)
  # Build rcutils_stubs as a separate static library
  # Users need to link: tf2_standalone, tf2, rcutils_stubs (in that order)
  add_library(rcutils_stubs STATIC src/rcutils_stubs.c)
  set_target_properties(rcutils_stubs PROPERTIES POSITION_INDEPENDENT_CODE ON)

  # Define to let code know we're in standalone mode if needed
  target_compile_definitions(${PROJECT_NAME} PRIVATE TF2_STANDALONE_STATIC_BUILD)

  # Install the stubs library for static builds
  install(TARGETS rcutils_stubs
    EXPORT ${PROJECT_NAME}-targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
else()
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

install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

install(DIRECTORY include/
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${PROJECT_NAME}
)

install(EXPORT ${PROJECT_NAME}-targets
  FILE ${PROJECT_NAME}-targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}
)

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
    if(NOT BUILD_SHARED_LIBS)
      target_link_libraries(test_transform_buffer rcutils_stubs)
    endif()
  else()
    find_package(GTest QUIET)
    if(GTest_FOUND)
      enable_testing()
      add_executable(test_transform_buffer test/test_transform_buffer.cpp)
      target_link_libraries(test_transform_buffer ${PROJECT_NAME} GTest::gtest_main)
      if(NOT BUILD_SHARED_LIBS)
        target_link_libraries(test_transform_buffer rcutils_stubs)
      endif()
      add_test(NAME test_transform_buffer COMMAND test_transform_buffer)
    endif()
  endif()
endif()

# Ament integration (for ROS2 builds)
if(ament_cmake_FOUND)
  ament_export_targets(${PROJECT_NAME}-targets HAS_LIBRARY_TARGET)
  ament_export_dependencies(tf2_types tf2)
  ament_package()
endif()
