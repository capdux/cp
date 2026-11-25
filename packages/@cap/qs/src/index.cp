// @cap/qs — Zero-allocation Query String Parser & Serializer
// Clean-room pure-CP implementation (inspired by npm `qs` & Rust `serde_qs`, MIT License).
// Supports nested brackets, dot notation, indexed & empty-bracket arrays, comma arrays,
// strict null handling, RFC 3986 percent-encoding, and depth/parameter-count DoS protection.

const std = @import("std");

pub const MAX_PARAMS: usize = 64;
pub const MAX_DEPTH: usize = 8;
pub const POOL_CAPACITY: usize = 4096;

pub err QsError {
    BufferTooSmall,
    TooManyParams,
    DepthExceeded,
    InvalidPercentEncoding,
    InvalidSyntax,
}

/// Configuration options for `QsParsed.parse` / `QsParsed.parseInto`.
pub struct QsParseOptions {
    max_params: usize = 64,
    max_depth: usize = 8,
    allow_dots: bool = false,
    comma_arrays: bool = false,
    ignore_query_prefix: bool = true,
    plus_as_space: bool = true,
    strict_null_handling: bool = false,
}

/// A single path segment parsed from a query string key (e.g. `a`, `b`, `0`, or `[]`).
pub struct QsPathSegment {
    name: []const u8 = "",
    is_index: bool = false,
    index: usize = 0,
    is_empty_bracket: bool = false,
    // Internal pool offsets to prevent dangling slices when QsParsed is returned by value.
    name_off: u16 = 0,
    name_len: u16 = 0,
    name_in_pool: bool = false,
}

/// A single parsed query string key-value entry with decoded key/value and path segments.
pub struct QsEntry {
    raw_key: []const u8 = "",
    decoded_key: []const u8 = "",
    decoded_value: []const u8 = "",
    is_null: bool = false,
    segments: [8]QsPathSegment = undefined,
    depth: usize = 0,
    // Internal pool offsets to prevent dangling slices when QsParsed is returned by value.
    key_off: u16 = 0,
    key_len: u16 = 0,
    key_in_pool: bool = false,
    val_off: u16 = 0,
    val_len: u16 = 0,
    val_in_pool: bool = false,
}

fn hexVal(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn isUnreserved(c: u8): bool {
    return (c >= 'A' && c <= 'Z')
        || (c >= 'a' && c <= 'z')
        || (c >= '0' && c <= '9')
        || c == '-' || c == '.' || c == '_' || c == '~';
}

fn needsDecode(s: []const u8, plus_as_space: bool): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '%' || (plus_as_space && c == '+')) return true;
    }
    return false;
}

fn parseDecimalIndex(s: []const u8): usize? {
    if (s.len == 0) return null;
    let val: usize = 0;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c < '0' || c > '9') return null;
        const digit = (c - '0') as usize;
        if (val > (std.math.maxInt(usize) - digit) / 10) return null;
        val = val * 10 + digit;
    }
    return val;
}

/// Decodes a percent-encoded byte slice into `out`.
/// When `plus_as_space` is true, `'+'` is decoded as `' '` (0x20).
/// Returns `QsError.InvalidPercentEncoding` on malformed `%XX` sequences and
/// `QsError.BufferTooSmall` if `out` cannot hold the decoded bytes.
pub fn percentDecode(
    input: []const u8,
    plus_as_space: bool,
    out: []u8,
): ([]const u8) ! QsError {
    let i: usize = 0;
    let pos: usize = 0;
    while (i < input.len) {
        const c = input[i];
        if (c == '%') {
            if (i + 2 >= input.len) return QsError.InvalidPercentEncoding;
            const hi = hexVal(input[i + 1]) ?? return QsError.InvalidPercentEncoding;
            const lo = hexVal(input[i + 2]) ?? return QsError.InvalidPercentEncoding;
            if (pos >= out.len) return QsError.BufferTooSmall;
            out[pos] = (hi << 4) | lo;
            pos += 1;
            i += 3;
        } else if (plus_as_space && c == '+') {
            if (pos >= out.len) return QsError.BufferTooSmall;
            out[pos] = ' ';
            pos += 1;
            i += 1;
        } else {
            if (pos >= out.len) return QsError.BufferTooSmall;
            out[pos] = c;
            pos += 1;
            i += 1;
        }
    }
    return out[0..pos];
}

