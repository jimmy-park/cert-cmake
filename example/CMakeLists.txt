set(OPENSSL_USE_STATIC_LIBS ON)
find_package(OpenSSL)

if(OPENSSL_FOUND)
    add_executable(main main.cpp load_cert.cpp load_cert.h)
    target_link_libraries(main PRIVATE cert::cert OpenSSL::Crypto OpenSSL::SSL)
endif()
