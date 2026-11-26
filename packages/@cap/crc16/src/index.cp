// @cap/crc16 — Zero-heap Industrial CRC-16 Checksum Engine
// Clean-room MIT implementation in pure CP (CCITT-FALSE, XMODEM, MODBUS, KERMIT, ARC, USB).

fn reflectU8(v: u8): u8 {
    let x: u8 = v;
    let out: u8 = 0;
    let i: u8 = 0;
    while (i < 8) {
        out = (out << 1) | (x & 1);
        x = x >> 1;
        i += 1;
    }
    return out;
}

fn reflectU16(v: u16): u16 {
    let x: u16 = v;
    let out: u16 = 0;
    let i: u8 = 0;
    while (i < 16) {
        out = (out << 1) | (x & 1);
        x = x >> 1;
        i += 1;
    }
    return out;
}

pub struct Crc16Hasher {
    poly: u16,
    init_val: u16,
    xor_out: u16,
    ref_in: bool,
    ref_out: bool,
    state: u16,

    pub static fn initCustom(poly: u16, init_val: u16, ref_in: bool, ref_out: bool, xor_out: u16): Crc16Hasher {
        return Crc16Hasher {
            poly: poly,
            init_val: init_val,
            xor_out: xor_out,
            ref_in: ref_in,
            ref_out: ref_out,
            state: init_val,
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
        while (i < data.len) {
            let b: u8 = data[i];
            if (_.ref_in) {
                b = reflectU8(b);
            }
            crc = crc ^ ((b as u16) << 8);
            let bit: u8 = 0;
            while (bit < 8) {
                if ((crc & 0x8000) != 0) {
                    crc = (crc << 1) ^ _.poly;
                } else {
                    crc = crc << 1;
                }
                bit += 1;
            }
            i += 1;
        }
        _.state = crc;
    }

    pub fn finalize(): u16 {
        let out: u16 = _.state;
        if (_.ref_out) {
            out = reflectU16(out);
        }
        return out ^ _.xor_out;
    }

    pub fn reset(*_) {
        _.state = _.init_val;
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