/// Encodes `input` into `out` strictly following RFC 3986 unreserved characters
/// (`A-Z`, `a-z`, `0-9`, `-`, `.`, `_`, `~`).
/// When `space_as_plus` is true, `' '` is encoded as `'+'` instead of `%20`.
pub fn percentEncode(
    input: []const u8,
    space_as_plus: bool,
    out: []u8,
): ([]const u8) ! QsError {
    const hex_digits = "0123456789ABCDEF";
    let i: usize = 0;
    let pos: usize = 0;
    while (i < input.len) : (i += 1) {
        const c = input[i];
        if (isUnreserved(c)) {
            if (pos >= out.len) return QsError.BufferTooSmall;
            out[pos] = c;
            pos += 1;
        } else if (space_as_plus && c == ' ') {
            if (pos >= out.len) return QsError.BufferTooSmall;
            out[pos] = '+';
            pos += 1;
        } else {
            if (pos + 3 > out.len) return QsError.BufferTooSmall;
            out[pos] = '%';
            out[pos + 1] = hex_digits[(c >> 4) & 0x0F];
            out[pos + 2] = hex_digits[c & 0x0F];
            pos += 3;
        }
    }
    return out[0..pos];
}

fn makeSegment(
    slice: []const u8,
    base_off: u16,
    seg_rel_off: usize,
    in_pool: bool,
    is_empty_bracket: bool,
): QsPathSegment {
    if (is_empty_bracket) {
        return QsPathSegment{
            name: "",
            is_index: false,
            index: 0,
            is_empty_bracket: true,
            name_off: 0,
            name_len: 0,
            name_in_pool: false,
        };
    }
    let is_idx = false;
    let idx_val: usize = 0;
    if (parsed := parseDecimalIndex(slice)) {
        is_idx = true;
        idx_val = parsed;
    }
    return QsPathSegment{
        name: slice,
        is_index: is_idx,
        index: idx_val,
        is_empty_bracket: false,
        name_off: if (in_pool) (base_off + (seg_rel_off as u16)) else 0,
        name_len: if (in_pool) (slice.len as u16) else 0,
        name_in_pool: in_pool,
    };
}

/// Zero-allocation parsed query string container with an inline decoded string pool
/// and up to `MAX_PARAMS` (64) entries.
pub struct QsParsed {
    pool: [4096]u8 = undefined,
    pool_len: usize = 0,
    entries: [64]QsEntry = undefined,
    count: usize = 0,

    pub static fn init(): QsParsed {
        return QsParsed{
            pool: undefined,
            pool_len: 0,
            entries: undefined,
            count: 0,
        };
    }

    /// Parses a query string and returns a value-contained `QsParsed`.
    pub static fn parse(input: []const u8, opts: QsParseOptions): QsParsed ! QsError {
        let res = QsParsed.init();
        res.parseInto(input, opts)?;
        return res;
    }

