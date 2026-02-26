// Copyright 2026, Open Source Robotics Foundation, Inc. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the copyright holder nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

/**
 * @file rcutils_stubs.cpp
 * @brief Stub implementations of rcutils functions for static builds
 *
 * This file provides minimal implementations of the rcutils functions that
 * tf2 depends on, allowing tf2_standalone to be built as a fully standalone
 * static library without runtime dependencies on ROS2 libraries.
 *
 * The following functions are stubbed:
 * - Logging functions (rcutils_log, rcutils_log_internal, etc.) -> no-op
 * - rcutils_snprintf/rcutils_vsnprintf -> standard vsnprintf
 * - rcutils_strerror -> std::strerror + std::strncpy
 * - rcutils_steady_time_now -> std::chrono::steady_clock
 * - rcutils_get_default_allocator -> malloc/free based allocator
 * - rcutils_logging_initialize_with_allocator -> no-op (returns OK)
 *
 * Only used for static builds (BUILD_SHARED_LIBS=OFF).
 */

#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

extern "C" {

// Type definitions (must match rcutils types)
typedef int rcutils_ret_t;
#define RCUTILS_RET_OK 0

typedef int64_t rcutils_time_point_value_t;

typedef struct rcutils_allocator_s
{
  void * (*allocate)(size_t size, void * state);
  void (* deallocate)(void * pointer, void * state);
  void * (*reallocate)(void * pointer, size_t size, void * state);
  void * (*zero_allocate)(size_t number_of_elements, size_t size_of_element, void * state);
  void * state;
} rcutils_allocator_t;

typedef struct rcutils_log_location_s
{
  const char * function_name;
  const char * file_name;
  size_t line_number;
} rcutils_log_location_t;

// Global logging state (stubbed)
bool g_rcutils_logging_initialized = true;  // Pretend we're already initialized

// Error string type (must match rcutils)
#define RCUTILS_ERROR_MESSAGE_MAX_LENGTH 1024

typedef struct rcutils_error_string_s
{
  char str[RCUTILS_ERROR_MESSAGE_MAX_LENGTH];
} rcutils_error_string_t;

// Default allocator implementation using malloc/free
static void * default_allocate(size_t size, void * state)
{
  (void)state;
  return std::malloc(size);
}

static void default_deallocate(void * pointer, void * state)
{
  (void)state;
  std::free(pointer);
}

static void * default_reallocate(void * pointer, size_t size, void * state)
{
  (void)state;
  return std::realloc(pointer, size);
}

static void * default_zero_allocate(size_t number_of_elements, size_t size_of_element, void * state)
{
  (void)state;
  return std::calloc(number_of_elements, size_of_element);
}

rcutils_allocator_t rcutils_get_default_allocator(void)
{
  rcutils_allocator_t allocator;
  allocator.allocate = default_allocate;
  allocator.deallocate = default_deallocate;
  allocator.reallocate = default_reallocate;
  allocator.zero_allocate = default_zero_allocate;
  allocator.state = nullptr;
  return allocator;
}

// Logging functions (all no-ops)
rcutils_ret_t rcutils_logging_initialize_with_allocator(rcutils_allocator_t allocator)
{
  (void)allocator;
  return RCUTILS_RET_OK;
}

rcutils_ret_t rcutils_logging_initialize(void)
{
  return RCUTILS_RET_OK;
}

rcutils_ret_t rcutils_logging_shutdown(void)
{
  return RCUTILS_RET_OK;
}

bool rcutils_logging_logger_is_enabled_for(const char * name, int severity)
{
  (void)name;
  (void)severity;
  return false;  // Disable all logging in standalone mode
}

void rcutils_log_internal(
  const rcutils_log_location_t * location,
  int severity,
  const char * name,
  const char * format,
  ...)
{
  (void)location;
  (void)severity;
  (void)name;
  (void)format;
  // No-op: logging disabled in standalone mode
}

void rcutils_log(
  const rcutils_log_location_t * location,
  int severity,
  const char * name,
  const char * format,
  ...)
{
  (void)location;
  (void)severity;
  (void)name;
  (void)format;
  // No-op: logging disabled in standalone mode
}

// Error handling stubs
rcutils_error_string_t rcutils_get_error_string(void)
{
  rcutils_error_string_t error_string;
  error_string.str[0] = '\0';  // Empty error string
  return error_string;
}

void rcutils_reset_error(void)
{
  // No-op: no error state to reset in standalone mode
}

// snprintf wrapper
int rcutils_snprintf(char * buffer, size_t buffer_size, const char * format, ...)
{
  va_list args;
  va_start(args, format);
  int result = std::vsnprintf(buffer, buffer_size, format, args);
  va_end(args);
  return result;
}

int rcutils_vsnprintf(char * buffer, size_t buffer_size, const char * format, va_list args)
{
  return std::vsnprintf(buffer, buffer_size, format, args);
}

// strerror wrapper (portable C++ — no #ifdefs needed)
void rcutils_strerror(char * buffer, size_t buffer_length)
{
  if (buffer == nullptr || buffer_length == 0) {
    return;
  }
  std::strncpy(buffer, std::strerror(errno), buffer_length - 1);
  buffer[buffer_length - 1] = '\0';
}

// Time function using std::chrono (portable — no #ifdefs needed)
rcutils_ret_t rcutils_steady_time_now(rcutils_time_point_value_t * now)
{
  if (now == nullptr) {
    return -1;  // RCUTILS_RET_INVALID_ARGUMENT
  }
  auto duration = std::chrono::steady_clock::now().time_since_epoch();
  *now = std::chrono::duration_cast<std::chrono::nanoseconds>(duration).count();
  return RCUTILS_RET_OK;
}

}  // extern "C"
