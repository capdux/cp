import { test, expect_eq } from "cap:test";
import {
    OP_TEXT,
    OP_BINARY,
    OP_PING,
    OP_CLOSE,
    CLOSE_NORMAL,
    WS_OK,
    WS_INCOMPLETE,
    WS_INVALID,
    FrameHeader,
    decodeHeader,
    encodeHeader,
    encodeFrame,
    applyMask,
    encodeClosePayload,
    isValidCloseCode,
} from "../../packages/ws/src/index.cp";

fn main() {
    test("1. @cap/ws-frame — unmasked short text frame roundtrip", () => {
        let key: [4]u8 = undefined;
        key[0] = 0; key[1] = 0; key[2] = 0; key[3] = 0;
        let payload: string = "Hello CP!";
        let buf: [64]u8 = undefined;
        let n: usize = encodeFrame(true, OP_TEXT, false, key, payload, buf[0..]);
        expect_eq(n, 2 + 9);

        let hdr: FrameHeader = undefined;
        expect_eq(decodeHeader(buf[0..n], &hdr), WS_OK);
        expect_eq(hdr.fin, true);
        expect_eq(hdr.opcode, OP_TEXT);
        expect_eq(hdr.masked, false);
        expect_eq(hdr.payload_len, 9);
        expect_eq(hdr.header_len, 2);
        expect_eq(buf[hdr.header_len], 'H');
    });

    test("2. @cap/ws-frame — masked client frame roundtrip & XOR unmasking", () => {
        let key: [4]u8 = undefined;
        key[0] = 0x37; key[1] = 0xfa; key[2] = 0x21; key[3] = 0x3d;
        let payload: string = "Hello";
        let buf: [64]u8 = undefined;
        let n: usize = encodeFrame(true, OP_TEXT, true, key, payload, buf[0..]);
        expect_eq(n, 6 + 5);

        let hdr: FrameHeader = undefined;
        expect_eq(decodeHeader(buf[0..n], &hdr), WS_OK);
        expect_eq(hdr.masked, true);
        expect_eq(hdr.header_len, 6);
        expect_eq(hdr.mask_key[0], 0x37);

        applyMask(buf[hdr.header_len..n], hdr.mask_key);
        expect_eq(buf[hdr.header_len], 'H');
        expect_eq(buf[hdr.header_len + 4], 'o');
    });

    test("3. @cap/ws-frame — 16-bit (126) and 64-bit (127) extended payload lengths", () => {
        let key: [4]u8 = undefined;
        key[0] = 1; key[1] = 2; key[2] = 3; key[3] = 4;
        let buf: [32]u8 = undefined;

        let h16: usize = encodeHeader(true, OP_BINARY, false, key, 1024, buf[0..]);
        expect_eq(h16, 4);
        let hdr16: FrameHeader = undefined;
        expect_eq(decodeHeader(buf[0..h16], &hdr16), WS_OK);
        expect_eq(hdr16.payload_len, 1024);

        let h64: usize = encodeHeader(true, OP_BINARY, true, key, 70000, buf[0..]);
        expect_eq(h64, 14);
        let hdr64: FrameHeader = undefined;
        expect_eq(decodeHeader(buf[0..h64], &hdr64), WS_OK);
        expect_eq(hdr64.payload_len, 70000);
        expect_eq(hdr64.masked, true);
    });

    test("4. @cap/ws-frame — close payload encoding & status code validation", () => {
        let cbuf: [32]u8 = undefined;
        let cn: usize = encodeClosePayload(CLOSE_NORMAL, "bye", cbuf[0..]);
        expect_eq(cn, 5);
        expect_eq(cbuf[0], 0x03);
        expect_eq(cbuf[1], 0xe8);
        expect_eq(cbuf[2], 'b');

        expect_eq(isValidCloseCode(1000), true);
        expect_eq(isValidCloseCode(4001), true);
        expect_eq(isValidCloseCode(1005), false);
        expect_eq(isValidCloseCode(1006), false);
        expect_eq(isValidCloseCode(999), false);
    });

    test("5. @cap/ws-frame — incomplete header & RFC 6455 protocol violation guards", () => {
        let hdr: FrameHeader = undefined;
        let one: [1]u8 = undefined;
        one[0] = 0x81;
        expect_eq(decodeHeader(one[0..1], &hdr), WS_INCOMPLETE);

        // Fragmented PING control frame (fin=0, opcode=9) MUST be rejected
        let bad_ping: [2]u8 = undefined;
        bad_ping[0] = OP_PING;
        bad_ping[1] = 0;
        expect_eq(decodeHeader(bad_ping[0..2], &hdr), WS_INVALID);

        // Invalid reserved opcode 0x3
        let bad_op: [2]u8 = undefined;
        bad_op[0] = 0x83;
        bad_op[1] = 0;
        expect_eq(decodeHeader(bad_op[0..2], &hdr), WS_INVALID);
    });

    test("6. @cap/ws-frame — small buffer protection", () => {
        let key: [4]u8 = undefined;
        key[0] = 0; key[1] = 0; key[2] = 0; key[3] = 0;
        let tiny: [3]u8 = undefined;
        expect_eq(encodeFrame(true, OP_CLOSE, false, key, "ok", tiny[0..]), 0);
        expect_eq(encodeClosePayload(CLOSE_NORMAL, "long", tiny[0..]), 0);
    });
}
