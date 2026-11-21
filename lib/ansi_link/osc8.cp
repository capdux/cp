// Source: Absorbed from /data/dev/bun/src/output.zig (OSC 8 Terminal Hyperlink Codec)
// Pure-CP zero-allocation OSC 8 terminal hyperlink formatter, parser, and stripper.
// Supports both ST (ESC \) and BEL (0x07) terminators per the OSC 8 hyperlink specification.

/// Parsed OSC 8 terminal hyperlink components (zero-copy slices referencing input).
pub struct Osc8Link {
    url: []const u8;
    text: []const u8;
    params: []const u8;
}

/// Check if a slice contains C0 control bytes (< 0x20) or DEL (0x7F),
/// which are forbidden inside OSC 8 parameter and URI fields.
fn hasControlByte(src: []const u8): bool {
    let i: usize = 0;
    while (i < src.len) {
        const b = src[i];
        if (b < 0x20 || b == 0x7F) return true;
        i += 1;
    }
    return false;
}

/// Check if `src` at offset `pos` starts with the 4-byte OSC 8 introducer `\x1b]8;`.
fn isOsc8IntroAt(src: []const u8, pos: usize): bool {
    if (pos + 4 > src.len) return false;
    return src[pos] == 0x1B && src[pos + 1] == ']' && src[pos + 2] == '8' && src[pos + 3] == ';';
}

/// Copy `src` into `dst` starting at `offset`, returning the updated offset.
fn writeBytes(dst: []u8, offset: usize, src: []const u8): usize {
    let i: usize = 0;
    while (i < src.len) {
        dst[offset + i] = src[i];
        i += 1;
    }
    return offset + src.len;
}

/// Find the end of an OSC terminator (BEL `0x07` or ST `ESC \`) starting at `pos`.
/// Returns the index immediately after the terminator, or `null` if unterminated.
fn findOscTerminatorEnd(src: []const u8, pos: usize): usize? {
    let i = pos;
    while (i < src.len) {
        if (src[i] == 0x07) {
            return i + 1;
        }
        if (src[i] == 0x1B && i + 1 < src.len && src[i + 1] == '\\') {
            return i + 2;
        }
        i += 1;
    }
    return null;
}

/// Compute the exact encoded byte length of an ST-terminated OSC 8 hyperlink.
pub fn osc8EncodedLen(url: []const u8, text: []const u8, params: []const u8): usize {
    // "\x1b]8;" (4) + params + ";" (1) + url + "\x1b\\" (2) + text + "\x1b]8;;\x1b\\" (7)
    return 14 + params.len + url.len + text.len;
}

/// Compute the exact encoded byte length of a BEL-terminated OSC 8 hyperlink (without params).
pub fn osc8BelEncodedLen(url: []const u8, text: []const u8): usize {
    // "\x1b]8;;" (5) + url + "\x07" (1) + text + "\x1b]8;;\x07" (6)
    return 12 + url.len + text.len;
}

/// Format an OSC 8 hyperlink using ST (`\x1b\\`) terminators:
/// `\x1b]8;<params>;<url>\x1b\\<text>\x1b]8;;\x1b\\`.
/// Returns `null` if `out` is too small or if `url`/`params` contain control bytes.
pub fn formatOsc8(url: []const u8, text: []const u8, params: []const u8, out: []u8): ([]const u8)? {
    if (hasControlByte(url) || hasControlByte(params)) return null;
    const needed = osc8EncodedLen(url, text, params);
    if (needed > out.len) return null;

    let w: usize = 0;
    w = writeBytes(out, w, "\x1b]8;");
    w = writeBytes(out, w, params);
    out[w] = ';';
    w += 1;
    w = writeBytes(out, w, url);
    w = writeBytes(out, w, "\x1b\\");
    w = writeBytes(out, w, text);
    w = writeBytes(out, w, "\x1b]8;;\x1b\\");
    return out[0..w];
}

/// Format an OSC 8 hyperlink using BEL (`\x07`) terminators:
/// `\x1b]8;;<url>\x07<text>\x1b]8;;\x07`.
/// Returns `null` if `out` is too small or if `url` contains control bytes.
pub fn formatOsc8Bel(url: []const u8, text: []const u8, out: []u8): ([]const u8)? {
    if (hasControlByte(url)) return null;
    const needed = osc8BelEncodedLen(url, text);
    if (needed > out.len) return null;

    let w: usize = 0;
    w = writeBytes(out, w, "\x1b]8;;");
    w = writeBytes(out, w, url);
    out[w] = 0x07;
    w += 1;
    w = writeBytes(out, w, text);
    w = writeBytes(out, w, "\x1b]8;;\x07");
    return out[0..w];
}

