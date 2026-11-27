// @cap/crc32c — Zero-heap pure-CP CRC-32C (Castagnoli, RFC 3720 / iSCSI) checksum
// Clean-room MIT implementation with GF(2) combine and LevelDB/RocksDB/GCS masking.
// Accelerated with comptime [8][256]u32 Slice-by-8 LUT, 16-lane @Vector(16, u8) chunked loads, and undefined stack buffers.

pub const CRC32C_POLY_REVERSED: u32 = 2197175160; // 0x82F63B78
pub const CRC32C_MASK_DELTA: u32 = 2726488792; // 0xA282EAD8

type C32cVec16 = @Vector(16, u8);

fn c32cByteStep(crc_in: u32, b: u8): u32 {
    let c: u32 = crc_in ^ (b as u32);
    c = (c >> 1) ^ (CRC32C_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32C_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32C_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32C_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32C_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32C_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32C_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32C_POLY_REVERSED & (0 -% (c & 1)));
    return c;
}

fn c32cBuildTables(): [8][256]u32 {
    @setEvalBranchQuota(80000);
    let tbl: [8][256]u32 = undefined;
    let i: usize = 0;
    while (i < 256) {
        tbl[0][i] = c32cByteStep(0, i as u8);
        i = i + 1;
    }
    let t: usize = 1;
    while (t < 8) {
        let j: usize = 0;
        while (j < 256) {
            const prev = tbl[t - 1][j];
            tbl[t][j] = (prev >> 8) ^ tbl[0][(prev & 255) as usize];
            j = j + 1;
        }
        t = t + 1;
    }
    return tbl;
}

const CRC32C_LUT: [8][256]u32 = c32cBuildTables();

fn c32cGf2MatrixTimes(mat: [32]u32, vec: u32): u32 {
    let sum: u32 = 0;
    let v: u32 = vec;
    let idx: usize = 0;
    while (v != 0 && idx < 32) {
        if ((v & 1) != 0) {
            sum = sum ^ mat[idx];
        }
        v = v >> 1;
        idx = idx + 1;
    }
    return sum;
}

fn c32cGf2MatrixSquare(src: [32]u32): [32]u32 {
    let out: [32]u32 = undefined;
    let n: usize = 0;
    while (n < 32) {
        out[n] = c32cGf2MatrixTimes(src, src[n]);
        n = n + 1;
    }
    return out;
}

fn crc32cUpdateBytes(crc: u32, data: []const u8): u32 {
    let c: u32 = crc ^ 4294967295; // 0xFFFFFFFF
    let i: usize = 0;
    const len: usize = data.len;
    // 16-byte SIMD load + unrolled 2x Slice-by-8 hot loop
    while (i + 16 <= len) {
        const v: C32cVec16 = data[i .. i + 16][0..16].*;
        const b: [16]u8 = v;
        const lo0: u32 = (b[0] as u32)
            | ((b[1] as u32) << 8)
            | ((b[2] as u32) << 16)
            | ((b[3] as u32) << 24);
        const x0: u32 = c ^ lo0;
        c = CRC32C_LUT[7][(x0 & 255) as usize]
            ^ CRC32C_LUT[6][((x0 >> 8) & 255) as usize]
            ^ CRC32C_LUT[5][((x0 >> 16) & 255) as usize]
            ^ CRC32C_LUT[4][((x0 >> 24) & 255) as usize]
            ^ CRC32C_LUT[3][b[4] as usize]
            ^ CRC32C_LUT[2][b[5] as usize]
            ^ CRC32C_LUT[1][b[6] as usize]
            ^ CRC32C_LUT[0][b[7] as usize];
        const lo1: u32 = (b[8] as u32)
            | ((b[9] as u32) << 8)
            | ((b[10] as u32) << 16)
            | ((b[11] as u32) << 24);
        const x1: u32 = c ^ lo1;
        c = CRC32C_LUT[7][(x1 & 255) as usize]
            ^ CRC32C_LUT[6][((x1 >> 8) & 255) as usize]
            ^ CRC32C_LUT[5][((x1 >> 16) & 255) as usize]
            ^ CRC32C_LUT[4][((x1 >> 24) & 255) as usize]
            ^ CRC32C_LUT[3][b[12] as usize]
            ^ CRC32C_LUT[2][b[13] as usize]
            ^ CRC32C_LUT[1][b[14] as usize]
            ^ CRC32C_LUT[0][b[15] as usize];
        i = i + 16;
    }
    // Slice-by-8 tail
    if (i + 8 <= len) {
        const lo: u32 = (data[i] as u32)
            | ((data[i + 1] as u32) << 8)
            | ((data[i + 2] as u32) << 16)
            | ((data[i + 3] as u32) << 24);
        const x: u32 = c ^ lo;
        c = CRC32C_LUT[7][(x & 255) as usize]
            ^ CRC32C_LUT[6][((x >> 8) & 255) as usize]
            ^ CRC32C_LUT[5][((x >> 16) & 255) as usize]
            ^ CRC32C_LUT[4][((x >> 24) & 255) as usize]
            ^ CRC32C_LUT[3][data[i + 4] as usize]
            ^ CRC32C_LUT[2][data[i + 5] as usize]
            ^ CRC32C_LUT[1][data[i + 6] as usize]
            ^ CRC32C_LUT[0][data[i + 7] as usize];
        i = i + 8;
    }
    // Slice-by-4 tail
    if (i + 4 <= len) {
        const w: u32 = (data[i] as u32)
            | ((data[i + 1] as u32) << 8)
            | ((data[i + 2] as u32) << 16)
            | ((data[i + 3] as u32) << 24);
        const x: u32 = c ^ w;
        c = CRC32C_LUT[3][(x & 255) as usize]
            ^ CRC32C_LUT[2][((x >> 8) & 255) as usize]
            ^ CRC32C_LUT[1][((x >> 16) & 255) as usize]
            ^ CRC32C_LUT[0][((x >> 24) & 255) as usize];
        i = i + 4;
    }
    while (i < len) {
        const idx: usize = ((c ^ (data[i] as u32)) & 255) as usize;
        c = (c >> 8) ^ CRC32C_LUT[0][idx];
        i = i + 1;
    }
    return c ^ 4294967295;
}

