#pragma once

#ifndef GEODE_DISTRIBUTEDSYSTEM_H_
#define GEODE_DISTRIBUTEDSYSTEM_H_

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

/**
 * @file
 */

#include "geode_globals.hpp"
#include "geode_types.hpp"
#include "ExceptionTypes.hpp"
#include "Properties.hpp"
#include "VectorT.hpp"
#include <mutex>
namespace apache {
namespace geode {
namespace client {
/**
 * @class DistributedSystem DistributedSystem.hpp
 * DistributedSystem encapsulates this applications "connection" into the
 * Geode Java servers distributed system. In order to participate in the
 * Geode Java servers distributed system, each application needs to connect to
 * the DistributedSystem.
 * Each application can only be connected to one DistributedSystem.
 */
class SystemProperties;
class DistributedSystemImpl;
class CacheRegionHelper;
class DiffieHellman;
class TcrConnection;

class CPPCACHE_EXPORT DistributedSystem : public SharedBase {
  /**
   * @brief public methods
   */
 public:
  /**
   * Initializes the Native Client system to be able to connect to the
   * Geode Java servers. If the name string is empty, then the default
   * "NativeDS" is used as the name of distributed system.
   * @throws LicenseException if no valid license is found.
   * @throws IllegalStateException if GFCPP variable is not set and
   *   product installation directory cannot be determined
   * @throws IllegalArgument exception if DS name is NULL
   * @throws AlreadyConnectedException if this call has succeeded once before
   *for this process
   **/
  bool connect(const char* name, const PropertiesPtr& configPtr);

  /**
   *@brief disconnect from the distributed system
   *@throws IllegalStateException if not connected
   */
  void disconnect();

  /** Returns the SystemProperties that were used to create this instance of the
   *  DistributedSystem
   * @return  SystemProperties
   */
  static SystemProperties* getSystemProperties();

  /** Returns the name that identifies the distributed system instance
   * @return  name
   */
  virtual const char* getName() const;

  /** Returns  true if connected, false otherwise
   *
   * @return  true if connected, false otherwise
   */
   bool isConnected();

  /**
   * @brief destructor
   */
  virtual ~DistributedSystem();

  /**
   * @brief constructors
   */
  DistributedSystem(const char* name);
 protected:

 private:
  char* m_name;
  bool m_connected;

 public:
  DistributedSystemImpl* m_impl;

  friend class CacheRegionHelper;
  friend class DistributedSystemImpl;
  friend class TcrConnection;

 private:
  DistributedSystem(const DistributedSystem&);

  const DistributedSystem& operator=(const DistributedSystem&);
  std::mutex m_disconnectLock;

};
}  // namespace client
}  // namespace geode
}  // namespace apache

#endif  // GEODE_DISTRIBUTEDSYSTEM_H_
