// Origin: Pure-CP Stdlib (Zero-Alloc ArcSight CEF Rev 25 SIEM Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation Micro Focus / ArcSight Common Event Format (CEF Rev 25) library:
// optional Syslog prefix extraction, escaped-pipe header splitting, severity normalization,
// whitespace-aware Extension key=value iteration, header/value unescaping, and header encoding.

/// Standard CEF header prefix marker.
pub const CEF_PREFIX: []const u8 = "CEF:";

/// Zero-copy decoded CEF Rev 25 event.
pub struct CefEvent {
    syslog_prefix: []const u8 = "",
    version: u8 = 0,
    device_vendor: []const u8 = "",
    device_product: []const u8 = "",
    device_version: []const u8 = "",
    signature_id: []const u8 = "",
    name: []const u8 = "",
    severity_raw: []const u8 = "",
    extension: []const u8 = "",

    /// Maps `severity_raw` (numeric `"0".."10"` or text `"Low"`/`"Medium"`/`"High"`/`"Very-High"`)
    /// to a unified numeric level in `0..10`.
    fn severityLevel(): u8 {
        if (n := parseSmallU8(_.severity_raw)) {
            if (n <= 10) {
                return n;
            }
            return 10;
        }
        if (bytesEqualIgnoreCase(_.severity_raw, "low")) {
            return 3;
        }
        if (bytesEqualIgnoreCase(_.severity_raw, "medium")) {
            return 5;
        }
        if (bytesEqualIgnoreCase(_.severity_raw, "high")) {
            return 8;
        }
        if (bytesEqualIgnoreCase(_.severity_raw, "very-high") || bytesEqualIgnoreCase(_.severity_raw, "very high")) {
            return 10;
        }
        return 0;
    }

    /// Returns a zero-allocation iterator over `key=value` pairs in `extension`.
    fn extensions(): CefExtIter {
        return CefExtIter.init(_.extension);
    }

    /// Finds the raw value for `key` in `extension`, or `null` if not present.
    fn findExt(key: []const u8): ([]const u8)? {
        return findCefExt(_.extension, key);
    }
}

/// Zero-copy Extension `key=value` pair (`raw_value` retains CEF escape sequences).
pub struct CefExtPair {
    key: []const u8 = "",
    raw_value: []const u8 = "",
}

// ---------------------------------------------------------------------------
// Internal ASCII, Escaping & Boundary Helpers (16-lane SIMD + Comptime LUTs)
// ---------------------------------------------------------------------------

type CefVec16 = @Vector(16, u8);

fn makeCefLowerLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if (i >= 'A' && i <= 'Z') {
            lut[i] = @as(u8, @intCast(i + 32));
        } else {
            lut[i] = @as(u8, @intCast(i));
        }
    }
    return lut;
}

fn makeCefKeyLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    let i: usize = 0;
    while (i < 256) : (i += 1) {
        if ((i >= 'a' && i <= 'z') || (i >= 'A' && i <= 'Z') || (i >= '0' && i <= '9') || i == '_' || i == '.' || i == '-') {
            lut[i] = 1;
        }
    }
    return lut;
}

const CEF_LOWER_LUT: [u8; 256] = makeCefLowerLut();
const CEF_KEY_LUT: [u8; 256] = makeCefKeyLut();

fn toLowerAscii(ch: u8): u8 {
    return CEF_LOWER_LUT[usize(ch)];
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= a.len) : (i += 16) {
        const va: CefVec16 = a[i .. i + 16][0..16].*;
        const vb: CefVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) return false;
    }
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn bytesEqualIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    const lo_A: CefVec16 = @splat(65 as u8);
    const hi_Z: CefVec16 = @splat(90 as u8);
    const case_bit: CefVec16 = @splat(0x20 as u8);
    while (i + 16 <= a.len) : (i += 16) {
        const va: CefVec16 = a[i .. i + 16][0..16].*;
        const vb: CefVec16 = b[i .. i + 16][0..16].*;
        const ma = (va >= lo_A) & (va <= hi_Z);
        const mb = (vb >= lo_A) & (vb <= hi_Z);
        const la = @select(u8, ma, va | case_bit, va);
        const lb = @select(u8, mb, vb | case_bit, vb);
        if (@reduce(.Or, la ^ lb) != 0) return false;
    }
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn cefFindByteSimd(s: []const u8, start: usize, target: u8): usize {
    let i = start;
    const t_v: CefVec16 = @splat(target);
    while (i + 16 <= s.len) : (i += 16) {
        const v: CefVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast(v == t_v);
        if (mask != 0) return i + (@ctz(mask) as usize);
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == target) return i;
    }
    return s.len;
}

