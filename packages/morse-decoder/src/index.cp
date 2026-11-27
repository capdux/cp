// @cap/morse-code — Zero-heap ITU-R M.1677 International Morse Code codec
// MIT License (clean-room CP implementation)

const TABLE_LEN: usize = 44;

const MORSE_CHARS: [44]u8 = [
    'A' as u8, 'B' as u8, 'C' as u8, 'D' as u8, 'E' as u8, 'F' as u8, 'G' as u8, 'H' as u8,
    'I' as u8, 'J' as u8, 'K' as u8, 'L' as u8, 'M' as u8, 'N' as u8, 'O' as u8, 'P' as u8,
    'Q' as u8, 'R' as u8, 'S' as u8, 'T' as u8, 'U' as u8, 'V' as u8, 'W' as u8, 'X' as u8,
    'Y' as u8, 'Z' as u8,
    '0' as u8, '1' as u8, '2' as u8, '3' as u8, '4' as u8, '5' as u8, '6' as u8, '7' as u8,
    '8' as u8, '9' as u8,
    '.' as u8, ',' as u8, '?' as u8, '/' as u8, '-' as u8, '(' as u8, ')' as u8, '@' as u8,
];

const MORSE_CODES: [44]string = [
    ".-", "-...", "-.-.", "-..", ".", "..-.", "--.", "....",
    "..", ".---", "-.-", ".-..", "--", "-.", "---", ".--.",
    "--.-", ".-.", "...", "-", "..-", "...-", ".--", "-..-",
    "-.--", "--..",
    "-----", ".----", "..---", "...--", "....-", ".....", "-....", "--...",
    "---..", "----.",
    ".-.-.-", "--..--", "..--..", "-..-.", "-....-", "-.--.", "-.--.-", ".--.-.",
];

fn toUpperAscii(c: u8) -> u8 {
    if (c >= 'a' as u8 && c <= 'z' as u8) {
        return c - 32;
    }
    return c;
}

fn isWs(c: u8) -> bool {
    return c == ' ' as u8 || c == '\t' as u8 || c == '\n' as u8 || c == '\r' as u8;
}

fn bytesEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn encodeChar(c: u8) -> string {
    const up: u8 = toUpperAscii(c);
    let i: usize = 0;
    while (i < TABLE_LEN) {
        if (MORSE_CHARS[i] == up) {
            return MORSE_CODES[i];
        }
        i += 1;
    }
    return "";
}

pub fn decodeSymbol(sym: string) -> u8 {
    if (sym.len == 0) {
        return 0;
    }
    let i: usize = 0;
    while (i < TABLE_LEN) {
        if (bytesEq(MORSE_CODES[i], sym)) {
            return MORSE_CHARS[i];
        }
        i += 1;
    }
    return 0;
}

fn appendSlice(out: []u8, pos: usize, s: string) -> i64 {
    if (s.len > out.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos + i] = s[i];
        i += 1;
    }
    return (pos + s.len) as i64;
}

pub fn encodeText(input: string, out: []u8) -> usize {
    let pos: usize = 0;
    let i: usize = 0;
    let need_word_sep: bool = false;
    let wrote_any: bool = false;

    while (i < input.len) {
        if (isWs(input[i])) {
            while (i < input.len && isWs(input[i])) {
                i += 1;
            }
            if (wrote_any) {
                need_word_sep = true;
            }
            continue;
        }

        const code: string = encodeChar(input[i]);
        if (code.len == 0) {
            return 0;
        }

        if (need_word_sep) {
            const p: i64 = appendSlice(out, pos, " / ");
            if (p < 0) {
                return 0;
            }
            pos = p as usize;
            need_word_sep = false;
        } else if (wrote_any) {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = ' ' as u8;
            pos += 1;
        }

        const p2: i64 = appendSlice(out, pos, code);
        if (p2 < 0) {
            return 0;
        }
        pos = p2 as usize;
        wrote_any = true;
        i += 1;
    }

    return pos;
}

pub fn decodeText(morse: string, out: []u8) -> usize {
    let pos: usize = 0;
    let i: usize = 0;

    while (i < morse.len) {
        while (i < morse.len && isWs(morse[i])) {
            i += 1;
        }
        if (i >= morse.len) {
            break;
        }
        const tok_start: usize = i;
        while (i < morse.len && !isWs(morse[i])) {
            i += 1;
        }
        const tok: string = morse[tok_start..i];

        if (tok.len == 1 && tok[0] == '/' as u8) {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = ' ' as u8;
            pos += 1;
        } else {
            const ch: u8 = decodeSymbol(tok);
            if (ch == 0) {
                return 0;
            }
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = ch;
            pos += 1;
        }
    }

    return pos;
}
