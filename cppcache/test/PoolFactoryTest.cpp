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

#include <gtest/gtest.h>
#include <geode/PoolFactory.hpp>

#include "mocks/MockCache.hpp"
#include "mocks/MockPoolManager.hpp"

using namespace apache::geode::client;

TEST(PoolFactoryTest, ThrowsExceptionIfPoolNameAlreadyExists) {
  MockCache cache;
  MockPoolManager poolManager(cache);

  // PoolManager::find returns existing pool with poolname.
  EXPECT_CALL(poolManager, find("poolname"))
    .Times(1)
    .WillOnce(testing::Return(PoolPtr(nullptr)));
  EXPECT_CALL(cache, getPoolManager())
    .Times(1)
    .WillRepeatedly(testing::ReturnRef(poolManager));

  PoolFactory factory(cache);
  EXPECT_THROW(factory.create("poolname"), IllegalStateException);
}

TEST(PoolFactoryTest, ThrowsIfNoCallToAddLocatorOrAddServer) {
  MockCache cache;
  MockPoolManager poolManager(cache);

  EXPECT_CALL(poolManager, find("poolname"))
    .Times(1)
    .WillOnce(testing::Return(nullptr));
  EXPECT_CALL(cache, getPoolManager())
    .Times(1)
    .WillRepeatedly(testing::ReturnRef(poolManager));

  PoolFactory factory(cache);

  // No call to PoolFactory::addServer or PoolFactory::addLocator

  EXPECT_THROW(factory.create("poolname"), IllegalStateException);
}

TEST(PoolFactoryTest, ThrowsIfCacheIsClosed) {
  MockCache cache;
  MockPoolManager poolManager(cache);

  EXPECT_CALL(poolManager, find("poolname"))
    .Times(1)
    .WillOnce(testing::Return(nullptr));
  EXPECT_CALL(cache, getPoolManager())
    .Times(1)
    .WillRepeatedly(testing::ReturnRef(poolManager));

  // Cache is already closed.
  EXPECT_CALL(cache, isClosed())
    .Times(1)
    .WillOnce(testing::Return(true));

  PoolFactory factory(cache);
  factory.addServer("localhost", 1234);

  EXPECT_THROW(factory.create("poolname"), CacheClosedException);
}
