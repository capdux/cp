// @cap/unflatten — Zero-allocation nested JSON flatten & unflatten engine for CP
// Clean-room MIT implementation with parity to npm `flat` / `unflatten`.

const UF_MAX_ENTRIES: usize = 48;
const UF_KEY_CAP: usize = 96;
const UF_VAL_CAP: usize = 128;
const UF_MAX_NODES: usize = 96;
const UF_SEG_CAP: usize = 48;

const UF_NODE_OBJ: u8 = 1;
const UF_NODE_LEAF: u8 = 2;

/// Configuration options for `flattenJson`, `flattenToJson`, and `unflattenJson`.
pub struct FlatOptions {
    pub delimiter: u8,
    pub max_depth: usize,
    pub safe: bool,

    /// Default options: `delimiter = '.'`, `max_depth = 0` (unlimited), `safe = false`.
    pub static fn defaults(): FlatOptions {
        return FlatOptions {
            delimiter: 46, // '.'
            max_depth: 0,
            safe: false,
        };
    }
}

/// Single flattened key-value entry stored on the stack.
pub struct FlatEntry {
    pub key: [96]u8,
    pub key_len: usize,
    pub val: [128]u8,
    pub val_len: usize,
}

fn ufStrEq(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn ufCopyBuf(dst: []u8, src: string): usize {
    let n = src.len;
    if (n > dst.len) {
        n = dst.len;
    }
    let i: usize = 0;
    while (i < n) {
        dst[i] = src[i];
        i = i + 1;
    }
    return n;
}

fn ufIsWs(ch: u8): bool {
    return ch == 32 || ch == 9 || ch == 10 || ch == 13;
}

fn ufIsDigit(ch: u8): bool {
    return ch >= 48 && ch <= 57;
}

fn ufIsHex(ch: u8): bool {
    return (ch >= 48 && ch <= 57) || (ch >= 65 && ch <= 70) || (ch >= 97 && ch <= 102);
}

fn ufSkipWs(s: string, pos: usize): usize {
    let i = pos;
    while (i < s.len && ufIsWs(s[i])) {
        i = i + 1;
    }
    return i;
}

fn ufFormatUsize(val_in: usize, out: []u8): usize {
    let tmp: [24]u8 = undefined;
    let v = val_in;
    let n: usize = 0;
    if (v == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (v > 0 && n < 24) {
            tmp[n] = ((v % 10) as u8) + 48;
            v = v / 10;
            n = n + 1;
        }
    }
    let pos: usize = 0;
    let k = n;
    while (k > 0 && pos < out.len) {
        k = k - 1;
        out[pos] = tmp[k];
        pos = pos + 1;
    }
    return pos;
}

fn ufEmptyEntry(): FlatEntry {
    let k: [96]u8 = undefined;
    let v: [128]u8 = undefined;
    k[0] = 0;
    v[0] = 0;
    return FlatEntry {
        key: k,
        key_len: 0,
        val: v,
        val_len: 0,
    };
}

/// Zero-allocation table holding up to 48 flattened JSON key-value pairs.
pub struct FlatTable {
    pub entries: [48]FlatEntry,
    pub len: usize,

    /// Create an empty `FlatTable`.
    pub static fn new(): FlatTable {
        let entries: [48]FlatEntry = undefined;
        return FlatTable {
            entries: entries,
            len: 0,
        };
    }

    /// Insert or update a flattened entry (`key` -> `raw_json_val`).
    /// Returns `false` if key/value exceeds fixed capacity or table is full.
    pub fn set(*_, key: string, raw_json_val: string): bool {
        if (key.len == 0 || key.len > UF_KEY_CAP || raw_json_val.len > UF_VAL_CAP) {
            return false;
        }
        let i: usize = 0;
        while (i < _.len) {
            const e = _.entries[i];
            if (ufStrEq(e.key[0..e.key_len], key)) {
                _.entries[i].val_len = ufCopyBuf(_.entries[i].val[0..UF_VAL_CAP], raw_json_val);
                return true;
            }
            i = i + 1;
        }
        if (_.len >= UF_MAX_ENTRIES) {
            return false;
        }
        let entry = ufEmptyEntry();
        entry.key_len = ufCopyBuf(entry.key[0..UF_KEY_CAP], key);
        entry.val_len = ufCopyBuf(entry.val[0..UF_VAL_CAP], raw_json_val);
        _.entries[_.len] = entry;
        _.len = _.len + 1;
        return true;
    }

    /// Look up `key` and copy its raw JSON value into `out`.
    /// Returns bytes written on success, or `-1` if not found or `out` is too small.
    pub fn get(key: string, out: []u8): i64 {
        let i: usize = 0;
        while (i < _.len) {
            const e = _.entries[i];
            if (ufStrEq(e.key[0..e.key_len], key)) {
                if (out.len < e.val_len) {
                    return -1;
                }
                return ufCopyBuf(out, e.val[0..e.val_len]) as i64;
            }
            i = i + 1;
        }
        return -1;
    }

    /// Return the number of flattened entries currently stored.
    pub fn count(): usize {
        return _.len;
    }
}

/// Validate and scan a JSON string literal starting at `pos` (which must be `'"'`).
/// Returns the byte index immediately after the closing `'"'`, or `-1` on error.
fn ufSkipString(s: string, pos: usize): i64 {
    if (pos >= s.len || s[pos] != 34) {
        return -1;
    }
    let i = pos + 1;
    while (i < s.len) {
        const ch = s[i];
        if (ch == 34) {
            return (i + 1) as i64;
        }
        if (ch < 32) {
            return -1;
        }
        if (ch == 92) { // '\\'
            i = i + 1;
            if (i >= s.len) {
                return -1;
            }
            const esc = s[i];
            if (esc == 34 || esc == 92 || esc == 47 || esc == 98 || esc == 102
                || esc == 110 || esc == 114 || esc == 116) {
                i = i + 1;
                continue;
            }
            if (esc == 117) { // 'u'
                if (i + 4 >= s.len) {
                    return -1;
                }
                if (!ufIsHex(s[i + 1]) || !ufIsHex(s[i + 2])
                    || !ufIsHex(s[i + 3]) || !ufIsHex(s[i + 4])) {
                    return -1;
                }
                i = i + 5;
                continue;
            }
            return -1;
        }
        i = i + 1;
    }
    return -1;
}

struct UfStrRes {
    ok: bool,
    next_pos: usize,
    out_len: usize,
}

/// Decode a JSON string literal starting at `pos` into `out`.
fn ufDecodeString(s: string, pos: usize, out: []u8): UfStrRes {
    if (pos >= s.len || s[pos] != 34) {
        return UfStrRes { ok: false, next_pos: pos, out_len: 0 };
    }
    let i = pos + 1;
    let w: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch == 34) {
            return UfStrRes { ok: true, next_pos: i + 1, out_len: w };
        }
        if (ch < 32) {
            return UfStrRes { ok: false, next_pos: i, out_len: 0 };
        }
        if (ch == 92) {
            i = i + 1;
            if (i >= s.len) {
                return UfStrRes { ok: false, next_pos: i, out_len: 0 };
            }
            const esc = s[i];
            let mapped: u8 = 0;
            if (esc == 34) {
                mapped = 34;
            } else if (esc == 92) {
                mapped = 92;
            } else if (esc == 47) {
                mapped = 47;
            } else if (esc == 98) {
                mapped = 8;
            } else if (esc == 102) {
                mapped = 12;
            } else if (esc == 110) {
                mapped = 10;
            } else if (esc == 114) {
                mapped = 13;
            } else if (esc == 116) {
                mapped = 9;
            } else {
                return UfStrRes { ok: false, next_pos: i, out_len: 0 };
            }
            if (w >= out.len) {
                return UfStrRes { ok: false, next_pos: i, out_len: 0 };
            }
            out[w] = mapped;
            w = w + 1;
            i = i + 1;
            continue;
        }
        if (w >= out.len) {
            return UfStrRes { ok: false, next_pos: i, out_len: 0 };
        }
        out[w] = ch;
        w = w + 1;
        i = i + 1;
    }
    return UfStrRes { ok: false, next_pos: i, out_len: 0 };
}

