// Source: cp/cap/test/test_ipsec_production_deep.cp
// Deep Production Test Suite for cap:ipsec
// Covers:
//   1. RFC 4303 ESP header parsing & decrypted trailer stripping (pad_len, next_header, ICV)
//   2. RFC 4302 AH 24-byte header (12B ICV) parsing + non-zero reserved & truncated length rejection
//   3. Full RFC 7296 IKEv2 IKE_SA_INIT header + SA -> KE -> NONCE -> NOTIFY chained payload iteration
//   4. IKEv2 Critical bit flag & isInitiator() / isResponse() helpers
//   5. Strict IKEv2 header guards: major_version != 2, length < 28, length > data.len, truncated < 28B
//   6. Malformed IKEv2 payload chain guards (len < 4, truncated payload body) & ESP trailer overflow

import { test, expect_eq } from "cap:test";
import {
    IPSEC_PROTO_ESP,
    IPSEC_PROTO_AH,
    IKE_UDP_PORT,
    IKE_NATT_UDP_PORT,
    ESP_MIN_HEADER_SIZE,
    AH_MIN_HEADER_SIZE,
    IKEV2_HEADER_SIZE,
    IKE_EXCH_SA_INIT,
    IKE_EXCH_AUTH,
    IKE_EXCH_CREATE_CHILD_SA,
    IKE_EXCH_INFORMATIONAL,
    IKE_FLAG_INITIATOR,
    IKE_FLAG_VERSION,
    IKE_FLAG_RESPONSE,
    PL_NONE,
    PL_SA,
    PL_KE,
    PL_IDI,
    PL_IDR,
    PL_CERT,
    PL_CERTREQ,
    PL_AUTH,
    PL_NONCE,
    PL_NOTIFY,
    PL_DELETE,
    PL_VENDOR_ID,
    PL_TSI,
    PL_TSR,
    PL_SK,
    EspPacket,
    EspTrailer,
    AhPacket,
    Ikev2Header,
    Ikev2Payload,
    Ikev2PayloadIter,
    parseEsp,
    parseEspTrailer,
    parseAh,
    parseIkev2,
    isIkev2,
} from "cap:net";

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

