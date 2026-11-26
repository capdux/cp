import { test, expect_eq } from "cap:test";
import {
    LiteralKind,
    ParsedLiteral,
    parseLiteral,
    unescapeString,
} from "../../packages/@cap/litrs/src/index.cp";

fn eqStr(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

fn main() {
    test("1. @cap/litrs — booleans & invalid inputs", () => {
        const t = parseLiteral("true");
        expect_eq(t.kind == LiteralKind.Bool, true);
        expect_eq(t.bool_val, true);

        const f = parseLiteral("false");
        expect_eq(f.kind == LiteralKind.Bool, true);
        expect_eq(f.bool_val, false);

        expect_eq(parseLiteral("").kind == LiteralKind.Invalid, true);
        expect_eq(parseLiteral("foobar").kind == LiteralKind.Invalid, true);
    });

    test("2. @cap/litrs — integer radices (dec/hex/bin/oct), underscores & suffixes", () => {
        const d = parseLiteral("1_000_000usize");
        expect_eq(d.kind == LiteralKind.Integer, true);
        expect_eq(d.int_val, 1000000);
        expect_eq(eqStr(d.suffix, "usize"), true);

        const h = parseLiteral("0xFF_0Au32");
        expect_eq(h.kind == LiteralKind.Integer, true);
        expect_eq(h.int_val, 0xFF0A);
        expect_eq(eqStr(h.suffix, "u32"), true);

        const b = parseLiteral("0b1010_0101u8");
        expect_eq(b.kind == LiteralKind.Integer, true);
        expect_eq(b.int_val, 0xA5);
        expect_eq(eqStr(b.suffix, "u8"), true);

        const o = parseLiteral("0o755i32");
        expect_eq(o.kind == LiteralKind.Integer, true);
        expect_eq(o.int_val, 493);
        expect_eq(eqStr(o.suffix, "i32"), true);

        expect_eq(parseLiteral("0x").kind == LiteralKind.Invalid, true);
        expect_eq(parseLiteral("0b2").kind == LiteralKind.Invalid, true);
    });

    test("3. @cap/litrs — float literals with fraction, exponent & f32/f64 suffix", () => {
        const f1 = parseLiteral("3.14_15f64");
        expect_eq(f1.kind == LiteralKind.Float, true);
        expect_eq(eqStr(f1.suffix, "f64"), true);

        const f2 = parseLiteral("1e-9f32");
        expect_eq(f2.kind == LiteralKind.Float, true);
        expect_eq(eqStr(f2.suffix, "f32"), true);

        const f3 = parseLiteral("2.5E+3");
        expect_eq(f3.kind == LiteralKind.Float, true);
        expect_eq(f3.suffix.len, 0);

        const f4 = parseLiteral("42f32");
        expect_eq(f4.kind == LiteralKind.Float, true);
        expect_eq(eqStr(f4.suffix, "f32"), true);

        expect_eq(parseLiteral("1e").kind == LiteralKind.Invalid, true);
    });

    test("4. @cap/litrs — char & byte literals with escapes", () => {
        expect_eq(parseLiteral("'a'").kind == LiteralKind.Char, true);
        expect_eq(parseLiteral("'\\n'").kind == LiteralKind.Char, true);
        expect_eq(parseLiteral("'\\x7F'").kind == LiteralKind.Char, true);
        expect_eq(parseLiteral("'\\u{1F600}'").kind == LiteralKind.Char, true);
        expect_eq(parseLiteral("b'A'").kind == LiteralKind.Byte, true);
        expect_eq(parseLiteral("b'\\xFF'").kind == LiteralKind.Byte, true);

        expect_eq(parseLiteral("''").kind == LiteralKind.Invalid, true);
        expect_eq(parseLiteral("'ab'").kind == LiteralKind.Invalid, true);
    });

    test("5. @cap/litrs — normal & raw strings and byte strings", () => {
        const s = parseLiteral("\"hello \\\"cp\\\"\"");
        expect_eq(s.kind == LiteralKind.String, true);
        expect_eq(s.is_raw_string, false);

        const rs = parseLiteral("r##\"raw \"# quote\"##");
        expect_eq(rs.kind == LiteralKind.String, true);
        expect_eq(rs.is_raw_string, true);
        expect_eq(rs.hash_count, 2);

        const bs = parseLiteral("b\"bytes\"");
        expect_eq(bs.kind == LiteralKind.ByteString, true);
        expect_eq(bs.is_raw_string, false);

        const rbs = parseLiteral("br#\"raw\\nbytes\"#");
        expect_eq(rbs.kind == LiteralKind.ByteString, true);
        expect_eq(rbs.is_raw_string, true);
        expect_eq(rbs.hash_count, 1);

        expect_eq(parseLiteral("\"unterminated").kind == LiteralKind.Invalid, true);
    });

    test("6. @cap/litrs — unescapeString for escapes, unicode & raw strings", () => {
        let buf: [64]u8 = undefined;

        const n1 = unescapeString("\"hi\\n\\t\\x21\"", buf[0..64]);
        expect_eq(n1, 5);
        expect_eq(eqStr(buf[0..usize(n1)], "hi\n\t!"), true);

        const n2 = unescapeString("\"\\u{41}\\u{42}\"", buf[0..64]);
        expect_eq(n2, 2);
        expect_eq(eqStr(buf[0..usize(n2)], "AB"), true);

        const n3 = unescapeString("r#\"keep\\nraw\"#", buf[0..64]);
        expect_eq(n3, 9);
        expect_eq(eqStr(buf[0..usize(n3)], "keep\\nraw"), true);

        const n4 = unescapeString("br\"a\\tb\"", buf[0..64]);
        expect_eq(n4, 4);
        expect_eq(eqStr(buf[0..usize(n4)], "a\\tb"), true);

        expect_eq(unescapeString("123", buf[0..64]), -1);
    });
}
