// Source: cp/cap/test/test_bzip2_production_deep.cp (Production deep tests for cap:bzip2)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    BZIP2_MAGIC,
    BZIP2_HEADER_SIZE,
    BZIP2_BLOCK_MAGIC,
    BZIP2_EOS_MAGIC,
    BZIP2_MIN_LEVEL,
    BZIP2_MAX_LEVEL,
    bzip2Crc32,
    bzip2CombineCrc,
    Bzip2BitReader,
    Bzip2StreamHeader,
    Bzip2BlockHeader,
    Bzip2EosFooter,
    Bzip2StreamProbe,
    isBzip2,
    parseBzip2Header,
    parseBzip2BlockHeader,
    parseBzip2Eos,
    probeBzip2Stream,
} from "cap:bzip2";

struct BitWriter {
    buf: [64]u8,
    bit_pos: usize,

    static fn init(): BitWriter {
        return BitWriter {
            buf: std.mem.zeroes([64]u8),
            bit_pos: 0,
        };
    }

    fn writeBit(*_, bit: bool) {
        const byte_idx = _.bit_pos / 8;
        const shift = (7 - (_.bit_pos % 8)) as u3;
        if (bit) {
            _.buf[byte_idx] |= (1 as u8) << shift;
        }
        _.bit_pos += 1;
    }

    fn writeBits(*_, val: u64, n: u8) {
        let i: u8 = 0;
        while (i < n) {
            const shift = (n - 1 - i) as u6;
            _.writeBit(((val >> shift) & 1) != 0);
            i += 1;
        }
    }

    fn byteLen(): usize {
        return (_.bit_pos + 7) / 8;
    }
}

test("1. bzip2 MSB-first CRC32 standard vector ('123456789' -> 0xFC891918) and bzip2CombineCrc", () => {
    expect_eq(BZIP2_HEADER_SIZE, 4 as usize);
    expect_eq(BZIP2_MIN_LEVEL, 1 as u8);
    expect_eq(BZIP2_MAX_LEVEL, 9 as u8);
    expect_eq(BZIP2_MAGIC.len, 3 as usize);

    // CRC-32/BZIP2 standard check value
    expect_eq(bzip2Crc32("123456789"), 0xFC891918 as u32);
    expect_eq(bzip2Crc32(""), 0x00000000 as u32);

    // Combine CRC: rol1(0) ^ crc1 = crc1; rol1(crc1) ^ crc2
    const c1 = bzip2CombineCrc(0, 0x80000001);
    expect_eq(c1, 0x80000001 as u32);
    // rol1(0x80000001) == 0x00000003; 0x00000003 ^ 0x00000005 == 0x00000006
    const c2 = bzip2CombineCrc(c1, 0x00000005);
    expect_eq(c2, 0x00000006 as u32);
});

test("2. Empty bzip2 stream ('BZh9' + EOS √π + 0x00000000) header and probeBzip2Stream", () => {
    let empty_bz2: [14]u8 = [
        'B', 'Z', 'h', '9',
        0x17, 0x72, 0x45, 0x38, 0x50, 0x90,
        0x00, 0x00, 0x00, 0x00,
    ];
    expect_eq(isBzip2(empty_bz2[0..]), true);

    if (hdr := parseBzip2Header(empty_bz2[0..])) {
        expect_eq(hdr.level, 9 as u8);
        expect_eq(hdr.blockSizeBytes(), 900000 as u32);
    } else {
        expect_eq(true, false);
    }

    if (probe := probeBzip2Stream(empty_bz2[0..])) {
        expect_eq(probe.level, 9 as u8);
        expect_eq(probe.is_empty_stream, true);
        expect_eq(probe.first_block_crc, 0 as u32);
        expect_eq(probe.eos_combined_crc, 0 as u32);
    } else {
        expect_eq(true, false);
    }

    // Direct EOS parsing via Bzip2BitReader
    let br = Bzip2BitReader.init(empty_bz2[4..]);
    if (eos := parseBzip2Eos(*br)) {
        expect_eq(eos.combined_crc32, 0 as u32);
        expect_eq(br.bitsRemaining(), 0 as usize);
    } else {
        expect_eq(true, false);
    }
});

