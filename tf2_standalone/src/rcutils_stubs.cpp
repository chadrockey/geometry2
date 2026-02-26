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

// No-op stubs for the 11 rcutils symbols that libtf2.a references.
// tf2 only uses rcutils for logging. Since rcutils_logging_logger_is_enabled_for()
// returns false, all other functions are dead code at runtime — they exist only
// to satisfy the linker. Only used for static builds (BUILD_SHARED_LIBS=OFF).

#include <cstddef>
#include <cstdint>

extern "C" {

// The structs must match rcutils's layout since libtf2.a passes them by value.
struct rcutils_allocator_t
{
  void * (*allocate)(size_t, void *);
  void (*deallocate)(void *, void *);
  void * (*reallocate)(void *, size_t, void *);
  void * (*zero_allocate)(size_t, size_t, void *);
  void * state;
};

struct rcutils_log_location_t;

struct rcutils_error_string_t
{
  char str[1024];
};

bool g_rcutils_logging_initialized = true;

rcutils_allocator_t rcutils_get_default_allocator() { return {}; }
int rcutils_logging_initialize_with_allocator(rcutils_allocator_t) { return 0; }
bool rcutils_logging_logger_is_enabled_for(const char *, int) { return false; }
void rcutils_log_internal(const rcutils_log_location_t *, int, const char *, const char *, ...) {}
void rcutils_log(const rcutils_log_location_t *, int, const char *, const char *, ...) {}
rcutils_error_string_t rcutils_get_error_string() { return {}; }
void rcutils_reset_error() {}
int rcutils_snprintf(char *, size_t, const char *, ...) { return 0; }
void rcutils_strerror(char *, size_t) {}
int rcutils_steady_time_now(int64_t *) { return 0; }

}  // extern "C"
