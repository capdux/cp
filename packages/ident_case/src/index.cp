// @cap/camelcase — Zero-heap programming case converter (camel, Pascal, snake, kebab, SCREAMING_SNAKE)
// MIT License (clean-room CP implementation)

fn isUpper(c: u8) -> bool {
    return c >= 'A' as u8 && c <= 'Z' as u8;
}

fn isLower(c: u8) -> bool {
    return c >= 'a' as u8 && c <= 'z' as u8;
}

fn isDigit(c: u8) -> bool {
    return c >= '0' as u8 && c <= '9' as u8;
}

fn isAlnum(c: u8) -> bool {
    return isUpper(c) || isLower(c) || isDigit(c);
}

fn toLower(c: u8) -> u8 {
    if (isUpper(c)) {
        return c + 32;
    }
    return c;
}

fn toUpper(c: u8) -> u8 {
    if (isLower(c)) {
        return c - 32;
    }
    return c;
}

fn nextWord(input: string, pos: usize, out_start: *usize, out_end: *usize) -> bool {
    let i: usize = pos;
    while (i < input.len && !isAlnum(input[i])) {
        i += 1;
    }
    if (i >= input.len) {
        return false;
    }

    const start: usize = i;
    i += 1;
    while (i < input.len && isAlnum(input[i])) {
        const prev: u8 = input[i - 1];
        const cur: u8 = input[i];
        // Boundary: lowercase/digit -> Uppercase (foo|Bar, v2|Beta)
        if ((isLower(prev) || isDigit(prev)) && isUpper(cur)) {
            break;
        }
        // Boundary: Acronym -> Title word (XML|Http, HTTP|Response)
        if (isUpper(prev) && isUpper(cur) && i + 1 < input.len && isLower(input[i + 1])) {
            break;
        }
        i += 1;
    }

    out_start.* = start;
    out_end.* = i;
    return true;
}

fn convertJoined(
    input: string,
    out: []u8,
    sep: u8,
    has_sep: bool,
    upper_all: bool,
    cap_first_word: bool,
    cap_rest_words: bool,
) -> usize {
    let pos: usize = 0;
    let out_pos: usize = 0;
    let word_idx: usize = 0;

    while (pos < input.len) {
        let ws: usize = 0;
        let we: usize = 0;
        if (!nextWord(input, pos, &ws, &we)) {
            break;
        }

        if (word_idx > 0 && has_sep) {
            if (out_pos >= out.len) {
                return 0;
            }
            out[out_pos] = sep;
            out_pos += 1;
        }

        const wlen: usize = we - ws;
        if (wlen > out.len - out_pos) {
            return 0;
        }

        let k: usize = 0;
        while (k < wlen) {
            const c: u8 = input[ws + k];
            if (upper_all) {
                out[out_pos] = toUpper(c);
            } else if (k == 0 && ((word_idx == 0 && cap_first_word) || (word_idx > 0 && cap_rest_words))) {
                out[out_pos] = toUpper(c);
            } else {
                out[out_pos] = toLower(c);
            }
            out_pos += 1;
            k += 1;
        }

        word_idx += 1;
        pos = we;
    }

    return out_pos;
}

pub fn toCamelCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, 0, false, false, false, true);
}

pub fn toPascalCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, 0, false, false, true, true);
}

pub fn toSnakeCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, '_' as u8, true, false, false, false);
}

pub fn toKebabCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, '-' as u8, true, false, false, false);
}

pub fn toScreamingSnakeCase(input: string, out: []u8) -> usize {
    return convertJoined(input, out, '_' as u8, true, true, false, false);
}
