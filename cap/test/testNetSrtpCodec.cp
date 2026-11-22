// Origin: Pure-CP Stdlib Test Suite (lib/net/srtpCodec.cp) | Created: 2026-11-23
// Production Deep Verification Suite for RFC 3711 SRTP/SRTCP & RFC 5764 DTLS-SRTP:
//   1. RFC 3711 Appendix B.3 official KDF test vector byte-for-byte verification (enc_key, auth_key, salt_key)
//   2. SRTP protect/unprotect roundtrip across all 3 profiles (incl. CSRC + RTP header extension)
//   3. Single-byte tamper in header/ciphertext/tag -> `AuthTagMismatch` without polluting replay state
//   4. 64-bit anti-replay sliding window (`ReplayDetected`, `PacketTooOld` & out-of-order acceptance)
//   5. 16-bit sequence rollover `65534 -> 65535 -> 0 -> 1` with automatic ROC increment & late pre-wrap recovery
//   6. RFC 3711 §3.4 SRTCP protect/unprotect roundtrip, E-flag verification & independent replay protection
//   7. RFC 5764 §4.2 DTLS-SRTP EKM split, boundary guards (`PacketTooShort`, `BufferTooSmall`, `InvalidKeyLen`) & `cap.net` parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    SRTP_MASTER_KEY_LEN,
    SRTP_MASTER_SALT_LEN,
    SRTP_AUTH_KEY_LEN,
    SRTP_HMAC_SHA1_80_TAG_LEN,
    SRTP_HMAC_SHA1_32_TAG_LEN,
    SRTP_REPLAY_WINDOW_SIZE,
    SRTP_DTLS_EKM_LEN,
    SrtpError,
    SrtpProfile,
    SrtpKeyMaterial,
    splitDtlsExportedKeyingMaterial,
    deriveSrtpSessionKeys,
    estimateSrtpPacketIndex,
    SrtpReplayWindow,
    SrtpContext,
} from "cap:net";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn makeTestMaster(): SrtpKeyMaterial {
    return SrtpKeyMaterial{
        master_key: [
            0xE1, 0xF9, 0x7A, 0x0D, 0x3E, 0x01, 0x8B, 0xE0,
            0xD6, 0x4F, 0xA3, 0x2C, 0x06, 0xDE, 0x41, 0x39,
        ],
        master_salt: [
            0x0E, 0xC6, 0x75, 0xAD, 0x49, 0x8A, 0xFE, 0xEB,
            0xB6, 0x96, 0x0B, 0x3A, 0xAB, 0xE6,
        ],
    };
}

fn buildRtpPacket(seq: u16, ssrc: u32, with_csrc_ext: bool, payload: []const u8, out: []u8): usize {
    // Byte 0: V=2 (0x80), optional X=1 (0x10) + CC=2 (0x02)
    out[0] = if (with_csrc_ext) 0x92 else 0x80;
    out[1] = 0x6F; // PT=111 (Opus)
    out[2] = ((seq >> 8) & 0xFF) as u8;
    out[3] = (seq & 0xFF) as u8;
    // Timestamp = 960 * seq
    const ts: u32 = (@as(u32, seq)) * 960;
    out[4] = ((ts >> 24) & 0xFF) as u8;
    out[5] = ((ts >> 16) & 0xFF) as u8;
    out[6] = ((ts >> 8) & 0xFF) as u8;
    out[7] = (ts & 0xFF) as u8;
    // SSRC
    out[8] = ((ssrc >> 24) & 0xFF) as u8;
    out[9] = ((ssrc >> 16) & 0xFF) as u8;
    out[10] = ((ssrc >> 8) & 0xFF) as u8;
    out[11] = (ssrc & 0xFF) as u8;

    let pos: usize = 12;
    if (with_csrc_ext) {
        // 2 CSRC entries (8 bytes)
        let c: usize = 0;
        while (c < 8) : (c += 1) {
            out[pos + c] = (0xA0 + c) as u8;
        }
        pos += 8;
        // 1-word RTP header extension (4B hdr + 4B data)
        out[pos + 0] = 0xBE;
        out[pos + 1] = 0xDE;
        out[pos + 2] = 0x00;
        out[pos + 3] = 0x01; // length = 1 word (4B)
        out[pos + 4] = 0x10;
        out[pos + 5] = 0x20;
        out[pos + 6] = 0x30;
        out[pos + 7] = 0x40;
        pos += 8;
    }

    let i: usize = 0;
    while (i < payload.len) : (i += 1) {
        out[pos + i] = payload[i];
    }
    return pos + payload.len;
}

