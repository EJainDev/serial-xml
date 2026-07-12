cmake_minimum_required(VERSION 4.3)

set(CMAKE_CXX_STANDARD 26)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPERIMENTAL_CXX_IMPORT_STD "451f2fe2-a8a2-47c3-bc32-94786d8fc91b")
set(CMAKE_CXX_MODULE_STD 1)

project(
  SerialXML
  VERSION 1.0.0.0
  LANGUAGES CXX)

option(BUILD_TESTS "Enable building of tests" OFF)
option(BUILD_EXAMPLES "Enable building of examples" ON)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

if(BUILD_TESTS)
  include(CTest)
  enable_testing()
endif()

# Library

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -freflection")

add_library(serial_xml STATIC)
target_sources(
  serial_xml
  PUBLIC FILE_SET
         modules
         TYPE
         CXX_MODULES
         BASE_DIRS
         "${CMAKE_CURRENT_SOURCE_DIR}/include"
         FILES
         "${CMAKE_CURRENT_SOURCE_DIR}/include/serial_xml.cxx")
target_include_directories(
  serial_xml PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
                    $<INSTALL_INTERFACE:include>)
target_compile_features(serial_xml PUBLIC cxx_std_26)

# Installation configuration

install(
  TARGETS serial_xml
  EXPORT serial_xml_targets
  FILE_SET modules
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/serial_xml
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

set(SerialXML_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/SerialXML")

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake/SerialXMLConfig.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/SerialXMLConfig.cmake"
  INSTALL_DESTINATION "${SerialXML_INSTALL_CMAKEDIR}")

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/SerialXMLConfigVersion.cmake"
  VERSION "${PROJECT_VERSION}"
  COMPATIBILITY SameMajorVersion)

install(
  EXPORT serial_xml_targets
  NAMESPACE serial_xml::
  DESTINATION "${SerialXML_INSTALL_CMAKEDIR}")

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/SerialXMLConfig.cmake"
              "${CMAKE_CURRENT_BINARY_DIR}/SerialXMLConfigVersion.cmake"
        DESTINATION "${SerialXML_INSTALL_CMAKEDIR}")

if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(BUILD_TESTS)
  add_subdirectory(tests)
endif()
