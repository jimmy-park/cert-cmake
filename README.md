# cert-cmake

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
    URL https://github.com/jimmy-park/cert-cmake/archive/1.0.0.tar.gz
)
FetchContent_MakeAvailable(cert-cmake)

# If you're using CPM.cmake
# CPMAddPackage(
#     NAME cert-cmake
#     URL https://github.com/jimmy-park/cert-cmake/archive/1.0.0.tar.gz
#     OPTIONS
#     "CERT_INSTALL ON"
# )

add_executable(main main.cpp)
target_link_libraries(main PRIVATE cert::cert)
```

### Example

```cpp
#include <openssl/ssl.h>
#include <cert.h>

X509_STORE* CreateCertStore()
{
    BIO* mem = nullptr;
    STACK_OF(X509_INFO)* inf = nullptr;
    X509_STORE* cts = nullptr;

    [&] {
        mem = BIO_new_mem_buf(kCert, sizeof(kCert));
        if (!mem)
            return;

        inf = PEM_X509_INFO_read_bio(mem, nullptr, nullptr, nullptr);
        if (!inf)
            return;

        cts = X509_STORE_new();
        if (!cts)
            return;

        for (int first = 0, last = sk_X509_INFO_num(inf); first < last; ++first) {
            X509_INFO* itmp = sk_X509_INFO_value(inf, first);
            if (!itmp)
                continue;

            if (itmp->x509)
                X509_STORE_add_cert(cts, itmp->x509);

            if (itmp->crl)
                X509_STORE_add_crl(cts, itmp->crl);
        }
    }();

    if (inf)
        sk_X509_INFO_pop_free(inf, X509_INFO_free);

    if (mem)
        BIO_free_all(mem);

    return cts;
}
```
