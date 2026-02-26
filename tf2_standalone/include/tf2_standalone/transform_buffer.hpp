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

#ifndef TF2_STANDALONE__TRANSFORM_BUFFER_HPP_
#define TF2_STANDALONE__TRANSFORM_BUFFER_HPP_

#include <chrono>
#include <memory>
#include <string>
#include <vector>

#include "tf2/LinearMath/Transform.hpp"
#include "tf2/time.hpp"
#include "tf2/exceptions.hpp"
#include "tf2_standalone/visibility_control.h"

namespace tf2_standalone
{

/// Default cache time for transforms (10 seconds)
inline constexpr tf2::Duration DEFAULT_CACHE_TIME = std::chrono::seconds(10);

/**
 * @brief A standalone transform buffer that wraps tf2::BufferCore.
 *
 * This class provides a ROS-agnostic API for coordinate transforms.
 * The public API uses only:
 * - tf2::Transform, tf2::Quaternion, tf2::Vector3 (from tf2)
 * - tf2::TimePoint, tf2::Duration (std::chrono based)
 * - std::string for frame IDs
 *
 * All ROS message types (geometry_msgs) are hidden internally via the PIMPL idiom.
 */
class TF2_STANDALONE_PUBLIC TransformBuffer
{
public:
  /**
   * @brief Construct a new TransformBuffer.
   * @param cache_time How long to keep a history of transforms
   */
  explicit TransformBuffer(tf2::Duration cache_time = DEFAULT_CACHE_TIME);

  /**
   * @brief Destructor.
   */
  ~TransformBuffer();

  // Non-copyable, non-movable
  TransformBuffer(const TransformBuffer &) = delete;
  TransformBuffer & operator=(const TransformBuffer &) = delete;
  TransformBuffer(TransformBuffer &&) = delete;
  TransformBuffer & operator=(TransformBuffer &&) = delete;

  /**
   * @brief Add a transform to the buffer.
   *
   * @param parent_frame The parent frame ID
   * @param child_frame The child frame ID
   * @param transform The transform that takes points from child to parent frame
   * @param stamp The timestamp of the transform
   * @param authority The source of this transform (for debugging)
   * @param is_static If true, this transform is constant over time
   * @return true if the transform was successfully added; false if a frame ID
   *         is empty, both frames resolve to the same name, the transform
   *         contains NaN values, or the quaternion is denormalized.
   *         Note: leading '/' characters are silently stripped from frame IDs.
   */
  [[nodiscard]] bool setTransform(
    const std::string & parent_frame,
    const std::string & child_frame,
    const tf2::Transform & transform,
    tf2::TimePoint stamp,
    const std::string & authority = "default",
    bool is_static = false);

  /**
   * @brief Look up the transform between two frames.
   *
   * @param target_frame The frame to transform into
   * @param source_frame The frame to transform from
   * @param time The time at which to look up the transform (TimePointZero for latest)
   * @return The transform from source_frame to target_frame
   * @throws tf2::InvalidArgumentException if a frame ID is empty or starts with '/'
   * @throws tf2::LookupException if a frame doesn't exist
   * @throws tf2::ConnectivityException if frames are not connected
   * @throws tf2::ExtrapolationException if time is outside the cache (subtypes:
   *         BackwardExtrapolationException, ForwardExtrapolationException,
   *         NoDataForExtrapolationException)
   */
  tf2::Transform lookupTransform(
    const std::string & target_frame,
    const std::string & source_frame,
    tf2::TimePoint time) const;

  /**
   * @brief Look up the transform between two frames at different times.
   *
   * @param target_frame The frame to transform into
   * @param target_time The time at which the target frame should be evaluated
   * @param source_frame The frame to transform from
   * @param source_time The time at which the source frame should be evaluated
   * @param fixed_frame The frame used to bridge the two time queries
   *        (target_frame→fixed_frame at target_time, fixed_frame→source_frame at source_time)
   * @return The transform from source_frame to target_frame
   * @throws tf2::InvalidArgumentException if a frame ID is empty or starts with '/'
   * @throws tf2::LookupException if a frame doesn't exist
   * @throws tf2::ConnectivityException if frames are not connected
   * @throws tf2::ExtrapolationException if time is outside the cache (subtypes:
   *         BackwardExtrapolationException, ForwardExtrapolationException,
   *         NoDataForExtrapolationException)
   */
  tf2::Transform lookupTransform(
    const std::string & target_frame,
    tf2::TimePoint target_time,
    const std::string & source_frame,
    tf2::TimePoint source_time,
    const std::string & fixed_frame) const;

  /**
   * @brief Check if a transform is available.
   *
   * @param target_frame The frame to transform into
   * @param source_frame The frame to transform from
   * @param time The time at which to look up the transform
   * @param error_msg If not null, filled with an error message on failure
   * @return true if the transform is available
   */
  bool canTransform(
    const std::string & target_frame,
    const std::string & source_frame,
    tf2::TimePoint time,
    std::string * error_msg = nullptr) const;

  /**
   * @brief Check if a transform is available between frames at different times.
   *
   * @param target_frame The frame to transform into
   * @param target_time The time at which the target frame should be evaluated
   * @param source_frame The frame to transform from
   * @param source_time The time at which the source frame should be evaluated
   * @param fixed_frame The frame used to bridge the two time queries
   *        (target_frame→fixed_frame at target_time, fixed_frame→source_frame at source_time)
   * @param error_msg If not null, filled with an error message on failure
   * @return true if the transform is available
   */
  bool canTransform(
    const std::string & target_frame,
    tf2::TimePoint target_time,
    const std::string & source_frame,
    tf2::TimePoint source_time,
    const std::string & fixed_frame,
    std::string * error_msg = nullptr) const;

  /**
   * @brief Get all frame names known to the buffer.
   * @return A vector of frame names
   */
  std::vector<std::string> getAllFrameNames() const;

  /**
   * @brief Clear all transforms from the buffer.
   */
  void clear();

  /**
   * @brief Get the cache duration.
   * @return The duration for which transforms are cached
   */
  tf2::Duration getCacheLength() const;

  /**
   * @brief Check if a frame exists in the buffer.
   * @param frame_id The frame to check
   * @return true if the frame exists
   */
  bool frameExists(const std::string & frame_id) const;

  /**
   * @brief Get a YAML representation of all frames (for debugging).
   * @return YAML string describing all frames
   */
  std::string allFramesAsYAML() const;

  /**
   * @brief Get a human-readable string of all frames (for debugging).
   * @return String describing all frames
   */
  std::string allFramesAsString() const;

private:
  class Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace tf2_standalone

#endif  // TF2_STANDALONE__TRANSFORM_BUFFER_HPP_
