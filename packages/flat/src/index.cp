// @cap/jsonflat — Zero-allocation JSON flattening & dot-prop path lookup engine for CP
// Clean-room MIT implementation inspired by npm `flat` and `dot-prop`.

const MAX_FLAT_ENTRIES: usize = 128;
const MAX_FLAT_POOL: usize = 8192;
const MAX_JSON_DEPTH: usize = 24;

pub struct FlatResult {
    pub pool: [MAX_FLAT_POOL]u8,
    pub pool_len: usize,
    pub key_starts: [MAX_FLAT_ENTRIES]usize,
    pub key_lens: [MAX_FLAT_ENTRIES]usize,
    pub val_starts: [MAX_FLAT_ENTRIES]usize,
    pub val_lens: [MAX_FLAT_ENTRIES]usize,
    pub count: usize,
    pub ok: bool,

    pub static fn empty() -> FlatResult {
        let p: [MAX_FLAT_POOL]u8 = undefined;
        let ks: [MAX_FLAT_ENTRIES]usize = undefined;
        let kl: [MAX_FLAT_ENTRIES]usize = undefined;
        let vs: [MAX_FLAT_ENTRIES]usize = undefined;
        let vl: [MAX_FLAT_ENTRIES]usize = undefined;
        return FlatResult{
            pool: p,
            pool_len: 0,
            key_starts: ks,
            key_lens: kl,
            val_starts: vs,
            val_lens: vl,
            count: 0,
            ok: true,
        };
    }

    pub fn pushEntry(*_, key: string, val: string) -> bool {
        if (_.count >= MAX_FLAT_ENTRIES) {
            _.ok = false;
            return false;
        }
        if (_.pool_len + key.len + val.len > MAX_FLAT_POOL) {
            _.ok = false;
            return false;
        }
        const ks = _.pool_len;
        let i: usize = 0;
        while (i < key.len) {
            _.pool[_.pool_len] = key[i];
            _.pool_len += 1;
            i += 1;
        }
        const vs = _.pool_len;
        i = 0;
        while (i < val.len) {
            _.pool[_.pool_len] = val[i];
            _.pool_len += 1;
            i += 1;
        }
        _.key_starts[_.count] = ks;
        _.key_lens[_.count] = key.len;
        _.val_starts[_.count] = vs;
        _.val_lens[_.count] = val.len;
        _.count += 1;
        return true;
    }

    pub fn keyAt(idx: usize) -> string {
        if (idx >= _.count) {
            return "";
        }
        const st = _.key_starts[idx];
        return _.pool[st..st + _.key_lens[idx]];
    }

    pub fn valAt(idx: usize) -> string {
        if (idx >= _.count) {
            return "";
        }
        const st = _.val_starts[idx];
        return _.pool[st..st + _.val_lens[idx]];
    }

    pub fn get(path: string) -> string? {
        let i: usize = 0;
        while (i < _.count) {
            const k = _.keyAt(i);
            if (strEq(k, path)) {
                return _.valAt(i);
            }
            i += 1;
        }
        return null;
    }

    pub fn has(path: string) -> bool {
        return _.get(path) != null;
    }
}

