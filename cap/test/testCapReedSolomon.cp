// cap/test/testCapReedSolomon.cp
// Comprehensive unit tests for @cap/reed-solomon

import { test, expect_eq } from "cap:test";
import {
    gfMul,
    gfPow,
    gfInv,
    RsCodec,
} from "../../packages/@cap/reed-solomon/src/index.cp";

fn main() {
    test("1. @cap/reed-solomon — GF(2^8) field axioms & inverse identity across all 255 non-zero elements", () => {
        expect_eq(gfInv(0), 0 as u8);
        expect_eq(gfMul(0, 123), 0 as u8);
        expect_eq(gfPow(2, 0), 1 as u8);
        expect_eq(gfPow(2, 8), 0x1D as u8); // x^8 == x^4 + x^3 + x^2 + 1 mod 0x11D

        let a: usize = 1;
        while (a < 256) {
            const elem = a as u8;
            const inv = gfInv(elem);
            expect_eq(gfMul(elem, inv), 1 as u8);
            a += 1;
        }
    });

    test("2. @cap/reed-solomon — RS(4, 2) encoding & verifyByte tamper detection", () => {
        const rs = RsCodec.init(4, 2);
        expect_eq(rs.dataShards(), 4 as usize);
        expect_eq(rs.parityShards(), 2 as usize);
        expect_eq(rs.totalShards(), 6 as usize);

        let data: [4]u8 = [0x11, 0x22, 0x33, 0x44];
        let parity: [2]u8 = [0, 0];
        expect_eq(rs.encodeByte(data[0..4], parity[0..2]), true);
        expect_eq(rs.verifyByte(data[0..4], parity[0..2]), true);

        // Corrupt 1 bit in data -> verification must fail
        data[2] ^= 0x01;
        expect_eq(rs.verifyByte(data[0..4], parity[0..2]), false);
    });

    test("3. @cap/reed-solomon — RS(4, 2) exact recovery when 2 data shards are lost", () => {
        const rs = RsCodec.init(4, 2);
        let orig: [6]u8 = [0xCA, 0xFE, 0xBA, 0xBE, 0, 0];
        expect_eq(rs.encodeByte(orig[0..4], orig[4..6]), true);

        // Lose data shards 0 and 3
        let shards: [6]u8 = [0, orig[1], orig[2], 0, orig[4], orig[5]];
        let present: [6]bool = [false, true, true, false, true, true];

        expect_eq(rs.reconstructByte(shards[0..6], present[0..6]), true);
        expect_eq(shards[0], 0xCA as u8);
        expect_eq(shards[1], 0xFE as u8);
        expect_eq(shards[2], 0xBA as u8);
        expect_eq(shards[3], 0xBE as u8);
        expect_eq(rs.verifyByte(shards[0..4], shards[4..6]), true);
    });

    test("4. @cap/reed-solomon — RS(4, 2) mixed loss (1 data + 1 parity) recovery", () => {
        const rs = RsCodec.init(4, 2);
        let orig: [6]u8 = [10, 20, 30, 40, 0, 0];
        expect_eq(rs.encodeByte(orig[0..4], orig[4..6]), true);

        // Lose data shard 1 and parity shard 0 (index 4)
        let shards: [6]u8 = [orig[0], 0, orig[2], orig[3], 0, orig[5]];
        let present: [6]bool = [true, false, true, true, false, true];

        expect_eq(rs.reconstructByte(shards[0..6], present[0..6]), true);
        expect_eq(shards[1], 20 as u8);
        expect_eq(shards[4], orig[4]);
        expect_eq(rs.verifyByte(shards[0..4], shards[4..6]), true);
    });

    test("5. @cap/reed-solomon — RS(6, 3) full 3-shard erasure recovery", () => {
        const rs = RsCodec.init(6, 3);
        let orig: [9]u8 = [0x01, 0x23, 0x45, 0x67, 0x89, 0xAB, 0, 0, 0];
        expect_eq(rs.encodeByte(orig[0..6], orig[6..9]), true);

        // Lose 3 data shards: indices 0, 2, 5
        let shards: [9]u8 = [0, orig[1], 0, orig[3], orig[4], 0, orig[6], orig[7], orig[8]];
        let present: [9]bool = [false, true, false, true, true, false, true, true, true];

        expect_eq(rs.reconstructByte(shards[0..9], present[0..9]), true);
        expect_eq(shards[0], 0x01 as u8);
        expect_eq(shards[2], 0x45 as u8);
        expect_eq(shards[5], 0xAB as u8);
        expect_eq(rs.verifyByte(shards[0..6], shards[6..9]), true);
    });

    test("6. @cap/reed-solomon — Losing more than parity_shards safely returns false", () => {
        const rs = RsCodec.init(4, 2);
        let shards: [6]u8 = [1, 2, 3, 4, 0, 0];
        expect_eq(rs.encodeByte(shards[0..4], shards[4..6]), true);

        // Only 3 shards survive (< data_shards=4) => unrecoverable
        let present: [6]bool = [false, false, false, true, true, true];
        expect_eq(rs.reconstructByte(shards[0..6], present[0..6]), false);
    });
}
