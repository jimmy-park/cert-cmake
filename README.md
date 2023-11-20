# cert-cmake

[![CI](https://github.com/jimmy-park/cert-cmake/actions/workflows/ci.yaml/badge.svg)](https://github.com/jimmy-park/cert-cmake/actions/workflows/ci.yaml)
[![Latest](https://github.com/jimmy-park/cert-cmake/actions/workflows/latest.yaml/badge.svg)](https://github.com/jimmy-park/cert-cmake/actions/workflows/latest.yaml)

Generate `cert.h` for loading in-memory cert

Require C++17 due to `inline` variable

## CMake Options

| Option              | Default       | Description                        |
| ------------------- | ------------- | ---------------------------------- |
| `CERT_DOWNLOAD_URL` | `(undefined)` | Download a PEM file from URL       |
| `CERT_INSTALL`      | `OFF`         | Install `cert.h` and CMake targets |

- `CERT_DOWNLOAD_URL`
  - When this isn't defined, download the latest CA bundle maintained by [curl](https://curl.se/docs/caextract.html)

## Usage

### Build

```sh
cmake --list-presets all                    # List all CMake presets
cmake --preset windows                      # Configure
cmake --build --preset windows              # Build
ctest --preset windows                      # Test
cmake --build --preset windows -t install   # Install
```

### Integration

Require CMake 3.23+

```CMake
include(FetchContent)
FetchContent_Declare(
    cert-cmake
    URL https://github.com/jimmy-park/cert-cmake/archive/main.tar.gz
)
FetchContent_MakeAvailable(cert-cmake)

# If you're using CPM.cmake
# CPMAddPackage(
#     NAME cert-cmake
#     URL https://github.com/jimmy-park/cert-cmake/archive/main.tar.gz
# )

add_executable(main main.cpp)
target_link_libraries(main PRIVATE cert::cert)
```

## Example

### curl

```cpp
static constexpr auto blob = curl_blob {
    const_cast<char*>(kCert),
    sizeof(kCert),
    CURL_BLOB_COPY
};

auto* curl = curl_easy_init();
curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob);
curl_easy_setopt(curl, CURLOPT_URL, "https://nghttp2.org/httpbin/get");
curl_easy_perform(curl);
curl_easy_cleanup(curl);
```

### OpenSSL

```cpp
template <auto func>
struct Deleter {
    template <typename T>
    constexpr void operator()(T* ptr) const
    {
        func(ptr);
    }
};

using X509Info = STACK_OF(X509_INFO);
void FreeX509Info(X509Info* ptr) { sk_X509_INFO_pop_free(ptr, X509_INFO_free); }
using X509Ptr = std::unique_ptr<X509Info, Deleter<FreeX509Info>>;
using BioPtr = std::unique_ptr<BIO, Deleter<BIO_free_all>>;

void LoadCert(void* ssl_context)
{
    // https://www.openssl.org/docs/manmaster/man3/OPENSSL_thread_stop.html
    struct OpenSSLCleaner {
        ~OpenSSLCleaner() { OPENSSL_thread_stop(); }
    };
    thread_local const OpenSSLCleaner openssl_clenaer;
    
    static const auto x509_ptr = [] {
        const auto bio_ptr = BioPtr { BIO_new_mem_buf(kCert, sizeof(kCert)) };
        return X509Ptr { PEM_X509_INFO_read_bio(bio_ptr.get(), nullptr, nullptr, nullptr) };
    }();

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
}
```

### curl + OpenSSL

```cpp
auto ssl_callback = +[](CURL*, void* ssl_ctx, void*) {
    LoadCert(ssl_ctx);
    return CURLE_OK;
};

auto* curl = curl_easy_init();
curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ssl_callback);
curl_easy_setopt(curl, CURLOPT_URL, "https://nghttp2.org/httpbin/get");
curl_easy_perform(curl);
curl_easy_cleanup(curl);
```
