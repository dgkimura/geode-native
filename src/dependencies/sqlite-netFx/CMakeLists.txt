# Licensed to the Apache Software Foundation (ASF) under one or more
# contributor license agreements.  See the NOTICE file distributed with
# this work for additional information regarding copyright ownership.
# The ASF licenses this file to You under the Apache License, Version 2.0
# (the "License"); you may not use this file except in compliance with
# the License.  You may obtain a copy of the License at
# 
#      http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
cmake_minimum_required( VERSION 3.3 )
project( sqlite-netFx )
set( ${PROJECT_NAME}_VERSION 1.0.102.0 )
set( ${PROJECT_NAME}_SHA265 b980e8a151ddc685ed30652d39573381aef7b623478cba80b02d0795607638e1 )
set( ${PROJECT_NAME}_URL "http://system.data.sqlite.org/blobs/${${PROJECT_NAME}_VERSION}/${PROJECT_NAME}451-binary-x64-2013-${${PROJECT_NAME}_VERSION}.zip" )

include(ExternalProject)

ExternalProject_Add( ${PROJECT_NAME}
   URL ${${PROJECT_NAME}_URL}
   URL_HASH SHA256=${${PROJECT_NAME}_SHA265}
  # SOURCE_DIR ${PROJECT_NAME}-prefix
   UPDATE_COMMAND ""
   BUILD_COMMAND ""
   CONFIGURE_COMMAND ""
   INSTALL_COMMAND ${CMAKE_COMMAND} -E copy <SOURCE_DIR>/System.Data.SQLite.dll <INSTALL_DIR>
)

ExternalProject_Get_Property( ${PROJECT_NAME} SOURCE_DIR )
set( ${PROJECT_NAME}_SOURCE_DIR ${SOURCE_DIR} )
ExternalProject_Get_Property( ${PROJECT_NAME} INSTALL_DIR )
set( ${PROJECT_NAME}_INSTALL_DIR ${INSTALL_DIR} )

set( DEPENDENCIES_${PROJECT_NAME}_DIR ${${PROJECT_NAME}_INSTALL_DIR} PARENT_SCOPE)
set( DEPENDENCIES_${PROJECT_NAME}_VERSION ${${PROJECT_NAME}_VERSION} PARENT_SCOPE)
set( ${PROJECT_NAME}_SHARED_LIB ${${PROJECT_NAME}_INSTALL_DIR}/System.Data.SQLite.dll PARENT_SCOPE)
