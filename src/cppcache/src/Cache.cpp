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
#include <geode/geode_globals.hpp>
#include <geode/SharedPtr.hpp>

#include <geode/DistributedSystem.hpp>
#include <DistributedSystemImpl.hpp>
#include <CacheXmlParser.hpp>
#include <CacheRegionHelper.hpp>
#include <geode/Cache.hpp>
#include <CacheImpl.hpp>
#include <UserAttributes.hpp>
#include <ProxyRegion.hpp>
#include <geode/FunctionService.hpp>
#include <geode/PoolManager.hpp>
#include <PdxInstanceFactoryImpl.hpp>

using namespace apache::geode::client;

extern ACE_Recursive_Thread_Mutex* g_disconnectLock;

/** Returns the name of this cache.
 * This method does not throw
 * <code>CacheClosedException</code> if the cache is closed.
 * @return the string name of this cache
 */
const char* Cache::getName() const { return m_cacheImpl->getName(); }

/**
 * Indicates if this cache has been closed.
 * After a new cache object is created, this method returns false;
 * After the close is called on this cache object, this method
 * returns true.
 *
 * @return true, if this cache is closed; false, otherwise
 */
bool Cache::isClosed() const { return m_cacheImpl->isClosed(); }

/**
 * Returns the distributed system that this cache was
 * {@link CacheFactory::create created} with. This method does not throw
 * <code>CacheClosedException</code> if the cache is closed.
 */
DistributedSystemPtr Cache::getDistributedSystem() const {
  DistributedSystemPtr result;
  result = m_cacheImpl->getDistributedSystem();
  return result;
}

void Cache::close() { close(false); }

/**
 * Terminates this object cache and releases all the local resources.
 * After this cache is closed, any further
 * method call on this cache or any region object will throw
 * <code>CacheClosedException</code>, unless otherwise noted.
 * @param keepalive whether to keep the durable client's queue
 * @throws CacheClosedException,  if the cache is already closed.
 */
void Cache::close(bool keepalive) {
  ACE_Guard<ACE_Recursive_Thread_Mutex> connectGuard(*g_disconnectLock);
  if (m_cacheImpl->getDistributedSystem()->currentInstances() > 0) return;
  m_cacheImpl->close(keepalive);

  try {
    m_cacheImpl->getDistributedSystem()->disconnect();
  } catch (const apache::geode::client::NotConnectedException&) {
  } catch (const apache::geode::client::Exception&) {
  } catch (...) {
  }
}

RegionPtr Cache::getRegion(const char* path) {
  LOGDEBUG("Cache::getRegion");
  RegionPtr result;
  m_cacheImpl->getRegion(path, result);

  if (result != NULLPTR) {
    if (isPoolInMultiuserMode(result)) {
      LOGWARN(
          "Pool [%s] attached with region [%s] is in multiuser authentication "
          "mode. "
          "Operations may fail as this instance does not have any credentials.",
          result->getAttributes()->getPoolName(), result->getFullPath());
    }
  }

  return result;
}

/**
 * Returns a set of root regions in the cache. Does not cause any
 * shared regions to be mapped into the cache. This set is a snapshot and
 * is not backed by the Cache. The regions passed in are cleared.
 *
 * @param regions the region collection object containing the returned set of
 * regions when the function returns
 */

void Cache::rootRegions(VectorOfRegion& regions) {
  m_cacheImpl->rootRegions(regions);
  /*VectorOfRegion tmp;
   //this can cause issue when pool attached with region in multiuserSecure mode
   m_cacheImpl->rootRegions(tmp);

   if (tmp.size() > 0)
   {
     for(size_t i = 0; i< tmp.size(); i++)
     {
       if (!isPoolInMultiuserMode(tmp.at(i)))
       {
         regions.push_back(tmp.at(i));
       }
     }
   }*/
}

RegionFactoryPtr Cache::createRegionFactory(RegionShortcut preDefinedRegion) {
  return m_cacheImpl->createRegionFactory(preDefinedRegion);
}

