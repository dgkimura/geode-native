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

#include <geode/GeodeCppCache.hpp>

#include "fw_helper.hpp"

using test::cout;
using test::endl;

using namespace apache::geode::client;

#define ROOT_NAME "testRegionMap"

#include "CacheHelper.hpp"

/**
 * @brief Test putting and getting entries without LRU enabled.
 */
BEGIN_TEST(TestRegionLRULastTen)
#if 1
    try {

      CacheHelper &cacheHelper = CacheHelper::getHelper();
      RegionPtr regionPtr;
      try {
      cacheHelper.createLRURegion(fwtest_Name, regionPtr);
      cout << regionPtr->getFullPath() << endl;
      }
      catch(std::exception & e)
      {
        cout << "A Exceptioned: "<< e.what()<<endl;
      }
      // put more than 10 items... verify limit is held.
      uint32_t i;
      for (i = 0; i < 10; i++) {
        char buf[100];
        sprintf(buf, "%d", i);
        CacheableKeyPtr key = CacheableKey::create(buf);
        sprintf(buf, "value of %d", i);
        CacheableStringPtr valuePtr = cacheHelper.createCacheable(buf);
        regionPtr->put(key, valuePtr);
        VectorOfCacheableKey vecKeys;
        regionPtr->keys(vecKeys);
        ASSERT(vecKeys.size() == (i + 1), "expected more entries");
      }
      for (i = 10; i < 20; i++) {
        char buf[100];
        sprintf(buf, "%d", i);
        CacheableKeyPtr key = CacheableKey::create(buf);
        sprintf(buf, "value of %d", i);
        CacheableStringPtr valuePtr = cacheHelper.createCacheable(buf);
        regionPtr->put(key, valuePtr);
        VectorOfCacheableKey vecKeys;
        regionPtr->keys(vecKeys);
        cacheHelper.showKeys(vecKeys);
        ASSERT(vecKeys.size() == (10), "expected 10 entries");
      }
      VectorOfCacheableKey vecKeys;
      regionPtr->keys(vecKeys);
      ASSERT(vecKeys.size() == 10, "expected 10 entries");
      // verify it is the last 10 keys..
      int expected = 0;
      int total = 0;
      for (int k = 10; k < 20; k++) {
        expected += k;
        auto key = std::dynamic_pointer_cast<CacheableString>(vecKeys.back());
        vecKeys.pop_back();
        total += atoi(key->asChar());
      }
      ASSERT(vecKeys.empty(), "expected no more than 10 keys.");
      ASSERT(expected == total, "checksum mismatch.");
    }
      catch(std::exception & e)
      {
        cout << "Exceptioned: "<< e.what()<<endl;
      }
      catch(...)
      {
        cout << "Failed 1" << endl;
      }

    #endif
END_TEST(TestRegionLRULastTen)

BEGIN_TEST(TestRegionNoLRU)
#if 1
  try {
    CacheHelper &cacheHelper = CacheHelper::getHelper();
    RegionPtr regionPtr;
    cacheHelper.createPlainRegion(fwtest_Name, regionPtr);
    // put more than 10 items... verify limit is held.
    uint32_t i;
    for (i = 0; i < 20; i++) {
      char buf[100];
      sprintf(buf, "%d", i);
      CacheableKeyPtr key = CacheableKey::create(buf);
      sprintf(buf, "value of %d", i);
      CacheableStringPtr valuePtr = cacheHelper.createCacheable(buf);
      regionPtr->put(key, valuePtr);
      VectorOfCacheableKey vecKeys;
      regionPtr->keys(vecKeys);
      cacheHelper.showKeys(vecKeys);
      ASSERT(vecKeys.size() == (i + 1), "unexpected entries count");
    }
  }
    catch(...)
    {
     cout << "Failed 2" <<endl;
    }
#endif

END_TEST(TestRegionNoLRU)

BEGIN_TEST(TestRegionLRULocal)
#if 1
  try {
    CacheHelper &cacheHelper = CacheHelper::getHelper();
    RegionPtr regionPtr;
    cacheHelper.createLRURegion(fwtest_Name, regionPtr);
    cout << regionPtr->getFullPath() << endl;
    // put more than 10 items... verify limit is held.
    uint32_t i;
    /** @TODO make this local scope and re-increase the iterations... would also
     * like to time it. */
    for (i = 0; i < 1000; i++) {
      char buf[100];
      sprintf(buf, "%d", i);
      CacheableKeyPtr key = CacheableKey::create(buf);
      sprintf(buf, "value of %d", i);
      CacheableStringPtr valuePtr = cacheHelper.createCacheable(buf);
      regionPtr->put(key, valuePtr);
      VectorOfCacheableKey vecKeys;
      regionPtr->keys(vecKeys);
      ASSERT(vecKeys.size() == (i < 10 ? i + 1 : 10), "expected more entries");
    }
  }
    catch(...)
    {
      cout << "Failed 3" <<endl;
    }
