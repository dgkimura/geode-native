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

#include "StackTrace.hpp"

namespace apache {
namespace geode {
namespace client {

StackTrace::StackTrace()
  : stacktrace(boost::stacktrace::stacktrace()) {}

StackTrace::~StackTrace() {}

void StackTrace::print() const {
  std::stringstream ss;
  ss << stacktrace;
  printf("%s\n", ss.str().c_str());
}

void StackTrace::getString(std::string& tracestring) const {
  std::stringstream ss;
  ss << stacktrace;
  tracestring = ss.str();
}

}  // namespace client
}  // namespace geode
}  // namespace apache
