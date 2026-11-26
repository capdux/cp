import { test, expect_eq } from "cap:test";
import {
    UnsignResult,
    sha256,
    hmacSha256,
    ctEqual,
    sign,
    unsign,
} from "../../packages/@cap/cookie-signature/src/index.cp";

fn bytesEqStr(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/cookie-signature — SHA-256 & HMAC-SHA256 RFC 4231 test vector", () => {
        // SHA-256("abc") = ba7816bf 8f01cfea ... f20015ad
        let digest: [32]u8 = [0; 32];
        sha256("abc", digest[0..32]);
        expect_eq(digest[0], 0xba as u8);
        expect_eq(digest[1], 0x78 as u8);
        expect_eq(digest[2], 0x16 as u8);
        expect_eq(digest[3], 0xbf as u8);
        expect_eq(digest[30], 0x15 as u8);
        expect_eq(digest[31], 0xad as u8);

        // RFC 4231 Test Case 2: key="Jefe", data="what do ya want for nothing?"
        // Expected: 5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843
        let mac: [32]u8 = [0; 32];
        hmacSha256("Jefe", "what do ya want for nothing?", mac[0..32]);
        expect_eq(mac[0], 0x5b as u8);
        expect_eq(mac[1], 0xdc as u8);
        expect_eq(mac[2], 0xc1 as u8);
        expect_eq(mac[3], 0x46 as u8);
        expect_eq(mac[30], 0x38 as u8);
        expect_eq(mac[31], 0x43 as u8);
    });

    test("2. @cap/cookie-signature — npm cookie-signature canonical sign('hello', 'tobiiscool')", () => {
        let out: [128]u8 = [0; 128];
        let n: usize = sign("hello", "tobiiscool", out[0..128]);
        expect_eq(bytesEqStr(out[0..128], n, "hello.DGDUkGlIkCzPz+C0B064FNgHdEjox7ch8tOBGslZ5QI"), true);
    });

    test("3. @cap/cookie-signature — unsign valid cookie including dotted values", () => {
        let out: [128]u8 = [0; 128];
        let n1: usize = sign("hello", "tobiiscool", out[0..128]);
        let r1: UnsignResult = unsign(out[0..n1], "tobiiscool");
        expect_eq(r1.ok, true);
        expect_eq(r1.val_len, 5 as usize);
        expect_eq(bytesEqStr(out[0..128], r1.val_len, "hello"), true);

        let n2: usize = sign("user.123.abc", "my-secret-key", out[0..128]);
        let r2: UnsignResult = unsign(out[0..n2], "my-secret-key");
        expect_eq(r2.ok, true);
        expect_eq(r2.val_len, 12 as usize);
        expect_eq(bytesEqStr(out[0..128], r2.val_len, "user.123.abc"), true);
    });

    test("4. @cap/cookie-signature — rejects modified payload, modified sig & wrong secret", () => {
        let out: [128]u8 = [0; 128];
        let n: usize = sign("session_42", "top-secret", out[0..128]);

        // Wrong secret
        expect_eq(unsign(out[0..n], "wrong-secret").ok, false);

        // Modified payload byte
        out[0] = 88; // 'X'
        expect_eq(unsign(out[0..n], "top-secret").ok, false);
        out[0] = 115; // restore 's'
        expect_eq(unsign(out[0..n], "top-secret").ok, true);

        // Modified signature byte
        out[n - 1] = out[n - 1] ^ 1;
        expect_eq(unsign(out[0..n], "top-secret").ok, false);
    });

    test("5. @cap/cookie-signature — ctEqual constant-time comparison", () => {
        expect_eq(ctEqual("abcdef", "abcdef"), true);
        expect_eq(ctEqual("abcdef", "abcdeg"), false);
        expect_eq(ctEqual("abc", "abcd"), false);
        expect_eq(ctEqual("", ""), true);
    });

    test("6. @cap/cookie-signature — empty input, missing dot & tiny buffer safety", () => {
        let out: [128]u8 = [0; 128];
        let n0: usize = sign("", "secret", out[0..128]);
        expect_eq(n0, 44 as usize); // "" + "." + 43-char base64
        let r0: UnsignResult = unsign(out[0..n0], "secret");
        expect_eq(r0.ok, true);
        expect_eq(r0.val_len, 0 as usize);

        expect_eq(unsign("nodothere", "secret").ok, false);
        expect_eq(unsign("", "secret").ok, false);

        let tiny: [10]u8 = [0; 10];
        expect_eq(sign("hello", "tobiiscool", tiny[0..10]), 0 as usize);
    });
}
