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

#ifndef TF2_STANDALONE__EXCEPTIONS_HPP_
#define TF2_STANDALONE__EXCEPTIONS_HPP_

#include <stdexcept>
#include <string>

namespace tf2_standalone
{

/// Base exception for all transform errors
class TransformException : public std::runtime_error
{
public:
  explicit TransformException(const std::string & message)
  : std::runtime_error(message) {}
};

/// Exception thrown when a frame is not found in the transform tree
class LookupException : public TransformException
{
public:
  explicit LookupException(const std::string & message)
  : TransformException(message) {}
};

/// Exception thrown when there is no connection between frames in the tree
class ConnectivityException : public TransformException
{
public:
  explicit ConnectivityException(const std::string & message)
  : TransformException(message) {}
};

/// Exception thrown when a transform is requested at a time outside the cache
class ExtrapolationException : public TransformException
{
public:
  explicit ExtrapolationException(const std::string & message)
  : TransformException(message) {}
};

/// Exception thrown when an invalid argument is passed
class InvalidArgumentException : public TransformException
{
public:
  explicit InvalidArgumentException(const std::string & message)
  : TransformException(message) {}
};

/// Exception thrown when a timeout occurs waiting for a transform
class TimeoutException : public TransformException
{
public:
  explicit TimeoutException(const std::string & message)
  : TransformException(message) {}
};

}  // namespace tf2_standalone

#endif  // TF2_STANDALONE__EXCEPTIONS_HPP_
