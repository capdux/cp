// @cap/regex-escape — Zero-heap regular expression literal & character-class escaper
// MIT License (clean-room CP implementation)

pub fn isMetaChar(c: u8) -> bool {
    return c == '\\' as u8
        || c == '.' as u8
        || c == '+' as u8
        || c == '*' as u8
        || c == '?' as u8
        || c == '(' as u8
        || c == ')' as u8
        || c == '|' as u8
        || c == '[' as u8
        || c == ']' as u8
        || c == '{' as u8
        || c == '}' as u8
        || c == '^' as u8
        || c == '$' as u8
        || c == '-' as u8;
}

fn isEscapeTarget(c: u8) -> bool {
    return isMetaChar(c) || c == '/' as u8;
}

pub fn needsEscape(input: string) -> bool {
    let i: usize = 0;
    while (i < input.len) {
        if (isEscapeTarget(input[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

pub fn escapedLen(input: string) -> usize {
    let total: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (isEscapeTarget(input[i])) {
            total += 2;
        } else {
            total += 1;
        }
        i += 1;
    }
    return total;
}

pub fn escapeRegex(input: string, out: []u8) -> usize {
    if (input.len == 0) {
        return 0;
    }
    const need: usize = escapedLen(input);
    if (need > out.len) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c: u8 = input[i];
        if (isEscapeTarget(c)) {
            out[pos] = '\\' as u8;
            out[pos + 1] = c;
            pos += 2;
        } else {
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    return pos;
}

pub fn escapeCharClass(input: string, out: []u8) -> usize {
    if (input.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        const c: u8 = input[i];
        const needs: bool = c == '\\' as u8
            || c == ']' as u8
            || c == '-' as u8
            || c == '^' as u8;
        if (needs) {
            if (pos + 2 > out.len) {
                return 0;
            }
            out[pos] = '\\' as u8;
            out[pos + 1] = c;
            pos += 2;
        } else {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = c;
            pos += 1;
        }
        i += 1;
    }
    return pos;
}

pub fn unescapeRegexLiteral(input: string, out: []u8) -> usize {
    if (input.len == 0) {
        return 0;
    }
    let pos: usize = 0;
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == '\\' as u8 && i + 1 < input.len) {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = input[i + 1];
            pos += 1;
            i += 2;
        } else {
            if (pos >= out.len) {
                return 0;
            }
            out[pos] = input[i];
            pos += 1;
            i += 1;
        }
    }
    return pos;
}