test("ipsec: RFC 4303 ESP header parsing & decrypted trailer stripping") {
    expect_eq(IPSEC_PROTO_ESP, 50);
    expect_eq(ESP_MIN_HEADER_SIZE, 8);

    // SPI=0x12345678, Seq=0x0000002A, decrypted payload: "HELLO" (5B) + 3B pad (1,2,3) + pad_len=3 + next_hdr=4 (IPv4) + 12B ICV
    let esp_pkt: [30]u8 = [
        0x12, 0x34, 0x56, 0x78, // SPI
        0x00, 0x00, 0x00, 0x2A, // Seq = 42
        0x48, 0x45, 0x4C, 0x4C, 0x4F, // "HELLO"
        0x01, 0x02, 0x03,       // 3 bytes padding
        0x03, 0x04,             // pad_len = 3, next_header = 4 (IP-in-IP)
        0xAA, 0xBB, 0xCC, 0xDD, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, // 12B ICV
    ];

    if (esp := parseEsp(esp_pkt[0..30])) {
        expect_eq(esp.spi, 0x12345678);
        expect_eq(esp.seq_num, 42);
        expect_eq(esp.payload_and_trailer.len, 22);

        if (tr := parseEspTrailer(esp.payload_and_trailer, 12)) {
            expect_eq(strEq(tr.inner_payload, "HELLO"), true);
            expect_eq(tr.pad_len, 3);
            expect_eq(tr.next_header, 4);
            expect_eq(tr.icv.len, 12);
            expect_eq(tr.icv[0], 0xAA);
            expect_eq(tr.icv[11], 0x88);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }

    // Too short ESP (< 8B) -> null
    expect_eq(parseEsp("\x12\x34\x56\x78\x00\x00\x00") == null, true);
}

test("ipsec: RFC 4302 AH 24-byte header (12B ICV) & reserved/length guards") {
    expect_eq(IPSEC_PROTO_AH, 51);
    expect_eq(AH_MIN_HEADER_SIZE, 12);

    // next_header=6 (TCP), payload_len_words=4 -> (4+2)*4 = 24B header (12B fixed + 12B ICV)
    // reserved=0x0000, SPI=0xCAFEBABE, Seq=99, 12B ICV, 4B TCP payload
    let ah_pkt: [28]u8 = [
        0x06, 0x04, 0x00, 0x00,
        0xCA, 0xFE, 0xBA, 0xBE,
        0x00, 0x00, 0x00, 0x63,
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C,
        0x54, 0x43, 0x50, 0x21, // "TCP!"
    ];

    if (ah := parseAh(ah_pkt[0..28])) {
        expect_eq(ah.next_header, 6);
        expect_eq(ah.payload_len_words, 4);
        expect_eq(ah.header_bytes, 24);
        expect_eq(ah.spi, 0xCAFEBABE);
        expect_eq(ah.seq_num, 99);
        expect_eq(ah.icv.len, 12);
        expect_eq(ah.icv[0], 0x01);
        expect_eq(ah.icv[11], 0x0C);
        expect_eq(strEq(ah.payload, "TCP!"), true);
    } else {
        expect_eq(true, false);
    }

    // Non-zero reserved -> rejected
    let bad_resv = ah_pkt;
    bad_resv[3] = 0x01;
    expect_eq(parseAh(bad_resv[0..28]) == null, true);

    // payload_len_words=8 -> 40B header > 28B buffer -> rejected
    let bad_len = ah_pkt;
    bad_len[1] = 8;
    expect_eq(parseAh(bad_len[0..28]) == null, true);

    // payload_len_words=0 -> 8B < AH_MIN_HEADER_SIZE (12) -> rejected
    let bad_small = ah_pkt;
    bad_small[1] = 0;
    expect_eq(parseAh(bad_small[0..28]) == null, true);
}

test("ipsec: full IKEv2 IKE_SA_INIT header + SA -> KE -> NONCE -> NOTIFY chained payloads") {
    expect_eq(IKE_UDP_PORT, 500);
    expect_eq(IKE_NATT_UDP_PORT, 4500);
    expect_eq(IKEV2_HEADER_SIZE, 28);

    // 28B header + SA(8B) + KE(10B) + NONCE(8B) + NOTIFY(6B) = 60B
    let ike: [60]u8 = [
        // Initiator SPI = 0x0102030405060708
        0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        // Responder SPI = 0x0000000000000000
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        // Next Payload = SA (33), Ver = 0x20 (v2.0), Exchange = IKE_SA_INIT (34), Flags = Initiator (0x08)
        PL_SA, 0x20, IKE_EXCH_SA_INIT, IKE_FLAG_INITIATOR,
        // Message ID = 0
        0x00, 0x00, 0x00, 0x00,
        // Total Length = 60 (0x0000003C)
        0x00, 0x00, 0x00, 0x3C,
        // Payload 1 (SA): next=KE(34), flags=0x00, len=8, body="SA01"
        PL_KE, 0x00, 0x00, 0x08, 0x53, 0x41, 0x30, 0x31,
        // Payload 2 (KE): next=NONCE(40), flags=0x80 (critical), len=10, body="KEYEX1"
        PL_NONCE, 0x80, 0x00, 0x0A, 0x4B, 0x45, 0x59, 0x45, 0x58, 0x31,
        // Payload 3 (NONCE): next=NOTIFY(41), flags=0x00, len=8, body="NONC"
        PL_NOTIFY, 0x00, 0x00, 0x08, 0x4E, 0x4F, 0x4E, 0x43,
        // Payload 4 (NOTIFY): next=NONE(0), flags=0x00, len=6, body="OK"
        PL_NONE, 0x00, 0x00, 0x06, 0x4F, 0x4B,
    ];

    expect_eq(isIkev2(ike[0..60]), true);
    if (hdr := parseIkev2(ike[0..60])) {
        expect_eq(hdr.initiator_spi, 0x0102030405060708);
        expect_eq(hdr.responder_spi, 0);
        expect_eq(hdr.next_payload, PL_SA);
        expect_eq(hdr.major_version, 2);
        expect_eq(hdr.minor_version, 0);
        expect_eq(hdr.exchange_type, IKE_EXCH_SA_INIT);
        expect_eq(hdr.isInitiator(), true);
        expect_eq(hdr.isResponse(), false);
        expect_eq(hdr.message_id, 0);
        expect_eq(hdr.length, 60);

        let it = hdr.payloads();
        let count: u32 = 0;
        while (pl := it.next()) {
            count += 1;
            if (count == 1) {
                expect_eq(pl.payload_type, PL_SA);
                expect_eq(pl.next_payload, PL_KE);
                expect_eq(pl.is_critical, false);
                expect_eq(strEq(pl.body, "SA01"), true);
            } else if (count == 2) {
                expect_eq(pl.payload_type, PL_KE);
                expect_eq(pl.next_payload, PL_NONCE);
                expect_eq(pl.is_critical, true);
                expect_eq(strEq(pl.body, "KEYEX1"), true);
            } else if (count == 3) {
                expect_eq(pl.payload_type, PL_NONCE);
                expect_eq(pl.next_payload, PL_NOTIFY);
                expect_eq(pl.is_critical, false);
                expect_eq(strEq(pl.body, "NONC"), true);
            } else if (count == 4) {
                expect_eq(pl.payload_type, PL_NOTIFY);
                expect_eq(pl.next_payload, PL_NONE);
                expect_eq(pl.is_critical, false);
                expect_eq(strEq(pl.body, "OK"), true);
            }
        }
        expect_eq(count, 4);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("ipsec: IKEv2 response flags & empty payload chain (PL_NONE)") {
    let resp: [28]u8 = [
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
        0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF, 0x00, 0x11,
        PL_NONE, 0x20, IKE_EXCH_INFORMATIONAL, IKE_FLAG_RESPONSE,
        0x00, 0x00, 0x00, 0x05,
        0x00, 0x00, 0x00, 0x1C, // length = 28
    ];

    if (hdr := parseIkev2(resp[0..28])) {
        expect_eq(hdr.isInitiator(), false);
        expect_eq(hdr.isResponse(), true);
        expect_eq(hdr.exchange_type, IKE_EXCH_INFORMATIONAL);
        expect_eq(hdr.message_id, 5);
        let it = hdr.payloads();
        expect_eq(it.next() == null, true);
        expect_eq(it.hasError(), false);
    } else {
        expect_eq(true, false);
    }
}

test("ipsec: strict IKEv2 header guards (version!=2, len<28, len>data.len, truncated)") {
    // 1. Truncated < 28B
    expect_eq(isIkev2("\x01\x02\x03\x04"), false);

    // 2. IKEv1 (major_version = 1 -> 0x10) -> rejected
    let v1: [28]u8 = [0; 28];
    v1[17] = 0x10;
    v1[27] = 28;
    expect_eq(parseIkev2(v1[0..28]) == null, true);

    // 3. msg_len < 28 (24) -> rejected
    let small_len: [28]u8 = [0; 28];
    small_len[17] = 0x20;
    small_len[27] = 24;
    expect_eq(parseIkev2(small_len[0..28]) == null, true);

    // 4. msg_len > data.len (40 > 28) -> rejected
    let big_len: [28]u8 = [0; 28];
    big_len[17] = 0x20;
    big_len[27] = 40;
    expect_eq(parseIkev2(big_len[0..28]) == null, true);
}

test("ipsec: malformed IKEv2 payload chain (len<4, truncated) & ESP trailer overflow") {
    // 1. IKEv2 payload with len=2 (< 4) -> iterator sets hasError()
    let bad_pl_len: [32]u8 = [0; 32];
    bad_pl_len[16] = PL_SA;
    bad_pl_len[17] = 0x20;
    bad_pl_len[27] = 32;
    bad_pl_len[28] = PL_NONE;
    bad_pl_len[31] = 2; // invalid payload length < 4
    if (hdr := parseIkev2(bad_pl_len[0..32])) {
        let it = hdr.payloads();
        expect_eq(it.next() == null, true);
        expect_eq(it.hasError(), true);
    } else {
        expect_eq(true, false);
    }

    // 2. IKEv2 payload length exceeds remaining slice (claims 20B, only 4B present)
    let trunc_pl: [32]u8 = [0; 32];
    trunc_pl[16] = PL_KE;
    trunc_pl[17] = 0x20;
    trunc_pl[27] = 32;
    trunc_pl[28] = PL_NONE;
    trunc_pl[31] = 20;
    if (hdr := parseIkev2(trunc_pl[0..32])) {
        let it = hdr.payloads();
        expect_eq(it.next() == null, true);
        expect_eq(it.hasError(), true);
    } else {
        expect_eq(true, false);
    }

    // 3. ESP trailer with pad_len > available body -> null
    let bad_esp_pay: [6]u8 = [0xAA, 0xBB, 0x05, 0x06, 0x00, 0x00]; // icv=2 -> trailer at [2..4], pad_len=5 > 2
    expect_eq(parseEspTrailer(bad_esp_pay[0..6], 2) == null, true);

    // 4. ESP trailer with icv_len + 2 > decrypted_payload.len -> null
    expect_eq(parseEspTrailer(bad_esp_pay[0..6], 5) == null, true);
}
