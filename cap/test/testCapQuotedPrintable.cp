import { test, expect_eq } from "cap:test";
import { encode, decode } from "../../packages/quoted-printable/src/index.cp";

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

fn containsSlice(haystack: []const u8, needle: []const u8) -> bool {
    if (needle.len == 0 || needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (testEq(haystack[i..i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn main() {
    test("1. @cap/quoted-printable — Plain ASCII & '=' -> '=3D'", () => {
        let buf: [64]u8 = undefined;
        const n = encode("a=b+c", buf[0..64]);
        expect_eq(testEq(buf[0..n], "a=3Db+c"), true);

        let dec_buf: [64]u8 = undefined;
        const dn = decode(buf[0..n], dec_buf[0..64]);
        expect_eq(testEq(dec_buf[0..dn as usize], "a=b+c"), true);
    });

    test("2. @cap/quoted-printable — UTF-8 Chinese/German =XX encoding & roundtrip", () => {
        const text = "你好, Grüße!";
        let enc: [128]u8 = undefined;
        let dec_buf: [128]u8 = undefined;

        const en = encode(text, enc[0..128]);
        expect_eq(en > text.len, true);
        expect_eq(containsSlice(enc[0..en], "=E4=BD=A0=E5=A5=BD"), true);
        expect_eq(containsSlice(enc[0..en], "=C3=BC=C3=9F"), true);

        const dn = decode(enc[0..en], dec_buf[0..128]);
        expect_eq(dn, text.len as i64);
        expect_eq(testEq(dec_buf[0..dn as usize], text), true);
    });

    test("3. @cap/quoted-printable — 76-column soft line break =\\r\\n & lossless decode", () => {
        // 100 'A' characters must trigger a soft line break at <= 76 chars
        let src: [100]u8 = undefined;
        let i: usize = 0;
        while (i < 100) {
            src[i] = 65; // 'A'
            i += 1;
        }

        let enc: [256]u8 = undefined;
        const en = encode(src[0..100], enc[0..256]);
        expect_eq(containsSlice(enc[0..en], "=\r\n"), true);

        // Verify no encoded line exceeds 76 chars
        let line_len: usize = 0;
        let k: usize = 0;
        while (k < en) {
            if (enc[k] == 13 && k + 1 < en && enc[k + 1] == 10) {
                expect_eq(line_len <= 76, true);
                line_len = 0;
                k += 2;
            } else {
                line_len += 1;
                k += 1;
            }
        }
        expect_eq(line_len <= 76, true);

        let dec_buf: [128]u8 = undefined;
        const dn = decode(enc[0..en], dec_buf[0..128]);
        expect_eq(dn, 100 as i64);
        expect_eq(testEq(dec_buf[0..100], src[0..100]), true);
    });

    test("4. @cap/quoted-printable — Trailing space & tab encoded as =20 / =09", () => {
        let enc: [64]u8 = undefined;
        const n1 = encode("hello ", enc[0..64]);
        expect_eq(testEq(enc[0..n1], "hello=20"), true);

        const n2 = encode("hello\t\r\nworld ", enc[0..64]);
        expect_eq(testEq(enc[0..n2], "hello=09\r\nworld=20"), true);
    });

    test("5. @cap/quoted-printable — Decode supports both =\\r\\n and =\\n soft breaks & lowercase hex", () => {
        const qp = "Hello =\r\nWorld =\nfrom =63=70!";
        let dec_buf: [64]u8 = undefined;
        const dn = decode(qp, dec_buf[0..64]);
        expect_eq(testEq(dec_buf[0..dn as usize], "Hello World from cp!"), true);
    });

    test("6. @cap/quoted-printable — Invalid =ZZ escape & undersized buffer protection", () => {
        let buf: [64]u8 = undefined;
        expect_eq(decode("bad=ZZseq", buf[0..64]), -1 as i64);
        expect_eq(decode("trailing=", buf[0..64]), -1 as i64);
        expect_eq(encode("", buf[0..64]), 0 as usize);

        let tiny: [3]u8 = undefined;
        expect_eq(encode("hello world", tiny[0..3]), 0 as usize);
        expect_eq(decode("hello world", tiny[0..3]), -1 as i64);
    });
}
