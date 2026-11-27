// Type definitions for CP

export fn cp_mbedtls_version(buf: [*]u8!, cap: usize): usize;
export fn cp_mbedtls_sha256(data: [*]&u8!?, len: usize, out32: [*]u8!): i32;
export fn cp_mbedtls_aes_gcm_roundtrip_test(): i32;
export error MbedTlsError { BufferTooSmall, HashFailed, AesGcmFailed }

export fn versionInto(buf: []u8): !string;
export fn sha256(data: &[]u8): ![32]u8;
export fn aesGcmRoundtripTest(): bool;
