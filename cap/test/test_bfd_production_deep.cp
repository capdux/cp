// Source: cp/cap/test/test_bfd_production_deep.cp
// Deep Production Test Suite for cap:bfd
// Covers:
//   1. Unauthenticated 24B BFD control packet + state/flags/diag decoding & detectionTimeUs
//   2. Simple Password authentication section (type 1, key_id, password slice)
//   3. Keyed MD5 (type 2) & Meticulous Keyed MD5 (type 3) authentication sections
//   4. Keyed SHA-1 (type 4) & Meticulous Keyed SHA-1 (type 5) authentication sections
//   5. RFC 5880 strict header guards (version!=1, detect_mult==0, multipoint==1, my_disc==0, bad len)
//   6. Malformed & truncated Auth section guards (Simple < 3, MD5 < 24, SHA1 < 28, mismatch len, unknown type)

import { test, expect_eq } from "cap:test";
import {
    BFD_UDP_PORT_SINGLE_HOP,
    BFD_UDP_PORT_ECHO,
    BFD_UDP_PORT_MULTI_HOP,
    BFD_VERSION,
    BFD_MIN_LEN,
    BfdDiag,
    BfdState,
    BfdAuthType,
    BfdAuth,
    BfdPacket,
    parseBfd,
    isBfd,
    detectionTimeUs,
} from "cap:iot";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("bfd: unauthenticated 24B control packet + flags/diag/state + detectionTimeUs") {
    expect_eq(BFD_UDP_PORT_SINGLE_HOP, 3784);
    expect_eq(BFD_UDP_PORT_ECHO, 3785);
    expect_eq(BFD_UDP_PORT_MULTI_HOP, 4784);
    expect_eq(BFD_VERSION, 1);
    expect_eq(BFD_MIN_LEN, 24);

    // Vers=1 (0x20) | Diag=3 (neighbor_signaled_session_down) = 0x23
    // Sta=3 (Up=0xC0) | P=1 (0x20) | F=0 | C=1 (0x08) | A=0 | D=1 (0x02) | M=0 = 0xEA
    // Detect Mult=3, Length=24
    // My Disc=0x11223344, Your Disc=0x55667788
    // Desired Min TX=50000 (0x0000C350), Required Min RX=100000 (0x000186A0), Echo RX=0
    let pkt: [24]u8 = [
        0x23, 0xEA, 0x03, 0x18,
        0x11, 0x22, 0x33, 0x44,
        0x55, 0x66, 0x77, 0x88,
        0x00, 0x00, 0xC3, 0x50,
        0x00, 0x01, 0x86, 0xA0,
        0x00, 0x00, 0x00, 0x00,
    ];

    expect_eq(isBfd(pkt[0..24]), true);
    if (b := parseBfd(pkt[0..24])) {
        expect_eq(b.version, 1);
        expect_eq(b.diag == BfdDiag.neighbor_signaled_session_down, true);
        expect_eq(b.state == BfdState.up, true);
        expect_eq(b.poll, true);
        expect_eq(b.final_flag, false);
        expect_eq(b.control_plane_independent, true);
        expect_eq(b.auth_present, false);
        expect_eq(b.demand, true);
        expect_eq(b.multipoint, false);
        expect_eq(b.detect_mult, 3);
        expect_eq(b.length, 24);
        expect_eq(b.my_discriminator, 0x11223344);
        expect_eq(b.your_discriminator, 0x55667788);
        expect_eq(b.desired_min_tx_us, 50000);
        expect_eq(b.required_min_rx_us, 100000);
        expect_eq(b.required_min_echo_rx_us, 0);
        expect_eq(b.auth == null, true);

        // Detection Time = 3 * max(50000, 100000) = 300000 us
        expect_eq(detectionTimeUs(b.desired_min_tx_us, b.required_min_rx_us, b.detect_mult), 300000);
        expect_eq(detectionTimeUs(200000, 50000, 5), 1000000);
    } else {
        expect_eq(true, false);
    }
}

