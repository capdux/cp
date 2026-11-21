// Source: cp/cap/test/test_eap_production_deep.cp
// Deep Production Test Suite for cap:eap
// Covers:
//   1. IEEE 802.1X EAPOL unwrapping (EAPOL-Packet -> EAP Response/Identity, EAPOL-Start, EAPOL-Key)
//   2. EAP Request/Response Identity & EAP-TLS decoding
//   3. EAP Success & EAP Failure decoding (strict length == 4)
//   4. RFC 3748 Section 5.7 Expanded Type (254) Vendor-ID / Vendor-Type / vendor_data decoding
//   5. Strict EAPOL guards: version 0 or > 3, truncated header, body_len > data.len
//   6. Strict EAP & Expanded guards: invalid code, Success/Failure len!=4, Request len==4, Expanded < 7B

import { test, expect_eq } from "cap:test";
import {
    EAPOL_ETHERTYPE,
    EAPOL_HEADER_SIZE,
    EAP_HEADER_SIZE,
    EAP_EXPANDED_HEADER_SIZE,
    EAP_TYPE_IDENTITY,
    EAP_TYPE_NOTIFICATION,
    EAP_TYPE_NAK,
    EAP_TYPE_MD5_CHALLENGE,
    EAP_TYPE_OTP,
    EAP_TYPE_GTC,
    EAP_TYPE_TLS,
    EAP_TYPE_TTLS,
    EAP_TYPE_AKA,
    EAP_TYPE_PEAP,
    EAP_TYPE_MSCHAPV2,
    EAP_TYPE_EXPANDED,
    EapolType,
    EapCode,
    EapolFrame,
    EapPacket,
    EapExpanded,
    parseEapol,
    isEapol,
    parseEap,
    isEap,
    parseEapExpanded,
} from "cap:eap";

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