    /// Parses a query string in-place into an existing `QsParsed` container (reusing stack memory).
    pub fn parseInto(*_, input: []const u8, opts: QsParseOptions): void ! QsError {
        _.pool_len = 0;
        _.count = 0;

        let src = input;
        if (opts.ignore_query_prefix && src.len > 0 && src[0] == '?') {
            src = src[1..];
        }
        if (src.len == 0) return;

        const max_params = if (opts.max_params > MAX_PARAMS) MAX_PARAMS else opts.max_params;
        const max_depth = if (opts.max_depth > MAX_DEPTH) MAX_DEPTH else opts.max_depth;

        let pos: usize = 0;
        while (pos < src.len) {
            const start = pos;
            while (pos < src.len && src[pos] != '&') : (pos += 1) {}
            const part = src[start..pos];
            if (pos < src.len && src[pos] == '&') {
                pos += 1;
            }
            if (part.len == 0) continue;

            // Split part on first '='
            let eq_idx: usize? = null;
            let k: usize = 0;
            while (k < part.len) : (k += 1) {
                if (part[k] == '=') {
                    eq_idx = k;
                    break;
                }
            }

            let raw_key: []const u8 = "";
            let raw_val: []const u8 = "";
            let has_eq = false;
            if (eq := eq_idx) {
                raw_key = part[0..eq];
                raw_val = part[eq + 1 ..];
                has_eq = true;
            } else {
                raw_key = part;
                raw_val = "";
                has_eq = false;
            }
            if (raw_key.len == 0) continue;

            // Decode key (zero-copy when no '%' or '+')
            let dec_key: []const u8 = raw_key;
            let key_in_pool = false;
            let key_off: u16 = 0;
            let key_len: u16 = 0;
            if (needsDecode(raw_key, opts.plus_as_space)) {
                const start_off = _.pool_len;
                const decoded = percentDecode(raw_key, opts.plus_as_space, _.pool[start_off..])?;
                _.pool_len += decoded.len;
                dec_key = decoded;
                key_in_pool = true;
                key_off = start_off as u16;
                key_len = decoded.len as u16;
            }

            // Parse key path segments
            let segs: [8]QsPathSegment = undefined;
            const depth = parseKeySegments(dec_key, key_off, key_in_pool, opts.allow_dots, max_depth, &segs)?;

            // Check comma-separated array splitting
            if (opts.comma_arrays && has_eq && hasByte(raw_val, ',')) {
                let vpos: usize = 0;
                let item_idx: usize = 0;
                while (true) {
                    const vstart = vpos;
                    while (vpos < raw_val.len && raw_val[vpos] != ',') : (vpos += 1) {}
                    const item_raw = raw_val[vstart..vpos];

                    if (_.count >= max_params) return QsError.TooManyParams;

                    let item_segs = segs;
                    let item_depth = depth;
                    const last_is_array = item_depth > 0
                        && (item_segs[item_depth - 1].is_empty_bracket || item_segs[item_depth - 1].is_index);
                    if (!last_is_array) {
                        if (item_depth >= max_depth) return QsError.DepthExceeded;
                        item_segs[item_depth] = QsPathSegment{
                            name: "",
                            is_index: true,
                            index: item_idx,
                            is_empty_bracket: false,
                            name_off: 0,
                            name_len: 0,
                            name_in_pool: false,
                        };
                        item_depth += 1;
                    }

                    let dec_val: []const u8 = item_raw;
                    let val_in_pool = false;
                    let val_off: u16 = 0;
                    let val_len: u16 = 0;
                    if (needsDecode(item_raw, opts.plus_as_space)) {
                        const v_pool_off = _.pool_len;
                        const dv = percentDecode(item_raw, opts.plus_as_space, _.pool[v_pool_off..])?;
                        _.pool_len += dv.len;
                        dec_val = dv;
                        val_in_pool = true;
                        val_off = v_pool_off as u16;
                        val_len = dv.len as u16;
                    }

                    _.entries[_.count] = QsEntry{
                        raw_key: raw_key,
                        decoded_key: dec_key,
                        decoded_value: dec_val,
                        is_null: false,
                        segments: item_segs,
                        depth: item_depth,
                        key_off: key_off,
                        key_len: key_len,
                        key_in_pool: key_in_pool,
                        val_off: val_off,
                        val_len: val_len,
                        val_in_pool: val_in_pool,
                    };
                    _.count += 1;
                    item_idx += 1;

                    if (vpos < raw_val.len && raw_val[vpos] == ',') {
                        vpos += 1;
                    } else {
                        break;
                    }
                }
            } else {
                if (_.count >= max_params) return QsError.TooManyParams;

                const is_null_val = !has_eq && opts.strict_null_handling;
                let dec_val: []const u8 = raw_val;
                let val_in_pool = false;
                let val_off: u16 = 0;
                let val_len: u16 = 0;
                if (has_eq && needsDecode(raw_val, opts.plus_as_space)) {
                    const v_pool_off = _.pool_len;
                    const dv = percentDecode(raw_val, opts.plus_as_space, _.pool[v_pool_off..])?;
                    _.pool_len += dv.len;
                    dec_val = dv;
                    val_in_pool = true;
                    val_off = v_pool_off as u16;
                    val_len = dv.len as u16;
                }

                _.entries[_.count] = QsEntry{
                    raw_key: raw_key,
                    decoded_key: dec_key,
                    decoded_value: dec_val,
                    is_null: is_null_val,
                    segments: segs,
                    depth: depth,
                    key_off: key_off,
                    key_len: key_len,
                    key_in_pool: key_in_pool,
                    val_off: val_off,
                    val_len: val_len,
                    val_in_pool: val_in_pool,
                };
                _.count += 1;
            }
        }
    }

    fn resolveKey(*_, idx: usize): []const u8 {
        const e = _.entries[idx];
        if (e.key_in_pool) {
            const off = e.key_off as usize;
            const n = e.key_len as usize;
            return _.pool[off .. off + n];
        }
        return e.decoded_key;
    }

