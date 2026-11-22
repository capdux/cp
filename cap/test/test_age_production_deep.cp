// Source: cp/cap/test/test_age_production_deep.cp (Production deep tests for cap:age)

import { test, expect_eq } from "cap:test";
import {
    AGE_VERSION_LINE,
    AGE_ARMOR_BEGIN,
    AGE_ARMOR_END,
    AGE_STREAM_CHUNK_SIZE,
    AGE_STREAM_TAG_SIZE,
    AGE_NONCE_SIZE,
    AgeStanza,
    AgeHeader,
    AgeStanzaIter,
    isAge,
    isAgeArmored,
    parseAgeHeader,
    isValidAgePublicKey,
    isValidAgeSecretKey,
} from "cap:crypto";

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

test("1. Full X25519 + scrypt multi-stanza age v1 header, header_for_mac, MAC, and payloadNonce", () => {
    expect_eq(bytesEq(AGE_VERSION_LINE, "age-encryption.org/v1"), true);
    expect_eq(AGE_STREAM_CHUNK_SIZE, 65536);
    expect_eq(AGE_STREAM_TAG_SIZE, 16);
    expect_eq(AGE_NONCE_SIZE, 16);

    const file_data =
        "age-encryption.org/v1\n" +
        "-> X25519 8h2lK3mN9pQ1rS4tU6vW8xY0zA2bC4dE6fG8hI0jK2l\n" +
        "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXowMTIzNDU\n" +
        "-> scrypt G8K2mN9pQ1rS4tU6vW8xYw 18\n" +
        "MDEyMzQ1Njc4OWFiY2RlZmdoaWprbG1ub3BxcnN0dXY\n" +
        "--- AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n" +
        "0123456789ABCDEFpayload_ciphertext";

    expect_eq(isAge(file_data), true);

    const hdr = parseAgeHeader(file_data) ?? unreachable;
    expect_eq(hdr.stanza_count, 2);
    expect_eq(bytesEq(hdr.mac_b64, "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA"), true);
    // header_for_mac 必须精确以 "---" 结尾（不含后面的空格与 MAC）
    expect_eq(hdr.header_for_mac.len >= 3, true);
    expect_eq(bytesEq(hdr.header_for_mac[hdr.header_for_mac.len - 3..hdr.header_for_mac.len], "---"), true);

    const nonce = hdr.payloadNonce() ?? unreachable;
    expect_eq(nonce.len, 16);
    expect_eq(bytesEq(nonce, "0123456789ABCDEF"), true);
    expect_eq(bytesEq(hdr.payload[16..hdr.payload.len], "payload_ciphertext"), true);

    let it = AgeStanzaIter.init(hdr.stanzas_slice);
    const s1 = it.next() ?? unreachable;
    expect_eq(bytesEq(s1.stanza_type, "X25519"), true);
    expect_eq(s1.argCount(), 1);
    expect_eq(bytesEq(s1.argAt(0) ?? "", "8h2lK3mN9pQ1rS4tU6vW8xY0zA2bC4dE6fG8hI0jK2l"), true);
    expect_eq(s1.argAt(1) == null, true);
    expect_eq(bytesEq(s1.body_b64, "YWJjZGVmZ2hpamtsbW5vcHFyc3R1dnd4eXowMTIzNDU"), true);

    const s2 = it.next() ?? unreachable;
    expect_eq(bytesEq(s2.stanza_type, "scrypt"), true);
    expect_eq(s2.argCount(), 2);
    expect_eq(bytesEq(s2.argAt(0) ?? "", "G8K2mN9pQ1rS4tU6vW8xYw"), true);
    expect_eq(bytesEq(s2.argAt(1) ?? "", "18"), true);
    expect_eq(s2.argAt(2) == null, true);

    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("2. Canonical 64-column wrapped stanza body (including exact 64-char + empty final line)", () => {
    const line64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    expect_eq(line64.len, 64);

    // Stanza 1: 64B + 12B 折行；Stanza 2: 64B + 空终结行（规范 48B 整数倍编码）
    const wrapped =
        "age-encryption.org/v1\n" +
        "-> ssh-rsa abcd1234\n" +
        line64 + "\n" +
        "MTIzNDU2Nzg5\n" +
        "-> ssh-ed25519 efgh5678\n" +
        line64 + "\n" +
        "\n" +
        "--- BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB\n" +
        "0123456789ABCDEF";

    const hdr = parseAgeHeader(wrapped) ?? unreachable;
    expect_eq(hdr.stanza_count, 2);

    let it = AgeStanzaIter.init(hdr.stanzas_slice);
    const s1 = it.next() ?? unreachable;
    expect_eq(bytesEq(s1.stanza_type, "ssh-rsa"), true);
    expect_eq(s1.body_b64.len, 64 + 1 + 12);

    const s2 = it.next() ?? unreachable;
    expect_eq(bytesEq(s2.stanza_type, "ssh-ed25519"), true);
    expect_eq(s2.body_b64.len, 64 + 1); // 64 字符 + 换行符 + 空末行
    expect_eq(it.next() == null, true);
    expect_eq(it.had_error, false);
});

test("3. Bech32 age1 public key and AGE-SECRET-KEY-1 secret key validation", () => {
    // 合法 62B 公钥（age1 + 58 小写 bech32）
    const valid_pub = "age1ql3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p";
    expect_eq(valid_pub.len, 62);
    expect_eq(isValidAgePublicKey(valid_pub), true);

    // 合法 74B 私钥（AGE-SECRET-KEY-1 + 58 大写 bech32）
    const valid_sec = "AGE-SECRET-KEY-1QQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ";
    expect_eq(valid_sec.len, 74);
    expect_eq(isValidAgeSecretKey(valid_sec), true);

    // 非法公钥：长度不对、包含非法 bech32 字符 'b'/'i'/'o'/'1' 或大写
    expect_eq(isValidAgePublicKey("age1short"), false);
    expect_eq(isValidAgePublicKey("age1bl3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p"), false);
    expect_eq(isValidAgePublicKey("age1il3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p"), false);
    expect_eq(isValidAgePublicKey("age1ol3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p"), false);
    expect_eq(isValidAgePublicKey("age11l3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p"), false);
    expect_eq(isValidAgePublicKey("age1QL3z7hjy54pw3hyww5ayyfg7zqgvc7w3j2elw8zmrj2kg5sfn9aqmcac8p"), false);

    // 非法私钥：小写或含 'O'/'B'/'I'/'1'
    expect_eq(isValidAgeSecretKey("AGE-SECRET-KEY-1qqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqqq"), false);
    expect_eq(isValidAgeSecretKey("AGE-SECRET-KEY-1OQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQQ"), false);
});

test("4. ASCII Armor detection with leading whitespace", () => {
    const armored =
        "  \n\t" +
        AGE_ARMOR_BEGIN + "\n" +
        "YWdlLWVuY3J5cHRpb24ub3JnL3YxCg==\n" +
        AGE_ARMOR_END + "\n";

    expect_eq(isAgeArmored(armored), true);
    expect_eq(isAge(armored), false);
    expect_eq(isAgeArmored("-----BEGIN PGP MESSAGE-----\n"), false);
    expect_eq(isAgeArmored(""), false);
});

test("5. Strict age v1 rejections: CRLF, bad MAC length, zero stanzas, trailing space, padded base64, short payload", () => {
    // 1. 拒绝 CRLF (\r\n)
    const crlf =
        "age-encryption.org/v1\r\n" +
        "-> X25519 abc\n" +
        "AAAA\n" +
        "--- AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
    expect_eq(isAge(crlf), false);
    expect_eq(parseAgeHeader(crlf) == null, true);

    // 2. 拒绝 0 个 Stanza
    const no_stanza =
        "age-encryption.org/v1\n" +
        "--- AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
    expect_eq(parseAgeHeader(no_stanza) == null, true);

    // 3. 拒绝空 MAC ("---\n") 与非 43B MAC
    const empty_mac =
        "age-encryption.org/v1\n" +
        "-> X25519 abc\n" +
        "AAAA\n" +
        "---\n";
    expect_eq(parseAgeHeader(empty_mac) == null, true);

    const short_mac =
        "age-encryption.org/v1\n" +
        "-> X25519 abc\n" +
        "AAAA\n" +
        "--- AAAAAAAAAA\n";
    expect_eq(parseAgeHeader(short_mac) == null, true);

    // 4. 拒绝 Stanza 首行尾随空格与带 '=' 填充的 Base64
    const trailing_space =
        "age-encryption.org/v1\n" +
        "-> X25519 abc \n" +
        "AAAA\n" +
        "--- AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
    expect_eq(parseAgeHeader(trailing_space) == null, true);

    const padded_b64 =
        "age-encryption.org/v1\n" +
        "-> X25519 abc\n" +
        "AAA=\n" +
        "--- AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n";
    expect_eq(parseAgeHeader(padded_b64) == null, true);

    // 5. 合法头但 payload 不足 16B 时 payloadNonce 返回 null
    const short_payload =
        "age-encryption.org/v1\n" +
        "-> X25519 abc\n" +
        "AAAA\n" +
        "--- AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA\n" +
        "short";
    const hdr = parseAgeHeader(short_payload) ?? unreachable;
    expect_eq(hdr.payloadNonce() == null, true);
});
