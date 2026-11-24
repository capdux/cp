/**
 * Minimal Mbed TLS 3.6.2 configuration for @cap/mbedtls.
 * Self-contained pure-C core crypto subset (zero external dependencies).
 */
#ifndef MBEDTLS_CONFIG_H
#define MBEDTLS_CONFIG_H

/* Core symmetric & AEAD ciphers */
#define MBEDTLS_CIPHER_C
#define MBEDTLS_AES_C
#define MBEDTLS_GCM_C
#define MBEDTLS_CHACHA20_C
#define MBEDTLS_POLY1305_C
#define MBEDTLS_CHACHAPOLY_C

/* Core cryptographic hashes */
#define MBEDTLS_SHA224_C
#define MBEDTLS_SHA256_C
#define MBEDTLS_SHA384_C
#define MBEDTLS_SHA512_C

/* Encoding & version metadata */
#define MBEDTLS_BASE64_C
#define MBEDTLS_VERSION_C
#define MBEDTLS_VERSION_FEATURES

#endif /* MBEDTLS_CONFIG_H */
