// Source: cp/cap/test/test_coap_production_deep.cp
// Deep Production Test Suite for cap:coap
// Covers:
//   1. CON GET with repeated Uri-Path & Uri-Query options round-trip
//   2. ACK 2.05 Content with Content-Format (50 = application/json) & JSON payload round-trip
//   3. Extended Delta/Length 13 & 14 boundaries (Option 60, Option 350, 280B option value, 300B payload)
//   4. Empty Ping (CON 0.00) & RST (RST 0.00) 4-byte messages + coapCode() helper
//   5. Strict RFC 7252 guards: version != 1, tkl > 8, lone 0xFF marker, nibble == 15, truncated option
//   6. CoapWriter out-of-order Option & buffer overflow guards

import { test, expect_eq } from "cap:test";
import {
    COAP_UDP_PORT,
    COAPS_UDP_PORT,
    COAP_VERSION,
    COAP_HEADER_SIZE,
    COAP_PAYLOAD_MARKER,
    COAP_CON,
    COAP_NON,
    COAP_ACK,
    COAP_RST,
    CODE_EMPTY,
    CODE_GET,
    CODE_POST,
    CODE_PUT,
    CODE_DELETE,
    CODE_CREATED,
    CODE_DELETED,
    CODE_VALID,
    CODE_CHANGED,
    CODE_CONTENT,
    CODE_BAD_REQUEST,
    CODE_NOT_FOUND,
    CODE_INTERNAL_ERROR,
    OPT_IF_MATCH,
    OPT_URI_HOST,
    OPT_ETAG,
    OPT_OBSERVE,
    OPT_URI_PORT,
    OPT_URI_PATH,
    OPT_CONTENT_FORMAT,
    OPT_MAX_AGE,
    OPT_URI_QUERY,
    OPT_ACCEPT,
    OPT_BLOCK2,
    OPT_BLOCK1,
    OPT_SIZE1,
    coapCode,
    CoapOption,
    CoapOptIter,
    CoapMessage,
    CoapWriter,
    parseCoap,
    isCoap,
} from "cap:coap";

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