pub fn crc32cAppend(crc: u32, data: []u8): u32 {
    return crc32cUpdateBytes(crc, data);
}

pub fn crc32c(data: []u8): u32 {
    return crc32cUpdateBytes(0, data);
}

pub fn crc32cStr(s: string): u32 {
    return crc32cUpdateBytes(0, s);
}

pub fn crc32cCombine(crc1: u32, crc2: u32, len2: u64): u32 {
    if (len2 == 0) {
        return crc1;
    }
    let odd: [32]u32 = undefined;
    odd[0] = CRC32C_POLY_REVERSED;
    let row: u32 = 1;
    let n: usize = 1;
    while (n < 32) {
        odd[n] = row;
        row = row << 1;
        n = n + 1;
    }
    let even = c32cGf2MatrixSquare(odd);
    odd = c32cGf2MatrixSquare(even);

    let c1: u32 = crc1;
    let rem: u64 = len2;
    while (rem != 0) {
        even = c32cGf2MatrixSquare(odd);
        if ((rem & 1) != 0) {
            c1 = c32cGf2MatrixTimes(even, c1);
        }
        rem = rem >> 1;
        if (rem == 0) {
            break;
        }
        odd = c32cGf2MatrixSquare(even);
        if ((rem & 1) != 0) {
            c1 = c32cGf2MatrixTimes(odd, c1);
        }
        rem = rem >> 1;
    }
    return c1 ^ crc2;
}

pub fn maskCrc32c(crc: u32): u32 {
    let rot: u32 = (crc >> 15) | (crc << 17);
    let sum: u64 = (rot as u64) + (CRC32C_MASK_DELTA as u64);
    let mask32: u64 = 4294967295;
    return (sum & mask32) as u32;
}

pub fn unmaskCrc32c(masked: u32): u32 {
    let base32: u64 = (1 as u64) << 32;
    let mask32: u64 = 4294967295;
    let diff: u64 = ((masked as u64) + base32 - (CRC32C_MASK_DELTA as u64)) & mask32;
    let rot: u32 = diff as u32;
    return (rot >> 17) | (rot << 15);
}

pub struct Crc32cHasher {
    pub state: u32,
    pub amount: u64,

    pub static fn new(): Crc32cHasher {
        return Crc32cHasher { state: 0, amount: 0 };
    }

    pub static fn newWithInitial(init_crc: u32): Crc32cHasher {
        return Crc32cHasher { state: init_crc, amount: 0 };
    }

    pub fn update(*_, buf: []u8) {
        _.state = crc32cUpdateBytes(_.state, buf);
        _.amount = _.amount + (buf.len as u64);
    }

    pub fn updateStr(*_, s: string) {
        _.state = crc32cUpdateBytes(_.state, s);
        _.amount = _.amount + (s.len as u64);
    }

    pub fn finalize(): u32 {
        return _.state;
    }

    pub fn reset(*_) {
        _.state = 0;
        _.amount = 0;
    }

    pub fn combine(*_, other: Crc32cHasher) {
        _.state = crc32cCombine(_.state, other.state, other.amount);
        _.amount = _.amount + other.amount;
    }
}
