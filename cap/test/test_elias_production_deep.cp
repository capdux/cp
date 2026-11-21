// Source: cp/cap/test/test_elias_production_deep.cp (Production deep tests for cap:elias)

import { test, expect_eq } from "cap:test";
import {
    BitWriter,
    BitReader,
    writeEliasGamma,
    readEliasGamma,
    writeEliasDelta,
    readEliasDelta,
    writeEliasOmega,
    readEliasOmega,
    writeExpGolombU,
    readExpGolombU,
    writeExpGolombS,
    readExpGolombS,
    writeRice,
    readRice,
    writeFibonacci,
    readFibonacci,
} from "cap:elias";

test("1. Elias Gamma, Delta, and Omega round-trip from 1..1_000_000 and 60-bit large integers", () => {
    const vals: [10]u64 = [
        1,
        2,
        3,
        4,
        7,
        10,
        255,
        65536,
        1000000,
        0x0ABCDEF123456789, // 60-bit large integer
    ];

    let buf: [512]u8 = [0xAA; 512]; // prefill non-zero to verify BitWriter clears bytes

    // Elias Gamma
    let bw_g = BitWriter.init(buf[0..]);
    let i: usize = 0;
    while (i < 10) {
        expect_eq(writeEliasGamma(*bw_g, vals[i]), true);
        i += 1;
    }
    let br_g = BitReader.init(bw_g.flush());
    i = 0;
    while (i < 10) {
        expect_eq(readEliasGamma(*br_g) ?? 0, vals[i]);
        i += 1;
    }

    // Elias Delta
    let bw_d = BitWriter.init(buf[0..]);
    i = 0;
    while (i < 10) {
        expect_eq(writeEliasDelta(*bw_d, vals[i]), true);
        i += 1;
    }
    let br_d = BitReader.init(bw_d.flush());
    i = 0;
    while (i < 10) {
        expect_eq(readEliasDelta(*br_d) ?? 0, vals[i]);
        i += 1;
    }

    // Elias Omega
    let bw_o = BitWriter.init(buf[0..]);
    i = 0;
    while (i < 10) {
        expect_eq(writeEliasOmega(*bw_o, vals[i]), true);
        i += 1;
    }
    let br_o = BitReader.init(bw_o.flush());
    i = 0;
    while (i < 10) {
        expect_eq(readEliasOmega(*br_o) ?? 0, vals[i]);
        i += 1;
    }
});

test("2. H.264/HEVC Exp-Golomb ue(v) and se(v) unsigned and signed sequence round-trip", () => {
    let buf: [256]u8 = [0; 256];
    let bw = BitWriter.init(buf[0..]);

    const uvals: [6]u64 = [0, 1, 2, 15, 1024, 999999999];
    const svals: [9]i64 = [0, 1, -1, 2, -2, 3, -3, 123456, -987654321];

    let i: usize = 0;
    while (i < 6) {
        expect_eq(writeExpGolombU(*bw, uvals[i]), true);
        i += 1;
    }
    i = 0;
    while (i < 9) {
        expect_eq(writeExpGolombS(*bw, svals[i]), true);
        i += 1;
    }

    let br = BitReader.init(bw.flush());
    i = 0;
    while (i < 6) {
        expect_eq(readExpGolombU(*br) ?? 999, uvals[i]);
        i += 1;
    }
    i = 0;
    while (i < 9) {
        expect_eq(readExpGolombS(*br) ?? 999, svals[i]);
        i += 1;
    }
});

test("3. Golomb-Rice round-trip for parameters k = 0, 2, 4, 8", () => {
    const ks: [4]u8 = [0, 2, 4, 8];
    const samples: [6]u64 = [0, 1, 3, 15, 64, 513];

    let buf: [1024]u8 = [0; 1024];
    let ki: usize = 0;
    while (ki < 4) {
        const k = ks[ki];
        let bw = BitWriter.init(buf[0..]);
        let si: usize = 0;
        while (si < 6) {
            expect_eq(writeRice(*bw, samples[si], k), true);
            si += 1;
        }
        let br = BitReader.init(bw.flush());
        si = 0;
        while (si < 6) {
            expect_eq(readRice(*br, k) ?? 99999, samples[si]);
            si += 1;
        }
        ki += 1;
    }
});

