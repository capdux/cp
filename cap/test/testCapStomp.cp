import { test, expect_eq } from "cap:test";
import {
    STOMP_OK,
    STOMP_INCOMPLETE,
    STOMP_INVALID,
    StompHeader,
    StompFrame,
    escapeHeader,
    unescapeHeader,
    encodeFrame,
    decodeFrame,
    getHeader,
} from "../../packages/stomp/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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

fn main() {
    test("1. @cap/stomp — CONNECT / SEND / MESSAGE encode & decode roundtrip", () => {
        const hdrs: [2]StompHeader = [
            StompHeader { key: "destination", value: "/queue/orders" },
            StompHeader { key: "content-type", value: "text/plain" },
        ];
        let wire: [256]u8 = undefined;
        const n = encodeFrame("SEND", hdrs[0..2], "hello stomp", wire[0..256]);
        expect_eq(n > 0, true);

        let frame = StompFrame.empty();
        const st = decodeFrame(wire[0..n], &frame);
        expect_eq(st, STOMP_OK);
        expect_eq(frame.consumed, n);
        expect_eq(testEq(frame.command, "SEND"), true);
        expect_eq(frame.header_count, 2 as usize);
        expect_eq(testEq(getHeader(frame, "destination"), "/queue/orders"), true);
        expect_eq(testEq(getHeader(frame, "content-type"), "text/plain"), true);
        expect_eq(testEq(getHeader(frame, "missing"), ""), true);
        expect_eq(testEq(frame.body, "hello stomp"), true);
    });

    test("2. @cap/stomp — STOMP 1.2 header escape & unescape", () => {
        const raw = "a:b\rc\nd\\e";
        let esc: [64]u8 = undefined;
        let unesc: [64]u8 = undefined;

        const n1 = escapeHeader(raw, esc[0..64]);
        expect_eq(testEq(esc[0..n1], "a\\cb\\rc\\nd\\\\e"), true);

        const n2 = unescapeHeader(esc[0..n1], unesc[0..64]);
        expect_eq(testEq(unesc[0..n2], raw), true);
    });

    test("3. @cap/stomp — Binary body containing embedded NUL bytes via content-length", () => {
        const bin_body: [5]u8 = [0x41, 0x00, 0x42, 0x00, 0x43];
        const hdrs: [2]StompHeader = [
            StompHeader { key: "destination", value: "/topic/bin" },
            StompHeader { key: "content-length", value: "5" },
        ];
        let wire: [128]u8 = undefined;
        const n = encodeFrame("MESSAGE", hdrs[0..2], bin_body[0..5], wire[0..128]);
        expect_eq(n > 0, true);

        let frame = StompFrame.empty();
        expect_eq(decodeFrame(wire[0..n], &frame), STOMP_OK);
        expect_eq(frame.body.len, 5 as usize);
        expect_eq(frame.body[0], 0x41 as u8);
        expect_eq(frame.body[1], 0x00 as u8);
        expect_eq(frame.body[2], 0x42 as u8);
        expect_eq(frame.body[3], 0x00 as u8);
        expect_eq(frame.body[4], 0x43 as u8);
    });

    test("4. @cap/stomp — Leading heartbeat EOLs (\\n and \\r\\n) skipped cleanly", () => {
        const raw = "\n\r\n\nCONNECTED\r\nversion:1.2\r\n\r\n\x00";
        let frame = StompFrame.empty();
        expect_eq(decodeFrame(raw, &frame), STOMP_OK);
        expect_eq(testEq(frame.command, "CONNECTED"), true);
        expect_eq(testEq(getHeader(frame, "version"), "1.2"), true);
        expect_eq(frame.body.len, 0 as usize);
        expect_eq(frame.consumed, raw.len);
    });

    test("5. @cap/stomp — Partial/truncated frames return STOMP_INCOMPLETE", () => {
        let frame = StompFrame.empty();
        expect_eq(decodeFrame("", &frame), STOMP_INCOMPLETE);
        expect_eq(decodeFrame("\n\n", &frame), STOMP_INCOMPLETE);
        expect_eq(decodeFrame("SEND\ndestination:/q\n\nno-nul-yet", &frame), STOMP_INCOMPLETE);
        expect_eq(decodeFrame("MESSAGE\ncontent-length:10\n\nabc\x00", &frame), STOMP_INCOMPLETE);
    });

    test("6. @cap/stomp — Invalid frames & undersized encode buffer rejected", () => {
        let frame = StompFrame.empty();
        // Missing ':' in header
        expect_eq(decodeFrame("SEND\nbadheader\n\n\x00", &frame), STOMP_INVALID);
        // Non-numeric content-length
        expect_eq(decodeFrame("SEND\ncontent-length:abc\n\n\x00", &frame), STOMP_INVALID);
        // Non-NUL byte after content-length
        expect_eq(decodeFrame("SEND\ncontent-length:2\n\nabX", &frame), STOMP_INVALID);

        // Undersized encode buffer returns 0
        const hdrs: [1]StompHeader = [StompHeader { key: "k", value: "v" }];
        let tiny: [4]u8 = undefined;
        expect_eq(encodeFrame("SEND", hdrs[0..1], "body", tiny[0..4]), 0 as usize);
        expect_eq(encodeFrame("", hdrs[0..1], "", tiny[0..4]), 0 as usize);
    });
}
