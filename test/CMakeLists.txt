set(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL)

if(NOT OPENSSL_FOUND)
    return()
endif()

cmake_policy(SET CMP0135 NEW)
include(FetchContent)

set(BUILD_CURL_EXE OFF)
set(BUILD_SHARED_LIBS OFF)
set(CURL_CA_BUNDLE none CACHE STRING "" FORCE)
set(CURL_CA_PATH none CACHE STRING "" FORCE)
set(CURL_DISABLE_TESTS ON)
set(CURL_USE_OPENSSL ON)
set(HTTP_ONLY ON)

FetchContent_Declare(
    curl
    URL https://github.com/curl/curl/archive/master.tar.gz
)
FetchContent_GetProperties(curl)

if(NOT curl_POPULATED)
    FetchContent_Populate(curl)
    add_subdirectory(${curl_SOURCE_DIR} ${curl_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_executable(test_cert
    load_cert.cpp
    load_cert.h
    test_main.cpp
)
target_link_libraries(test_cert PRIVATE
    cert::cert
    CURL::libcurl
    OpenSSL::Crypto
    OpenSSL::SSL
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(test_cert PRIVATE /WX /W4 /permissive-)
else()
    target_compile_options(test_cert PRIVATE
        -Werror -Wall -Wextra -pedantic

        # Disable specific warnings
        -Wno-overlength-strings
    )
endif()

add_test(test_cert test_cert)