test("4. Fibonacci Zeckendorf exact bit patterns (1->11, 2->011, 10->010011) and large integer round-trip", () => {
    // Verify exact bit patterns:
    //   1  -> "11"       (2 bits)
    //   2  -> "011"      (3 bits)
    //   10 -> "010011"   (6 bits)
    // Concatenated 11 bits: 11011010 011_00000 -> byte0 = 0xDA, byte1 = 0x60
    let pat_buf: [4]u8 = [0; 4];
    let bw_p = BitWriter.init(pat_buf[0..]);
    expect_eq(writeFibonacci(*bw_p, 1), true);
    expect_eq(writeFibonacci(*bw_p, 2), true);
    expect_eq(writeFibonacci(*bw_p, 10), true);
    expect_eq(bw_p.bit_pos, 11);
    expect_eq(pat_buf[0], 0xDA as u8);
    expect_eq(pat_buf[1], 0x60 as u8);

    let br_p = BitReader.init(bw_p.flush());
    expect_eq(readFibonacci(*br_p) ?? 0, 1 as u64);
    expect_eq(readFibonacci(*br_p) ?? 0, 2 as u64);
    expect_eq(readFibonacci(*br_p) ?? 0, 10 as u64);

    // Round-trip across wide range including 60-bit values
    const fvals: [7]u64 = [1, 2, 3, 10, 144, 1000000, 0x0555555555555555];
    let fbuf: [128]u8 = [0; 128];
    let bw = BitWriter.init(fbuf[0..]);
    let i: usize = 0;
    while (i < 7) {
        expect_eq(writeFibonacci(*bw, fvals[i]), true);
        i += 1;
    }
    let br = BitReader.init(bw.flush());
    i = 0;
    while (i < 7) {
        expect_eq(readFibonacci(*br) ?? 0, fvals[i]);
        i += 1;
    }
});

test("5. Invalid input, truncated bitstream, and excessive leading-zero overflow rejection", () => {
    let buf: [16]u8 = [0; 16];
    let bw = BitWriter.init(buf[0..]);

    // n == 0 rejected by Elias Gamma/Delta/Omega and Fibonacci
    expect_eq(writeEliasGamma(*bw, 0), false);
    expect_eq(writeEliasDelta(*bw, 0), false);
    expect_eq(writeEliasOmega(*bw, 0), false);
    expect_eq(writeFibonacci(*bw, 0), false);

    // Invalid Rice parameter k > 62 rejected
    expect_eq(writeRice(*bw, 10, 63), false);
    let br_k = BitReader.init(buf[0..]);
    expect_eq(readRice(*br_k, 63) == null, true);

    // Empty / truncated bitstream rejected
    let br_empty = BitReader.init("");
    expect_eq(readEliasGamma(*br_empty) == null, true);
    expect_eq(readEliasDelta(*br_empty) == null, true);
    expect_eq(readEliasOmega(*br_empty) == null, true);
    expect_eq(readExpGolombU(*br_empty) == null, true);
    expect_eq(readFibonacci(*br_empty) == null, true);

    // 72 leading zeros (9 zero bytes) must be rejected without overflowing 64-bit shift
    let zeros: [10]u8 = [0; 10];
    zeros[9] = 0x80;
    let br_z = BitReader.init(zeros[0..]);
    expect_eq(readEliasGamma(*br_z) == null, true);

    // Output buffer overflow on tiny 1-byte buffer
    let tiny: [1]u8 = [0; 1];
    let bw_tiny = BitWriter.init(tiny[0..]);
    expect_eq(writeEliasGamma(*bw_tiny, 100000), false);
});
