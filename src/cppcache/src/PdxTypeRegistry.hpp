#pragma once

#ifndef GEODE_PDXTYPEREGISTRY_H_
#define GEODE_PDXTYPEREGISTRY_H_

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

#include <geode/PdxSerializable.hpp>
#include "PdxRemotePreservedData.hpp"
#include "ReadWriteLock.hpp"
#include <map>
#include <ace/ACE.h>
#include <ace/Recursive_Thread_Mutex.h>
#include "PdxType.hpp"
#include "EnumInfo.hpp"
#include "PreservedDataExpiryHandler.hpp"

namespace apache {
namespace geode {
namespace client {

struct PdxTypeLessThan {
  bool operator()(PdxTypePtr const& n1, PdxTypePtr const& n2) const {
    // call to PdxType::operator <()
    return *n1 < *n2;
  }
};

typedef std::map<int32_t, PdxTypePtr> TypeIdVsPdxType;
typedef std::map</*char**/ std::string, PdxTypePtr> TypeNameVsPdxType;
typedef HashMapT<PdxSerializablePtr, PdxRemotePreservedDataPtr>
    PreservedHashMap;
typedef std::map<PdxTypePtr, int32_t, PdxTypeLessThan> PdxTypeToTypeIdMap;


class CPPCACHE_EXPORT PdxTypeRegistry {
 private:
  TypeIdVsPdxType* typeIdToPdxType;

  TypeIdVsPdxType* remoteTypeIdToMergedPdxType;

  TypeNameVsPdxType* localTypeToPdxType;

  // TODO:: preserveData need to be of type WeakHashMap
  // static std::map<PdxSerializablePtr , PdxRemotePreservedDataPtr>
  // *preserveData;
  // static CacheableHashMapPtr preserveData;
  PreservedHashMap preserveData;

  ACE_RW_Thread_Mutex g_readerWriterLock;

  ACE_RW_Thread_Mutex g_preservedDataLock;

  bool pdxIgnoreUnreadFields;

  bool pdxReadSerialized;

  CacheableHashMapPtr enumToInt;

  CacheableHashMapPtr intToEnum;

 public:
  PdxTypeRegistry();

  virtual ~PdxTypeRegistry();

  void init();

  void cleanup();

  // test hook;
  int testGetNumberOfPdxIds();

  // test hook
  int testNumberOfPreservedData();

  void addPdxType(int32_t typeId, PdxTypePtr pdxType);

  PdxTypePtr getPdxType(int32_t typeId);

  void addLocalPdxType(const char* localType, PdxTypePtr pdxType);

  // newly added
  PdxTypePtr getLocalPdxType(const char* localType);

  void setMergedType(int32_t remoteTypeId, PdxTypePtr mergedType);

  PdxTypePtr getMergedType(int32_t remoteTypeId);

  void setPreserveData(PdxSerializablePtr obj,
                              PdxRemotePreservedDataPtr preserveDataPtr);

  PdxRemotePreservedDataPtr getPreserveData(PdxSerializablePtr obj);

  void clear();

  int32_t getPDXIdForType(const char* type, const char* poolname,
                               PdxTypePtr nType, bool checkIfThere);

  bool getPdxIgnoreUnreadFields() { return pdxIgnoreUnreadFields; }

  void setPdxIgnoreUnreadFields(bool value) {
    pdxIgnoreUnreadFields = value;
  }

  void setPdxReadSerialized(bool value) { pdxReadSerialized = value; }

  bool getPdxReadSerialized() { return pdxReadSerialized; }

  inline PreservedHashMap& getPreserveDataMap() { return preserveData; };

  int32_t getEnumValue(EnumInfoPtr ei);

  EnumInfoPtr getEnum(int32_t enumVal);

  int32_t getPDXIdForType(PdxTypePtr nType, const char* poolname);

  ACE_RW_Thread_Mutex& getPreservedDataLock() {
    return g_preservedDataLock;
  }

 private:
  PdxTypeToTypeIdMap* pdxTypeToTypeIdMap;
};

CPPCACHE_EXPORT PdxTypeRegistry *  getPdxTypeRegistry();

}  // namespace client
}  // namespace geode
}  // namespace apache

#endif  // GEODE_PDXTYPEREGISTRY_H_
