// Source: cp/cap/test/test_diameter_production_deep.cp
// Deep Production Test Suite for cap:diameter
// Covers:
//   1. CER/CEA & CCR/CCA full round-trip encoding & decoding + findAvp(Session-Id/Origin-Host/Result-Code)
//   2. 4-byte alignment padding verification for 1B / 2B / 3B / 5B payloads + zero-padding check
//   3. Vendor-Specific 12B AVP (3GPP Vendor-ID 10415) + asU32 / asI32 (negative & MIN) / asU64
//   4. Header flag helpers (R/P/E/T) & 24-bit Command-Code / Message-Length edge values
//   5. Strict error guards: version!=1, non-4B msg_len, reserved header/AVP flags, truncated header/AVP
//   6. DiameterWriter buffer overflow & invalid argument guards

import { test, expect_eq } from "cap:test";
import {
    DIAMETER_HEADER_SIZE,
    DIAMETER_VERSION,
    DIAMETER_PORT,
    DIAM_FLAG_REQUEST,
    DIAM_FLAG_PROXIABLE,
    DIAM_FLAG_ERROR,
    DIAM_FLAG_RETRANSMIT,
    AVP_FLAG_VENDOR,
    AVP_FLAG_MANDATORY,
    AVP_FLAG_PROTECTED,
    CMD_CER_CEA,
    CMD_ACR_ACA,
    CMD_CCR_CCA,
    CMD_DWR_DWA,
    CMD_DPR_DPA,
    CMD_AIR_AIA,
    AVP_AUTH_APP_ID,
    AVP_SESSION_ID,
    AVP_ORIGIN_HOST,
    AVP_VENDOR_ID,
    AVP_RESULT_CODE,
    AVP_DEST_REALM,
    AVP_DEST_HOST,
    AVP_ORIGIN_REALM,
    DiameterHeader,
    DiameterAvp,
    DiameterMessage,
    DiameterAvpIter,
    DiameterWriter,
    parseDiameterMessage,
    isDiameter,
} from "cap:diameter";

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

