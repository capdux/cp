// @cap/crc64 — Zero-allocation 64-bit CRC engine (ECMA-182, XZ, GO-ISO) for CP
// Clean-room MIT implementation inspired by Rust `crc` and `crc64fast`.
//
// Supported standards & official `"123456789"` check values:
// - ECMA-182 (`crc64Ecma`, `initEcma`):  0x6C40DF5F0B497347
// - CRC-64/XZ (`crc64Xz`, `initXz`):     0x995DC9BBDF1939FA
// - CRC-64/GO-ISO (`crc64Iso`, `initIso`): 0xB90956C775A41001
//
// Uses comptime Slice-by-4 (4x256 LUT) for 4-byte/iter streaming with zero heap allocation.

fn makeU64(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

fn buildSlice4Table(poly: u64, reflected: bool) -> [4][256]u64 {
    @setEvalBranchQuota(40000);
    let tbl: [4][256]u64 = undefined;
    const msb: u64 = (1 as u64) << 63;

    let i: usize = 0;
    while (i < 256) {
        if (reflected) {
            let c: u64 = i as u64;
            let bit: u32 = 0;
            while (bit < 8) {
                if ((c & 1) != 0) {
                    c = (c >> 1) ^ poly;
                } else {
                    c = c >> 1;
                }
                bit += 1;
            }
            tbl[0][i] = c;
        } else {
            let c: u64 = (i as u64) << 56;
            let bit: u32 = 0;
            while (bit < 8) {
                if ((c & msb) != 0) {
                    c = (c << 1) ^ poly;
                } else {
                    c = c << 1;
                }
                bit += 1;
            }
            tbl[0][i] = c;
        }
        i += 1;
    }

    let t: usize = 1;
    while (t < 4) {
        let j: usize = 0;
        while (j < 256) {
            const prev = tbl[t - 1][j];
            if (reflected) {
                tbl[t][j] = (prev >> 8) ^ tbl[0][(prev & (0xFF as u64)) as usize];
            } else {
                tbl[t][j] = (prev << 8) ^ tbl[0][((prev >> 56) & (0xFF as u64)) as usize];
            }
            j += 1;
        }
        t += 1;
    }
    return tbl;
}

const CRC64_ECMA_LUT: [4][256]u64 = buildSlice4Table(makeU64(0x42F0E1EB as u32, 0xA9EA3693 as u32), false);
const CRC64_XZ_LUT: [4][256]u64 = buildSlice4Table(makeU64(0xC96C5795 as u32, 0xD7870F42 as u32), true);
const CRC64_ISO_LUT: [4][256]u64 = buildSlice4Table(makeU64(0xD8000000 as u32, 0x00000000 as u32), true);

fn updateReflectedSlice4(crc_in: u64, data: []const u8, tbl: [4][256]u64) -> u64 {
    let c = crc_in;
    let i: usize = 0;
    while (i + 4 <= data.len) {
        const w: u32 = (data[i] as u32)
            | ((data[i + 1] as u32) << 8)
            | ((data[i + 2] as u32) << 16)
            | ((data[i + 3] as u32) << 24);
        const x: u32 = ((c & (0xFFFFFFFF as u64)) as u32) ^ w;
        c = (c >> 32)
            ^ tbl[3][(x & 0xFF) as usize]
            ^ tbl[2][((x >> 8) & 0xFF) as usize]
            ^ tbl[1][((x >> 16) & 0xFF) as usize]
            ^ tbl[0][((x >> 24) & 0xFF) as usize];
        i += 4;
    }
    while (i < data.len) {
        const idx = ((c ^ (data[i] as u64)) & (0xFF as u64)) as usize;
        c = (c >> 8) ^ tbl[0][idx];
        i += 1;
    }
    return c;
}

fn updateNormalSlice4(crc_in: u64, data: []const u8, tbl: [4][256]u64) -> u64 {
    let c = crc_in;
    let i: usize = 0;
    while (i + 4 <= data.len) {
        const w: u32 = ((data[i] as u32) << 24)
            | ((data[i + 1] as u32) << 16)
            | ((data[i + 2] as u32) << 8)
            | (data[i + 3] as u32);
        const x: u32 = ((c >> 32) as u32) ^ w;
        c = (c << 32)
            ^ tbl[3][((x >> 24) & 0xFF) as usize]
            ^ tbl[2][((x >> 16) & 0xFF) as usize]
            ^ tbl[1][((x >> 8) & 0xFF) as usize]
            ^ tbl[0][(x & 0xFF) as usize];
        i += 4;
    }
    while (i < data.len) {
        const idx = (((c >> 56) ^ (data[i] as u64)) & (0xFF as u64)) as usize;
        c = (c << 8) ^ tbl[0][idx];
        i += 1;
    }
    return c;
}

/// Streaming 64-bit CRC hasher backed by comptime Slice-by-4 lookup tables (zero heap allocation).
pub struct Crc64Hasher {
    pub table: [256]u64,
    pub crc: u64,
    pub init_val: u64,
    pub xorout: u64,
    pub reflected: bool,
    preset: u8, // 0=custom, 1=ecma, 2=xz, 3=iso

    static fn initCustom(poly: u64, init_val: u64, xorout: u64, reflected: bool) -> Crc64Hasher {
        let tbl: [256]u64 = undefined;
        const msb: u64 = (1 as u64) << 63;

        let i: usize = 0;
        while (i < 256) {
            if (reflected) {
                let c: u64 = i as u64;
                let bit: u32 = 0;
                while (bit < 8) {
                    if ((c & 1) != 0) {
                        c = (c >> 1) ^ poly;
                    } else {
                        c = c >> 1;
                    }
                    bit += 1;
                }
                tbl[i] = c;
            } else {
                let c: u64 = (i as u64) << 56;
                let bit: u32 = 0;
                while (bit < 8) {
                    if ((c & msb) != 0) {
                        c = (c << 1) ^ poly;
                    } else {
                        c = c << 1;
                    }
                    bit += 1;
                }
                tbl[i] = c;
            }
            i += 1;
        }

        return Crc64Hasher{
            table: tbl,
            crc: init_val,
            init_val: init_val,
            xorout: xorout,
            reflected: reflected,
            preset: 0,
        };
    }

    /// Initializes a CRC-64/ECMA-182 hasher (normal poly `0x42F0E1EBA9EA3693`).
    pub static fn initEcma() -> Crc64Hasher {
        return Crc64Hasher{
            table: CRC64_ECMA_LUT[0],
            crc: 0 as u64,
            init_val: 0 as u64,
            xorout: 0 as u64,
            reflected: false,
            preset: 1,
        };
    }

    /// Initializes a CRC-64/XZ hasher (reflected poly `0xC96C5795D7870F42`, init/xorout `~0`).
    pub static fn initXz() -> Crc64Hasher {
        const all_ones = makeU64(0xFFFFFFFF as u32, 0xFFFFFFFF as u32);
        return Crc64Hasher{
            table: CRC64_XZ_LUT[0],
            crc: all_ones,
            init_val: all_ones,
            xorout: all_ones,
            reflected: true,
            preset: 2,
        };
    }

    /// Initializes a CRC-64/GO-ISO hasher (reflected poly `0xD800000000000000`, init/xorout `~0`).
    pub static fn initIso() -> Crc64Hasher {
        const all_ones = makeU64(0xFFFFFFFF as u32, 0xFFFFFFFF as u32);
        return Crc64Hasher{
            table: CRC64_ISO_LUT[0],
            crc: all_ones,
            init_val: all_ones,
            xorout: all_ones,
            reflected: true,
            preset: 3,
        };
    }

    /// Feeds a chunk of bytes into the running CRC-64 state.
    pub fn update(*_, data: []const u8) {
        if (_.preset == 1) {
            _.crc = updateNormalSlice4(_.crc, data, CRC64_ECMA_LUT);
            return;
        }
        if (_.preset == 2) {
            _.crc = updateReflectedSlice4(_.crc, data, CRC64_XZ_LUT);
            return;
        }
        if (_.preset == 3) {
            _.crc = updateReflectedSlice4(_.crc, data, CRC64_ISO_LUT);
            return;
        }
        let c = _.crc;
        let i: usize = 0;
        if (_.reflected) {
            while (i < data.len) {
                const idx = ((c ^ (data[i] as u64)) & (0xFF as u64)) as usize;
                c = (c >> 8) ^ _.table[idx];
                i += 1;
            }
        } else {
            while (i < data.len) {
                const idx = (((c >> 56) ^ (data[i] as u64)) & (0xFF as u64)) as usize;
                c = (c << 8) ^ _.table[idx];
                i += 1;
            }
        }
        _.crc = c;
    }

    /// Returns the finalized 64-bit checksum without mutating internal state.
    pub fn finalize() -> u64 {
        return _.crc ^ _.xorout;
    }

    /// Resets the running checksum to its initial value for reuse.
    pub fn reset(*_) {
        _.crc = _.init_val;
    }
}

/// One-shot CRC-64/ECMA-182 checksum.
pub fn crc64Ecma(data: []const u8) -> u64 {
    return updateNormalSlice4(0 as u64, data, CRC64_ECMA_LUT);
}

/// One-shot CRC-64/XZ checksum.
pub fn crc64Xz(data: []const u8) -> u64 {
    const all_ones = makeU64(0xFFFFFFFF as u32, 0xFFFFFFFF as u32);
    return updateReflectedSlice4(all_ones, data, CRC64_XZ_LUT) ^ all_ones;
}

/// One-shot CRC-64/GO-ISO checksum.
pub fn crc64Iso(data: []const u8) -> u64 {
    const all_ones = makeU64(0xFFFFFFFF as u32, 0xFFFFFFFF as u32);
    return updateReflectedSlice4(all_ones, data, CRC64_ISO_LUT) ^ all_ones;
}