fn ufSkipNumber(s: string, pos: usize): i64 {
    let i = pos;
    if (i < s.len && s[i] == 45) {
        i = i + 1;
    }
    if (i >= s.len) {
        return -1;
    }
    if (s[i] == 48) {
        i = i + 1;
    } else if (s[i] >= 49 && s[i] <= 57) {
        i = i + 1;
        while (i < s.len && ufIsDigit(s[i])) {
            i = i + 1;
        }
    } else {
        return -1;
    }
    if (i < s.len && s[i] == 46) {
        i = i + 1;
        if (i >= s.len || !ufIsDigit(s[i])) {
            return -1;
        }
        while (i < s.len && ufIsDigit(s[i])) {
            i = i + 1;
        }
    }
    if (i < s.len && (s[i] == 101 || s[i] == 69)) {
        i = i + 1;
        if (i < s.len && (s[i] == 43 || s[i] == 45)) {
            i = i + 1;
        }
        if (i >= s.len || !ufIsDigit(s[i])) {
            return -1;
        }
        while (i < s.len && ufIsDigit(s[i])) {
            i = i + 1;
        }
    }
    return i as i64;
}

/// Strictly validate and skip any JSON value starting at `pos`.
fn ufSkipValue(s: string, pos: usize, depth: usize): i64 {
    if (depth > 32) {
        return -1;
    }
    const p = ufSkipWs(s, pos);
    if (p >= s.len) {
        return -1;
    }
    const ch = s[p];
    if (ch == 34) {
        return ufSkipString(s, p);
    }
    if (ch == 45 || ufIsDigit(ch)) {
        return ufSkipNumber(s, p);
    }
    if (ch == 116) { // true
        if (p + 4 <= s.len && ufStrEq(s[p..(p + 4)], "true")) {
            return (p + 4) as i64;
        }
        return -1;
    }
    if (ch == 102) { // false
        if (p + 5 <= s.len && ufStrEq(s[p..(p + 5)], "false")) {
            return (p + 5) as i64;
        }
        return -1;
    }
    if (ch == 110) { // null
        if (p + 4 <= s.len && ufStrEq(s[p..(p + 4)], "null")) {
            return (p + 4) as i64;
        }
        return -1;
    }
    if (ch == 123) { // '{'
        let cur = ufSkipWs(s, p + 1);
        if (cur < s.len && s[cur] == 125) {
            return (cur + 1) as i64;
        }
        while (true) {
            cur = ufSkipWs(s, cur);
            const kEnd = ufSkipString(s, cur);
            if (kEnd < 0) {
                return -1;
            }
            cur = ufSkipWs(s, kEnd as usize);
            if (cur >= s.len || s[cur] != 58) {
                return -1;
            }
            const vEnd = ufSkipValue(s, cur + 1, depth + 1);
            if (vEnd < 0) {
                return -1;
            }
            cur = ufSkipWs(s, vEnd as usize);
            if (cur >= s.len) {
                return -1;
            }
            if (s[cur] == 44) {
                cur = cur + 1;
                continue;
            }
            if (s[cur] == 125) {
                return (cur + 1) as i64;
            }
            return -1;
        }
    }
    if (ch == 91) { // '['
        let cur = ufSkipWs(s, p + 1);
        if (cur < s.len && s[cur] == 93) {
            return (cur + 1) as i64;
        }
        while (true) {
            const vEnd = ufSkipValue(s, cur, depth + 1);
            if (vEnd < 0) {
                return -1;
            }
            cur = ufSkipWs(s, vEnd as usize);
            if (cur >= s.len) {
                return -1;
            }
            if (s[cur] == 44) {
                cur = cur + 1;
                continue;
            }
            if (s[cur] == 93) {
                return (cur + 1) as i64;
            }
            return -1;
        }
    }
    return -1;
}