test("bfd: Simple Password authentication section (type 1)") {
    // 24B header + 9B Simple Password auth (Type=1, Len=9, KeyID=7, "secret") = 33B
    let pkt: [33]u8 = [
        0x20, 0x94, 0x05, 0x21, // Vers=1, Diag=0; Sta=2 (Init=0x80) | F=1 (0x10) | A=1 (0x04) = 0x94; Mult=5; Len=33
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x02,
        0x00, 0x0F, 0x42, 0x40, // 1_000_000 us
        0x00, 0x0F, 0x42, 0x40,
        0x00, 0x00, 0x00, 0x00,
        // Simple Password Auth Section (9 bytes)
        0x01, 0x09, 0x07,
        0x73, 0x65, 0x63, 0x72, 0x65, 0x74, // "secret"
    ];

    if (b := parseBfd(pkt[0..33])) {
        expect_eq(b.state == BfdState.init, true);
        expect_eq(b.final_flag, true);
        expect_eq(b.auth_present, true);
        if (auth := b.auth) {
            expect_eq(auth.auth_type == BfdAuthType.simple_password, true);
            expect_eq(auth.auth_len, 9);
            expect_eq(auth.auth_key_id, 7);
            expect_eq(auth.sequence_number, 0);
            expect_eq(strEq(auth.data, "secret"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("bfd: Keyed MD5 (type 2) & Meticulous Keyed MD5 (type 3)") {
    // 24B header + 24B Keyed MD5 auth = 48B
    let pkt: [48]u8 = [0; 48];
    pkt[0] = 0x21; // Vers=1, Diag=1 (control_detection_time_expired)
    pkt[1] = 0x44; // Sta=1 (Down=0x40) | A=1 (0x04)
    pkt[2] = 3;
    pkt[3] = 48;
    pkt[7] = 1;    // My Disc = 1
    // Auth Section at 24..48
    pkt[24] = 2;   // Keyed MD5
    pkt[25] = 24;
    pkt[26] = 11;  // Key ID
    pkt[28] = 0x00; pkt[29] = 0x00; pkt[30] = 0x01; pkt[31] = 0x02; // Seq = 258
    pkt[32] = 0xAA;
    pkt[47] = 0xBB;

    if (b := parseBfd(pkt[0..48])) {
        expect_eq(b.diag == BfdDiag.control_detection_time_expired, true);
        expect_eq(b.state == BfdState.down, true);
        if (auth := b.auth) {
            expect_eq(auth.auth_type == BfdAuthType.keyed_md5, true);
            expect_eq(auth.auth_len, 24);
            expect_eq(auth.auth_key_id, 11);
            expect_eq(auth.sequence_number, 258);
            expect_eq(auth.data.len, 16);
            expect_eq(auth.data[0], 0xAA);
            expect_eq(auth.data[15], 0xBB);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Switch to Meticulous Keyed MD5 (type 3)
    pkt[24] = 3;
    if (b := parseBfd(pkt[0..48])) {
        if (auth := b.auth) {
            expect_eq(auth.auth_type == BfdAuthType.meticulous_keyed_md5, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("bfd: Keyed SHA-1 (type 4) & Meticulous Keyed SHA-1 (type 5)") {
    // 24B header + 28B Keyed SHA-1 auth = 52B
    let pkt: [52]u8 = [0; 52];
    pkt[0] = 0x27; // Vers=1, Diag=7 (administratively_down)
    pkt[1] = 0x04; // Sta=0 (AdminDown) | A=1 (0x04)
    pkt[2] = 4;
    pkt[3] = 52;
    pkt[4] = 0xDE; pkt[5] = 0xAD; pkt[6] = 0xBE; pkt[7] = 0xEF;
    // Auth Section at 24..52
    pkt[24] = 4;   // Keyed SHA-1
    pkt[25] = 28;
    pkt[26] = 22;
    pkt[28] = 0x12; pkt[29] = 0x34; pkt[30] = 0x56; pkt[31] = 0x78;
    pkt[32] = 0x11;
    pkt[51] = 0x99;

    if (b := parseBfd(pkt[0..52])) {
        expect_eq(b.diag == BfdDiag.administratively_down, true);
        expect_eq(b.state == BfdState.admin_down, true);
        if (auth := b.auth) {
            expect_eq(auth.auth_type == BfdAuthType.keyed_sha1, true);
            expect_eq(auth.auth_len, 28);
            expect_eq(auth.auth_key_id, 22);
            expect_eq(auth.sequence_number, 0x12345678);
            expect_eq(auth.data.len, 20);
            expect_eq(auth.data[0], 0x11);
            expect_eq(auth.data[19], 0x99);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Switch to Meticulous Keyed SHA-1 (type 5)
    pkt[24] = 5;
    if (b := parseBfd(pkt[0..52])) {
        if (auth := b.auth) {
            expect_eq(auth.auth_type == BfdAuthType.meticulous_keyed_sha1, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("bfd: strict RFC 5880 header guards (version, mult, multipoint, disc, len)") {
    let base: [24]u8 = [0; 24];
    base[0] = 0x20; // Vers=1
    base[1] = 0xC0; // Sta=Up
    base[2] = 3;    // Mult=3
    base[3] = 24;   // Len=24
    base[7] = 1;    // My Disc=1
    expect_eq(isBfd(base[0..24]), true);

    // 1. Truncated < 24B
    expect_eq(parseBfd(base[0..23]) == null, true);

    // 2. Wrong version (version = 2 -> 0x40)
    let bad_ver = base;
    bad_ver[0] = 0x40;
    expect_eq(parseBfd(bad_ver[0..24]) == null, true);

    // 3. detect_mult == 0
    let bad_mult = base;
    bad_mult[2] = 0;
    expect_eq(parseBfd(bad_mult[0..24]) == null, true);

    // 4. multipoint bit set (M=1)
    let bad_mp = base;
    bad_mp[1] = 0xC1;
    expect_eq(parseBfd(bad_mp[0..24]) == null, true);

    // 5. my_discriminator == 0
    let bad_disc = base;
    bad_disc[7] = 0;
    expect_eq(parseBfd(bad_disc[0..24]) == null, true);

    // 6. length < 24 or > data.len
    let bad_len_small = base;
    bad_len_small[3] = 20;
    expect_eq(parseBfd(bad_len_small[0..24]) == null, true);

    let bad_len_big = base;
    bad_len_big[3] = 32;
    expect_eq(parseBfd(bad_len_big[0..24]) == null, true);

    // 7. A=0 but length=28 (> 24)
    let bad_extra: [28]u8 = [0; 28];
    bad_extra[0] = 0x20;
    bad_extra[1] = 0xC0;
    bad_extra[2] = 3;
    bad_extra[3] = 28;
    bad_extra[7] = 1;
    expect_eq(parseBfd(bad_extra[0..28]) == null, true);
}

test("bfd: malformed & truncated Auth section guards") {
    // 1. Keyed MD5 with auth_len = 20 (< 24) -> rejected
    let bad_md5: [44]u8 = [0; 44];
    bad_md5[0] = 0x20; bad_md5[1] = 0xC4; bad_md5[2] = 3; bad_md5[3] = 44; bad_md5[7] = 1;
    bad_md5[24] = 2; bad_md5[25] = 20;
    expect_eq(parseBfd(bad_md5[0..44]) == null, true);

    // 2. Keyed SHA-1 with auth_len = 24 (< 28) -> rejected
    let bad_sha1: [48]u8 = [0; 48];
    bad_sha1[0] = 0x20; bad_sha1[1] = 0xC4; bad_sha1[2] = 3; bad_sha1[3] = 48; bad_sha1[7] = 1;
    bad_sha1[24] = 4; bad_sha1[25] = 24;
    expect_eq(parseBfd(bad_sha1[0..48]) == null, true);

    // 3. Unknown auth_type = 9 -> rejected
    let bad_type: [32]u8 = [0; 32];
    bad_type[0] = 0x20; bad_type[1] = 0xC4; bad_type[2] = 3; bad_type[3] = 32; bad_type[7] = 1;
    bad_type[24] = 9; bad_type[25] = 8;
    expect_eq(parseBfd(bad_type[0..32]) == null, true);

    // 4. auth_len mismatch with packet length (pkt len=32 -> 8B auth, auth_len claims 6) -> rejected
    let bad_mismatch: [32]u8 = [0; 32];
    bad_mismatch[0] = 0x20; bad_mismatch[1] = 0xC4; bad_mismatch[2] = 3; bad_mismatch[3] = 32; bad_mismatch[7] = 1;
    bad_mismatch[24] = 1; bad_mismatch[25] = 6;
    expect_eq(parseBfd(bad_mismatch[0..32]) == null, true);
}
