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

#define ROOT_NAME "testConnect"

#include "fw_dunit.hpp"
#include <geode/CacheFactory.hpp>

using namespace apache::geode::client;

const char* host_name = "Suds";
DUNIT_TASK(s1p1, CreateRegionOne)
  {
    auto factory = CacheFactory::createCacheFactory();
    auto cache = std::make_shared<Cache>(factory->create());
    try {
      cache->getDistributedSystem().connect(cache.get());
      FAIL("Expected an exception.");
    } catch (const AlreadyConnectedException& ex) {
      LOG("Got expected exception.");
      LOG(ex.what());
    }

    cache->getDistributedSystem().disconnect();

    try {
      // doing second disconnect to force the exception
      cache->getDistributedSystem().disconnect();
      FAIL("Expected an exception.");
    } catch (const NotConnectedException& ex) {
      LOG("Got expected exception.");
      LOG(ex.what());
    }
  }
ENDTASK