fn ufJoinKey(prefix: string, delim: u8, seg: string, out: []u8): i64 {
    if (prefix.len == 0) {
        if (seg.len > out.len) {
            return -1;
        }
        return ufCopyBuf(out, seg) as i64;
    }
    const need = prefix.len + 1 + seg.len;
    if (need > out.len) {
        return -1;
    }
    let pos = ufCopyBuf(out, prefix);
    out[pos] = delim;
    pos = pos + 1;
    let i: usize = 0;
    while (i < seg.len) {
        out[pos] = seg[i];
        pos = pos + 1;
        i = i + 1;
    }
    return pos as i64;
}

fn ufFlattenValue(
    s: string,
    pos: usize,
    prefix: string,
    depth: usize,
    opts: FlatOptions,
    out_table: *FlatTable,
): i64 {
    if (depth > 32) {
        return -1;
    }
    const delim: u8 = if (opts.delimiter == 0) 46 else opts.delimiter;
    const p = ufSkipWs(s, pos);
    if (p >= s.len) {
        return -1;
    }
    const ch = s[p];

    // Nested object `{...}`
    if (ch == 123) {
        if (opts.max_depth > 0 && depth >= opts.max_depth && prefix.len > 0) {
            const end = ufSkipValue(s, p, depth);
            if (end < 0) {
                return -1;
            }
            if (!out_table.set(prefix, s[p..(end as usize)])) {
                return -1;
            }
            return end;
        }
        let cur = ufSkipWs(s, p + 1);
        if (cur < s.len && s[cur] == 125) {
            if (prefix.len > 0) {
                if (!out_table.set(prefix, "{}")) {
                    return -1;
                }
            }
            return (cur + 1) as i64;
        }
        while (true) {
            cur = ufSkipWs(s, cur);
            let kbuf: [96]u8 = undefined;
            const kres = ufDecodeString(s, cur, kbuf[0..UF_KEY_CAP]);
            if (!kres.ok) {
                return -1;
            }
            cur = ufSkipWs(s, kres.next_pos);
            if (cur >= s.len || s[cur] != 58) {
                return -1;
            }
            let next_prefix: [96]u8 = undefined;
            const nplen = ufJoinKey(prefix, delim, kbuf[0..kres.out_len], next_prefix[0..UF_KEY_CAP]);
            if (nplen < 0) {
                return -1;
            }
            const vend = ufFlattenValue(
                s,
                cur + 1,
                next_prefix[0..(nplen as usize)],
                depth + 1,
                opts,
                out_table,
            );
            if (vend < 0) {
                return -1;
            }
            cur = ufSkipWs(s, vend as usize);
            if (cur >= s.len) {
                return -1;
            }
            if (s[cur] == 44) {
                cur = cur + 1;
                continue;
            }
            if (s[cur] == 125) {
                return (cur + 1) as i64;
            }
            return -1;
        }
    }

    // Nested array `[...]`
    if (ch == 91) {
        const stop_here = (opts.safe && prefix.len > 0)
            || (opts.max_depth > 0 && depth >= opts.max_depth && prefix.len > 0);
        if (stop_here) {
            const end = ufSkipValue(s, p, depth);
            if (end < 0) {
                return -1;
            }
            if (!out_table.set(prefix, s[p..(end as usize)])) {
                return -1;
            }
            return end;
        }
        let cur = ufSkipWs(s, p + 1);
        if (cur < s.len && s[cur] == 93) {
            if (prefix.len > 0) {
                if (!out_table.set(prefix, "[]")) {
                    return -1;
                }
            }
            return (cur + 1) as i64;
        }
        let idx: usize = 0;
        while (true) {
            let ibuf: [24]u8 = undefined;
            const ilen = ufFormatUsize(idx, ibuf[0..24]);
            let next_prefix: [96]u8 = undefined;
            const nplen = ufJoinKey(prefix, delim, ibuf[0..ilen], next_prefix[0..UF_KEY_CAP]);
            if (nplen < 0) {
                return -1;
            }
            const vend = ufFlattenValue(
                s,
                cur,
                next_prefix[0..(nplen as usize)],
                depth + 1,
                opts,
                out_table,
            );
            if (vend < 0) {
                return -1;
            }
            cur = ufSkipWs(s, vend as usize);
            if (cur >= s.len) {
                return -1;
            }
            if (s[cur] == 44) {
                cur = cur + 1;
                idx = idx + 1;
                continue;
            }
            if (s[cur] == 93) {
                return (cur + 1) as i64;
            }
            return -1;
        }
    }

    // Scalar leaf value
    const end = ufSkipValue(s, p, depth);
    if (end < 0) {
        return -1;
    }
    if (prefix.len > 0) {
        if (!out_table.set(prefix, s[p..(end as usize)])) {
            return -1;
        }
    }
    return end;
}

