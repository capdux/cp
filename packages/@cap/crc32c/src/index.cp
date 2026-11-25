// @cap/crc32c — Zero-allocation CRC32C (Castagnoli 0x82F63B78) Checksum, Streaming Hasher,
// GF(2) Parallel Combine, and LevelDB/RocksDB/TFRecord Mask/Unmask.
// Clean-room pure-CP implementation of the public RFC 3720 / Castagnoli CRC32C specification (MIT License).

/// Reversed Castagnoli polynomial (`0x82F63B78`).
pub const CASTAGNOLI_POLY: u32 = 0x82F63B78;

/// LevelDB / RocksDB / TensorFlow TFRecord CRC32C mask delta (`0xa282ead8`).
pub const MASK_DELTA: u32 = 0xa282ead8;

fn crc32cByte(crc_in: u32, byte: u8): u32 {
    let c: u32 = crc_in ^ (byte as u32);
    let bit: usize = 0;
    while (bit < 8) : (bit += 1) {
        if ((c & 1) != 0) {
            c = (c >> 1) ^ CASTAGNOLI_POLY;
        } else {
            c = c >> 1;
        }
    }
    return c;
}

/// Comptime-style 256-entry lookup table generator (or runtime cached in hasher / computed directly).
fn buildTable(): [256]u32 {
    let tbl: [256]u32 = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let c: u32 = i as u32;
        let bit: usize = 0;
        while (bit < 8) : (bit += 1) {
            if ((c & 1) != 0) {
                c = (c >> 1) ^ CASTAGNOLI_POLY;
            } else {
                c = c >> 1;
            }
        }
        tbl[i] = c;
    }
    return tbl;
}

/// Computes CRC32C (Castagnoli) of `data` in one shot.
/// Matches RFC 3720 / iSCSI / LevelDB / gRPC / S3 (`crc32c("123456789") == 0xE3069283`).
pub fn crc32c(data: []const u8): u32 {
    return crc32cAppend(0, data);
}

/// Incrementally appends `data` to an existing finalized CRC32C value `prev`.
pub fn crc32cAppend(prev: u32, data: []const u8): u32 {
    if (data.len == 0) return prev;
    const tbl = buildTable();
    let crc: u32 = ~prev;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        const idx: usize = ((crc ^ (data[i] as u32)) & 0xFF) as usize;
        crc = (crc >> 8) ^ tbl[idx];
    }
    return ~crc;
}

/// Masks a CRC32C value using the standard LevelDB / RocksDB / TFRecord formula,
/// preventing problematic checksums of data that already contains embedded CRCs.
pub fn mask(crc: u32): u32 {
    return (((crc >> 15) | (crc << 17)) +% MASK_DELTA);
}

/// Reverses `mask(crc)` to recover the original CRC32C value.
pub fn unmask(masked: u32): u32 {
    const rot: u32 = masked -% MASK_DELTA;
    return (rot >> 17) | (rot << 15);
}

fn gf2MatrixTimes(mat: *[32]u32, vec_in: u32): u32 {
    let sum: u32 = 0;
    let v = vec_in;
    let idx: usize = 0;
    while (v != 0) {
        if ((v & 1) != 0) {
            sum ^= mat.*[idx];
        }
        v >>= 1;
        idx += 1;
    }
    return sum;
}

fn gf2MatrixSquare(square: *[32]u32, mat: *[32]u32) {
    let n: usize = 0;
    while (n < 32) : (n += 1) {
        square.*[n] = gf2MatrixTimes(mat, mat.*[n]);
    }
}

/// Combines two independently computed CRC32C checksums `crc1 = crc32c(buf1)` and
/// `crc2 = crc32c(buf2)` into `crc32c(buf1 ++ buf2)` in O(log(len2)) time without
/// re-reading `buf1` or `buf2` (standard GF(2) matrix squaring, ideal for parallel/chunked I/O).
pub fn crc32cCombine(crc1_in: u32, crc2: u32, len2_in: u64): u32 {
    if (len2_in == 0) return crc1_in;

    let even: [32]u32 = [0; 32];
    let odd: [32]u32 = [0; 32];

    odd[0] = CASTAGNOLI_POLY;
    let row: u32 = 1;
    let n: usize = 1;
    while (n < 32) : (n += 1) {
        odd[n] = row;
        row <<= 1;
    }

    gf2MatrixSquare(&even, &odd);
    gf2MatrixSquare(&odd, &even);

    let crc1 = crc1_in;
    let len2 = len2_in;
    while (len2 != 0) {
        gf2MatrixSquare(&even, &odd);
        if ((len2 & 1) != 0) {
            crc1 = gf2MatrixTimes(&even, crc1);
        }
        len2 >>= 1;
        if (len2 == 0) break;

        gf2MatrixSquare(&odd, &even);
        if ((len2 & 1) != 0) {
            crc1 = gf2MatrixTimes(&odd, crc1);
        }
        len2 >>= 1;
    }

    return crc1 ^ crc2;
}

/// Streaming CRC32C hasher with precomputed 256-entry table (zero heap allocation).
pub struct Crc32cHasher {
    state: u32 = 0xFFFFFFFF,
    total_len: u64 = 0,
    table: [256]u32 = [0; 256],

    pub static fn init(): Crc32cHasher {
        return Crc32cHasher{
            state: 0xFFFFFFFF,
            total_len: 0,
            table: buildTable(),
        };
    }

    pub fn update(*_, data: []const u8) {
        let crc = _.state;
        let i: usize = 0;
        while (i < data.len) : (i += 1) {
            const idx: usize = ((crc ^ (data[i] as u32)) & 0xFF) as usize;
            crc = (crc >> 8) ^ _.table[idx];
        }
        _.state = crc;
        _.total_len += data.len as u64;
    }

    /// Returns the finalized CRC32C checksum (non-destructive; further `update` calls remain valid).
    pub fn finalize(): u32 {
        return ~_.state;
    }

    /// Returns the LevelDB/TFRecord masked CRC32C checksum.
    pub fn finalizeMasked(): u32 {
        return mask(~_.state);
    }

    pub fn bytesHashed(): u64 {
        return _.total_len;
    }

    pub fn reset(*_) {
        _.state = 0xFFFFFFFF;
        _.total_len = 0;
    }
}
