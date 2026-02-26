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

#include "tf2_standalone/transform_buffer.hpp"

#include <chrono>
#include <memory>
#include <string>
#include <vector>

// Internal includes (hidden from users)
#include "geometry_msgs/msg/transform_stamped.hpp"
#include "tf2/buffer_core.hpp"

namespace tf2_standalone
{

namespace
{

// Convert tf2::Transform to geometry_msgs::msg::TransformStamped
geometry_msgs::msg::TransformStamped toMsg(
  const std::string & parent_frame,
  const std::string & child_frame,
  const tf2::Transform & transform,
  tf2::TimePoint stamp)
{
  geometry_msgs::msg::TransformStamped msg;

  msg.header.frame_id = parent_frame;
  auto stamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
    stamp.time_since_epoch());
  auto stamp_sec = std::chrono::duration_cast<std::chrono::seconds>(stamp_ns);
  msg.header.stamp.sec = static_cast<int32_t>(stamp_sec.count());
  msg.header.stamp.nanosec = static_cast<uint32_t>(
    (stamp_ns - stamp_sec).count());

  msg.child_frame_id = child_frame;

  const tf2::Vector3 & origin = transform.getOrigin();
  msg.transform.translation.x = origin.x();
  msg.transform.translation.y = origin.y();
  msg.transform.translation.z = origin.z();

  const tf2::Quaternion & rotation = transform.getRotation();
  msg.transform.rotation.x = rotation.x();
  msg.transform.rotation.y = rotation.y();
  msg.transform.rotation.z = rotation.z();
  msg.transform.rotation.w = rotation.w();

  return msg;
}

// Convert geometry_msgs::msg::TransformStamped to tf2::Transform
tf2::Transform fromMsg(const geometry_msgs::msg::TransformStamped & msg)
{
  tf2::Vector3 origin(
    msg.transform.translation.x,
    msg.transform.translation.y,
    msg.transform.translation.z);

  tf2::Quaternion rotation(
    msg.transform.rotation.x,
    msg.transform.rotation.y,
    msg.transform.rotation.z,
    msg.transform.rotation.w);

  return tf2::Transform(rotation, origin);
}

}  // namespace

// PIMPL implementation
class TransformBuffer::Impl
{
public:
  explicit Impl(tf2::Duration cache_time)
  : buffer_core_(cache_time)
  {
  }

  tf2::BufferCore buffer_core_;
};

TransformBuffer::TransformBuffer(tf2::Duration cache_time)
: impl_(std::make_unique<Impl>(cache_time))
{
}

TransformBuffer::~TransformBuffer() = default;

bool TransformBuffer::setTransform(
  const std::string & parent_frame,
  const std::string & child_frame,
  const tf2::Transform & transform,
  tf2::TimePoint stamp,
  const std::string & authority,
  bool is_static)
{
  auto msg = toMsg(parent_frame, child_frame, transform, stamp);
  return impl_->buffer_core_.setTransform(msg, authority, is_static);
}

tf2::Transform TransformBuffer::lookupTransform(
  const std::string & target_frame,
  const std::string & source_frame,
  tf2::TimePoint time) const
{
  auto msg = impl_->buffer_core_.lookupTransform(target_frame, source_frame, time);
  return fromMsg(msg);
}

tf2::Transform TransformBuffer::lookupTransform(
  const std::string & target_frame,
  tf2::TimePoint target_time,
  const std::string & source_frame,
  tf2::TimePoint source_time,
  const std::string & fixed_frame) const
{
  auto msg = impl_->buffer_core_.lookupTransform(
    target_frame, target_time,
    source_frame, source_time,
    fixed_frame);
  return fromMsg(msg);
}

bool TransformBuffer::canTransform(
  const std::string & target_frame,
  const std::string & source_frame,
  tf2::TimePoint time,
  std::string * error_msg) const
{
  return impl_->buffer_core_.canTransform(target_frame, source_frame, time, error_msg);
}

bool TransformBuffer::canTransform(
  const std::string & target_frame,
  tf2::TimePoint target_time,
  const std::string & source_frame,
  tf2::TimePoint source_time,
  const std::string & fixed_frame,
  std::string * error_msg) const
{
  return impl_->buffer_core_.canTransform(
    target_frame, target_time,
    source_frame, source_time,
    fixed_frame, error_msg);
}

std::vector<std::string> TransformBuffer::getAllFrameNames() const
{
  return impl_->buffer_core_.getAllFrameNames();
}

void TransformBuffer::clear()
{
  impl_->buffer_core_.clear();
}

tf2::Duration TransformBuffer::getCacheLength() const
{
  return impl_->buffer_core_.getCacheLength();
}

bool TransformBuffer::frameExists(const std::string & frame_id) const
{
  // _frameExists is in tf2's "backwards compatibility" section (not formally
  // deprecated, but underscore-prefixed). No proper public replacement exists;
  // getAllFrameNames() would work but is O(n). Acceptable here because we
  // control the tf2 fork bundled in this repo.
  return impl_->buffer_core_._frameExists(frame_id);
}

std::string TransformBuffer::allFramesAsYAML() const
{
  return impl_->buffer_core_.allFramesAsYAML();
}

std::string TransformBuffer::allFramesAsString() const
{
  return impl_->buffer_core_.allFramesAsString();
}

}  // namespace tf2_standalone
