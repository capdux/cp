import { test, expect_eq } from "cap:test";
import {
    U256,
    DivRem256,
    zero,
    one,
    fromU64,
    fromU128,
    maxVal,
    fromHex,
} from "../../packages/@cap/bigint-u256/src/index.cp";

fn sliceEqStr(buf: []const u8, len: usize, expected: string): bool {
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
    test("1. @cap/bigint-u256 — cross-limb carry add & wraparound sub", () => {
        let max_v: U256 = maxVal();
        // maxVal + 1 == 0
        expect_eq(max_v.wrappingAdd(one()).isZero(), true);
        // 0 - 1 == maxVal
        expect_eq(zero().wrappingSub(one()).eq(max_v), true);

        // Cross-limb carry from limb 0 -> limb 1
        let max64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        let a: U256 = fromU64(max64);
        let sum: U256 = a.wrappingAdd(one());
        expect_eq(sum.limbs[0], 0 as u64);
        expect_eq(sum.limbs[1], 1 as u64);
        expect_eq(sum.wrappingSub(one()).eq(a), true);
    });

    test("2. @cap/bigint-u256 — 128x128 -> 256-bit exact multiplication", () => {
        // 2^128 - 1 = 0xffffffffffffffffffffffffffffffff
        let max64: u64 = ((0xFFFFFFFF as u64) << 32) | (0xFFFFFFFF as u64);
        let m128: U256 = fromU128(max64, max64);
        // (2^128 - 1)^2 = 2^256 - 2^129 + 1
        // limbs: [1, 0, max64 - 1, max64]
        let sq: U256 = m128.wrappingMul(m128);
        expect_eq(sq.limbs[0], 1 as u64);
        expect_eq(sq.limbs[1], 0 as u64);
        expect_eq(sq.limbs[2], max64 - 1);
        expect_eq(sq.limbs[3], max64);

        // (2^64) * (2^64) == 2^128
        let p64: U256 = one().shl(64);
        let p128: U256 = p64.wrappingMul(p64);
        expect_eq(p128.eq(one().shl(128)), true);
    });

    test("3. @cap/bigint-u256 — divRem verifies q*d + r == n & div-by-zero safety", () => {
        let n: U256 = fromHex("0x123456789abcdef0fedcba9876543210aabbccddeeff00112233445566778899");
        let d: U256 = fromHex("0x1000000000000000100000001");
        let qr: DivRem256 = n.divRem(d);

        // Verify q * d + r == n and r < d
        let recon: U256 = qr.quot.wrappingMul(d).wrappingAdd(qr.rem);
        expect_eq(recon.eq(n), true);
        expect_eq(qr.rem.cmp(d), -1 as i32);

        // Exact division: (2^128) / (2^64) == 2^64, rem == 0
        let qr2: DivRem256 = one().shl(128).divRem(one().shl(64));
        expect_eq(qr2.quot.eq(one().shl(64)), true);
        expect_eq(qr2.rem.isZero(), true);

        // Division by zero safely returns zero
        let qr0: DivRem256 = n.divRem(zero());
        expect_eq(qr0.quot.isZero(), true);
        expect_eq(qr0.rem.isZero(), true);
    });

    test("4. @cap/bigint-u256 — shl, shr & bitwise AND/OR/XOR", () => {
        let v: U256 = fromU64(0x12345678);
        let shifted: U256 = v.shl(100);
        expect_eq(shifted.shr(100).eq(v), true);
        expect_eq(v.shl(256).isZero(), true);
        expect_eq(maxVal().shr(256).isZero(), true);

        let a: U256 = fromHex("0xf0f00000000000000000000000000000000000000000000000000000000000aa");
        let b: U256 = fromHex("0x0ff0000000000000000000000000000000000000000000000000000000000055");
        expect_eq(a.bitAnd(b).eq(fromHex("0x00f0000000000000000000000000000000000000000000000000000000000000")), true);
        expect_eq(a.bitOr(b).eq(fromHex("0xfff00000000000000000000000000000000000000000000000000000000000ff")), true);
        expect_eq(a.bitXor(a).isZero(), true);
    });

    test("5. @cap/bigint-u256 — fromHex & toHex round-trip with 64-hex-digit Ethereum/crypto values", () => {
        let hex_in: string = "0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141";
        let secp_n: U256 = fromHex(hex_in);
        let buf: [64]u8 = [0; 64];
        let written: usize = secp_n.toHex(buf[0..64]);
        expect_eq(written, 64 as usize);
        expect_eq(sliceEqStr(buf[0..64], written, "fffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141"), true);

        let z_len: usize = zero().toHex(buf[0..64]);
        expect_eq(sliceEqStr(buf[0..64], z_len, "0"), true);

        let small: U256 = fromHex("0x0000deadbeef");
        let s_len: usize = small.toHex(buf[0..64]);
        expect_eq(sliceEqStr(buf[0..64], s_len, "deadbeef"), true);
    });

    test("6. @cap/bigint-u256 — cmp, leadingZeros & bitLen", () => {
        expect_eq(zero().leadingZeros(), 256 as u32);
        expect_eq(zero().bitLen(), 0 as u32);
        expect_eq(one().leadingZeros(), 255 as u32);
        expect_eq(one().bitLen(), 1 as u32);
        expect_eq(maxVal().leadingZeros(), 0 as u32);
        expect_eq(maxVal().bitLen(), 256 as u32);

        let p200: U256 = one().shl(200);
        expect_eq(p200.bitLen(), 201 as u32);
        expect_eq(p200.leadingZeros(), 55 as u32);

        expect_eq(one().cmp(p200), -1 as i32);
        expect_eq(p200.cmp(one()), 1 as i32);
        expect_eq(p200.cmp(p200), 0 as i32);
    });
}
