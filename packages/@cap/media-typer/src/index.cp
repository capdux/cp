// @cap/media-typer — Pure CP zero-allocation RFC 6838 media type parser & formatter
// Parity with npm `media-typer` (RFC 6838 Section 4.2).

pub struct MediaType {
    pub type_start: usize,
    pub type_len: usize,
    pub subtype_start: usize,
    pub subtype_len: usize,
    pub suffix_start: usize,
    pub suffix_len: usize,
    pub has_suffix: bool,
    pub valid: bool,
}

fn mtInvalid(): MediaType {
    return MediaType {
        type_start: 0,
        type_len: 0,
        subtype_start: 0,
        subtype_len: 0,
        suffix_start: 0,
        suffix_len: 0,
        has_suffix: false,
        valid: false,
    };
}

fn mtIsAlphaNum(c: u8): bool {
    if (c >= ('a' as u8) && c <= ('z' as u8)) {
        return true;
    }
    if (c >= ('A' as u8) && c <= ('Z' as u8)) {
        return true;
    }
    if (c >= ('0' as u8) && c <= ('9' as u8)) {
        return true;
    }
    return false;
}

fn mtIsTypeChar(c: u8): bool {
    if (mtIsAlphaNum(c)) {
        return true;
    }
    if (c == ('!' as u8) || c == ('#' as u8) || c == ('$' as u8) || c == ('&' as u8)) {
        return true;
    }
    if (c == ('^' as u8) || c == ('_' as u8) || c == ('-' as u8)) {
        return true;
    }
    return false;
}

fn mtIsSubtypeChar(c: u8): bool {
    if (mtIsTypeChar(c) || c == ('.' as u8)) {
        return true;
    }
    return false;
}

fn mtIsValidTypeSlice(s: []const u8): bool {
    if (s.len == 0 || s.len > 127) {
        return false;
    }
    if (!mtIsAlphaNum(s[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        if (!mtIsTypeChar(s[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

pub fn isValidToken(s: string): bool {
    if (s.len == 0 || s.len > 127) {
        return false;
    }
    if (!mtIsAlphaNum(s[0])) {
        return false;
    }
    let i: usize = 1;
    while (i < s.len) {
        if (!mtIsSubtypeChar(s[i])) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

pub fn parse(input: string): MediaType {
    if (input.len == 0) {
        return mtInvalid();
    }

    // Locate the single '/' separator; reject any extra '/'
    let slash_pos: usize = 0;
    let found_slash = false;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == ('/' as u8)) {
            if (found_slash) {
                return mtInvalid();
            }
            slash_pos = i;
            found_slash = true;
        }
        i = i + 1;
    }

    if (!found_slash || slash_pos == 0 || slash_pos + 1 >= input.len) {
        return mtInvalid();
    }

    // Validate type part (RFC 6838 restricted-name without '.' or '+')
    let type_part = input[0..slash_pos];
    if (!mtIsValidTypeSlice(type_part)) {
        return mtInvalid();
    }

    // Validate full subtype+suffix part length and characters
    let sub_start = slash_pos + 1;
    let raw_sub = input[sub_start..input.len];
    if (raw_sub.len == 0 || raw_sub.len > 127) {
        return mtInvalid();
    }
    if (!mtIsAlphaNum(raw_sub[0])) {
        return mtInvalid();
    }

    let last_plus: usize = 0;
    let has_plus = false;
    let j: usize = 0;
    while (j < raw_sub.len) {
        let c = raw_sub[j];
        if (c == ('+' as u8)) {
            last_plus = j;
            has_plus = true;
        } else if (!mtIsSubtypeChar(c)) {
            return mtInvalid();
        }
        j = j + 1;
    }

    if (has_plus) {
        if (last_plus == 0 || last_plus + 1 >= raw_sub.len) {
            return mtInvalid();
        }
        let suf_start = sub_start + last_plus + 1;
        let suf_slice = input[suf_start..input.len];
        if (!mtIsValidTypeSlice(suf_slice)) {
            return mtInvalid();
        }
        return MediaType {
            type_start: 0,
            type_len: slash_pos,
            subtype_start: sub_start,
            subtype_len: last_plus,
            suffix_start: suf_start,
            suffix_len: input.len - suf_start,
            has_suffix: true,
            valid: true,
        };
    }

    return MediaType {
        type_start: 0,
        type_len: slash_pos,
        subtype_start: sub_start,
        subtype_len: raw_sub.len,
        suffix_start: 0,
        suffix_len: 0,
        has_suffix: false,
        valid: true,
    };
}

pub fn testMediaType(input: string): bool {
    let mt = parse(input);
    return mt.valid;
}

pub fn format(t: string, subtype: string, suffix: string, out: []u8): usize {
    if (!mtIsValidTypeSlice(t)) {
        return 0;
    }
    if (!isValidToken(subtype)) {
        return 0;
    }

    let has_suf = suffix.len > 0;
    let need = t.len + 1 + subtype.len;
    if (has_suf) {
        if (!mtIsValidTypeSlice(suffix)) {
            return 0;
        }
        if (subtype.len + 1 + suffix.len > 127) {
            return 0;
        }
        need = need + 1 + suffix.len;
    }

    if (out.len < need) {
        return 0;
    }

    let pos: usize = 0;
    let i: usize = 0;
    while (i < t.len) {
        out[pos] = t[i];
        pos = pos + 1;
        i = i + 1;
    }
    out[pos] = '/' as u8;
    pos = pos + 1;

    i = 0;
    while (i < subtype.len) {
        out[pos] = subtype[i];
        pos = pos + 1;
        i = i + 1;
    }

    if (has_suf) {
        out[pos] = '+' as u8;
        pos = pos + 1;
        i = 0;
        while (i < suffix.len) {
            out[pos] = suffix[i];
            pos = pos + 1;
            i = i + 1;
        }
    }

    return pos;
}

export {
    MediaType,
    parse,
    testMediaType,
    format,
    isValidToken
};
