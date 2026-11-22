// Origin: Pure-CP Stdlib (Zero-Alloc bzip2 Container & Bitstream Parser) | Created: 2026-11-22

pub const BZIP2_MAGIC: []const u8 = "BZh";
pub const BZIP2_HEADER_SIZE: usize = 4;
pub const BZIP2_BLOCK_MAGIC: u64 = 0x314159265359;
pub const BZIP2_EOS_MAGIC: u64 = 0x177245385090;
pub const BZIP2_MIN_LEVEL: u8 = 1;
pub const BZIP2_MAX_LEVEL: u8 = 9;

/// Computes bzip2 MSB-first CRC32 (forward polynomial 0x04C11DB7, init 0xFFFFFFFF, final XOR).
/// Standard test vector: `bzip2Crc32("123456789") == 0xFC891918`.
pub fn bzip2Crc32(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        crc ^= (data[i] as u32) << 24;
        let bit: u8 = 0;
        while (bit < 8) {
            if ((crc & 0x80000000) != 0) {
                crc = (crc << 1) ^ 0x04C11DB7;
            } else {
                crc = crc << 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return ~crc;
}

/// Combines a bzip2 block CRC into the stream's combined CRC:
/// `((combined << 1) | (combined >> 31)) ^ block_crc`.
pub fn bzip2CombineCrc(combined: u32, block_crc: u32): u32 {
    return ((combined << 1) | (combined >> 31)) ^ block_crc;
}

/// Zero-copy MSB-first bitstream reader for bzip2 streams.
pub struct Bzip2BitReader {
    data: []const u8,
    bit_pos: usize,

    static fn init(data: []const u8): Bzip2BitReader {
        return Bzip2BitReader {
            data: data,
            bit_pos: 0,
        };
    }

    fn bitsRemaining(): usize {
        const total = _.data.len * 8;
        if (_.bit_pos >= total) {
            return 0;
        }
        return total - _.bit_pos;
    }

    fn readBit(*_): bool? {
        if (_.bit_pos >= _.data.len * 8) {
            return null;
        }
        const byte_idx = _.bit_pos / 8;
        const shift = (7 - (_.bit_pos % 8)) as u3;
        const bit = ((_.data[byte_idx] >> shift) & 1) != 0;
        _.bit_pos += 1;
        return bit;
    }

    fn readBits(*_, n: u8): u32? {
        if (n == 0 || n > 32 || _.bitsRemaining() < (n as usize)) {
            return null;
        }
        let acc: u32 = 0;
        let i: u8 = 0;
        while (i < n) {
            if (b := _.readBit()) {
                acc = (acc << 1) | (if (b) { 1 as u32 } else { 0 as u32 });
            } else {
                return null;
            }
            i += 1;
        }
        return acc;
    }

    fn readU48(*_): u64? {
        if (_.bitsRemaining() < 48) {
            return null;
        }
        if (hi := _.readBits(16)) {
            if (lo := _.readBits(32)) {
                return ((hi as u64) << 32) | (lo as u64);
            }
        }
        return null;
    }

    fn alignToByte(*_) {
        const rem = _.bit_pos % 8;
        if (rem != 0) {
            _.bit_pos += 8 - rem;
        }
    }
}

/// Parsed 4-byte bzip2 stream header (`"BZh" + '1'..'9'`).
pub struct Bzip2StreamHeader {
    level: u8,

    /// Returns the block size in bytes (`level * 100000`).
    fn blockSizeBytes(): u32 {
        return (_.level as u32) * 100000;
    }
}

/// Parsed bzip2 compressed block header.
pub struct Bzip2BlockHeader {
    block_crc32: u32,
    randomized: bool,
    orig_ptr: u32,
    symbol_count: u16,
    num_trees: u8,
    num_selectors: u16,
}

/// Parsed bzip2 End-Of-Stream footer.
pub struct Bzip2EosFooter {
    combined_crc32: u32,
}

/// High-level probe result for a bzip2 stream (empty stream or first block).
pub struct Bzip2StreamProbe {
    level: u8,
    is_empty_stream: bool,
    first_block_crc: u32,
    eos_combined_crc: u32,
}

fn popcount16(v: u32): u16 {
    let x = v & 0xFFFF;
    let cnt: u16 = 0;
    while (x != 0) {
        cnt += (x & 1) as u16;
        x >>= 1;
    }
    return cnt;
}

/// Fast magic and level check for a bzip2 stream (`"BZh"` + `'1'..'9'`).
pub fn isBzip2(data: []const u8): bool {
    if (data.len < BZIP2_HEADER_SIZE) {
        return false;
    }
    if (data[0] != 'B' || data[1] != 'Z' || data[2] != 'h') {
        return false;
    }
    return data[3] >= '1' && data[3] <= '9';
}

/// Parses and validates the 4-byte bzip2 stream header.
pub fn parseBzip2Header(data: []const u8): Bzip2StreamHeader? {
    if (!isBzip2(data)) {
        return null;
    }
    return Bzip2StreamHeader {
        level: data[3] - '0',
    };
}

/// Parses a bzip2 block header from an MSB-first `Bzip2BitReader`.
/// Validates 48-bit π magic (`0x314159265359`), `num_trees in 2..6`, and `num_selectors in 1..18002`.
pub fn parseBzip2BlockHeader(br: *Bzip2BitReader): Bzip2BlockHeader? {
    if (magic := br.readU48()) {
        if (magic != BZIP2_BLOCK_MAGIC) {
            return null;
        }
    } else {
        return null;
    }

    let block_crc32: u32 = 0;
    if (crc := br.readBits(32)) {
        block_crc32 = crc;
    } else {
        return null;
    }

    let randomized = false;
    if (r := br.readBit()) {
        randomized = r;
    } else {
        return null;
    }

    let orig_ptr: u32 = 0;
    if (op := br.readBits(24)) {
        orig_ptr = op;
    } else {
        return null;
    }

    // L1 16-bit bitmap + L2 16-bit bitmaps
    let l1: u32 = 0;
    if (m1 := br.readBits(16)) {
        l1 = m1;
    } else {
        return null;
    }

    let symbol_count: u16 = 0;
    let i: u8 = 0;
    while (i < 16) {
        const mask: u32 = (1 as u32) << ((15 - i) as u5);
        if ((l1 & mask) != 0) {
            if (l2 := br.readBits(16)) {
                symbol_count += popcount16(l2);
            } else {
                return null;
            }
        }
        i += 1;
    }

    let num_trees: u8 = 0;
    if (nt := br.readBits(3)) {
        if (nt < 2 || nt > 6) {
            return null;
        }
        num_trees = nt as u8;
    } else {
        return null;
    }

    let num_selectors: u16 = 0;
    if (ns := br.readBits(15)) {
        if (ns < 1 || ns > 18002) {
            return null;
        }
        num_selectors = ns as u16;
    } else {
        return null;
    }

    return Bzip2BlockHeader {
        block_crc32: block_crc32,
        randomized: randomized,
        orig_ptr: orig_ptr,
        symbol_count: symbol_count,
        num_trees: num_trees,
        num_selectors: num_selectors,
    };
}

/// Parses a bzip2 End-Of-Stream footer (48-bit √π magic `0x177245385090` + 32-bit combined CRC).
pub fn parseBzip2Eos(br: *Bzip2BitReader): Bzip2EosFooter? {
    if (magic := br.readU48()) {
        if (magic != BZIP2_EOS_MAGIC) {
            return null;
        }
    } else {
        return null;
    }
    if (crc := br.readBits(32)) {
        return Bzip2EosFooter {
            combined_crc32: crc,
        };
    }
    return null;
}

/// Probes a bzip2 stream, detecting either an empty stream (EOS immediately after `"BZhN"`)
/// or parsing the first compressed block header.
pub fn probeBzip2Stream(data: []const u8): Bzip2StreamProbe? {
    if (hdr := parseBzip2Header(data)) {
        let br = Bzip2BitReader.init(data[BZIP2_HEADER_SIZE..]);
        if (magic := br.readU48()) {
            if (magic == BZIP2_EOS_MAGIC) {
                if (eos_crc := br.readBits(32)) {
                    return Bzip2StreamProbe {
                        level: hdr.level,
                        is_empty_stream: true,
                        first_block_crc: 0,
                        eos_combined_crc: eos_crc,
                    };
                }
                return null;
            }
            if (magic == BZIP2_BLOCK_MAGIC) {
                br.bit_pos = 0;
                if (blk := parseBzip2BlockHeader(*br)) {
                    return Bzip2StreamProbe {
                        level: hdr.level,
                        is_empty_stream: false,
                        first_block_crc: blk.block_crc32,
                        eos_combined_crc: 0,
                    };
                }
                return null;
            }
        }
    }
    return null;
}
