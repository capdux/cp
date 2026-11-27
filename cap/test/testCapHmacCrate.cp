import { test, expect_eq } from "cap:test";
import {
    HmacSha256,
    HmacSha512,
    hmacSha256Bytes,
    hmacSha256Hex,
    hmacSha512,
    hmacSha512Hex,
    encodeHex32,
    encodeHex64,
    ctEqualBytes,
} from "../../packages/hmac/src/index.cp";

fn main() {
    test("1. hmac — SHA256 RFC 4231 Test Case 2 (key='Jefe', data='what do ya want for nothing?')", () => {
        let hex: [64]u8 = undefined;
        const n = hmacSha256Hex("Jefe", "what do ya want for nothing?", hex[0..64]);
        expect_eq(n, 64 as usize);
        expect_eq(hex[0..64] as string, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    });

    test("2. hmac — SHA256 RFC 4231 Test Case 1 (20x 0x0b key, 'Hi There')", () => {
        let key: [20]u8 = [11; 20];
        let mac = HmacSha256.newFromSlice(key[0..20]);
        mac.updateStr("Hi There");
        let dig: [32]u8 = undefined;
        mac.finalize(dig[0..32]);
        let hex: [64]u8 = undefined;
        * = encodeHex32(dig[0..32], hex[0..64]);
        expect_eq(hex[0..64] as string, "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
    });

    test("3. hmac — SHA256 streaming multi-chunk update matches one-shot", () => {
        let mac = HmacSha256.newFromKey("Jefe");
        mac.updateStr("what do ya ");
        mac.updateStr("want for ");
        mac.updateStr("nothing?");
        let dig: [32]u8 = undefined;
        mac.finalize(dig[0..32]);
        let hex: [64]u8 = undefined;
        * = encodeHex32(dig[0..32], hex[0..64]);
        expect_eq(hex[0..64] as string, "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
    });

    test("4. hmac — SHA256 constant-time verifySlice accepts valid tag and rejects tampered tag", () => {
        let m1 = HmacSha256.newFromKey("secret-key");
        m1.updateStr("payload");
        let tag: [32]u8 = undefined;
        m1.finalize(tag[0..32]);

        let m2 = HmacSha256.newFromKey("secret-key");
        m2.updateStr("payload");
        expect_eq(m2.verifySlice(tag[0..32]), true);

        tag[0] = tag[0] ^ 1;
        let m3 = HmacSha256.newFromKey("secret-key");
        m3.updateStr("payload");
        expect_eq(m3.verifySlice(tag[0..32]), false);
    });

    test("5. hmac — SHA256 RFC 4231 Test Case 6 (131-byte key > block size 64)", () => {
        let long_key: [131]u8 = [170; 131]; // 0xaa * 131
        let mac = HmacSha256.newFromSlice(long_key[0..131]);
        mac.updateStr("Test Using Larger Than Block-Size Key - Hash Key First");
        let dig: [32]u8 = undefined;
        mac.finalize(dig[0..32]);
        let hex: [64]u8 = undefined;
        * = encodeHex32(dig[0..32], hex[0..64]);
        expect_eq(hex[0..64] as string, "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
    });

    test("6. hmac — verifySlice rejects wrong-length slice", () => {
        let mac = HmacSha256.newFromKey("k");
        let short_tag: [16]u8 = [0; 16];
        expect_eq(mac.verifySlice(short_tag[0..16]), false);
    });

    test("7. hmac — HmacSha512 RFC 4231 Test Cases 1, 2 & 6", () => {
        // Test Case 2
        let hex: [128]u8 = undefined;
        const n = hmacSha512Hex("Jefe", "what do ya want for nothing?", hex[0..128]);
        expect_eq(n, 128 as usize);
        expect_eq(
            hex[0..128] as string,
            "164b7a7bfcf819e2e395fbe73b56e0a387bd64222e831fd610270cd7ea2505549758bf75c05a994a6d034f65f8f0e6fdcaeab1a34d4a6b4b636e070a38bce737",
        );

        // Test Case 1 (20x 0x0b)
        const key1: [20]u8 = [11; 20];
        let dig: [64]u8 = undefined;
        hmacSha512(&key1, "Hi There", dig[0..64]);
        * = encodeHex64(dig[0..64], hex[0..128]);
        expect_eq(
            hex[0..128] as string,
            "87aa7cdea5ef619d4ff0b4241a1d6cb02379f4e2ce4ec2787ad0b30545e17cdedaa833b7d6b8a702038b274eaea3f4e4be9d914eeb61f1702e696c203a126854",
        );

        // Test Case 6 (131x 0xaa > 128-byte SHA512 block size)
        const key6: [131]u8 = [170; 131];
        let mac6 = HmacSha512.newFromSlice(&key6);
        mac6.updateStr("Test Using Larger Than Block-Size Key - Hash Key First");
        mac6.finalize(dig[0..64]);
        * = encodeHex64(dig[0..64], hex[0..128]);
        expect_eq(
            hex[0..128] as string,
            "80b24263c7c1a3ebb71493c1dd7be8b49b46d1f41b4aeec1121b013783f8f3526b56d037e05f2598bd0fd2215d6a1e5295e64f73f63f0aec8b915a985d786598",
        );
    });

    test("8. hmac — HmacSha512 streaming update, constant-time verifySlice & ctEqualBytes", () => {
        let m1 = HmacSha512.newFromKey("top-secret");
        m1.update("part1-");
        m1.updateStr("part2");
        let tag: [64]u8 = undefined;
        m1.finalize(tag[0..64]);

        let m2 = HmacSha512.newFromKey("top-secret");
        m2.updateStr("part1-part2");
        expect_eq(m2.verifySlice(tag[0..64]), true);

        tag[63] = tag[63] ^ 0x80;
        let m3 = HmacSha512.newFromKey("top-secret");
        m3.updateStr("part1-part2");
        expect_eq(m3.verifySlice(tag[0..64]), false);
        expect_eq(m3.verifySlice(tag[0..32]), false);

        expect_eq(ctEqualBytes("abcdef", "abcdef"), true);
        expect_eq(ctEqualBytes("abcdef", "abcdeg"), false);
        expect_eq(ctEqualBytes("abc", "abcd"), false);

        // One-shot hmacSha256Bytes raw bytes
        let d256: [32]u8 = undefined;
        hmacSha256Bytes("Jefe", "what do ya want for nothing?", d256[0..32]);
        let mv = HmacSha256.newFromKey("Jefe");
        mv.updateStr("what do ya want for nothing?");
        expect_eq(mv.verifyConstSlice(d256[0..32]), true);
    });
}
