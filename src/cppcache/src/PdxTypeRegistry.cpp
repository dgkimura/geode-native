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
 * PdxTypeRegistry.cpp
 *
 *  Created on: Dec 9, 2011
 *      Author: npatel
 */

#include "PdxTypeRegistry.hpp"
#include "SerializationRegistry.hpp"

namespace apache {
namespace geode {
namespace client {



// TODO::Add support for weakhashmap
// std::map<PdxSerializablePtr, PdxRemotePreservedDataPtr>
// *PdxTypeRegistry::preserveData = NULL;
//PreservedHashMap PdxTypeRegistry::preserveData;
    static PdxTypeRegistry* theGlobalPdxTypeRegistry = NULL;
    PdxTypeRegistry *getPdxTypeRegistry() {
      if (theGlobalPdxTypeRegistry == NULL)
      {
        theGlobalPdxTypeRegistry = new PdxTypeRegistry();
      }
      return theGlobalPdxTypeRegistry;
    }

PdxTypeRegistry::PdxTypeRegistry()
: typeIdToPdxType(NULL),
  remoteTypeIdToMergedPdxType(NULL),
  localTypeToPdxType(NULL),
  enumToInt(nullptr) ,
  intToEnum(nullptr),
  pdxTypeToTypeIdMap(NULL)

{}

PdxTypeRegistry::~PdxTypeRegistry() {}

void PdxTypeRegistry::init() {
  // try{
  typeIdToPdxType = new TypeIdVsPdxType();
  remoteTypeIdToMergedPdxType = new TypeIdVsPdxType();
  localTypeToPdxType = new TypeNameVsPdxType();
  // preserveData = CacheableHashMap::create();
  // preserveData = PreservedHashMapPtr(new PreservedHashMap());
  enumToInt = CacheableHashMap::create();
  intToEnum = CacheableHashMap::create();
  pdxTypeToTypeIdMap = new PdxTypeToTypeIdMap();
  /*}catch(const std::bad_alloc&){
  throw apache::geode::client::OutOfMemoryException( "Out of Memory while
  executing new in
  PdxTypeRegistry::init()");
  }*/
}

void PdxTypeRegistry::cleanup() {
  clear();
  GF_SAFE_DELETE(typeIdToPdxType);
  GF_SAFE_DELETE(remoteTypeIdToMergedPdxType);
  GF_SAFE_DELETE(localTypeToPdxType);
  GF_SAFE_DELETE(pdxTypeToTypeIdMap);
  intToEnum = nullptr;
  enumToInt = nullptr;
  // GF_SAFE_DELETE(preserveData);
}

int PdxTypeRegistry::testGetNumberOfPdxIds() {
  return static_cast<int>(typeIdToPdxType->size());
}

int PdxTypeRegistry::testNumberOfPreservedData() { return preserveData.size(); }

int32_t PdxTypeRegistry::getPDXIdForType(const char* type, const char* poolname,
                                       PdxTypePtr nType, bool checkIfThere) {
  // WriteGuard guard(g_readerWriterLock);
  if (checkIfThere) {
    PdxTypePtr lpdx = getLocalPdxType(type);
    if (lpdx != nullptr) {
      int id = lpdx->getTypeId();
      if (id != 0) {
        return id;
      }
    }
  }

  int typeId = SerializationRegistry::GetPDXIdForType(poolname, nType);
  nType->setTypeId(typeId);

  addPdxType(typeId, nType);
  return typeId;
}

int32_t PdxTypeRegistry::getPDXIdForType(PdxTypePtr nType, const char* poolname) {
  PdxTypeToTypeIdMap* tmp = pdxTypeToTypeIdMap;
  int32_t typeId = 0;
  PdxTypeToTypeIdMap::iterator iter = tmp->find(nType);
  if (iter != tmp->end()) {
    typeId = iter->second;
    if (typeId != 0) {
      return typeId;
    }
  }
  /*WriteGuard guard(g_readerWriterLock);
  tmp = pdxTypeToTypeIdMap;
  if(tmp->find(nType) != tmp->end()) {
    typeId = tmp->operator[](nType);
    if (typeId != 0) {
      return typeId;
    }
  }*/
  typeId = SerializationRegistry::GetPDXIdForType(poolname, nType);
  nType->setTypeId(typeId);
  tmp = pdxTypeToTypeIdMap;
  tmp->insert(std::make_pair(nType, typeId));
  pdxTypeToTypeIdMap = tmp;
  addPdxType(typeId, nType);
  return typeId;
}

void PdxTypeRegistry::clear() {
  {
    WriteGuard guard(g_readerWriterLock);
    if (typeIdToPdxType != NULL) typeIdToPdxType->clear();

    if (remoteTypeIdToMergedPdxType != NULL) {
      remoteTypeIdToMergedPdxType->clear();
    }

    if (localTypeToPdxType != NULL) localTypeToPdxType->clear();

    if (intToEnum != nullptr) intToEnum->clear();

    if (enumToInt != nullptr) enumToInt->clear();

    if (pdxTypeToTypeIdMap != NULL) pdxTypeToTypeIdMap->clear();
  }
  {
    WriteGuard guard(getPreservedDataLock());
    preserveData.clear();
  }
}

void PdxTypeRegistry::addPdxType(int32_t typeId, PdxTypePtr pdxType) {
  WriteGuard guard(g_readerWriterLock);
  std::pair<int32_t, PdxTypePtr> pc(typeId, pdxType);
  typeIdToPdxType->insert(pc);
}

PdxTypePtr PdxTypeRegistry::getPdxType(int32_t typeId) {
  ReadGuard guard(g_readerWriterLock);
  PdxTypePtr retValue = nullptr;
  TypeIdVsPdxType::iterator iter;
  iter = typeIdToPdxType->find(typeId);
  if (iter != typeIdToPdxType->end()) {
    retValue = (*iter).second;
    return retValue;
  }
  return nullptr;
}

void PdxTypeRegistry::addLocalPdxType(const char* localType,
                                      PdxTypePtr pdxType) {
  WriteGuard guard(g_readerWriterLock);
  localTypeToPdxType->insert(
      std::pair<std::string, PdxTypePtr>(localType, pdxType));
}

PdxTypePtr PdxTypeRegistry::getLocalPdxType(const char* localType) {
  ReadGuard guard(g_readerWriterLock);
  PdxTypePtr localTypePtr = nullptr;
  TypeNameVsPdxType::iterator it;
  it = localTypeToPdxType->find(localType);
  if (it != localTypeToPdxType->end()) {
    localTypePtr = (*it).second;
    return localTypePtr;
  }
  return nullptr;
}

void PdxTypeRegistry::setMergedType(int32_t remoteTypeId, PdxTypePtr mergedType) {
  WriteGuard guard(g_readerWriterLock);
  std::pair<int32_t, PdxTypePtr> mergedTypePair(remoteTypeId, mergedType);
  remoteTypeIdToMergedPdxType->insert(mergedTypePair);
}

PdxTypePtr PdxTypeRegistry::getMergedType(int32_t remoteTypeId) {
  PdxTypePtr retVal = nullptr;
  TypeIdVsPdxType::iterator it;
  it = remoteTypeIdToMergedPdxType->find(remoteTypeId);
  if (it != remoteTypeIdToMergedPdxType->end()) {
    retVal = (*it).second;
    return retVal;
  }
  return retVal;
}

void PdxTypeRegistry::setPreserveData(PdxSerializablePtr obj,
                                      PdxRemotePreservedDataPtr pData) {
  WriteGuard guard(getPreservedDataLock());
  pData->setOwner(obj);
  if (preserveData.contains(obj)) {
    // reset expiry task
    // TODO: check value for NULL
    long expTaskId = preserveData[obj]->getPreservedDataExpiryTaskId();
    CacheImpl::expiryTaskManager->resetTask(expTaskId, 5);
    LOGDEBUG("PdxTypeRegistry::setPreserveData Reset expiry task Done");
    pData->setPreservedDataExpiryTaskId(expTaskId);
    preserveData[obj] = pData;
  } else {
    // schedule new expiry task
    PreservedDataExpiryHandler* handler =
        new PreservedDataExpiryHandler(obj, 20);
    long id =
        CacheImpl::expiryTaskManager->scheduleExpiryTask(handler, 20, 0, false);
    pData->setPreservedDataExpiryTaskId(id);
    LOGDEBUG(
        "PdxTypeRegistry::setPreserveData Schedule new expirt task with id=%ld",
        id);
    preserveData.insert(obj, pData);
  }

  LOGDEBUG(
      "PdxTypeRegistry::setPreserveData Successfully inserted new entry in "
      "preservedData");
}

PdxRemotePreservedDataPtr PdxTypeRegistry::getPreserveData(
    PdxSerializablePtr pdxobj) {
  ReadGuard guard(getPreservedDataLock());
  PreservedHashMap::Iterator iter = preserveData.find((pdxobj));
  if (iter != preserveData.end()) {
    PdxRemotePreservedDataPtr retValPtr = iter.second();
    return retValPtr;
  }
  return nullptr;
}

int32_t PdxTypeRegistry::getEnumValue(EnumInfoPtr ei) {
  // TODO locking - naive concurrent optimization?
  CacheableHashMapPtr tmp;
  tmp = enumToInt;
  if (tmp->contains(ei)) {
    auto val = std::static_pointer_cast<CacheableInt32>(tmp->operator[](ei));
    return val->value();
  }
  WriteGuard guard(g_readerWriterLock);
  tmp = enumToInt;
  if (tmp->contains(ei)) {
    auto val = std::static_pointer_cast<CacheableInt32>(tmp->operator[](ei));
    return val->value();
  }
  int val = SerializationRegistry::GetEnumValue(ei);
  tmp = enumToInt;
  tmp->update(ei, CacheableInt32::create(val));
  enumToInt = tmp;
  return val;
}

EnumInfoPtr PdxTypeRegistry::getEnum(int32_t enumVal) {
  // TODO locking - naive concurrent optimization?
  EnumInfoPtr ret;
  CacheableHashMapPtr tmp;
  auto enumValPtr = CacheableInt32::create(enumVal);
  tmp = intToEnum;
  if (tmp->contains(enumValPtr)) {
    ret = std::static_pointer_cast<EnumInfo>(tmp->operator[](enumValPtr));
  }

  if (ret != nullptr) {
    return ret;
  }

  WriteGuard guard(g_readerWriterLock);
  tmp = intToEnum;
  if (tmp->contains(enumValPtr)) {
    ret = std::static_pointer_cast<EnumInfo>(tmp->operator[](enumValPtr));
  }

  if (ret != nullptr) {
    return ret;
  }

  ret = std::static_pointer_cast<EnumInfo>(
      SerializationRegistry::GetEnum(enumVal));
  tmp = intToEnum;
  tmp->update(enumValPtr, ret);
  intToEnum = tmp;
  return ret;
}
}  // namespace client
}  // namespace geode
}  // namespace apache