fn cefFindPipeOrBsSimd(s: []const u8, start: usize): usize {
    let i = start;
    const p_v: CefVec16 = @splat('|' as u8);
    const bs_v: CefVec16 = @splat('\\' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: CefVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast((v == p_v) | (v == bs_v));
        if (mask != 0) return i + (@ctz(mask) as usize);
    }
    while (i < s.len) : (i += 1) {
        if (s[i] == '|' || s[i] == '\\') return i;
    }
    return s.len;
}

fn cefFindExtBreakSimd(s: []const u8, start: usize): usize {
    let i = start;
    const bs_v: CefVec16 = @splat('\\' as u8);
    const sp_v: CefVec16 = @splat(' ' as u8);
    const tb_v: CefVec16 = @splat('\t' as u8);
    while (i + 16 <= s.len) : (i += 16) {
        const v: CefVec16 = s[i .. i + 16][0..16].*;
        const mask: u16 = @bitCast((v == bs_v) | (v == sp_v) | (v == tb_v));
        if (mask != 0) return i + (@ctz(mask) as usize);
    }
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '\\' || c == ' ' || c == '\t') return i;
    }
    return s.len;
}

fn stripLineEnding(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && (s[end - 1] == '\r' || s[end - 1] == '\n')) {
        end -= 1;
    }
    return s[0..end];
}

fn trimRightSpaces(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && (s[end - 1] == ' ' || s[end - 1] == '\t')) {
        end -= 1;
    }
    return s[0..end];
}

fn parseSmallU8(s: []const u8): u8? {
    if (s.len == 0) {
        return null;
    }
    let acc: u16 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < '0' || ch > '9') {
            return null;
        }
        acc = acc * 10 + ((ch - '0') as u16);
        if (acc > 255) {
            return null;
        }
        i += 1;
    }
    return acc as u8;
}

fn isCefKeyChar(ch: u8): bool {
    return CEF_KEY_LUT[usize(ch)] != 0;
}

