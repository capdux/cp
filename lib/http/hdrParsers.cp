// Source: Absorbed from /data/dev/bun/src/http/Headers.cp | Synced: 2026-11-23T01:58:51Z
// lib/http/header_parsers.cp
// Standard HTTP Header Parsers: Cache-Control, Content-Disposition, Accept, Range, Content-Range

const std = @import("std");

fn trimHttpWs(s: []const u8): []const u8 {
    return std.mem.trim(u8, s, " \t\r\n");
}

fn unquoteHttpToken(s: []const u8): []const u8 {
    const t = trimHttpWs(s);
    if (t.len >= 2 && t[0] == '"' && t[t.len - 1] == '"') {
        return t[1 .. t.len - 1];
    }
    return t;
}

fn parseQFixed(raw: []const u8): u16? {
    const s = trimHttpWs(raw);
    if (s.len == 0) return null;
    if (std.mem.eql(u8, s, "1") || std.mem.eql(u8, s, "1.0") || std.mem.eql(u8, s, "1.00") || std.mem.eql(u8, s, "1.000")) {
        return 1000;
    }
    if (std.mem.eql(u8, s, "0") || std.mem.eql(u8, s, "0.0") || std.mem.eql(u8, s, "0.00") || std.mem.eql(u8, s, "0.000")) {
        return 0;
    }
    if (s.len >= 3 && s[0] == '0' && s[1] == '.') {
        let val: u16 = 0;
        let mult: u16 = 100;
        let i: usize = 2;
        while (i < s.len && i < 5) : (i += 1) {
            const ch = s[i];
            if (ch < '0' || ch > '9') return null;
            val += ((ch - '0') as u16) * mult;
            mult /= 10;
        }
        return val;
    }
    return null;
}

// ============================================================================
// 1. RFC 9111 Cache-Control Parser & Formatter
// ============================================================================

pub struct CacheControl {
    pub max_age: u64? = null;
    pub s_maxage: u64? = null;
    pub min_fresh: u64? = null;
    pub max_stale: u64? = null;
    pub stale_while_revalidate: u64? = null;
    pub stale_if_error: u64? = null;
    pub no_cache: bool = false;
    pub no_store: bool = false;
    pub no_transform: bool = false;
    pub only_if_cached: bool = false;
    pub must_revalidate: bool = false;
    pub proxy_revalidate: bool = false;
    pub is_public: bool = false;
    pub is_private: bool = false;
    pub immutable: bool = false;

    pub static fn parse(header: []const u8): CacheControl {
        let cc = CacheControl{};
        let iter = std.mem.splitScalar(u8, header, ',');
        while (part := iter.next()) {
            const dir = trimHttpWs(part);
            if (dir.len == 0) continue;

            if (eq := std.mem.indexOfScalar(u8, dir, '=')) {
                const name = trimHttpWs(dir[0..eq]);
                const val_str = unquoteHttpToken(dir[eq + 1 ..]);
                const num_opt = std.fmt.parseInt(u64, val_str, 10) catch null;
                if (std.ascii.eqlIgnoreCase(name, "max-age")) {
                    cc.max_age = num_opt;
                } else if (std.ascii.eqlIgnoreCase(name, "s-maxage")) {
                    cc.s_maxage = num_opt;
                } else if (std.ascii.eqlIgnoreCase(name, "min-fresh")) {
                    cc.min_fresh = num_opt;
                } else if (std.ascii.eqlIgnoreCase(name, "max-stale")) {
                    cc.max_stale = num_opt;
                } else if (std.ascii.eqlIgnoreCase(name, "stale-while-revalidate")) {
                    cc.stale_while_revalidate = num_opt;
                } else if (std.ascii.eqlIgnoreCase(name, "stale-if-error")) {
                    cc.stale_if_error = num_opt;
                } else if (std.ascii.eqlIgnoreCase(name, "no-cache")) {
                    cc.no_cache = true;
                } else if (std.ascii.eqlIgnoreCase(name, "private")) {
                    cc.is_private = true;
                }
            } else {
                if (std.ascii.eqlIgnoreCase(dir, "no-cache")) {
                    cc.no_cache = true;
                } else if (std.ascii.eqlIgnoreCase(dir, "no-store")) {
                    cc.no_store = true;
                } else if (std.ascii.eqlIgnoreCase(dir, "no-transform")) {
                    cc.no_transform = true;
                } else if (std.ascii.eqlIgnoreCase(dir, "only-if-cached")) {
                    cc.only_if_cached = true;
                } else if (std.ascii.eqlIgnoreCase(dir, "must-revalidate")) {
                    cc.must_revalidate = true;
                } else if (std.ascii.eqlIgnoreCase(dir, "proxy-revalidate")) {
                    cc.proxy_revalidate = true;
                } else if (std.ascii.eqlIgnoreCase(dir, "public")) {
                    cc.is_public = true;
                } else if (std.ascii.eqlIgnoreCase(dir, "private")) {
                    cc.is_private = true;
                } else if (std.ascii.eqlIgnoreCase(dir, "immutable")) {
                    cc.immutable = true;
                } else if (std.ascii.eqlIgnoreCase(dir, "max-stale")) {
                    cc.max_stale = 0xFFFFFFFF;
                }
            }
        }
        return cc;
    }