/// Parse a single complete OSC 8 hyperlink sequence (`\x1b]8;<params>;<url><TERM><text>\x1b]8;;<TERM>`).
/// Supports both ST (`\x1b\\`) and BEL (`\x07`) terminators (including mixed open/close terminators).
/// Rejects unterminated sequences, missing delimiters, nested OSC 8 openings, or trailing bytes.
pub fn parseOsc8(input: []const u8): Osc8Link? {
    if (!isOsc8IntroAt(input, 0)) return null;

    // 1. Parse params up to the second ';'
    let i: usize = 4;
    let semi_found = false;
    let semi_pos: usize = 0;
    while (i < input.len) {
        const b = input[i];
        if (b == ';') {
            semi_found = true;
            semi_pos = i;
            break;
        }
        if (b == 0x07 || b == 0x1B) return null;
        i += 1;
    }
    if (!semi_found) return null;
    const params = input[4..semi_pos];

    // 2. Parse URL up to ST (ESC \) or BEL (0x07)
    const url_start = semi_pos + 1;
    let url_end: usize = url_start;
    let text_start: usize = 0;
    let open_closed = false;
    i = url_start;
    while (i < input.len) {
        if (input[i] == 0x07) {
            url_end = i;
            text_start = i + 1;
            open_closed = true;
            break;
        }
        if (input[i] == 0x1B) {
            if (i + 1 < input.len && input[i + 1] == '\\') {
                url_end = i;
                text_start = i + 2;
                open_closed = true;
                break;
            }
            return null;
        }
        i += 1;
    }
    if (!open_closed) return null;
    const url = input[url_start..url_end];

    // 3. Scan visible text until the closing OSC 8 sequence `\x1b]8;<opt_params>;<TERM>`
    i = text_start;
    while (i < input.len) {
        if (isOsc8IntroAt(input, i)) {
            const close_start = i;
            let j: usize = i + 4;
            let close_semi = false;
            while (j < input.len) {
                if (input[j] == ';') {
                    close_semi = true;
                    j += 1;
                    break;
                }
                if (input[j] == 0x07 || input[j] == 0x1B) return null;
                j += 1;
            }
            if (!close_semi) return null;

            // Closing sequence must have an empty URL immediately followed by BEL or ST
            let close_end: usize = 0;
            if (j < input.len && input[j] == 0x07) {
                close_end = j + 1;
            } else if (j + 1 < input.len && input[j] == 0x1B && input[j + 1] == '\\') {
                close_end = j + 2;
            } else {
                // Non-empty URL inside text (nested/malformed OSC 8) or unterminated close
                return null;
            }

            // Must consume the entire input for a single OSC 8 sequence
            if (close_end != input.len) return null;

            return Osc8Link {
                url: url,
                text: input[text_start..close_start],
                params: params,
            };
        }
        i += 1;
    }

    // Missing closing sequence
    return null;
}

/// Strip all OSC 8 control sequences from `input`, preserving visible text and non-OSC-8 bytes.
/// Returns the stripped slice written into `out`, or `null` if `out` is too small.
pub fn stripOsc8(input: []const u8, out: []u8): ([]const u8)? {
    let i: usize = 0;
    let w: usize = 0;
    while (i < input.len) {
        if (isOsc8IntroAt(input, i)) {
            if (term_end := findOscTerminatorEnd(input, i + 4)) {
                i = term_end;
            } else {
                // Unterminated OSC 8 header at end of input: drop trailing control sequence
                i = input.len;
            }
            continue;
        }
        if (w >= out.len) return null;
        out[w] = input[i];
        w += 1;
        i += 1;
    }
    return out[0..w];
}

/// Fast check whether `input` contains any OSC 8 hyperlink sequence introducer (`\x1b]8;`).
pub fn hasOsc8(input: []const u8): bool {
    if (input.len < 4) return false;
    let i: usize = 0;
    const limit = input.len - 3;
    while (i < limit) {
        if (input[i] == 0x1B && input[i + 1] == ']' && input[i + 2] == '8' && input[i + 3] == ';') {
            return true;
        }
        i += 1;
    }
    return false;
}
