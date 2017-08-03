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

#include <geode/CacheFactory.hpp>
#include <CppCacheLibrary.hpp>
#include <geode/Cache.hpp>
#include <CacheImpl.hpp>
#include <geode/SystemProperties.hpp>
#include <geode/PoolManager.hpp>
#include <PoolAttributes.hpp>
#include <CacheConfig.hpp>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Guard_T.h>
#include <map>
#include <string>
#include <DistributedSystemImpl.hpp>
#include <SerializationRegistry.hpp>
#include <PdxInstantiator.hpp>
#include <PdxEnumInstantiator.hpp>
#include <PdxType.hpp>
#include <PdxTypeRegistry.hpp>
#include "DiskVersionTag.hpp"
#include "TXCommitMessage.hpp"
#include <functional>
#include "version.h"

#define DEFAULT_CACHE_NAME "default_GeodeCache"
#define DEFAULT_SERVER_PORT 40404
#define DEFAULT_SERVER_HOST "localhost"

extern ACE_Recursive_Thread_Mutex* g_disconnectLock;

bool Cache_CreatedFromCacheFactory = false;

namespace apache {
namespace geode {
namespace client {
ACE_Recursive_Thread_Mutex g_cfLock;

typedef std::map<std::string, CachePtr> StringToCachePtrMap;

void* CacheFactory::m_cacheMap = (void*)nullptr;

CacheFactoryPtr* CacheFactory::default_CacheFactory = nullptr;

CacheFactoryPtr CacheFactory::createCacheFactory(
    const PropertiesPtr& configPtr) {
  return std::make_shared<CacheFactory>(configPtr);
}

void CacheFactory::init() {
  if (m_cacheMap == (void*)nullptr) {
    m_cacheMap = (void*)new StringToCachePtrMap();
  }
  if (!reinterpret_cast<StringToCachePtrMap*>(m_cacheMap)) {
    throw OutOfMemoryException("CacheFactory::create: ");
  }
}

void CacheFactory::create_(const char* name, PropertiesPtr dsProp,
                           const char* id_data, CachePtr& cptr,
                           bool ignorePdxUnreadFields, bool readPdxSerialized) {
  CppCacheLibrary::initLib();

  cptr = nullptr;
  if (!reinterpret_cast<StringToCachePtrMap*>(m_cacheMap)) {
    throw IllegalArgumentException(
        "CacheFactory::create: cache map is not initialized");
  }
  if (name == nullptr) {
    throw IllegalArgumentException("CacheFactory::create: name is nullptr");
  }
  if (name[0] == '\0') {
    name = "NativeCache";
  }

  cptr = std::make_shared<Cache>(name, dsProp, ignorePdxUnreadFields,
                                 readPdxSerialized);
}  // namespace client

const char* CacheFactory::getVersion() { return PRODUCT_VERSION; }

const char* CacheFactory::getProductDescription() {
  return PRODUCT_VENDOR " " PRODUCT_NAME " " PRODUCT_VERSION " (" PRODUCT_BITS
                        ") " PRODUCT_BUILDDATE;
}

CacheFactory::CacheFactory() {
  ignorePdxUnreadFields = false;
  pdxReadSerialized = false;
  dsProp = nullptr;
  pf = nullptr;
}

CacheFactory::CacheFactory(const PropertiesPtr dsProps) {
  ignorePdxUnreadFields = false;
  pdxReadSerialized = false;
  this->dsProp = dsProps;
  this->pf = nullptr;
}

CachePtr CacheFactory::create() {
  ACE_Guard<ACE_Recursive_Thread_Mutex> connectGuard(*g_disconnectLock);

  LOGFINE("CacheFactory called DistributedSystem::connect");

  default_CacheFactory = new CacheFactoryPtr(shared_from_this());
  Cache_CreatedFromCacheFactory = true;
  auto cache = create(DEFAULT_CACHE_NAME, dsProp, nullptr);

  cache->m_cacheImpl->getSerializationRegistry()->addType2(std::bind(
      TXCommitMessage::create,
      std::ref(*(cache->m_cacheImpl->getMemberListForVersionStamp()))));

  cache->m_cacheImpl->getSerializationRegistry()->addType(
      GeodeTypeIdsImpl::PDX, PdxInstantiator::createDeserializable);
  cache->m_cacheImpl->getSerializationRegistry()->addType(
      GeodeTypeIds::CacheableEnum, PdxEnumInstantiator::createDeserializable);
  cache->m_cacheImpl->getSerializationRegistry()->addType(
      GeodeTypeIds::PdxType,
      std::bind(PdxType::CreateDeserializable,
                cache->m_cacheImpl->getPdxTypeRegistry()));

  cache->m_cacheImpl->getSerializationRegistry()->addType(std::bind(
      VersionTag::createDeserializable,
      std::ref(*(cache->m_cacheImpl->getMemberListForVersionStamp()))));
  cache->m_cacheImpl->getSerializationRegistry()->addType2(
      GeodeTypeIdsImpl::DiskVersionTag,
      std::bind(
          DiskVersionTag::createDeserializable,
          std::ref(*(cache->m_cacheImpl->getMemberListForVersionStamp()))));

  cache->m_cacheImpl->getPdxTypeRegistry()->setPdxIgnoreUnreadFields(
      cache->getPdxIgnoreUnreadFields());
  cache->m_cacheImpl->getPdxTypeRegistry()->setPdxReadSerialized(
      cache->getPdxReadSerialized());

  return cache;
}

CachePtr CacheFactory::create(const char* name, PropertiesPtr dsProp,
                              const CacheAttributesPtr& attrs /*= nullptr*/) {
  ACE_Guard<ACE_Recursive_Thread_Mutex> connectGuard(*g_disconnectLock);

  CachePtr cptr;
  CacheFactory::create_(name, dsProp, "", cptr, ignorePdxUnreadFields,
                        pdxReadSerialized);
  cptr->m_cacheImpl->setAttributes(attrs);
  try {
    const char* cacheXml =
        cptr->getDistributedSystem().getSystemProperties().cacheXMLFile();
    if (cacheXml != 0 && strlen(cacheXml) > 0) {
      cptr->initializeDeclarativeCache(cacheXml);
    } else {
      cptr->m_cacheImpl->initServices();
    }
  } catch (const apache::geode::client::RegionExistsException&) {
    LOGWARN("Attempt to create existing regions declaratively");
  } catch (const apache::geode::client::Exception&) {
    if (!cptr->isClosed()) {
      cptr->close();
      cptr = nullptr;
    }
    throw;
  } catch (...) {
    if (!cptr->isClosed()) {
      cptr->close();
      cptr = nullptr;
    }
    throw apache::geode::client::UnknownException(
        "Exception thrown in CacheFactory::create");
  }

  return cptr;
}

CacheFactory::~CacheFactory() {}
void CacheFactory::cleanup() {
  if (m_cacheMap != nullptr) {
    if ((reinterpret_cast<StringToCachePtrMap*>(m_cacheMap))->empty() == true) {
      (reinterpret_cast<StringToCachePtrMap*>(m_cacheMap))->clear();
    }
    delete (reinterpret_cast<StringToCachePtrMap*>(m_cacheMap));
    m_cacheMap = nullptr;
  }
}

void CacheFactory::handleXML(CachePtr& cachePtr, const char* cachexml,
                             DistributedSystem& system) {
  CacheConfig config(cachexml);

  RegionConfigMapT regionMap = config.getRegionList();
  RegionConfigMapT::const_iterator iter = regionMap.begin();
  while (iter != regionMap.end()) {
    std::string regionName = (*iter).first;
    RegionConfigPtr regConfPtr = (*iter).second;

    AttributesFactory af;
    af.setLruEntriesLimit(regConfPtr->getLruEntriesLimit());
    af.setConcurrencyLevel(regConfPtr->getConcurrency());
    af.setInitialCapacity(regConfPtr->entries());
    af.setCachingEnabled(regConfPtr->getCaching());

    RegionAttributesPtr regAttrsPtr;
    regAttrsPtr = af.createRegionAttributes();

    const RegionShortcut regionShortcut =
        (regAttrsPtr->getCachingEnabled() ? RegionShortcut::CACHING_PROXY
                                          : RegionShortcut::PROXY);
    RegionFactoryPtr regionFactoryPtr =
        cachePtr->createRegionFactory(regionShortcut);
    regionFactoryPtr->create(regionName.c_str());
    ++iter;
  }
}

CacheFactoryPtr CacheFactory::set(const char* name, const char* value) {
  if (this->dsProp == nullptr) this->dsProp = Properties::create();
  this->dsProp->insert(name, value);
  return shared_from_this();
}

CacheFactoryPtr CacheFactory::setPdxIgnoreUnreadFields(bool ignore) {
  ignorePdxUnreadFields = ignore;
  return shared_from_this();
}

CacheFactoryPtr CacheFactory::setPdxReadSerialized(bool prs) {
  pdxReadSerialized = prs;
  return shared_from_this();
}
}  // namespace client
}  // namespace geode
}  // namespace apache
