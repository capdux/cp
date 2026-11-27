// cap/test/testCapPkcs1.cp
// Comprehensive unit tests for @cap/pkcs1

import { test, expect_eq } from "cap:test";
import {
    PK1_VERSION_TWO_PRIME,
    PK1_ERR_INVALID_VERSION,
    PK1_ERR_NEGATIVE_INTEGER,
    PK1_ERR_INVALID_PEM_LABEL,
    PK1_ERR_INVALID_DER,
    RsaPublicKey,
    RsaPrivateKey,
    modulusBitLen,
    encodeRsaPublicKeyDer,
    decodeRsaPublicKeyDer,
    encodeRsaPublicKeyPem,
    decodeRsaPublicKeyPem,
    encodeRsaPrivateKeyDer,
    decodeRsaPrivateKeyDer,
    encodeRsaPrivateKeyPem,
    decodeRsaPrivateKeyPem,
} from "../../packages/pkcs1/src/index.cp";

fn pk1TestEqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn makeSamplePrivateKey(): RsaPrivateKey {
    let k = RsaPrivateKey.empty();
    k.version = PK1_VERSION_TWO_PRIME;
    k.e = 65537 as u64;
    // 128-byte (1024-bit) modulus with high bit 0x80 set
    k.n_len = 128;
    k.d_len = 128;
    let i: usize = 0;
    while (i < 128) {
        k.n[i] = ((0xA0 + (i % 64)) & 0xFF) as u8;
        k.d[i] = ((0x81 + (i % 97)) & 0xFF) as u8;
        i = i + 1;
    }
    k.n[0] = 0xC3 as u8; // MSB bit 7 set -> requires 0x00 sign-bit padding in DER
    k.n[127] = 0x01 as u8; // odd modulus

    // 64-byte CRT factors (p, q, dp, dq, qinv)
    k.p_len = 64;
    k.q_len = 64;
    k.dp_len = 64;
    k.dq_len = 64;
    k.qinv_len = 64;
    i = 0;
    while (i < 64) {
        k.p[i] = ((0xD0 + (i % 32)) & 0xFF) as u8;
        k.q[i] = ((0xE0 + (i % 31)) & 0xFF) as u8;
        k.dp[i] = ((0x11 + i) & 0xFF) as u8;
        k.dq[i] = ((0x22 + i) & 0xFF) as u8;
        k.qinv[i] = ((0x90 + i) & 0xFF) as u8;
        i = i + 1;
    }
    return k;
}

