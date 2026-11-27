import { test, expect_eq } from "cap:test";
import { Aes128Gcm, Aes256Gcm } from "../../packages/aes-gcm/src/index.cp";

fn hexNib(c: u8): u8 {
    if (c >= '0' && c <= '9') { return c - '0'; }
    if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
    return c - 'A' + 10;
}

fn fromHex(hex: string, out: []u8): usize {
    const n = hex.len / 2;
    let i: usize = 0;
    while (i < n) : (i += 1) {
        out[i] = (hexNib(hex[i * 2]) << 4) | hexNib(hex[i * 2 + 1]);
    }
    return n;
}

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) { return false; }
    }
    return true;
}

fn main() {
    test("1. @cap/aes-gcm — NIST SP 800-38D AES-128-GCM Test Case 1 (empty plaintext & AAD)", () => {
        const z: u8 = 0;
        const key: [16]u8 = [z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z];
        const nonce: [12]u8 = [z, z, z, z, z, z, z, z, z, z, z, z];
        const gcm = Aes128Gcm.new(key);

        let ct: [1]u8 = [z];
        let tag = gcm.encrypt(nonce, "", "", ct[0..0]);

        let exp_tag: [16]u8 = undefined;
        * = fromHex("58e2fccefa7e3061367f1d57a4e7455a", exp_tag[0..16]);
        expect_eq(eqBytes(tag[0..16], exp_tag[0..16]), true);

        let pt: [1]u8 = [z];
        expect_eq(gcm.decrypt(nonce, "", "", tag, pt[0..0]), true);
    });

    test("2. @cap/aes-gcm — NIST SP 800-38D AES-128-GCM Test Case 2 (16B zero plaintext)", () => {
        const z: u8 = 0;
        const key: [16]u8 = [z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z];
        const nonce: [12]u8 = [z, z, z, z, z, z, z, z, z, z, z, z];
        const pt_in: [16]u8 = [z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z];
        const gcm = Aes128Gcm.new(key);

        let ct: [16]u8 = undefined;
        let tag = gcm.encrypt(nonce, "", pt_in[0..16], ct[0..16]);

        let exp_ct: [16]u8 = undefined;
        let exp_tag: [16]u8 = undefined;
        * = fromHex("0388dace60b6a392f328c2b971b2fe78", exp_ct[0..16]);
        * = fromHex("ab6e47d42cec13bdf53a67b21257bddf", exp_tag[0..16]);
        expect_eq(eqBytes(ct[0..16], exp_ct[0..16]), true);
        expect_eq(eqBytes(tag[0..16], exp_tag[0..16]), true);

        let pt_out: [16]u8 = undefined;
        expect_eq(gcm.decrypt(nonce, "", ct[0..16], tag, pt_out[0..16]), true);
        expect_eq(eqBytes(pt_out[0..16], pt_in[0..16]), true);
    });

    test("3. @cap/aes-gcm — NIST SP 800-38D AES-128-GCM Test Case 4 (60B PT + 20B AAD)", () => {
        let key: [16]u8 = undefined;
        let nonce: [12]u8 = undefined;
        let aad: [20]u8 = undefined;
        let pt: [60]u8 = undefined;
        let exp_ct: [60]u8 = undefined;
        let exp_tag: [16]u8 = undefined;

        * = fromHex("feffe9928665731c6d6a8f9467308308", key[0..16]);
        * = fromHex("cafebabefacedbaddecaf888", nonce[0..12]);
        * = fromHex("feedfacedeadbeeffeedfacedeadbeefabaddad2", aad[0..20]);
        * = fromHex("d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39", pt[0..60]);
        * = fromHex("42831ec2217774244b7221b784d0d49ce3aa212f2c02a4e035c17e2329aca12e21d514b25466931c7d8f6a5aac84aa051ba30b396a0aac973d58e091", exp_ct[0..60]);
        * = fromHex("5bc94fbc3221a5db94fae95ae7121a47", exp_tag[0..16]);

        const gcm = Aes128Gcm.new(key);
        let ct: [60]u8 = undefined;
        let tag = gcm.encrypt(nonce, aad[0..20], pt[0..60], ct[0..60]);
        expect_eq(eqBytes(ct[0..60], exp_ct[0..60]), true);
        expect_eq(eqBytes(tag[0..16], exp_tag[0..16]), true);

        let dec_pt: [60]u8 = undefined;
        expect_eq(gcm.decrypt(nonce, aad[0..20], ct[0..60], tag, dec_pt[0..60]), true);
        expect_eq(eqBytes(dec_pt[0..60], pt[0..60]), true);
    });

    test("4. @cap/aes-gcm — NIST SP 800-38D AES-256-GCM Test Case 13 & 14 (zero key/IV)", () => {
        const z: u8 = 0;
        let key: [32]u8 = undefined;
        let i: usize = 0;
        while (i < 32) : (i += 1) { key[i] = 0; }
        const nonce: [12]u8 = [z, z, z, z, z, z, z, z, z, z, z, z];
        const gcm = Aes256Gcm.new(key);

        let dummy: [1]u8 = [z];
        let tag13 = gcm.encrypt(nonce, "", "", dummy[0..0]);
        let exp13: [16]u8 = undefined;
        * = fromHex("530f8afbc74536b9a963b4f1c4cb738b", exp13[0..16]);
        expect_eq(eqBytes(tag13[0..16], exp13[0..16]), true);

        const pt14: [16]u8 = [z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z];
        let ct14: [16]u8 = undefined;
        let tag14 = gcm.encrypt(nonce, "", pt14[0..16], ct14[0..16]);
        let exp_ct14: [16]u8 = undefined;
        let exp_tag14: [16]u8 = undefined;
        * = fromHex("cea7403d4d606b6e074ec5d3baf39d18", exp_ct14[0..16]);
        * = fromHex("d0d1c8a799996bf0265b98b5d48ab919", exp_tag14[0..16]);
        expect_eq(eqBytes(ct14[0..16], exp_ct14[0..16]), true);
        expect_eq(eqBytes(tag14[0..16], exp_tag14[0..16]), true);
    });

    test("5. @cap/aes-gcm — NIST SP 800-38D AES-256-GCM Test Case 16 (60B PT + 20B AAD)", () => {
        let key: [32]u8 = undefined;
        let nonce: [12]u8 = undefined;
        let aad: [20]u8 = undefined;
        let pt: [60]u8 = undefined;
        let exp_ct: [60]u8 = undefined;
        let exp_tag: [16]u8 = undefined;

        * = fromHex("feffe9928665731c6d6a8f9467308308feffe9928665731c6d6a8f9467308308", key[0..32]);
        * = fromHex("cafebabefacedbaddecaf888", nonce[0..12]);
        * = fromHex("feedfacedeadbeeffeedfacedeadbeefabaddad2", aad[0..20]);
        * = fromHex("d9313225f88406e5a55909c5aff5269a86a7a9531534f7da2e4c303d8a318a721c3c0c95956809532fcf0e2449a6b525b16aedf5aa0de657ba637b39", pt[0..60]);
        * = fromHex("522dc1f099567d07f47f37a32a84427d643a8cdcbfe5c0c97598a2bd2555d1aa8cb08e48590dbb3da7b08b1056828838c5f61e6393ba7a0abcc9f662", exp_ct[0..60]);
        * = fromHex("76fc6ece0f4e1768cddf8853bb2d551b", exp_tag[0..16]);

        const gcm = Aes256Gcm.new(key);
        let ct: [60]u8 = undefined;
        let tag = gcm.encrypt(nonce, aad[0..20], pt[0..60], ct[0..60]);
        expect_eq(eqBytes(ct[0..60], exp_ct[0..60]), true);
        expect_eq(eqBytes(tag[0..16], exp_tag[0..16]), true);

        let dec_pt: [60]u8 = undefined;
        expect_eq(gcm.decrypt(nonce, aad[0..20], ct[0..60], tag, dec_pt[0..60]), true);
        expect_eq(eqBytes(dec_pt[0..60], pt[0..60]), true);
    });

    test("6. @cap/aes-gcm — tamper detection on ciphertext, AAD, and tag", () => {
        let key: [16]u8 = undefined;
        let nonce: [12]u8 = undefined;
        * = fromHex("000102030405060708090a0b0c0d0e0f", key[0..16]);
        * = fromHex("101112131415161718191a1b", nonce[0..12]);
        const gcm = Aes128Gcm.new(key);

        const msg = "hello authenticated cp";
        const aad = "header-v1";
        let ct: [22]u8 = undefined;
        let tag = gcm.encrypt(nonce, aad, msg, ct[0..22]);
        let pt: [22]u8 = undefined;

        expect_eq(gcm.decrypt(nonce, aad, ct[0..22], tag, pt[0..22]), true);

        // Flip 1 bit in ciphertext
        ct[5] = ct[5] ^ 0x01;
        expect_eq(gcm.decrypt(nonce, aad, ct[0..22], tag, pt[0..22]), false);
        ct[5] = ct[5] ^ 0x01;

        // Modify AAD
        expect_eq(gcm.decrypt(nonce, "header-v2", ct[0..22], tag, pt[0..22]), false);

        // Flip 1 bit in tag
        tag[15] = tag[15] ^ 0x80;
        expect_eq(gcm.decrypt(nonce, aad, ct[0..22], tag, pt[0..22]), false);
    });
}
