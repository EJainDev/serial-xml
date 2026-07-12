find_package(GTest REQUIRED)

add_executable(serial_xml_tests test_serial_xml.cpp)
target_link_libraries(serial_xml_tests GTest::gtest GTest::gtest_main
                      serial_xml)

include(GoogleTest)
gtest_discover_tests(serial_xml_tests)
