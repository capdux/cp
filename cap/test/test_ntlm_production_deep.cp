// Source: cp/cap/test/test_ntlm_production_deep.cp (Production deep tests for cap:ntlm)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    NTLM_SIGNATURE,
    NTLM_MSG_NEGOTIATE,
    NTLM_MSG_CHALLENGE,
    NTLM_MSG_AUTHENTICATE,
    NTLMSSP_NEGOTIATE_UNICODE,
    NTLMSSP_NEGOTIATE_OEM,
    NTLMSSP_REQUEST_TARGET,
    NTLMSSP_NEGOTIATE_SIGN,
    NTLMSSP_NEGOTIATE_SEAL,
    NTLMSSP_NEGOTIATE_NTLM,
    NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY,
    NTLMSSP_NEGOTIATE_TARGET_INFO,
    NTLMSSP_NEGOTIATE_VERSION,
    NTLMSSP_NEGOTIATE_128,
    NTLMSSP_NEGOTIATE_56,
    MsvAvEOL,
    MsvAvNbComputerName,
    MsvAvNbDomainName,
    MsvAvDnsComputerName,
    MsvAvDnsDomainName,
    MsvAvDnsTreeName,
    MsvAvFlags,
    MsvAvTimestamp,
    MsvAvSingleHost,
    MsvAvTargetName,
    MsvAvChannelBindings,
    NtlmVersion,
    NtlmAvPair,
    NtlmAvPairIter,
    NtlmNegotiate,
    NtlmChallenge,
    NtlmAuthenticate,
    peekNtlmType,
    isNtlm,
    parseNtlmNegotiate,
    parseNtlmChallenge,
    parseNtlmAuthenticate,
} from "cap:ntlm";

