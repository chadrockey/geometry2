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

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <string>
#include <vector>

#include "tf2_standalone/transform_buffer.hpp"
#include "tf2/exceptions.hpp"

using namespace std::chrono_literals;

namespace
{

constexpr double EPSILON = 1e-9;

bool quaternionsEqual(const tf2::Quaternion & q1, const tf2::Quaternion & q2)
{
  return std::abs(q1.x() - q2.x()) < EPSILON &&
         std::abs(q1.y() - q2.y()) < EPSILON &&
         std::abs(q1.z() - q2.z()) < EPSILON &&
         std::abs(q1.w() - q2.w()) < EPSILON;
}

bool vectorsEqual(const tf2::Vector3 & v1, const tf2::Vector3 & v2)
{
  return std::abs(v1.x() - v2.x()) < EPSILON &&
         std::abs(v1.y() - v2.y()) < EPSILON &&
         std::abs(v1.z() - v2.z()) < EPSILON;
}

bool transformsEqual(const tf2::Transform & t1, const tf2::Transform & t2)
{
  return vectorsEqual(t1.getOrigin(), t2.getOrigin()) &&
         quaternionsEqual(t1.getRotation(), t2.getRotation());
}

}  // namespace

class TransformBufferTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    buffer_ = std::make_unique<tf2_standalone::TransformBuffer>();
    base_time_ = tf2::get_now();
  }

  std::unique_ptr<tf2_standalone::TransformBuffer> buffer_;
  tf2::TimePoint base_time_;
};

TEST_F(TransformBufferTest, Construction)
{
  EXPECT_NO_THROW(tf2_standalone::TransformBuffer());
  EXPECT_NO_THROW(tf2_standalone::TransformBuffer(5s));
  EXPECT_NO_THROW(tf2_standalone::TransformBuffer(std::chrono::seconds(20)));
}

TEST_F(TransformBufferTest, SetAndLookupIdentity)
{
  tf2::Transform identity = tf2::Transform::getIdentity();

  EXPECT_TRUE(buffer_->setTransform("world", "base", identity, base_time_));

  tf2::Transform result = buffer_->lookupTransform("world", "base", base_time_);
  EXPECT_TRUE(transformsEqual(result, identity));
}

TEST_F(TransformBufferTest, SetAndLookupTranslation)
{
  tf2::Transform transform;
  transform.setIdentity();
  transform.setOrigin(tf2::Vector3(1.0, 2.0, 3.0));

  EXPECT_TRUE(buffer_->setTransform("world", "base", transform, base_time_));

  tf2::Transform result = buffer_->lookupTransform("world", "base", base_time_);
  EXPECT_TRUE(vectorsEqual(result.getOrigin(), tf2::Vector3(1.0, 2.0, 3.0)));
}

TEST_F(TransformBufferTest, SetAndLookupRotation)
{
  tf2::Transform transform;
  transform.setIdentity();
  tf2::Quaternion q;
  q.setRPY(0.1, 0.2, 0.3);
  transform.setRotation(q);

  EXPECT_TRUE(buffer_->setTransform("world", "base", transform, base_time_));

  tf2::Transform result = buffer_->lookupTransform("world", "base", base_time_);
  EXPECT_TRUE(quaternionsEqual(result.getRotation(), q));
}

TEST_F(TransformBufferTest, SetAndLookupFullTransform)
{
  tf2::Transform transform;
  transform.setOrigin(tf2::Vector3(1.5, -2.5, 3.5));
  tf2::Quaternion q;
  q.setRPY(M_PI / 4, M_PI / 6, M_PI / 3);
  transform.setRotation(q);

  EXPECT_TRUE(buffer_->setTransform("world", "sensor", transform, base_time_));

  tf2::Transform result = buffer_->lookupTransform("world", "sensor", base_time_);
  EXPECT_TRUE(transformsEqual(result, transform));
}

TEST_F(TransformBufferTest, LookupInverseTransform)
{
  tf2::Transform transform;
  transform.setOrigin(tf2::Vector3(1.0, 0.0, 0.0));
  transform.setRotation(tf2::Quaternion::getIdentity());

  EXPECT_TRUE(buffer_->setTransform("world", "base", transform, base_time_));

  // Lookup inverse (base -> world)
  tf2::Transform inverse = buffer_->lookupTransform("base", "world", base_time_);
  EXPECT_TRUE(vectorsEqual(inverse.getOrigin(), tf2::Vector3(-1.0, 0.0, 0.0)));
}

TEST_F(TransformBufferTest, ChainedTransforms)
{
  // world -> base: translate by (1, 0, 0)
  tf2::Transform t1;
  t1.setIdentity();
  t1.setOrigin(tf2::Vector3(1.0, 0.0, 0.0));
  EXPECT_TRUE(buffer_->setTransform("world", "base", t1, base_time_));

  // base -> sensor: translate by (0, 1, 0)
  tf2::Transform t2;
  t2.setIdentity();
  t2.setOrigin(tf2::Vector3(0.0, 1.0, 0.0));
  EXPECT_TRUE(buffer_->setTransform("base", "sensor", t2, base_time_));

  // world -> sensor should be (1, 1, 0)
  tf2::Transform result = buffer_->lookupTransform("world", "sensor", base_time_);
  EXPECT_TRUE(vectorsEqual(result.getOrigin(), tf2::Vector3(1.0, 1.0, 0.0)));
}