    static fn appendToken(buf: []u8, pos: *usize, tok: []const u8): ! {
        if (pos.* > 0) {
            if (pos.* + 2 > buf.len) return error.BufferTooSmall;
            buf[pos.*] = ',';
            buf[pos.* + 1] = ' ';
            pos.* += 2;
        }
        if (pos.* + tok.len > buf.len) return error.BufferTooSmall;
        @memcpy(buf[pos.* .. pos.* + tok.len], tok);
        pos.* += tok.len;
    }

    pub fn format(buf: []u8): ([]const u8)! {
        let pos: usize = 0;
        if (_.is_public) try CacheControl.appendToken(buf, &pos, "public");
        if (_.is_private) try CacheControl.appendToken(buf, &pos, "private");
        if (_.no_cache) try CacheControl.appendToken(buf, &pos, "no-cache");
        if (_.no_store) try CacheControl.appendToken(buf, &pos, "no-store");
        if (_.no_transform) try CacheControl.appendToken(buf, &pos, "no-transform");
        if (_.only_if_cached) try CacheControl.appendToken(buf, &pos, "only-if-cached");
        if (_.must_revalidate) try CacheControl.appendToken(buf, &pos, "must-revalidate");
        if (_.proxy_revalidate) try CacheControl.appendToken(buf, &pos, "proxy-revalidate");
        if (_.immutable) try CacheControl.appendToken(buf, &pos, "immutable");
        let tmp: [64]u8 = undefined;
        if (v := _.max_age) {
            const s = std.fmt.bufPrint(&tmp, "max-age={d}", .{v}) catch return error.BufferTooSmall;
            try CacheControl.appendToken(buf, &pos, s);
        }
        if (v := _.s_maxage) {
            const s = std.fmt.bufPrint(&tmp, "s-maxage={d}", .{v}) catch return error.BufferTooSmall;
            try CacheControl.appendToken(buf, &pos, s);
        }
        if (v := _.stale_while_revalidate) {
            const s = std.fmt.bufPrint(&tmp, "stale-while-revalidate={d}", .{v}) catch return error.BufferTooSmall;
            try CacheControl.appendToken(buf, &pos, s);
        }
        return buf[0..pos];
    }
}

// ============================================================================
// 2. RFC 6266 Content-Disposition Parser
// ============================================================================

pub enum DispositionType: u8 {
    Inline = 0,
    Attachment = 1,
    FormData = 2,
    Other = 3,
}

pub struct ContentDisposition {
    pub disposition: DispositionType = DispositionType.Other,
    pub raw_type: []const u8 = "",
    pub name: ([]const u8)? = null;
    pub filename: ([]const u8)? = null;
    pub filename_ext: ([]const u8)? = null;