    fn resolveVal(*_, idx: usize): []const u8 {
        const e = _.entries[idx];
        if (e.val_in_pool) {
            const off = e.val_off as usize;
            const n = e.val_len as usize;
            return _.pool[off .. off + n];
        }
        return e.decoded_value;
    }

    fn resolveSegName(*_, entry_idx: usize, seg_idx: usize): []const u8 {
        const seg = _.entries[entry_idx].segments[seg_idx];
        if (seg.name_in_pool) {
            const off = seg.name_off as usize;
            const n = seg.name_len as usize;
            return _.pool[off .. off + n];
        }
        return seg.name;
    }

    /// Returns the total number of parsed entries.
    pub fn len(*_): usize {
        return _.count;
    }

    /// Returns the entry at index `idx`, or `null` if out of bounds.
    pub fn entryAt(*_, idx: usize): QsEntry? {
        if (idx >= _.count) return null;
        return _.entries[idx];
    }

    /// Returns the decoded key of entry `idx`, safe across value moves.
    pub fn entryKey(*_, idx: usize): ([]const u8)? {
        if (idx >= _.count) return null;
        return _.resolveKey(idx);
    }

    /// Returns the decoded value of entry `idx`, or `null` if out of bounds or strict-null.
    pub fn entryValue(*_, idx: usize): ([]const u8)? {
        if (idx >= _.count) return null;
        if (_.entries[idx].is_null) return null;
        return _.resolveVal(idx);
    }

    /// Returns true if `flat_key` was parsed as a strict-null parameter (no `=` sign).
    pub fn isNull(*_, flat_key: []const u8): bool {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (std.mem.eql(u8, _.resolveKey(i), flat_key)) {
                return _.entries[i].is_null;
            }
        }
        return false;
    }

    /// Looks up a flat key (e.g. `foo` in `foo=bar`). Returns `null` if absent or strict-null.
    pub fn get(*_, flat_key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (std.mem.eql(u8, _.resolveKey(i), flat_key)) {
                if (_.entries[i].is_null) return null;
                return _.resolveVal(i);
            }
        }
        return null;
    }

    /// Looks up a 2-level nested key (e.g. `user[name]` or `user.name`).
    pub fn getNested2(*_, k1: []const u8, k2: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].depth == 2) {
                if (std.mem.eql(u8, _.resolveSegName(i, 0), k1)
                    && std.mem.eql(u8, _.resolveSegName(i, 1), k2))
                {
                    if (_.entries[i].is_null) return null;
                    return _.resolveVal(i);
                }
            }
        }
        return null;
    }

    /// Looks up a 3-level nested key (e.g. `a[b][c]` or `a.b.c`).
    pub fn getNested3(*_, k1: []const u8, k2: []const u8, k3: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            if (_.entries[i].depth == 3) {
                if (std.mem.eql(u8, _.resolveSegName(i, 0), k1)
                    && std.mem.eql(u8, _.resolveSegName(i, 1), k2)
                    && std.mem.eql(u8, _.resolveSegName(i, 2), k3))
                {
                    if (_.entries[i].is_null) return null;
                    return _.resolveVal(i);
                }
            }
        }
        return null;
    }

    /// Looks up the `idx`-th element of an array key, supporting:
    /// - explicit indexed brackets (`arr[0]=x&arr[1]=y`)
    /// - empty brackets (`arr[]=x&arr[]=y`) and mixed (`arr[0]=x&arr[]=y`)
    /// - comma-split arrays (`tags=a,b` when `comma_arrays: true`)
    /// - repeated flat keys (`arr=x&arr=y`)
    pub fn getArrayItem(*_, key: []const u8, idx: usize): ([]const u8)? {
        let next_auto_idx: usize = 0;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const e = _.entries[i];
            if (e.depth == 2 && std.mem.eql(u8, _.resolveSegName(i, 0), key)) {
                const seg1 = e.segments[1];
                if (seg1.is_index) {
                    if (seg1.index == idx) {
                        if (e.is_null) return null;
                        return _.resolveVal(i);
                    }
                    if (seg1.index >= next_auto_idx) {
                        next_auto_idx = seg1.index + 1;
                    }
                } else if (seg1.is_empty_bracket) {
                    if (next_auto_idx == idx) {
                        if (e.is_null) return null;
                        return _.resolveVal(i);
                    }
                    next_auto_idx += 1;
                }
            } else if (e.depth == 1 && std.mem.eql(u8, _.resolveSegName(i, 0), key)) {
                if (next_auto_idx == idx) {
                    if (e.is_null) return null;
                    return _.resolveVal(i);
                }
                next_auto_idx += 1;
            }
        }
        return null;
    }

    /// Counts all array items belonging to `key` (indexed brackets, empty brackets,
    /// comma-split items, or repeated flat keys).
    pub fn countArrayItems(*_, key: []const u8): usize {
        let total: usize = 0;
        let i: usize = 0;
        while (i < _.count) : (i += 1) {
            const e = _.entries[i];
            if (e.depth == 2 && std.mem.eql(u8, _.resolveSegName(i, 0), key)) {
                const seg1 = e.segments[1];
                if (seg1.is_index || seg1.is_empty_bracket) {
                    total += 1;
                }
            } else if (e.depth == 1 && std.mem.eql(u8, _.resolveSegName(i, 0), key)) {
                total += 1;
            }
        }
        return total;
    }
}

