cmake_minimum_required(VERSION 3.10.2)
project(ProjectTemplate)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DURHO3D_TEMPLATE")
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DURHO3D_TEMPLATE")

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake/Modules)
include(UrhoCommon)

add_subdirectory(app)
