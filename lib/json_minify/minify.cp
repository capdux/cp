// Source: Absorbed from /data/dev/bun/src/json_parser.zig (JSON & JSONC Zero-Alloc Minifier)
// Pure-CP zero-allocation JSON/JSONC minifier: strips comments (// and /* */), trailing commas, and non-string whitespace.

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn writeByte(out: []u8, pos: *usize, b: u8): bool {
    if (pos.* >= out.len) return false;
    out[pos.*] = b;
    pos.* += 1;
    return true;
}

/// Skip whitespace and (when `allow_comments`) `//` or `/* ... */` comments starting at `pos`.
/// Returns false if a block comment is unterminated.
fn skipTrivia(src: []const u8, pos: *usize, allow_comments: bool): bool {
    while (pos.* < src.len) {
        if (isWs(src[pos.*])) {
            pos.* += 1;
            continue;
        }
        if (allow_comments && src[pos.*] == '/' && pos.* + 1 < src.len) {
            if (src[pos.* + 1] == '/') {
                pos.* += 2;
                while (pos.* < src.len && src[pos.*] != '\n') {
                    pos.* += 1;
                }
                continue;
            }
            if (src[pos.* + 1] == '*') {
                pos.* += 2;
                let closed = false;
                while (pos.* + 1 < src.len) {
                    if (src[pos.*] == '*' && src[pos.* + 1] == '/') {
                        pos.* += 2;
                        closed = true;
                        break;
                    }
                    pos.* += 1;
                }
                if (!closed) return false;
                continue;
            }
        }
        break;
    }
    return true;
}

/// Minify strict RFC 8259 JSON (strips all whitespace outside string literals).
/// Returns null on unterminated string or output buffer overflow.
export fn minifyJson(src: []const u8, out: []u8): ([]const u8)? {
    return minifyJsoncWithOptions(src, false, false, out);
}

/// Minify JSONC (strips `//` and `/* ... */` comments, trailing commas before `}` or `]`, and whitespace).
/// Returns null on unterminated string/comment or output buffer overflow.
export fn minifyJsonc(src: []const u8, out: []u8): ([]const u8)? {
    return minifyJsoncWithOptions(src, true, true, out);
}

export fn minifyJsoncWithOptions(
    src: []const u8,
    strip_comments: bool,
    strip_trailing_commas: bool,
    out: []u8,
): ([]const u8)? {
    let i: usize = 0;
    // Strip optional UTF-8 BOM
    if (src.len >= 3 && src[0] == 0xEF && src[1] == 0xBB && src[2] == 0xBF) {
        i = 3;
    }

    let w: usize = 0;
    while (i < src.len) {
        if (!skipTrivia(src, &i, strip_comments)) return null;
        if (i >= src.len) break;

        const c = src[i];
        if (c == '"') {
            if (!writeByte(out, &w, '"')) return null;
            i += 1;
            let closed = false;
            while (i < src.len) {
                const ch = src[i];
                if (!writeByte(out, &w, ch)) return null;
                i += 1;
                if (ch == '\\') {
                    if (i >= src.len) return null;
                    if (!writeByte(out, &w, src[i])) return null;
                    i += 1;
                } else if (ch == '"') {
                    closed = true;
                    break;
                }
            }
            if (!closed) return null;
            continue;
        }

        if (strip_trailing_commas && c == ',') {
            let look = i + 1;
            if (!skipTrivia(src, &look, strip_comments)) return null;
            if (look < src.len && (src[look] == '}' || src[look] == ']')) {
                i = look;
                continue;
            }
        }

        if (!writeByte(out, &w, c)) return null;
        i += 1;
    }

    return out[0..w];
}