test("SRTP RFC 3711 Appendix B.3: official KDF test vector byte-for-byte verification") {
    const master = makeTestMaster();
    const keys = deriveSrtpSessionKeys(master);

    // Official RFC 3711 Appendix B.3 expected outputs
    const exp_enc: [16]u8 = [
        0xC6, 0x1E, 0x7A, 0x93, 0x74, 0x4F, 0x39, 0xEE,
        0x10, 0x73, 0x4A, 0xFE, 0x3F, 0xF7, 0xA0, 0x87,
    ];
    const exp_salt: [14]u8 = [
        0x30, 0xCB, 0xBC, 0x08, 0x86, 0x3D, 0x8C, 0x85,
        0xD4, 0x9D, 0xB3, 0x4A, 0x9A, 0xE1,
    ];
    const exp_auth: [20]u8 = [
        0xCE, 0xBE, 0x32, 0x1F, 0x6F, 0xF7, 0x71, 0x6B,
        0x6F, 0xD4, 0xAB, 0x49, 0xAF, 0x25, 0x6A, 0x15,
        0x6D, 0x38, 0xBA, 0xA4,
    ];

    expect_true(bytesEq(keys.enc_key[0..], exp_enc[0..]));
    expect_true(bytesEq(keys.salt_key[0..], exp_salt[0..]));
    expect_true(bytesEq(keys.auth_key[0..], exp_auth[0..]));
    // SRTCP keys must be distinct from SRTP keys (different labels 0x03..0x05)
    expect_true(!bytesEq(keys.srtcp_enc_key[0..], keys.enc_key[0..]));
    expect_true(!bytesEq(keys.srtcp_auth_key[0..], keys.auth_key[0..]));
    expect_true(!bytesEq(keys.srtcp_salt_key[0..], keys.salt_key[0..]));
}