fn strEq(a: string, b: string) -> bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isWs(c: u8) -> bool {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn skipWs(s: string, pos: usize) -> usize {
    let i = pos;
    while (i < s.len && isWs(s[i])) {
        i += 1;
    }
    return i;
}

fn parseJsonStringEnd(s: string, open_quote: usize) -> usize {
    let i = open_quote + 1;
    while (i < s.len) {
        if (s[i] == '\\') {
            i += 2;
            continue;
        }
        if (s[i] == '"') {
            return i;
        }
        i += 1;
    }
    return s.len;
}

fn formatUsize(v: usize, buf: []u8) -> string {
    let tmp: [24]u8 = undefined;
    let n: usize = 0;
    let x = v;
    if (x == 0) {
        tmp[0] = '0';
        n = 1;
    } else {
        while (x > 0 && n < 24) {
            tmp[n] = ((x % 10) as u8) + '0';
            x = x / 10;
            n += 1;
        }
    }
    let pos: usize = 0;
    let k = n;
    while (k > 0 && pos < buf.len) {
        k -= 1;
        buf[pos] = tmp[k];
        pos += 1;
    }
    return buf[0..pos];
}

fn parseValue(
    s: string,
    start_pos: usize,
    prefix: []u8,
    prefix_len: usize,
    sep: u8,
    depth: usize,
    res: *FlatResult
) -> usize {
    if (depth > MAX_JSON_DEPTH) {
        res.ok = false;
        return s.len;
    }
    let i = skipWs(s, start_pos);
    if (i >= s.len) {
        res.ok = false;
        return i;
    }

    const c = s[i];
    if (c == '{') {
        i = skipWs(s, i + 1);
        if (i < s.len && s[i] == '}') {
            if (prefix_len > 0) {
                * = res.pushEntry(prefix[0..prefix_len], "{}");
            }
            return i + 1;
        }
        while (i < s.len) {
            i = skipWs(s, i);
            if (i >= s.len || s[i] != '"') {
                res.ok = false;
                return s.len;
            }
            const key_end = parseJsonStringEnd(s, i);
            if (key_end >= s.len) {
                res.ok = false;
                return s.len;
            }
            const key = s[i + 1..key_end];
            i = skipWs(s, key_end + 1);
            if (i >= s.len || s[i] != ':') {
                res.ok = false;
                return s.len;
            }
            i += 1;

            let next_len = prefix_len;
            if (next_len > 0 && next_len < prefix.len) {
                prefix[next_len] = sep;
                next_len += 1;
            }
            let k: usize = 0;
            while (k < key.len && next_len < prefix.len) {
                prefix[next_len] = key[k];
                next_len += 1;
                k += 1;
            }

            i = parseValue(s, i, prefix, next_len, sep, depth + 1, res);
            if (!res.ok) {
                return s.len;
            }
            i = skipWs(s, i);
            if (i < s.len && s[i] == ',') {
                i += 1;
                continue;
            }
            if (i < s.len && s[i] == '}') {
                return i + 1;
            }
            res.ok = false;
            return s.len;
        }
        res.ok = false;
        return s.len;
    }

    if (c == '[') {
        i = skipWs(s, i + 1);
        if (i < s.len && s[i] == ']') {
            if (prefix_len > 0) {
                * = res.pushEntry(prefix[0..prefix_len], "[]");
            }
            return i + 1;
        }
        let idx: usize = 0;
        let idx_buf: [24]u8 = undefined;
        while (i < s.len) {
            const idx_str = formatUsize(idx, idx_buf[0..24]);
            let next_len = prefix_len;
            if (next_len > 0 && next_len < prefix.len) {
                prefix[next_len] = sep;
                next_len += 1;
            }
            let k: usize = 0;
            while (k < idx_str.len && next_len < prefix.len) {
                prefix[next_len] = idx_str[k];
                next_len += 1;
                k += 1;
            }

            i = parseValue(s, i, prefix, next_len, sep, depth + 1, res);
            if (!res.ok) {
                return s.len;
            }
            idx += 1;
            i = skipWs(s, i);
            if (i < s.len && s[i] == ',') {
                i += 1;
                continue;
            }
            if (i < s.len && s[i] == ']') {
                return i + 1;
            }
            res.ok = false;
            return s.len;
        }
        res.ok = false;
        return s.len;
    }

    if (c == '"') {
        const end_q = parseJsonStringEnd(s, i);
        if (end_q >= s.len) {
            res.ok = false;
            return s.len;
        }
        * = res.pushEntry(prefix[0..prefix_len], s[i + 1..end_q]);
        return end_q + 1;
    }

    // Primitive: number, true, false, null
    const p_start = i;
    while (i < s.len && !isWs(s[i]) && s[i] != ',' && s[i] != '}' && s[i] != ']') {
        i += 1;
    }
    if (i == p_start) {
        res.ok = false;
        return s.len;
    }
    * = res.pushEntry(prefix[0..prefix_len], s[p_start..i]);
    return i;
}

pub fn flattenJsonWith(json_str: string, sep: u8) -> FlatResult {
    let res = FlatResult.empty();
    let prefix_buf: [256]u8 = undefined;
    const end_pos = parseValue(json_str, 0 as usize, prefix_buf[0..256], 0 as usize, sep, 0 as usize, &res);
    if (!res.ok) {
        return res;
    }
    const tail = skipWs(json_str, end_pos);
    if (tail != json_str.len) {
        res.ok = false;
    }
    return res;
}

pub fn flattenJson(json_str: string) -> FlatResult {
    return flattenJsonWith(json_str, '.');
}

pub fn getByPath(json_str: string, dot_path: string, out: []u8) -> string? {
    const flat = flattenJson(json_str);
    if (!flat.ok) {
        return null;
    }
    const hit = flat.get(dot_path);
    if (hit == null) {
        return null;
    }
    const v = hit ?? "";
    let n: usize = 0;
    while (n < v.len && n < out.len) {
        out[n] = v[n];
        n += 1;
    }
    return out[0..n];
}

pub fn hasPath(json_str: string, dot_path: string) -> bool {
    const flat = flattenJson(json_str);
    return flat.ok && flat.has(dot_path);
}

fn appendStr(out: []u8, pos: usize, s: string) -> usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

pub fn formatFlatJsonObject(flat: FlatResult, out: []u8) -> string {
    let pos: usize = 0;
    pos = appendStr(out, pos, "{");
    let i: usize = 0;
    while (i < flat.count) {
        if (i > 0) {
            pos = appendStr(out, pos, ",");
        }
        pos = appendStr(out, pos, "\"");
        pos = appendStr(out, pos, flat.keyAt(i));
        pos = appendStr(out, pos, "\":\"");
        pos = appendStr(out, pos, flat.valAt(i));
        pos = appendStr(out, pos, "\"");
        i += 1;
    }
    pos = appendStr(out, pos, "}");
    return out[0..pos];
}

export {
    FlatResult,
    flattenJson,
    flattenJsonWith,
    getByPath,
    hasPath,
    formatFlatJsonObject,
};
