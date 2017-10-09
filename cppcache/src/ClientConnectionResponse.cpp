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
#include "ClientConnectionResponse.hpp"
#include <geode/DataOutput.hpp>
#include <geode/DataInput.hpp>

using namespace apache::geode::client;

void ClientConnectionResponse::fromData(DataInput& input) {
  m_serverFound = input.readBoolean();
  if (m_serverFound) {
    m_server.fromData(input);
  }
}

int8_t ClientConnectionResponse::typeId() const {
  return static_cast<int8_t>(GeodeTypeIdsImpl::ClientConnectionResponse);
}

uint32_t ClientConnectionResponse::objectSize() const {
  return (m_server.objectSize());
}

ServerLocation ClientConnectionResponse::getServerLocation() const {
  return m_server;
}
