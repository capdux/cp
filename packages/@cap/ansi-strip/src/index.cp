// @cap/ansi-strip — Zero-heap ECMA-48 / ANSI escape sequence stripper
// MIT License (clean-room CP implementation)

fn skipAnsiAt(input: string, pos: usize) -> usize {
    const c: u8 = input[pos];

    // C1 CSI (0x9B)
    if (c == 0x9B) {
        let i: usize = pos + 1;
        while (i < input.len && input[i] >= 0x30 && input[i] <= 0x3F) {
            i += 1;
        }
        while (i < input.len && input[i] >= 0x20 && input[i] <= 0x2F) {
            i += 1;
        }
        if (i < input.len && input[i] >= 0x40 && input[i] <= 0x7E) {
            i += 1;
        }
        return i - pos;
    }

    if (c != 0x1B) {
        return 0;
    }

    // Truncated trailing ESC at end of input
    if (pos + 1 >= input.len) {
        return 1;
    }

    const nxt: u8 = input[pos + 1];

    // CSI: ESC [ [0-9:;<=>?]* [ -/]* [@-~]
    if (nxt == '[' as u8) {
        let i: usize = pos + 2;
        while (i < input.len && input[i] >= 0x30 && input[i] <= 0x3F) {
            i += 1;
        }
        while (i < input.len && input[i] >= 0x20 && input[i] <= 0x2F) {
            i += 1;
        }
        if (i < input.len && input[i] >= 0x40 && input[i] <= 0x7E) {
            i += 1;
        }
        return i - pos;
    }

    // OSC: ESC ] ... (BEL 0x07 | ST ESC \ | C1 ST 0x9C)
    if (nxt == ']' as u8) {
        let i: usize = pos + 2;
        while (i < input.len) {
            if (input[i] == 0x07 || input[i] == 0x9C) {
                i += 1;
                break;
            }
            if (input[i] == 0x1B && i + 1 < input.len && input[i + 1] == '\\' as u8) {
                i += 2;
                break;
            }
            i += 1;
        }
        return i - pos;
    }

    // DCS / SOS / PM / APC: ESC (P|X|^|_) ... ST
    if (nxt == 'P' as u8 || nxt == 'X' as u8 || nxt == '^' as u8 || nxt == '_' as u8) {
        let i: usize = pos + 2;
        while (i < input.len) {
            if (input[i] == 0x9C) {
                i += 1;
                break;
            }
            if (input[i] == 0x1B && i + 1 < input.len && input[i + 1] == '\\' as u8) {
                i += 2;
                break;
            }
            i += 1;
        }
        return i - pos;
    }

    // nF sequence: ESC [ -/]+ [0-~]
    if (nxt >= 0x20 && nxt <= 0x2F) {
        let i: usize = pos + 2;
        while (i < input.len && input[i] >= 0x20 && input[i] <= 0x2F) {
            i += 1;
        }
        if (i < input.len && input[i] >= 0x30 && input[i] <= 0x7E) {
            i += 1;
        }
        return i - pos;
    }

    // Standard 2-byte ESC sequence (e.g., ESC 7, ESC 8, ESC c, ESC M)
    return 2;
}

pub fn hasAnsi(input: string) -> bool {
    let i: usize = 0;
    while (i < input.len) {
        const c: u8 = input[i];
        if (c == 0x1B || c == 0x9B) {
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn strippedLen(input: string) -> usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const skip: usize = skipAnsiAt(input, i);
        if (skip > 0) {
            i += skip;
        } else {
            total += 1;
            i += 1;
        }
    }
    return total;
}

pub fn stripAnsi(input: string, out: []u8) -> usize {
    if (input.len == 0) {
        return 0;
    }
    const need: usize = strippedLen(input);
    if (need > out.len) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const skip: usize = skipAnsiAt(input, i);
        if (skip > 0) {
            i += skip;
        } else {
            out[pos] = input[i];
            pos += 1;
            i += 1;
        }
    }
    return pos;
}

pub fn visibleAsciiCount(input: string) -> usize {
    let count: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const skip: usize = skipAnsiAt(input, i);
        if (skip > 0) {
            i += skip;
        } else {
            // Count UTF-8 scalar starts (all ASCII bytes & leading UTF-8 bytes)
            if ((input[i] & 0xC0) != 0x80) {
                count += 1;
            }
            i += 1;
        }
    }
    return count;
}
