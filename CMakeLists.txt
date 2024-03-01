cmake_minimum_required(VERSION 3.7.1)
project(cim_proc VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDART 17)

set(SOURCES main.cpp)

set(PUGIXML_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../../Components/pugixml/src")

set(PUGIXML_SOURCES 
        ${PUGIXML_DIR}/pugixml.cpp
        ${PUGIXML_DIR}/pugiconfig.hpp
        ${PUGIXML_DIR}/pugixml.hpp)

add_executable(cim_proc ${SOURCES} ${PUGIXML_SOURCES})

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
