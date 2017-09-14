#pragma once

#ifndef GEODE_EXCEPTION_H_
#define GEODE_EXCEPTION_H_

/*
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file
 */

#include <stdexcept>

#include "geode_types.hpp"

namespace apache {
namespace geode {
namespace client {

class Exception;
typedef std::shared_ptr<Exception> ExceptionPtr;

class DistributedSystem;

/**
 * @class Exception Exception.hpp
 * A description of an exception that occurred during a cache operation.
 */
class CPPCACHE_EXPORT Exception : public std::runtime_error {
  /**
   * @brief public methods
   */
 public:
  /** Creates an exception.
   * @param  msg1 message pointer, this is copied into the exception.
   **/
  Exception(const char* msg1);

  Exception(const std::string& msg1);

  /** Creates an exception as a copy of the given other exception.
   * @param  other the original exception.
   *
   **/
  Exception(const Exception& other);

  /**
   * @brief destructor
   */
  virtual ~Exception();

  /** Get a stacktrace string from the location the exception was created.
   */
  virtual std::string getStackTrace() const;

  /** Return the name of this exception type. */
  virtual const char* getName() const;

 protected:
  static bool s_exceptionStackTraceEnabled;

  StackTracePtr m_stack;

 private:
  static void setStackTraces(bool stackTraceEnabled);

  friend class DistributedSystem;
};
}  // namespace client
}  // namespace geode
}  // namespace apache

#endif  // GEODE_EXCEPTION_H_