/// Flatten a nested JSON object or array into `out_table`.
/// Returns the number of entries written, or `-1` on invalid JSON or capacity overflow.
pub fn flattenJson(json_in: string, opts: FlatOptions, out_table: *FlatTable): i64 {
    out_table.len = 0;
    const p = ufSkipWs(json_in, 0);
    if (p >= json_in.len || (json_in[p] != 123 && json_in[p] != 91)) {
        return -1;
    }
    const end = ufFlattenValue(json_in, p, "", 0, opts, out_table);
    if (end < 0) {
        out_table.len = 0;
        return -1;
    }
    const tail = ufSkipWs(json_in, end as usize);
    if (tail != json_in.len) {
        out_table.len = 0;
        return -1;
    }
    return out_table.len as i64;
}

fn ufEmitByte(out: []u8, pos: usize, b: u8): i64 {
    if (pos >= out.len) {
        return -1;
    }
    out[pos] = b;
    return (pos + 1) as i64;
}

fn ufEmitBytes(out: []u8, pos: usize, s: string): i64 {
    if (pos + s.len > out.len) {
        return -1;
    }
    let w = pos;
    let i: usize = 0;
    while (i < s.len) {
        out[w] = s[i];
        w = w + 1;
        i = i + 1;
    }
    return w as i64;
}

