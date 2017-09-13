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

#include <cstdlib>
#include <ace/OS.h>

#include <geode/Exception.hpp>
#include <geode/CacheableString.hpp>
#include <StackTrace.hpp>
#include <ace/TSS_T.h>

#include <string>

namespace apache {
namespace geode {
namespace client {

// globals can only be trusted to initialize to ZERO.
bool Exception::s_exceptionStackTraceEnabled = false;

void Exception::setStackTraces(bool stackTraceEnabled) {
  s_exceptionStackTraceEnabled = stackTraceEnabled;
}

Exception::Exception(const std::string& msg)
  : Exception(msg.c_str()) {
}

Exception::Exception(const char* msg1)
  : std::runtime_error(msg1) {
  if (s_exceptionStackTraceEnabled/* || forceTrace*/) {
    m_stack = std::unique_ptr<StackTrace>();
  }
}

Exception::~Exception() {}

const char _exception_name_Exception[] = "apache::geode::client::Exception";

const char* Exception::getName() const { return _exception_name_Exception; }

const char* Exception::getMessage() const { return what(); }

void Exception::showMessage() const {
  printf("%s: msg = %s\n", this->getName(), what());
}

void Exception::printStackTrace() const {
  showMessage();
  if (m_stack == nullptr) {
    fprintf(stdout, "  No stack available.\n");
  } else {
    m_stack->print();
  }
}

#ifndef _SOLARIS

size_t Exception::getStackTrace(char* buffer, size_t maxLength) const {
  size_t len = 0;
  if (maxLength > 0) {
    std::string traceString;
    if (m_stack == nullptr) {
      traceString = "  No stack available.\n";
    } else {
      m_stack->getString(traceString);
    }
    len = ACE_OS::snprintf(buffer, maxLength, "%s", traceString.c_str());
  }
  return len;
}

#endif

Exception::Exception(const Exception& other)
    : std::runtime_error(other.what()),
      m_stack(other.m_stack) {}

// class to store/clear last server exception in TSS area

class TSSExceptionString {
 private:
  std::string m_exMsg;

 public:
  TSSExceptionString() : m_exMsg() {}
  virtual ~TSSExceptionString() {}

  inline std::string& str() { return m_exMsg; }

  static ACE_TSS<TSSExceptionString> s_tssExceptionMsg;
};

ACE_TSS<TSSExceptionString> TSSExceptionString::s_tssExceptionMsg;

void setTSSExceptionMessage(const char* exMsg) {
  TSSExceptionString::s_tssExceptionMsg->str().clear();
  if (exMsg != nullptr) {
    TSSExceptionString::s_tssExceptionMsg->str().append(exMsg);
  }
}

const char* getTSSExceptionMessage() {
  return TSSExceptionString::s_tssExceptionMsg->str().c_str();
}
}  // namespace client
}  // namespace geode
}  // namespace apache
