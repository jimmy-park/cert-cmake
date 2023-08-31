#include "load_cert.h"

#include <memory>
#include <vector>

#include <cert.h>
#include <openssl/crypto.h>
#include <openssl/ssl.h>

using X509Info = STACK_OF(X509_INFO);

namespace detail {

template <auto func>
struct Deleter {
    template <typename T>
    constexpr void operator()(T* ptr) const
    {
        func(ptr);
    }
};

static void FreeX509Info(X509Info* ptr) { sk_X509_INFO_pop_free(ptr, X509_INFO_free); }
using X509Ptr = std::unique_ptr<X509Info, Deleter<FreeX509Info>>;
using BioPtr = std::unique_ptr<BIO, Deleter<BIO_free_all>>;

} // namespace detail

bool LoadCert(void* ssl_context)
{
    // https://www.openssl.org/docs/manmaster/man3/OPENSSL_thread_stop.html
    struct OpenSSLCleaner {
        ~OpenSSLCleaner() { OPENSSL_thread_stop(); }
    };
    thread_local const OpenSSLCleaner openssl_clenaer;

    static const auto x509_ptr = [] {
        const auto bio_ptr = detail::BioPtr { BIO_new_mem_buf(kCert, sizeof(kCert)) };
        return detail::X509Ptr { PEM_X509_INFO_read_bio(bio_ptr.get(), nullptr, nullptr, nullptr) };
    }();

    static const auto certs = [] {
        std::vector<X509*> x509s;

        for (int first = 0, last = sk_X509_INFO_num(x509_ptr.get()); first < last; ++first) {
            auto* value = sk_X509_INFO_value(x509_ptr.get(), first);
            if (value && value->x509) {
                x509s.push_back(value->x509);
            } else {
                x509s.clear();
                break;
            }
        }

        return x509s;
    }();

    const auto success = [&] {
        if (!x509_ptr)
            return false;

        if (certs.empty())
            return false;

        if (!ssl_context)
            return false;

        auto* x509_store = SSL_CTX_get_cert_store(static_cast<SSL_CTX*>(ssl_context));
        if (!x509_store)
            return false;

        for (const auto x509 : certs) {
            if (X509_STORE_add_cert(x509_store, x509) != 1)
                return false;
        }

        return true;
    }();

    return success;
}
