// @cap/iban — Zero-heap ISO 13616 IBAN validator, Mod-97-10 engine & formatter
// MIT License (clean-room CP implementation)

const COUNTRY_COUNT: usize = 88;
const MAX_IBAN_LEN: usize = 34;

fn toUpperAscii(c: u8) -> u8 {
    if (c >= 'a' as u8 && c <= 'z' as u8) {
        return c - 32;
    }
    return c;
}

fn isUpperAlpha(c: u8) -> bool {
    return c >= 'A' as u8 && c <= 'Z' as u8;
}

fn isDigit(c: u8) -> bool {
    return c >= '0' as u8 && c <= '9' as u8;
}

pub fn countryLength(c0: u8, c1: u8) -> usize {
    const ch0: u8 = toUpperAscii(c0);
    const ch1: u8 = toUpperAscii(c1);
    if (!isUpperAlpha(ch0) || !isUpperAlpha(ch1)) {
        return 0;
    }

    const tbl_codes = "ADAEALATAZBABEBGBHBIBRBYCHCRCYCZDEDJDKDOEEESFIFKFOFRGBGEGIGLGRGTHRHUILIEIQISITJOKWKZLBLCLILTLULVLYMCMDMEMKMNMRMTMUNINLNOOMPKPLPSPTQARORSRUSASCSDSESISKSMSOSTSVTLTNTRUAVAVGXKYE";
    const tbl_lens: [87]u8 = [
        24, 23, 28, 20, 28, 20, 16, 22, 22, 27, 29, 28, 21, 22, 28, 24,
        22, 27, 18, 28, 20, 24, 18, 18, 18, 27, 22, 22, 23, 18, 27, 28,
        21, 28, 23, 22, 23, 26, 27, 30, 30, 20, 28, 32, 21, 20, 20, 21,
        25, 27, 24, 22, 19, 20, 27, 31, 30, 28, 18, 15, 23, 24, 28, 29,
        25, 29, 24, 22, 33, 24, 31, 18, 24, 19, 24, 27, 23, 25, 28, 23,
        24, 26, 29, 22, 24, 20, 30,
    ];

    if (ch0 == 'E' as u8 && ch1 == 'G' as u8) {
        return 29;
    }

    let i: usize = 0;
    while (i < 87) {
        if (tbl_codes[i * 2] == ch0 && tbl_codes[i * 2 + 1] == ch1) {
            return tbl_lens[i] as usize;
        }
        i += 1;
    }
    return 0;
}

pub fn electronicFormat(input: string, out: []u8) -> usize {
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c: u8 = input[i];
        if (c == ' ' as u8 || c == '-' as u8 || c == '\t' as u8) {
            i += 1;
            continue;
        }
        if (pos >= out.len) {
            return 0;
        }
        out[pos] = toUpperAscii(c);
        pos += 1;
        i += 1;
    }
    return pos;
}

pub fn friendlyFormat(input: string, out: []u8) -> usize {
    let elec: [64]u8 = undefined;
    const n: usize = electronicFormat(input, elec[0..64]);
    if (n == 0) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < n) {
        if (i > 0 && (i % 4) == 0) {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = ' ' as u8;
            pos += 1;
        }
        if (pos >= out.len) {
            return 0;
        }
        out[pos] = elec[i];
        pos += 1;
        i += 1;
    }
    return pos;
}

pub fn mod97(electronic: string) -> u32 {
    if (electronic.len < 5) {
        return 0;
    }
    let rem: u32 = 0;
    let step: usize = 0;
    while (step < electronic.len) {
        let idx: usize = step + 4;
        if (idx >= electronic.len) {
            idx -= electronic.len;
        }
        const c: u8 = toUpperAscii(electronic[idx]);
        if (isDigit(c)) {
            rem = (rem * 10 + ((c - ('0' as u8)) as u32)) % 97;
        } else if (isUpperAlpha(c)) {
            const v: u32 = ((c - ('A' as u8)) as u32) + 10;
            rem = (rem * 100 + v) % 97;
        } else {
            return 0;
        }
        step += 1;
    }
    return rem;
}

pub fn isValid(input: string) -> bool {
    let buf: [40]u8 = undefined;
    let n: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c: u8 = input[i];
        if (c == ' ' as u8 || c == '-' as u8 || c == '\t' as u8) {
            i += 1;
            continue;
        }
        if (n >= MAX_IBAN_LEN) {
            return false;
        }
        const up: u8 = toUpperAscii(c);
        if (!isUpperAlpha(up) && !isDigit(up)) {
            return false;
        }
        buf[n] = up;
        n += 1;
        i += 1;
    }

    if (n < 15) {
        return false;
    }
    if (!isUpperAlpha(buf[0]) || !isUpperAlpha(buf[1])) {
        return false;
    }
    if (!isDigit(buf[2]) || !isDigit(buf[3])) {
        return false;
    }

    const expected: usize = countryLength(buf[0], buf[1]);
    if (expected == 0 || expected != n) {
        return false;
    }

    return mod97(buf[0..n]) == 1;
}

pub fn computeCheckDigits(
    country0: u8,
    country1: u8,
    bban: string,
    out_d0: *u8,
    out_d1: *u8,
) -> bool {
    const c0: u8 = toUpperAscii(country0);
    const c1: u8 = toUpperAscii(country1);
    const expected: usize = countryLength(c0, c1);
    if (expected < 5 || expected > MAX_IBAN_LEN) {
        return false;
    }

    let buf: [40]u8 = undefined;
    buf[0] = c0;
    buf[1] = c1;
    buf[2] = '0' as u8;
    buf[3] = '0' as u8;

    let pos: usize = 4;
    let i: usize = 0;
    while (i < bban.len) {
        const c: u8 = bban[i];
        if (c == ' ' as u8 || c == '-' as u8) {
            i += 1;
            continue;
        }
        if (pos >= expected) {
            return false;
        }
        const up: u8 = toUpperAscii(c);
        if (!isUpperAlpha(up) && !isDigit(up)) {
            return false;
        }
        buf[pos] = up;
        pos += 1;
        i += 1;
    }

    if (pos != expected) {
        return false;
    }

    const rem: u32 = mod97(buf[0..expected]);
    const check: u32 = (98 as u32) - rem;
    out_d0.* = ('0' as u8) + ((check / 10) as u8);
    out_d1.* = ('0' as u8) + ((check % 10) as u8);
    return true;
}
