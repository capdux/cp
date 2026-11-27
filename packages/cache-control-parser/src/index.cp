// @cap/cache-control — Zero-allocation RFC 9111 HTTP Cache-Control parser, formatter & TTL engine for CP
// Clean-room MIT implementation inspired by npm `cache-control-parser` and Rust `http-cache-semantics`.

import { trim, eqIgnoreCase, findByte } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";

fn unquote(s_in: []const u8) -> []const u8 {
    const s = trim(s_in);
    if (s.len >= (2 as usize) && s[0] == '"' && s[s.len - (1 as usize)] == '"') {
        return s[1..(s.len - (1 as usize))];
    }
    return s;
}

fn parseNonNegI64(s_in: []const u8) -> i64 {
    const s = unquote(s_in);
    if (s.len == (0 as usize) || s.len > (18 as usize)) {
        return -1 as i64;
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c = s[i];
        if (c < '0' || c > '9') {
            return -1 as i64;
        }
        val = (val * (10 as u64)) + ((c - '0') as u64);
        i += 1;
    }
    return val as i64;
}

fn writeStrChecked(dst: []u8, pos: usize, s: string) -> i64 {
    if (pos + s.len > dst.len) {
        return -1 as i64;
    }
    return writeStr(s, dst, pos) as i64;
}

fn writeU64Checked(dst: []u8, pos: usize, v_in: u64) -> i64 {
    let tmp: [24]u8 = undefined;
    const n: usize = writeU64(v_in, tmp[0..24], 0);
    if (pos + n > dst.len) {
        return -1 as i64;
    }
    return writeStr(tmp[0..n], dst, pos) as i64;
}

fn appendDirectiveFlag(dst: []u8, pos_in: usize, written: *usize, name: string) -> i64 {
    let pos = pos_in;
    if (written.* > (0 as usize)) {
        const sep = writeStrChecked(dst, pos, ", ");
        if (sep < (0 as i64)) { return -1 as i64; }
        pos = sep as usize;
    }
    const nxt = writeStrChecked(dst, pos, name);
    if (nxt < (0 as i64)) { return -1 as i64; }
    written.* += 1;
    return nxt;
}

fn appendDirectiveKv(dst: []u8, pos_in: usize, written: *usize, name: string, val: i64) -> i64 {
    let pos = pos_in;
    if (written.* > (0 as usize)) {
        const sep = writeStrChecked(dst, pos, ", ");
        if (sep < (0 as i64)) { return -1 as i64; }
        pos = sep as usize;
    }
    const p1 = writeStrChecked(dst, pos, name);
    if (p1 < (0 as i64)) { return -1 as i64; }
    const p2 = writeStrChecked(dst, p1 as usize, "=");
    if (p2 < (0 as i64)) { return -1 as i64; }
    const p3 = writeU64Checked(dst, p2 as usize, val as u64);
    if (p3 < (0 as i64)) { return -1 as i64; }
    written.* += 1;
    return p3;
}

pub struct CacheControl {
    pub max_age: i64,
    pub s_maxage: i64,
    pub stale_while_revalidate: i64,
    pub stale_if_error: i64,
    pub no_cache: bool,
    pub no_store: bool,
    pub is_public: bool,
    pub is_private: bool,
    pub must_revalidate: bool,
    pub immutable: bool,
    pub no_transform: bool,

