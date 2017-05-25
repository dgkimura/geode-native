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

#pragma once

#include "Region.hpp"
#include "Pool.hpp"
#include "PoolManager.hpp"
#include "PoolFactory.hpp"
#include "CacheableString.hpp"
#include "Cache.hpp"

using namespace System;

namespace Apache
{
  namespace Geode
  {
    namespace Client
    {
      namespace native = apache::geode::client;

      PoolFactory^ PoolManager::CreateFactory(Cache^ cache)
      {
		  
        return PoolFactory::Create(apache::geode::client::getPoolManager()->createFactory());
      }

      const Dictionary<String^, Pool^>^ PoolManager::GetAll()
      {
        auto pools = native::getPoolManager()->getAll();
        auto result = gcnew Dictionary<String^, Pool^>();
        for (native::HashMapOfPools::Iterator iter = pools.begin(); iter != pools.end(); ++iter)
        {
          auto key = CacheableString::GetString(iter.first().get());
          auto val = Pool::Create(iter.second());
          result->Add(key, val);
        }
        return result;
      }

      Pool^ PoolManager::Find(String^ name)
      {
        ManagedString mg_name( name );
		auto pool = native::getPoolManager()->find(mg_name.CharPtr);
        return Pool::Create(pool);
      }

      Pool^ PoolManager::Find(Client::Region<Object^, Object^>^ region)
      {
        return Pool::Create(native::getPoolManager()->find(region->GetNative()));
      }

      void PoolManager::Close(Boolean KeepAlive)
      {
        native::getPoolManager()->close(KeepAlive);
      }

	  void PoolManager::Close()
	  {
		  native::getPoolManager()->close();
	  }
    }  // namespace Client
  }  // namespace Geode
}  // namespace Apache
