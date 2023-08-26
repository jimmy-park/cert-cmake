#include <iostream>

#include <cert.h>
#include <curl/curl.h>
#include <openssl/ssl.h>

#include "load_cert.h"

struct CurlCleaner {
    CurlCleaner() { curl_global_init(CURL_GLOBAL_ALL); }
    ~CurlCleaner() { curl_global_cleanup(); }
} curl_cleaner;

int main()
{
    // curl example
    std::cout << "==== curl example ====\n";

    static constexpr auto blob = curl_blob {
        const_cast<char*>(kCert),
        sizeof(kCert),
        CURL_BLOB_COPY
    };

    long response_code = 0;
    auto* curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob);
    curl_easy_setopt(curl, CURLOPT_URL, "https://nghttp2.org/httpbin/get");
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    if (curl_easy_perform(curl) == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);

    if (response_code != 200)
        return -1;

    // OpenSSL example
    std::cout << "==== OpenSSL example ====\n";

    auto* ctx = SSL_CTX_new(TLS_method());
    auto success = LoadCert(ctx);
    SSL_CTX_free(ctx);

    if (!success)
        return -1;

    std::cout << "Success\n";

    // curl + OpenSSL example
    std::cout << "==== curl + OpenSSL example ====\n";

    auto ssl_callback = +[](CURL*, void* ssl_ctx, void*) {
        LoadCert(ssl_ctx);
        return CURLE_OK;
    };

    response_code = 0;
    curl = curl_easy_init();
    curl_easy_setopt(curl, CURLOPT_SSL_CTX_FUNCTION, ssl_callback);
    curl_easy_setopt(curl, CURLOPT_URL, "https://nghttp2.org/httpbin/get");
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    if (curl_easy_perform(curl) == CURLE_OK)
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    curl_easy_cleanup(curl);

    if (response_code != 200)
        return -1;

    return 0;
}