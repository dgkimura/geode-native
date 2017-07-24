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
/*
 * PdxSerializable.cpp
 *
 *  Created on: Sep 29, 2011
 *      Author: npatel
 */

#include <geode/PdxSerializable.hpp>
#include <GeodeTypeIdsImpl.hpp>
#include <geode/CacheableString.hpp>
#include <PdxHelper.hpp>
#include <geode/CacheableKeys.hpp>

namespace apache {
namespace geode {
namespace client {
PdxSerializable::PdxSerializable() {}

PdxSerializable::~PdxSerializable() {}

int8_t PdxSerializable::typeId() const {
  return static_cast<int8_t>(GeodeTypeIdsImpl::PDX);
}

void PdxSerializable::toData(DataOutput& output) const {
  LOGDEBUG("SerRegistry.cpp:serializePdx:86: PDX Object Type = %s",
           typeid(*this).name());
  PdxHelper::serializePdx(output, *this);
}

Serializable* PdxSerializable::fromData(DataInput& input) {
  throw UnsupportedOperationException(
      "operation PdxSerializable::fromData() is not supported ");
}

CacheableStringPtr PdxSerializable::toString() const {
  return CacheableString::create("PdxSerializable");
}

bool PdxSerializable::operator==(const CacheableKey& other) const {
  return (this == &other);
}

int32_t PdxSerializable::hashcode() const {
  uint64_t hash = static_cast<uint64_t>((intptr_t)this);
  return apache::geode::client::serializer::hashcode(hash);
}
}  // namespace client
}  // namespace geode
}  // namespace apache