fn hasByte(s: []const u8, b: u8): bool {
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        if (s[i] == b) return true;
    }
    return false;
}

fn parseKeySegments(
    dec_key: []const u8,
    key_off: u16,
    key_in_pool: bool,
    allow_dots: bool,
    max_depth: usize,
    out_segs: *[8]QsPathSegment,
): usize ! QsError {
    if (dec_key.len == 0) return QsError.InvalidSyntax;
    if (max_depth == 0) return QsError.DepthExceeded;

    let depth: usize = 0;
    let pos: usize = 0;

    // Parse root segment up to '[' or (allow_dots and '.')
    while (pos < dec_key.len) : (pos += 1) {
        const c = dec_key[pos];
        if (c == '[' || (allow_dots && c == '.')) break;
        if (c == ']') return QsError.InvalidSyntax;
    }
    if (pos == 0) return QsError.InvalidSyntax;

    out_segs.*[depth] = makeSegment(dec_key[0..pos], key_off, 0, key_in_pool, false);
    depth += 1;

    // Parse subsequent bracket or dot segments
    while (pos < dec_key.len) {
        const c = dec_key[pos];
        if (c == '[') {
            pos += 1;
            const seg_start = pos;
            let found_close = false;
            while (pos < dec_key.len) : (pos += 1) {
                if (dec_key[pos] == ']') {
                    found_close = true;
                    break;
                }
                if (dec_key[pos] == '[') return QsError.InvalidSyntax;
            }
            if (!found_close) return QsError.InvalidSyntax;

            const inner = dec_key[seg_start..pos];
            pos += 1; // skip ']'

            if (depth >= max_depth) return QsError.DepthExceeded;
            if (inner.len == 0) {
                out_segs.*[depth] = makeSegment("", key_off, seg_start, false, true);
            } else {
                out_segs.*[depth] = makeSegment(inner, key_off, seg_start, key_in_pool, false);
            }
            depth += 1;
        } else if (allow_dots && c == '.') {
            pos += 1;
            const seg_start = pos;
            while (pos < dec_key.len) : (pos += 1) {
                const nc = dec_key[pos];
                if (nc == '.' || nc == '[') break;
                if (nc == ']') return QsError.InvalidSyntax;
            }
            if (pos == seg_start) return QsError.InvalidSyntax;

            if (depth >= max_depth) return QsError.DepthExceeded;
            out_segs.*[depth] = makeSegment(dec_key[seg_start..pos], key_off, seg_start, key_in_pool, false);
            depth += 1;
        } else {
            return QsError.InvalidSyntax;
        }
    }

    return depth;
}

/// Zero-allocation query string serializer writing directly into a caller-supplied buffer.
pub struct QsBuilder {
    buf: []u8,
    pos: usize = 0,
    count: usize = 0,
    add_query_prefix: bool = false,
    space_as_plus: bool = false,

    pub static fn init(buf: []u8): QsBuilder {
        return QsBuilder{
            buf: buf,
            pos: 0,
            count: 0,
            add_query_prefix: false,
            space_as_plus: false,
        };
    }

    pub static fn initWithOptions(
        buf: []u8,
        add_query_prefix: bool,
        space_as_plus: bool,
    ): QsBuilder {
        return QsBuilder{
            buf: buf,
            pos: 0,
            count: 0,
            add_query_prefix: add_query_prefix,
            space_as_plus: space_as_plus,
        };
    }

    fn writeRawByte(*_, b: u8): void ! QsError {
        if (_.pos >= _.buf.len) return QsError.BufferTooSmall;
        _.buf[_.pos] = b;
        _.pos += 1;
    }

