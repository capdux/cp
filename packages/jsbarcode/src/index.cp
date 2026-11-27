// @cap/barcode — Zero-allocation 1D barcode encoder & SVG renderer for CP
// Clean-room MIT implementation inspired by npm `jsbarcode` and Rust `barcoders`.
// Supports: EAN-13 (with checksum), UPC-A (with checksum), Code 128 (Set B full printable ASCII), Code 39.

const MAX_MODULES: usize = 512;

pub struct Barcode {
    pub modules: [MAX_MODULES]u8, // 1 = black bar, 0 = white space
    pub len: usize,
    pub ok: bool,

    pub static fn empty() -> Barcode {
        let m: [MAX_MODULES]u8 = undefined;
        return Barcode{ modules: m, len: 0, ok: false };
    }

    pub fn pushBit(*_, bit: u8) {
        if (_.len < MAX_MODULES) {
            _.modules[_.len] = bit;
            _.len += 1;
        }
    }

    pub fn pushBits(*_, pattern: string) {
        let i: usize = 0;
        while (i < pattern.len) {
            if (pattern[i] == '1') {
                _.pushBit(1);
            } else {
                _.pushBit(0);
            }
            i += 1;
        }
    }

    pub fn pushWidths(*_, widths: string, start_black: bool) {
        let is_black = start_black;
        let i: usize = 0;
        while (i < widths.len) {
            const w: usize = (widths[i] - '0') as usize;
            let k: usize = 0;
            while (k < w) {
                if (is_black) {
                    _.pushBit(1);
                } else {
                    _.pushBit(0);
                }
                k += 1;
            }
            is_black = !is_black;
            i += 1;
        }
    }

    pub fn bitAt(idx: usize) -> u8 {
        if (idx >= _.len) {
            return 0;
        }
        return _.modules[idx];
    }

    pub fn renderAscii(out: []u8) -> string {
        let pos: usize = 0;
        let i: usize = 0;
        while (i < _.len && pos < out.len) {
            if (_.modules[i] == 1) {
                out[pos] = '#';
            } else {
                out[pos] = ' ';
            }
            pos += 1;
            i += 1;
        }
        return out[0..pos];
    }