fn ufEmitJsonString(out: []u8, pos: usize, s: string): i64 {
    let w = ufEmitByte(out, pos, 34);
    if (w < 0) {
        return -1;
    }
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch == 34) {
            w = ufEmitBytes(out, w as usize, "\\\"");
        } else if (ch == 92) {
            w = ufEmitBytes(out, w as usize, "\\\\");
        } else if (ch == 10) {
            w = ufEmitBytes(out, w as usize, "\\n");
        } else if (ch == 13) {
            w = ufEmitBytes(out, w as usize, "\\r");
        } else if (ch == 9) {
            w = ufEmitBytes(out, w as usize, "\\t");
        } else {
            w = ufEmitByte(out, w as usize, ch);
        }
        if (w < 0) {
            return -1;
        }
        i = i + 1;
    }
    return ufEmitByte(out, w as usize, 34);
}

/// Flatten `json_in` and serialize the result directly as a flat JSON object string into `out`.
/// Returns bytes written on success, or `-1` on invalid JSON or buffer overflow.
pub fn flattenToJson(json_in: string, opts: FlatOptions, out: []u8): i64 {
    let table = FlatTable.new();
    const count = flattenJson(json_in, opts, &table);
    if (count < 0) {
        return -1;
    }
    let w = ufEmitByte(out, 0, 123); // '{'
    if (w < 0) {
        return -1;
    }
    let i: usize = 0;
    while (i < table.len) {
        if (i > 0) {
            w = ufEmitByte(out, w as usize, 44); // ','
            if (w < 0) {
                return -1;
            }
        }
        const e = table.entries[i];
        w = ufEmitJsonString(out, w as usize, e.key[0..e.key_len]);
        if (w < 0) {
            return -1;
        }
        w = ufEmitByte(out, w as usize, 58); // ':'
        if (w < 0) {
            return -1;
        }
        w = ufEmitBytes(out, w as usize, e.val[0..e.val_len]);
        if (w < 0) {
            return -1;
        }
        i = i + 1;
    }
    return ufEmitByte(out, w as usize, 125); // '}'
}

struct UfNode {
    kind: u8,
    name: [48]u8,
    name_len: usize,
    val: [128]u8,
    val_len: usize,
    first_child: i32,
    last_child: i32,
    next_sibling: i32,
}

struct UfPool {
    nodes: [96]UfNode,
    count: usize,
}