test("SRTP protect/unprotect roundtrip across all 3 profiles (incl. CSRC + RTP extension)") {
    const master = makeTestMaster();
    const payload = "Hello WebRTC SRTP encrypted Opus audio frame!";

    // 1. AES128_CM_HMAC_SHA1_80 with CSRC + extension header (hdr_len = 28)
    let tx80 = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx80 = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_80);

    let rtp_buf: [128]u8 = [0; 128];
    const rtp_len = buildRtpPacket(42, 0xCAFEBABE, true, payload, rtp_buf[0..]);

    let srtp_buf: [160]u8 = [0; 160];
    const srtp_len = tx80.protectRtp(rtp_buf[0..rtp_len], srtp_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(srtp_len, rtp_len + SRTP_HMAC_SHA1_80_TAG_LEN);
    // 28-byte header stays cleartext, payload is encrypted
    expect_true(bytesEq(srtp_buf[0..28], rtp_buf[0..28]));
    expect_true(!bytesEq(srtp_buf[28..rtp_len], rtp_buf[28..rtp_len]));

    let dec_buf: [128]u8 = [0; 128];
    const dec_len = rx80.unprotectRtp(srtp_buf[0..srtp_len], dec_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(dec_len, rtp_len);
    expect_true(bytesEq(dec_buf[0..dec_len], rtp_buf[0..rtp_len]));

    // 2. AES128_CM_HMAC_SHA1_32 (4-byte auth tag)
    let tx32 = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_32);
    let rx32 = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_32);
    const srtp32_len = tx32.protectRtp(rtp_buf[0..rtp_len], srtp_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(srtp32_len, rtp_len + SRTP_HMAC_SHA1_32_TAG_LEN);
    const dec32_len = rx32.unprotectRtp(srtp_buf[0..srtp32_len], dec_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(bytesEq(dec_buf[0..dec32_len], rtp_buf[0..rtp_len]));

    // 3. NULL_CIPHER_HMAC_SHA1_80 (cleartext payload + 10-byte auth tag)
    let tx_null = SrtpContext.init(master, SrtpProfile.null_cipher_hmac_sha1_80);
    let rx_null = SrtpContext.init(master, SrtpProfile.null_cipher_hmac_sha1_80);
    const srtp_null_len = tx_null.protectRtp(rtp_buf[0..rtp_len], srtp_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(bytesEq(srtp_buf[0..rtp_len], rtp_buf[0..rtp_len]));
    const dec_null_len = rx_null.unprotectRtp(srtp_buf[0..srtp_null_len], dec_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(bytesEq(dec_buf[0..dec_null_len], rtp_buf[0..rtp_len]));
}

test("SRTP tamper detection: single-byte flip in header/ciphertext/tag triggers AuthTagMismatch") {
    const master = makeTestMaster();
    let tx = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_80);

    let rtp_buf: [64]u8 = [0; 64];
    const rtp_len = buildRtpPacket(100, 0x11223344, false, "SRTP-Payload-Data", rtp_buf[0..]);
    let srtp_buf: [80]u8 = [0; 80];
    const srtp_len = tx.protectRtp(rtp_buf[0..rtp_len], srtp_buf[0..]) catch { expect_true(false); return; };

    let dec_buf: [64]u8 = [0; 64];

    // 1. Flip 1 bit in RTP header
    srtp_buf[5] ^= 0x01;
    let err1: bool = false;
    * = rx.unprotectRtp(srtp_buf[0..srtp_len], dec_buf[0..]) catch |e| {
        if (e == SrtpError.AuthTagMismatch) err1 = true;
    };
    expect_true(err1);
    srtp_buf[5] ^= 0x01;

    // 2. Flip 1 bit in encrypted payload
    srtp_buf[15] ^= 0x80;
    let err2: bool = false;
    * = rx.unprotectRtp(srtp_buf[0..srtp_len], dec_buf[0..]) catch |e| {
        if (e == SrtpError.AuthTagMismatch) err2 = true;
    };
    expect_true(err2);
    srtp_buf[15] ^= 0x80;

    // 3. Flip 1 bit in HMAC tag
    srtp_buf[srtp_len - 1] ^= 0x04;
    let err3: bool = false;
    * = rx.unprotectRtp(srtp_buf[0..srtp_len], dec_buf[0..]) catch |e| {
        if (e == SrtpError.AuthTagMismatch) err3 = true;
    };
    expect_true(err3);
    srtp_buf[srtp_len - 1] ^= 0x04;

    // 4. Original unmodified packet MUST STILL succeed (failed auth never polluted replay window!)
    const ok_len = rx.unprotectRtp(srtp_buf[0..srtp_len], dec_buf[0..]) catch { expect_true(false); return; };
    expect_eq(ok_len, rtp_len);
}

test("SRTP 64-bit anti-replay window: out-of-order acceptance, ReplayDetected & PacketTooOld") {
    let win = SrtpReplayWindow{};
    win.check(100) catch { expect_true(false); };
    win.accept(100);
    win.check(105) catch { expect_true(false); };
    win.accept(105);

    // Out-of-order packet 102 within 64-packet window succeeds once, then rejected as duplicate
    win.check(102) catch { expect_true(false); };
    win.accept(102);
    let got_dup: bool = false;
    win.check(102) catch |e| {
        if (e == SrtpError.ReplayDetected) got_dup = true;
    };
    expect_true(got_dup);

    // Packet 41 is 105 - 41 = 64 behind highest_index (105) -> PacketTooOld
    let got_old: bool = false;
    win.check(41) catch |e| {
        if (e == SrtpError.PacketTooOld) got_old = true;
    };
    expect_true(got_old);

    // End-to-end duplicate rejection via SrtpContext
    const master = makeTestMaster();
    let tx = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rtp_buf: [64]u8 = [0; 64];
    let srtp_buf: [80]u8 = [0; 80];
    let dec_buf: [64]u8 = [0; 64];

    const rlen = buildRtpPacket(7, 0x55667788, false, "pkt7", rtp_buf[0..]);
    const slen = tx.protectRtp(rtp_buf[0..rlen], srtp_buf[0..]) catch { expect_true(false); return; };
    * = rx.unprotectRtp(srtp_buf[0..slen], dec_buf[0..]) catch { expect_true(false); return; };

    let got_replay: bool = false;
    * = rx.unprotectRtp(srtp_buf[0..slen], dec_buf[0..]) catch |e| {
        if (e == SrtpError.ReplayDetected) got_replay = true;
    };
    expect_true(got_replay);
}

test("SRTP 16-bit sequence rollover (65534 -> 65535 -> 0 -> 1): automatic ROC increment & late recovery") {
    // Direct index estimation checks
    expect_eq(estimateSrtpPacketIndex(0, 65535, 0), @as(u64, 65536));
    expect_eq(estimateSrtpPacketIndex(1, 0, 65534), @as(u64, 65534));

    const master = makeTestMaster();
    let tx = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_80);

    let rtp_buf: [64]u8 = [0; 64];
    let srtp_late: [80]u8 = [0; 80];
    let srtp_buf: [80]u8 = [0; 80];
    let dec_buf: [64]u8 = [0; 64];

    // Protect seq=65533 first, hold it back to simulate late arrival after rollover
    const len_late_rtp = buildRtpPacket(65533, 0xAABBCCDD, false, "late-65533", rtp_buf[0..]);
    const len_late_srtp = tx.protectRtp(rtp_buf[0..len_late_rtp], srtp_late[0..]) catch { expect_true(false); return; };

    // Send & receive 65534, 65535, 0, 1 across 16-bit wrap boundary
    const seqs: [4]u16 = [65534, 65535, 0, 1];
    let i: usize = 0;
    while (i < 4) : (i += 1) {
        const rlen = buildRtpPacket(seqs[i], 0xAABBCCDD, false, "rollover-frame", rtp_buf[0..]);
        const slen = tx.protectRtp(rtp_buf[0..rlen], srtp_buf[0..]) catch { expect_true(false); return; };
        const dlen = rx.unprotectRtp(srtp_buf[0..slen], dec_buf[0..]) catch { expect_true(false); return; };
        expect_eq(dlen, rlen);
        expect_true(bytesEq(dec_buf[0..dlen], rtp_buf[0..rlen]));
    }
    expect_eq(tx.tx_roc, @as(u32, 1));
    expect_eq(rx.rx_roc, @as(u32, 1));

    // Late packet seq=65533 arrives after ROC rolled to 1 -> must still estimate ROC=0 and decrypt cleanly!
    const d_late = rx.unprotectRtp(srtp_late[0..len_late_srtp], dec_buf[0..]) catch { expect_true(false); return; };
    expect_eq(d_late, len_late_rtp);
    expect_true(bytesEq(dec_buf[12..d_late], "late-65533"));
}

test("SRTCP RFC 3711 §3.4: protect/unprotect roundtrip, E-flag & independent replay protection") {
    const master = makeTestMaster();
    let tx = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_80);
    let rx = SrtpContext.init(master, SrtpProfile.aes128_cm_hmac_sha1_80);

    // 28-byte RTCP Sender Report (8-byte header + 20-byte sender info)
    let rtcp: [28]u8 = [0; 28];
    rtcp[0] = 0x80;
    rtcp[1] = 200; // PT=200 (SR)
    rtcp[2] = 0x00;
    rtcp[3] = 0x06;
    rtcp[4] = 0x12;
    rtcp[5] = 0x34;
    rtcp[6] = 0x56;
    rtcp[7] = 0x78; // SSRC
    let k: usize = 8;
    while (k < 28) : (k += 1) {
        rtcp[k] = (0x40 + k) as u8;
    }

    let srtcp: [64]u8 = [0; 64];
    const slen = tx.protectRtcp(rtcp[0..], srtcp[0..]) catch { expect_true(false); return; };
    // 28 RTCP + 4 (E|index) + 10 (HMAC-SHA1-80) = 42 bytes
    expect_eq(slen, @as(usize, 42));
    // First 8 bytes cleartext, payload encrypted, E-flag bit set on trailer byte 28
    expect_true(bytesEq(srtcp[0..8], rtcp[0..8]));
    expect_true(!bytesEq(srtcp[8..28], rtcp[8..28]));
    expect_true((srtcp[28] & 0x80) != 0);

    let dec_rtcp: [32]u8 = [0; 32];
    const dlen = rx.unprotectRtcp(srtcp[0..slen], dec_rtcp[0..]) catch { expect_true(false); return; };
    expect_eq(dlen, @as(usize, 28));
    expect_true(bytesEq(dec_rtcp[0..dlen], rtcp[0..]));

    // Replaying same SRTCP packet must fail with ReplayDetected
    let got_rep: bool = false;
    * = rx.unprotectRtcp(srtcp[0..slen], dec_rtcp[0..]) catch |e| {
        if (e == SrtpError.ReplayDetected) got_rep = true;
    };
    expect_true(got_rep);
}

