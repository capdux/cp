// Source: Absorbed from /data/dev/bun/src/http_types/Encoding.cp | Synced: 2026-11-23T21:45:24Z
// lib/http/encoding.cp
// CP HTTP Content & Transfer Encoding & Accept-Encoding Negotiation (cap:http)

const std = @import("std");

pub enum Encoding {
    Identity,
    Gzip,
    Deflate,
    Brotli,
    Zstd,
    Chunked,

    pub fn canUseLibDeflate(): bool {
        return switch (_) {
            .Gzip, .Deflate => true,
            else => false,
        };
    }

    pub fn isCompressed(): bool {
        return switch (_) {
            .Gzip, .Deflate, .Brotli, .Zstd => true,
            else => false,
        };
    }

    pub fn toString(): []const u8 {
        return switch (_) {
            .Identity => "identity",
            .Gzip => "gzip",
            .Deflate => "deflate",
            .Brotli => "br",
            .Zstd => "zstd",
            .Chunked => "chunked",
        };
    }

    pub static fn fromString(raw: []const u8): Encoding? {
        let start: usize = 0;
        let end: usize = raw.len;
        while (start < end && (raw[start] == ' ' || raw[start] == '\t')) : (start += 1) {}
        while (end > start && (raw[end - 1] == ' ' || raw[end - 1] == '\t')) : (end -= 1) {}
        const s = raw[start..end];
        if (std.ascii.eqlIgnoreCase(s, "gzip") || std.ascii.eqlIgnoreCase(s, "x-gzip")) return Encoding.Gzip;
        if (std.ascii.eqlIgnoreCase(s, "deflate")) return Encoding.Deflate;
        if (std.ascii.eqlIgnoreCase(s, "br")) return Encoding.Brotli;
        if (std.ascii.eqlIgnoreCase(s, "zstd")) return Encoding.Zstd;
        if (std.ascii.eqlIgnoreCase(s, "identity")) return Encoding.Identity;
        if (std.ascii.eqlIgnoreCase(s, "chunked")) return Encoding.Chunked;
        return null;
    }
}

pub struct AcceptEncodingItem {
    pub encoding: Encoding = Encoding.Identity,
    pub is_wildcard: bool = false,
    pub raw_name: []const u8 = "",
    pub q_x1000: u16 = 1000,
    pub order: usize = 0,
}

pub struct ContentEncodingNegotiator {
    fn trimWs(s: []const u8): []const u8 {
        let start: usize = 0;
        let end: usize = s.len;
        while (start < end && (s[start] == ' ' || s[start] == '\t')) : (start += 1) {}
        while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t')) : (end -= 1) {}
        return s[start..end];
    }

    /// Parses q-value (e.g. "0.8", "1", "1.000", "0") into fixed-point 0..1000.
    pub static fn parseQValue(raw: []const u8): u16? {
        const s = trimWs(raw);
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

    /// Parses an `Accept-Encoding` header into `out`, sorted descending by `q_x1000` (stable on tie).
    pub static fn parseAcceptEncoding(header: []const u8, out: []AcceptEncodingItem): usize {
        let count: usize = 0;
        let iter = std.mem.splitScalar(u8, header, ',');
        while (token := iter.next()) {
            const trimmed = trimWs(token);
            if (trimmed.len == 0) continue;
            if (count >= out.len) break;

            let name = trimmed;
            let q_val: u16 = 1000;
            if (semi := std.mem.indexOfScalar(u8, trimmed, ';')) {
                name = trimWs(trimmed[0..semi]);
                let param_iter = std.mem.splitScalar(u8, trimmed[semi + 1 ..], ';');
                while (p := param_iter.next()) {
                    const pt = trimWs(p);
                    if (eq := std.mem.indexOfScalar(u8, pt, '=')) {
                        const k = trimWs(pt[0..eq]);
                        const v = trimWs(pt[eq + 1 ..]);
                        if (std.ascii.eqlIgnoreCase(k, "q")) {
                            if (parsed_q := parseQValue(v)) {
                                q_val = parsed_q;
                            }
                        }
                    }
                }
            }

            if (std.mem.eql(u8, name, "*")) {
                out[count] = AcceptEncodingItem{
                    encoding: Encoding.Identity,
                    is_wildcard: true,
                    raw_name: name,
                    q_x1000: q_val,
                    order: count,
                };
                count += 1;
            } else if (enc := Encoding.fromString(name)) {
                out[count] = AcceptEncodingItem{
                    encoding: enc,
                    is_wildcard: false,
                    raw_name: name,
                    q_x1000: q_val,
                    order: count,
                };
                count += 1;
            }
        }

        // Insertion sort descending by q_x1000, preserving original order on ties
        let i: usize = 1;
        while (i < count) : (i += 1) {
            const key = out[i];
            let j: usize = i;
            while (j > 0) {
                const prev = out[j - 1];
                if (prev.q_x1000 < key.q_x1000 || (prev.q_x1000 == key.q_x1000 && prev.order > key.order)) {
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

    /// Selects the highest-priority supported encoding given client `Accept-Encoding` header.
    pub static fn negotiate(accept_encoding_header: []const u8, server_supported: []const Encoding): Encoding {
        if (accept_encoding_header.len == 0 || server_supported.len == 0) {
            return Encoding.Identity;
        }
        let items: [16]AcceptEncodingItem = undefined;
        const n = parseAcceptEncoding(accept_encoding_header, &items);
        if (n == 0) return Encoding.Identity;

        let wildcard_q: u16? = null;
        let k: usize = 0;
        while (k < n) : (k += 1) {
            if (items[k].is_wildcard) {
                wildcard_q = items[k].q_x1000;
                break;
            }
        }

        // First check explicitly ranked items (already sorted descending by q_x1000)
        k = 0;
        while (k < n) : (k += 1) {
            const item = items[k];
            if (item.q_x1000 == 0) continue;
            if (item.is_wildcard) {
                // Match first server_supported encoding that wasn't explicitly set to q=0
                for (sup of server_supported) {
                    let explicitly_rejected = false;
                    let m: usize = 0;
                    while (m < n) : (m += 1) {
                        if (!items[m].is_wildcard && items[m].encoding == sup && items[m].q_x1000 == 0) {
                            explicitly_rejected = true;
                            break;
                        }
                    }
                    if (!explicitly_rejected) return sup;
                }
            } else {
                for (sup of server_supported) {
                    if (sup == item.encoding) {
                        return sup;
                    }
                }
            }
        }

        if (wq := wildcard_q) {
            if (wq > 0) return server_supported[0];
        }
        return Encoding.Identity;
    }
}
