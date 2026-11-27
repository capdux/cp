import { test, expect_eq } from "cap:test";
import {
    encode,
    decode,
    encodedLen,
    NS_OK,
    NS_INCOMPLETE,
    NS_INVALID,
} from "../../packages/netstring/src/index.cp";

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i += 1;
    }
    return true;
}

fn main() {
    test("encode and decode official Bernstein vectors", () => {
        let buf: [64]u8 = undefined;
        let n1 = encode("hello world!", buf[0..64]);
        expect_eq(n1, 16);
        expect_eq(eqBytes(buf[0..16], "12:hello world!,"), true);

        let f = decode(buf[0..16]);
        expect_eq(f.status, NS_OK);
        expect_eq(f.consumed, 16);
        expect_eq(eqBytes(buf[f.payload_start .. f.payload_start + f.payload_len], "hello world!"), true);
    });

    test("empty payload encodes as 0:, and decodes cleanly", () => {
        let buf: [8]u8 = undefined;
        expect_eq(encodedLen(0), 3);
        let n = encode("", buf[0..8]);
        expect_eq(n, 3);
        expect_eq(eqBytes(buf[0..3], "0:,"), true);
        let f = decode(buf[0..3]);
        expect_eq(f.status, NS_OK);
        expect_eq(f.payload_len, 0);
        expect_eq(f.consumed, 3);
    });

    test("binary-safe payload with null bytes, colons, and commas", () => {
        const raw: [5]u8 = [0x00, ':', ',', 0xFF, 0x0A];
        let buf: [32]u8 = undefined;
        let n = encode(raw[0..5], buf[0..32]);
        expect_eq(n, 8);
        let f = decode(buf[0..8]);
        expect_eq(f.status, NS_OK);
        expect_eq(f.payload_len, 5);
        expect_eq(eqBytes(buf[f.payload_start .. f.payload_start + 5], raw[0..5]), true);
    });

    test("concatenated multi-frame stream decoding via consumed offset", () => {
        const stream: []const u8 = "3:foo,3:bar,0:,";
        let f1 = decode(stream);
        expect_eq(f1.status, NS_OK);
        expect_eq(eqBytes(stream[f1.payload_start .. f1.payload_start + f1.payload_len], "foo"), true);

        let rest1 = stream[f1.consumed..stream.len];
        let f2 = decode(rest1);
        expect_eq(f2.status, NS_OK);
        expect_eq(eqBytes(rest1[f2.payload_start .. f2.payload_start + f2.payload_len], "bar"), true);

        let rest2 = rest1[f2.consumed..rest1.len];
        let f3 = decode(rest2);
        expect_eq(f3.status, NS_OK);
        expect_eq(f3.payload_len, 0);
    });

    test("incomplete partial frames return NS_INCOMPLETE", () => {
        expect_eq(decode("").status, NS_INCOMPLETE);
        expect_eq(decode("12").status, NS_INCOMPLETE);
        expect_eq(decode("5:hel").status, NS_INCOMPLETE);
        expect_eq(decode("5:hello").status, NS_INCOMPLETE);
    });

    test("leading zeros, missing trailing comma, and undersized buffer fail safely", () => {
        expect_eq(decode("01:a,").status, NS_INVALID);
        expect_eq(decode(":abc,").status, NS_INVALID);
        expect_eq(decode("3:abc;").status, NS_INVALID);
        expect_eq(decode("abc").status, NS_INVALID);
        let tiny: [4]u8 = undefined;
        expect_eq(encode("hello", tiny[0..4]), -1);
    });
}
