// Source: cp/lib/crc/crc.cp (Pure CP Full-Spectrum CRC-8/16/32/32C/64 Checksums)
const std = @import("std");

pub fn crc8Smbus(data: []const u8): u8 {
    let crc: u8 = 0x00;
    for (b of data) {
        crc ^= b;
        let i: u8 = 0;
        while (i < 8) : (i += 1) {
            if ((crc & 0x80) != 0) {
                crc = (crc << 1) ^ 0x07;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

pub fn crc16CcittFalse(data: []const u8): u16 {
    let crc: u16 = 0xFFFF;
    for (b of data) {
        crc ^= (@as(u16, b) << 8);
        let i: u8 = 0;
        while (i < 8) : (i += 1) {
            if ((crc & 0x8000) != 0) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

pub fn crc16Xmodem(data: []const u8): u16 {
    let crc: u16 = 0x0000;
    for (b of data) {
        crc ^= (@as(u16, b) << 8);
        let i: u8 = 0;
        while (i < 8) : (i += 1) {
            if ((crc & 0x8000) != 0) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc <<= 1;
            }
        }
    }
    return crc;
}

pub fn crc16Modbus(data: []const u8): u16 {
    let crc: u16 = 0xFFFF;
    for (b of data) {
        crc ^= @as(u16, b);
        let i: u8 = 0;
        while (i < 8) : (i += 1) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    return crc;
}

fn makeCrc32Table(poly_rev: u32): [256]u32 {
    @setEvalBranchQuota(5000);
    let tbl: [256]u32 = undefined;
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
        tbl[i] = c;
    }
    return tbl;
}

const CRC32_IEEE_TABLE: [256]u32 = makeCrc32Table(0xEDB88320);
const CRC32C_ISCSI_TABLE: [256]u32 = makeCrc32Table(0x82F63B78);

pub const Crc32Stream = struct {
    state: u32,
    is_iscsi: bool,

    pub fn initIeee(): Crc32Stream {
        return Crc32Stream{ .state = 0xFFFFFFFF, .is_iscsi = false };
    }

    pub fn initCastagnoli(): Crc32Stream {
        return Crc32Stream{ .state = 0xFFFFFFFF, .is_iscsi = true };
    }

    pub fn update(self: *Crc32Stream, data: []const u8) {
        let c = self.state;
        if (self.is_iscsi) {
            for (b of data) {
                c = CRC32C_ISCSI_TABLE[(c ^ b) & 0xFF] ^ (c >> 8);
            }
        } else {
            for (b of data) {
                c = CRC32_IEEE_TABLE[(c ^ b) & 0xFF] ^ (c >> 8);
            }
        }
        self.state = c;
    }

    pub fn final(self: Crc32Stream): u32 {
        return self.state ^ 0xFFFFFFFF;
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

fn makeCrc64Table(poly_rev: u64): [256]u64 {
    @setEvalBranchQuota(5000);
    let tbl: [256]u64 = undefined;
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
        tbl[i] = c;
    }
    return tbl;
}

const CRC64_XZ_TABLE: [256]u64 = makeCrc64Table(0xC96C5795D7870F42);
const CRC64_ECMA_TABLE: [256]u64 = makeCrc64Table(0xC96C5795D7870F42);

pub const Crc64Stream = struct {
    state: u64,
    is_ecma: bool,

    pub fn initXz(): Crc64Stream {
        return Crc64Stream{ .state = 0xFFFFFFFF_FFFFFFFF, .is_ecma = false };
    }

    pub fn initEcma182(): Crc64Stream {
        return Crc64Stream{ .state = 0, .is_ecma = true };
    }

    pub fn update(self: *Crc64Stream, data: []const u8) {
        let c = self.state;
        if (self.is_ecma) {
            for (b of data) {
                c = CRC64_ECMA_TABLE[(c ^ b) & 0xFF] ^ (c >> 8);
            }
        } else {
            for (b of data) {
                c = CRC64_XZ_TABLE[(c ^ b) & 0xFF] ^ (c >> 8);
            }
        }
        self.state = c;
    }

    pub fn final(self: Crc64Stream): u64 {
        return if (self.is_ecma) self.state else (self.state ^ 0xFFFFFFFF_FFFFFFFF);
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
