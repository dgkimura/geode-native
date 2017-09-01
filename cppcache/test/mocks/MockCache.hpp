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

#include <gmock/gmock.h>

#include <geode/Cache.hpp>
#include <geode/PoolManager.hpp>

class MockCache : public Cache
{
public:

  MOCK_METHOD1(createRegionFactory, RegionFactoryPtr(RegionShortcut regionShortcut));

  MOCK_METHOD1(initializeDeclarativeCache, void(const char* cacheXml));

  MOCK_CONST_METHOD0(getName, const std::string&());

  MOCK_CONST_METHOD0(isClosed, bool());

  MOCK_CONST_METHOD0(getDistributedSystem, DistributedSystem&());

  MOCK_METHOD0(close, void());

  MOCK_METHOD1(close, void(bool keepalive));

  MOCK_METHOD1(getRegion, RegionPtr(const char* path));

  MOCK_METHOD1(rootRegions, void(VectorOfRegion& regions));

  MOCK_METHOD0(getQueryService, QueryServicePtr());

  MOCK_METHOD1(getQueryService, QueryServicePtr(const char* poolName));

  MOCK_METHOD0(readyForEvents, void());

  MOCK_METHOD2(createAuthenticatedView, RegionServicePtr(PropertiesPtr userSecurityProperties,
    const char* poolName));

  MOCK_METHOD0(getCacheTransactionManager, CacheTransactionManagerPtr());

  MOCK_METHOD0(getPdxIgnoreUnreadFields, bool());

  MOCK_METHOD0(getPdxReadSerialized, bool());

  MOCK_METHOD0(getTypeRegistry, TypeRegistry&());

  MOCK_METHOD1(createPdxInstanceFactory, PdxInstanceFactoryPtr(const char* className));

  MOCK_CONST_METHOD0(getStatisticsFactory, apache::geode::statistics::StatisticsFactory*());

  MOCK_CONST_METHOD2(createDataInput, std::unique_ptr<DataInput>(const uint8_t* buffer, int32_t len));

  MOCK_CONST_METHOD0(createDataOutput, std::unique_ptr<DataOutput>());

  MOCK_CONST_METHOD0(getPoolManager, PoolManager&());
};
