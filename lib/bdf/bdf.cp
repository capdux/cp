// Origin: Pure-CP Stdlib (Zero-Alloc Adobe BDF 2.1/2.2 Bitmap Font Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation Adobe BDF 2.1/2.2 parser: global font header,
// STARTPROPERTIES..ENDPROPERTIES iterator, STARTCHAR..ENDCHAR glyph iterator,
// MSB-first bitmap pixel decoder, and codepoint lookup.

const std = @import("std");

/// Bounding box (`width`, `height`, `x_off`, `y_off`) for a BDF font or glyph.
pub struct BdfBBox {
    width: i32 = 0,
    height: i32 = 0,
    x_off: i32 = 0,
    y_off: i32 = 0,
}

/// Zero-copy parsed Adobe BDF global font header.
pub struct BdfHeader {
    version: []const u8 = "",
    font_name: []const u8 = "",
    point_size: i32 = 0,
    x_res: i32 = 0,
    y_res: i32 = 0,
    bbox: BdfBBox = BdfBBox {},
    declared_chars: u32 = 0,
    ascent: i32 = 0,
    descent: i32 = 0,
    chars_offset: usize = 0,
}

/// Zero-copy key/value entry from `STARTPROPERTIES` .. `ENDPROPERTIES`.
pub struct BdfProperty {
    name: []const u8 = "",
    value: []const u8 = "",
}

/// Zero-copy parsed BDF glyph from `STARTCHAR` .. `ENDCHAR`.
pub struct BdfGlyph {
    name: []const u8 = "",
    encoding: i32 = -1,
    dwidth_x: i32 = 0,
    dwidth_y: i32 = 0,
    bbox: BdfBBox = BdfBBox {},
    bitmap_slice: []const u8 = "",
}

struct LineSlice {
    text: []const u8,
    line_start: usize,
    next_pos: usize,
}

struct TokenSlice {
    token: []const u8,
    rest: []const u8,
}

// ---------------------------------------------------------------------------
// Internal Zero-Alloc Line & Number Helpers
// ---------------------------------------------------------------------------

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn nextLine(data: []const u8, pos: usize): LineSlice? {
    if (pos >= data.len) {
        return null;
    }
    let i = pos;
    while (i < data.len && data[i] != '\n') {
        i += 1;
    }
    let end = i;
    if (end > pos && data[end - 1] == '\r') {
        end -= 1;
    }
    const next_p = if (i < data.len) i + 1 else data.len;
    return LineSlice {
        text: data[pos..end],
        line_start: pos,
        next_pos: next_p,
    };
}

fn splitFirstToken(s: []const u8): TokenSlice? {
    let i: usize = 0;
    while (i < s.len && isWs(s[i])) {
        i += 1;
    }
    if (i >= s.len) {
        return null;
    }
    const start = i;
    while (i < s.len && !isWs(s[i])) {
        i += 1;
    }
    let r = i;
    while (r < s.len && isWs(s[r])) {
        r += 1;
    }
    return TokenSlice {
        token: s[start..i],
        rest: s[r..],
    };
}

fn parseI32(s: []const u8): i32? {
    if (s.len == 0) {
        return null;
    }
    return std.fmt.parseInt(i32, s, 10) catch return null;
}

fn parseU32(s: []const u8): u32? {
    if (s.len == 0) {
        return null;
    }
    return std.fmt.parseInt(u32, s, 10) catch return null;
}

fn hexNibble(b: u8): u8? {
    if (b >= '0' && b <= '9') return b - '0';
    if (b >= 'A' && b <= 'F') return b - 'A' + 10;
    if (b >= 'a' && b <= 'f') return b - 'a' + 10;
    return null;
}

fn parseHexU8(hi: u8, lo: u8): u8? {
    const h = hexNibble(hi) ?? return null;
    const l = hexNibble(lo) ?? return null;
    return (h << 4) | l;
}

