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

#pragma once

#ifndef GEODE_TESTOBJECT_DELTAFASTASSETACCOUNT_H_
#define GEODE_TESTOBJECT_DELTAFASTASSETACCOUNT_H_

/*
 * @brief User class for testing the query functionality.
 */
#include <string>

#include <ace/ACE.h>
#include <ace/OS.h>
#include <ace/Time_Value.h>

#include <geode/Delta.hpp>

#include "fwklib/FrameworkTest.hpp"

#include "FastAsset.hpp"

#ifdef _WIN32
#ifdef BUILD_TESTOBJECT
#define TESTOBJECT_EXPORT _GEODE_LIBEXP
#else
#define TESTOBJECT_EXPORT _GEODE_LIBIMP
#endif
#else
#define TESTOBJECT_EXPORT
#endif

using namespace apache::geode::client;
using namespace testframework;
namespace testobject {

class TESTOBJECT_EXPORT DeltaFastAssetAccount : public Cacheable, public Delta {
 private:
  bool encodeTimestamp;
  int32_t acctId;
  std::shared_ptr<CacheableString> customerName;
  double netWorth;
  std::shared_ptr<CacheableHashMap> assets;
  uint64_t timestamp;
  bool getBeforeUpdate;

  inline size_t getObjectSize(const std::shared_ptr<Serializable>& obj) const {
    return (obj == nullptr ? 0 : obj->objectSize());
  }

 public:
  DeltaFastAssetAccount()
      : Delta(nullptr),
        encodeTimestamp(0),
        acctId(0),
        customerName(nullptr),
        netWorth(0.0),
        assets(nullptr),
        timestamp(0),
        getBeforeUpdate(false) {}
  DeltaFastAssetAccount(int index, bool encodeTimestp, int maxVal,
                        int asstSize = 0, bool getbfrUpdate = false);

  ~DeltaFastAssetAccount() noexcept override {}
  void toData(apache::geode::client::DataOutput& output) const override;
  void fromData(apache::geode::client::DataInput& input) override;
  void toDelta(apache::geode::client::DataOutput& output) const override;
  void fromDelta(apache::geode::client::DataInput& input) override;

  std::string toString() const override {
    char buf[102500];
    sprintf(buf,
            "DeltaFastAssetAccount:[acctId = %d customerName = %s netWorth = "
            "%f timestamp = %lld]",
            acctId, customerName->toString().c_str(), netWorth, timestamp);
    return buf;
  }

  int getAcctId() { return acctId; }

  std::shared_ptr<CacheableString> getCustomerName() { return customerName; }

  double getNetWorth() { return netWorth; }

  void incrementNetWorth() { ++netWorth; }

  std::shared_ptr<CacheableHashMap> getAssets() { return assets; }
  int getIndex() { return acctId; }
  uint64_t getTimestamp() {
    if (encodeTimestamp) {
      return timestamp;
    } else {
      return 0;
    }
  }

  void resetTimestamp() {
    if (encodeTimestamp) {
      ACE_Time_Value startTime;
      startTime = ACE_OS::gettimeofday();
      ACE_UINT64 tusec;
      startTime.to_usec(tusec);
      timestamp = tusec * 1000;
    } else {
      timestamp = 0;
    }
  }

  void update() {
    incrementNetWorth();
    if (encodeTimestamp) {
      resetTimestamp();
    }
  }

  int32_t classId() const override { return 41; }

  bool hasDelta() const override { return true; }

  size_t objectSize() const override {
    auto objectSize = sizeof(DeltaFastAssetAccount);
    return objectSize;
  }

  virtual std::shared_ptr<Delta> clone() const override {
    auto clonePtr = std::make_shared<DeltaFastAssetAccount>();
    clonePtr->assets = CacheableHashMap::create();
    for (const auto& item : *(this->assets)) {
      auto key = std::dynamic_pointer_cast<CacheableInt32>(item.first);
      auto asset = std::dynamic_pointer_cast<FastAsset>(item.second);
      clonePtr->assets->emplace(key, asset->copy());
    }
    return clonePtr;
  }

  static apache::geode::client::Serializable* createDeserializable() {
    return new DeltaFastAssetAccount();
  }
};
}  // namespace testobject

#endif  // GEODE_TESTOBJECT_DELTAFASTASSETACCOUNT_H_
