// Source: cp/cap/test/test_minisign_production_deep.cp (Production deep tests for cap:minisign)

import { test, expect_eq } from "cap:test";
import {
    MINISIGN_UNTRUSTED_PREFIX,
    MINISIGN_TRUSTED_PREFIX,
    MINISIGN_ALG_LEGACY,
    MINISIGN_ALG_PREHASH,
    MINISIGN_PUBKEY_BIN_LEN,
    MINISIGN_SIG_BIN_LEN,
    MINISIGN_GLOBAL_SIG_BIN_LEN,
    MinisignPublicKey,
    MinisignSignature,
    parseMinisignPublicKeyLine,
    parseMinisignPublicKey,
    parseMinisignSignature,
    formatMinisignPublicKey,
    formatMinisignSignature,
} from "cap:minisign";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("1. Minisign 2-line .pub and single-line -P RW... public key parsing with keyIdHex", () => {
    expect_eq(MINISIGN_PUBKEY_BIN_LEN, 42);
    expect_eq(MINISIGN_SIG_BIN_LEN, 74);
    expect_eq(MINISIGN_GLOBAL_SIG_BIN_LEN, 64);
    expect_eq(bytesEq(MINISIGN_ALG_LEGACY, "Ed"), true);
    expect_eq(bytesEq(MINISIGN_ALG_PREHASH, "ED"), true);

    // 真实 Zig 官方发布用 Minisign 公钥向量（42 字节 Base64: RW...）
    const pub_line = "RWSGOq2NVecA2UPNdBUZykf1CCb147pkmdtYxgb3Ti+JO/wCYvhbAb/U";
    const pub_file =
        "untrusted comment: minisign public key: E7620F1842B4E81F\n" +
        pub_line + "\n";

    const pk1 = parseMinisignPublicKey(pub_file) ?? unreachable;
    expect_eq(bytesEq(pk1.untrusted_comment, "minisign public key: E7620F1842B4E81F"), true);

    const pk2 = parseMinisignPublicKeyLine(pub_line) ?? unreachable;
    expect_eq(pk2.untrusted_comment.len, 0);

    let i: usize = 0;
    while (i < 8) {
        expect_eq(pk1.key_id[i], pk2.key_id[i]);
        i += 1;
    }
    let j: usize = 0;
    while (j < 32) {
        expect_eq(pk1.public_key[j], pk2.public_key[j]);
        j += 1;
    }

    // 校验 keyIdHex 输出 16 字节大写十六进制
    let hex_buf: [16]u8 = [0; 16];
    pk1.keyIdHex(*hex_buf);
    // "RWSGOq2NVecA2..." 前 10 字节为 'R','W', 0x64,0x86,0x3A,0xAD,0x8D,0x55,0xE7,0x00 -> "64863AAD8D55E700"
    expect_eq(bytesEq(hex_buf[0..16], "64863AAD8D55E700"), true);
});

test("2. Format and parse round-trip for 2-line .pub and 4-line prehashed ED / legacy Ed .minisig", () => {
    let key_id: [8]u8 = [0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0xCD, 0xEF];
    let pk_bytes: [32]u8 = [0x42; 32];
    let sig_bytes: [64]u8 = [0xA5; 64];
    let global_bytes: [64]u8 = [0x5A; 64];

    // 公钥往返
    let pub_buf: [256]u8 = [0; 256];
    let pub_s: []u8 = pub_buf[0..256];
    const pub_len = formatMinisignPublicKey("cp release key", key_id, pk_bytes, *pub_s) ?? unreachable;
    const pk = parseMinisignPublicKey(pub_buf[0..pub_len]) ?? unreachable;
    expect_eq(bytesEq(pk.untrusted_comment, "cp release key"), true);
    let hex_id: [16]u8 = [0; 16];
    pk.keyIdHex(*hex_id);
    expect_eq(bytesEq(hex_id[0..16], "0123456789ABCDEF"), true);

    // 预哈希 "ED" 4 行签名往返
    let sig_buf: [512]u8 = [0; 512];
    let sig_s: []u8 = sig_buf[0..512];
    const sig_len = formatMinisignSignature(
        "signature from cp release key",
        true,
        key_id,
        sig_bytes,
        "timestamp:1700000000\tfile:cp.tar.gz\thashed",
        global_bytes,
        *sig_s,
    ) ?? unreachable;

    const parsed_ed = parseMinisignSignature(sig_buf[0..sig_len]) ?? unreachable;
    expect_eq(parsed_ed.alg[0], 'E');
    expect_eq(parsed_ed.alg[1], 'D');
    expect_eq(parsed_ed.is_prehashed, true);
    expect_eq(parsed_ed.has_global_sig, true);
    expect_eq(parsed_ed.matchesKeyId(pk), true);
    expect_eq(bytesEq(parsed_ed.untrusted_comment, "signature from cp release key"), true);
    expect_eq(bytesEq(parsed_ed.trusted_comment, "timestamp:1700000000\tfile:cp.tar.gz\thashed"), true);
    expect_eq(parsed_ed.signature[0], 0xA5);
    expect_eq(parsed_ed.signature[63], 0xA5);
    expect_eq(parsed_ed.global_signature[0], 0x5A);
    expect_eq(parsed_ed.global_signature[63], 0x5A);

    // 传统 "Ed" 4 行签名往返 + key_id 不匹配检测
    let sig_legacy_buf: [512]u8 = [0; 512];
    let sig_legacy_s: []u8 = sig_legacy_buf[0..512];
    let other_key_id: [8]u8 = [0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA, 0x99, 0x88];
    const leg_len = formatMinisignSignature(
        "legacy sig",
        false,
        other_key_id,
        sig_bytes,
        "timestamp:1700000001\tfile:legacy.bin",
        global_bytes,
        *sig_legacy_s,
    ) ?? unreachable;

    const parsed_leg = parseMinisignSignature(sig_legacy_buf[0..leg_len]) ?? unreachable;
    expect_eq(parsed_leg.alg[0], 'E');
    expect_eq(parsed_leg.alg[1], 'd');
    expect_eq(parsed_leg.is_prehashed, false);
    expect_eq(parsed_leg.has_global_sig, true);
    expect_eq(parsed_leg.matchesKeyId(pk), false);
});