/// Strips surrounding double quotes from a BDF property string value while respecting `""` escapes.
fn unquoteBdfValue(raw: []const u8): []const u8 {
    const t = trimWs(raw);
    if (t.len < 2 || t[0] != '"') {
        return t;
    }
    let i: usize = 1;
    while (i < t.len) {
        if (t[i] == '"') {
            if (i + 1 < t.len && t[i + 1] == '"') {
                // Escaped double quote `""` inside BDF string literal
                i += 2;
                continue;
            }
            return t[1..i];
        }
        i += 1;
    }
    return t;
}

// ---------------------------------------------------------------------------
// 1. Detection & Global Font Header Parser
// ---------------------------------------------------------------------------

/// Returns `true` if `data` starts with `"STARTFONT "` on its first line.
pub fn isBdf(data: []const u8): bool {
    const first = nextLine(data, 0) ?? return false;
    const prefix = "STARTFONT ";
    return first.text.len >= prefix.len && std.mem.eql(u8, first.text[0..prefix.len], prefix);
}

/// Parses the global Adobe BDF 2.1/2.2 font header up through `CHARS` (or first `STARTCHAR`).
pub fn parseBdfHeader(data: []const u8): BdfHeader? {
    if (!isBdf(data)) {
        return null;
    }
    let hdr = BdfHeader {};
    let pos: usize = 0;
    let in_props = false;
    let found_startfont = false;

    while (ls := nextLine(data, pos)) {
        pos = ls.next_pos;
        const tok_opt = splitFirstToken(ls.text);
        if (tok_opt == null) {
            continue;
        }
        const tok = tok_opt.?;
        const kw = tok.token;
        const rest = trimWs(tok.rest);

        if (std.mem.eql(u8, kw, "STARTFONT")) {
            if (rest.len == 0) {
                return null;
            }
            hdr.version = rest;
            found_startfont = true;
        } else if (std.mem.eql(u8, kw, "FONT")) {
            hdr.font_name = rest;
        } else if (std.mem.eql(u8, kw, "SIZE")) {
            const t1 = splitFirstToken(rest) ?? return null;
            const t2 = splitFirstToken(t1.rest) ?? return null;
            const t3 = splitFirstToken(t2.rest) ?? return null;
            hdr.point_size = parseI32(t1.token) ?? return null;
            hdr.x_res = parseI32(t2.token) ?? return null;
            hdr.y_res = parseI32(t3.token) ?? return null;
        } else if (std.mem.eql(u8, kw, "FONTBOUNDINGBOX")) {
            const t1 = splitFirstToken(rest) ?? return null;
            const t2 = splitFirstToken(t1.rest) ?? return null;
            const t3 = splitFirstToken(t2.rest) ?? return null;
            const t4 = splitFirstToken(t3.rest) ?? return null;
            hdr.bbox = BdfBBox {
                width: parseI32(t1.token) ?? return null,
                height: parseI32(t2.token) ?? return null,
                x_off: parseI32(t3.token) ?? return null,
                y_off: parseI32(t4.token) ?? return null,
            };
        } else if (std.mem.eql(u8, kw, "STARTPROPERTIES")) {
            in_props = true;
        } else if (std.mem.eql(u8, kw, "ENDPROPERTIES")) {
            in_props = false;
        } else if (in_props && std.mem.eql(u8, kw, "FONT_ASCENT")) {
            if (v := parseI32(rest)) {
                hdr.ascent = v;
            }
        } else if (in_props && std.mem.eql(u8, kw, "FONT_DESCENT")) {
            if (v := parseI32(rest)) {
                hdr.descent = v;
            }
        } else if (std.mem.eql(u8, kw, "CHARS")) {
            const t1 = splitFirstToken(rest) ?? return null;
            hdr.declared_chars = parseU32(t1.token) ?? return null;
            hdr.chars_offset = ls.next_pos;
            break;
        } else if (std.mem.eql(u8, kw, "STARTCHAR")) {
            hdr.chars_offset = ls.line_start;
            break;
        }
    }

    if (!found_startfont) {
        return null;
    }
    return hdr;
}

// ---------------------------------------------------------------------------
// 2. STARTPROPERTIES .. ENDPROPERTIES Iterator
// ---------------------------------------------------------------------------

