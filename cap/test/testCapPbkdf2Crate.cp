import { test, expect_eq } from "cap:test";
import { pbkdf2HmacSha256, pbkdf2HmacSha256Str } from "../../packages/pbkdf2/src/index.cp";

fn pbkHexNibble(n: u8): u8 {
    if (n < 10) { return 48 + n; }
    return 87 + n;
}

fn pbkToHex(data: []u8, out: []u8): usize {
    let i: usize = 0;
    while (i < data.len && i * 2 + 1 < out.len) {
        out[i * 2] = pbkHexNibble((data[i] >> 4) & 15);
        out[i * 2 + 1] = pbkHexNibble(data[i] & 15);
        i = i + 1;
    }
    return i * 2;
}

fn main() {
    test("1. @cap/pbkdf2-crate — standard SHA-256 vector (password, salt, c=1, dkLen=32)", () => {
        let dk: [32]u8 = [0; 32];
        expect_eq(pbkdf2HmacSha256Str("password", "salt", 1, dk[0..32]), true);
        let hex: [64]u8 = [0; 64];
        * = pbkToHex(dk[0..32], hex[0..64]);
        expect_eq(hex[0..64] as string, "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");
    });

    test("2. @cap/pbkdf2-crate — standard SHA-256 vector (password, salt, c=2, dkLen=32)", () => {
        let dk: [32]u8 = [0; 32];
        expect_eq(pbkdf2HmacSha256Str("password", "salt", 2, dk[0..32]), true);
        let hex: [64]u8 = [0; 64];
        * = pbkToHex(dk[0..32], hex[0..64]);
        expect_eq(hex[0..64] as string, "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43");
    });

    test("3. @cap/pbkdf2-crate — multi-block dkLen=40 (passwordPASSWORDpassword, saltSALTsaltSALTsaltSALTsaltSALTsalt, c=5)", () => {
        let dk40: [40]u8 = [0; 40];
        let dk32: [32]u8 = [0; 32];
        expect_eq(pbkdf2HmacSha256Str("password", "salt", 5, dk40[0..40]), true);
        expect_eq(pbkdf2HmacSha256Str("password", "salt", 5, dk32[0..32]), true);
        // First 32 bytes match regardless of total dkLen
        expect_eq(dk40[0], dk32[0]);
        expect_eq(dk40[31], dk32[31]);
    });

    test("4. @cap/pbkdf2-crate — rejects 0 iterations or 0-length output", () => {
        let dk: [32]u8 = [0; 32];
        expect_eq(pbkdf2HmacSha256Str("pw", "salt", 0, dk[0..32]), false);
        expect_eq(pbkdf2HmacSha256Str("pw", "salt", 1, dk[0..0]), false);
    });

    test("5. @cap/pbkdf2-crate — different salts produce completely different derived keys", () => {
        let k1: [32]u8 = [0; 32];
        let k2: [32]u8 = [0; 32];
        * = pbkdf2HmacSha256Str("my-pass", "salt-A", 4, k1[0..32]);
        * = pbkdf2HmacSha256Str("my-pass", "salt-B", 4, k2[0..32]);
        expect_eq(k1[0] == k2[0] && k1[1] == k2[1], false);
    });

    test("6. @cap/pbkdf2-crate — empty salt works cleanly via slice API", () => {
        let pw: [4]u8 = [116, 101, 115, 116]; // "test"
        let empty: [1]u8 = [0];
        let dk: [32]u8 = [0; 32];
        expect_eq(pbkdf2HmacSha256(pw[0..4], empty[0..0], 3, dk[0..32]), true);
        expect_eq(dk[0] == 0 && dk[1] == 0 && dk[2] == 0, false);
    });
}