test("3. Bit-level encoded Bzip2BlockHeader (48-bit π magic, CRC, orig_ptr, L1/L2 bitmap, trees/selectors) and probeBzip2Stream", () => {
    let bw = BitWriter.init();
    // Stream header "BZh5"
    bw.writeBits('B' as u64, 8);
    bw.writeBits('Z' as u64, 8);
    bw.writeBits('h' as u64, 8);
    bw.writeBits('5' as u64, 8);

    // Block header:
    bw.writeBits(BZIP2_BLOCK_MAGIC, 48); // π magic
    bw.writeBits(0xCAFEBABE, 32);        // block_crc32
    bw.writeBit(false);                  // randomized = false
    bw.writeBits(0x012345, 24);          // orig_ptr (24-bit)
    // L1 bitmap: 2 groups active (bit 15 and bit 0 -> 0x8001)
    bw.writeBits(0x8001, 16);
    // L2[0]: 0x000F (4 symbols)
    bw.writeBits(0x000F, 16);
    // L2[1]: 0xAAAA (8 symbols) -> total symbol_count = 12
    bw.writeBits(0xAAAA, 16);
    // num_trees = 4 (3 bits)
    bw.writeBits(4, 3);
    // num_selectors = 123 (15 bits)
    bw.writeBits(123, 15);

    const len = bw.byteLen();

    // Direct block header parse after 4-byte stream header
    let br = Bzip2BitReader.init(bw.buf[4..len]);
    if (blk := parseBzip2BlockHeader(*br)) {
        expect_eq(blk.block_crc32, 0xCAFEBABE as u32);
        expect_eq(blk.randomized, false);
        expect_eq(blk.orig_ptr, 0x012345 as u32);
        expect_eq(blk.symbol_count, 12 as u16);
        expect_eq(blk.num_trees, 4 as u8);
        expect_eq(blk.num_selectors, 123 as u16);
    } else {
        expect_eq(true, false);
    }

    // probeBzip2Stream detects non-empty stream and extracts first block CRC
    if (probe := probeBzip2Stream(bw.buf[0..len])) {
        expect_eq(probe.level, 5 as u8);
        expect_eq(probe.is_empty_stream, false);
        expect_eq(probe.first_block_crc, 0xCAFEBABE as u32);
    } else {
        expect_eq(true, false);
    }
});

test("4. Bzip2BitReader cross-byte bit extraction and alignToByte()", () => {
    // 0b10110011 = 0xB3, 0b01011100 = 0x5C, 0b11110000 = 0xF0
    let raw: [3]u8 = [0xB3, 0x5C, 0xF0];
    let br = Bzip2BitReader.init(raw[0..]);
    expect_eq(br.bitsRemaining(), 24 as usize);

    // Read 3 bits: 101 = 5
    if (v := br.readBits(3)) {
        expect_eq(v, 5 as u32);
    } else {
        expect_eq(true, false);
    }
    // Read 10 bits crossing byte boundary: 10011_01011 = 0x26B (619)
    if (v := br.readBits(10)) {
        expect_eq(v, 0x26B as u32);
    } else {
        expect_eq(true, false);
    }
    expect_eq(br.bitsRemaining(), 11 as usize);

    // Align to next byte boundary (skips remaining 3 bits of byte 1 -> jumps to byte 2)
    br.alignToByte();
    expect_eq(br.bitsRemaining(), 8 as usize);
    if (v := br.readBits(8)) {
        expect_eq(v, 0xF0 as u32);
    } else {
        expect_eq(true, false);
    }
    expect_eq(br.bitsRemaining(), 0 as usize);

    // Reading past end returns null
    if (_ := br.readBit()) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});

test("5. Malformed bzip2 rejection: bad 'BZh', level '0', num_trees < 2, bad EOS magic, and truncation", () => {
    // 5a. Wrong magic
    let bad_magic: [4]u8 = ['B', 'Z', 'x', '9'];
    expect_eq(isBzip2(bad_magic[0..]), false);
    if (_ := parseBzip2Header(bad_magic[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5b. Invalid level '0'
    let bad_lvl: [4]u8 = ['B', 'Z', 'h', '0'];
    expect_eq(isBzip2(bad_lvl[0..]), false);
    if (_ := parseBzip2Header(bad_lvl[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5c. Block header with num_trees = 1 (< 2) must reject
    let bw = BitWriter.init();
    bw.writeBits(BZIP2_BLOCK_MAGIC, 48);
    bw.writeBits(0x11223344, 32);
    bw.writeBit(false);
    bw.writeBits(0, 24);
    bw.writeBits(0, 16); // L1 = 0
    bw.writeBits(1, 3);  // invalid num_trees = 1
    bw.writeBits(10, 15);
    let br_bad = Bzip2BitReader.init(bw.buf[0..bw.byteLen()]);
    if (_ := parseBzip2BlockHeader(*br_bad)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    // 5d. Bad EOS magic and truncated buffers
    let bad_eos: [10]u8 = [0x17, 0x72, 0x45, 0x38, 0x50, 0x91, 0, 0, 0, 0];
    let br_eos = Bzip2BitReader.init(bad_eos[0..]);
    if (_ := parseBzip2Eos(*br_eos)) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }

    let trunc_stream: [8]u8 = ['B', 'Z', 'h', '9', 0x17, 0x72, 0x45, 0x38];
    if (_ := probeBzip2Stream(trunc_stream[0..])) {
        expect_eq(true, false);
    } else {
        expect_eq(true, true);
    }
});