test("3. OpenBSD Signify 2-line signature compatibility (no trusted comment / global sig)", () => {
    // 构造 4 行后再截断为前 2 行模拟 OpenBSD Signify .sig（且带 CRLF）
    let key_id: [8]u8 = [0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88];
    let sig_bytes: [64]u8 = [0x33; 64];
    let global_bytes: [64]u8 = [0x00; 64];
    let full_buf: [512]u8 = [0; 512];
    let full_s: []u8 = full_buf[0..512];
    const full_len = formatMinisignSignature("verify with openbsd.pub", false, key_id, sig_bytes, "t", global_bytes, *full_s) ?? unreachable;

    // 找到第 2 个 '\n'，只取前 2 行
    let lf_count: usize = 0;
    let cut: usize = 0;
    while (cut < full_len) {
        if (full_buf[cut] == '\n') {
            lf_count += 1;
            if (lf_count == 2) {
                cut += 1;
                break;
            }
        }
        cut += 1;
    }

    const signify = parseMinisignSignature(full_buf[0..cut]) ?? unreachable;
    expect_eq(signify.is_prehashed, false);
    expect_eq(signify.has_global_sig, false);
    expect_eq(signify.trusted_comment.len, 0);
    expect_eq(bytesEq(signify.untrusted_comment, "verify with openbsd.pub"), true);
    expect_eq(signify.key_id[0], 0x11);
    expect_eq(signify.signature[63], 0x33);
});

test("4. Malformed Base64, wrong algorithm tags, wrong decoded lengths, and missing prefixes", () => {
    // 1. 缺 untrusted comment 前缀
    expect_eq(parseMinisignPublicKey("bad comment\nRWSGOq2NVecA2UPNdBUZykf1CCb147pkmdtYxgb3Ti+JO/wCYvhbAb/U\n") == null, true);

    // 2. 公钥算法标识不是 "RW"（将 RW 改为 Ed，Base64 首字符变动）
    // "Ed" + 40 字节 0 -> "RW" 为 0x52 0x57；"Ed" 为 0x45 0x64 -> "RW..." 改为 "RW" 以外的 "RW"->"Ed"
    const wrong_alg_pk = "RW" + "AAAA"; // 长度非法（解码不是 42B）
    expect_eq(parseMinisignPublicKeyLine(wrong_alg_pk) == null, true);
    // 56 字符全 'A' 解码为 42B 全 0x00（算法不是 "RW"）
    const zero_42_b64 = "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
    expect_eq(parseMinisignPublicKeyLine(zero_42_b64) == null, true);

    // 3. 非法 Base64 字符（含 '@'）
    const bad_b64 = "RWSGOq2NVecA2UPNdBUZykf1CCb147pkmdtYxgb3Ti+JO/wCYvhbAb/@";
    expect_eq(parseMinisignPublicKeyLine(bad_b64) == null, true);

    // 4. 签名算法不是 "Ed" / "ED"（100 字符解码为 74B 全 0）
    const zero_74_b64 =
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=";
    expect_eq(parseMinisignSignature("untrusted comment: x\n" + zero_74_b64 + "\n") == null, true);

    // 5. 4 行签名缺少 trusted comment 前缀或缺少第 4 行 global signature
    const missing_trusted =
        "untrusted comment: x\n" +
        "RWRkAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=\n" +
        "wrong prefix: abc\n" +
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA==\n";
    expect_eq(parseMinisignSignature(missing_trusted) == null, true);
});

test("5. Formatter buffer overflow protection returns null safely", () => {
    let key_id: [8]u8 = [1; 8];
    let pk: [32]u8 = [2; 32];
    let sig: [64]u8 = [3; 64];
    let gsig: [64]u8 = [4; 64];

    let tiny: [16]u8 = [0; 16];
    let tiny_s: []u8 = tiny[0..16];
    expect_eq(formatMinisignPublicKey("comment", key_id, pk, *tiny_s) == null, true);
    expect_eq(formatMinisignSignature("u", true, key_id, sig, "t", gsig, *tiny_s) == null, true);
});
