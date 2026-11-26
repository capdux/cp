// @cap/rot13 — Zero-heap ROT13, ROT47, Caesar shift & Atbash substitution
// MIT License (clean-room CP implementation)

fn shiftLetter(c: u8, norm_shift: u8) -> u8 {
    if (c >= 'A' as u8 && c <= 'Z' as u8) {
        const idx: u8 = ((c - ('A' as u8)) + norm_shift) % 26;
        return ('A' as u8) + idx;
    }
    if (c >= 'a' as u8 && c <= 'z' as u8) {
        const idx: u8 = ((c - ('a' as u8)) + norm_shift) % 26;
        return ('a' as u8) + idx;
    }
    return c;
}

pub fn rot13(input: string, out: []u8) -> usize {
    if (input.len == 0 || input.len > out.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < input.len) {
        out[i] = shiftLetter(input[i], 13);
        i += 1;
    }
    return input.len;
}

pub fn rot13InPlace(buf: []u8) {
    let i: usize = 0;
    while (i < buf.len) {
        buf[i] = shiftLetter(buf[i], 13);
        i += 1;
    }
}

pub fn rot47(input: string, out: []u8) -> usize {
    if (input.len == 0 || input.len > out.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < input.len) {
        const c: u8 = input[i];
        if (c >= 33 && c <= 126) {
            const idx: u8 = ((c - 33) + 47) % 94;
            out[i] = idx + 33;
        } else {
            out[i] = c;
        }
        i += 1;
    }
    return input.len;
}

pub fn caesar(input: string, shift: i32, out: []u8) -> usize {
    if (input.len == 0 || input.len > out.len) {
        return 0;
    }
    let norm: u8 = 0;
    if (shift >= 0) {
        norm = ((shift as u32) % 26) as u8;
    } else {
        const mag: u64 = (-(shift as i64)) as u64;
        const rem: u64 = mag % 26;
        if (rem == 0) {
            norm = 0;
        } else {
            norm = (26 - rem) as u8;
        }
    }

    let i: usize = 0;
    while (i < input.len) {
        out[i] = shiftLetter(input[i], norm);
        i += 1;
    }
    return input.len;
}

pub fn atbash(input: string, out: []u8) -> usize {
    if (input.len == 0 || input.len > out.len) {
        return 0;
    }
    let i: usize = 0;
    while (i < input.len) {
        const c: u8 = input[i];
        if (c >= 'A' as u8 && c <= 'Z' as u8) {
            out[i] = ('Z' as u8) - (c - ('A' as u8));
        } else if (c >= 'a' as u8 && c <= 'z' as u8) {
            out[i] = ('z' as u8) - (c - ('a' as u8));
        } else {
            out[i] = c;
        }
        i += 1;
    }
    return input.len;
}
