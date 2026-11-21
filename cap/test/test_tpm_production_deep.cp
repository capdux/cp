// Source: cp/cap/test/test_tpm_production_deep.cp (Production deep tests for cap:tpm)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    TPM_HEADER_SIZE,
    TPM_GENERATED_VALUE,
    TPM_ST_NO_SESSIONS,
    TPM_ST_SESSIONS,
    TPM_ST_ATTEST_CERTIFY,
    TPM_ST_ATTEST_QUOTE,
    TPM_ST_ATTEST_TIME,
    TPM_ST_ATTEST_CREATION,
    TPM_ALG_RSA,
    TPM_ALG_SHA1,
    TPM_ALG_KEYEDHASH,
    TPM_ALG_SHA256,
    TPM_ALG_SHA384,
    TPM_ALG_SHA512,
    TPM_ALG_ECC,
    TPM_ALG_SYMCIPHER,
    TPM_CC_STARTUP,
    TPM_CC_QUOTE,
    TPM_CC_GET_CAPABILITY,
    TPM_CC_GET_RANDOM,
    TPM_CC_PCR_READ,
    TPM_RC_SUCCESS,
    ATTR_FIXED_TPM,
    ATTR_FIXED_PARENT,
    ATTR_SENSITIVE_DATA_ORIGIN,
    ATTR_USER_WITH_AUTH,
    ATTR_RESTRICTED,
    ATTR_DECRYPT,
    ATTR_SIGN_ENCRYPT,
    TpmHeader,
    parseTpmHeader,
    encodeTpmHeader,
    TpmClockInfo,
    TpmAttest,
    isTpmAttest,
    parseTpmAttest,
    TpmPcrSelection,
    TpmPcrSelectIter,
    TpmQuoteInfo,
    parseTpmQuoteInfo,
    TpmPublicHeader,
    parseTpmPublicHeader,
} from "cap:tpm";

