import {
    encodeQuotedPrintable,
    decodeQuotedPrintable,
} from "cap:mail";
import { test, expect } from "cap:test";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("encodeQuotedPrintable and decodeQuotedPrintable UTF-8 & trailing space round-trip", () => {
    let enc_buf: [256]u8 = [0; 256];
    let dec_buf: [256]u8 = [0; 256];

    const src = "Price = 100€ ";
    const enc = encodeQuotedPrintable(src, enc_buf[0..]).?;
    // '=' -> '=3D', '€' -> '=E2=82=AC', trailing ' ' -> '=20'
    expect(strEq(enc, "Price =3D 100=E2=82=AC=20"));
    expect(strEq(decodeQuotedPrintable(enc, dec_buf[0..], true).?, src));
});

test("soft line breaks, 76-col wrapping, and strict vs lenient decoding", () => {
    let enc_buf: [256]u8 = [0; 256];
    let dec_buf: [256]u8 = [0; 256];

    const long_line = "01234567890123456789012345678901234567890123456789012345678901234567890123456789";
    const enc = encodeQuotedPrintable(long_line, enc_buf[0..]).?;
    expect(enc.len > long_line.len); // soft line break inserted
    expect(strEq(decodeQuotedPrintable(enc, dec_buf[0..], true).?, long_line));

    // Soft line break with LF and trailing spaces
    expect(strEq(decodeQuotedPrintable("hello= \nworld", dec_buf[0..], true).?, "helloworld"));

    // Malformed =ZZ
    expect(decodeQuotedPrintable("a=ZZb", dec_buf[0..], true) == null);
    expect(strEq(decodeQuotedPrintable("a=ZZb", dec_buf[0..], false).?, "a=ZZb"));
});