    pub static fn parse(header: []const u8): ContentDisposition {
        let cd = ContentDisposition{};
        let iter = std.mem.splitScalar(u8, header, ';');
        if (first := iter.next()) {
            const disp = trimHttpWs(first);
            cd.raw_type = disp;
            if (std.ascii.eqlIgnoreCase(disp, "inline")) {
                cd.disposition = DispositionType.Inline;
            } else if (std.ascii.eqlIgnoreCase(disp, "attachment")) {
                cd.disposition = DispositionType.Attachment;
            } else if (std.ascii.eqlIgnoreCase(disp, "form-data")) {
                cd.disposition = DispositionType.FormData;
            } else {
                cd.disposition = DispositionType.Other;
            }
        }
        while (param := iter.next()) {
            const p = trimHttpWs(param);
            if (p.len == 0) continue;
            if (eq := std.mem.indexOfScalar(u8, p, '=')) {
                const k = trimHttpWs(p[0..eq]);
                const v_raw = trimHttpWs(p[eq + 1 ..]);
                if (std.ascii.eqlIgnoreCase(k, "name")) {
                    cd.name = unquoteHttpToken(v_raw);
                } else if (std.ascii.eqlIgnoreCase(k, "filename")) {
                    cd.filename = unquoteHttpToken(v_raw);
                } else if (std.ascii.eqlIgnoreCase(k, "filename*")) {
                    // RFC 5987 ext-value: charset'lang'value
                    if (sq1 := std.mem.indexOfScalar(u8, v_raw, '\'')) {
                        if (sq2 := std.mem.indexOfScalarPos(u8, v_raw, sq1 + 1, '\'')) {
                            cd.filename_ext = v_raw[sq2 + 1 ..];
                        } else {
                            cd.filename_ext = v_raw;
                        }
                    } else {
                        cd.filename_ext = unquoteHttpToken(v_raw);
                    }
                }
            }
        }
        return cd;
    }

    pub fn effectiveFilename(): ([]const u8)? {
        if (ext := _.filename_ext) return ext;
        return _.filename;
    }
}

// ============================================================================
// 3. RFC 9110 §12.5.1 Accept Content Negotiation Parser
// ============================================================================

pub struct AcceptItem {
    pub media_range: []const u8 = "",
    pub type_part: []const u8 = "",
    pub subtype_part: []const u8 = "",
    pub q_x1000: u16 = 1000;
    pub specificity: u8 = 0;
    pub order: usize = 0;
}

pub struct AcceptParser {
    pub static fn parse(header: []const u8, out: []AcceptItem): usize {
        let count: usize = 0;
        let iter = std.mem.splitScalar(u8, header, ',');
        while (tok := iter.next()) {
            const t = trimHttpWs(tok);
            if (t.len == 0 || count >= out.len) continue;

            let media_range = t;
            let q_val: u16 = 1000;
            let param_count: u8 = 0;
            if (semi := std.mem.indexOfScalar(u8, t, ';')) {
                media_range = trimHttpWs(t[0..semi]);
                let p_iter = std.mem.splitScalar(u8, t[semi + 1 ..], ';');
                while (p := p_iter.next()) {
                    const pt = trimHttpWs(p);
                    if (eq := std.mem.indexOfScalar(u8, pt, '=')) {
                        const pk = trimHttpWs(pt[0..eq]);
                        const pv = trimHttpWs(pt[eq + 1 ..]);
                        if (std.ascii.eqlIgnoreCase(pk, "q")) {
                            if (pq := parseQFixed(pv)) {
                                q_val = pq;
                            }
                        } else {
                            param_count += 1;
                        }
                    }
                }
            }

            const slash = std.mem.indexOfScalar(u8, media_range, '/') ?? continue;
            const tp = trimHttpWs(media_range[0..slash]);
            const sub = trimHttpWs(media_range[slash + 1 ..]);
            if (tp.len == 0 || sub.len == 0) continue;

            let spec: u8 = 2;
            if (std.mem.eql(u8, tp, "*") && std.mem.eql(u8, sub, "*")) {
                spec = 0;
            } else if (std.mem.eql(u8, sub, "*")) {
                spec = 1;
            } else if (param_count > 0) {
                spec = 3;
            }

            out[count] = AcceptItem{
                media_range: media_range,
                type_part: tp,
                subtype_part: sub,
                q_x1000: q_val,
                specificity: spec,
                order: count,
            };
            count += 1;
        }

        // Stable sort descending by q_x1000, then specificity, then original order
        let i: usize = 1;
        while (i < count) : (i += 1) {
            const key = out[i];
            let j = i;
            while (j > 0) {
                const prev = out[j - 1];
                const swap = (prev.q_x1000 < key.q_x1000) ||
                    (prev.q_x1000 == key.q_x1000 && prev.specificity < key.specificity) ||
                    (prev.q_x1000 == key.q_x1000 && prev.specificity == key.specificity && prev.order > key.order);
                if (swap) {
                    out[j] = prev;
                    j -= 1;
                } else {
                    break;
                }
            }
            out[j] = key;
        }
        return count;
    }