/// Locates the byte offset of `"CEF:"` either at index 0 or immediately after whitespace.
fn findCefStart(line: []const u8): usize? {
    if (line.len < 4) {
        return null;
    }
    let i: usize = 0;
    while (i + 4 <= line.len) {
        i = cefFindByteSimd(line, i, 'C');
        if (i + 4 > line.len) break;
        if (line[i + 1] == 'E' && line[i + 2] == 'F' && line[i + 3] == ':') {
            if (i == 0 || line[i - 1] == ' ' || line[i - 1] == '\t') {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

/// Finds the next unescaped `'|'` starting at `start`, respecting `\\` and `\|`.
fn findUnescapedPipe(s: []const u8, start: usize): usize? {
    let i = start;
    while (i < s.len) {
        i = cefFindPipeOrBsSimd(s, i);
        if (i >= s.len) break;
        if (s[i] == '\\') {
            if (i + 1 < s.len) {
                i += 2;
            } else {
                i += 1;
            }
            continue;
        }
        return i;
    }
    return null;
}

// ---------------------------------------------------------------------------
// Public Detection & Header Parsing API
// ---------------------------------------------------------------------------

/// Returns `true` if `line` is a valid CEF event (with or without Syslog prefix).
pub fn isCef(line: []const u8): bool {
    if (ev := parseCefLine(line)) {
        * = ev;
        return true;
    }
    return false;
}

/// Parses a single CEF Rev 25 log line (with optional Syslog prefix) into a zero-copy `CefEvent`.
/// Returns `null` if `"CEF:"` is missing, version is invalid, or fewer than 7 unescaped pipes exist.
pub fn parseCefLine(line: []const u8): CefEvent? {
    const clean = stripLineEnding(line);
    let cef_pos: usize = 0;
    if (p := findCefStart(clean)) {
        cef_pos = p;
    } else {
        return null;
    }

    const syslog_prefix = trimRightSpaces(clean[0..cef_pos]);
    const rest = clean[cef_pos + 4..];

    let pipes: [7]usize = [0; 7];
    let cur: usize = 0;
    let p_idx: usize = 0;
    while (p_idx < 7) {
        if (p := findUnescapedPipe(rest, cur)) {
            pipes[p_idx] = p;
            cur = p + 1;
            p_idx += 1;
        } else {
            return null;
        }
    }

    const ver_slice = rest[0..pipes[0]];
    let version: u8 = 0;
    if (v := parseSmallU8(ver_slice)) {
        version = v;
    } else {
        return null;
    }

    return CefEvent {
        syslog_prefix: syslog_prefix,
        version: version,
        device_vendor: rest[pipes[0] + 1..pipes[1]],
        device_product: rest[pipes[1] + 1..pipes[2]],
        device_version: rest[pipes[2] + 1..pipes[3]],
        signature_id: rest[pipes[3] + 1..pipes[4]],
        name: rest[pipes[4] + 1..pipes[5]],
        severity_raw: rest[pipes[5] + 1..pipes[6]],
        extension: rest[pipes[6] + 1..],
    };
}

// ---------------------------------------------------------------------------
// Extension Key=Value Streaming Iterator & Lookup
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over CEF Extension `key=value` pairs.
/// Allows spaces inside values and splits strictly at `<space><valid_key>=` boundaries
/// while respecting `\=`, `\\`, `\n`, and `\r` escapes.
pub struct CefExtIter {
    ext: []const u8 = "",
    pos: usize = 0,

    pub static fn init(extension: []const u8): CefExtIter {
        return CefExtIter {
            ext: extension,
            pos: 0,
        };
    }

    fn next(*_): CefExtPair? {
        while (_.pos < _.ext.len && (_.ext[_.pos] == ' ' || _.ext[_.pos] == '\t')) {
            _.pos += 1;
        }
        if (_.pos >= _.ext.len) {
            return null;
        }

        // Read key up to '='
        const key_start = _.pos;
        let eq_pos = key_start;
        while (eq_pos < _.ext.len && isCefKeyChar(_.ext[eq_pos])) {
            eq_pos += 1;
        }
        if (eq_pos == key_start || eq_pos >= _.ext.len || _.ext[eq_pos] != '=') {
            _.pos = _.ext.len;
            return null;
        }

        const key = _.ext[key_start..eq_pos];
        const val_start = eq_pos + 1;
        let scan = val_start;

        while (scan < _.ext.len) {
            scan = cefFindExtBreakSimd(_.ext, scan);
            if (scan >= _.ext.len) break;
            const ch = _.ext[scan];
            if (ch == '\\') {
                if (scan + 1 < _.ext.len) {
                    scan += 2;
                } else {
                    scan += 1;
                }
                continue;
            }
            const ws_pos = scan;
            let cand = scan + 1;
            while (cand < _.ext.len && (_.ext[cand] == ' ' || _.ext[cand] == '\t')) {
                cand += 1;
            }
            // Check whether `cand` starts `<valid_key>=`
            let k = cand;
            while (k < _.ext.len && isCefKeyChar(_.ext[k])) {
                k += 1;
            }
            if (k > cand && k < _.ext.len && _.ext[k] == '=') {
                _.pos = cand;
                return CefExtPair {
                    key: key,
                    raw_value: _.ext[val_start..ws_pos],
                };
            }
            scan = cand;
        }

        _.pos = _.ext.len;
        return CefExtPair {
            key: key,
            raw_value: _.ext[val_start.._.ext.len],
        };
    }
}

/// Finds the raw value for `key` (exact match) inside a CEF Extension string.
pub fn findCefExt(extension: []const u8, key: []const u8): ([]const u8)? {
    let it = CefExtIter.init(extension);
    while (pair := it.next()) {
        if (bytesEqual(pair.key, key)) {
            return pair.raw_value;
        }
    }
    return null;
}

// ---------------------------------------------------------------------------
// Zero-Alloc Unescaping & Header Encoding
// ---------------------------------------------------------------------------

/// Unescapes a CEF header field (`\|` -> `|`, `\\` -> `\`) into `out`.
/// Returns bytes written, or `null` if `out` has insufficient capacity.
pub fn unescapeCefHeader(raw: []const u8, out: []u8): usize? {
    let i: usize = 0;
    let w: usize = 0;
    while (i < raw.len) {
        const next_bs = cefFindByteSimd(raw, i, '\\');
        const plain = next_bs - i;
        if (plain > 0) {
            if (w + plain > out.len) return null;
            @memcpy(out[w .. w + plain], raw[i..next_bs]);
            w += plain;
            i = next_bs;
        }
        if (i >= raw.len) break;
        if (w >= out.len) {
            return null;
        }
        if (i + 1 < raw.len) {
            const nxt = raw[i + 1];
            if (nxt == '|' || nxt == '\\') {
                out[w] = nxt;
                w += 1;
                i += 2;
                continue;
            }
        }
        out[w] = raw[i];
        w += 1;
        i += 1;
    }
    return w;
}

/// Unescapes a CEF Extension value (`\=` -> `=`, `\\` -> `\`, `\n` -> LF, `\r` -> CR) into `out`.
/// Returns bytes written, or `null` if `out` has insufficient capacity.
pub fn unescapeCefExtValue(raw: []const u8, out: []u8): usize? {
    let i: usize = 0;
    let w: usize = 0;
    while (i < raw.len) {
        const next_bs = cefFindByteSimd(raw, i, '\\');
        const plain = next_bs - i;
        if (plain > 0) {
            if (w + plain > out.len) return null;
            @memcpy(out[w .. w + plain], raw[i..next_bs]);
            w += plain;
            i = next_bs;
        }
        if (i >= raw.len) break;
        if (w >= out.len) {
            return null;
        }
        if (i + 1 < raw.len) {
            const nxt = raw[i + 1];
            if (nxt == '=' || nxt == '\\') {
                out[w] = nxt;
                w += 1;
                i += 2;
                continue;
            }
            if (nxt == 'n') {
                out[w] = '\n';
                w += 1;
                i += 2;
                continue;
            }
            if (nxt == 'r') {
                out[w] = '\r';
                w += 1;
                i += 2;
                continue;
            }
        }
        out[w] = raw[i];
        w += 1;
        i += 1;
    }
    return w;
}

fn writeRaw(out: []u8, pos: usize, s: []const u8): usize? {
    if (pos + s.len > out.len) {
        return null;
    }
    if (s.len > 0) {
        @memcpy(out[pos .. pos + s.len], s);
    }
    return pos + s.len;
}

fn writeEscapedHeaderField(out: []u8, pos: usize, s: []const u8): usize? {
    let w = pos;
    let i: usize = 0;
    while (i < s.len) {
        const hit = cefFindPipeOrBsSimd(s, i);
        const plain = hit - i;
        if (plain > 0) {
            if (w + plain > out.len) return null;
            @memcpy(out[w .. w + plain], s[i..hit]);
            w += plain;
            i = hit;
        }
        if (i >= s.len) break;
        if (w + 2 > out.len) {
            return null;
        }
        out[w] = '\\';
        out[w + 1] = s[i];
        w += 2;
        i += 1;
    }
    return w;
}

fn writeU8Decimal(out: []u8, pos: usize, v: u8): usize? {
    let tmp: [3]u8 = [0; 3];
    let n: usize = 0;
    let x = v;
    if (x == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (x > 0) {
            tmp[n] = ('0' as u8) + (x % 10);
            x = x / 10;
            n += 1;
        }
    }
    if (pos + n > out.len) {
        return null;
    }
    let i: usize = 0;
    while (i < n) {
        out[pos + i] = tmp[n - 1 - i];
        i += 1;
    }
    return pos + n;
}

/// Encodes a CEF header (`CEF:<ver>|<vendor>|<product>|<dev_ver>|<sig_id>|<name>|<severity>|`)
/// into `out`, automatically escaping `|` and `\` in header fields.
/// Returns total bytes written, or `null` if `out` is too small.
pub fn formatCefHeader(
    version: u8,
    vendor: []const u8,
    product: []const u8,
    dev_ver: []const u8,
    sig_id: []const u8,
    name: []const u8,
    severity: []const u8,
    out: []u8,
): usize? {
    let pos: usize = 0;
    if (p := writeRaw(out, pos, CEF_PREFIX)) { pos = p; } else { return null; }
    if (p := writeU8Decimal(out, pos, version)) { pos = p; } else { return null; }

    const fields: [6][]const u8 = [vendor, product, dev_ver, sig_id, name, severity];
    let i: usize = 0;
    while (i < 6) {
        if (pos >= out.len) { return null; }
        out[pos] = '|';
        pos += 1;
        if (p := writeEscapedHeaderField(out, pos, fields[i])) { pos = p; } else { return null; }
        i += 1;
    }
    if (pos >= out.len) { return null; }
    out[pos] = '|';
    pos += 1;
    return pos;
}