fn putU16Le(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn putU32Le(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn putSigAndType(buf: []u8, msg_type: u32) {
    buf[0] = 'N';
    buf[1] = 'T';
    buf[2] = 'L';
    buf[3] = 'M';
    buf[4] = 'S';
    buf[5] = 'S';
    buf[6] = 'P';
    buf[7] = 0;
    putU32Le(buf, 8, msg_type);
}

fn putSecBuf(buf: []u8, desc_off: usize, len: u16, max_len: u16, data_off: u32) {
    putU16Le(buf, desc_off, len);
    putU16Le(buf, desc_off + 2, max_len);
    putU32Le(buf, desc_off + 4, data_off);
}

fn copyInto(buf: []u8, off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) {
        buf[off + i] = src[i];
        i += 1;
    }
}

test("1. NTLM constants, signature peek & Type 1 Negotiate (with and without Version)", () => {
    expect_eq(NTLM_SIGNATURE.len, 8 as usize);
    expect_eq(NTLMSSP_NEGOTIATE_OEM, 0x00000002 as u32);
    expect_eq(NTLMSSP_REQUEST_TARGET, 0x00000004 as u32);
    expect_eq(NTLMSSP_NEGOTIATE_SIGN, 0x00000010 as u32);
    expect_eq(NTLMSSP_NEGOTIATE_SEAL, 0x00000020 as u32);
    expect_eq(NTLMSSP_NEGOTIATE_128, 0x20000000 as u32);
    expect_eq(NTLMSSP_NEGOTIATE_56, 0x80000000 as u32);
    expect_eq(MsvAvDnsTreeName, 5 as u16);
    expect_eq(MsvAvFlags, 6 as u16);
    expect_eq(MsvAvTimestamp, 7 as u16);
    expect_eq(MsvAvSingleHost, 8 as u16);
    expect_eq(MsvAvTargetName, 9 as u16);
    expect_eq(MsvAvChannelBindings, 10 as u16);

    // Type 1 with Version + Domain + Workstation
    let buf: [64]u8 = std.mem.zeroes([64]u8);
    putSigAndType(buf[0..], NTLM_MSG_NEGOTIATE);
    const flags = NTLMSSP_NEGOTIATE_UNICODE | NTLMSSP_NEGOTIATE_NTLM | NTLMSSP_NEGOTIATE_VERSION;
    putU32Le(buf[0..], 12, flags);
    putSecBuf(buf[0..], 16, 4, 4, 40); // domain = "CORP"
    putSecBuf(buf[0..], 24, 5, 5, 44); // workstation = "WKSTN"
    // Version: Windows 10.0 Build 19045, NTLM rev 15
    buf[32] = 10;
    buf[33] = 0;
    putU16Le(buf[0..], 34, 19045);
    buf[39] = 15;
    copyInto(buf[0..], 40, "CORP");
    copyInto(buf[0..], 44, "WKSTN");

    expect_eq(isNtlm(buf[0..49]), true);
    expect_eq(peekNtlmType(buf[0..49]) ?? 0, NTLM_MSG_NEGOTIATE);

    const neg = parseNtlmNegotiate(buf[0..49]) ?? unreachable;
    expect_eq(neg.flags, flags);
    expect_eq(std.mem.eql(u8, neg.domain, "CORP"), true);
    expect_eq(std.mem.eql(u8, neg.workstation, "WKSTN"), true);
    const ver = neg.version ?? unreachable;
    expect_eq(ver.major, 10 as u8);
    expect_eq(ver.minor, 0 as u8);
    expect_eq(ver.build, 19045 as u16);
    expect_eq(ver.ntlm_revision, 15 as u8);

    // Minimal 16-byte Type 1 without Version
    let min_buf: [16]u8 = std.mem.zeroes([16]u8);
    putSigAndType(min_buf[0..], NTLM_MSG_NEGOTIATE);
    putU32Le(min_buf[0..], 12, NTLMSSP_NEGOTIATE_NTLM);
    const neg_min = parseNtlmNegotiate(min_buf[0..]) ?? unreachable;
    expect_eq(neg_min.domain.len, 0 as usize);
    expect_eq(neg_min.workstation.len, 0 as usize);
    expect_eq(neg_min.version == null, true);
});

test("2. Type 2 Challenge + 8B Server Nonce + AV_PAIR TLV iteration & EOL stop", () => {
    let buf: [128]u8 = std.mem.zeroes([128]u8);
    putSigAndType(buf[0..], NTLM_MSG_CHALLENGE);
    const flags = NTLMSSP_NEGOTIATE_UNICODE
        | NTLMSSP_NEGOTIATE_NTLM
        | NTLMSSP_NEGOTIATE_EXTENDED_SESSIONSECURITY
        | NTLMSSP_NEGOTIATE_TARGET_INFO
        | NTLMSSP_NEGOTIATE_VERSION;
    putSecBuf(buf[0..], 12, 4, 4, 56); // target_name = "CORP" at 56
    putU32Le(buf[0..], 20, flags);
    // 8-byte ServerChallenge
    let i: usize = 0;
    while (i < 8) {
        buf[24 + i] = (0xA0 + i) as u8;
        i += 1;
    }
    // Version at [48..56]
    buf[48] = 6;
    buf[49] = 3;
    putU16Le(buf[0..], 50, 9600);
    buf[55] = 15;
    copyInto(buf[0..], 56, "CORP");

    // Build TargetInfo AV_PAIRs at offset 60:
    // Pair 1: MsvAvNbDomainName(2) = "CORP" (4B) -> 8B
    // Pair 2: MsvAvDnsComputerName(3) = "dc01.corp" (9B) -> 13B
    // Pair 3: MsvAvEOL(0), len=0 -> 4B
    // Trailing ignored bytes after EOL: 4B
    let p: usize = 60;
    putU16Le(buf[0..], p, MsvAvNbDomainName);
    putU16Le(buf[0..], p + 2, 4);
    copyInto(buf[0..], p + 4, "CORP");
    p += 8;

    putU16Le(buf[0..], p, MsvAvDnsComputerName);
    putU16Le(buf[0..], p + 2, 9);
    copyInto(buf[0..], p + 4, "dc01.corp");
    p += 13;

    putU16Le(buf[0..], p, MsvAvEOL);
    putU16Le(buf[0..], p + 2, 0);
    p += 4;

    // Extra bytes after EOL must never be yielded
    putU16Le(buf[0..], p, MsvAvDnsDomainName);
    putU16Le(buf[0..], p + 2, 0);
    p += 4;

    const ti_len = (p - 60) as u16;
    putSecBuf(buf[0..], 40, ti_len, ti_len, 60);

    const ch = parseNtlmChallenge(buf[0..p]) ?? unreachable;
    expect_eq(ch.server_challenge[0], 0xA0 as u8);
    expect_eq(ch.server_challenge[7], 0xA7 as u8);
    expect_eq(std.mem.eql(u8, ch.target_name, "CORP"), true);
    const ver = ch.version ?? unreachable;
    expect_eq(ver.major, 6 as u8);
    expect_eq(ver.minor, 3 as u8);
    expect_eq(ver.build, 9600 as u16);

    let it = ch.avPairs();
    const a1 = it.next() ?? unreachable;
    expect_eq(a1.av_id, MsvAvNbDomainName);
    expect_eq(std.mem.eql(u8, a1.value, "CORP"), true);

    const a2 = it.next() ?? unreachable;
    expect_eq(a2.av_id, MsvAvDnsComputerName);
    expect_eq(std.mem.eql(u8, a2.value, "dc01.corp"), true);

    expect_eq(it.next() == null, true);
    expect_eq(it.sawEol(), true);
    expect_eq(it.isTruncated(), false);

    // Lookup helper
    expect_eq(std.mem.eql(u8, ch.findAvPair(MsvAvDnsComputerName) ?? unreachable, "dc01.corp"), true);
    expect_eq(ch.findAvPair(MsvAvNbComputerName) == null, true);
    expect_eq(ch.findAvPair(MsvAvDnsDomainName) == null, true);
});

test("3. Type 3 Authenticate with all 6 Security Buffers & NTLMv1 vs NTLMv2 detection", () => {
    let buf: [256]u8 = std.mem.zeroes([256]u8);
    putSigAndType(buf[0..], NTLM_MSG_AUTHENTICATE);

    let pos: usize = 72;
    // 1. lm_response (24 bytes)
    putSecBuf(buf[0..], 12, 24, 24, pos as u32);
    buf[pos] = 0x11;
    pos += 24;

    // 2. nt_response (48 bytes -> NTLMv2)
    putSecBuf(buf[0..], 20, 48, 48, pos as u32);
    buf[pos] = 0x22;
    pos += 48;

    // 3. domain ("CORP")
    putSecBuf(buf[0..], 28, 4, 4, pos as u32);
    copyInto(buf[0..], pos, "CORP");
    pos += 4;

    // 4. user ("alice")
    putSecBuf(buf[0..], 36, 5, 5, pos as u32);
    copyInto(buf[0..], pos, "alice");
    pos += 5;

    // 5. workstation ("DEVBOX")
    putSecBuf(buf[0..], 44, 6, 6, pos as u32);
    copyInto(buf[0..], pos, "DEVBOX");
    pos += 6;

    // 6. session_key (16 bytes)
    putSecBuf(buf[0..], 52, 16, 16, pos as u32);
    buf[pos] = 0xEE;
    pos += 16;

    const flags = NTLMSSP_NEGOTIATE_UNICODE | NTLMSSP_NEGOTIATE_NTLM | NTLMSSP_NEGOTIATE_VERSION;
    putU32Le(buf[0..], 60, flags);
    buf[64] = 10;
    buf[65] = 0;
    putU16Le(buf[0..], 66, 22621);
    buf[71] = 15;

    const auth = parseNtlmAuthenticate(buf[0..pos]) ?? unreachable;
    expect_eq(auth.lm_response.len, 24 as usize);
    expect_eq(auth.lm_response[0], 0x11 as u8);
    expect_eq(auth.nt_response.len, 48 as usize);
    expect_eq(auth.nt_response[0], 0x22 as u8);
    expect_eq(std.mem.eql(u8, auth.domain, "CORP"), true);
    expect_eq(std.mem.eql(u8, auth.user, "alice"), true);
    expect_eq(std.mem.eql(u8, auth.workstation, "DEVBOX"), true);
    expect_eq(auth.session_key.len, 16 as usize);
    expect_eq(auth.session_key[0], 0xEE as u8);
    expect_eq(auth.isNtlmV2(), true);
    expect_eq((auth.version ?? unreachable).build, 22621 as u16);

    // Shrink nt_response to 24 bytes -> NTLMv1
    putSecBuf(buf[0..], 20, 24, 24, 96);
    const auth_v1 = parseNtlmAuthenticate(buf[0..pos]) ?? unreachable;
    expect_eq(auth_v1.isNtlmV2(), false);
});

test("4. NtlmAvPairIter truncation detection (short header & short payload)", () => {
    // Truncated 2-byte header
    const short_hdr: [2]u8 = [0x01, 0x00];
    let it1 = NtlmAvPairIter.init(short_hdr[0..]);
    expect_eq(it1.next() == null, true);
    expect_eq(it1.isTruncated(), true);
    expect_eq(it1.sawEol(), false);

    // Claims 10 bytes of value, only 3 bytes available
    const short_val: [7]u8 = [0x01, 0x00, 0x0A, 0x00, 'A', 'B', 'C'];
    let it2 = NtlmAvPairIter.init(short_val[0..]);
    expect_eq(it2.next() == null, true);
    expect_eq(it2.isTruncated(), true);
});

test("5. Malformed signatures, type mismatches, out-of-bounds Security Buffers & missing Version rejection", () => {
    // Bad signature
    expect_eq(isNtlm("NOTNTLM\x00\x01\x00\x00\x00"), false);
    expect_eq(peekNtlmType("NTLMSSP") == null, true);

    // Unknown message type 99
    let bad_type: [16]u8 = std.mem.zeroes([16]u8);
    putSigAndType(bad_type[0..], 99);
    expect_eq(peekNtlmType(bad_type[0..]) == null, true);
    expect_eq(isNtlm(bad_type[0..]), false);

    // Type mismatch: Type 1 passed to Challenge/Authenticate
    let t1: [32]u8 = std.mem.zeroes([32]u8);
    putSigAndType(t1[0..], NTLM_MSG_NEGOTIATE);
    expect_eq(parseNtlmChallenge(t1[0..]) == null, true);
    expect_eq(parseNtlmAuthenticate(t1[0..]) == null, true);

    // Type 1 with VERSION flag set but buffer only 32 bytes (< 40)
    putU32Le(t1[0..], 12, NTLMSSP_NEGOTIATE_VERSION);
    expect_eq(parseNtlmNegotiate(t1[0..]) == null, true);

    // Type 1 with out-of-bounds Security Buffer offset
    putU32Le(t1[0..], 12, NTLMSSP_NEGOTIATE_NTLM);
    putSecBuf(t1[0..], 16, 8, 8, 100);
    expect_eq(parseNtlmNegotiate(t1[0..]) == null, true);

    // Type 1 with len > max_len
    putSecBuf(t1[0..], 16, 10, 4, 20);
    expect_eq(parseNtlmNegotiate(t1[0..]) == null, true);

    // Type 2 Challenge with TARGET_INFO flag but truncated to 40 bytes
    let t2: [40]u8 = std.mem.zeroes([40]u8);
    putSigAndType(t2[0..], NTLM_MSG_CHALLENGE);
    putU32Le(t2[0..], 20, NTLMSSP_NEGOTIATE_TARGET_INFO);
    expect_eq(parseNtlmChallenge(t2[0..]) == null, true);

    // Type 3 Authenticate with out-of-bounds user buffer
    let t3: [64]u8 = std.mem.zeroes([64]u8);
    putSigAndType(t3[0..], NTLM_MSG_AUTHENTICATE);
    putSecBuf(t3[0..], 36, 4, 4, 62); // 62 + 4 = 66 > 64
    expect_eq(parseNtlmAuthenticate(t3[0..]) == null, true);
});