    pub fn renderSvg(bar_width: usize, height: usize, quiet_zone: usize, out: []u8) -> string {
        if (!_.ok || _.len == 0) {
            return out[0..0];
        }
        const total_w = (_.len + quiet_zone * 2) * bar_width;
        let pos: usize = 0;
        pos = appendStr(out, pos, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"");
        pos = appendUsize(out, pos, total_w);
        pos = appendStr(out, pos, "\" height=\"");
        pos = appendUsize(out, pos, height);
        pos = appendStr(out, pos, "\" viewBox=\"0 0 ");
        pos = appendUsize(out, pos, total_w);
        pos = appendStr(out, pos, " ");
        pos = appendUsize(out, pos, height);
        pos = appendStr(out, pos, "\"><rect width=\"100%\" height=\"100%\" fill=\"#fff\"/>");

        let i: usize = 0;
        while (i < _.len) {
            if (_.modules[i] == 1) {
                const start = i;
                while (i < _.len && _.modules[i] == 1) {
                    i += 1;
                }
                const run_w = (i - start) * bar_width;
                const x = (start + quiet_zone) * bar_width;
                pos = appendStr(out, pos, "<rect x=\"");
                pos = appendUsize(out, pos, x);
                pos = appendStr(out, pos, "\" y=\"0\" width=\"");
                pos = appendUsize(out, pos, run_w);
                pos = appendStr(out, pos, "\" height=\"");
                pos = appendUsize(out, pos, height);
                pos = appendStr(out, pos, "\" fill=\"#000\"/>");
            } else {
                i += 1;
            }
        }
        pos = appendStr(out, pos, "</svg>");
        return out[0..pos];
    }
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendUsize(out: []u8, pos: usize, v: usize) -> usize {
    let tmp: [24]u8 = undefined;
    let n: usize = 0;
    let x = v;
    if (x == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (x > 0 && n < 24) {
            tmp[n] = ((x % 10) as u8) + '0';
            x = x / 10;
            n += 1;
        }
    }
    let p = pos;
    let k = n;
    while (k > 0 && p < out.len) {
        k -= 1;
        out[p] = tmp[k];
        p += 1;
    }
    return p;
}

fn isDigit(c: u8) -> bool {
    return c >= '0' && c <= '9';
}

// ---------------------------------------------------------------------------
// EAN-13 & UPC-A (GS1 Standard)
// ---------------------------------------------------------------------------

pub fn ean13Checksum(digits12: string) -> i8 {
    if (digits12.len != 12) {
        return -1;
    }
    let sum: i32 = 0;
    let i: usize = 0;
    while (i < 12) {
        if (!isDigit(digits12[i])) {
            return -1;
        }
        const d: i32 = (digits12[i] - '0') as i32;
        if ((i % 2) == 0) {
            sum += d;
        } else {
            sum += d * 3;
        }
        i += 1;
    }
    const rem = @rem(sum, 10);
    if (rem == 0) {
        return 0;
    }
    return (10 - rem) as i8;
}

pub fn isValidEan13(code: string) -> bool {
    if (code.len != 13 || !isDigit(code[12])) {
        return false;
    }
    const expected = ean13Checksum(code[0..12]);
    return expected >= 0 && expected == ((code[12] - '0') as i8);
}

pub fn upcAChecksum(digits11: string) -> i8 {
    if (digits11.len != 11) {
        return -1;
    }
    let sum: i32 = 0;
    let i: usize = 0;
    while (i < 11) {
        if (!isDigit(digits11[i])) {
            return -1;
        }
        const d: i32 = (digits11[i] - '0') as i32;
        if ((i % 2) == 0) {
            sum += d * 3;
        } else {
            sum += d;
        }
        i += 1;
    }
    const rem = @rem(sum, 10);
    if (rem == 0) {
        return 0;
    }
    return (10 - rem) as i8;
}

pub fn isValidUpcA(code: string) -> bool {
    if (code.len != 12 || !isDigit(code[11])) {
        return false;
    }
    const expected = upcAChecksum(code[0..11]);
    return expected >= 0 && expected == ((code[11] - '0') as i8);
}

fn eanLCode(d: u8) -> string {
    const tbl: [10]string = [
        "0001101", "0011001", "0010011", "0111101", "0100011",
        "0110001", "0101111", "0111011", "0110111", "0001011",
    ];
    return tbl[d as usize];
}

fn eanGCode(d: u8) -> string {
    const tbl: [10]string = [
        "0100111", "0110011", "0011011", "0100001", "0011101",
        "0111001", "0000101", "0010001", "0001001", "0010111",
    ];
    return tbl[d as usize];
}

fn eanRCode(d: u8) -> string {
    const tbl: [10]string = [
        "1110010", "1100110", "1101100", "1000010", "1011100",
        "1001110", "1010000", "1000100", "1001000", "1110100",
    ];
    return tbl[d as usize];
}

fn eanParityPattern(first_digit: u8) -> string {
    const tbl: [10]string = [
        "LLLLLL", "LLGLGG", "LLGGLG", "LLGGGL", "LGLLGG",
        "LGGLLG", "LGGGLL", "LGLGLG", "LGLGGL", "LGGLGL",
    ];
    return tbl[first_digit as usize];
}

pub fn encodeEan13(code: string) -> Barcode {
    let bc = Barcode.empty();
    let full: [13]u8 = undefined;
    if (code.len == 12) {
        const chk = ean13Checksum(code);
        if (chk < 0) {
            return bc;
        }
        let i: usize = 0;
        while (i < 12) {
            full[i] = code[i] - '0';
            i += 1;
        }
        full[12] = chk as u8;
    } else if (code.len == 13) {
        if (!isValidEan13(code)) {
            return bc;
        }
        let i: usize = 0;
        while (i < 13) {
            full[i] = code[i] - '0';
            i += 1;
        }
    } else {
        return bc;
    }

    const parity = eanParityPattern(full[0]);
    bc.pushBits("101"); // Start guard (3)
    let i: usize = 0;
    while (i < 6) {
        const d = full[i + 1];
        if (parity[i] == 'L') {
            bc.pushBits(eanLCode(d));
        } else {
            bc.pushBits(eanGCode(d));
        }
        i += 1;
    }
    bc.pushBits("01010"); // Center guard (5)
    i = 7;
    while (i < 13) {
        bc.pushBits(eanRCode(full[i]));
        i += 1;
    }
    bc.pushBits("101"); // End guard (3) -> Total 95 modules
    bc.ok = true;
    return bc;
}

pub fn encodeUpcA(code: string) -> Barcode {
    // UPC-A is mathematically identical to EAN-13 with leading '0'
    let buf: [13]u8 = undefined;
    buf[0] = '0';
    if (code.len == 11) {
        const chk = upcAChecksum(code);
        if (chk < 0) {
            return Barcode.empty();
        }
        let i: usize = 0;
        while (i < 11) {
            buf[i + 1] = code[i];
            i += 1;
        }
        buf[12] = (chk as u8) + '0';
        return encodeEan13(buf[0..13]);
    }
    if (code.len == 12) {
        if (!isValidUpcA(code)) {
            return Barcode.empty();
        }
        let i: usize = 0;
        while (i < 12) {
            buf[i + 1] = code[i];
            i += 1;
        }
        return encodeEan13(buf[0..13]);
    }
    return Barcode.empty();
}

// ---------------------------------------------------------------------------
// Code 128 (Set B — Full Printable ASCII 32..126)
// ---------------------------------------------------------------------------

fn code128Pattern(val: usize) -> string {
    const tbl: [107]string = [
        "212222", "222122", "222221", "121223", "121322", "131222", "122213", "122312", "132212", "221213",
        "221312", "231212", "112232", "122132", "122231", "113222", "123122", "123221", "223211", "221132",
        "221231", "213212", "223112", "312131", "311222", "321122", "321221", "312212", "322112", "322211",
        "212123", "212321", "232121", "111323", "131123", "131321", "112313", "132113", "132311", "211313",
        "231113", "231311", "112133", "112331", "132131", "113123", "113321", "133121", "313121", "211331",
        "231131", "213113", "213311", "213131", "311123", "311321", "331121", "312113", "312311", "332111",
        "314111", "221411", "431111", "111224", "111422", "121124", "121421", "141122", "141221", "112214",
        "112412", "122114", "122411", "142112", "142211", "241211", "221114", "413111", "241112", "134111",
        "111242", "121142", "121241", "114212", "124112", "124211", "411212", "421112", "421211", "212141",
        "214121", "412121", "111143", "111341", "131141", "114113", "114311", "411113", "411311", "113141",
        "114131", "311141", "411131", "211412", "211214", "211232", "2331112",
    ];
    if (val >= 107) {
        return "";
    }
    return tbl[val];
}

pub fn encodeCode128B(text: string) -> Barcode {
    let bc = Barcode.empty();
    if (text.len == 0 || text.len > 40) {
        return bc;
    }
    // Start Code B = 104
    let checksum: usize = 104;
    bc.pushWidths(code128Pattern(104), true);

    let i: usize = 0;
    while (i < text.len) {
        const c = text[i];
        if (c < 32 || c > 126) {
            return Barcode.empty();
        }
        const val: usize = (c - 32) as usize;
        checksum += val * (i + 1);
        bc.pushWidths(code128Pattern(val), true);
        i += 1;
    }

    const chk_val = checksum % 103;
    bc.pushWidths(code128Pattern(chk_val), true);
    // Stop code = 106 (13 modules: "2331112")
    bc.pushWidths(code128Pattern(106), true);
    bc.ok = true;
    return bc;
}

// ---------------------------------------------------------------------------
// Code 39 (0-9, A-Z, - . $ / + % space)
// ---------------------------------------------------------------------------

fn code39Pattern(c: u8) -> string {
    const ch = if (c >= 'a' && c <= 'z') { c - 32 } else { c };
    switch (ch) {
        '0' => { return "111221211"; },
        '1' => { return "211211112"; },
        '2' => { return "112211112"; },
        '3' => { return "212211111"; },
        '4' => { return "111221112"; },
        '5' => { return "211221111"; },
        '6' => { return "112221111"; },
        '7' => { return "111211212"; },
        '8' => { return "211211211"; },
        '9' => { return "112211211"; },
        'A' => { return "211112112"; },
        'B' => { return "112112112"; },
        'C' => { return "212112111"; },
        'D' => { return "111122112"; },
        'E' => { return "211122111"; },
        'F' => { return "112122111"; },
        'G' => { return "111112212"; },
        'H' => { return "211112211"; },
        'I' => { return "112112211"; },
        'J' => { return "111122211"; },
        'K' => { return "211111122"; },
        'L' => { return "112111122"; },
        'M' => { return "212111121"; },
        'N' => { return "111121122"; },
        'O' => { return "211121121"; },
        'P' => { return "112121121"; },
        'Q' => { return "111111222"; },
        'R' => { return "211111221"; },
        'S' => { return "112111221"; },
        'T' => { return "111121221"; },
        'U' => { return "221111112"; },
        'V' => { return "122111112"; },
        'W' => { return "222111111"; },
        'X' => { return "121121112"; },
        'Y' => { return "221121111"; },
        'Z' => { return "122121111"; },
        '-' => { return "121111212"; },
        '.' => { return "221111211"; },
        ' ' => { return "122111211"; },
        '$' => { return "121212111"; },
        '/' => { return "121211121"; },
        '+' => { return "121112121"; },
        '%' => { return "111212121"; },
        '*' => { return "121121211"; },
        _ => { return ""; },
    }
}

pub fn encodeCode39(text: string) -> Barcode {
    let bc = Barcode.empty();
    if (text.len == 0 || text.len > 32) {
        return bc;
    }
    bc.pushWidths(code39Pattern('*'), true);
    bc.pushBit(0); // inter-character narrow gap
    let i: usize = 0;
    while (i < text.len) {
        const pat = code39Pattern(text[i]);
        if (pat.len == 0 || text[i] == '*') {
            return Barcode.empty();
        }
        bc.pushWidths(pat, true);
        bc.pushBit(0);
        i += 1;
    }
    bc.pushWidths(code39Pattern('*'), true);
    bc.ok = true;
    return bc;
}

export {
    Barcode,
    ean13Checksum,
    isValidEan13,
    upcAChecksum,
    isValidUpcA,
    encodeEan13,
    encodeUpcA,
    encodeCode128B,
    encodeCode39,
};
