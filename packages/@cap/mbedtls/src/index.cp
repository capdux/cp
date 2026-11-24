// @cap/mbedtls — ARM Mbed TLS 3.6.2 core cryptography suite for CP
// License: Apache-2.0 (Upstream: Mbed-TLS/mbedtls v3.6.2)

pub @c {
    fn cp_mbedtls_version(buf: [*]u8!, cap: usize): usize;
    fn cp_mbedtls_sha256(data: [*]const u8?!, len: usize, out32: [*]u8!): i32;
    fn cp_mbedtls_aes_gcm_roundtrip_test(): i32;
}

pub err MbedTlsError {
    BufferTooSmall,
    HashFailed,
    AesGcmFailed,
}

/// Writes the linked Mbed TLS version string (e.g. "3.6.2") into `buf` and returns the slice.
pub fn versionInto(buf: []u8): string! {
    if (buf.len < 2) return MbedTlsError.BufferTooSmall;
    let n: usize = 0;
    unsafe {
        n = cp_mbedtls_version(buf.ptr, buf.len);
    }
    if (n == 0) return MbedTlsError.BufferTooSmall;
    return buf[0..n];
}

/// Computes the 32-byte SHA-256 digest of `data`.
pub fn sha256(data: []const u8): [32]u8! {
    let out: [32]u8 = [0; 32];
    let rc: i32 = 0;
    unsafe {
        const ptr: [*]const u8?! = if (data.len == 0) null else data.ptr;
        rc = cp_mbedtls_sha256(ptr, data.len, &out);
    }
    if (rc != 0) return MbedTlsError.HashFailed;
    return out;
}

/// Runs an internal AES-128-GCM encrypt/decrypt/authenticate self-test.
pub fn aesGcmRoundtripTest(): bool {
    let rc: i32 = -1;
    unsafe {
        rc = cp_mbedtls_aes_gcm_roundtrip_test();
    }
    return rc == 0;
}
