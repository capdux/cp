// @cap/crc64 — Zero-allocation 64-bit CRC engine (ECMA-182, XZ, GO-ISO) for CP
// Clean-room MIT implementation inspired by Rust `crc` and `crc64fast`.
//
// Supported standards & official `"123456789"` check values:
// - ECMA-182 (`crc64Ecma`, `initEcma`):  0x6C40DF5F0B497347
// - CRC-64/XZ (`crc64Xz`, `initXz`):     0x995DC9BBDF1939FA
// - CRC-64/GO-ISO (`crc64Iso`, `initIso`): 0xB90956C775A41001
//
// Uses 256-entry stack lookup tables (2KB) for fast byte-at-a-time streaming with zero heap allocation.

fn makeU64(hi: u32, lo: u32) -> u64 {
    return ((hi as u64) << 32) | (lo as u64);
}

/// Streaming 64-bit CRC hasher with precomputed 256-entry lookup table (zero heap allocation).
pub struct Crc64Hasher {
    pub table: [256]u64,
    pub crc: u64,
    pub init_val: u64,
    pub xorout: u64,
    pub reflected: bool,

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
        };
    }

    /// Initializes a CRC-64/ECMA-182 hasher (normal poly `0x42F0E1EBA9EA3693`).
    pub static fn initEcma() -> Crc64Hasher {
        const poly = makeU64(0x42F0E1EB as u32, 0xA9EA3693 as u32);
        return Crc64Hasher.initCustom(poly, 0 as u64, 0 as u64, false);
    }

    /// Initializes a CRC-64/XZ hasher (reflected poly `0xC96C5795D7870F42`, init/xorout `~0`).
    pub static fn initXz() -> Crc64Hasher {
        const poly = makeU64(0xC96C5795 as u32, 0xD7870F42 as u32);
        const all_ones = makeU64(0xFFFFFFFF as u32, 0xFFFFFFFF as u32);
        return Crc64Hasher.initCustom(poly, all_ones, all_ones, true);
    }

    /// Initializes a CRC-64/GO-ISO hasher (reflected poly `0xD800000000000000`, init/xorout `~0`).
    pub static fn initIso() -> Crc64Hasher {
        const poly = makeU64(0xD8000000 as u32, 0x00000000 as u32);
        const all_ones = makeU64(0xFFFFFFFF as u32, 0xFFFFFFFF as u32);
        return Crc64Hasher.initCustom(poly, all_ones, all_ones, true);
    }

    /// Feeds a chunk of bytes into the running CRC-64 state.
    pub fn update(*_, data: []const u8) {
        let i: usize = 0;
        if (_.reflected) {
            while (i < data.len) {
                const idx = ((_.crc ^ (data[i] as u64)) & (0xFF as u64)) as usize;
                _.crc = (_.crc >> 8) ^ _.table[idx];
                i += 1;
            }
        } else {
            while (i < data.len) {
                const idx = (((_.crc >> 56) ^ (data[i] as u64)) & (0xFF as u64)) as usize;
                _.crc = (_.crc << 8) ^ _.table[idx];
                i += 1;
            }
        }
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
    let h = Crc64Hasher.initEcma();
    h.update(data);
    return h.finalize();
}

/// One-shot CRC-64/XZ checksum.
pub fn crc64Xz(data: []const u8) -> u64 {
    let h = Crc64Hasher.initXz();
    h.update(data);
    return h.finalize();
}

/// One-shot CRC-64/GO-ISO checksum.
pub fn crc64Iso(data: []const u8) -> u64 {
    let h = Crc64Hasher.initIso();
    h.update(data);
    return h.finalize();
}