fn putU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn putU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn putU64Be(buf: []u8, off: usize, v: u64) {
    putU32Be(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    putU32Be(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn copyBytes(dst: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        dst[off + i] = src[i];
        i += 1;
    }
}

test("1. TPM 2.0 10B command & response header encode/parse roundtrip", () => {
    expect_eq(TPM_HEADER_SIZE, 10 as usize);
    expect_eq(TPM_GENERATED_VALUE, 0xFF544347 as u32);
    expect_eq(TPM_ST_ATTEST_TIME, 0x8019 as u16);
    expect_eq(TPM_ST_ATTEST_CREATION, 0x801A as u16);
    expect_eq(TPM_ALG_KEYEDHASH, 0x0008 as u16);
    expect_eq(TPM_ALG_SHA384, 0x000C as u16);
    expect_eq(TPM_ALG_SHA512, 0x000D as u16);
    expect_eq(TPM_ALG_SYMCIPHER, 0x0025 as u16);
    expect_eq(TPM_CC_STARTUP, 0x00000144 as u32);
    expect_eq(TPM_CC_QUOTE, 0x00000158 as u32);
    expect_eq(TPM_CC_GET_CAPABILITY, 0x0000017A as u32);
    expect_eq(TPM_CC_PCR_READ, 0x0000017E as u32);

    // Command: TPM2_GetRandom (no sessions, 2-byte body requesting 32 bytes)
    let cmd: [16]u8 = std.mem.zeroes([16]u8);
    const hdr_len = encodeTpmHeader(TPM_ST_NO_SESSIONS, TPM_CC_GET_RANDOM, 2, cmd[0..]) ?? unreachable;
    expect_eq(hdr_len, 10 as usize);
    putU16Be(cmd[0..], 10, 32);

    const parsed_cmd = parseTpmHeader(cmd[0..]) ?? unreachable;
    expect_eq(parsed_cmd.tag, TPM_ST_NO_SESSIONS);
    expect_eq(parsed_cmd.size, 12 as u32);
    expect_eq(parsed_cmd.code, TPM_CC_GET_RANDOM);
    expect_eq(parsed_cmd.hasSessions(), false);
    expect_eq(parsed_cmd.isSuccess(), false);
    expect_eq(parsed_cmd.body.len, 2 as usize);
    expect_eq(parsed_cmd.body[0], 0 as u8);
    expect_eq(parsed_cmd.body[1], 32 as u8);

    // Response: TPM_RC_SUCCESS with sessions
    let rsp: [20]u8 = std.mem.zeroes([20]u8);
    const rsp_hdr = encodeTpmHeader(TPM_ST_SESSIONS, TPM_RC_SUCCESS, 10, rsp[0..]) ?? unreachable;
    expect_eq(rsp_hdr, 10 as usize);
    rsp[10] = 0xAB;
    const parsed_rsp = parseTpmHeader(rsp[0..]) ?? unreachable;
    expect_eq(parsed_rsp.hasSessions(), true);
    expect_eq(parsed_rsp.isSuccess(), true);
    expect_eq(parsed_rsp.body.len, 10 as usize);
    expect_eq(parsed_rsp.body[0], 0xAB as u8);
});

test("2. Full TPMS_ATTEST + TPM_ST_ATTEST_QUOTE & TPMS_QUOTE_INFO PCR Selection bitmask", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    let pos: usize = 0;

    // magic (4B) + type (2B)
    putU32Be(buf[0..], pos, TPM_GENERATED_VALUE);
    pos += 4;
    putU16Be(buf[0..], pos, TPM_ST_ATTEST_QUOTE);
    pos += 2;

    // qualifiedSigner (TPM2B_NAME: 4 bytes)
    putU16Be(buf[0..], pos, 4);
    pos += 2;
    buf[pos] = 0x00;
    buf[pos + 1] = 0x0B;
    buf[pos + 2] = 0xDE;
    buf[pos + 3] = 0xAD;
    pos += 4;

    // extraData / nonce (TPM2B_DATA: 6 bytes)
    putU16Be(buf[0..], pos, 6);
    pos += 2;
    copyBytes(buf[0..], pos, "NONCE1");
    pos += 6;

    // TPMS_CLOCK_INFO (17B)
    putU64Be(buf[0..], pos, 9876543210);
    pos += 8;
    putU32Be(buf[0..], pos, 42); // reset_count
    pos += 4;
    putU32Be(buf[0..], pos, 7); // restart_count
    pos += 4;
    buf[pos] = 1; // safe = true
    pos += 1;

    // firmwareVersion (8B)
    putU64Be(buf[0..], pos, 0x2026000100020003);
    pos += 8;

    // TPMS_QUOTE_INFO: 2 PCR selection banks (SHA256: PCR 0, 1, 7; SHA1: PCR 10)
    putU32Be(buf[0..], pos, 2); // count = 2
    pos += 4;

    // Bank 0: SHA256, 3 bytes, PCR 0 (bit 0), PCR 1 (bit 1), PCR 7 (bit 7) => 0x83, 0x00, 0x00
    putU16Be(buf[0..], pos, TPM_ALG_SHA256);
    pos += 2;
    buf[pos] = 3;
    pos += 1;
    buf[pos] = 0x83;
    buf[pos + 1] = 0x00;
    buf[pos + 2] = 0x00;
    pos += 3;

    // Bank 1: SHA1, 3 bytes, PCR 10 (byte 1, bit 2 = 0x04)
    putU16Be(buf[0..], pos, TPM_ALG_SHA1);
    pos += 2;
    buf[pos] = 3;
    pos += 1;
    buf[pos] = 0x00;
    buf[pos + 1] = 0x04;
    buf[pos + 2] = 0x00;
    pos += 3;

    // pcrDigest (TPM2B_DIGEST: 32 bytes)
    putU16Be(buf[0..], pos, 32);
    pos += 2;
    let d: usize = 0;
    while (d < 32) {
        buf[pos + d] = (0xA0 + d) as u8;
        d += 1;
    }
    pos += 32;

    expect_eq(isTpmAttest(buf[0..pos]), true);
    const att = parseTpmAttest(buf[0..pos]) ?? unreachable;
    expect_eq(att.isQuote(), true);
    expect_eq(att.isCertify(), false);
    expect_eq(att.qualified_signer.len, 4 as usize);
    expect_eq(att.qualified_signer[2], 0xDE as u8);
    expect_eq(att.extra_data.len, 6 as usize);
    expect_eq(att.extra_data[0], 0x4E as u8); // 'N'
    expect_eq(att.clock_info.clock, 9876543210 as u64);
    expect_eq(att.clock_info.reset_count, 42 as u32);
    expect_eq(att.clock_info.restart_count, 7 as u32);
    expect_eq(att.clock_info.safe, true);
    expect_eq(att.firmware_version, 0x2026000100020003 as u64);

    const quote = parseTpmQuoteInfo(att.attested_body) ?? unreachable;
    expect_eq(quote.pcr_select_count, 2 as u32);
    expect_eq(quote.pcr_digest.len, 32 as usize);
    expect_eq(quote.pcr_digest[0], 0xA0 as u8);
    expect_eq(quote.pcr_digest[31], 0xBF as u8);

    let it = quote.pcrSelections();
    const s0 = it.next() ?? unreachable;
    expect_eq(s0.hash_alg, TPM_ALG_SHA256);
    expect_eq(s0.isPcrSelected(0), true);
    expect_eq(s0.isPcrSelected(1), true);
    expect_eq(s0.isPcrSelected(2), false);
    expect_eq(s0.isPcrSelected(7), true);
    expect_eq(s0.isPcrSelected(8), false);
    expect_eq(s0.isPcrSelected(24), false); // out of 3-byte range

    const s1 = it.next() ?? unreachable;
    expect_eq(s1.hash_alg, TPM_ALG_SHA1);
    expect_eq(s1.isPcrSelected(10), true);
    expect_eq(s1.isPcrSelected(9), false);

    expect_eq(it.next() == null, true);
});

test("3. TPMS_ATTEST certify type recognition", () => {
    let buf: [40]u8 = std.mem.zeroes([40]u8);
    putU32Be(buf[0..], 0, TPM_GENERATED_VALUE);
    putU16Be(buf[0..], 4, TPM_ST_ATTEST_CERTIFY);
    // qualifiedSigner len=0, extraData len=0, clock_info(17B, safe=0), fw_ver(8B) -> 6+2+2+17+8 = 35B
    buf[35] = 0x55;
    const att = parseTpmAttest(buf[0..36]) ?? unreachable;
    expect_eq(att.isCertify(), true);
    expect_eq(att.isQuote(), false);
    expect_eq(att.clock_info.safe, false);
    expect_eq(att.attested_body.len, 1 as usize);
    expect_eq(att.attested_body[0], 0x55 as u8);
});

test("4. TPMT_PUBLIC header parsing & object_attributes flags (ECC restricted signing key vs RSA decrypt key)", () => {
    let pub_buf: [64]u8 = std.mem.zeroes([64]u8);
    putU16Be(pub_buf[0..], 0, TPM_ALG_ECC);
    putU16Be(pub_buf[0..], 2, TPM_ALG_SHA256);
    const attrs = ATTR_FIXED_TPM | ATTR_FIXED_PARENT | ATTR_SENSITIVE_DATA_ORIGIN | ATTR_USER_WITH_AUTH | ATTR_RESTRICTED | ATTR_SIGN_ENCRYPT;
    putU32Be(pub_buf[0..], 4, attrs);
    putU16Be(pub_buf[0..], 8, 32); // 32B authPolicy
    pub_buf[10] = 0x11;
    pub_buf[41] = 0x22;
    pub_buf[42] = 0xFE; // start of params_and_unique

    const p = parseTpmPublicHeader(pub_buf[0..50]) ?? unreachable;
    expect_eq(p.alg_type, TPM_ALG_ECC);
    expect_eq(p.name_alg, TPM_ALG_SHA256);
    expect_eq(p.object_attributes, attrs);
    expect_eq(p.isRestricted(), true);
    expect_eq(p.isFixedTpm(), true);
    expect_eq(p.isSigningKey(), true);
    expect_eq(p.auth_policy.len, 32 as usize);
    expect_eq(p.auth_policy[0], 0x11 as u8);
    expect_eq(p.auth_policy[31], 0x22 as u8);
    expect_eq(p.params_and_unique.len, 8 as usize);
    expect_eq(p.params_and_unique[0], 0xFE as u8);

    // Non-restricted RSA decrypt key (no authPolicy)
    let rsa_buf: [16]u8 = std.mem.zeroes([16]u8);
    putU16Be(rsa_buf[0..], 0, TPM_ALG_RSA);
    putU16Be(rsa_buf[0..], 2, TPM_ALG_SHA256);
    putU32Be(rsa_buf[0..], 4, ATTR_DECRYPT | ATTR_USER_WITH_AUTH);
    putU16Be(rsa_buf[0..], 8, 0);
    const p2 = parseTpmPublicHeader(rsa_buf[0..]) ?? unreachable;
    expect_eq(p2.isRestricted(), false);
    expect_eq(p2.isFixedTpm(), false);
    expect_eq(p2.isSigningKey(), false);
    expect_eq(p2.auth_policy.len, 0 as usize);
    expect_eq(p2.params_and_unique.len, 6 as usize);
});

test("5. Malformed TPM header, TPMS_ATTEST, TPMS_QUOTE_INFO & TPMT_PUBLIC rejection", () => {
    // 1. Header: truncated, invalid tag, size < 10, size > data.len
    let hdr: [16]u8 = std.mem.zeroes([16]u8);
    expect_eq(parseTpmHeader(hdr[0..9]) == null, true);

    putU16Be(hdr[0..], 0, 0x8000); // invalid tag
    putU32Be(hdr[0..], 2, 10);
    expect_eq(parseTpmHeader(hdr[0..]) == null, true);
    expect_eq(encodeTpmHeader(0x8000, TPM_CC_STARTUP, 0, hdr[0..]) == null, true);
    expect_eq(encodeTpmHeader(TPM_ST_NO_SESSIONS, TPM_CC_STARTUP, 0, hdr[0..8]) == null, true);

    putU16Be(hdr[0..], 0, TPM_ST_NO_SESSIONS);
    putU32Be(hdr[0..], 2, 9); // size < 10
    expect_eq(parseTpmHeader(hdr[0..]) == null, true);

    putU32Be(hdr[0..], 2, 32); // size > data.len (16)
    expect_eq(parseTpmHeader(hdr[0..]) == null, true);

    // 2. TPMS_ATTEST: bad magic, truncated TPM2B, invalid safe byte (> 1)
    let att: [40]u8 = std.mem.zeroes([40]u8);
    putU32Be(att[0..], 0, 0x11223344);
    expect_eq(isTpmAttest(att[0..]), false);
    expect_eq(parseTpmAttest(att[0..]) == null, true);

    putU32Be(att[0..], 0, TPM_GENERATED_VALUE);
    putU16Be(att[0..], 4, TPM_ST_ATTEST_QUOTE);
    putU16Be(att[0..], 6, 100); // qualifiedSigner exceeds buffer
    expect_eq(parseTpmAttest(att[0..]) == null, true);

    putU16Be(att[0..], 6, 0);
    putU16Be(att[0..], 8, 0);
    att[26] = 2; // invalid TPMS_CLOCK_INFO.safe (> 1)
    expect_eq(parseTpmAttest(att[0..]) == null, true);

    // 3. TPMS_QUOTE_INFO: truncated & huge count guard
    let q: [16]u8 = std.mem.zeroes([16]u8);
    putU32Be(q[0..], 0, 0x00FFFFFF); // count far exceeds buffer
    expect_eq(parseTpmQuoteInfo(q[0..]) == null, true);
    expect_eq(parseTpmQuoteInfo(q[0..2]) == null, true);

    // 4. TPMT_PUBLIC: truncated header or policy_len overflow
    let pub_bad: [12]u8 = std.mem.zeroes([12]u8);
    expect_eq(parseTpmPublicHeader(pub_bad[0..9]) == null, true);
    putU16Be(pub_bad[0..], 8, 32); // policy_len > remaining 2B
    expect_eq(parseTpmPublicHeader(pub_bad[0..]) == null, true);
});
