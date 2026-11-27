// cap/test/testCapChacha20Crate.cp
// Comprehensive unit tests for @cap/chacha20-crate (RFC 8439 ChaCha20)

import { test, expect_eq } from "cap:test";
import {
    CHACHA20_KEY_LEN,
    CHACHA20_NONCE_LEN,
    CHACHA20_BLOCK_LEN,
    chacha20Block,
    ChaCha20Cipher,
} from "../../packages/chacha20/src/index.cp";

fn cc20TestHexNibble(ch: u8): u8 {
    if (ch >= ('0' as u8) && ch <= ('9' as u8)) {
        return ch - ('0' as u8);
    }
    return (ch - ('a' as u8)) + (10 as u8);
}

fn cc20TestMatchesHex(bytes: []u8, expected_hex: string): bool {
    if (bytes.len * (2 as usize) != expected_hex.len) {
        return false;
    }
    let i: usize = 0;
    while (i < bytes.len) {
        let hi: u8 = cc20TestHexNibble(expected_hex[i * (2 as usize)]);
        let lo: u8 = cc20TestHexNibble(expected_hex[i * (2 as usize) + (1 as usize)]);
        let exp: u8 = (hi << 4) | lo;
        if (bytes[i] != exp) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn cc20FillSeqKey(key: []u8) {
    let i: usize = 0;
    while (i < (32 as usize)) {
        key[i] = i as u8;
        i += 1;
    }
}

fn main() {
    test("1. @cap/chacha20-crate — RFC 8439 Section 2.3.2 block vector", () => {
        expect_eq(CHACHA20_KEY_LEN, 32 as usize);
        expect_eq(CHACHA20_NONCE_LEN, 12 as usize);
        expect_eq(CHACHA20_BLOCK_LEN, 64 as usize);

        let key: [32]u8 = undefined;
        cc20FillSeqKey(key[0..32]);

        let nonce: [12]u8 = [
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x09 as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x4a as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        ];

        let out: [64]u8 = undefined;
        let ok = chacha20Block(key[0..32], 1 as u32, nonce[0..12], out[0..64]);
        expect_eq(ok, true);

        let exp = "10f1e7e4d13b5915500fdd1fa32071c4c7d1f4c733c068030422aa9ac3d46c4ed2826446079faa0914c2d705d98b02a2b5129cd1de164eb9cbd083e8a2503c4e";
        expect_eq(cc20TestMatchesHex(out[0..64], exp), true);
    });

    test("2. @cap/chacha20-crate — RFC 8439 Section 2.4.2 encryption & decryption", () => {
        let key: [32]u8 = undefined;
        cc20FillSeqKey(key[0..32]);

        let nonce: [12]u8 = [
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x4a as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        ];

        let pt_str = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
        expect_eq(pt_str.len, 114 as usize);

        let buf: [114]u8 = undefined;
        let i: usize = 0;
        while (i < (114 as usize)) {
            buf[i] = pt_str[i];
            i += 1;
        }

        let maybe_c = ChaCha20Cipher.newWithCounter(key[0..32], nonce[0..12], 1 as u32);
        expect_eq(maybe_c != null, true);
        let c = maybe_c.?;
        c.applyKeystream(buf[0..114]);

        let exp_ct = "6e2e359a2568f98041ba0728dd0d6981e97e7aec1d4360c20a27afccfd9fae0bf91b65c5524733ab8f593dabcd62b3571639d624e65152ab8f530c359f0861d807ca0dbf500d6a6156a38e088a22b65e52bc514d16ccf806818ce91ab77937365af90bbf74a35be6b40b8eedf2785e42874d";
        expect_eq(cc20TestMatchesHex(buf[0..114], exp_ct), true);

        // Decrypt with fresh cipher
        let dec_c = ChaCha20Cipher.newWithCounter(key[0..32], nonce[0..12], 1 as u32).?;
        dec_c.applyKeystream(buf[0..114]);
        let match_pt: bool = true;
        i = 0;
        while (i < (114 as usize)) {
            if (buf[i] != pt_str[i]) {
                match_pt = false;
            }
            i += 1;
        }
        expect_eq(match_pt, true);
    });

    test("3. @cap/chacha20-crate — Streaming chunked applyKeystream across block boundary", () => {
        let key: [32]u8 = undefined;
        cc20FillSeqKey(key[0..32]);

        let nonce: [12]u8 = [
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x4a as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        ];

        let pt_str = "Ladies and Gentlemen of the class of '99: If I could offer you only one tip for the future, sunscreen would be it.";
        let buf: [114]u8 = undefined;
        let i: usize = 0;
        while (i < (114 as usize)) {
            buf[i] = pt_str[i];
            i += 1;
        }

        let c = ChaCha20Cipher.newWithCounter(key[0..32], nonce[0..12], 1 as u32).?;
        c.applyKeystream(buf[0..1]);
        c.applyKeystream(buf[1..14]);
        c.applyKeystream(buf[14..64]);
        c.applyKeystream(buf[64..70]);
        c.applyKeystream(buf[70..114]);

        let exp_ct = "6e2e359a2568f98041ba0728dd0d6981e97e7aec1d4360c20a27afccfd9fae0bf91b65c5524733ab8f593dabcd62b3571639d624e65152ab8f530c359f0861d807ca0dbf500d6a6156a38e088a22b65e52bc514d16ccf806818ce91ab77937365af90bbf74a35be6b40b8eedf2785e42874d";
        expect_eq(cc20TestMatchesHex(buf[0..114], exp_ct), true);
    });

    test("4. @cap/chacha20-crate — seek() jumps directly to target block counter", () => {
        let key: [32]u8 = undefined;
        cc20FillSeqKey(key[0..32]);

        let nonce: [12]u8 = [
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x09 as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x4a as u8,
            0x00 as u8, 0x00 as u8, 0x00 as u8, 0x00 as u8,
        ];

        let direct: [64]u8 = undefined;
        * = chacha20Block(key[0..32], 5 as u32, nonce[0..12], direct[0..64]);

        let c = ChaCha20Cipher.new(key[0..32], nonce[0..12]).?;
        let junk: [17]u8 = undefined;
        let i: usize = 0;
        while (i < (17 as usize)) {
            junk[i] = 0;
            i += 1;
        }
        c.applyKeystream(junk[0..17]);

        c.seek(5 as u32);
        let zeros: [64]u8 = undefined;
        i = 0;
        while (i < (64 as usize)) {
            zeros[i] = 0;
            i += 1;
        }
        c.applyKeystream(zeros[0..64]);

        let same: bool = true;
        i = 0;
        while (i < (64 as usize)) {
            if (zeros[i] != direct[i]) {
                same = false;
            }
            i += 1;
        }
        expect_eq(same, true);
    });

    test("5. @cap/chacha20-crate — new() defaults to counter 0 & multi-block roundtrip", () => {
        let key: [32]u8 = undefined;
        cc20FillSeqKey(key[0..32]);
        let nonce: [12]u8 = [
            1 as u8, 2 as u8, 3 as u8, 4 as u8,
            5 as u8, 6 as u8, 7 as u8, 8 as u8,
            9 as u8, 10 as u8, 11 as u8, 12 as u8,
        ];

        let b1: [192]u8 = undefined;
        let b2: [192]u8 = undefined;
        let i: usize = 0;
        while (i < (192 as usize)) {
            let v: u8 = (i % (251 as usize)) as u8;
            b1[i] = v;
            b2[i] = v;
            i += 1;
        }

        let c1 = ChaCha20Cipher.new(key[0..32], nonce[0..12]).?;
        let c2 = ChaCha20Cipher.newWithCounter(key[0..32], nonce[0..12], 0 as u32).?;
        c1.applyKeystream(b1[0..192]);
        c2.applyKeystream(b2[0..192]);

        expect_eq(b1[0], b2[0]);
        expect_eq(b1[64], b2[64]);
        expect_eq(b1[191], b2[191]);

        c1.seek(0 as u32);
        c1.applyKeystream(b1[0..192]);
        expect_eq(b1[0], 0 as u8);
        expect_eq(b1[191], (191 % 251) as u8);
    });

    test("6. @cap/chacha20-crate — Invalid key/nonce/output length bounds checks", () => {
        let short_key: [16]u8 = undefined;
        let short_nonce: [8]u8 = undefined;
        let key: [32]u8 = undefined;
        let nonce: [12]u8 = undefined;
        let short_out: [32]u8 = undefined;
        let full_out: [64]u8 = undefined;

        expect_eq(chacha20Block(short_key[0..16], 0 as u32, nonce[0..12], full_out[0..64]), false);
        expect_eq(chacha20Block(key[0..32], 0 as u32, short_nonce[0..8], full_out[0..64]), false);
        expect_eq(chacha20Block(key[0..32], 0 as u32, nonce[0..12], short_out[0..32]), false);

        expect_eq(ChaCha20Cipher.new(short_key[0..16], nonce[0..12]) == null, true);
        expect_eq(ChaCha20Cipher.new(key[0..32], short_nonce[0..8]) == null, true);
        expect_eq(ChaCha20Cipher.newWithCounter(short_key[0..16], nonce[0..12], 1 as u32) == null, true);
    });
}
