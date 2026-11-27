// @cap/crc16 — Zero-heap 256-entry LUT Industrial CRC-16 Checksum Engine
// Clean-room MIT implementation in pure CP (CCITT-FALSE, XMODEM, MODBUS, KERMIT, ARC, USB).

fn reflectU16(v: u16): u16 {
    let x: u16 = v;
    x = ((x & 0xFF00) >> 8) | ((x & 0x00FF) << 8);
    x = ((x & 0xF0F0) >> 4) | ((x & 0x0F0F) << 4);
    x = ((x & 0xCCCC) >> 2) | ((x & 0x3333) << 2);
    x = ((x & 0xAAAA) >> 1) | ((x & 0x5555) << 1);
    return x;
}

fn crc16BuildTable(poly: u16, ref_in: bool): [256]u16 {
    let tbl: [256]u16 = undefined;
    let i: usize = 0;
    if (ref_in) {
        const rpoly: u16 = reflectU16(poly);
        while (i < 256) : (i += 1) {
            let crc: u16 = i as u16;
            let bit: u8 = 0;
            while (bit < 8) : (bit += 1) {
                if ((crc & 1) != 0) {
                    crc = (crc >> 1) ^ rpoly;
                } else {
                    crc = crc >> 1;
                }
            }
            tbl[i] = crc;
        }
    } else {
        while (i < 256) : (i += 1) {
            let crc: u16 = (i as u16) << 8;
            let bit: u8 = 0;
            while (bit < 8) : (bit += 1) {
                if ((crc & 0x8000) != 0) {
                    crc = (crc << 1) ^ poly;
                } else {
                    crc = crc << 1;
                }
            }
            tbl[i] = crc;
        }
    }
    return tbl;
}

pub struct Crc16Hasher {
    table: [256]u16,
    poly: u16,
    init_val: u16,
    xor_out: u16,
    ref_in: bool,
    ref_out: bool,
    state: u16,

    pub static fn initCustom(poly: u16, init_val: u16, ref_in: bool, ref_out: bool, xor_out: u16): Crc16Hasher {
        const start_state: u16 = if (ref_in) reflectU16(init_val) else init_val;
        return Crc16Hasher {
            table: crc16BuildTable(poly, ref_in),
            poly: poly,
            init_val: init_val,
            xor_out: xor_out,
            ref_in: ref_in,
            ref_out: ref_out,
            state: start_state,
        };
    }

    // CRC-16/IBM-3740 (CCITT-FALSE): check("123456789") == 0x29B1
    pub static fn initCcittFalse(): Crc16Hasher {
        return Crc16Hasher.initCustom(0x1021, 0xFFFF, false, false, 0x0000);
    }

    // CRC-16/XMODEM: check("123456789") == 0x31C3
    pub static fn initXmodem(): Crc16Hasher {
        return Crc16Hasher.initCustom(0x1021, 0x0000, false, false, 0x0000);
    }

    // CRC-16/MODBUS: check("123456789") == 0x4B37
    pub static fn initModbus(): Crc16Hasher {
        return Crc16Hasher.initCustom(0x8005, 0xFFFF, true, true, 0x0000);
    }

    // CRC-16/KERMIT (CCITT-TRUE): check("123456789") == 0x2189
    pub static fn initKermit(): Crc16Hasher {
        return Crc16Hasher.initCustom(0x1021, 0x0000, true, true, 0x0000);
    }

    // CRC-16/ARC (IBM): check("123456789") == 0xBB3D
    pub static fn initArc(): Crc16Hasher {
        return Crc16Hasher.initCustom(0x8005, 0x0000, true, true, 0x0000);
    }

    // CRC-16/USB: check("123456789") == 0xB4C8
    pub static fn initUsb(): Crc16Hasher {
        return Crc16Hasher.initCustom(0x8005, 0xFFFF, true, true, 0xFFFF);
    }

    pub fn update(*_, data: []const u8) {
        let crc: u16 = _.state;
        let i: usize = 0;
        if (_.ref_in) {
            while (i < data.len) : (i += 1) {
                const idx: usize = ((crc ^ (data[i] as u16)) & 0xFF) as usize;
                crc = (crc >> 8) ^ _.table[idx];
            }
        } else {
            while (i < data.len) : (i += 1) {
                const idx: usize = (((crc >> 8) ^ (data[i] as u16)) & 0xFF) as usize;
                crc = (crc << 8) ^ _.table[idx];
            }
        }
        _.state = crc;
    }

    pub fn finalize(): u16 {
        let out: u16 = _.state;
        if (_.ref_in != _.ref_out) {
            out = reflectU16(out);
        }
        return out ^ _.xor_out;
    }

    pub fn reset(*_) {
        _.state = if (_.ref_in) reflectU16(_.init_val) else _.init_val;
    }
}

pub fn crc16CcittFalse(data: []const u8): u16 {
    let h = Crc16Hasher.initCcittFalse();
    h.update(data);
    return h.finalize();
}

pub fn crc16Xmodem(data: []const u8): u16 {
    let h = Crc16Hasher.initXmodem();
    h.update(data);
    return h.finalize();
}

pub fn crc16Modbus(data: []const u8): u16 {
    let h = Crc16Hasher.initModbus();
    h.update(data);
    return h.finalize();
}

pub fn crc16Kermit(data: []const u8): u16 {
    let h = Crc16Hasher.initKermit();
    h.update(data);
    return h.finalize();
}

pub fn crc16Arc(data: []const u8): u16 {
    let h = Crc16Hasher.initArc();
    h.update(data);
    return h.finalize();
}

pub fn crc16Usb(data: []const u8): u16 {
    let h = Crc16Hasher.initUsb();
    h.update(data);
    return h.finalize();
}