fn ufAllocNode(pool: *UfPool, kind: u8, name: string): i32 {
    if (pool.count >= UF_MAX_NODES || name.len > UF_SEG_CAP) {
        return -1;
    }
    let nbuf: [48]u8 = undefined;
    let vbuf: [128]u8 = undefined;
    nbuf[0] = 0;
    vbuf[0] = 0;
    const nlen = ufCopyBuf(nbuf[0..UF_SEG_CAP], name);
    const idx = pool.count as i32;
    pool.nodes[pool.count] = UfNode {
        kind: kind,
        name: nbuf,
        name_len: nlen,
        val: vbuf,
        val_len: 0,
        first_child: -1,
        last_child: -1,
        next_sibling: -1,
    };
    pool.count = pool.count + 1;
    return idx;
}

fn ufFindChild(pool: *UfPool, parent_idx: i32, seg: string): i32 {
    let cur = pool.nodes[parent_idx as usize].first_child;
    while (cur >= 0) {
        const node = pool.nodes[cur as usize];
        if (ufStrEq(node.name[0..node.name_len], seg)) {
            return cur;
        }
        cur = node.next_sibling;
    }
    return -1;
}

fn ufAppendChild(pool: *UfPool, parent_idx: i32, child_idx: i32) {
    const p = parent_idx as usize;
    if (pool.nodes[p].first_child < 0) {
        pool.nodes[p].first_child = child_idx;
        pool.nodes[p].last_child = child_idx;
    } else {
        const last = pool.nodes[p].last_child as usize;
        pool.nodes[last].next_sibling = child_idx;
        pool.nodes[p].last_child = child_idx;
    }
}

fn ufInsertPath(pool: *UfPool, path: string, delim: u8, raw_val: string): bool {
    if (path.len == 0 || raw_val.len > UF_VAL_CAP) {
        return false;
    }
    let parent: i32 = 0;
    let start: usize = 0;
    let i: usize = 0;
    while (i <= path.len) {
        if (i == path.len || path[i] == delim) {
            const seg = path[start..i];
            const is_last = (i == path.len);
            if (is_last) {
                const existing = ufFindChild(pool, parent, seg);
                if (existing >= 0) {
                    const ex = existing as usize;
                    pool.nodes[ex].kind = UF_NODE_LEAF;
                    pool.nodes[ex].val_len = ufCopyBuf(pool.nodes[ex].val[0..UF_VAL_CAP], raw_val);
                    pool.nodes[ex].first_child = -1;
                    pool.nodes[ex].last_child = -1;
                } else {
                    const leaf = ufAllocNode(pool, UF_NODE_LEAF, seg);
                    if (leaf < 0) {
                        return false;
                    }
                    pool.nodes[leaf as usize].val_len = ufCopyBuf(pool.nodes[leaf as usize].val[0..UF_VAL_CAP], raw_val);
                    ufAppendChild(pool, parent, leaf);
                }
                return true;
            } else {
                let child = ufFindChild(pool, parent, seg);
                if (child < 0) {
                    child = ufAllocNode(pool, UF_NODE_OBJ, seg);
                    if (child < 0) {
                        return false;
                    }
                    ufAppendChild(pool, parent, child);
                } else if (pool.nodes[child as usize].kind != UF_NODE_OBJ) {
                    pool.nodes[child as usize].kind = UF_NODE_OBJ;
                    pool.nodes[child as usize].val_len = 0;
                }
                parent = child;
                start = i + 1;
            }
        }
        i = i + 1;
    }
    return true;
}

fn ufIsSequentialArray(pool: *UfPool, node_idx: i32): bool {
    let cur = pool.nodes[node_idx as usize].first_child;
    if (cur < 0) {
        return false;
    }
    let expected: usize = 0;
    let ibuf: [24]u8 = undefined;
    while (cur >= 0) {
        const ilen = ufFormatUsize(expected, ibuf[0..24]);
        const node = pool.nodes[cur as usize];
        if (!ufStrEq(node.name[0..node.name_len], ibuf[0..ilen])) {
            return false;
        }
        expected = expected + 1;
        cur = node.next_sibling;
    }
    return expected > 0;
}