TEST_F(TransformBufferTest, CanTransform)
{
  tf2::Transform transform = tf2::Transform::getIdentity();
  EXPECT_TRUE(buffer_->setTransform("world", "base", transform, base_time_));

  EXPECT_TRUE(buffer_->canTransform("world", "base", base_time_));
  EXPECT_TRUE(buffer_->canTransform("base", "world", base_time_));
  EXPECT_FALSE(buffer_->canTransform("world", "nonexistent", base_time_));
}

TEST_F(TransformBufferTest, CanTransformWithErrorMsg)
{
  std::string error_msg;
  EXPECT_FALSE(buffer_->canTransform("world", "nonexistent", base_time_, &error_msg));
  EXPECT_FALSE(error_msg.empty());
}

TEST_F(TransformBufferTest, GetAllFrameNames)
{
  tf2::Transform transform = tf2::Transform::getIdentity();
  EXPECT_TRUE(buffer_->setTransform("world", "base", transform, base_time_));
  EXPECT_TRUE(buffer_->setTransform("base", "sensor", transform, base_time_));

  std::vector<std::string> frames = buffer_->getAllFrameNames();
  EXPECT_EQ(frames.size(), 3u);  // world, base, sensor
}

TEST_F(TransformBufferTest, FrameExists)
{
  tf2::Transform transform = tf2::Transform::getIdentity();
  EXPECT_TRUE(buffer_->setTransform("world", "base", transform, base_time_));

  EXPECT_TRUE(buffer_->frameExists("world"));
  EXPECT_TRUE(buffer_->frameExists("base"));
  EXPECT_FALSE(buffer_->frameExists("nonexistent"));
}

TEST_F(TransformBufferTest, Clear)
{
  tf2::Transform transform = tf2::Transform::getIdentity();
  EXPECT_TRUE(buffer_->setTransform("world", "base", transform, base_time_));
  EXPECT_TRUE(buffer_->canTransform("world", "base", base_time_));

  buffer_->clear();

  EXPECT_FALSE(buffer_->canTransform("world", "base", base_time_));
}

TEST_F(TransformBufferTest, StaticTransform)
{
  tf2::Transform transform;
  transform.setOrigin(tf2::Vector3(1.0, 2.0, 3.0));
  transform.setRotation(tf2::Quaternion::getIdentity());

  // Set as static transform
  EXPECT_TRUE(buffer_->setTransform("world", "base", transform, base_time_, "test", true));

  // Should be available at any time
  tf2::TimePoint future_time = base_time_ + std::chrono::seconds(1000);
  EXPECT_TRUE(buffer_->canTransform("world", "base", future_time));

  tf2::Transform result = buffer_->lookupTransform("world", "base", future_time);
  EXPECT_TRUE(vectorsEqual(result.getOrigin(), tf2::Vector3(1.0, 2.0, 3.0)));
}

TEST_F(TransformBufferTest, LookupException)
{
  EXPECT_THROW(
    buffer_->lookupTransform("world", "nonexistent", base_time_),
    tf2::LookupException);
}

TEST_F(TransformBufferTest, MoveConstruction)
{
  tf2::Transform transform = tf2::Transform::getIdentity();
  buffer_->setTransform("world", "base", transform, base_time_);

  tf2_standalone::TransformBuffer moved_buffer = std::move(*buffer_);

  EXPECT_TRUE(moved_buffer.canTransform("world", "base", base_time_));
}

TEST_F(TransformBufferTest, AllFramesAsString)
{
  tf2::Transform transform = tf2::Transform::getIdentity();
  EXPECT_TRUE(buffer_->setTransform("world", "base", transform, base_time_));

  std::string frames_str = buffer_->allFramesAsString();
  EXPECT_FALSE(frames_str.empty());
  EXPECT_NE(frames_str.find("base"), std::string::npos);
}

TEST_F(TransformBufferTest, AllFramesAsYAML)
{
  tf2::Transform transform = tf2::Transform::getIdentity();
  EXPECT_TRUE(buffer_->setTransform("world", "base", transform, base_time_));

  std::string yaml = buffer_->allFramesAsYAML();
  EXPECT_FALSE(yaml.empty());
}

TEST_F(TransformBufferTest, GetCacheLength)
{
  tf2_standalone::TransformBuffer buffer(std::chrono::seconds(20));
  EXPECT_EQ(buffer.getCacheLength(), std::chrono::seconds(20));
}