    pub static fn matchesMime(item: AcceptItem, mime: []const u8): bool {
        let base = mime;
        if (semi := std.mem.indexOfScalar(u8, mime, ';')) {
            base = mime[0..semi];
        }
        base = trimHttpWs(base);
        const slash = std.mem.indexOfScalar(u8, base, '/') ?? return false;
        const m_type = trimHttpWs(base[0..slash]);
        const m_sub = trimHttpWs(base[slash + 1 ..]);

        if (std.mem.eql(u8, item.type_part, "*") && std.mem.eql(u8, item.subtype_part, "*")) return true;
        if (!std.ascii.eqlIgnoreCase(item.type_part, m_type)) return false;
        if (std.mem.eql(u8, item.subtype_part, "*")) return true;
        return std.ascii.eqlIgnoreCase(item.subtype_part, m_sub);
    }

    pub static fn negotiate(accept_header: []const u8, supported: []const []const u8): ([]const u8)? {
        if (supported.len == 0) return null;
        if (trimHttpWs(accept_header).len == 0) return supported[0];

        let items: [32]AcceptItem = undefined;
        const n = AcceptParser.parse(accept_header, &items);
        if (n == 0) return supported[0];

        let i: usize = 0;
        while (i < n) : (i += 1) {
            const item = items[i];
            if (item.q_x1000 == 0) continue;
            for (cand of supported) {
                if (AcceptParser.matchesMime(item, cand)) {
                    // Ensure there isn't a more specific q=0 rejection for `cand`
                    let rejected = false;
                    let k: usize = 0;
                    while (k < n) : (k += 1) {
                        if (items[k].q_x1000 == 0 && items[k].specificity > item.specificity && AcceptParser.matchesMime(items[k], cand)) {
                            rejected = true;
                            break;
                        }
                    }
                    if (!rejected) return cand;
                }
            }
        }
        return null;
    }
}

// ============================================================================
// 4. RFC 9110 §14.1.2 Range & §14.4 Content-Range Parsers
// ============================================================================

pub struct ResolvedByteRange {
    pub start: u64 = 0;
    pub end: u64 = 0;

    pub fn length(): u64 {
        if (_.end < _.start) return 0;
        return (_.end - _.start) + 1;
    }
}

pub struct ByteRangeSpec {
    pub start: u64 = 0;
    pub end: u64 = 0;
    pub has_start: bool = false;
    pub has_end: bool = false;
    pub is_suffix: bool = false;
    pub suffix_len: u64 = 0;

    pub fn resolve(total_len: u64): ResolvedByteRange? {
        if (total_len == 0) return null;
        if (_.is_suffix) {
            if (_.suffix_len == 0) return null;
            const take = if (_.suffix_len > total_len) total_len else _.suffix_len;
            return ResolvedByteRange{
                start: total_len - take,
                end: total_len - 1,
            };
        }
        if (!_.has_start) return null;
        if (_.start >= total_len) return null;
        let resolved_end = total_len - 1;
        if (_.has_end) {
            if (_.end < _.start) return null;
            if (_.end < total_len) {
                resolved_end = _.end;
            }
        }
        return ResolvedByteRange{
            start: _.start,
            end: resolved_end,
        };
    }
}

pub const MAX_RANGE_SPECS: usize = 16;

pub struct RangeHeader {
    pub unit: []const u8 = "bytes";
    pub ranges: [MAX_RANGE_SPECS]ByteRangeSpec = undefined;
    pub count: usize = 0;
    pub is_valid: bool = false;

