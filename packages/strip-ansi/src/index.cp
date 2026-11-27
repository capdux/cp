// @cap/ansi-strip — Zero-heap ECMA-48 / ANSI escape sequence stripper
// Reuses @cap/anstyle-parse VT100/ANSI state machine (MIT License)

import {
    TOKEN_TEXT,
    AnsiToken,
    tokenizeAnsi,
    stripAnsiToBuf,
    hasAnsi,
    visibleByteLen,
} from "../../anstyle-parse/src/index.cp";

pub fn strippedLen(input: string) -> usize {
    return visibleByteLen(input);
}

pub fn stripAnsi(input: string, out: []u8) -> usize {
    if (input.len == 0) {
        return 0;
    }
    const need: usize = visibleByteLen(input);
    if (need > out.len) {
        return 0;
    }
    return stripAnsiToBuf(input, out);
}

pub fn visibleAsciiCount(input: string) -> usize {
    let count: usize = 0;
    let offset: usize = 0;
    let toks: [64]AnsiToken = [
        AnsiToken { kind: 0, start: 0, len: 0, final_byte: 0 };
        64
    ];
    while (offset < input.len) {
        const sub: string = input[offset..input.len];
        const n: usize = tokenizeAnsi(sub, toks[0..64]);
        if (n == 0) {
            break;
        }
        let ti: usize = 0;
        let consumed: usize = 0;
        while (ti < n) {
            const tok: AnsiToken = toks[ti];
            if (tok.kind == TOKEN_TEXT) {
                let k: usize = 0;
                while (k < tok.len) {
                    // Count UTF-8 scalar starts (all ASCII bytes & leading UTF-8 bytes)
                    if ((sub[tok.start + k] & 0xC0) != 0x80) {
                        count += 1;
                    }
                    k += 1;
                }
            }
            consumed += tok.len;
            ti += 1;
        }
        if (consumed == 0) {
            break;
        }
        offset += consumed;
    }
    return count;
}

export {
    hasAnsi,
    strippedLen,
    stripAnsi,
    visibleAsciiCount,
};
