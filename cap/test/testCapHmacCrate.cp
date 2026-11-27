import { test, expect_eq } from "cap:test";
import {
    HmacSha256, hmacSha256Hex, encodeHex32
} from "../../packages/hmac/src/index.cp";

fn main() {
    test("1. @cap/hmac-crate — RFC 4231 Test Case 2 (key='Jefe', data='what do ya want for nothing?')", () => {
        let hex: [64]u8 = [0; 64];
        let n = hmacSha256Hex("Jefe", "what do ya want for nothing?", hex[0..64]);
        expect_eq(n, 64);
        expect_eq(hex[0..64] as string, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    });

    test("2. @cap/hmac-crate — RFC 4231 Test Case 1 (20x 0x0b key, 'Hi There')", () => {
        let key: [20]u8 = [11; 20];
        let mac = HmacSha256.newFromSlice(key[0..20]);
        mac.updateStr("Hi There");
        let dig: [32]u8 = [0; 32];
        mac.finalize(dig[0..32]);
        let hex: [64]u8 = [0; 64];
        * = encodeHex32(dig[0..32], hex[0..64]);
        expect_eq(hex[0..64] as string, "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    });

    test("3. @cap/hmac-crate — streaming multi-chunk update matches one-shot", () => {
        let mac = HmacSha256.newFromKey("Jefe");
        mac.updateStr("what do ya ");
        mac.updateStr("want for ");
        mac.updateStr("nothing?");
        let dig: [32]u8 = [0; 32];
        mac.finalize(dig[0..32]);
        let hex: [64]u8 = [0; 64];
        * = encodeHex32(dig[0..32], hex[0..64]);
        expect_eq(hex[0..64] as string, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    });

    test("4. @cap/hmac-crate — constant-time verifySlice accepts valid tag and rejects tampered tag", () => {
        let m1 = HmacSha256.newFromKey("secret-key");
        m1.updateStr("payload");
        let tag: [32]u8 = [0; 32];
        m1.finalize(tag[0..32]);

        let m2 = HmacSha256.newFromKey("secret-key");
        m2.updateStr("payload");
        expect_eq(m2.verifySlice(tag[0..32]), true);

        tag[0] = tag[0] ^ 1;
        let m3 = HmacSha256.newFromKey("secret-key");
        m3.updateStr("payload");
        expect_eq(m3.verifySlice(tag[0..32]), false);
    });

    test("5. @cap/hmac-crate — RFC 4231 Test Case 6 (131-byte key > block size 64)", () => {
        let long_key: [131]u8 = [170; 131]; // 0xaa * 131
        let mac = HmacSha256.newFromSlice(long_key[0..131]);
        mac.updateStr("Test Using Larger Than Block-Size Key - Hash Key First");
        let dig: [32]u8 = [0; 32];
        mac.finalize(dig[0..32]);
        let hex: [64]u8 = [0; 64];
        * = encodeHex32(dig[0..32], hex[0..64]);
        expect_eq(hex[0..64] as string, "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
    });

    test("6. @cap/hmac-crate — verifySlice rejects wrong-length slice", () => {
        let mac = HmacSha256.newFromKey("k");
        let short_tag: [16]u8 = [0; 16];
        expect_eq(mac.verifySlice(short_tag[0..16]), false);
    });
}