test("RFC 5764 DTLS-SRTP EKM split, boundary guards & cap.net facade parity") {
    let ekm: [60]u8 = [0; 60];
    let i: usize = 0;
    while (i < 60) : (i += 1) {
        ekm[i] = @as(u8, @intCast(i + 1));
    }

    const pair = cap.net.splitDtlsExportedKeyingMaterial(ekm[0..]) catch { expect_true(false); return; };
    expect_eq(pair.client.master_key[0], @as(u8, 1));
    expect_eq(pair.client.master_key[15], @as(u8, 16));
    expect_eq(pair.server.master_key[0], @as(u8, 17));
    expect_eq(pair.client.master_salt[0], @as(u8, 33));
    expect_eq(pair.server.master_salt[0], @as(u8, 47));
    expect_eq(pair.server.master_salt[13], @as(u8, 60));

    // Short EKM -> InvalidKeyLen
    let got_klen: bool = false;
    * = splitDtlsExportedKeyingMaterial(ekm[0..40]) catch |e| {
        if (e == SrtpError.InvalidKeyLen) got_klen = true;
    };
    expect_true(got_klen);

    // PacketTooShort & BufferTooSmall guards
    let ctx = cap.net.SrtpContext.init(pair.client, SrtpProfile.aes128_cm_hmac_sha1_80);
    const short_p: [8]u8 = [0; 8];
    let out: [64]u8 = [0; 64];
    let got_short: bool = false;
    * = ctx.protectRtp(short_p[0..], out[0..]) catch |e| {
        if (e == SrtpError.PacketTooShort) got_short = true;
    };
    expect_true(got_short);

    let rtp_ok: [20]u8 = [0; 20];
    rtp_ok[0] = 0x80;
    let tiny: [16]u8 = [0; 16];
    let got_small: bool = false;
    * = ctx.protectRtp(rtp_ok[0..], tiny[0..]) catch |e| {
        if (e == SrtpError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small);

    // Facade constants parity
    expect_eq(cap.net.SRTP_MASTER_KEY_LEN, SRTP_MASTER_KEY_LEN);
    expect_eq(cap.net.SRTP_MASTER_SALT_LEN, SRTP_MASTER_SALT_LEN);
    expect_eq(cap.net.SRTP_AUTH_KEY_LEN, SRTP_AUTH_KEY_LEN);
    expect_eq(cap.net.SRTP_REPLAY_WINDOW_SIZE, SRTP_REPLAY_WINDOW_SIZE);
    expect_eq(cap.net.SRTP_DTLS_EKM_LEN, SRTP_DTLS_EKM_LEN);
}