fn main() {
    test("1. @cap/pkcs1 — RsaPublicKey DER encode & decode with 0x80 high-bit zero padding", () => {
        let mod_bytes: [128]u8 = [0; 128];
        mod_bytes[0] = 0xB5 as u8; // bit 7 is 1 -> DER INTEGER must prepend 0x00
        let i: usize = 1;
        while (i < 128) {
            mod_bytes[i] = (i & 0xFF) as u8;
            i = i + 1;
        }
        const mk = RsaPublicKey.new(mod_bytes[0..128], 65537 as u64);
        expect_eq(mk.ok, true);

        let der_out: [256]u8 = [0; 256];
        const dlen = encodeRsaPublicKeyDer(mk.key, der_out[0..256]);
        // SEQUENCE (0x30 0x81 0x89) + INTEGER n (0x02 0x81 0x81 0x00 || 128 bytes = 132) + INTEGER e (0x02 0x03 01 00 01 = 5) => 3 + 137 = 140 bytes
        expect_eq(dlen, 140 as i64);
        // Verify 0x00 sign-bit padding byte right after INTEGER header (offset 3: 02 81 81 00 B5)
        expect_eq(der_out[3], 0x02 as u8);
        expect_eq(der_out[4], 0x81 as u8);
        expect_eq(der_out[5], 0x81 as u8);
        expect_eq(der_out[6], 0x00 as u8);
        expect_eq(der_out[7], 0xB5 as u8);

        const decoded = decodeRsaPublicKeyDer(der_out[0..140]);
        expect_eq(decoded.ok, true);
        expect_eq(decoded.key.n_len, 128 as usize);
        expect_eq(decoded.key.e, 65537 as u64);
        expect_eq(pk1TestEqBytes(decoded.key.n[0..128], mod_bytes[0..128]), true);
    });

    test("2. @cap/pkcs1 — modulusBitLen exact bit widths (1024 / 2048 / 4096 / non-byte-aligned)", () => {
        // 1024-bit (128 bytes, MSB = 0x80)
        let m1024: [128]u8 = [0; 128];
        m1024[0] = 0x80 as u8;
        expect_eq(modulusBitLen(RsaPublicKey.new(m1024[0..128], 65537 as u64).key), 1024 as usize);

        // 2048-bit (256 bytes, MSB = 0xFF)
        let m2048: [256]u8 = [0; 256];
        m2048[0] = 0xFF as u8;
        expect_eq(modulusBitLen(RsaPublicKey.new(m2048[0..256], 65537 as u64).key), 2048 as usize);

        // 4096-bit (512 bytes, MSB = 0xC0)
        let m4096: [512]u8 = [0; 512];
        m4096[0] = 0xC0 as u8;
        expect_eq(modulusBitLen(RsaPublicKey.new(m4096[0..512], 65537 as u64).key), 4096 as usize);

        // Non-byte-aligned: 0x05 0x00 -> 3 + 8 = 11 bits
        let m11: [2]u8 = [0x05 as u8, 0x00 as u8];
        expect_eq(modulusBitLen(RsaPublicKey.new(m11[0..2], 3 as u64).key), 11 as usize);
    });

    test("3. @cap/pkcs1 — RSA PUBLIC KEY PEM round-trip", () => {
        const sk = makeSamplePrivateKey();
        const pubk = sk.publicKey();

        let pem_buf: [512]u8 = [0; 512];
        const plen = encodeRsaPublicKeyPem(pubk, false, pem_buf[0..512]);
        expect_eq(plen > 0, true);
        expect_eq(pk1TestEqBytes(pem_buf[0..31], "-----BEGIN RSA PUBLIC KEY-----\n"), true);

        const decoded = decodeRsaPublicKeyPem(pem_buf[0..(plen as usize)]);
        expect_eq(decoded.ok, true);
        expect_eq(decoded.key.e, 65537 as u64);
        expect_eq(decoded.key.n_len, 128 as usize);
        expect_eq(pk1TestEqBytes(decoded.key.n[0..128], pubk.n[0..128]), true);
    });

    test("4. @cap/pkcs1 — RsaPrivateKey full 9-field DER round-trip", () => {
        const sk = makeSamplePrivateKey();
        let der_out: [1024]u8 = [0; 1024];
        const dlen = encodeRsaPrivateKeyDer(sk, der_out[0..1024]);
        expect_eq(dlen > 0, true);

        const decoded = decodeRsaPrivateKeyDer(der_out[0..(dlen as usize)]);
        expect_eq(decoded.ok, true);
        expect_eq(decoded.key.version, PK1_VERSION_TWO_PRIME);
        expect_eq(decoded.key.e, 65537 as u64);
        expect_eq(decoded.key.n_len, 128 as usize);
        expect_eq(decoded.key.d_len, 128 as usize);
        expect_eq(decoded.key.p_len, 64 as usize);
        expect_eq(decoded.key.q_len, 64 as usize);
        expect_eq(decoded.key.dp_len, 64 as usize);
        expect_eq(decoded.key.dq_len, 64 as usize);
        expect_eq(decoded.key.qinv_len, 64 as usize);
        expect_eq(pk1TestEqBytes(decoded.key.n[0..128], sk.n[0..128]), true);
        expect_eq(pk1TestEqBytes(decoded.key.d[0..128], sk.d[0..128]), true);
        expect_eq(pk1TestEqBytes(decoded.key.p[0..64], sk.p[0..64]), true);
        expect_eq(pk1TestEqBytes(decoded.key.q[0..64], sk.q[0..64]), true);
        expect_eq(pk1TestEqBytes(decoded.key.dp[0..64], sk.dp[0..64]), true);
        expect_eq(pk1TestEqBytes(decoded.key.dq[0..64], sk.dq[0..64]), true);
        expect_eq(pk1TestEqBytes(decoded.key.qinv[0..64], sk.qinv[0..64]), true);
    });

    test("5. @cap/pkcs1 — RSA PRIVATE KEY PEM round-trip & zeroize() wiping", () => {
        let sk = makeSamplePrivateKey();
        let pem_buf: [1536]u8 = [0; 1536];
        const plen = encodeRsaPrivateKeyPem(sk, false, pem_buf[0..1536]);
        expect_eq(plen > 0, true);
        expect_eq(pk1TestEqBytes(pem_buf[0..32], "-----BEGIN RSA PRIVATE KEY-----\n"), true);

        const decoded = decodeRsaPrivateKeyPem(pem_buf[0..(plen as usize)]);
        expect_eq(decoded.ok, true);
        expect_eq(pk1TestEqBytes(decoded.key.d[0..128], sk.d[0..128]), true);
        expect_eq(pk1TestEqBytes(decoded.key.qinv[0..64], sk.qinv[0..64]), true);

        // Securely wipe private key via @cap/subtle-ct
        sk.zeroize();
        expect_eq(sk.n_len, 0 as usize);
        expect_eq(sk.d_len, 0 as usize);
        expect_eq(sk.e, 0 as u64);
        let any_nonzero = false;
        let i: usize = 0;
        while (i < 512) {
            if (sk.n[i] != (0 as u8) || sk.d[i] != (0 as u8)) {
                any_nonzero = true;
            }
            i = i + 1;
        }
        i = 0;
        while (i < 256) {
            if (sk.p[i] != (0 as u8) || sk.q[i] != (0 as u8)
                || sk.dp[i] != (0 as u8) || sk.dq[i] != (0 as u8)
                || sk.qinv[i] != (0 as u8)) {
                any_nonzero = true;
            }
            i = i + 1;
        }
        expect_eq(any_nonzero, false);
    });

    test("6. @cap/pkcs1 — Reject wrong PEM label, invalid version, negative integer & truncated DER", () => {
        // Wrong PEM label (PRIVATE KEY instead of RSA PRIVATE KEY)
        const wrong_priv_pem = "-----BEGIN PRIVATE KEY-----\nMA0CAQACAwEAAQ==\n-----END PRIVATE KEY-----\n";
        const r_pem1 = decodeRsaPrivateKeyPem(wrong_priv_pem);
        expect_eq(r_pem1.ok, false);
        expect_eq(r_pem1.err, PK1_ERR_INVALID_PEM_LABEL);

        // Wrong PEM label (PUBLIC KEY instead of RSA PUBLIC KEY)
        const wrong_pub_pem = "-----BEGIN PUBLIC KEY-----\nMAgCAQECAwEAAQ==\n-----END PUBLIC KEY-----\n";
        const r_pem2 = decodeRsaPublicKeyPem(wrong_pub_pem);
        expect_eq(r_pem2.ok, false);
        expect_eq(r_pem2.err, PK1_ERR_INVALID_PEM_LABEL);

        // Invalid RSAPrivateKey version = 1 (multi-prime unsupported in two-prime strict mode)
        let bad_ver: [5]u8 = [0x30 as u8, 0x03 as u8, 0x02 as u8, 0x01 as u8, 0x01 as u8];
        const r_ver = decodeRsaPrivateKeyDer(bad_ver[0..5]);
        expect_eq(r_ver.ok, false);
        expect_eq(r_ver.err, PK1_ERR_INVALID_VERSION);

        // Negative modulus INTEGER in RSAPublicKey (02 01 FF without leading 0x00)
        let neg_pub: [8]u8 = [
            0x30 as u8, 0x06 as u8,
            0x02 as u8, 0x01 as u8, 0xFF as u8, // negative!
            0x02 as u8, 0x01 as u8, 0x03 as u8,
        ];
        const r_neg = decodeRsaPublicKeyDer(neg_pub[0..8]);
        expect_eq(r_neg.ok, false);
        expect_eq(r_neg.err, PK1_ERR_NEGATIVE_INTEGER);

        // Truncated DER
        let trunc: [3]u8 = [0x30 as u8, 0x20 as u8, 0x02 as u8];
        const r_trunc = decodeRsaPublicKeyDer(trunc[0..3]);
        expect_eq(r_trunc.ok, false);
        expect_eq(r_trunc.err, PK1_ERR_INVALID_DER);
    });
}
