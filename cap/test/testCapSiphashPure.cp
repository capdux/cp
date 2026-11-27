// cap/test/testCapSiphashPure.cp
// Comprehensive unit tests for @cap/siphash-pure

import { test, expect_eq } from "cap:test";
import {
    siphash24,
    siphash13,
    siphash24Key16,
    siphash13Key16,
} from "../../packages/siphash/src/index.cp";

fn u64FromHalves(hi: u32, lo: u32): u64 {
    return ((hi as u64) << (32 as u6)) | (lo as u64);
}

fn refKey(): [16]u8 {
    let k: [16]u8 = undefined;
    let i: usize = 0;
    while (i < 16) {
        k[i] = i as u8;
        i += 1;
    }
    return k;
}

fn main() {
    test("1. @cap/siphash-pure — Official SipHash-2-4 paper vectors (0, 1, 15 bytes)", () => {
        const key = refKey();
        let msg: [15]u8 = undefined;
        let i: usize = 0;
        while (i < 15) {
            msg[i] = i as u8;
            i += 1;
        }

        // len=0: 0x726fdb47dd0e0e31
        expect_eq(siphash24Key16(key, msg[0..0]), u64FromHalves(0x726FDB47 as u32, 0xDD0E0E31 as u32));
        // len=1: 0x74f839c593dc67fd
        expect_eq(siphash24Key16(key, msg[0..1]), u64FromHalves(0x74F839C5 as u32, 0x93DC67FD as u32));
        // len=15: 0xa129ca6149be45e5
        expect_eq(siphash24Key16(key, msg[0..15]), u64FromHalves(0xA129CA61 as u32, 0x49BE45E5 as u32));
    });

    test("2. @cap/siphash-pure — Official SipHash-1-3 reference vectors (0 & 15 bytes)", () => {
        const key = refKey();
        let msg: [15]u8 = undefined;
        let i: usize = 0;
        while (i < 15) {
            msg[i] = i as u8;
            i += 1;
        }

        // len=0: 0xabac0158050fc4dc
        expect_eq(siphash13Key16(key, msg[0..0]), u64FromHalves(0xABAC0158 as u32, 0x050FC4DC as u32));
        // len=1: 0xc9f49bf37d57ca93
        expect_eq(siphash13Key16(key, msg[0..1]), u64FromHalves(0xC9F49BF3 as u32, 0x7D57CA93 as u32));
        // len=15: 0xd320d86d2a519956
        expect_eq(siphash13Key16(key, msg[0..15]), u64FromHalves(0xD320D86D as u32, 0x2A519956 as u32));
    });

    test("3. @cap/siphash-pure — siphash24Key16 / siphash13Key16 match (k0, k1) forms", () => {
        const key = refKey();
        const k0 = u64FromHalves(0x07060504 as u32, 0x03020100 as u32);
        const k1 = u64FromHalves(0x0F0E0D0C as u32, 0x0B0A0908 as u32);

        expect_eq(siphash24(k0, k1, "cap-lang"), siphash24Key16(key, "cap-lang"));
        expect_eq(siphash13(k0, k1, "cap-lang"), siphash13Key16(key, "cap-lang"));
    });

    test("4. @cap/siphash-pure — Multi-block 64-byte input consistency", () => {
        const key = refKey();
        let data64: [64]u8 = undefined;
        let i: usize = 0;
        while (i < 64) {
            data64[i] = i as u8;
            i += 1;
        }
        const h1 = siphash24Key16(key, data64[0..64]);
        const h2 = siphash24Key16(key, data64[0..64]);
        const h13 = siphash13Key16(key, data64[0..64]);
        expect_eq(h1, h2);
        expect_eq(h1 != h13, true);
    });

    test("5. @cap/siphash-pure — Key sensitivity avalanche effect", () => {
        const hA = siphash24(1 as u64, 2 as u64, "sensitive-payload");
        const hB = siphash24(1 as u64, 3 as u64, "sensitive-payload");
        expect_eq(hA != hB, true);
        // Count differing bits in XOR (avalanche should flip many bits).
        let diff = hA ^ hB;
        let bits: usize = 0;
        while (diff != (0 as u64)) {
            if ((diff & (1 as u64)) != (0 as u64)) {
                bits += 1;
            }
            diff = diff >> (1 as u6);
        }
        expect_eq(bits >= (16 as usize), true);
    });

    test("6. @cap/siphash-pure — Single-bit input change avalanche effect", () => {
        let a: [16]u8 = undefined;
        let b: [16]u8 = undefined;
        let i: usize = 0;
        while (i < 16) {
            a[i] = 0x41 as u8;
            b[i] = 0x41 as u8;
            i += 1;
        }
        b[7] = 0x40 as u8; // flip 1 bit

        const hA = siphash24(12345 as u64, 67890 as u64, a[0..16]);
        const hB = siphash24(12345 as u64, 67890 as u64, b[0..16]);
        expect_eq(hA != hB, true);
    });
}