    fn writeRawSlice(*_, s: []const u8): void ! QsError {
        if (_.pos + s.len > _.buf.len) return QsError.BufferTooSmall;
        @memcpy(_.buf[_.pos .. _.pos + s.len], s);
        _.pos += s.len;
    }

    fn writeEncoded(*_, s: []const u8): void ! QsError {
        const enc = percentEncode(s, _.space_as_plus, _.buf[_.pos..])?;
        _.pos += enc.len;
    }

    fn writePrefixOrSep(*_): void ! QsError {
        if (_.count == 0) {
            if (_.add_query_prefix) {
                _.writeRawByte('?')?;
            }
        } else {
            _.writeRawByte('&')?;
        }
    }

    /// Appends a flat `key=val` pair with RFC 3986 percent-encoding.
    pub fn add(*_, key: []const u8, val: []const u8): void ! QsError {
        const saved_pos = _.pos;
        const saved_count = _.count;
        errdefer {
            _.pos = saved_pos;
            _.count = saved_count;
        }
        _.writePrefixOrSep()?;
        _.writeEncoded(key)?;
        _.writeRawByte('=')?;
        _.writeEncoded(val)?;
        _.count += 1;
    }

    /// Appends a 2-level nested pair `k1[k2]=val`.
    pub fn addNested2(*_, k1: []const u8, k2: []const u8, val: []const u8): void ! QsError {
        const saved_pos = _.pos;
        const saved_count = _.count;
        errdefer {
            _.pos = saved_pos;
            _.count = saved_count;
        }
        _.writePrefixOrSep()?;
        _.writeEncoded(k1)?;
        _.writeRawByte('[')?;
        _.writeEncoded(k2)?;
        _.writeRawByte(']')?;
        _.writeRawByte('=')?;
        _.writeEncoded(val)?;
        _.count += 1;
    }

    /// Appends a 3-level nested pair `k1[k2][k3]=val`.
    pub fn addNested3(*_, k1: []const u8, k2: []const u8, k3: []const u8, val: []const u8): void ! QsError {
        const saved_pos = _.pos;
        const saved_count = _.count;
        errdefer {
            _.pos = saved_pos;
            _.count = saved_count;
        }
        _.writePrefixOrSep()?;
        _.writeEncoded(k1)?;
        _.writeRawByte('[')?;
        _.writeEncoded(k2)?;
        _.writeRawByte(']')?;
        _.writeRawByte('[')?;
        _.writeEncoded(k3)?;
        _.writeRawByte(']')?;
        _.writeRawByte('=')?;
        _.writeEncoded(val)?;
        _.count += 1;
    }

    /// Appends an indexed array item `key[idx]=val`.
    pub fn addArrayItem(*_, key: []const u8, idx: usize, val: []const u8): void ! QsError {
        const saved_pos = _.pos;
        const saved_count = _.count;
        errdefer {
            _.pos = saved_pos;
            _.count = saved_count;
        }
        _.writePrefixOrSep()?;
        _.writeEncoded(key)?;
        _.writeRawByte('[')?;
        let num_buf: [24]u8 = undefined;
        const idx_str = std.fmt.bufPrint(num_buf[0..], "{d}", .{idx}) catch return QsError.BufferTooSmall;
        _.writeRawSlice(idx_str)?;
        _.writeRawByte(']')?;
        _.writeRawByte('=')?;
        _.writeEncoded(val)?;
        _.count += 1;
    }

    /// Appends an empty-bracket array item `key[]=val`.
    pub fn addBracketArrayItem(*_, key: []const u8, val: []const u8): void ! QsError {
        const saved_pos = _.pos;
        const saved_count = _.count;
        errdefer {
            _.pos = saved_pos;
            _.count = saved_count;
        }
        _.writePrefixOrSep()?;
        _.writeEncoded(key)?;
        _.writeRawSlice("[]=")?;
        _.writeEncoded(val)?;
        _.count += 1;
    }

    /// Appends a strict-null parameter `key` (without `=`).
    pub fn addNull(*_, key: []const u8): void ! QsError {
        const saved_pos = _.pos;
        const saved_count = _.count;
        errdefer {
            _.pos = saved_pos;
            _.count = saved_count;
        }
        _.writePrefixOrSep()?;
        _.writeEncoded(key)?;
        _.count += 1;
    }

    /// Returns the serialized query string slice written so far.
    pub fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    /// Resets the builder so the underlying buffer can be reused.
    pub fn reset(*_) {
        _.pos = 0;
        _.count = 0;
    }
}
