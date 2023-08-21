#include <cert.h>
#include <curl/curl.h>
#include <openssl/ssl.h>

#include "load_cert.h"

int main()
{
    // curl example
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

    // OpenSSL example
    auto* ctx = SSL_CTX_new(TLS_method());
    LoadCert(ctx);
    SSL_CTX_free(ctx);

    return 0;
}