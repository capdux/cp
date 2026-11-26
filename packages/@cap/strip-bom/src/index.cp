// @cap/strip-bom — Pure CP zero-allocation BOM detection, stripping & prepending
// Parity with npm `strip-bom` & `strip-bom-buf` (UTF-8, UTF-16LE/BE, UTF-32LE/BE).

pub const BOM_NONE: u8 = 0;
pub const BOM_UTF8: u8 = 1;
pub const BOM_UTF16_LE: u8 = 2;
pub const BOM_UTF16_BE: u8 = 3;
pub const BOM_UTF32_LE: u8 = 4;
pub const BOM_UTF32_BE: u8 = 5;

pub struct BomInfo {
    pub kind: u8,
    pub byte_len: usize,
}

pub fn detectBom(data: []u8): BomInfo {
    // 4-byte BOMs must be checked BEFORE 2-byte UTF-16LE (FF FE 00 00 vs FF FE)
    if (data.len >= 4) {
        if (data[0] == 0xFF && data[1] == 0xFE && data[2] == 0x00 && data[3] == 0x00) {
            return BomInfo { kind: BOM_UTF32_LE, byte_len: 4 };
        }
        if (data[0] == 0x00 && data[1] == 0x00 && data[2] == 0xFE && data[3] == 0xFF) {
            return BomInfo { kind: BOM_UTF32_BE, byte_len: 4 };
        }
    }

    // 3-byte UTF-8 BOM: EF BB BF
    if (data.len >= 3) {
        if (data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
            return BomInfo { kind: BOM_UTF8, byte_len: 3 };
        }
    }

    // 2-byte UTF-16 BOMs: FF FE (LE) / FE FF (BE)
    if (data.len >= 2) {
        if (data[0] == 0xFF && data[1] == 0xFE) {
            return BomInfo { kind: BOM_UTF16_LE, byte_len: 2 };
        }
        if (data[0] == 0xFE && data[1] == 0xFF) {
            return BomInfo { kind: BOM_UTF16_BE, byte_len: 2 };
        }
    }

    return BomInfo { kind: BOM_NONE, byte_len: 0 };
}

pub fn hasUtf8Bom(s: string): bool {
    if (s.len < 3) {
        return false;
    }
    return s[0] == 0xEF && s[1] == 0xBB && s[2] == 0xBF;
}

pub fn stripUtf8Bom(s: string): string {
    if (hasUtf8Bom(s)) {
        return s[3..s.len];
    }
    return s;
}

pub fn stripAnyBomOffset(data: []u8): usize {
    let info = detectBom(data);
    return info.byte_len;
}

pub fn prependUtf8Bom(s: string, out: []u8): usize {
    if (hasUtf8Bom(s)) {
        if (out.len < s.len) {
            return 0;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[i] = s[i];
            i = i + 1;
        }
        return s.len;
    }

    let need = (3 as usize) + s.len;
    if (out.len < need) {
        return 0;
    }
    out[0] = 0xEF;
    out[1] = 0xBB;
    out[2] = 0xBF;
    let i: usize = 0;
    while (i < s.len) {
        out[(3 as usize) + i] = s[i];
        i = i + 1;
    }
    return need;
}

export {
    BOM_NONE,
    BOM_UTF8,
    BOM_UTF16_LE,
    BOM_UTF16_BE,
    BOM_UTF32_LE,
    BOM_UTF32_BE,
    BomInfo,
    detectBom,
    hasUtf8Bom,
    stripUtf8Bom,
    stripAnyBomOffset,
    prependUtf8Bom
};