/// Zero-copy iterator over font properties between `STARTPROPERTIES` and `ENDPROPERTIES`.
pub struct BdfPropIter {
    src: []const u8 = "",
    pos: usize = 0,
    done: bool = true,

    fn init(data: []const u8): BdfPropIter {
        let p: usize = 0;
        while (ls := nextLine(data, p)) {
            p = ls.next_pos;
            if (tok := splitFirstToken(ls.text)) {
                if (std.mem.eql(u8, tok.token, "STARTPROPERTIES")) {
                    return BdfPropIter { src: data, pos: p, done: false };
                }
                if (std.mem.eql(u8, tok.token, "STARTCHAR")) {
                    break;
                }
            }
        }
        return BdfPropIter { src: data, pos: data.len, done: true };
    }

    fn next(*_): BdfProperty? {
        if (_.done) {
            return null;
        }
        while (ls := nextLine(_.src, _.pos)) {
            _.pos = ls.next_pos;
            const tok_opt = splitFirstToken(ls.text);
            if (tok_opt == null) {
                continue;
            }
            const tok = tok_opt.?;
            if (std.mem.eql(u8, tok.token, "ENDPROPERTIES") ||
                std.mem.eql(u8, tok.token, "CHARS") ||
                std.mem.eql(u8, tok.token, "STARTCHAR"))
            {
                _.done = true;
                return null;
            }
            if (std.mem.eql(u8, tok.token, "COMMENT")) {
                continue;
            }
            return BdfProperty {
                name: tok.token,
                value: unquoteBdfValue(tok.rest),
            };
        }
        _.done = true;
        return null;
    }
}

// ---------------------------------------------------------------------------
// 3. STARTCHAR .. ENDCHAR Glyph Iterator & Pixel Decoder
// ---------------------------------------------------------------------------

/// Zero-copy iterator over all `STARTCHAR` .. `ENDCHAR` glyph blocks in a BDF font.
pub struct BdfGlyphIter {
    src: []const u8 = "",
    pos: usize = 0,
    done: bool = true,

    fn init(data: []const u8): BdfGlyphIter {
        return BdfGlyphIter {
            src: data,
            pos: 0,
            done: data.len == 0,
        };
    }

    /// Initializes glyph iteration directly from a parsed `BdfHeader.chars_offset`.
    fn initFromHeader(data: []const u8, hdr: BdfHeader): BdfGlyphIter {
        const start = if (hdr.chars_offset <= data.len) hdr.chars_offset else data.len;
        return BdfGlyphIter {
            src: data,
            pos: start,
            done: start >= data.len,
        };
    }

