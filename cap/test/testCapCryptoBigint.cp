// cap/test/testCapCryptoBigint.cp
// Comprehensive unit tests for @cap/crypto-bigint

import { test, expect_eq } from "cap:test";
import {
    U256,
    addMod,
    subMod,
} from "../../packages/crypto-bigint/src/index.cp";

fn main() {
    test("1. @cap/crypto-bigint — ZERO/ONE constants, fromU64, BE/LE bytes & hex roundtrip", () => {
        expect_eq(U256.ZERO.bitLen(), 0 as usize);
        expect_eq(U256.ONE.bitLen(), 1 as usize);
        expect_eq(U256.ZERO.ctEq(U256.fromU64(0 as u64)), true);
        expect_eq(U256.ONE.ctEq(U256.fromU64(1 as u64)), true);

        const hex = "0123456789abcdef00112233445566778899aabbccddeeffdeadbeefcafebabe";
        const parsedOpt = U256.fromBeHex(hex);
        expect_eq(parsedOpt != null, true);
        const v = parsedOpt.?;

        const be = v.toBeBytes();
        expect_eq(be[0], 0x01 as u8);
        expect_eq(be[31], 0xbe as u8);
        expect_eq(U256.fromBeBytes(be).ctEq(v), true);

        const le = v.toLeBytes();
        expect_eq(le[0], 0xbe as u8);
        expect_eq(le[31], 0x01 as u8);
        expect_eq(U256.fromLeBytes(le).ctEq(v), true);

        let hexOut: [64]u8 = undefined;
        const n = v.toHex(hexOut[0..64]);
        expect_eq(n, 64 as usize);
        expect_eq(hexOut[0], '0' as u8);
        expect_eq(hexOut[1], '1' as u8);
        expect_eq(hexOut[62], 'b' as u8);
        expect_eq(hexOut[63], 'e' as u8);

        // Invalid hex rejected
        expect_eq(U256.fromBeHex("1234") == null, true);
        expect_eq(U256.fromBeHex("zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz") == null, true);
    });

    test("2. @cap/crypto-bigint — wrappingAdd & wrappingSub with 256-bit wraparound", () => {
        const maxHex = "ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff";
        const maxV = U256.fromBeHex(maxHex).?;

        expect_eq(maxV.bitLen(), 256 as usize);
        expect_eq(maxV.wrappingAdd(U256.ONE).ctEq(U256.ZERO), true);
        expect_eq(U256.ZERO.wrappingSub(U256.ONE).ctEq(maxV), true);

        const a = U256.fromU64(0x100000000 as u64);
        const b = U256.fromU64(0x23456789 as u64);
        const sum = a.wrappingAdd(b);
        expect_eq(sum.wrappingSub(b).ctEq(a), true);
    });

    test("3. @cap/crypto-bigint — wrappingMul across multi-limb boundaries", () => {
        // 2^64 * 2^64 = 2^128
        const two64 = U256.fromBeHex("0000000000000000000000000000000000000000000000010000000000000000").?;
        const two128 = U256.fromBeHex("0000000000000000000000000000000100000000000000000000000000000000").?;
        expect_eq(two64.wrappingMul(two64).ctEq(two128), true);

        // (2^256 - 1) * (2^256 - 1) mod 2^256 = 1
        const zero: U256 = U256.ZERO;
        const maxV: U256 = zero.wrappingSub(U256.ONE);
        expect_eq(maxV.wrappingMul(maxV).ctEq(U256.ONE), true);

        // 0x123456789abcdef0 * 0x10000 = 0x123456789abcdef00000
        const x = U256.fromBeHex("000000000000000000000000000000000000000000000000123456789abcdef0").?;
        const expected = U256.fromBeHex("00000000000000000000000000000000000000000000123456789abcdef00000").?;
        expect_eq(x.wrappingMul(U256.fromU64(0x10000 as u64)).ctEq(expected), true);
    });

    test("4. @cap/crypto-bigint — shl1, shr1 & bitLen across limb boundaries", () => {
        let v: U256 = U256.ONE;
        let i: usize = 0;
        while (i < 64) {
            v = v.shl1();
            i += 1;
        }
        expect_eq(v.bitLen(), 65 as usize);
        expect_eq(v.limbs[0], 0 as u64);
        expect_eq(v.limbs[1], 1 as u64);

        const back = v.shr1();
        expect_eq(back.bitLen(), 64 as usize);
        expect_eq(back.limbs[1], 0 as u64);
        expect_eq(back.limbs[0], ((0x80000000 as u64) << 32));
    });

    test("5. @cap/crypto-bigint — Constant-time ctEq, ctCmp & ctSelect", () => {
        const a = U256.fromBeHex("000000000000000000000000000000000000000000000000000000000000000a").?;
        const b = U256.fromBeHex("0000000000000000000000000000000000000000000000000000000000000014").?;
        const big = U256.fromBeHex("8000000000000000000000000000000000000000000000000000000000000000").?;

        expect_eq(a.ctCmp(b), -1 as i32);
        expect_eq(b.ctCmp(a), 1 as i32);
        expect_eq(a.ctCmp(a), 0 as i32);
        expect_eq(big.ctCmp(b), 1 as i32);
        expect_eq(b.ctCmp(big), -1 as i32);

        expect_eq(U256.ctSelect(true, a, b).ctEq(a), true);
        expect_eq(U256.ctSelect(false, a, b).ctEq(b), true);
    });

    test("6. @cap/crypto-bigint — Cryptographic addMod & subMod on secp256k1 prime p", () => {
        // secp256k1 field prime p = 2^256 - 2^32 - 977
        const p = U256.fromBeHex("fffffffffffffffffffffffffffffffffffffffffffffffffffffffefffffc2f").?;
        const pMinus1 = p.wrappingSub(U256.ONE);
        const pMinus2 = p.wrappingSub(U256.fromU64(2 as u64));
        const ten = U256.fromU64(10 as u64);

        // (p - 1) + 1 mod p = 0
        expect_eq(addMod(pMinus1, U256.ONE, p).ctEq(U256.ZERO), true);
        // (p - 1) + (p - 1) mod p = p - 2 (tests 256-bit carry overflow!)
        expect_eq(addMod(pMinus1, pMinus1, p).ctEq(pMinus2), true);
        // 0 - 1 mod p = p - 1
        expect_eq(subMod(U256.ZERO, U256.ONE, p).ctEq(pMinus1), true);
        // (p - 2) + 10 mod p = 8, and 8 - 10 mod p = p - 2
        const r = addMod(pMinus2, ten, p);
        expect_eq(r.ctEq(U256.fromU64(8 as u64)), true);
        expect_eq(subMod(r, ten, p).ctEq(pMinus2), true);
    });
}