test("coap: CON GET with repeated Uri-Path & Uri-Query round-trip") {
    expect_eq(COAP_UDP_PORT, 5683);
    expect_eq(COAPS_UDP_PORT, 5684);
    expect_eq(COAP_VERSION, 1);
    expect_eq(COAP_HEADER_SIZE, 4);

    let buf: [256]u8 = [0; 256];
    let w = CoapWriter.init(buf[0..256], COAP_CON, CODE_GET, 0xABCD, "\x11\x22\x33\x44");
    expect_eq(w.writeOption(OPT_URI_HOST, "iot.local"), true);
    expect_eq(w.writeOption(OPT_URI_PATH, "api"), true);
    expect_eq(w.writeOption(OPT_URI_PATH, "temperature"), true);
    expect_eq(w.writeOption(OPT_URI_QUERY, "unit=c"), true);

    if (pkt := w.finish()) {
        expect_eq(isCoap(pkt), true);
        if (msg := parseCoap(pkt)) {
            expect_eq(msg.version, 1);
            expect_eq(msg.msg_type, COAP_CON);
            expect_eq(msg.code, CODE_GET);
            expect_eq(msg.codeClass(), 0);
            expect_eq(msg.codeDetail(), 1);
            expect_eq(msg.isRequest(), true);
            expect_eq(msg.isResponse(), false);
            expect_eq(msg.isEmpty(), false);
            expect_eq(msg.message_id, 0xABCD);
            expect_eq(strEq(msg.token, "\x11\x22\x33\x44"), true);
            expect_eq(msg.payload.len, 0);

            let it = msg.options();
            let count: u32 = 0;
            while (opt := it.next()) {
                count += 1;
                if (count == 1) {
                    expect_eq(opt.number, OPT_URI_HOST);
                    expect_eq(strEq(opt.value, "iot.local"), true);
                } else if (count == 2) {
                    expect_eq(opt.number, OPT_URI_PATH);
                    expect_eq(strEq(opt.value, "api"), true);
                } else if (count == 3) {
                    expect_eq(opt.number, OPT_URI_PATH);
                    expect_eq(strEq(opt.value, "temperature"), true);
                } else if (count == 4) {
                    expect_eq(opt.number, OPT_URI_QUERY);
                    expect_eq(strEq(opt.value, "unit=c"), true);
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

test("coap: ACK 2.05 Content with Content-Format & JSON payload round-trip") {
    let buf: [256]u8 = [0; 256];
    let w = CoapWriter.init(buf[0..256], COAP_ACK, CODE_CONTENT, 0xABCD, "\x11\x22\x33\x44");
    expect_eq(w.writeUintOption(OPT_CONTENT_FORMAT, 50), true); // 50 = application/json
    expect_eq(w.writeUintOption(OPT_MAX_AGE, 30), true);
    expect_eq(w.writePayload("{\"temp\":22.5}"), true);

    if (pkt := w.finish()) {
        if (msg := parseCoap(pkt)) {
            expect_eq(msg.msg_type, COAP_ACK);
            expect_eq(msg.code, CODE_CONTENT);
            expect_eq(msg.codeClass(), 2);
            expect_eq(msg.codeDetail(), 5);
            expect_eq(msg.isResponse(), true);
            expect_eq(msg.contentFormat() == 50, true);
            expect_eq(strEq(msg.payload, "{\"temp\":22.5}"), true);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("coap: extended Delta/Length 13 & 14 boundaries (Opt 60, Opt 350, 280B val, 300B payload)") {
    let long_opt: [280]u8 = [0x42; 280]; // > 269B -> length nibble 14 (+2B)
    long_opt[0] = 0x53; // 'S'
    long_opt[279] = 0x45; // 'E'

    let long_pay: [300]u8 = [0x50; 300];
    long_pay[0] = 0x7B; // '{'
    long_pay[299] = 0x7D; // '}'

    let buf: [1024]u8 = [0; 1024];
    let w = CoapWriter.init(buf[0..1024], COAP_NON, CODE_POST, 0x0042, "");
    // Option 60 (delta = 60 -> nibble 13, 1B ext = 47)
    expect_eq(w.writeUintOption(OPT_SIZE1, 300), true);
    // Option 350 (delta = 290 -> nibble 14, 2B ext = 21) with 280B value (nibble 14, 2B ext = 11)
    expect_eq(w.writeOption(350, long_opt[0..280]), true);
    expect_eq(w.writePayload(long_pay[0..300]), true);

    if (pkt := w.finish()) {
        if (msg := parseCoap(pkt)) {
            expect_eq(msg.payload.len, 300);
            expect_eq(msg.payload[0], 0x7B);
            expect_eq(msg.payload[299], 0x7D);

            if (o60 := msg.findOption(OPT_SIZE1)) {
                expect_eq(o60.value.len, 2); // 300 = 0x012C
                expect_eq(o60.value[0], 0x01);
                expect_eq(o60.value[1], 0x2C);
            } else {
                expect_eq(true, false);
            }

            if (o350 := msg.findOption(350)) {
                expect_eq(o350.value.len, 280);
                expect_eq(o350.value[0], 0x53);
                expect_eq(o350.value[279], 0x45);
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

test("coap: Empty Ping/RST 4-byte messages & coapCode() helper") {
    expect_eq(coapCode(2, 5), CODE_CONTENT);
    expect_eq(coapCode(4, 4), CODE_NOT_FOUND);
    expect_eq(coapCode(5, 0), CODE_INTERNAL_ERROR);

    let buf: [8]u8 = [0; 8];
    let w = CoapWriter.init(buf[0..8], COAP_RST, CODE_EMPTY, 0x9999, "");
    if (pkt := w.finish()) {
        expect_eq(pkt.len, 4);
        if (msg := parseCoap(pkt)) {
            expect_eq(msg.msg_type, COAP_RST);
            expect_eq(msg.isEmpty(), true);
            expect_eq(msg.isRequest(), false);
            expect_eq(msg.isResponse(), false);
            expect_eq(msg.message_id, 0x9999);
        } else {
            expect_eq(true, false);
        }
    } else {
        expect_eq(true, false);
    }
}

test("coap: strict RFC 7252 guards (bad version, tkl>8, lone 0xFF, nibble=15, truncated)") {
    // 1. Too short (< 4B)
    expect_eq(isCoap("\x40\x01\x00"), false);

    // 2. Wrong version (ver=2 -> 0x80)
    expect_eq(parseCoap("\x80\x01\x00\x01") == null, true);

    // 3. tkl = 9 (> 8) -> rejected
    expect_eq(parseCoap("\x49\x01\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00") == null, true);

    // 4. Lone 0xFF payload marker with zero payload bytes -> rejected
    expect_eq(parseCoap("\x40\x01\x00\x01\xFF") == null, true);

    // 5. Reserved nibble = 15 (0xF0 or 0x0F, not 0xFF) -> rejected
    expect_eq(parseCoap("\x40\x01\x00\x01\xF0") == null, true);
    expect_eq(parseCoap("\x40\x01\x00\x01\x1F") == null, true);

    // 6. Truncated extended delta=13 (missing 1B ext) & truncated option value
    expect_eq(parseCoap("\x40\x01\x00\x01\xD0") == null, true);
    expect_eq(parseCoap("\x40\x01\x00\x01\xB5abc") == null, true);
}

test("coap: CoapWriter out-of-order Option & buffer overflow guards") {
    let buf: [64]u8 = [0; 64];
    let w = CoapWriter.init(buf[0..64], COAP_CON, CODE_GET, 1, "");
    expect_eq(w.writeOption(OPT_URI_PATH, "a"), true);
    // Out-of-order option number (3 < 11) -> rejected
    expect_eq(w.writeOption(OPT_URI_HOST, "x"), false);
    expect_eq(w.hasError(), true);
    expect_eq(w.finish() == null, true);

    // Buffer too small for payload
    let tiny: [6]u8 = [0; 6];
    let w2 = CoapWriter.init(tiny[0..6], COAP_CON, CODE_POST, 2, "");
    expect_eq(w2.writePayload("hello"), false);
    expect_eq(w2.finish() == null, true);
}
