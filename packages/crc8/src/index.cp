// @cap/crc8 — Zero-allocation CRC-8 Checksum Family
// Clean-room MIT implementation in pure CP.
// Supports SMBus, Maxim/Dallas 1-Wire, SAE-J1850, AUTOSAR, CDMA2000, Bluetooth HEC & custom polynomials.

fn reflect8(b: u8) -> u8 {
    let x = b;
    x = ((x & 0xF0) >> 4) | ((x & 0x0F) << 4);
    x = ((x & 0xCC) >> 2) | ((x & 0x33) << 2);
    x = ((x & 0xAA) >> 1) | ((x & 0x55) << 1);
    return x;
}

pub struct Crc8Hasher {
    table: [256]u8,
    state: u8,
    init_val: u8,
    ref_in: bool,
    ref_out: bool,
    xor_out: u8,

    /// Creates a custom CRC-8 hasher with a precomputed 256-entry lookup table.
    pub static fn initCustom(poly: u8, init_val: u8, ref_in: bool, ref_out: bool, xor_out: u8) -> Crc8Hasher {
        let tbl: [256]u8 = undefined;
        let i: usize = 0;
        if (ref_in) {
            const rpoly = reflect8(poly);
            while (i < 256) {
                let crc = i as u8;
                let bit: usize = 0;
                while (bit < 8) {
                    if ((crc & 1) != 0) {
                        crc = (crc >> 1) ^ rpoly;
                    } else {
                        crc = crc >> 1;
                    }
                    bit += 1;
                }
                tbl[i] = crc;
                i += 1;
            }
        } else {
            while (i < 256) {
                let crc = i as u8;
                let bit: usize = 0;
                while (bit < 8) {
                    if ((crc & 0x80) != 0) {
                        crc = (crc << 1) ^ poly;
                    } else {
                        crc = crc << 1;
                    }
                    bit += 1;
                }
                tbl[i] = crc;
                i += 1;
            }
        }

        let start_state = init_val;
        if (ref_in) {
            start_state = reflect8(init_val);
        }

        return Crc8Hasher {
            table: tbl,
            state: start_state,
            init_val: init_val,
            ref_in: ref_in,
            ref_out: ref_out,
            xor_out: xor_out,
        };
    }

    /// CRC-8/SMBUS (poly=0x07, init=0x00, refIn=false, refOut=false, xorOut=0x00, check=0xF4).
    pub static fn initSmbus() -> Crc8Hasher {
        return Crc8Hasher.initCustom(0x07, 0x00, false, false, 0x00);
    }

    /// CRC-8/MAXIM-DOW (1-Wire) (poly=0x31, init=0x00, refIn=true, refOut=true, xorOut=0x00, check=0xA1).
    pub static fn initMaxim() -> Crc8Hasher {
        return Crc8Hasher.initCustom(0x31, 0x00, true, true, 0x00);
    }

    /// CRC-8/SAE-J1850 (poly=0x1D, init=0xFF, refIn=false, refOut=false, xorOut=0xFF, check=0x4B).
    pub static fn initSaeJ1850() -> Crc8Hasher {
        return Crc8Hasher.initCustom(0x1D, 0xFF, false, false, 0xFF);
    }

    /// CRC-8/AUTOSAR (poly=0x2F, init=0xFF, refIn=false, refOut=false, xorOut=0xFF, check=0xDF).
    pub static fn initAutosar() -> Crc8Hasher {
        return Crc8Hasher.initCustom(0x2F, 0xFF, false, false, 0xFF);
    }

    /// CRC-8/CDMA2000 (poly=0x9B, init=0xFF, refIn=false, refOut=false, xorOut=0x00, check=0xDA).
    pub static fn initCdma2000() -> Crc8Hasher {
        return Crc8Hasher.initCustom(0x9B, 0xFF, false, false, 0x00);
    }

    /// CRC-8/BLUETOOTH (HEC) (poly=0xA7, init=0x00, refIn=true, refOut=true, xorOut=0x00, check=0x26).
    pub static fn initBluetooth() -> Crc8Hasher {
        return Crc8Hasher.initCustom(0xA7, 0x00, true, true, 0x00);
    }

    /// Feeds a byte slice into the running CRC-8 state.
    pub fn update(*_, data: []const u8) {
        let crc: u8 = _.state;
        let i: usize = 0;
        while (i + 4 <= data.len) {
            crc = _.table[(crc ^ data[i]) as usize];
            crc = _.table[(crc ^ data[i + 1]) as usize];
            crc = _.table[(crc ^ data[i + 2]) as usize];
            crc = _.table[(crc ^ data[i + 3]) as usize];
            i += 4;
        }
        while (i < data.len) : (i += 1) {
            crc = _.table[(crc ^ data[i]) as usize];
        }
        _.state = crc;
    }

    /// Returns the finalized 8-bit CRC value without mutating internal state.
    pub fn finalize() -> u8 {
        let out = _.state;
        if (_.ref_in != _.ref_out) {
            out = reflect8(out);
        }
        return out ^ _.xor_out;
    }

    /// Resets the hasher back to its initial state.
    pub fn reset(*_) {
        if (_.ref_in) {
            _.state = reflect8(_.init_val);
        } else {
            _.state = _.init_val;
        }
    }
}

/// One-shot CRC-8/SMBUS checksum.
pub fn crc8Smbus(data: []const u8) -> u8 {
    let h = Crc8Hasher.initSmbus();
    h.update(data);
    return h.finalize();
}

/// One-shot CRC-8/MAXIM-DOW (1-Wire) checksum.
pub fn crc8Maxim(data: []const u8) -> u8 {
    let h = Crc8Hasher.initMaxim();
    h.update(data);
    return h.finalize();
}

/// One-shot CRC-8/SAE-J1850 checksum.
pub fn crc8SaeJ1850(data: []const u8) -> u8 {
    let h = Crc8Hasher.initSaeJ1850();
    h.update(data);
    return h.finalize();
}

/// One-shot CRC-8/AUTOSAR checksum.
pub fn crc8Autosar(data: []const u8) -> u8 {
    let h = Crc8Hasher.initAutosar();
    h.update(data);
    return h.finalize();
}

/// One-shot CRC-8/CDMA2000 checksum.
pub fn crc8Cdma2000(data: []const u8) -> u8 {
    let h = Crc8Hasher.initCdma2000();
    h.update(data);
    return h.finalize();
}

/// One-shot CRC-8/BLUETOOTH (HEC) checksum.
pub fn crc8Bluetooth(data: []const u8) -> u8 {
    let h = Crc8Hasher.initBluetooth();
    h.update(data);
    return h.finalize();
}
