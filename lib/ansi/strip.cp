// Source: Absorbed from /data/dev/bun/src/string/immutable.zig | Synced: 2026-11-22T13:00:28Z (ANSI/VT100 Escape Sequence Stripper & Visible Length)
// Pure-CP zero-allocation ANSI CSI/OSC/DCS/ESC stripper and visible ASCII/UTF-8 character counter.

/// Check if a byte is an ANSI CSI parameter byte (0x30..=0x3F).
fn isCsiParam(b: u8): bool {
    return b >= 0x30 && b <= 0x3F;
}

/// Check if a byte is an ANSI CSI intermediate byte (0x20..=0x2F).
fn isCsiIntermediate(b: u8): bool {
    return b >= 0x20 && b <= 0x2F;
}

/// Check if a byte is an ANSI CSI final byte (0x40..=0x7E).
fn isCsiFinal(b: u8): bool {
    return b >= 0x40 && b <= 0x7E;
}

/// Advance past a single ANSI/VT100 escape sequence starting at `src[pos] == 0x1B` (or C1 CSI 0x9B).
/// Returns the new index right after the escape sequence.
fn skipEscapeAt(src: []const u8, pos: usize): usize {
    if (pos >= src.len) return pos;
    let i = pos;
    if (src[i] == 0x9B) {
        // 8-bit C1 CSI
        i += 1;
        while (i < src.len && (isCsiParam(src[i]) || isCsiIntermediate(src[i]))) {
            i += 1;
        }
        if (i < src.len && isCsiFinal(src[i])) i += 1;
        return i;
    }
    if (src[i] != 0x1B) return i;
    i += 1;
    if (i >= src.len) return i;

    const next = src[i];
    if (next == '[') {
        // CSI: ESC [ params* intermediates* final
        i += 1;
        while (i < src.len && isCsiParam(src[i])) i += 1;
        while (i < src.len && isCsiIntermediate(src[i])) i += 1;
        if (i < src.len && isCsiFinal(src[i])) i += 1;
        return i;
    }
    if (next == ']' || next == 'P' || next == '_' || next == '^' || next == 'X') {
        // OSC (]), DCS (P), APC (_), PM (^), SOS (X): terminated by BEL (0x07) or ST (ESC \)
        i += 1;
        while (i < src.len) {
            if (src[i] == 0x07) {
                return i + 1;
            }
            if (src[i] == 0x1B && i + 1 < src.len && src[i + 1] == '\\') {
                return i + 2;
            }
            i += 1;
        }
        return i;
    }
    if (next == '(' || next == ')' || next == '*' || next == '+') {
        // Charset designation: ESC ( X
        i += 1;
        if (i < src.len) i += 1;
        return i;
    }
    // 2-byte ESC sequence (e.g. ESC 7, ESC 8, ESC c, ESC M)
    return i + 1;
}

/// Fast check whether `src` contains any ANSI escape sequence initiator (ESC 0x1B or C1 CSI 0x9B).
export fn hasAnsi(src: []const u8): bool {
    let i: usize = 0;
    while (i < src.len) {
        if (src[i] == 0x1B || src[i] == 0x9B) return true;
        i += 1;
    }
    return false;
}

/// Strip all ANSI/VT100 escape sequences (CSI, OSC hyperlinks, DCS, 2-byte ESC) into `out`.
/// Returns the stripped slice, or `null` if `out` is too small.
export fn stripAnsi(src: []const u8, out: []u8): ([]const u8)? {
    let i: usize = 0;
    let w: usize = 0;
    while (i < src.len) {
        if (src[i] == 0x1B || src[i] == 0x9B) {
            i = skipEscapeAt(src, i);
            continue;
        }
        if (w >= out.len) return null;
        out[w] = src[i];
        w += 1;
        i += 1;
    }
    return out[0..w];
}

/// Compute the byte length of `src` after stripping ANSI escape sequences (without allocating).
export fn strippedByteLen(src: []const u8): usize {
    let i: usize = 0;
    let count: usize = 0;
    while (i < src.len) {
        if (src[i] == 0x1B || src[i] == 0x9B) {
            i = skipEscapeAt(src, i);
            continue;
        }
        count += 1;
        i += 1;
    }
    return count;
}

/// Count visible UTF-8 codepoints in `src` (excluding ANSI escape sequences).
export fn visibleCodepointCount(src: []const u8): usize {
    let i: usize = 0;
    let count: usize = 0;
    while (i < src.len) {
        if (src[i] == 0x1B || src[i] == 0x9B) {
            i = skipEscapeAt(src, i);
            continue;
        }
        // Count non-continuation UTF-8 bytes
        if ((src[i] & 0xC0) != 0x80) {
            count += 1;
        }
        i += 1;
    }
    return count;
}
