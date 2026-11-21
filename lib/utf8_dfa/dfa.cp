// Source: Absorbed from /data/dev/bun/src/unicode/uucode_lib/src/utf8.cp | Synced: 2026-11-21T12:33:35Z
//! Bjoern Hoehrmann's branchless UTF-8 DFA decoder + fast ASCII codepoint predicates.

export const UTF8_ACCEPT: usize = 0;
export const UTF8_REJECT: usize = 12;
export const REPLACEMENT_CHAR: u21 = 0xFFFD;

// Maps byte values to character classes.
const utf8d: [256]u8 = [
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
    7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
    8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
   10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,
];

// Maps (state + character class) to the next DFA state.
const state_utf8d: [108]u8 = [
    0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
   12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
   12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
   12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
   12,36,12,12,12,12,12,12,12,12,12,12,
];

export struct DecodeStep {
    state: usize,
    cp: u21,
}

/// Feed one byte into the UTF-8 DFA.
export fn decodeByte(state: usize, cp: u21, byte: u8): DecodeStep {
    const cls: u5 = @intCast(utf8d[byte]);
    const mask: u21 = 0xFF;
    const next_cp: u21 = if (state != UTF8_ACCEPT)
        @as(u21, byte & 0x3F) | (cp << 6)
    else
        (mask >> cls) & @as(u21, byte);
    const next_state: usize = state_utf8d[state + @as(usize, cls)];
    return DecodeStep{ state: next_state, cp: next_cp };
}

export fn isDoneDecoding(state: usize): bool {
    return state == UTF8_ACCEPT || state == UTF8_REJECT;
}

/// Lossy UTF-8 codepoint iterator (invalid/overlong sequences emit U+FFFD).
export struct Utf8Iterator {
    i: usize = 0,
    bytes: []const u8,

    pub static fn init(bytes: []const u8): Utf8Iterator {
        return Utf8Iterator{ i: 0, bytes: bytes };
    }

    pub fn next(*_): u21? {
        if (_.i >= _.bytes.len) return null;
        let cp: u21 = 0;
        let state: usize = UTF8_ACCEPT;
        while (true) {
            const step = decodeByte(state, cp, _.bytes[_.i]);
            state = step.state;
            cp = step.cp;
            _.i += 1;
            if (isDoneDecoding(state) || _.i >= _.bytes.len) break;
        }
        if (state == UTF8_ACCEPT) return cp;
        return REPLACEMENT_CHAR;
    }

    pub fn peek(): u21? {
        let copy = Utf8Iterator{ i: _.i, bytes: _.bytes };
        return copy.next();
    }

    pub fn remaining(): []const u8 {
        if (_.i >= _.bytes.len) return _.bytes[_.bytes.len..];
        return _.bytes[_.i..];
    }

    pub fn reset(*_) {
        _.i = 0;
    }
}

/// Validate whether a byte slice is strictly valid UTF-8 using the DFA.
export fn isValidUtf8(bytes: []const u8): bool {
    let state: usize = UTF8_ACCEPT;
    let cp: u21 = 0;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        const step = decodeByte(state, cp, bytes[i]);
        state = step.state;
        cp = step.cp;
        if (state == UTF8_REJECT) return false;
    }
    return state == UTF8_ACCEPT;
}

/// Count Unicode scalar values in a byte slice (invalid sequences counted as U+FFFD).
export fn countCodepoints(bytes: []const u8): usize {
    let it = Utf8Iterator.init(bytes);
    let n: usize = 0;
    while (it.next() != null) {
        n += 1;
    }
    return n;
}

// --- Fast ASCII Codepoint Predicates (u21) ---

export fn isAscii(c: u21): bool {
    return c < 128;
}

export fn isDigit(c: u21): bool {
    return c >= '0' && c <= '9';
}

export fn isUpper(c: u21): bool {
    return c >= 'A' && c <= 'Z';
}

export fn isLower(c: u21): bool {
    return c >= 'a' && c <= 'z';
}

export fn isAlphabetic(c: u21): bool {
    return isUpper(c) || isLower(c);
}

export fn isAlphanumeric(c: u21): bool {
    return isDigit(c) || isAlphabetic(c);
}

export fn isHex(c: u21): bool {
    return isDigit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

export fn isControl(c: u21): bool {
    return c <= 0x1F || c == 0x7F;
}

export fn isPrint(c: u21): bool {
    return isAscii(c) && !isControl(c);
}

export fn isWhitespace(c: u21): bool {
    return c == ' ' || (c >= '\t' && c <= '\r');
}

/// Branchless ASCII uppercase conversion.
export fn toUpper(c: u21): u21 {
    const mask: u21 = @as(u21, @intFromBool(isLower(c))) << 5;
    return c ^ mask;
}

/// Branchless ASCII lowercase conversion.
export fn toLower(c: u21): u21 {
    const mask: u21 = @as(u21, @intFromBool(isUpper(c))) << 5;
    return c | mask;
}
