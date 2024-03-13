cmake_minimum_required(VERSION 3.23)

project(
    cert-cmake
    VERSION 1.3.1
    DESCRIPTION "Generate cert.h for loading in-memory cert"
    HOMEPAGE_URL "https://github.com/jimmy-park/cert-cmake"
    LANGUAGES CXX
)

option(CERT_INSTALL "Install cert.h and CMake targets" OFF)

if("${CERT_SOURCE}" MATCHES "^http")
    # Download CA certificates from internet
    file(DOWNLOAD ${CERT_SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/cert.pem)
    file(SIZE ${CMAKE_CURRENT_BINARY_DIR}/cert.pem CERT_DOWNLOAD_SIZE)

    if(CERT_DOWNLOAD_SIZE EQUAL 0)
        message(FATAL_ERROR "Failed to download from ${CERT_SOURCE}")
    endif()
elseif(EXISTS "${CERT_SOURCE}" AND IS_ABSOLUTE "${CERT_SOURCE}" AND NOT IS_DIRECTORY "${CERT_SOURCE}")
    # Copy the local PEM file
    file(COPY_FILE "${CERT_SOURCE}" ${CMAKE_CURRENT_BINARY_DIR}/cert.pem ONLY_IF_DIFFERENT)
else()
    # Download the latest CA bundle maintained by curl
    file(
        DOWNLOAD https://curl.se/ca/cacert-2024-03-11.pem ${CMAKE_CURRENT_BINARY_DIR}/cert.pem
        EXPECTED_HASH SHA256=1794c1d4f7055b7d02c2170337b61b48a2ef6c90d77e95444fd2596f4cac609f
    )
endif()

# Generate cert.h
file(READ ${CMAKE_CURRENT_BINARY_DIR}/cert.pem CERT)
string(REGEX MATCHALL "-----BEGIN CERTIFICATE-----[^-]*-----END CERTIFICATE-----" CERT "${CERT}")
string(REPLACE "\n" ";" CERT "${CERT}")
list(TRANSFORM CERT PREPEND "    \"")
list(TRANSFORM CERT APPEND "\\n\"")
string(REPLACE ";" "\n" CERT "${CERT}")
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cert.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/cert.h
    @ONLY
)

# Create a target
add_library(cert INTERFACE)
add_library(cert::cert ALIAS cert)
target_compile_features(cert INTERFACE cxx_std_17)
target_sources(cert INTERFACE
    FILE_SET HEADERS
    BASE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/include
    FILES ${CMAKE_CURRENT_BINARY_DIR}/include/cert.h
)

# Install/Export targets
if(CERT_INSTALL)
    include(CMakePackageConfigHelpers)
    include(GNUInstallDirs)

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/cert-config-version.cmake
        COMPATIBILITY SameMajorVersion
    )
    install(
        TARGETS cert
        EXPORT certTargets
        FILE_SET HEADERS
    )
    install(
        EXPORT certTargets
        NAMESPACE cert::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cert
    )
    install(
        FILES
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cert-config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cert-config-version.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/cert
    )
endif()

if(PROJECT_IS_TOP_LEVEL AND NOT CMAKE_CROSSCOMPILING)
    include(CTest)
    add_subdirectory(test)
endif()
