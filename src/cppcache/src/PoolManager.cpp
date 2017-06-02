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
#include <geode/PoolManager.hpp>
#include <ace/Recursive_Thread_Mutex.h>
#include <ace/Guard_T.h>
#include <CacheRegionHelper.hpp>

#include <PoolAttributes.hpp>

using namespace apache::geode::client;
#define DEFAULT_SERVER_PORT 40404
#define DEFAULT_SERVER_HOST "localhost"
// TODO: make this a member of TcrConnectionManager.
HashMapOfPools* connectionPools = NULL; /*new HashMapOfPools( )*/
ACE_Recursive_Thread_Mutex connectionPoolsLock;

namespace apache {
namespace geode {
namespace client {
static PoolManagerPtr g_poolManager = nullptr;
PoolManagerPtr getPoolManager()
{
    return g_poolManager;
}
void SetPoolManager(PoolManagerPtr poolManagerPtr)
{
  g_poolManager = poolManagerPtr;
}
}  // namespace client
}  // namespace geode
}  // namespace apache

void removePool(const char* name) {
  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connectionPoolsLock);
  connectionPools->erase(CacheableString::create(name));
}

PoolFactoryPtr PoolManager::createFactory() {
  static auto poolFactoryPtr = std::make_shared<PoolFactory>(this);
  return poolFactoryPtr;
}


PoolPtr PoolManager::determineDefaultPool() {
  PoolPtr pool = nullptr;
  HashMapOfPools allPools = getAll();
  size_t currPoolSize = allPools.size();
  CacheImpl * cacheImpl = CacheRegionHelper::getCacheImpl(m_cache);
  // means user has not set any pool attributes
  PoolFactoryPtr pf = createFactory();
  if(pf != nullptr){
    if (currPoolSize == 0) {
      if (!pf->m_addedServerOrLocator) {
        pf->addServer(DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT);
      }

      pool = pf->create(DEFAULT_POOL_NAME, m_cache->shared_from_this());
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
    PoolPtr defaultPool = cacheImpl->getDefaultPool();

    if (!pf->m_addedServerOrLocator) {
      pf->addServer(DEFAULT_SERVER_HOST, DEFAULT_SERVER_PORT);
    }

    if (defaultPool != nullptr) {
      // once default pool is created, we will not create
      if (*(defaultPool->m_attrs) == *(pf->m_attrs)) {
        return defaultPool;
      } else {
        throw IllegalStateException(
            "Existing cache's default pool was not compatible");
      }
    }

    pool = nullptr;

    // return any existing pool if it matches
    for (auto iter = allPools.begin(); iter != allPools.end(); ++iter) {
      auto currPool = iter.second();
      if (*(currPool->m_attrs) == *(pf->m_attrs)) {
        return currPool;
      }
    }

    // defaul pool is null
    GF_DEV_ASSERT(defaultPool == nullptr);

    if (defaultPool == nullptr) {
      pool = pf->create(DEFAULT_POOL_NAME, m_cache->shared_from_this());
      LOGINFO("Created default pool");
      // creating default so setting this as defaul pool
      cacheImpl->setDefaultPool(pool);
    }

    return pool;
  }
}



PoolPtr PoolManager::createOrGetDefaultPool() {

  if (m_cache->isClosed() == false &&
      CacheRegionHelper::getCacheImpl(m_cache)->getDefaultPool() != nullptr) {
    return  CacheRegionHelper::getCacheImpl(m_cache)->getDefaultPool();
  }

  PoolPtr pool = find(DEFAULT_POOL_NAME);

  // if default_poolFactory is null then we are not using latest API....
  if (pool == nullptr) {
    pool = determineDefaultPool();
  }

  return pool;
}


void PoolManager::close(bool keepAlive) {
  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connectionPoolsLock);

  if (connectionPools == NULL) {
    return;
  }

  std::vector<PoolPtr> poolsList;

  for (HashMapOfPools::Iterator iter = connectionPools->begin();
       iter != connectionPools->end(); ++iter) {
    poolsList.push_back(iter.second());
  }

  for (std::vector<PoolPtr>::iterator iter = poolsList.begin();
       iter != poolsList.end(); ++iter) {
    (*iter)->destroy(keepAlive);
  }

  GF_SAFE_DELETE(connectionPools);
}

PoolPtr PoolManager::find(const char* name) {
  ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connectionPoolsLock);

  if (connectionPools == NULL) {
    connectionPools = new HashMapOfPools();
  }

  if (name) {
    HashMapOfPools::Iterator iter =
        connectionPools->find(CacheableString::create(name));

    PoolPtr poolPtr = nullptr;

    if (iter != connectionPools->end()) {
      poolPtr = iter.second();
      GF_DEV_ASSERT(poolPtr != nullptr);
    }

    return poolPtr;
  } else {
    return nullptr;
  }
}

PoolPtr PoolManager::find(RegionPtr region) {
  return find(region->getAttributes()->getPoolName());
}

const HashMapOfPools& PoolManager::getAll() {
  if (connectionPools == NULL) {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connectionPoolsLock);
    if (connectionPools == NULL) {
      connectionPools = new HashMapOfPools();
    }
  }
  return *connectionPools;
}


PoolManager::PoolManager(Cache* cache): m_cache(cache)
{
  if (connectionPools == NULL) {
    ACE_Guard<ACE_Recursive_Thread_Mutex> guard(connectionPoolsLock);
    if (connectionPools == NULL) {
      connectionPools = new HashMapOfPools();
    }
  }

}

void PoolManager::addPool(CacheableStringPtr name, PoolPtr pool) {
  pool->setPoolManager(this);
  connectionPools->insert(name, pool);
}
