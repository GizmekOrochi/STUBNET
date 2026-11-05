cmake_minimum_required(VERSION 3.18)
project(Stubnet VERSION 0.1 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)
# Boost.Asio is header-only here, but the headers are required.
find_path(BOOST_ASIO_INCLUDE_DIR NAMES boost/asio.hpp)
if(NOT BOOST_ASIO_INCLUDE_DIR)
    message(FATAL_ERROR
        "Boost.Asio headers not found (boost/asio.hpp). "
        "On Ubuntu/Debian install them with: sudo apt install libboost-dev")
endif()
add_library(stubnet STATIC
    src/core/sub/network.cpp
    src/core/sub/discovery.cpp
    src/core/sub/peer/peer_manager.cpp
    src/core/sub/event/eventbus.cpp
    src/core/sub/communication/security.cpp
    src/core/sub/communication/transport.cpp
)
target_include_directories(stubnet PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
    ${BOOST_ASIO_INCLUDE_DIR}
)
target_compile_definitions(stubnet PUBLIC BOOST_ERROR_CODE_HEADER_ONLY)
target_link_libraries(stubnet PUBLIC OpenSSL::SSL OpenSSL::Crypto Threads::Threads)
if(MSVC)
    target_compile_options(stubnet PRIVATE /W4)
else()
    target_compile_options(stubnet PRIVATE -Wall -Wextra -Wpedantic)
endif()
add_executable(stubnet_demo src/main.cpp)
target_link_libraries(stubnet_demo PRIVATE stubnet)

enable_testing()
# Like NovaC: one registered test runner linked against the real static library.
file(GLOB STUBNET_TEST_SOURCES CONFIGURE_DEPENDS "tests/*_tests.cpp")
add_executable(stubnet_tests tests/main.cpp ${STUBNET_TEST_SOURCES})
target_include_directories(stubnet_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests)
target_link_libraries(stubnet_tests PRIVATE stubnet)
add_test(NAME stubnet_unit COMMAND stubnet_tests)
set_tests_properties(stubnet_unit PROPERTIES TIMEOUT 30)
