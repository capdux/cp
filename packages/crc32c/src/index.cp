// @cap/crc32c — Zero-heap pure-CP CRC-32C (Castagnoli, RFC 3720 / iSCSI) checksum
// Clean-room MIT implementation with GF(2) combine and LevelDB/RocksDB/GCS masking.

pub const CRC32C_POLY_REVERSED: u32 = 2197175160; // 0x82F63B78
pub const CRC32C_MASK_DELTA: u32 = 2726488792; // 0xA282EAD8

fn c32cByteStep(crc_in: u32, b: u8): u32 {
    let c: u32 = crc_in ^ (b as u32);
    let bit: usize = 0;
    while (bit < 8) {
        if ((c & 1) != 0) {
            c = (c >> 1) ^ CRC32C_POLY_REVERSED;
        } else {
            c = c >> 1;
        }
        bit = bit + 1;
    }
    return c;
}

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
    let out: [32]u32 = [0; 32];
    let n: usize = 0;
    while (n < 32) {
        out[n] = c32cGf2MatrixTimes(src, src[n]);
        n = n + 1;
    }
    return out;
}

pub fn crc32cAppend(crc: u32, data: []u8): u32 {
    let c: u32 = crc ^ 4294967295; // 0xFFFFFFFF
    let i: usize = 0;
    while (i < data.len) {
        c = c32cByteStep(c, data[i]);
        i = i + 1;
    }
    return c ^ 4294967295;
}

pub fn crc32c(data: []u8): u32 {
    return crc32cAppend(0, data);
}

pub fn crc32cStr(s: string): u32 {
    let c: u32 = 4294967295; // 0 ^ 0xFFFFFFFF
    let i: usize = 0;
    while (i < s.len) {
        c = c32cByteStep(c, s[i]);
        i = i + 1;
    }
    return c ^ 4294967295;
}

pub fn crc32cCombine(crc1: u32, crc2: u32, len2: u64): u32 {
    if (len2 == 0) {
        return crc1;
    }
    let odd: [32]u32 = [0; 32];
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
        _.state = crc32cAppend(_.state, buf);
        _.amount = _.amount + (buf.len as u64);
    }

    pub fn updateStr(*_, s: string) {
        let c: u32 = _.state ^ 4294967295;
        let i: usize = 0;
        while (i < s.len) {
            c = c32cByteStep(c, s[i]);
            i = i + 1;
        }
        _.state = c ^ 4294967295;
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