#endif
END_TEST(TestRegionLRULocal)

BEGIN_TEST(TestRecentlyUsedBit)
  // Put twenty items in region. LRU is set to 10.
  // So 10 through 19 should be in region (started at 0)
  // get 15 so it is marked recently used.
  // put 9 more...  check that 15 was skipped for eviction.
  // put 1 more...  15 should then have been evicted.
    try {
  CacheHelper& cacheHelper = CacheHelper::getHelper();
  RegionPtr regionPtr;
  cacheHelper.createLRURegion(fwtest_Name, regionPtr);
  cout << regionPtr->getFullPath() << endl;
  // put more than 10 items... verify limit is held.
  uint32_t i;
  char buf[100];
  for (i = 0; i < 20; i++) {
    sprintf(buf, "%d", i);
    CacheableKeyPtr key = CacheableKey::create(buf);
    sprintf(buf, "value of %d", i);
    CacheableStringPtr valuePtr = cacheHelper.createCacheable(buf);
    regionPtr->put(key, valuePtr);
  }
  sprintf(buf, "%d", 15);
  CacheableStringPtr value2Ptr;
  CacheableKeyPtr key2 = CacheableKey::create(buf);
  value2Ptr = std::dynamic_pointer_cast<CacheableString>(regionPtr->get(key2));
  ASSERT(value2Ptr != nullptr, "expected to find key 15 in cache.");
  for (i = 20; i < 35; i++) {
    sprintf(buf, "%d", i);
    CacheableKeyPtr key = CacheableKey::create(buf);
    CacheableStringPtr valuePtr = cacheHelper.createCacheable(buf);
    regionPtr->put(key, valuePtr);
    VectorOfCacheableKey vecKeys;
    regionPtr->keys(vecKeys);
    cacheHelper.showKeys(vecKeys);
    ASSERT(vecKeys.size() == 10, "expected more entries");
  }
  ASSERT(regionPtr->containsKey(key2), "expected to find key 15 in cache.");
  {
    sprintf(buf, "%d", 35);
    CacheableKeyPtr key = CacheableKey::create(buf);
    CacheableStringPtr valuePtr = cacheHelper.createCacheable(buf);
    regionPtr->put(key, valuePtr);
    VectorOfCacheableKey vecKeys;
    regionPtr->keys(vecKeys);
    cacheHelper.showKeys(vecKeys);
    ASSERT(vecKeys.size() == 10, "expected more entries");
  }
  ASSERT(regionPtr->containsKey(key2) == false, "15 should have been evicted.");
    }
    catch(...)
    {
      cout << "Failed 4" <<endl;
    }
END_TEST(TestRecentlyUsedBit)

BEGIN_TEST(TestEmptiedMap)
  try {
    CacheHelper &cacheHelper = CacheHelper::getHelper();
    RegionPtr regionPtr;
    cacheHelper.createLRURegion(fwtest_Name, regionPtr);
    cout << regionPtr->getFullPath() << endl;
    // put more than 10 items... verify limit is held.
    uint32_t i;
    for (i = 0; i < 10; i++) {
      char buf[100];
      sprintf(buf, "%d", i);
      CacheableKeyPtr key = CacheableKey::create(buf);
      sprintf(buf, "value of %d", i);
      CacheableStringPtr valuePtr = cacheHelper.createCacheable(buf);
      regionPtr->put(key, valuePtr);
      VectorOfCacheableKey vecKeys;
      regionPtr->keys(vecKeys);
      ASSERT(vecKeys.size() == (i + 1), "expected more entries");
    }
    for (i = 0; i < 10; i++) {
      char buf[100];
      sprintf(buf, "%d", i);
      CacheableKeyPtr key = CacheableKey::create(buf);
      regionPtr->destroy(key);
      cout << "removed key " << std::dynamic_pointer_cast<CacheableString>(key)->asChar()
           << endl;
    }
    VectorOfCacheableKey vecKeys;
    regionPtr->keys(vecKeys);
    ASSERT(vecKeys.size() == 0, "expected more entries");
    for (i = 20; i < 40; i++) {
      char buf[100];
      sprintf(buf, "%d", i);
      CacheableKeyPtr key = CacheableKey::create(buf);
      sprintf(buf, "value of %d", i);
      CacheableStringPtr valuePtr = cacheHelper.createCacheable(buf);
      regionPtr->put(key, valuePtr);
    }
    vecKeys.clear();
    regionPtr->keys(vecKeys);
    ASSERT(vecKeys.size() == 10, "expected more entries");
  }
  catch(...)
  {
    cout << "Failed 3" <<endl;
  }
END_TEST(TestEmptiedMap)
