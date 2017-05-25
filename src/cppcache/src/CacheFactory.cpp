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

#include "config.h"
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
#include <string>
#include <DistributedSystemImpl.hpp>
#include <SerializationRegistry.hpp>
#include <PdxInstantiator.hpp>
#include <PdxEnumInstantiator.hpp>
#include <PdxType.hpp>
#include <PdxTypeRegistry.hpp>
#include <CacheFactoryImpl.hpp>
#include <CacheRegionHelper.hpp>

#include "version.h"

#define DEFAULT_DS_NAME "default_GeodeDS"
#define DEFAULT_CACHE_NAME "default_GeodeCache"
#define DEFAULT_SERVER_PORT 40404
#define DEFAULT_SERVER_HOST "localhost"

extern ACE_Recursive_Thread_Mutex* g_disconnectLock;

namespace apache {
namespace geode {
namespace client {

CacheFactoryPtr* s_factory = nullptr;

PoolPtr CacheFactory::createOrGetDefaultPool(CachePtr cache) {
  ACE_Guard<ACE_Recursive_Thread_Mutex> connectGuard(*g_disconnectLock);

  if (cache->isClosed() == false &&
      CacheRegionHelper::getCacheImpl(cache.get())->getDefaultPool() != nullptr) {
    return  CacheRegionHelper::getCacheImpl(cache.get())->getDefaultPool();
  }

  PoolPtr pool = getPoolManager()->find(DEFAULT_POOL_NAME);

  // if default_poolFactory is null then we are not using latest API....
  if (pool == nullptr && s_factory != nullptr) {
    pool = (*s_factory)->determineDefaultPool(cache);
  }

  return pool;
}

CacheFactoryPtr CacheFactory::createCacheFactory(
    const PropertiesPtr& configPtr) {
  // need to create PoolFactory instance
  s_factory = new CacheFactoryPtr(std::make_shared<CacheFactory>(configPtr));
  return *s_factory;
}

void CacheFactory::create_(const char* name, DistributedSystemPtr& system,
                           const char* id_data, CachePtr& cptr,
                           bool ignorePdxUnreadFields, bool readPdxSerialized) {
  CppCacheLibrary::initLib();

  cptr = nullptr;
  if (system == nullptr) {
    throw IllegalArgumentException(
        "CacheFactory::create: system uninitialized");
  }
  if (name == NULL) {
    throw IllegalArgumentException("CacheFactory::create: name is NULL");
  }
  if (name[0] == '\0') {
    name = "NativeCache";
  }
  
  CachePtr cep = std::make_shared<Cache>(name, system, id_data, ignorePdxUnreadFields,
                         readPdxSerialized);
  if (!cep) {
    throw OutOfMemoryException("Out of Memory");
  }
  cptr = cep;
  return;
  }

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
  pimpl = std::unique_ptr<CacheFactoryImpl>(new CacheFactoryImpl());
}

CacheFactory::CacheFactory(const PropertiesPtr dsProps) {
  ignorePdxUnreadFields = false;
  pdxReadSerialized = false;
  this->dsProp = dsProps;
  this->pf = nullptr;
  pimpl = std::unique_ptr<CacheFactoryImpl>(new CacheFactoryImpl());
}

CachePtr CacheFactory::create() {

  ACE_Guard<ACE_Recursive_Thread_Mutex> connectGuard(*g_disconnectLock);
  DistributedSystemPtr dsPtr = nullptr;

  // should we compare deafult DS properties here??
  if (DistributedSystem::isConnected()) {
    dsPtr = DistributedSystem::getInstance();
  } else {
    dsPtr = DistributedSystem::connect(DEFAULT_DS_NAME, dsProp);
    LOGFINE("CacheFactory called DistributedSystem::connect");
  }

  CachePtr cache = nullptr;
  if (cache == nullptr)
  {
	cache = create(DEFAULT_CACHE_NAME, dsPtr,
	               dsPtr->getSystemProperties()->cacheXMLFile(), nullptr);
  }

  SerializationRegistry::addType(GeodeTypeIdsImpl::PDX,
                                 PdxInstantiator::createDeserializable);
  SerializationRegistry::addType(GeodeTypeIds::CacheableEnum,
                                 PdxEnumInstantiator::createDeserializable);
  SerializationRegistry::addType(GeodeTypeIds::PdxType,
                                 PdxType::CreateDeserializable);
  PdxTypeRegistry::setPdxIgnoreUnreadFields(cache->getPdxIgnoreUnreadFields());
  PdxTypeRegistry::setPdxReadSerialized(cache->getPdxReadSerialized());

  return cache;
}

CachePtr CacheFactory::create(const char* name,
                              DistributedSystemPtr system /*= nullptr*/,
                              const char* cacheXml /*= 0*/,
                              const CacheAttributesPtr& attrs /*= nullptr*/) {
  ACE_Guard<ACE_Recursive_Thread_Mutex> connectGuard(*g_disconnectLock);

  CachePtr cptr;
  CacheFactory::create_(name, system, "", cptr, ignorePdxUnreadFields,
                        pdxReadSerialized);
  cptr->m_cacheImpl->setAttributes(attrs);
  try {
    if (cacheXml != 0 && strlen(cacheXml) > 0) {
      cptr->initializeDeclarativeCache(cacheXml);
    } else {
      std::string file = system->getSystemProperties()->cacheXMLFile();
      if (file != "") {
        cptr->initializeDeclarativeCache(file.c_str());
      } else {
        cptr->m_cacheImpl->initServices();
      }
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

PoolPtr CacheFactory::determineDefaultPool(CachePtr cachePtr) {
  PoolPtr pool = nullptr;
  HashMapOfPools allPools = getPoolManager()->getAll();
  size_t currPoolSize = allPools.size();
  CacheImpl * cacheImpl = CacheRegionHelper::getCacheImpl(cachePtr.get());
  // means user has not set any pool attributes
  if (this->pf == nullptr) {
    this->pf = getPoolFactory(cachePtr->shared_from_this());
    if (currPoolSize == 0) {
      if (!this->pf->m_addedServerOrLocator) {
        this->pf->addServer(DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT);
      }

      pool = this->pf->create(DEFAULT_POOL_NAME,cachePtr);
      // creatubg default pool so setting this as default pool
      LOGINFO("Set default pool with localhost:40404");
	  cacheImpl->setDefaultPool(pool);
      return pool;
    } else if (currPoolSize == 1) {
      pool = allPools.begin().second();
      LOGINFO("Set default pool from existing pool.");
	  cacheImpl->setDefaultPool(pool);
      return pool;
    } else {
      // can't set anything as deafult pool
      return nullptr;
    }
  } else {
    PoolPtr defaulPool = cacheImpl->getDefaultPool();

    if (!this->pf->m_addedServerOrLocator) {
      this->pf->addServer(DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT);
    }

    if (defaulPool != nullptr) {
      // once default pool is created, we will not create
      if (*(defaulPool->m_attrs) == *(this->pf->m_attrs)) {
        return defaulPool;
      } else {
        throw IllegalStateException(
            "Existing cache's default pool was not compatible");
      }
    }

    pool = nullptr;

    // return any existing pool if it matches
    for (auto iter = allPools.begin(); iter != allPools.end(); ++iter) {
      auto currPool = iter.second();
      if (*(currPool->m_attrs) == *(this->pf->m_attrs)) {
        return currPool;
      }
    }

    // defaul pool is null
    GF_DEV_ASSERT(defaulPool == nullptr);

    if (defaulPool == nullptr) {
      pool = this->pf->create(DEFAULT_POOL_NAME, cachePtr);
      LOGINFO("Created default pool");
      // creating default so setting this as defaul pool
	  cacheImpl->setDefaultPool(pool);
    }

    return pool;
  }
}

PoolFactoryPtr CacheFactory::getPoolFactory(CachePtr cachePtr) {
  if (this->pf == nullptr) {
    this->pf = getPoolManager()->createFactory();
  }
  return this->pf;
}

CacheFactory::~CacheFactory() {}

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
