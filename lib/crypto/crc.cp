// Source: cp/lib/crc/crc.cp (Pure CP Full-Spectrum CRC-8/16/32/32C/64 Checksums)
const std = @import("std");

fn makeCrc8Table(poly: u8): [256]u8 {
    @setEvalBranchQuota(20000);
    let tbl: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let c: u8 = @intCast(i);
        let j: usize = 0;
        while (j < 8) : (j += 1) {
            if ((c & 0x80) != 0) {
                c = (c << 1) ^ poly;
            } else {
                c <<= 1;
            }
        }
        tbl[i] = c;
    }
    return tbl;
}

fn makeCrc16MsbTable(poly: u16): [256]u16 {
    @setEvalBranchQuota(20000);
    let tbl: [256]u16 = undefined;
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let c: u16 = @as(u16, @intCast(i)) << 8;
        let j: usize = 0;
        while (j < 8) : (j += 1) {
            if ((c & 0x8000) != 0) {
                c = (c << 1) ^ poly;
            } else {
                c <<= 1;
            }
        }
        tbl[i] = c;
    }
    return tbl;
}

fn makeCrc16LsbTable(poly_rev: u16): [256]u16 {
    @setEvalBranchQuota(20000);
    let tbl: [256]u16 = undefined;
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let c: u16 = @intCast(i);
        let j: usize = 0;
        while (j < 8) : (j += 1) {
            if ((c & 1) != 0) {
                c = (c >> 1) ^ poly_rev;
            } else {
                c >>= 1;
            }
        }
        tbl[i] = c;
    }
    return tbl;
}

const CRC8_SMBUS_TABLE: [256]u8 = makeCrc8Table(0x07);
const CRC16_CCITT_TABLE: [256]u16 = makeCrc16MsbTable(0x1021);
const CRC16_MODBUS_TABLE: [256]u16 = makeCrc16LsbTable(0xA001);

pub fn crc8Smbus(data: []const u8): u8 {
    let crc: u8 = 0x00;
    for (b of data) {
        crc = CRC8_SMBUS_TABLE[crc ^ b];
    }
    return crc;
}

pub fn crc16CcittFalse(data: []const u8): u16 {
    let crc: u16 = 0xFFFF;
    for (b of data) {
        crc = (crc << 8) ^ CRC16_CCITT_TABLE[((crc >> 8) ^ @as(u16, b)) & 0xFF];
    }
    return crc;
}

pub fn crc16Xmodem(data: []const u8): u16 {
    let crc: u16 = 0x0000;
    for (b of data) {
        crc = (crc << 8) ^ CRC16_CCITT_TABLE[((crc >> 8) ^ @as(u16, b)) & 0xFF];
    }
    return crc;
}

pub fn crc16Modbus(data: []const u8): u16 {
    let crc: u16 = 0xFFFF;
    for (b of data) {
        crc = (crc >> 8) ^ CRC16_MODBUS_TABLE[(crc ^ @as(u16, b)) & 0xFF];
    }
    return crc;
}

fn makeCrc32Slice4Table(poly_rev: u32): [4][256]u32 {
    @setEvalBranchQuota(40000);
    let tbl: [4][256]u32 = undefined;
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let c: u32 = @intCast(i);
        let j: usize = 0;
        while (j < 8) : (j += 1) {
            if ((c & 1) != 0) {
                c = (c >> 1) ^ poly_rev;
            } else {
                c >>= 1;
            }
        }
        tbl[0][i] = c;
    }
    let k: usize = 1;
    while (k < 4) : (k += 1) {
        let m: usize = 0;
        while (m < 256) : (m += 1) {
            const prev = tbl[k - 1][m];
            tbl[k][m] = (prev >> 8) ^ tbl[0][prev & 0xFF];
        }
    }
    return tbl;
}

const CRC32_IEEE_TABLE4: [4][256]u32 = makeCrc32Slice4Table(0xEDB88320);
const CRC32C_ISCSI_TABLE4: [4][256]u32 = makeCrc32Slice4Table(0x82F63B78);

fn crc32UpdateSlice4(init_state: u32, data: []const u8, tbl: *const [4][256]u32): u32 {
    let c: u32 = init_state;
    let i: usize = 0;
    while (i + 4 <= data.len) : (i += 4) {
        const word: u32 = @as(u32, data[i]) |
            (@as(u32, data[i + 1]) << 8) |
            (@as(u32, data[i + 2]) << 16) |
            (@as(u32, data[i + 3]) << 24);
        c ^= word;
        c = tbl[3][c & 0xFF] ^
            tbl[2][(c >> 8) & 0xFF] ^
            tbl[1][(c >> 16) & 0xFF] ^
            tbl[0][(c >> 24) & 0xFF];
    }
    while (i < data.len) : (i += 1) {
        c = tbl[0][(c ^ @as(u32, data[i])) & 0xFF] ^ (c >> 8);
    }
    return c;
}

pub const Crc32Stream = struct {
    state: u32,
    is_iscsi: bool,

    pub static fn initIeee(): Crc32Stream {
        return Crc32Stream{ .state = 0xFFFFFFFF, .is_iscsi = false };
    }

    pub static fn initCastagnoli(): Crc32Stream {
        return Crc32Stream{ .state = 0xFFFFFFFF, .is_iscsi = true };
    }

    pub fn update(*_, data: []const u8) {
        if (_.is_iscsi) {
            _.state = crc32UpdateSlice4(_.state, data, &CRC32C_ISCSI_TABLE4);
        } else {
            _.state = crc32UpdateSlice4(_.state, data, &CRC32_IEEE_TABLE4);
        }
    }

    pub fn final(): u32 {
        return _.state ^ 0xFFFFFFFF;
    }
};

