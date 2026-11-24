// @cap/mbedtls thin C FFI bridge for CP
// License: Apache-2.0

#include "mbedtls/aes.h"
#include "mbedtls/gcm.h"
#include "mbedtls/sha256.h"
#include "mbedtls/version.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

size_t cp_mbedtls_version(char *buf, size_t cap) {
    if (!buf || cap == 0) {
        return 0;
    }
    char tmp[18];
    memset(tmp, 0, sizeof(tmp));
    mbedtls_version_get_string(tmp);
    size_t n = strlen(tmp);
    if (n >= cap) {
        n = cap - 1;
    }
    memcpy(buf, tmp, n);
    buf[n] = '\0';
    return n;
}

int32_t cp_mbedtls_sha256(const uint8_t *data, size_t len, uint8_t *out32) {
    if (!out32 || (!data && len > 0)) {
        return -1;
    }
    const uint8_t empty = 0;
    const uint8_t *src = (len == 0) ? &empty : data;
    return (int32_t)mbedtls_sha256(src, len, out32, 0);
}

int32_t cp_mbedtls_aes_gcm_roundtrip_test(void) {
    static const uint8_t key[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f
    };
    static const uint8_t iv[12] = {
        0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
        0xa8, 0xa9, 0xaa, 0xab
    };
    static const uint8_t aad[8] = { 'c', 'a', 'p', '-', 'a', 'e', 'a', 'd' };
    static const uint8_t pt[16] = {
        'h', 'e', 'l', 'l', 'o', '-', 'm', 'b',
        'e', 'd', 't', 'l', 's', '3', '6', '2'
    };
    uint8_t ct[16];
    uint8_t dec[16];
    uint8_t tag[16];

    mbedtls_gcm_context ctx;
    mbedtls_gcm_init(&ctx);

    int rc = mbedtls_gcm_setkey(&ctx, MBEDTLS_CIPHER_ID_AES, key, 128);
    if (rc == 0) {
        rc = mbedtls_gcm_crypt_and_tag(
            &ctx, MBEDTLS_GCM_ENCRYPT, sizeof(pt),
            iv, sizeof(iv), aad, sizeof(aad), pt, ct, sizeof(tag), tag
        );
    }
    if (rc == 0) {
        rc = mbedtls_gcm_auth_decrypt(
            &ctx, sizeof(ct),
            iv, sizeof(iv), aad, sizeof(aad), tag, sizeof(tag), ct, dec
        );
    }
    mbedtls_gcm_free(&ctx);

    if (rc != 0) {
        return (int32_t)rc;
    }
    return (memcmp(pt, dec, sizeof(pt)) == 0) ? 0 : -1;
}
