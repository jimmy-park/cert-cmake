# cert-cmake

[![CI](https://github.com/jimmy-park/cert-cmake/actions/workflows/ci.yaml/badge.svg)](https://github.com/jimmy-park/cert-cmake/actions/workflows/ci.yaml)
[![CodeQL](https://github.com/jimmy-park/cert-cmake/actions/workflows/codeql.yaml/badge.svg)](https://github.com/jimmy-park/cert-cmake/actions/workflows/codeql.yaml)
[![Latest](https://github.com/jimmy-park/cert-cmake/actions/workflows/latest.yaml/badge.svg)](https://github.com/jimmy-park/cert-cmake/actions/workflows/latest.yaml)

Generate `cert.h` for loading in-memory cert

Require C++17 due to `inline` variable

## Usage

### CMake Integration

Require CMake 3.23+ due to `target_sources(FILE_SET)`

```CMake
include(FetchContent)

set(CERT_INSTALL ON) # default : OFF

FetchContent_Declare(
    cert-cmake
    URL https://github.com/jimmy-park/cert-cmake/archive/main.tar.gz
)
FetchContent_MakeAvailable(cert-cmake)

# If you're using CPM.cmake
# CPMAddPackage(
#     NAME cert-cmake
#     URL https://github.com/jimmy-park/cert-cmake/archive/main.tar.gz
#     OPTIONS
#     "CERT_INSTALL ON"
# )

add_executable(main main.cpp)
target_link_libraries(main PRIVATE cert::cert)
```

### Example

```cpp
#include <memory>
#include <vector>

#include <cert.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

void LoadCert(void* ssl_context)
{
    using ST_X509_INFO = STACK_OF(X509_INFO);

    static constexpr auto x509_deleter = [](ST_X509_INFO* ptr) {
        if (ptr)
            sk_X509_INFO_pop_free(ptr, X509_INFO_free);
    };

    using X509Ptr = std::unique_ptr<ST_X509_INFO, decltype(x509_deleter)>;

    static const auto x509_ptr {
        []() -> X509Ptr {
            ST_X509_INFO* ptr { nullptr };

            if (BIO* mem = BIO_new_mem_buf(kCert, sizeof(kCert)); mem) {
                ptr = PEM_X509_INFO_read_bio(mem, nullptr, nullptr, nullptr);
                BIO_free_all(mem);
            }

            return { ptr, x509_deleter };
        }()
    };

    static const auto certs = [] {
        std::vector<X509*> x509s;

        for (int first = 0, last = sk_X509_INFO_num(x509_ptr.get()); first < last; ++first) {
            auto* value = sk_X509_INFO_value(x509_ptr.get(), first);
            if (value && value->x509)
                x509s.push_back(value->x509);
        }

        return x509s;
    }();

    if (!ssl_context)
        return;

    auto* x509_store = SSL_CTX_get_cert_store(static_cast<SSL_CTX*>(ssl_context));

    for (const auto x509 : certs)
        X509_STORE_add_cert(x509_store, x509);

    // https://www.openssl.org/docs/manmaster/man3/OPENSSL_thread_stop.html
    OPENSSL_thread_stop();
}
```
