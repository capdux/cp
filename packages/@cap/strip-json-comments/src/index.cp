// @cap/strip-json-comments — Zero-allocation JSONC comment & trailing-comma stripper for CP
// Clean-room MIT implementation inspired by npm `strip-json-comments` (150M+/wk).

pub struct StripOptions {
    pub preserve_whitespace: bool,
    pub strip_trailing_commas: bool,
}

fn isSpace(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

// Look ahead past whitespace and comments after a comma to check if next token is '}' or ']'.
fn isTrailingComma(src: []const u8, commaIdx: usize) -> bool {
    let i = commaIdx + (1 as usize);
    while (i < src.len) {
        const c = src[i];
        if (isSpace(c)) {
            i += 1;
            continue;
        }
        if (c == '/' && i + (1 as usize) < src.len && src[i + (1 as usize)] == '/') {
            i += 2;
            while (i < src.len && src[i] != '\n') {
                i += 1;
            }
            continue;
        }
        if (c == '/' && i + (1 as usize) < src.len && src[i + (1 as usize)] == '*') {
            i += 2;
            let closed = false;
            while (i + (1 as usize) < src.len) {
                if (src[i] == '*' && src[i + (1 as usize)] == '/') {
                    i += 2;
                    closed = true;
                    break;
                }
                i += 1;
            }
            if (!closed) {
                return false;
            }
            continue;
        }
        return c == '}' || c == ']';
    }
    return false;
}

pub fn stripComments(src: []const u8, dst: []u8, opts: StripOptions) -> i64 {
    let i: usize = 0;
    let outPos: usize = 0;

    while (i < src.len) {
        const c = src[i];

        // 1. Double-quoted string literal: preserve verbatim including \" and // inside
        if (c == '"') {
            if (outPos >= dst.len) { return -1 as i64; }
            dst[outPos] = '"';
            outPos += 1;
            i += 1;

            while (i < src.len) {
                const sc = src[i];
                if (sc == '\\' && i + (1 as usize) < src.len) {
                    if (outPos + (2 as usize) > dst.len) { return -1 as i64; }
                    dst[outPos] = sc;
                    dst[outPos + (1 as usize)] = src[i + (1 as usize)];
                    outPos += 2;
                    i += 2;
                    continue;
                }
                if (outPos >= dst.len) { return -1 as i64; }
                dst[outPos] = sc;
                outPos += 1;
                i += 1;
                if (sc == '"') {
                    break;
                }
            }
            continue;
        }

        // 2. Single-line comment // ...
        if (c == '/' && i + (1 as usize) < src.len && src[i + (1 as usize)] == '/') {
            while (i < src.len && src[i] != '\n' && src[i] != '\r') {
                if (opts.preserve_whitespace) {
                    if (outPos >= dst.len) { return -1 as i64; }
                    dst[outPos] = ' ';
                    outPos += 1;
                }
                i += 1;
            }
            continue;
        }

        // 3. Block comment /* ... */
        if (c == '/' && i + (1 as usize) < src.len && src[i + (1 as usize)] == '*') {
            // Emit spaces for "/*" if preserve_whitespace
            if (opts.preserve_whitespace) {
                if (outPos + (2 as usize) > dst.len) { return -1 as i64; }
                dst[outPos] = ' ';
                dst[outPos + (1 as usize)] = ' ';
                outPos += 2;
            }
            i += 2;

            while (i < src.len) {
                if (src[i] == '*' && i + (1 as usize) < src.len && src[i + (1 as usize)] == '/') {
                    if (opts.preserve_whitespace) {
                        if (outPos + (2 as usize) > dst.len) { return -1 as i64; }
                        dst[outPos] = ' ';
                        dst[outPos + (1 as usize)] = ' ';
                        outPos += 2;
                    }
                    i += 2;
                    break;
                }
                const bc = src[i];
                if (opts.preserve_whitespace) {
                    if (outPos >= dst.len) { return -1 as i64; }
                    if (bc == '\n' || bc == '\r') {
                        dst[outPos] = bc;
                    } else {
                        dst[outPos] = ' ';
                    }
                    outPos += 1;
                }
                i += 1;
            }
            continue;
        }

        // 4. Optional trailing comma removal before '}' or ']'
        if (c == ',' && opts.strip_trailing_commas && isTrailingComma(src, i)) {
            if (opts.preserve_whitespace) {
                if (outPos >= dst.len) { return -1 as i64; }
                dst[outPos] = ' ';
                outPos += 1;
            }
            i += 1;
            continue;
        }

        // 5. Regular byte
        if (outPos >= dst.len) { return -1 as i64; }
        dst[outPos] = c;
        outPos += 1;
        i += 1;
    }

    return outPos as i64;
}

pub fn stripJsonc(src: []const u8, dst: []u8) -> i64 {
    const opts = StripOptions{
        preserve_whitespace: true,
        strip_trailing_commas: true,
    };
    return stripComments(src, dst, opts);
}

pub fn stripCompact(src: []const u8, dst: []u8) -> i64 {
    const opts = StripOptions{
        preserve_whitespace: false,
        strip_trailing_commas: true,
    };
    return stripComments(src, dst, opts);
}

export {
    StripOptions,
    stripComments,
    stripJsonc,
    stripCompact,
};