test("eap: EAPOL unwrapping (EAPOL-Packet -> EAP Response/Identity, Start, Key)") {
    expect_eq(EAPOL_ETHERTYPE, 0x888E);
    expect_eq(EAPOL_HEADER_SIZE, 4);
    expect_eq(EAP_HEADER_SIZE, 4);
    expect_eq(EAP_EXPANDED_HEADER_SIZE, 8);

    // EAPOL v2, type=0 (EAP-Packet), body_len=10
    // Inner EAP: code=2 (Response), id=7, len=10, type=1 (Identity), "alice"
    let frame: [14]u8 = [
        0x02, 0x00, 0x00, 0x0A,
        0x02, 0x07, 0x00, 0x0A, EAP_TYPE_IDENTITY,
        0x61, 0x6C, 0x69, 0x63, 0x65,
    ];

    expect_eq(isEapol(frame[0..14]), true);
    if (eapol := parseEapol(frame[0..14])) {
        expect_eq(eapol.version, 2);
        expect_eq(eapol.pkt_type == EapolType.eap_packet, true);
        expect_eq(eapol.body_len, 10);
        if (eap := eapol.asEap()) {
            expect_eq(eap.code == EapCode.response, true);
            expect_eq(eap.identifier, 7);
            expect_eq(eap.length, 10);
            expect_eq(eap.eap_type == EAP_TYPE_IDENTITY, true);
            expect_eq(strEq(eap.type_data, "alice"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // EAPOL-Start (v1, type=1, body_len=0)
    let start_f: [4]u8 = [0x01, 0x01, 0x00, 0x00];
    if (eapol := parseEapol(start_f[0..4])) {
        expect_eq(eapol.version, 1);
        expect_eq(eapol.pkt_type == EapolType.start, true);
        expect_eq(eapol.body.len, 0);
        expect_eq(eapol.asEap() == null, true);
    } else {
        expect_eq(true, false);
    }

    // EAPOL-Key (v2, type=3, body_len=4)
    let key_f: [8]u8 = [0x02, 0x03, 0x00, 0x04, 0x02, 0x00, 0x8A, 0x00];
    if (eapol := parseEapol(key_f[0..8])) {
        expect_eq(eapol.pkt_type == EapolType.key, true);
        expect_eq(eapol.body.len, 4);
    } else {
        expect_eq(true, false);
    }
}

test("eap: EAP Request/TLS & EAP Response/Identity decoding") {
    // EAP Request (1), id=0x15, len=6, type=EAP_TYPE_TLS (13), flags=0x20 (Start)
    let tls_start: [6]u8 = [0x01, 0x15, 0x00, 0x06, EAP_TYPE_TLS, 0x20];
    expect_eq(isEap(tls_start[0..6]), true);
    if (eap := parseEap(tls_start[0..6])) {
        expect_eq(eap.code == EapCode.request, true);
        expect_eq(eap.identifier, 0x15);
        expect_eq(eap.length, 6);
        expect_eq(eap.eap_type == EAP_TYPE_TLS, true);
        expect_eq(eap.type_data.len, 1);
        expect_eq(eap.type_data[0], 0x20);
    } else {
        expect_eq(true, false);
    }
}

test("eap: EAP Success (3) & Failure (4) decoding") {
    let succ: [4]u8 = [0x03, 0x42, 0x00, 0x04];
    if (eap := parseEap(succ[0..4])) {
        expect_eq(eap.code == EapCode.success, true);
        expect_eq(eap.identifier, 0x42);
        expect_eq(eap.length, 4);
        expect_eq(eap.eap_type == null, true);
        expect_eq(eap.type_data.len, 0);
    } else {
        expect_eq(true, false);
    }

    let fail: [4]u8 = [0x04, 0x99, 0x00, 0x04];
    if (eap := parseEap(fail[0..4])) {
        expect_eq(eap.code == EapCode.failure, true);
        expect_eq(eap.identifier, 0x99);
        expect_eq(eap.length, 4);
        expect_eq(eap.eap_type == null, true);
        expect_eq(eap.type_data.len, 0);
    } else {
        expect_eq(true, false);
    }
}

test("eap: RFC 3748 Section 5.7 Expanded Type (254) Vendor-ID / Vendor-Type") {
    // EAP Request, id=3, len=16, type=254
    // Expanded: Vendor-ID = 0x00372A (WFA = 14122), Vendor-Type = 0x00000001 (WPS), data="WPS!"
    let pkt: [16]u8 = [
        0x01, 0x03, 0x00, 0x10, EAP_TYPE_EXPANDED,
        0x00, 0x37, 0x2A,
        0x00, 0x00, 0x00, 0x01,
        0x57, 0x50, 0x53, 0x21,
    ];

    if (eap := parseEap(pkt[0..16])) {
        expect_eq(eap.eap_type == EAP_TYPE_EXPANDED, true);
        if (exp := parseEapExpanded(eap.type_data)) {
            expect_eq(exp.vendor_id, 0x00372A);
            expect_eq(exp.vendor_type, 1);
            expect_eq(strEq(exp.vendor_data, "WPS!"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("eap: strict EAPOL guards (version 0 / >3, truncated, oversized body_len)") {
    expect_eq(isEapol("\x01\x00\x00"), false);
    expect_eq(parseEapol("\x00\x00\x00\x00") == null, true); // version 0
    expect_eq(parseEapol("\x04\x00\x00\x00") == null, true); // version 4
    expect_eq(parseEapol("\x02\x00\x00\x10\x01\x02") == null, true); // body_len=16 > 2
}

test("eap: strict EAP & Expanded guards (bad code, Success len!=4, Request len==4, Expanded < 7B)") {
    // Invalid code 0 and 5
    expect_eq(parseEap("\x00\x01\x00\x04") == null, true);
    expect_eq(parseEap("\x05\x01\x00\x04") == null, true);

    // Success / Failure with length != 4 -> rejected
    expect_eq(parseEap("\x03\x01\x00\x05\x00") == null, true);
    expect_eq(parseEap("\x04\x01\x00\x06\x00\x00") == null, true);

    // Request / Response with length == 4 (missing eap_type) -> rejected
    expect_eq(parseEap("\x01\x01\x00\x04") == null, true);
    expect_eq(parseEap("\x02\x01\x00\x04") == null, true);

    // Declared length > data.len -> rejected
    expect_eq(parseEap("\x01\x01\x00\x10\x01abc") == null, true);

    // Expanded type_data < 7B -> rejected
    expect_eq(parseEapExpanded("\x00\x37\x2A\x00\x00\x00") == null, true);
}