QueryServicePtr Cache::getQueryService() {
  return m_cacheImpl->getQueryService();
}

QueryServicePtr Cache::getQueryService(const char* poolName) {
  return m_cacheImpl->getQueryService(poolName);
}

CacheTransactionManagerPtr Cache::getCacheTransactionManager() {
  return m_cacheImpl->getCacheTransactionManager();
}

Cache::Cache(const char* name, DistributedSystemPtr sys,
             bool ignorePdxUnreadFields, bool readPdxSerialized) {
  m_cacheImpl =
      new CacheImpl(this, name, sys, ignorePdxUnreadFields, readPdxSerialized);
}
Cache::Cache(const char* name, DistributedSystemPtr sys, const char* id_data,
             bool ignorePdxUnreadFields, bool readPdxSerialized) {
  m_cacheImpl = new CacheImpl(this, name, sys, id_data, ignorePdxUnreadFields,
                              readPdxSerialized);
}

PoolPtr  Cache::createOrGetDefaultPool()
{
 return m_cacheImpl->createOrGetDefaultPool();
}


Cache::~Cache() { delete m_cacheImpl; }

/** Initialize the cache by the contents of an xml file
  * @param  cacheXml
  *         The xml file
  * @throws OutOfMemoryException
  * @throws CacheXmlException
  *         Something went wrong while parsing the XML
  * @throws IllegalStateException
  *         If xml file is well-flrmed but not valid
  * @throws RegionExistsException if a region is already in
  *         this cache
  * @throws CacheClosedException if the cache is closed
  *         at the time of region creation
  * @throws UnknownException otherwise
  */
void Cache::initializeDeclarativeCache(const char* cacheXml) {
  CacheXmlParser* xmlParser = CacheXmlParser::parse(cacheXml);
  xmlParser->setAttributes(this);
  m_cacheImpl->initServices();
  xmlParser->create(this);
  delete xmlParser;
  xmlParser = NULL;
}

void Cache::readyForEvents() { m_cacheImpl->readyForEvents(); }

bool Cache::isPoolInMultiuserMode(RegionPtr regionPtr) {
  const char* poolName = regionPtr->getAttributes()->getPoolName();

  if (poolName != NULL) {
    PoolPtr poolPtr = m_cacheImpl->getPoolManager()->find(poolName);
    if (poolPtr != NULLPTR && !poolPtr->isDestroyed()) {
      return poolPtr->getMultiuserAuthentication();
    }
  }
  return false;
}

bool Cache::getPdxIgnoreUnreadFields() {
  return m_cacheImpl->getPdxIgnoreUnreadFields();
}

bool Cache::getPdxReadSerialized() {
  return m_cacheImpl->getPdxReadSerialized();
}

PdxInstanceFactoryPtr Cache::createPdxInstanceFactory(const char* className) {
  PdxInstanceFactoryPtr pIFPtr(new PdxInstanceFactoryImpl(className));
  return pIFPtr;
}

RegionServicePtr Cache::createAuthenticatedView(
    PropertiesPtr userSecurityProperties, const char* poolName) {
  if (poolName == NULL) {
    if (!this->isClosed() && m_cacheImpl->getDefaultPool() != NULLPTR) {
      return m_cacheImpl->getDefaultPool()->createSecureUserCache(
          userSecurityProperties);
    }

    throw IllegalStateException(
        "Either cache has been closed or there are more than two pool."
        "Pass poolname to get the secure Cache");
  } else {
    if (!this->isClosed()) {
      if (poolName != NULL) {
        PoolPtr poolPtr = m_cacheImpl->getPoolManager()->find(poolName);
        if (poolPtr != NULLPTR && !poolPtr->isDestroyed()) {
          return poolPtr->createSecureUserCache(userSecurityProperties);
        }
        throw IllegalStateException(
            "Either pool not found or it has been destroyed");
      }
      throw IllegalArgumentException("poolname is NULL");
    }

    throw IllegalStateException("Cache has been closed");
  }
  return NULLPTR;
}