    pub static fn parse(header: []const u8) -> CacheControl {
        let cc = CacheControl{
            max_age: -1 as i64,
            s_maxage: -1 as i64,
            stale_while_revalidate: -1 as i64,
            stale_if_error: -1 as i64,
            no_cache: false,
            no_store: false,
            is_public: false,
            is_private: false,
            must_revalidate: false,
            immutable: false,
            no_transform: false,
        };

        let pos: usize = 0;
        let segStart: usize = 0;
        let inQuote = false;
        let esc = false;

        while (pos <= header.len) {
            const atEnd = pos == header.len;
            let isComma = false;
            if (!atEnd) {
                const c = header[pos];
                if (inQuote) {
                    if (esc) {
                        esc = false;
                    } else if (c == '\\') {
                        esc = true;
                    } else if (c == '"') {
                        inQuote = false;
                    }
                } else {
                    if (c == '"') {
                        inQuote = true;
                    } else if (c == ',') {
                        isComma = true;
                    }
                }
            }

            if (atEnd || isComma) {
                const part = trim(header[segStart..pos]);
                if (part.len > (0 as usize)) {
                    const eqIdx: usize = findByte(part, '=' as u8, 0) ?? part.len;
                    const key = trim(part[0..eqIdx]);
                    const hasVal = eqIdx < part.len;

                    if (eqIgnoreCase(key, "no-cache")) {
                        cc.no_cache = true;
                    } else if (eqIgnoreCase(key, "no-store")) {
                        cc.no_store = true;
                    } else if (eqIgnoreCase(key, "public")) {
                        cc.is_public = true;
                    } else if (eqIgnoreCase(key, "private")) {
                        cc.is_private = true;
                    } else if (eqIgnoreCase(key, "must-revalidate")) {
                        cc.must_revalidate = true;
                    } else if (eqIgnoreCase(key, "immutable")) {
                        cc.immutable = true;
                    } else if (eqIgnoreCase(key, "no-transform")) {
                        cc.no_transform = true;
                    } else if (hasVal) {
                        const rawVal = part[(eqIdx + (1 as usize))..part.len];
                        const num = parseNonNegI64(rawVal);
                        if (num >= (0 as i64)) {
                            if (eqIgnoreCase(key, "max-age")) {
                                cc.max_age = num;
                            } else if (eqIgnoreCase(key, "s-maxage")) {
                                cc.s_maxage = num;
                            } else if (eqIgnoreCase(key, "stale-while-revalidate")) {
                                cc.stale_while_revalidate = num;
                            } else if (eqIgnoreCase(key, "stale-if-error")) {
                                cc.stale_if_error = num;
                            }
                        }
                    }
                }
                segStart = pos + (1 as usize);
            }
            pos += 1;
        }

        return cc;
    }

    pub fn isCacheableShared() -> bool {
        if (_.no_store || _.is_private) {
            return false;
        }
        return _.s_maxage >= (0 as i64) || _.max_age >= (0 as i64) || _.is_public;
    }

    pub fn effectiveTtl(is_shared: bool) -> i64 {
        if (_.no_store || _.no_cache) {
            return 0 as i64;
        }
        if (is_shared) {
            if (_.is_private) {
                return 0 as i64;
            }
            if (_.s_maxage >= (0 as i64)) {
                return _.s_maxage;
            }
            if (_.max_age >= (0 as i64)) {
                return _.max_age;
            }
            return -1 as i64;
        }
        return _.max_age;
    }

    pub fn format(dst: []u8) -> i64 {
        let pos: usize = 0;
        let written: usize = 0;

        if (_.is_public) {
            const p = appendDirectiveFlag(dst, pos, &written, "public");
            if (p < (0 as i64)) { return -1 as i64; }
            pos = p as usize;
        }
        if (_.is_private) {
            const p = appendDirectiveFlag(dst, pos, &written, "private");
            if (p < (0 as i64)) { return -1 as i64; }
            pos = p as usize;
        }
        if (_.no_cache) {
            const p = appendDirectiveFlag(dst, pos, &written, "no-cache");
            if (p < (0 as i64)) { return -1 as i64; }
            pos = p as usize;
        }
        if (_.no_store) {
            const p = appendDirectiveFlag(dst, pos, &written, "no-store");
            if (p < (0 as i64)) { return -1 as i64; }
            pos = p as usize;
        }
        if (_.must_revalidate) {
            const p = appendDirectiveFlag(dst, pos, &written, "must-revalidate");
            if (p < (0 as i64)) { return -1 as i64; }
            pos = p as usize;
        }
        if (_.immutable) {
            const p = appendDirectiveFlag(dst, pos, &written, "immutable");
            if (p < (0 as i64)) { return -1 as i64; }
            pos = p as usize;
        }
        if (_.no_transform) {
            const p = appendDirectiveFlag(dst, pos, &written, "no-transform");
            if (p < (0 as i64)) { return -1 as i64; }
            pos = p as usize;
        }
        if (_.max_age >= (0 as i64)) {
            const p = appendDirectiveKv(dst, pos, &written, "max-age", _.max_age);
            if (p < (0 as i64)) { return -1 as i64; }
            pos = p as usize;
        }
        if (_.s_maxage >= (0 as i64)) {
            const p = appendDirectiveKv(dst, pos, &written, "s-maxage", _.s_maxage);
            if (p < (0 as i64)) { return -1 as i64; }
            pos = p as usize;
        }
        if (_.stale_while_revalidate >= (0 as i64)) {
            const p = appendDirectiveKv(dst, pos, &written, "stale-while-revalidate", _.stale_while_revalidate);
            if (p < (0 as i64)) { return -1 as i64; }
            pos = p as usize;
        }
        if (_.stale_if_error >= (0 as i64)) {
            const p = appendDirectiveKv(dst, pos, &written, "stale-if-error", _.stale_if_error);
            if (p < (0 as i64)) { return -1 as i64; }
            pos = p as usize;
        }
        return pos as i64;
    }
}

export {
    CacheControl,
};
