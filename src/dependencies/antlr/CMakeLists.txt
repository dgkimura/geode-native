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
project( antlr )

set( ${PROJECT_NAME}_VERSION 2.7.7 )
set( ${PROJECT_NAME}_SHA265 853aeb021aef7586bda29e74a6b03006bcb565a755c86b66032d8ec31b67dbb9 )
set( ${PROJECT_NAME}_URL "http://www.antlr2.org/download/antlr-${${PROJECT_NAME}_VERSION}.tar.gz" )
set( ${PROJECT_NAME}_EXTERN ${PROJECT_NAME}-extern )

include(ExternalProject)

ExternalProject_Add( ${${PROJECT_NAME}_EXTERN}
   URL ${${PROJECT_NAME}_URL}
   URL_HASH SHA256=${${PROJECT_NAME}_SHA265}
   UPDATE_COMMAND ""
   CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DCMAKE_INSTALL_PREFIX=<INSTALL_DIR>
)

ExternalProject_Get_Property( ${${PROJECT_NAME}_EXTERN} SOURCE_DIR )
set( ${PROJECT_NAME}_SOURCE_DIR ${SOURCE_DIR} )
ExternalProject_Get_Property( ${${PROJECT_NAME}_EXTERN} INSTALL_DIR )
set( ${PROJECT_NAME}_INSTALL_DIR ${INSTALL_DIR} )
set( DEPENDENCIES_${PROJECT_NAME}_DIR ${${PROJECT_NAME}_INSTALL_DIR} PARENT_SCOPE)

ExternalProject_Add_Step( ${${PROJECT_NAME}_EXTERN} CMakeLists.txt
    DEPENDEES patch
    DEPENDERS configure
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.in
    BYPRODUCTS ${${PROJECT_NAME}_SOURCE_DIR}/CMakeLists.txt
    COMMAND ${CMAKE_COMMAND} -E copy  ${CMAKE_CURRENT_SOURCE_DIR}/CMakeLists.txt.in ${${PROJECT_NAME}_SOURCE_DIR}/CMakeLists.txt
)

ExternalProject_Add_Step( ${${PROJECT_NAME}_EXTERN} patches
    ALWAYS 0
    DEPENDEES download
    DEPENDERS patch
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/patches
    BYPRODUCTS ${${PROJECT_NAME}_SOURCE_DIR}/lib/cpp/antlr/CharScanner.hpp
    WORKING_DIRECTORY ${${PROJECT_NAME}_SOURCE_DIR}
    COMMAND ${PATCH} -u -p1 < ${CMAKE_CURRENT_SOURCE_DIR}/patches
)

add_library(${PROJECT_NAME} INTERFACE)
target_include_directories(${PROJECT_NAME} INTERFACE
  $<BUILD_INTERFACE:${${PROJECT_NAME}_INSTALL_DIR}/include>
)
target_link_libraries(${PROJECT_NAME} INTERFACE
  ${${PROJECT_NAME}_INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}${PROJECT_NAME}$<$<CONFIG:Debug>:d>${CMAKE_STATIC_LIBRARY_SUFFIX}
)
add_dependencies(${PROJECT_NAME} ${${PROJECT_NAME}_EXTERN})