test("diameter: CER/CEA & CCR/CCA full round-trip + findAvp") {
    expect_eq(DIAMETER_HEADER_SIZE, 20);
    expect_eq(DIAMETER_VERSION, 1);
    expect_eq(DIAMETER_PORT, 3868);

    // 1. CER Request
    let cer_buf: [256]u8 = [0; 256];
    let w1 = DiameterWriter.init(
        cer_buf[0..256],
        DIAM_FLAG_REQUEST,
        CMD_CER_CEA,
        0,
        0x10203040,
        0x50607080,
    );
    expect_eq(w1.writeAvp(AVP_ORIGIN_HOST, AVP_FLAG_MANDATORY, 0, "mme01.epc.net"), true);
    expect_eq(w1.writeAvp(AVP_ORIGIN_REALM, AVP_FLAG_MANDATORY, 0, "epc.net"), true);
    expect_eq(w1.writeU32Avp(AVP_VENDOR_ID, true, 10415), true);
    expect_eq(w1.writeU32Avp(AVP_AUTH_APP_ID, true, 16777251), true);

    if (pkt := w1.finish()) {
        expect_eq(isDiameter(pkt), true);
        expect_eq(pkt.len % 4, 0);
        if (msg := parseDiameterMessage(pkt)) {
            expect_eq(msg.header.version, 1);
            expect_eq(msg.header.msg_len as usize, pkt.len);
            expect_eq(msg.header.command_code, CMD_CER_CEA);
            expect_eq(msg.header.application_id, 0);
            expect_eq(msg.header.hop_by_hop_id, 0x10203040);
            expect_eq(msg.header.end_to_end_id, 0x50607080);
            expect_eq(msg.isRequest(), true);
            expect_eq(msg.isProxiable(), false);
            expect_eq(msg.isError(), false);
            expect_eq(msg.isRetransmit(), false);

            if (oh := msg.findAvp(AVP_ORIGIN_HOST)) {
                expect_eq(oh.isMandatory(), true);
                expect_eq(oh.isVendor(), false);
                expect_eq(strEq(oh.data, "mme01.epc.net"), true);
            } else {
                expect_eq(true, false);
            }
            if (vid := msg.findAvp(AVP_VENDOR_ID)) {
                expect_eq(vid.asU32() == 10415, true);
            } else {
                expect_eq(true, false);
            }
            expect_eq(msg.findAvp(AVP_RESULT_CODE) == null, true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // 2. CCA Answer (R=0, P=1) with Session-Id, Origin-Host, Result-Code=2001
    let cca_buf: [256]u8 = [0; 256];
    let w2 = DiameterWriter.init(
        cca_buf[0..256],
        DIAM_FLAG_PROXIABLE,
        CMD_CCR_CCA,
        4,
        0xAABBCCDD,
        0x11223344,
    );
    expect_eq(w2.writeAvp(AVP_SESSION_ID, AVP_FLAG_MANDATORY, 0, "ocs.net;17000;99"), true);
    expect_eq(w2.writeAvp(AVP_ORIGIN_HOST, AVP_FLAG_MANDATORY, 0, "ocs01.net"), true);
    expect_eq(w2.writeU32Avp(AVP_RESULT_CODE, true, 2001), true);
    expect_eq(w2.writeAvp(AVP_DEST_HOST, AVP_FLAG_MANDATORY, 0, "pgw01.net"), true);
    expect_eq(w2.writeAvp(AVP_DEST_REALM, AVP_FLAG_MANDATORY, 0, "net"), true);

    if (pkt := w2.finish()) {
        if (msg := parseDiameterMessage(pkt)) {
            expect_eq(msg.isRequest(), false);
            expect_eq(msg.isProxiable(), true);
            expect_eq(msg.header.command_code, CMD_CCR_CCA);
            expect_eq(msg.header.application_id, 4);

            if (sid := msg.findAvp(AVP_SESSION_ID)) {
                expect_eq(strEq(sid.data, "ocs.net;17000;99"), true);
            } else {
                expect_eq(true, false);
            }
            if (rc := msg.findAvp(AVP_RESULT_CODE)) {
                expect_eq(rc.asU32() == 2001, true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("diameter: 4-byte alignment padding for 1B / 2B / 3B / 5B payloads") {
    let buf: [256]u8 = [0xFF; 256]; // pre-fill with 0xFF to verify zero-padding!
    let w = DiameterWriter.init(buf[0..256], DIAM_FLAG_REQUEST, CMD_DWR_DWA, 0, 1, 2);

    expect_eq(w.writeAvp(1001, AVP_FLAG_MANDATORY, 0, "A"), true);     // 8+1=9 -> padded 12 (3 zeros)
    expect_eq(w.writeAvp(1002, AVP_FLAG_MANDATORY, 0, "BC"), true);    // 8+2=10 -> padded 12 (2 zeros)
    expect_eq(w.writeAvp(1003, AVP_FLAG_MANDATORY, 0, "DEF"), true);   // 8+3=11 -> padded 12 (1 zero)
    expect_eq(w.writeAvp(1004, AVP_FLAG_MANDATORY, 0, "GHIJK"), true); // 8+5=13 -> padded 16 (3 zeros)

    if (pkt := w.finish()) {
        // Total = 20 + 12 + 12 + 12 + 16 = 72
        expect_eq(pkt.len, 72);
        // Verify padding bytes were zeroed
        expect_eq(pkt[20 + 9], 0);
        expect_eq(pkt[20 + 10], 0);
        expect_eq(pkt[20 + 11], 0);
        expect_eq(pkt[32 + 10], 0);
        expect_eq(pkt[32 + 11], 0);
        expect_eq(pkt[44 + 11], 0);
        expect_eq(pkt[56 + 13], 0);
        expect_eq(pkt[56 + 14], 0);
        expect_eq(pkt[56 + 15], 0);

        if (msg := parseDiameterMessage(pkt)) {
            let it = msg.avps();
            let count: u32 = 0;
            while (avp := it.next()) {
                count += 1;
                if (count == 1) {
                    expect_eq(avp.code, 1001);
                    expect_eq(avp.avp_len, 9);
                    expect_eq(strEq(avp.data, "A"), true);
                } else if (count == 2) {
                    expect_eq(avp.code, 1002);
                    expect_eq(avp.avp_len, 10);
                    expect_eq(strEq(avp.data, "BC"), true);
                } else if (count == 3) {
                    expect_eq(avp.code, 1003);
                    expect_eq(avp.avp_len, 11);
                    expect_eq(strEq(avp.data, "DEF"), true);
                } else if (count == 4) {
                    expect_eq(avp.code, 1004);
                    expect_eq(avp.avp_len, 13);
                    expect_eq(strEq(avp.data, "GHIJK"), true);
                }
            }
            expect_eq(count, 4);
            expect_eq(it.hasError(), false);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("diameter: 3GPP Vendor-Specific 12B AVP (10415) + asU32 / asI32 / asU64") {
    let buf: [256]u8 = [0; 256];
    let w = DiameterWriter.init(
        buf[0..256],
        DIAM_FLAG_REQUEST | DIAM_FLAG_PROXIABLE,
        CMD_AIR_AIA,
        16777251,
        0x12345678,
        0x9ABCDEF0,
    );

    // 3GPP Vendor-Specific AVP (V|M, vendor_id=10415, 5-byte payload -> 12+5=17 -> padded 20)
    expect_eq(
        w.writeAvp(1407, AVP_FLAG_VENDOR | AVP_FLAG_MANDATORY, 10415, "\x01\x02\x03\x04\x05"),
        true,
    );
    // Signed i32 negative (-42 = 0xFFFFFFD6)
    expect_eq(w.writeAvp(2001, AVP_FLAG_MANDATORY, 0, "\xFF\xFF\xFF\xD6"), true);
    // Signed i32 MIN (-2147483648 = 0x80000000)
    expect_eq(w.writeAvp(2002, AVP_FLAG_PROTECTED, 0, "\x80\x00\x00\x00"), true);
    // Unsigned u64 (0x0102030405060708)
    expect_eq(
        w.writeAvp(2003, AVP_FLAG_VENDOR, 10415, "\x01\x02\x03\x04\x05\x06\x07\x08"),
        true,
    );

    if (pkt := w.finish()) {
        if (msg := parseDiameterMessage(pkt)) {
            if (a1 := msg.findAvp(1407)) {
                expect_eq(a1.isVendor(), true);
                expect_eq(a1.isMandatory(), true);
                expect_eq(a1.isProtected(), false);
                expect_eq(a1.vendor_id, 10415);
                expect_eq(a1.avp_len, 17);
                expect_eq(strEq(a1.data, "\x01\x02\x03\x04\x05"), true);
                expect_eq(a1.asU32() == null, true);
                expect_eq(a1.asU64() == null, true);
            } else {
                expect_eq(true, false);
            }

            if (a2 := msg.findAvp(2001)) {
                expect_eq(a2.asI32() == -42, true);
                expect_eq(a2.asU32() == 0xFFFFFFD6, true);
            } else {
                expect_eq(true, false);
            }

            if (a3 := msg.findAvp(2002)) {
                expect_eq(a3.isProtected(), true);
                expect_eq(a3.asI32() == -2147483648, true);
            } else {
                expect_eq(true, false);
            }

            if (a4 := msg.findAvp(2003)) {
                expect_eq(a4.isVendor(), true);
                expect_eq(a4.vendor_id, 10415);
                expect_eq(a4.asU64() == 0x0102030405060708, true);
            } else {
                expect_eq(true, false);
            }
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("diameter: header flags (E/T) & 24-bit Command-Code + empty AVP list") {
    let buf: [64]u8 = [0; 64];
    let w = DiameterWriter.init(
        buf[0..64],
        DIAM_FLAG_ERROR | DIAM_FLAG_RETRANSMIT,
        0x00ABCDEF,
        0xFFFFFFFF,
        0x00000001,
        0x80000002,
    );
    if (pkt := w.finish()) {
        expect_eq(pkt.len, 20);
        if (msg := parseDiameterMessage(pkt)) {
            expect_eq(msg.isRequest(), false);
            expect_eq(msg.isProxiable(), false);
            expect_eq(msg.isError(), true);
            expect_eq(msg.isRetransmit(), true);
            expect_eq(msg.header.command_code, 0x00ABCDEF);
            expect_eq(msg.header.application_id, 0xFFFFFFFF);
            let it = msg.avps();
            expect_eq(it.next() == null, true);
            expect_eq(it.hasError(), false);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("diameter: strict guards (version!=1, non-4B len, reserved flags, truncated AVP)") {
    // 1. Too short (< 20B)
    expect_eq(isDiameter("\x01\x00\x00\x14"), false);

    // 2. Wrong version (version = 2)
    let bad_ver: [20]u8 = [0; 20];
    bad_ver[0] = 2;
    bad_ver[3] = 20;
    expect_eq(parseDiameterMessage(bad_ver[0..20]) == null, true);

    // 3. msg_len not multiple of 4 (22)
    let bad_mod: [24]u8 = [0; 24];
    bad_mod[0] = 1;
    bad_mod[3] = 22;
    expect_eq(parseDiameterMessage(bad_mod[0..24]) == null, true);

    // 4. msg_len < 20 (16) or > data.len (28)
    let bad_small: [20]u8 = [0; 20];
    bad_small[0] = 1;
    bad_small[3] = 16;
    expect_eq(parseDiameterMessage(bad_small[0..20]) == null, true);

    let bad_big: [20]u8 = [0; 20];
    bad_big[0] = 1;
    bad_big[3] = 28;
    expect_eq(parseDiameterMessage(bad_big[0..20]) == null, true);

    // 5. Non-zero reserved header flags (0x01)
    let bad_flags: [20]u8 = [0; 20];
    bad_flags[0] = 1;
    bad_flags[3] = 20;
    bad_flags[4] = DIAM_FLAG_REQUEST | 0x01;
    expect_eq(parseDiameterMessage(bad_flags[0..20]) == null, true);

    // 6. Truncated AVP inside valid 28-byte message (AVP claims len=20, only 8 bytes available)
    let trunc_avp: [28]u8 = [0; 28];
    trunc_avp[0] = 1;
    trunc_avp[3] = 28;
    trunc_avp[4] = DIAM_FLAG_REQUEST;
    // AVP at offset 20: code=263, flags=0x40, avp_len=20 (exceeds 8B remaining)
    trunc_avp[23] = 0x07;
    trunc_avp[24] = AVP_FLAG_MANDATORY;
    trunc_avp[27] = 20;
    if (msg := parseDiameterMessage(trunc_avp[0..28])) {
        let it = msg.avps();
        expect_eq(it.next() == null, true);
        expect_eq(it.hasError(), true);
    } else {
        expect_eq(true, false);
    }

    // 7. Vendor-Specific AVP with avp_len < 12 (avp_len = 10)
    let bad_vendor_len: [32]u8 = [0; 32];
    bad_vendor_len[0] = 1;
    bad_vendor_len[3] = 32;
    bad_vendor_len[24] = AVP_FLAG_VENDOR;
    bad_vendor_len[27] = 10;
    if (msg := parseDiameterMessage(bad_vendor_len[0..32])) {
        let it = msg.avps();
        expect_eq(it.next() == null, true);
        expect_eq(it.hasError(), true);
    } else {
        expect_eq(true, false);
    }
}

test("diameter: DiameterWriter overflow and invalid argument guards") {
    // Buffer smaller than 20B
    let tiny: [16]u8 = [0; 16];
    let w_tiny = DiameterWriter.init(tiny[0..16], DIAM_FLAG_REQUEST, CMD_ACR_ACA, 0, 1, 2);
    expect_eq(w_tiny.hasError(), true);
    expect_eq(w_tiny.finish() == null, true);

    // Invalid reserved flags in init
    let ok_buf: [32]u8 = [0; 32];
    let w_badf = DiameterWriter.init(ok_buf[0..32], 0x08, CMD_DPR_DPA, 0, 1, 2);
    expect_eq(w_badf.hasError(), true);
    expect_eq(w_badf.finish() == null, true);

    // Overflow while writing AVP (32B buffer -> 12B left -> 16B AVP fails)
    let small: [32]u8 = [0; 32];
    let w_ovf = DiameterWriter.init(small[0..32], DIAM_FLAG_REQUEST, CMD_DPR_DPA, 0, 1, 2);
    expect_eq(w_ovf.writeAvp(AVP_ORIGIN_HOST, AVP_FLAG_MANDATORY, 0, "12345678"), false);
    expect_eq(w_ovf.hasError(), true);
    expect_eq(w_ovf.finish() == null, true);
}