pub fn crc32Ieee(data: []const u8): u32 {
    let st = Crc32Stream.initIeee();
    st.update(data);
    return st.final();
}

pub fn crc32Castagnoli(data: []const u8): u32 {
    let st = Crc32Stream.initCastagnoli();
    st.update(data);
    return st.final();
}

pub fn crc32Combine(crc1: u32, crc2: u32, len2: usize): u32 {
    // GF(2) matrix operator for combining two CRC-32/ISO-HDLC checksums in O(log len2)
    if (len2 == 0) { return crc1; }
    let odd: [32]u32 = undefined;
    let even: [32]u32 = undefined;
    odd[0] = 0xEDB88320;
    let row: u32 = 1;
    let n: usize = 1;
    while (n < 32) : (n += 1) {
        odd[n] = row;
        row <<= 1;
    }

    gf2MatrixSquare(&even, &odd);
    gf2MatrixSquare(&odd, &even);

    let c1 = crc1;
    let l2 = len2;
    while (true) {
        gf2MatrixSquare(&even, &odd);
        if ((l2 & 1) != 0) {
            c1 = gf2MatrixTimes(&even, c1);
        }
        l2 >>= 1;
        if (l2 == 0) { break; }

        gf2MatrixSquare(&odd, &even);
        if ((l2 & 1) != 0) {
            c1 = gf2MatrixTimes(&odd, c1);
        }
        l2 >>= 1;
        if (l2 == 0) { break; }
    }
    return c1 ^ crc2;
}

fn gf2MatrixTimes(mat: *const [32]u32, vec_in: u32): u32 {
    let sum: u32 = 0;
    let vec = vec_in;
    let idx: usize = 0;
    while (vec != 0) : (idx += 1) {
        if ((vec & 1) != 0) {
            sum ^= mat[idx];
        }
        vec >>= 1;
    }
    return sum;
}

fn gf2MatrixSquare(square: *[32]u32, mat: *const [32]u32) {
    let n: usize = 0;
    while (n < 32) : (n += 1) {
        square[n] = gf2MatrixTimes(mat, mat[n]);
    }
}

fn makeCrc64Slice4Table(poly_rev: u64): [4][256]u64 {
    @setEvalBranchQuota(40000);
    let tbl: [4][256]u64 = undefined;
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        let c: u64 = @intCast(i);
        let j: usize = 0;
        while (j < 8) : (j += 1) {
            if ((c & 1) != 0) {
                c = (c >> 1) ^ poly_rev;
            } else {
                c >>= 1;
            }
        }
        tbl[0][i] = c;
    }
    let k: usize = 1;
    while (k < 4) : (k += 1) {
        let m: usize = 0;
        while (m < 256) : (m += 1) {
            const prev = tbl[k - 1][m];
            tbl[k][m] = (prev >> 8) ^ tbl[0][@as(usize, @intCast(prev & 0xFF))];
        }
    }
    return tbl;
}

const CRC64_XZ_POLY: u64 = (@as(u64, 0xC96C5795) << 32) | @as(u64, 0xD7870F42);
const CRC64_ALL_ONES: u64 = ~@as(u64, 0);
const CRC64_XZ_TABLE4: [4][256]u64 = makeCrc64Slice4Table(CRC64_XZ_POLY);

pub const Crc64Stream = struct {
    state: u64,
    is_ecma: bool,

    pub static fn initXz(): Crc64Stream {
        return Crc64Stream{ .state = CRC64_ALL_ONES, .is_ecma = false };
    }

    pub static fn initEcma182(): Crc64Stream {
        return Crc64Stream{ .state = 0, .is_ecma = true };
    }

    pub fn update(*_, data: []const u8) {
        let c: u64 = _.state;
        let i: usize = 0;
        while (i + 4 <= data.len) : (i += 4) {
            const word: u64 = @as(u64, data[i]) |
                (@as(u64, data[i + 1]) << 8) |
                (@as(u64, data[i + 2]) << 16) |
                (@as(u64, data[i + 3]) << 24);
            const x: u64 = c ^ word;
            c = (c >> 32) ^
                CRC64_XZ_TABLE4[3][@as(usize, @intCast(x & 0xFF))] ^
                CRC64_XZ_TABLE4[2][@as(usize, @intCast((x >> 8) & 0xFF))] ^
                CRC64_XZ_TABLE4[1][@as(usize, @intCast((x >> 16) & 0xFF))] ^
                CRC64_XZ_TABLE4[0][@as(usize, @intCast((x >> 24) & 0xFF))];
        }
        while (i < data.len) : (i += 1) {
            c = CRC64_XZ_TABLE4[0][@as(usize, @intCast((c ^ @as(u64, data[i])) & 0xFF))] ^ (c >> 8);
        }
        _.state = c;
    }

    pub fn final(): u64 {
        return if (_.is_ecma) _.state else (_.state ^ CRC64_ALL_ONES);
    }
};

pub fn crc64Xz(data: []const u8): u64 {
    let st = Crc64Stream.initXz();
    st.update(data);
    return st.final();
}

pub fn crc64Ecma182(data: []const u8): u64 {
    let st = Crc64Stream.initEcma182();
    st.update(data);
    return st.final();
}
