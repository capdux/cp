// Source: Absorbed from /data/dev/bun/src/json_parser.zig | Synced: 2026-11-21T15:15:50Z (JSON & JSONC Zero-Alloc Minifier)
// Pure-CP zero-allocation JSON/JSONC minifier: strips comments (// and /* */), trailing commas, and non-string whitespace.

type JsonMinVec16u8 = @Vector(16, u8);

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn jsonMinSimdSkipWs(src: []const u8, start: usize): usize {
    let i: usize = start;
    const n = src.len;
    const v_sp: JsonMinVec16u8 = @splat(' ');
    const v_tab: JsonMinVec16u8 = @splat('\t');
    const v_cr: JsonMinVec16u8 = @splat('\r');
    const v_lf: JsonMinVec16u8 = @splat('\n');
    while (i + 16 <= n) {
        const chunk: JsonMinVec16u8 = src[i..][0..16].*;
        const m_sp: u16 = @bitCast(chunk == v_sp);
        const m_tab: u16 = @bitCast(chunk == v_tab);
        const m_cr: u16 = @bitCast(chunk == v_cr);
        const m_lf: u16 = @bitCast(chunk == v_lf);
        const m_non_ws: u16 = ~(m_sp | m_tab | m_cr | m_lf);
        if (m_non_ws != 0) {
            return i + (@ctz(m_non_ws) as usize);
        }
        i += 16;
    }
    while (i < n && isWs(src[i])) : (i += 1) {}
    return i;
}

fn jsonMinSimdFindByte(src: []const u8, start: usize, target: u8): usize {
    let i: usize = start;
    const n = src.len;
    const v_target: JsonMinVec16u8 = @splat(target);
    while (i + 16 <= n) {
        const chunk: JsonMinVec16u8 = src[i..][0..16].*;
        const mask: u16 = @bitCast(chunk == v_target);
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n && src[i] != target) : (i += 1) {}
    return i;
}

fn jsonMinSimdFindStringSpecial(src: []const u8, start: usize): usize {
    let i: usize = start;
    const n = src.len;
    const v_quote: JsonMinVec16u8 = @splat('"');
    const v_bs: JsonMinVec16u8 = @splat('\\');
    while (i + 16 <= n) {
        const chunk: JsonMinVec16u8 = src[i..][0..16].*;
        const m_q: u16 = @bitCast(chunk == v_quote);
        const m_b: u16 = @bitCast(chunk == v_bs);
        const mask: u16 = m_q | m_b;
        if (mask != 0) {
            return i + (@ctz(mask) as usize);
        }
        i += 16;
    }
    while (i < n) : (i += 1) {
        const c = src[i];
        if (c == '"' || c == '\\') return i;
    }
    return n;
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
            pos.* = jsonMinSimdSkipWs(src, pos.*);
            continue;
        }
        if (allow_comments && src[pos.*] == '/' && pos.* + 1 < src.len) {
            if (src[pos.* + 1] == '/') {
                pos.* = jsonMinSimdFindByte(src, pos.* + 2, '\n');
                continue;
            }
            if (src[pos.* + 1] == '*') {
                pos.* += 2;
                let closed = false;
                while (pos.* + 1 < src.len) {
                    const star = jsonMinSimdFindByte(src, pos.*, '*');
                    if (star + 1 >= src.len) {
                        pos.* = src.len;
                        break;
                    }
                    if (src[star + 1] == '/') {
                        pos.* = star + 2;
                        closed = true;
                        break;
                    }
                    pos.* = star + 1;
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
pub fn minifyJson(src: []const u8, out: []u8): ([]const u8)? {
    return minifyJsoncWithOptions(src, false, false, out);
}

/// Minify JSONC (strips `//` and `/* ... */` comments, trailing commas before `}` or `]`, and whitespace).
/// Returns null on unterminated string/comment or output buffer overflow.
pub fn minifyJsonc(src: []const u8, out: []u8): ([]const u8)? {
    return minifyJsoncWithOptions(src, true, true, out);
}

pub fn minifyJsoncWithOptions(
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
                const hit = jsonMinSimdFindStringSpecial(src, i);
                const span = hit - i;
                if (span > 0) {
                    if (w + span > out.len) return null;
                    @memcpy(out[w .. w + span], src[i..hit]);
                    w += span;
                    i = hit;
                }
                if (i >= src.len) break;
                const ch = src[i];
                if (!writeByte(out, &w, ch)) return null;
                i += 1;
                if (ch == '\\') {
                    if (i >= src.len) return null;
                    if (!writeByte(out, &w, src[i])) return null;
                    i += 1;
                } else {
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
