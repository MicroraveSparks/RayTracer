#include <User.h>
#include <string.h>
#include <mbedtls/sha256.h>

const unsigned char* hash_password(const char* password) {
    static unsigned char output_hash[32]; // SHA-256 produces a 32-byte hash
    mbedtls_sha256_context ctx;
    mbedtls_sha256_init(&ctx);
    mbedtls_sha256_starts(&ctx, 0); // 0 for SHA-256, 1 for SHA-224
    mbedtls_sha256_update(&ctx, (const unsigned char*)password, strlen(password));
    mbedtls_sha256_finish(&ctx, output_hash);
    mbedtls_sha256_free(&ctx);
    return output_hash;
}