    pub static fn parse(header: []const u8): RangeHeader {
        let rh = RangeHeader{};
        const trimmed = trimHttpWs(header);
        const eq = std.mem.indexOfScalar(u8, trimmed, '=') ?? return rh;
        const unit = trimHttpWs(trimmed[0..eq]);
        if (unit.len == 0) return rh;
        rh.unit = unit;

        const spec_list = trimHttpWs(trimmed[eq + 1 ..]);
        if (spec_list.len == 0) return rh;

        let iter = std.mem.splitScalar(u8, spec_list, ',');
        while (part := iter.next()) {
            const p = trimHttpWs(part);
            if (p.len == 0) return RangeHeader{};
            const dash = std.mem.indexOfScalar(u8, p, '-') ?? return RangeHeader{};
            const left = trimHttpWs(p[0..dash]);
            const right = trimHttpWs(p[dash + 1 ..]);

            if (left.len == 0 && right.len == 0) return RangeHeader{};
            if (rh.count >= MAX_RANGE_SPECS) break;

            if (left.len == 0) {
                const suf = std.fmt.parseInt(u64, right, 10) catch return RangeHeader{};
                rh.ranges[rh.count] = ByteRangeSpec{
                    is_suffix: true,
                    suffix_len: suf,
                };
                rh.count += 1;
            } else {
                const s = std.fmt.parseInt(u64, left, 10) catch return RangeHeader{};
                if (right.len == 0) {
                    rh.ranges[rh.count] = ByteRangeSpec{
                        start: s,
                        has_start: true,
                        has_end: false,
                    };
                } else {
                    const e = std.fmt.parseInt(u64, right, 10) catch return RangeHeader{};
                    if (e < s) return RangeHeader{};
                    rh.ranges[rh.count] = ByteRangeSpec{
                        start: s,
                        end: e,
                        has_start: true,
                        has_end: true,
                    };
                }
                rh.count += 1;
            }
        }
        rh.is_valid = rh.count > 0;
        return rh;
    }
}

pub struct ContentRange {
    pub unit: []const u8 = "bytes",
    pub start: u64? = null;
    pub end: u64? = null;
    pub total_length: u64? = null;
    pub is_unsatisfied: bool = false;
    pub is_valid: bool = false;

    pub static fn parse(header: []const u8): ContentRange {
        let cr = ContentRange{};
        const t = trimHttpWs(header);
        const sp = std.mem.indexOfScalar(u8, t, ' ') ?? return cr;
        cr.unit = trimHttpWs(t[0..sp]);
        const rest = trimHttpWs(t[sp + 1 ..]);
        const slash = std.mem.indexOfScalar(u8, rest, '/') ?? return cr;
        const range_part = trimHttpWs(rest[0..slash]);
        const total_part = trimHttpWs(rest[slash + 1 ..]);

        if (std.mem.eql(u8, total_part, "*")) {
            cr.total_length = null;
        } else {
            cr.total_length = std.fmt.parseInt(u64, total_part, 10) catch return ContentRange{};
        }

        if (std.mem.eql(u8, range_part, "*")) {
            if (cr.total_length == null) return ContentRange{};
            cr.is_unsatisfied = true;
            cr.is_valid = true;
            return cr;
        }

        const dash = std.mem.indexOfScalar(u8, range_part, '-') ?? return ContentRange{};
        const s_str = trimHttpWs(range_part[0..dash]);
        const e_str = trimHttpWs(range_part[dash + 1 ..]);
        const s_val = std.fmt.parseInt(u64, s_str, 10) catch return ContentRange{};
        const e_val = std.fmt.parseInt(u64, e_str, 10) catch return ContentRange{};
        if (e_val < s_val) return ContentRange{};
        if (tot := cr.total_length) {
            if (e_val >= tot) return ContentRange{};
        }

        cr.start = s_val;
        cr.end = e_val;
        cr.is_valid = true;
        return cr;
    }

    pub fn format(buf: []u8): ([]const u8)! {
        if (!_.is_valid) return error.InvalidContentRange;
        if (_.is_unsatisfied) {
            if (tot := _.total_length) {
                return std.fmt.bufPrint(buf, "{s} */{d}", .{ _.unit, tot }) catch return error.BufferTooSmall;
            }
            return error.InvalidContentRange;
        }
        const s = _.start ?? return error.InvalidContentRange;
        const e = _.end ?? return error.InvalidContentRange;
        if (tot := _.total_length) {
            return std.fmt.bufPrint(buf, "{s} {d}-{d}/{d}", .{ _.unit, s, e, tot }) catch return error.BufferTooSmall;
        }
        return std.fmt.bufPrint(buf, "{s} {d}-{d}/*", .{ _.unit, s, e }) catch return error.BufferTooSmall;
    }
}
