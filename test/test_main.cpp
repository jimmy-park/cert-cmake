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

    long response_code = 0;
    auto* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob);
    curl_easy_setopt(curl, CURLOPT_URL, "https://nghttp2.org/httpbin/get");

    if (curl_easy_perform(curl) == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);

    if (response_code != 200)
        return -1;

    // OpenSSL example
    auto* ctx = SSL_CTX_new(TLS_method());
    auto success = LoadCert(ctx);
    SSL_CTX_free(ctx);

    if (!success)
        return -1;

    // curl + OpenSSL example
    auto ssl_callback = +[](CURL*, void* ssl_ctx, void*) {
        LoadCert(ssl_ctx);
        return CURLE_OK;
    };

    response_code = 0;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ssl_callback);
    curl_easy_setopt(curl, CURLOPT_URL, "https://nghttp2.org/httpbin/get");

    if (curl_easy_perform(curl) == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);

    if (response_code != 200)
        return -1;

    return 0;
}