import { test, expect_eq } from "cap:test";
import { encode, encodeBinary, decode, decodeLossy, encodedLen, isUnreserved } from "../../packages/@cap/urlencoding/src/index.cp";

fn main() {
    test("isUnreserved matches RFC 3986 unreserved set", () => {
        expect_eq(isUnreserved('A'), true);
        expect_eq(isUnreserved('Z'), true);
        expect_eq(isUnreserved('a'), true);
        expect_eq(isUnreserved('z'), true);
        expect_eq(isUnreserved('0'), true);
        expect_eq(isUnreserved('9'), true);
        expect_eq(isUnreserved('-'), true);
        expect_eq(isUnreserved('_'), true);
        expect_eq(isUnreserved('.'), true);
        expect_eq(isUnreserved('~'), true);

        expect_eq(isUnreserved(' '), false);
        expect_eq(isUnreserved('/'), false);
        expect_eq(isUnreserved('%'), false);
        expect_eq(isUnreserved('+'), false);
        expect_eq(isUnreserved('!'), false);
        expect_eq(isUnreserved(0xFF), false);
    });

    test("encode and encodedLen handle ASCII, spaces, and UTF-8", () => {
        const s1 = "hello world! a-z_0.9~";
        const expected1 = "hello%20world%21%20a-z_0.9~";
        expect_eq(encodedLen(s1), expected1.len);

        let buf: [128]u8 = undefined;
        let n = encode(s1, buf[0..128]);
        expect_eq(n, expected1.len);
        expect_eq(std.mem.eql(u8, buf[0..n], expected1), true);

        const s2 = "This string will be URL encoded.";
        const expected2 = "This%20string%20will%20be%20URL%20encoded.";
        n = encode(s2, buf[0..128]);
        expect_eq(std.mem.eql(u8, buf[0..n], expected2), true);

        // UTF-8 multi-byte ("α" = 0xCE 0xB1)
        const utf8_in: [2]u8 = [0xCE, 0xB1];
        n = encodeBinary(utf8_in[0..2], buf[0..128]);
        expect_eq(std.mem.eql(u8, buf[0..n], "%CE%B1"), true);
    });

    test("encodeBinary handles NUL, high bytes, and buffer limits", () => {
        const raw: [4]u8 = [0x00, 0x0A, 0x7F, 0xFF];
        let buf: [32]u8 = undefined;
        let n = encodeBinary(raw[0..4], buf[0..32]);
        expect_eq(std.mem.eql(u8, buf[0..n], "%00%0A%7F%FF"), true);

        // Truncated buffer stops cleanly without partial %X
        let small: [5]u8 = undefined;
        n = encodeBinary(raw[0..4], small[0..5]);
        expect_eq(n, 3);
        expect_eq(std.mem.eql(u8, small[0..n], "%00"), true);
    });

    test("decode handles upper/lower hex and roundtrip", () => {
        let enc_buf: [128]u8 = undefined;
        let dec_buf: [128]u8 = undefined;

        const orig = "key=val&path=/a/b?c=100% #frag";
        const enc_len = encode(orig, enc_buf[0..128]);
        const dec_len = decode(enc_buf[0..enc_len], dec_buf[0..128]);
        expect_eq(dec_len, orig.len as i64);
        expect_eq(std.mem.eql(u8, dec_buf[0..(dec_len as usize)], orig), true);

        // Mixed-case hex decoding
        const mixed = "%68%65%6c%6C%6f%20%77%6F%72%6c%64";
        const mlen = decode(mixed, dec_buf[0..128]);
        expect_eq(mlen, 11);
        expect_eq(std.mem.eql(u8, dec_buf[0..11], "hello world"), true);
    });

    test("decode rejects malformed escapes and insufficient buffer with -1", () => {
        let buf: [16]u8 = undefined;
        expect_eq(decode("%", buf[0..16]), -1);
        expect_eq(decode("abc%2", buf[0..16]), -1);
        expect_eq(decode("%ZZ", buf[0..16]), -1);
        expect_eq(decode("%2G", buf[0..16]), -1);
        expect_eq(decode("100% ok", buf[0..16]), -1);

        let tiny: [3]u8 = undefined;
        expect_eq(decode("abcd", tiny[0..3]), -1);
        expect_eq(decode("ab%20c", tiny[0..3]), -1);
    });

    test("decodeLossy decodes valid escapes and passes invalid % verbatim", () => {
        let buf: [64]u8 = undefined;
        let n = decodeLossy("100%25%20ok%ZZ%2", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "100% ok%ZZ%2"), true);

        n = decodeLossy("trailing%", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "trailing%"), true);

        n = decodeLossy("%41%42%43", buf[0..64]);
        expect_eq(std.mem.eql(u8, buf[0..n], "ABC"), true);
    });
}
