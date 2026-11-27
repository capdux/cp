// @cap/crc32fast — Slice-by-8 IEEE CRC32 hasher in pure CP
// Accelerated with comptime [8][256]u32 LUT (8 bytes per hot-loop iteration) and undefined stack buffers.

pub const CRC32_POLY_REVERSED: u32 = 3988292384; // 0xEDB88320

fn crcByteStep(crc_in: u32, b: u8): u32 {
    let c: u32 = crc_in ^ (b as u32);
    c = (c >> 1) ^ (CRC32_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32_POLY_REVERSED & (0 -% (c & 1)));
    c = (c >> 1) ^ (CRC32_POLY_REVERSED & (0 -% (c & 1)));
    return c;
}

fn crcBuildTables(): [8][256]u32 {
    @setEvalBranchQuota(80000);
    let tbl: [8][256]u32 = undefined;
    let i: usize = 0;
    while (i < 256) {
        tbl[0][i] = crcByteStep(0, i as u8);
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

const CRC32_LUT: [8][256]u32 = crcBuildTables();

fn crcGf2MatrixTimes(mat: [32]u32, vec: u32): u32 {
    let sum: u32 = 0;
    let v = vec;
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

fn crcGf2MatrixSquare(src: [32]u32): [32]u32 {
    let out: [32]u32 = undefined;
    let n: usize = 0;
    while (n < 32) {
        out[n] = crcGf2MatrixTimes(src, src[n]);
        n = n + 1;
    }
    return out;
}

pub fn combineCrc32(crc1: u32, crc2: u32, len2: u64): u32 {
    if (len2 == 0) { return crc1; }
    let odd: [32]u32 = undefined;
    odd[0] = CRC32_POLY_REVERSED;
    let row: u32 = 1;
    let n: usize = 1;
    while (n < 32) {
        odd[n] = row;
        row = row << 1;
        n = n + 1;
    }
    let even = crcGf2MatrixSquare(odd);
    odd = crcGf2MatrixSquare(even);

    let c1 = crc1;
    let rem = len2;
    while (rem != 0) {
        even = crcGf2MatrixSquare(odd);
        if ((rem & 1) != 0) {
            c1 = crcGf2MatrixTimes(even, c1);
        }
        rem = rem >> 1;
        if (rem == 0) { break; }
        odd = crcGf2MatrixSquare(even);
        if ((rem & 1) != 0) {
            c1 = crcGf2MatrixTimes(odd, c1);
        }
        rem = rem >> 1;
    }
    return c1 ^ crc2;
}

fn crc32UpdateSlice(state_in: u32, buf: []const u8): u32 {
    let c: u32 = state_in ^ 4294967295; // 0xFFFFFFFF
    let i: usize = 0;
    // Slice-by-8 hot loop: 8 bytes per iteration
    while (i + 8 <= buf.len) {
        const lo: u32 = (buf[i] as u32)
            | ((buf[i + 1] as u32) << 8)
            | ((buf[i + 2] as u32) << 16)
            | ((buf[i + 3] as u32) << 24);
        const x: u32 = c ^ lo;
        c = CRC32_LUT[7][(x & 255) as usize]
            ^ CRC32_LUT[6][((x >> 8) & 255) as usize]
            ^ CRC32_LUT[5][((x >> 16) & 255) as usize]
            ^ CRC32_LUT[4][((x >> 24) & 255) as usize]
            ^ CRC32_LUT[3][buf[i + 4] as usize]
            ^ CRC32_LUT[2][buf[i + 5] as usize]
            ^ CRC32_LUT[1][buf[i + 6] as usize]
            ^ CRC32_LUT[0][buf[i + 7] as usize];
        i = i + 8;
    }
    // Slice-by-4 tail
    if (i + 4 <= buf.len) {
        const w: u32 = (buf[i] as u32)
            | ((buf[i + 1] as u32) << 8)
            | ((buf[i + 2] as u32) << 16)
            | ((buf[i + 3] as u32) << 24);
        const x: u32 = c ^ w;
        c = CRC32_LUT[3][(x & 255) as usize]
            ^ CRC32_LUT[2][((x >> 8) & 255) as usize]
            ^ CRC32_LUT[1][((x >> 16) & 255) as usize]
            ^ CRC32_LUT[0][((x >> 24) & 255) as usize];
        i = i + 4;
    }
    while (i < buf.len) {
        const idx: usize = ((c ^ (buf[i] as u32)) & 255) as usize;
        c = (c >> 8) ^ CRC32_LUT[0][idx];
        i = i + 1;
    }
    return c ^ 4294967295;
}

pub struct Crc32Hasher {
    pub state: u32,
    pub amount: u64,

    pub static fn new(): Crc32Hasher {
        return Crc32Hasher { state: 0, amount: 0 };
    }

    pub static fn newWithInitial(init_crc: u32): Crc32Hasher {
        return Crc32Hasher { state: init_crc, amount: 0 };
    }

    pub fn update(*_, buf: []const u8) {
        _.state = crc32UpdateSlice(_.state, buf);
        _.amount = _.amount + (buf.len as u64);
    }

    pub fn updateStr(*_, s: string) {
        _.state = crc32UpdateSlice(_.state, s);
        _.amount = _.amount + (s.len as u64);
    }

    pub fn finalize(): u32 {
        return _.state;
    }

    pub fn reset(*_) {
        _.state = 0;
        _.amount = 0;
    }

    pub fn combine(*_, other: Crc32Hasher) {
        _.state = combineCrc32(_.state, other.state, other.amount);
        _.amount = _.amount + other.amount;
    }
}

pub fn hashBytes(buf: []const u8): u32 {
    return crc32UpdateSlice(0, buf);
}

pub fn hashStr(s: string): u32 {
    return crc32UpdateSlice(0, s);
}