TEST_F(TransformBufferTest, TimePointZeroGetsLatest)
{
  // Add transforms at different times
  tf2::Transform t1;
  t1.setOrigin(tf2::Vector3(1.0, 0.0, 0.0));
  t1.setRotation(tf2::Quaternion::getIdentity());
  EXPECT_TRUE(buffer_->setTransform("world", "base", t1, base_time_));

  tf2::Transform t2;
  t2.setOrigin(tf2::Vector3(2.0, 0.0, 0.0));
  t2.setRotation(tf2::Quaternion::getIdentity());
  EXPECT_TRUE(buffer_->setTransform("world", "base", t2, base_time_ + 1s));

  // TimePointZero should get the latest
  tf2::Transform result = buffer_->lookupTransform("world", "base", tf2::TimePointZero);
  EXPECT_TRUE(vectorsEqual(result.getOrigin(), tf2::Vector3(2.0, 0.0, 0.0)));
}

TEST_F(TransformBufferTest, LookupTransformFiveArgTimeTravelThroughFixedFrame)
{
  // Scenario: a moving "base" frame relative to a fixed "world" frame,
  // with a static "sensor" attached to "base".
  // Query: where was "sensor" at source_time, expressed in "world" at target_time?
  // The fixed_frame bridges the two time queries.

  // world -> base at t=0: base is at (1, 0, 0)
  tf2::Transform world_base_t0;
  world_base_t0.setIdentity();
  world_base_t0.setOrigin(tf2::Vector3(1.0, 0.0, 0.0));
  EXPECT_TRUE(buffer_->setTransform("world", "base", world_base_t0, base_time_));

  // world -> base at t=1: base has moved to (3, 0, 0)
  tf2::Transform world_base_t1;
  world_base_t1.setIdentity();
  world_base_t1.setOrigin(tf2::Vector3(3.0, 0.0, 0.0));
  EXPECT_TRUE(buffer_->setTransform("world", "base", world_base_t1, base_time_ + 1s));

  // base -> sensor: static offset (0, 1, 0)
  tf2::Transform base_sensor;
  base_sensor.setIdentity();
  base_sensor.setOrigin(tf2::Vector3(0.0, 1.0, 0.0));
  EXPECT_TRUE(buffer_->setTransform("base", "sensor", base_sensor, base_time_, "test", true));

  // 5-arg lookup: sensor at source_time=t0, expressed in world at target_time=t1
  // fixed_frame="world" bridges the two instants.
  // Result = world_base(t1)^-1 * world_base(t0) * base_sensor
  //        = inverse(translate(3,0,0)) * translate(1,0,0) * translate(0,1,0)
  //        = translate(-3,0,0) * translate(1,1,0)
  //        = translate(-2,1,0)
  tf2::Transform result = buffer_->lookupTransform(
    "world", base_time_ + 1s,
    "sensor", base_time_,
    "world");

  // sensor was at (1,1,0) in world at t=0.
  // The 5-arg form re-expresses that in the world frame at t=1, which for a
  // fixed "world" frame gives the same result: (1,1,0).
  EXPECT_TRUE(vectorsEqual(result.getOrigin(), tf2::Vector3(1.0, 1.0, 0.0)));
}

TEST_F(TransformBufferTest, CanTransformFiveArgTimeTravelThroughFixedFrame)
{
  // Set up the same scenario as the 5-arg lookupTransform test
  tf2::Transform world_base_t0;
  world_base_t0.setIdentity();
  world_base_t0.setOrigin(tf2::Vector3(1.0, 0.0, 0.0));
  EXPECT_TRUE(buffer_->setTransform("world", "base", world_base_t0, base_time_));

  tf2::Transform world_base_t1;
  world_base_t1.setIdentity();
  world_base_t1.setOrigin(tf2::Vector3(3.0, 0.0, 0.0));
  EXPECT_TRUE(buffer_->setTransform("world", "base", world_base_t1, base_time_ + 1s));

  tf2::Transform base_sensor;
  base_sensor.setIdentity();
  base_sensor.setOrigin(tf2::Vector3(0.0, 1.0, 0.0));
  EXPECT_TRUE(buffer_->setTransform("base", "sensor", base_sensor, base_time_, "test", true));

  // Should succeed: all frames exist and times are in range
  EXPECT_TRUE(buffer_->canTransform(
    "world", base_time_ + 1s,
    "sensor", base_time_,
    "world"));

  // Should fail: nonexistent target frame
  EXPECT_FALSE(buffer_->canTransform(
    "nonexistent", base_time_ + 1s,
    "sensor", base_time_,
    "world"));

  // Should fail with error message
  std::string error_msg;
  EXPECT_FALSE(buffer_->canTransform(
    "nonexistent", base_time_ + 1s,
    "sensor", base_time_,
    "world", &error_msg));
  EXPECT_FALSE(error_msg.empty());
}

TEST_F(TransformBufferTest, ExtrapolationException)
{
  tf2::Transform transform;
  transform.setIdentity();
  transform.setOrigin(tf2::Vector3(1.0, 0.0, 0.0));

  // Add a single non-static transform at base_time_
  EXPECT_TRUE(buffer_->setTransform("world", "base", transform, base_time_));

  // Querying far outside the cache window should throw ExtrapolationException
  tf2::TimePoint future_time = base_time_ + std::chrono::seconds(100);
  EXPECT_THROW(
    buffer_->lookupTransform("world", "base", future_time),
    tf2::ExtrapolationException);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
