// @cap/magnet-uri — Zero-heap BitTorrent Magnet URI (BEP-9 / BEP-53) parser & builder
// Clean-room MIT implementation for CP (compatible with npm magnet-uri & Rust magnet-url).

pub const HASH_UNKNOWN: u8 = 0;
pub const HASH_BTIH_HEX: u8 = 1;    // 40-char SHA-1 hex (BitTorrent v1)
pub const HASH_BTIH_BASE32: u8 = 2; // 32-char SHA-1 Base32 (BitTorrent v1)
pub const HASH_BTMH_V2: u8 = 3;     // 68-char multihash 1220 + 64-hex SHA-256 (BitTorrent v2)

pub struct MagnetLink {
    pub hash_kind: u8,
    pub info_hash: string,
    pub display_name: string,
    pub exact_length: i64,
    pub trackers: [8]string,
    pub tracker_count: usize,
    pub valid: bool,
}

fn toLower(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') { return c + 32; }
    return c;
}

fn eqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (toLower(a[i]) != toLower(b[i])) { return false; }
        i += 1;
    }
    return true;
}

fn startsWithIgnoreCase(s: string, prefix: string): bool {
    if (s.len < prefix.len) { return false; }
    return eqIgnoreCase(s[0..prefix.len], prefix);
}

fn isHexStr(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        let c: u8 = s[i];
        let ok: bool = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
        if (!ok) { return false; }
        i += 1;
    }
    return s.len > 0;
}

fn isBase32Str(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        let c: u8 = toLower(s[i]);
        let ok: bool = (c >= 'a' && c <= 'z') || (c >= '2' && c <= '7');
        if (!ok) { return false; }
        i += 1;
    }
    return s.len > 0;
}

fn parsePositiveI64(s: string): i64 {
    if (s.len == 0) { return -1; }
    let v: i64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        let c: u8 = s[i];
        if (c < '0' || c > '9') { return -1; }
        if (v > 900000000000000000) { return -1; }
        v = v * 10 + ((c - '0') as i64);
        i += 1;
    }
    return v;
}

pub struct TopicMatch {
    pub kind: u8,
    pub hash: string,
}

/// Validate a BitTorrent `urn:btih:...` or `urn:btmh:...` exact topic value and classify its hash kind.
pub fn classifyExactTopic(xt: string): TopicMatch {
    let none: TopicMatch = TopicMatch { kind: HASH_UNKNOWN, hash: "" };
    if (startsWithIgnoreCase(xt, "urn:btih:")) {
        let h: string = xt[9..xt.len];
        if (h.len == 40 && isHexStr(h)) {
            return TopicMatch { kind: HASH_BTIH_HEX, hash: h };
        }
        if (h.len == 32 && isBase32Str(h)) {
            return TopicMatch { kind: HASH_BTIH_BASE32, hash: h };
        }
        return none;
    }
    if (startsWithIgnoreCase(xt, "urn:btmh:")) {
        let h: string = xt[9..xt.len];
        if (h.len == 68 && startsWithIgnoreCase(h, "1220") && isHexStr(h)) {
            return TopicMatch { kind: HASH_BTMH_V2, hash: h };
        }
        return none;
    }
    return none;
}

/// Parse a BitTorrent `magnet:?...` URI zero-copy into a `MagnetLink`.
pub fn parseMagnet(uri: string): MagnetLink {
    let trackers: [8]string = undefined;
    let t: usize = 0;
    while (t < 8) {
        trackers[t] = "";
        t += 1;
    }
    let res: MagnetLink = MagnetLink {
        hash_kind: HASH_UNKNOWN,
        info_hash: "",
        display_name: "",
        exact_length: -1,
        trackers: trackers,
        tracker_count: 0,
        valid: false,
    };

    if (!startsWithIgnoreCase(uri, "magnet:?")) {
        return res;
    }

    let query: string = uri[8..uri.len];
    let i: usize = 0;
    while (i < query.len) {
        let seg_start: usize = i;
        while (i < query.len && query[i] != '&') {
            i += 1;
        }
        let seg_end: usize = i;
        if (seg_end > seg_start) {
            let eq: usize = seg_start;
            while (eq < seg_end && query[eq] != '=') {
                eq += 1;
            }
            if (eq < seg_end) {
                let k: string = query[seg_start..eq];
                let v: string = query[(eq + 1)..seg_end];
                if (eqIgnoreCase(k, "xt")) {
                    let tm: TopicMatch = classifyExactTopic(v);
                    if (tm.kind != HASH_UNKNOWN && res.hash_kind == HASH_UNKNOWN) {
                        res.hash_kind = tm.kind;
                        res.info_hash = tm.hash;
                    }
                } else if (eqIgnoreCase(k, "dn")) {
                    res.display_name = v;
                } else if (eqIgnoreCase(k, "xl")) {
                    let sz: i64 = parsePositiveI64(v);
                    if (sz >= 0) {
                        res.exact_length = sz;
                    }
                } else if (eqIgnoreCase(k, "tr")) {
                    if (v.len > 0 && res.tracker_count < 8) {
                        res.trackers[res.tracker_count] = v;
                        res.tracker_count += 1;
                    }
                }
            }
        }
        if (i < query.len) { i += 1; }
    }

    if (res.hash_kind != HASH_UNKNOWN) {
        res.valid = true;
    }
    return res;
}

fn appendBytes(dst: []u8, pos: usize, src: string): usize {
    if (pos + src.len > dst.len) { return 0; }
    let i: usize = 0;
    while (i < src.len) {
        dst[pos + i] = src[i];
        i += 1;
    }
    return pos + src.len;
}

/// Format a BitTorrent v1 magnet URI (`magnet:?xt=urn:btih:<hash>[&dn=<name>][&tr=<tracker>...]`).
/// Returns bytes written to `out`, or 0 on invalid hash / insufficient buffer.
pub fn formatMagnetV1(info_hash: string, display_name: string, trackers: []const string, out: []u8): usize {
    let valid_h: bool = (info_hash.len == 40 && isHexStr(info_hash)) || (info_hash.len == 32 && isBase32Str(info_hash));
    if (!valid_h) { return 0; }

    let pos: usize = appendBytes(out, 0, "magnet:?xt=urn:btih:");
    if (pos == 0) { return 0; }
    pos = appendBytes(out, pos, info_hash);
    if (pos == 0) { return 0; }

    if (display_name.len > 0) {
        pos = appendBytes(out, pos, "&dn=");
        if (pos == 0) { return 0; }
        pos = appendBytes(out, pos, display_name);
        if (pos == 0) { return 0; }
    }

    let i: usize = 0;
    while (i < trackers.len) {
        if (trackers[i].len > 0) {
            pos = appendBytes(out, pos, "&tr=");
            if (pos == 0) { return 0; }
            pos = appendBytes(out, pos, trackers[i]);
            if (pos == 0) { return 0; }
        }
        i += 1;
    }
    return pos;
}
