// Copyright 2015-2016, Open Source Robotics Foundation, Inc. All rights reserved.
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
//    * Neither the name of the Open Source Robotics Foundation nor the names of its
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

#ifndef TF2__TIME_H_
#define TF2__TIME_H_

#include <chrono>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <string>

namespace tf2
{

/// Duration type using nanoseconds
using Duration = std::chrono::nanoseconds;

/// TimePoint type using system_clock with nanosecond precision
using TimePoint = std::chrono::time_point<std::chrono::system_clock, Duration>;

using IDuration = std::chrono::duration<int, std::nano>;

/// Zero time constant
static const TimePoint TimePointZero = TimePoint(IDuration::zero());

/// Get the current time
inline TimePoint get_now()
{
  return std::chrono::time_point_cast<Duration>(std::chrono::system_clock::now());
}

/// Convert seconds to Duration
inline Duration durationFromSec(double t_sec)
{
  return std::chrono::duration_cast<Duration>(std::chrono::duration<double>(t_sec));
}

/// Convert seconds to TimePoint
inline TimePoint timeFromSec(double t_sec)
{
  return TimePoint(std::chrono::duration_cast<Duration>(std::chrono::duration<double>(t_sec)));
}

/// Convert Duration to seconds
inline double durationToSec(const tf2::Duration & input)
{
  return std::chrono::duration<double>(input).count();
}

/// Convert TimePoint to seconds
inline double timeToSec(const TimePoint & timepoint)
{
  return std::chrono::duration<double>(timepoint.time_since_epoch()).count();
}

/// Display a TimePoint as a string
inline std::string displayTimePoint(const TimePoint & stamp)
{
  const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(stamp.time_since_epoch());
  const auto nanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(
    stamp.time_since_epoch() - seconds);
  std::stringstream ss;
  ss << seconds.count() << "." << std::setw(9) << std::setfill('0') << nanoseconds.count();
  return ss.str();
}

}  // namespace tf2

#endif  // TF2__TIME_H_