fn ufEmitNode(pool: *UfPool, node_idx: i32, out: []u8, pos: usize, is_root: bool): i64 {
    const node = pool.nodes[node_idx as usize];
    if (node.kind == UF_NODE_LEAF) {
        return ufEmitBytes(out, pos, node.val[0..node.val_len]);
    }

    // Reconstruct sequential integer keys ("0", "1", ...) as JSON arrays (for non-root containers)
    if (!is_root && ufIsSequentialArray(pool, node_idx)) {
        let w = ufEmitByte(out, pos, 91); // '['
        if (w < 0) {
            return -1;
        }
        let cur = node.first_child;
        let first = true;
        while (cur >= 0) {
            if (!first) {
                w = ufEmitByte(out, w as usize, 44); // ','
                if (w < 0) {
                    return -1;
                }
            }
            first = false;
            w = ufEmitNode(pool, cur, out, w as usize, false);
            if (w < 0) {
                return -1;
            }
            cur = pool.nodes[cur as usize].next_sibling;
        }
        return ufEmitByte(out, w as usize, 93); // ']'
    }

    let w = ufEmitByte(out, pos, 123); // '{'
    if (w < 0) {
        return -1;
    }
    let cur = node.first_child;
    let first = true;
    while (cur >= 0) {
        if (!first) {
            w = ufEmitByte(out, w as usize, 44); // ','
            if (w < 0) {
                return -1;
            }
        }
        first = false;
        const child = pool.nodes[cur as usize];
        w = ufEmitJsonString(out, w as usize, child.name[0..child.name_len]);
        if (w < 0) {
            return -1;
        }
        w = ufEmitByte(out, w as usize, 58); // ':'
        if (w < 0) {
            return -1;
        }
        w = ufEmitNode(pool, cur, out, w as usize, false);
        if (w < 0) {
            return -1;
        }
        cur = child.next_sibling;
    }
    return ufEmitByte(out, w as usize, 125); // '}'
}

/// Reconstruct a nested JSON object from a flattened JSON object string into `out`.
/// Merges shared prefixes and restores sequential numeric index keys into JSON arrays.
/// Returns bytes written on success, or `-1` on invalid JSON or buffer overflow.
pub fn unflattenJson(flat_json: string, opts: FlatOptions, out: []u8): i64 {
    const delim: u8 = if (opts.delimiter == 0) 46 else opts.delimiter;
    let p = ufSkipWs(flat_json, 0);
    if (p >= flat_json.len || flat_json[p] != 123) {
        return -1;
    }

    let nodes: [96]UfNode = undefined;
    let pool = UfPool { nodes: nodes, count: 0 };
    const root = ufAllocNode(&pool, UF_NODE_OBJ, "");
    if (root < 0) {
        return -1;
    }

    let cur = ufSkipWs(flat_json, p + 1);
    if (cur < flat_json.len && flat_json[cur] == 125) {
        const tail = ufSkipWs(flat_json, cur + 1);
        if (tail != flat_json.len) {
            return -1;
        }
        return ufEmitBytes(out, 0, "{}");
    }

    while (true) {
        cur = ufSkipWs(flat_json, cur);
        let kbuf: [96]u8 = undefined;
        const kres = ufDecodeString(flat_json, cur, kbuf[0..UF_KEY_CAP]);
        if (!kres.ok) {
            return -1;
        }
        cur = ufSkipWs(flat_json, kres.next_pos);
        if (cur >= flat_json.len || flat_json[cur] != 58) {
            return -1;
        }
        const vStart = ufSkipWs(flat_json, cur + 1);
        const vEnd = ufSkipValue(flat_json, vStart, 0);
        if (vEnd < 0) {
            return -1;
        }
        if (!ufInsertPath(&pool, kbuf[0..kres.out_len], delim, flat_json[vStart..(vEnd as usize)])) {
            return -1;
        }
        cur = ufSkipWs(flat_json, vEnd as usize);
        if (cur >= flat_json.len) {
            return -1;
        }
        if (flat_json[cur] == 44) {
            cur = cur + 1;
            continue;
        }
        if (flat_json[cur] == 125) {
            const tail = ufSkipWs(flat_json, cur + 1);
            if (tail != flat_json.len) {
                return -1;
            }
            break;
        }
        return -1;
    }

    return ufEmitNode(&pool, 0, out, 0, true);
}

export {
    FlatOptions,
    FlatEntry,
    FlatTable,
    flattenJson,
    flattenToJson,
    unflattenJson,
};
