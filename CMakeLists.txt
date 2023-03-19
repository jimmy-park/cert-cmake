cmake_minimum_required(VERSION 3.23)

project(
    cert-cmake
    VERSION 1.1.0
    DESCRIPTION "Generate cert.h for loading in-memory cert"
    HOMEPAGE_URL "https://github.com/jimmy-park/cert-cmake"
    LANGUAGES CXX
)

# Custom options
option(CERT_INSTALL "Install cert.h and CMake targets" OFF)

# Download extracted CA certs from curl
file(
    DOWNLOAD https://curl.se/ca/cacert-2023-01-10.pem ${CMAKE_CURRENT_BINARY_DIR}/cert.pem
    EXPECTED_HASH SHA256=fb1ecd641d0a02c01bc9036d513cb658bbda62a75e246bedbc01764560a639f0
)

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

if(PROJECT_IS_TOP_LEVEL)
    add_subdirectory(example)
endif()