    fn next(*_): BdfGlyph? {
        if (_.done) {
            return null;
        }

        // Advance to next STARTCHAR
        let glyph = BdfGlyph {};
        let found_start = false;
        while (ls := nextLine(_.src, _.pos)) {
            _.pos = ls.next_pos;
            const tok_opt = splitFirstToken(ls.text);
            if (tok_opt == null) {
                continue;
            }
            const tok = tok_opt.?;
            if (std.mem.eql(u8, tok.token, "ENDFONT")) {
                _.done = true;
                return null;
            }
            if (std.mem.eql(u8, tok.token, "STARTCHAR")) {
                glyph.name = trimWs(tok.rest);
                found_start = true;
                break;
            }
        }
        if (!found_start) {
            _.done = true;
            return null;
        }

        // Parse glyph metadata until BITMAP, then capture bitmap_slice until ENDCHAR
        let in_bitmap = false;
        let bitmap_start: usize = 0;

        while (ls := nextLine(_.src, _.pos)) {
            _.pos = ls.next_pos;
            const tok_opt = splitFirstToken(ls.text);
            if (tok_opt == null) {
                continue;
            }
            const tok = tok_opt.?;
            const kw = tok.token;
            const rest = trimWs(tok.rest);

            if (std.mem.eql(u8, kw, "ENDCHAR")) {
                if (in_bitmap && ls.line_start >= bitmap_start) {
                    glyph.bitmap_slice = _.src[bitmap_start..ls.line_start];
                }
                return glyph;
            }
            if (std.mem.eql(u8, kw, "STARTCHAR") || std.mem.eql(u8, kw, "ENDFONT")) {
                // Missing ENDCHAR for previous glyph -> malformed block
                _.done = true;
                return null;
            }
            if (in_bitmap) {
                continue;
            }

            if (std.mem.eql(u8, kw, "ENCODING")) {
                if (t1 := splitFirstToken(rest)) {
                    if (v := parseI32(t1.token)) {
                        glyph.encoding = v;
                    }
                }
            } else if (std.mem.eql(u8, kw, "DWIDTH")) {
                if (t1 := splitFirstToken(rest)) {
                    if (t2 := splitFirstToken(t1.rest)) {
                        if (dx := parseI32(t1.token)) {
                            glyph.dwidth_x = dx;
                        }
                        if (dy := parseI32(t2.token)) {
                            glyph.dwidth_y = dy;
                        }
                    }
                }
            } else if (std.mem.eql(u8, kw, "BBX")) {
                if (t1 := splitFirstToken(rest)) {
                    if (t2 := splitFirstToken(t1.rest)) {
                        if (t3 := splitFirstToken(t2.rest)) {
                            if (t4 := splitFirstToken(t3.rest)) {
                                const w_opt = parseI32(t1.token);
                                const h_opt = parseI32(t2.token);
                                const xo_opt = parseI32(t3.token);
                                const yo_opt = parseI32(t4.token);
                                if (w_opt != null && h_opt != null && xo_opt != null && yo_opt != null) {
                                    glyph.bbox = BdfBBox {
                                        width: w_opt.?,
                                        height: h_opt.?,
                                        x_off: xo_opt.?,
                                        y_off: yo_opt.?,
                                    };
                                }
                            }
                        }
                    }
                }
            } else if (std.mem.eql(u8, kw, "BITMAP")) {
                in_bitmap = true;
                bitmap_start = ls.next_pos;
            }
        }

        // Reached EOF without ENDCHAR
        _.done = true;
        return null;
    }
}

/// Queries whether pixel `(row, col)` (0-based from top-left) is lit in `glyph.bitmap_slice`.
/// Decodes MSB-first with `((width + 7) / 8) * 2` hex characters per row.
/// Returns `null` if `(row, col)` is out of bounds or the bitmap row is missing/malformed.
pub fn bdfGlyphPixel(glyph: BdfGlyph, row: u32, col: u32): bool? {
    if (glyph.bbox.width <= 0 || glyph.bbox.height <= 0) {
        return null;
    }
    const w = glyph.bbox.width as u32;
    const h = glyph.bbox.height as u32;
    if (row >= h || col >= w) {
        return null;
    }

    const row_bytes: usize = ((w + 7) / 8) as usize;
    const row_hex_len: usize = row_bytes * 2;

    let p: usize = 0;
    let cur_row: u32 = 0;
    while (ls := nextLine(glyph.bitmap_slice, p)) {
        p = ls.next_pos;
        const hex_line = trimWs(ls.text);
        if (hex_line.len == 0) {
            continue;
        }
        if (cur_row == row) {
            if (hex_line.len < row_hex_len) {
                return null;
            }
            const byte_idx: usize = (col / 8) as usize;
            const bit_shift: u3 = (7 - (col % 8)) as u3;
            const byte_val = parseHexU8(hex_line[byte_idx * 2], hex_line[byte_idx * 2 + 1]) ?? return null;
            return ((byte_val >> bit_shift) & 1) != 0;
        }
        cur_row += 1;
    }
    return null;
}

/// Finds the first glyph matching `codepoint` (`ENCODING <codepoint>`) in a BDF font buffer.
pub fn findBdfGlyph(data: []const u8, codepoint: i32): BdfGlyph? {
    if (!isBdf(data)) {
        return null;
    }
    let it = BdfGlyphIter.init(data);
    while (g := it.next()) {
        if (g.encoding == codepoint) {
            return g;
        }
    }
    return null;
}
