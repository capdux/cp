// cap/test/testCapSct.cp
// Comprehensive unit tests for @cap/sct (RFC 6962 §3.2 / §3.3)

import { test, expect_eq } from "cap:test";
import {
    SctVersion,
    HashAlgorithm,
    SignatureAlgorithm,
    SignedCertificateTimestamp,
    parseSct,
    parseSctList,
    encodeSct,
} from "../../packages/@cap/sct/src/index.cp";

fn makeTestSct(ts: u64, extLen: usize, hashAlg: HashAlgorithm, sigAlg: SignatureAlgorithm): SignedCertificateTimestamp {
    let logId: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 32) {
        logId[i] = u8(0xA0 + i);
        i += 1;
    }
    const ext = if (extLen == 0) "" else "ext!";
    return SignedCertificateTimestamp{
        valid: true,
        version: SctVersion.V1,
        log_id: logId,
        timestamp_ms: ts,
        extensions: ext,
        hash_alg: hashAlg,
        sig_alg: sigAlg,
        signature: "SIGBYTES_DER_1234",
    };
}

fn main() {
    test("1. @cap/sct — parseSct & encodeSct exact RFC 6962 §3.2 roundtrip", () => {
        const orig = makeTestSct(1700000000123 as u64, 0, HashAlgorithm.Sha256, SignatureAlgorithm.Ecdsa);
        let wire: [128]u8 = undefined;
        const n = encodeSct(orig, wire[0..128]);
        // 47 + 0 + 17 = 64 bytes
        expect_eq(n, 64 as usize);
        expect_eq(wire[0], 0 as u8);
        expect_eq(wire[1], 0xA0 as u8);
        expect_eq(wire[32], 0xBF as u8);

        const parsed = parseSct(wire[0..n]);
        expect_eq(parsed.valid, true);
        expect_eq(parsed.version == SctVersion.V1, true);
        expect_eq(parsed.log_id[0], 0xA0 as u8);
        expect_eq(parsed.log_id[31], 0xBF as u8);
        expect_eq(parsed.timestamp_ms, 1700000000123 as u64);
        expect_eq(parsed.extensions.len, 0 as usize);
        expect_eq(parsed.hash_alg == HashAlgorithm.Sha256, true);
        expect_eq(parsed.sig_alg == SignatureAlgorithm.Ecdsa, true);
        expect_eq(parsed.signature.len, 17 as usize);
        expect_eq(parsed.signature[0], 'S' as u8);
        expect_eq(parsed.signature[16], '4' as u8);
    });

    test("2. @cap/sct — parseSct with non-empty extensions & RSA/Ed25519 algorithms", () => {
        const orig = makeTestSct(1650000000000 as u64, 4, HashAlgorithm.Sha512, SignatureAlgorithm.Ed25519);
        let wire: [128]u8 = undefined;
        const n = encodeSct(orig, wire[0..128]);
        expect_eq(n, 68 as usize);

        const parsed = parseSct(wire[0..n]);
        expect_eq(parsed.valid, true);
        expect_eq(parsed.extensions.len, 4 as usize);
        expect_eq(parsed.extensions[0], 'e' as u8);
        expect_eq(parsed.extensions[3], '!' as u8);
        expect_eq(parsed.hash_alg == HashAlgorithm.Sha512, true);
        expect_eq(parsed.sig_alg == SignatureAlgorithm.Ed25519, true);

        let reEncoded: [128]u8 = undefined;
        const n2 = encodeSct(parsed, reEncoded[0..128]);
        expect_eq(n2, n);
        let i: usize = 0;
        while (i < n) {
            expect_eq(reEncoded[i], wire[i]);
            i += 1;
        }
    });

    test("3. @cap/sct — parseSct rejects malformed, truncated & trailing-garbage inputs", () => {
        const orig = makeTestSct(1700000000000 as u64, 0, HashAlgorithm.Sha256, SignatureAlgorithm.Rsa);
        let wire: [128]u8 = undefined;
        const n = encodeSct(orig, wire[0..128]);

        // Truncated below minimum header
        expect_eq(parseSct(wire[0..46]).valid, false);
        // Truncated inside signature
        expect_eq(parseSct(wire[0..(n - 1)]).valid, false);
        // Trailing extra byte
        wire[n] = 0xFF as u8;
        expect_eq(parseSct(wire[0..(n + 1)]).valid, false);

        // Bad version != 0
        wire[0] = 1 as u8;
        expect_eq(parseSct(wire[0..n]).valid, false);
        wire[0] = 0 as u8;

        // Unknown hash algorithm
        wire[43] = 99 as u8;
        expect_eq(parseSct(wire[0..n]).valid, false);
        wire[43] = 4 as u8;

        // Zero signature length
        wire[45] = 0 as u8;
        wire[46] = 0 as u8;
        expect_eq(parseSct(wire[0..47]).valid, false);
    });

    test("4. @cap/sct — parseSctList parses multi-entry RFC 6962 §3.3 TLS extension", () => {
        const s1 = makeTestSct(1700000001000 as u64, 0, HashAlgorithm.Sha256, SignatureAlgorithm.Ecdsa);
        const s2 = makeTestSct(1700000002000 as u64, 4, HashAlgorithm.Sha384, SignatureAlgorithm.Rsa);
        let b1: [128]u8 = undefined;
        let b2: [128]u8 = undefined;
        const n1 = encodeSct(s1, b1[0..128]);
        const n2 = encodeSct(s2, b2[0..128]);

        // Build SignedCertificateTimestampList: 2B total + (2B n1 + b1) + (2B n2 + b2)
        const total: usize = n1 + n2 + (4 as usize);
        let tlsExt: [256]u8 = undefined;
        tlsExt[0] = u8((total >> 8) & 0xFF);
        tlsExt[1] = u8(total & 0xFF);
        let pos: usize = 2;
        tlsExt[pos] = u8((n1 >> 8) & 0xFF);
        tlsExt[pos + 1] = u8(n1 & 0xFF);
        pos += 2;
        let i: usize = 0;
        while (i < n1) {
            tlsExt[pos + i] = b1[i];
            i += 1;
        }
        pos += n1;
        tlsExt[pos] = u8((n2 >> 8) & 0xFF);
        tlsExt[pos + 1] = u8(n2 & 0xFF);
        pos += 2;
        let j: usize = 0;
        while (j < n2) {
            tlsExt[pos + j] = b2[j];
            j += 1;
        }
        pos += n2;

        let outList: [4]SignedCertificateTimestamp = undefined;
        const count = parseSctList(tlsExt[0..pos], outList[0..4]);
        expect_eq(count, 2 as i64);
        expect_eq(outList[0].timestamp_ms, 1700000001000 as u64);
        expect_eq(outList[0].sig_alg == SignatureAlgorithm.Ecdsa, true);
        expect_eq(outList[1].timestamp_ms, 1700000002000 as u64);
        expect_eq(outList[1].extensions.len, 4 as usize);
        expect_eq(outList[1].sig_alg == SignatureAlgorithm.Rsa, true);
    });

    test("5. @cap/sct — parseSctList rejects bad list length & small output buffer", () => {
        const emptyExt: [2]u8 = @as([2]u8, [0x00, 0x00]);
        let outList: [2]SignedCertificateTimestamp = undefined;
        expect_eq(parseSctList(emptyExt[0..2], outList[0..2]), -1 as i64);

        const wrongTotal: [4]u8 = @as([4]u8, [0x00, 0x10, 0x00, 0x01]);
        expect_eq(parseSctList(wrongTotal[0..4], outList[0..2]), -1 as i64);

        // Single valid SCT in list, but out.len == 0
        const s1 = makeTestSct(1700000003000 as u64, 0, HashAlgorithm.Sha256, SignatureAlgorithm.Ecdsa);
        let b1: [128]u8 = undefined;
        const n1 = encodeSct(s1, b1[0..128]);
        const total: usize = n1 + (2 as usize);
        let tlsExt: [140]u8 = undefined;
        tlsExt[0] = u8((total >> 8) & 0xFF);
        tlsExt[1] = u8(total & 0xFF);
        tlsExt[2] = u8((n1 >> 8) & 0xFF);
        tlsExt[3] = u8(n1 & 0xFF);
        let i: usize = 0;
        while (i < n1) {
            tlsExt[4 + i] = b1[i];
            i += 1;
        }
        expect_eq(parseSctList(tlsExt[0..(2 + total)], outList[0..0]), -1 as i64);
    });

    test("6. @cap/sct — encodeSct rejects invalid SCT & insufficient output buffer", () => {
        const bad = parseSct("");
        let buf: [128]u8 = undefined;
        expect_eq(encodeSct(bad, buf[0..128]), 0 as usize);

        const good = makeTestSct(1700000004000 as u64, 0, HashAlgorithm.Sha256, SignatureAlgorithm.Ecdsa);
        expect_eq(encodeSct(good, buf[0..32]), 0 as usize);
        expect_eq(encodeSct(good, buf[0..64]), 64 as usize);
    });
}
