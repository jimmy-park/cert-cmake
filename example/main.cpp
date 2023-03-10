#include <openssl/ssl.h>

#include "load_cert.h"

int main()
{
    auto* ctx = SSL_CTX_new(TLSv1_2_method());

    LoadCert(ctx);

    SSL_CTX_free(ctx);

    return 0;
}