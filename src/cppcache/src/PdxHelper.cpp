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
 * PdxHelper.cpp
 *
 *  Created on: Dec 13, 2011
 *      Author: npatel
 */

#include "PdxHelper.hpp"
#include "PdxTypeRegistry.hpp"
#include "PdxWriterWithTypeCollector.hpp"
#include "SerializationRegistry.hpp"
#include "PdxLocalReader.hpp"
#include "PdxRemoteReader.hpp"
#include "PdxType.hpp"
#include "PdxReaderWithTypeCollector.hpp"
#include "PdxInstanceImpl.hpp"
#include "Utils.hpp"
#include "PdxRemoteWriter.hpp"
#include "CacheRegionHelper.hpp"
#include <geode/Cache.hpp>

namespace apache {
namespace geode {
namespace client {
uint8_t PdxHelper::PdxHeader = 8;

PdxHelper::PdxHelper() {}

PdxHelper::~PdxHelper() {}
//    TODO: WWSD
//CacheImpl* PdxHelper::getCacheImpl() {
//  CachePtr cache = CacheFactory::getAnyInstance();
//  if (cache == NULLPTR) {
//    throw IllegalStateException("cache has not been created yet.");
//    ;
//  }
//  if (cache->isClosed()) {
//    throw IllegalStateException("cache has been closed. ");
//  }
//  return CacheRegionHelper::getCacheImpl(cache.ptr());
//}

void PdxHelper::serializePdx(DataOutput& output,
                             const PdxSerializable& pdxObject) {
  serializePdx(
      output,
      std::static_pointer_cast<PdxSerializable>(
          std::const_pointer_cast<Serializable>(pdxObject.shared_from_this())));
}

void PdxHelper::serializePdx(DataOutput& output,
                             const PdxSerializablePtr& pdxObject) {
  const char* pdxClassname = NULL;

  auto pdxII = std::dynamic_pointer_cast<PdxInstanceImpl>(pdxObject);

  if (pdxII != NULL) {
    PdxTypePtr piPt = pdxII->getPdxType();
    if (piPt != nullptr &&
        piPt->getTypeId() ==
            0)  // from pdxInstance factory need to get typeid from server
    {
      int typeId = getPdxTypeRegistry()->getPDXIdForType(piPt, output.getPoolName());
      pdxII->setPdxId(typeId);
    }
    auto plw = std::make_shared<PdxLocalWriter>(output, piPt);
    pdxII->toData(plw);
    plw->endObjectWriting();  // now write typeid
    int len = 0;
    uint8_t* pdxStream = plw->getPdxStream(len);
    pdxII->updatePdxStream(pdxStream, len);

    delete[] pdxStream;

    return;
  }

  const char* pdxType = pdxObject->getClassName();
  pdxClassname = pdxType;
  PdxTypePtr localPdxType = getPdxTypeRegistry()->getLocalPdxType(pdxType);

  if (localPdxType == nullptr) {
    // need to grab type info, as fromdata is not called yet

    PdxWriterWithTypeCollectorPtr ptc =
        std::make_shared<PdxWriterWithTypeCollector>(output, pdxType);
    pdxObject->toData(std::dynamic_pointer_cast<PdxWriter>(ptc));
    PdxTypePtr nType = ptc->getPdxLocalType();

    nType->InitializeType();

    // SerializationRegistry::GetPDXIdForType(output.getPoolName(), nType);
    int32_t nTypeId = getPdxTypeRegistry()->getPDXIdForType(
        pdxType, output.getPoolName(), nType, true);
    nType->setTypeId(nTypeId);

    ptc->endObjectWriting();
    getPdxTypeRegistry()->addLocalPdxType(pdxType, nType);
    getPdxTypeRegistry()->addPdxType(nTypeId, nType);

//    TODO: WWSD
//    //[ToDo] need to write bytes for stats
//    CacheImpl* cacheImpl = PdxHelper::getCacheImpl();
//    if (cacheImpl != NULL) {
//      uint8_t* stPos = const_cast<uint8_t*>(output.getBuffer()) +
//                       ptc->getStartPositionOffset();
//      int pdxLen = PdxHelper::readInt32(stPos);
//      cacheImpl->m_cacheStats->incPdxSerialization(
//          pdxLen + 1 + 2 * 4);  // pdxLen + 93 DSID + len + typeID
//    }

  } else  // we know locasl type, need to see preerved data
  {
    // if object got from server than create instance of RemoteWriter otherwise
    // local writer.

    PdxRemotePreservedDataPtr pd = getPdxTypeRegistry()->getPreserveData(pdxObject);

    // now always remotewriter as we have API Read/WriteUnreadFields
    // so we don't know whether user has used those or not;; Can we do some
    // trick here?
    PdxRemoteWriterPtr prw = nullptr;

    if (pd != nullptr) {
      PdxTypePtr mergedPdxType =
          getPdxTypeRegistry()->getPdxType(pd->getMergedTypeId());
      prw = std::make_shared<PdxRemoteWriter>(output, mergedPdxType, pd);
    } else {
      prw = std::make_shared<PdxRemoteWriter>(output, pdxClassname);
    }
    pdxObject->toData(std::dynamic_pointer_cast<PdxWriter>(prw));
    prw->endObjectWriting();

//    TODO: WWSD
//    //[ToDo] need to write bytes for stats
//    CacheImpl* cacheImpl = PdxHelper::getCacheImpl();
//    if (cacheImpl != NULL) {
//      uint8_t* stPos = const_cast<uint8_t*>(output.getBuffer()) +
//                       prw->getStartPositionOffset();
//      int pdxLen = PdxHelper::readInt32(stPos);
//      cacheImpl->m_cacheStats->incPdxSerialization(
//          pdxLen + 1 + 2 * 4);  // pdxLen + 93 DSID + len + typeID
//    }
  }
}

PdxSerializablePtr PdxHelper::deserializePdx(DataInput& dataInput,
                                             bool forceDeserialize,
                                             int32_t typeId, int32_t length) {
  char* pdxClassname = NULL;
  PdxSerializablePtr pdxObjectptr = nullptr;
  PdxTypePtr pdxLocalType = nullptr;

  PdxTypePtr pType = getPdxTypeRegistry()->getPdxType(typeId);
  if (pType != nullptr) {  // this may happen with PdxInstanceFactory {
    pdxLocalType = getPdxTypeRegistry()->getLocalPdxType(
        pType->getPdxClassName());  // this should be fine for IPdxTypeMapper
  }
  if (pType != nullptr && pdxLocalType != nullptr)  // type found
  {
    pdxClassname = pType->getPdxClassName();
    LOGDEBUG("deserializePdx ClassName = %s, isLocal = %d ",
             pType->getPdxClassName(), pType->isLocal());

    pdxObjectptr = SerializationRegistry::getPdxType(pdxClassname);
    if (pType->isLocal())  // local type no need to read Unread data
    {
      PdxLocalReaderPtr plr =
          std::make_shared<PdxLocalReader>(dataInput, pType, length);
      pdxObjectptr->fromData(std::dynamic_pointer_cast<PdxReader>(plr));
      plr->MoveStream();
    } else {
      PdxRemoteReaderPtr prr =
          std::make_shared<PdxRemoteReader>(dataInput, pType, length);
      pdxObjectptr->fromData(std::dynamic_pointer_cast<PdxReader>(prr));
      PdxTypePtr mergedVersion =
          getPdxTypeRegistry()->getMergedType(pType->getTypeId());

      PdxRemotePreservedDataPtr preserveData =
          prr->getPreservedData(mergedVersion, pdxObjectptr);
      if (preserveData != nullptr) {
        getPdxTypeRegistry()->setPreserveData(
            pdxObjectptr, preserveData);  // it will set data in weakhashmap
      }
      prr->MoveStream();
    }
  } else {
    // type not found; need to get from server
    if (pType == nullptr) {
      pType = std::static_pointer_cast<PdxType>(
          SerializationRegistry::GetPDXTypeById(dataInput.getPoolName(),
                                                typeId));
      pdxLocalType = getPdxTypeRegistry()->getLocalPdxType(pType->getPdxClassName());
    }
    /* adongre  - Coverity II
     * CID 29298: Unused pointer value (UNUSED_VALUE)
     * Pointer "pdxClassname" returned by "pType->getPdxClassName()" is never
     * used.
     * Fix : Commented the line
     */
    // pdxClassname = pType->getPdxClassName();
    pdxObjectptr = SerializationRegistry::getPdxType(pType->getPdxClassName());
    PdxSerializablePtr pdxRealObject = pdxObjectptr;
    if (pdxLocalType == nullptr)  // need to know local type
    {
      PdxReaderWithTypeCollectorPtr prtc =
          std::make_shared<PdxReaderWithTypeCollector>(dataInput, pType,
                                                       length);
      pdxObjectptr->fromData(std::dynamic_pointer_cast<PdxReader>(prtc));

      // Check for the PdxWrapper

      pdxLocalType = prtc->getLocalType();

      if (pType->Equals(pdxLocalType)) {
        getPdxTypeRegistry()->addLocalPdxType(pdxRealObject->getClassName(), pType);
        getPdxTypeRegistry()->addPdxType(pType->getTypeId(), pType);
        pType->setLocal(true);
      } else {
        // Need to know local type and then merge type
        pdxLocalType->InitializeType();
        pdxLocalType->setTypeId(getPdxTypeRegistry()->getPDXIdForType(
            pdxObjectptr->getClassName(), dataInput.getPoolName(), pdxLocalType,
            true));
        pdxLocalType->setLocal(true);
        getPdxTypeRegistry()->addLocalPdxType(pdxRealObject->getClassName(),
                                         pdxLocalType);  // added local type
        getPdxTypeRegistry()->addPdxType(pdxLocalType->getTypeId(), pdxLocalType);

        pType->InitializeType();
        getPdxTypeRegistry()->addPdxType(pType->getTypeId(),
                                    pType);  // adding remote type

        // create merge type
        createMergedType(pdxLocalType, pType, dataInput);

        PdxTypePtr mergedVersion =
            getPdxTypeRegistry()->getMergedType(pType->getTypeId());

        PdxRemotePreservedDataPtr preserveData =
            prtc->getPreservedData(mergedVersion, pdxObjectptr);
        if (preserveData != nullptr) {
          getPdxTypeRegistry()->setPreserveData(pdxObjectptr, preserveData);
        }
      }
      prtc->MoveStream();
    } else {  // remote reader will come here as local type is there
      pType->InitializeType();
      LOGDEBUG("Adding type %d ", pType->getTypeId());
      getPdxTypeRegistry()->addPdxType(pType->getTypeId(),
                                  pType);  // adding remote type
      PdxRemoteReaderPtr prr =
          std::make_shared<PdxRemoteReader>(dataInput, pType, length);
      pdxObjectptr->fromData(std::dynamic_pointer_cast<PdxReader>(prr));

      // Check for PdxWrapper to getObject.

      createMergedType(pdxLocalType, pType, dataInput);

      PdxTypePtr mergedVersion =
          getPdxTypeRegistry()->getMergedType(pType->getTypeId());

      PdxRemotePreservedDataPtr preserveData =
          prr->getPreservedData(mergedVersion, pdxObjectptr);
      if (preserveData != nullptr) {
        getPdxTypeRegistry()->setPreserveData(pdxObjectptr, preserveData);
      }
      prr->MoveStream();
    }
  }
  return pdxObjectptr;
}

PdxSerializablePtr PdxHelper::deserializePdx(DataInput& dataInput,
                                             bool forceDeserialize) {
  if (getPdxTypeRegistry()->getPdxReadSerialized() == false || forceDeserialize) {
    // Read Length
    int32_t len;
    dataInput.readInt(&len);

    int32_t typeId;
    // read typeId
    dataInput.readInt(&typeId);
//    TODO: WWSD
//    CacheImpl* cacheImpl = PdxHelper::getCacheImpl();
//    if (cacheImpl != NULL) {
//      cacheImpl->m_cacheStats->incPdxDeSerialization(len +
//                                                     9);  // pdxLen + 1 + 2*4
//    }
    return PdxHelper::deserializePdx(dataInput, forceDeserialize, (int32_t)typeId,
                                     (int32_t)len);

  } else {
    // Read Length
    int32_t len;
    dataInput.readInt(&len);

    int typeId;
    // read typeId
    dataInput.readInt(&typeId);

    PdxTypePtr pType = getPdxTypeRegistry()->getPdxType(typeId);

    if (pType == nullptr) {
      // TODO shared_ptr why redef?
      PdxTypePtr pType = std::static_pointer_cast<PdxType>(
          SerializationRegistry::GetPDXTypeById(dataInput.getPoolName(),
                                                typeId));
      getPdxTypeRegistry()->addLocalPdxType(pType->getPdxClassName(), pType);
      getPdxTypeRegistry()->addPdxType(pType->getTypeId(), pType);
    }

    // TODO::Enable it once the PdxInstanceImple is CheckedIn.
    auto pdxObject = std::make_shared<PdxInstanceImpl>(
        const_cast<uint8_t*>(dataInput.currentBufferPosition()), len, typeId);

    dataInput.advanceCursor(len);
//  TODO: WWSD
//    CacheImpl* cacheImpl = PdxHelper::getCacheImpl();
//    if (cacheImpl != NULL) {
//      cacheImpl->m_cacheStats->incPdxInstanceCreations();
//    }
    return pdxObject;
  }
}

void PdxHelper::createMergedType(PdxTypePtr localType, PdxTypePtr remoteType,
                                 DataInput& dataInput) {
  PdxTypePtr mergedVersion = localType->mergeVersion(remoteType);

  if (mergedVersion->Equals(localType)) {
    getPdxTypeRegistry()->setMergedType(remoteType->getTypeId(), localType);
  } else if (mergedVersion->Equals(remoteType)) {
    getPdxTypeRegistry()->setMergedType(remoteType->getTypeId(), remoteType);
  } else {  // need to create new version
    mergedVersion->InitializeType();
    if (mergedVersion->getTypeId() == 0) {
      mergedVersion->setTypeId(SerializationRegistry::GetPDXIdForType(
          dataInput.getPoolName(), mergedVersion));
    }

    // getPdxTypeRegistry()->AddPdxType(remoteType->TypeId, mergedVersion);
    getPdxTypeRegistry()->addPdxType(mergedVersion->getTypeId(), mergedVersion);
    getPdxTypeRegistry()->setMergedType(remoteType->getTypeId(), mergedVersion);
    getPdxTypeRegistry()->setMergedType(mergedVersion->getTypeId(), mergedVersion);
  }
}

int32_t PdxHelper::readInt32(uint8_t* offsetPosition) {
  int32_t data = offsetPosition[0];
  data = (data << 8) | offsetPosition[1];
  data = (data << 8) | offsetPosition[2];
  data = (data << 8) | offsetPosition[3];

  return data;
}

void PdxHelper::writeInt32(uint8_t* offsetPosition, int32_t value) {
  offsetPosition[0] = static_cast<uint8_t>(value >> 24);
  offsetPosition[1] = static_cast<uint8_t>(value >> 16);
  offsetPosition[2] = static_cast<uint8_t>(value >> 8);
  offsetPosition[3] = static_cast<uint8_t>(value);
}

int32_t PdxHelper::readInt16(uint8_t* offsetPosition) {
  int16_t data = offsetPosition[0];
  data = (data << 8) | offsetPosition[1];
  return static_cast<int32_t>(data);
}

int32_t PdxHelper::readUInt16(uint8_t* offsetPosition) {
  uint16_t data = offsetPosition[0];
  data = (data << 8) | offsetPosition[1];
  return static_cast<int32_t>(data);
}

int32_t PdxHelper::readByte(uint8_t* offsetPosition) {
  return static_cast<int32_t>(offsetPosition[0]);
}

void PdxHelper::writeInt16(uint8_t* offsetPosition, int32_t value) {
  int16_t val = static_cast<int16_t>(value);
  offsetPosition[0] = static_cast<uint8_t>(val >> 8);
  offsetPosition[1] = static_cast<uint8_t>(val);
}

void PdxHelper::writeByte(uint8_t* offsetPosition, int32_t value) {
  offsetPosition[0] = static_cast<uint8_t>(value);
}

int32_t PdxHelper::readInt(uint8_t* offsetPosition, int size) {
  switch (size) {
    case 1:
      return readByte(offsetPosition);
    case 2:
      return readUInt16(offsetPosition);
    case 4:
      return readInt32(offsetPosition);
  }
  throw;
}

int32_t PdxHelper::getEnumValue(const char* enumClassName, const char* enumName,
                                int hashcode) {
  auto ei = std::make_shared<EnumInfo>(enumClassName, enumName, hashcode);
  return getPdxTypeRegistry()->getEnumValue(ei);
}

EnumInfoPtr PdxHelper::getEnum(int enumId) {
  EnumInfoPtr ei = getPdxTypeRegistry()->getEnum(enumId);
  return ei;
}
}  // namespace client
}  // namespace geode
}  // namespace apache
