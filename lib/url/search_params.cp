// Source: Absorbed from /data/dev/bun/src/url/url.cp | Synced: 2026-11-22T10:56:52Z
// ============================================================================
// lib/url/search_params.cp — WHATWG URLSearchParams & Query Scanner
// ============================================================================
import { PercentEncoding } from "./percent";
const std = @import("std");

pub struct QueryParam {
    pub key: string = "",
    pub value: string = "",
}

pub struct ScanResult {
    pub name: string = "",
    pub value: string = "",
    pub name_needs_decoding: bool = false,
    pub value_needs_decoding: bool = false,
}

/// Zero-allocation query string scanner / tokenizer
pub struct Scanner {
    pub query_string: string = "",
    pub i: usize = 0,
    pub start: usize = 0,

    pub static fn init(query_string: string): Scanner {
        if (query_string.len > 0 and query_string[0] == (63 as u8)) { // '?'
            return Scanner{ .query_string = query_string, .i = 1, .start = 1 };
        }
        return Scanner{ .query_string = query_string, .i = 0, .start = 0 };
    }

    pub fn reset(*_) {
        _.i = _.start;
    }

    pub fn next(*_): ScanResult? {
        while (_.i < _.query_string.len and _.query_string[_.i] == (38 as u8)) { // '&'
            _.i += 1;
        }
        if (_.i >= _.query_string.len) return null;

        let start_pos = _.i;
        let eq_pos: usize? = null;
        let name_needs_dec = false;
        let val_needs_dec = false;

        while (_.i < _.query_string.len and _.query_string[_.i] != (38 as u8)) {
            let c = _.query_string[_.i];
            if (c == (61 as u8) and eq_pos == null) { // '='
                eq_pos = _.i;
            } else if (c == (37 as u8) or c == (43 as u8)) { // '%' or '+'
                if (eq_pos == null) {
                    name_needs_dec = true;
                } else {
                    val_needs_dec = true;
                }
            }
            _.i += 1;
        }

        let name_slice: []const u8 = "";
        let val_slice: []const u8 = "";

        if (eq := eq_pos) {
            name_slice = _.query_string[start_pos..eq];
            val_slice = _.query_string[eq + 1 .. _.i];
        } else {
            name_slice = _.query_string[start_pos.._.i];
            val_slice = "";
        }

        return ScanResult{
            .name = name_slice,
            .value = val_slice,
            .name_needs_decoding = name_needs_dec,
            .value_needs_decoding = val_needs_dec,
        };
    }
}

pub struct URLSearchParams {
    pub entries: [64]QueryParam = [QueryParam{}; 64],
    pub dynamic_entries: []QueryParam = [],
    pub len: usize = 0,
    pub allocator: std.mem.Allocator? = null,

    pub static fn init(alloc: std.mem.Allocator?): URLSearchParams {
        return URLSearchParams{
            .entries = [QueryParam{}; 64],
            .dynamic_entries = [],
            .len = 0,
            .allocator = alloc,
        };
    }

    pub fn count(): usize {
        return _.len;
    }

    pub fn isEmpty(): bool {
        return _.len == 0;
    }

    pub fn deinit(*_) {
        if (_.dynamic_entries.len > 0) {
            if (alloc := _.allocator) {
                alloc.free(_.dynamic_entries);
            }
            _.dynamic_entries = [];
        }
        _.len = 0;
    }

    pub fn getEntry(idx: usize): QueryParam {
        if (_.dynamic_entries.len > 0) {
            return _.dynamic_entries[idx];
        }
        return _.entries[idx];
    }

    fn setEntry(*_, idx: usize, param: QueryParam) {
        if (_.dynamic_entries.len > 0) {
            _.dynamic_entries[idx] = param;
        } else {
            _.entries[idx] = param;
        }
    }

    pub fn get(key: string): string? {
        let i: usize = 0;
        while (i < _.len) {
            let entry = _.getEntry(i);
            if (std.mem.eql(u8, entry.key, key)) {
                return entry.value;
            }
            i += 1;
        }
        return null;
    }

    /// Looks up `key` and percent-decodes its value (including `+` as space) into `out`.
    pub fn getDecoded(key: string, out: []u8): string? {
        if (raw_val := _.get(key)) {
            let n = PercentEncoding.decodeToBuf(raw_val, true, out);
            return out[0..n];
        }
        return null;
    }

    pub fn has(key: string): bool {
        return _.get(key) != null;
    }

    pub fn getAll(key: string, target: []string): usize {
        let found_count: usize = 0;
        let i: usize = 0;
        while (i < _.len and found_count < target.len) {
            let entry = _.getEntry(i);
            if (std.mem.eql(u8, entry.key, key)) {
                target[found_count] = entry.value;
                found_count += 1;
            }
            i += 1;
        }
        return found_count;
    }

    pub fn append(*_, key: string, value: string) {
        let param = QueryParam{ .key = key, .value = value };
        if (_.dynamic_entries.len == 0) {
            if (_.len < 64) {
                _.entries[_.len] = param;
                _.len += 1;
                return;
            }
            // Grow to dynamic slice
            if (alloc := _.allocator) {
                let new_slice = alloc.alloc(QueryParam, 128) catch return;
                let i: usize = 0;
                while (i < _.len) {
                    new_slice[i] = _.entries[i];
                    i += 1;
                }
                new_slice[_.len] = param;
                _.dynamic_entries = new_slice;
                _.len += 1;
                return;
            }
            return;
        }

        // Already dynamic
        if (_.len < _.dynamic_entries.len) {
            _.dynamic_entries[_.len] = param;
            _.len += 1;
            return;
        }

        if (alloc := _.allocator) {
            let new_cap = _.dynamic_entries.len * 2;
            let new_slice = alloc.realloc(_.dynamic_entries, new_cap) catch return;
            new_slice[_.len] = param;
            _.dynamic_entries = new_slice;
            _.len += 1;
        }
    }

    pub fn set(*_, key: string, value: string) {
        let i: usize = 0;
        while (i < _.len) {
            let entry = _.getEntry(i);
            if (std.mem.eql(u8, entry.key, key)) {
                _.setEntry(i, QueryParam{ .key = key, .value = value });
                return;
            }
            i += 1;
        }
        _.append(key, value);
    }

    pub fn delete(*_, key: string) {
        let i: usize = 0;
        while (i < _.len) {
            let entry = _.getEntry(i);
            if (std.mem.eql(u8, entry.key, key)) {
                let j = i;
                while (j + 1 < _.len) {
                    let next_entry = _.getEntry(j + 1);
                    _.setEntry(j, next_entry);
                    j += 1;
                }
                _.len -= 1;
            } else {
                i += 1;
            }
        }
    }

    /// Removes all entries from this `URLSearchParams`.
    pub fn clear(*_) {
        _.len = 0;
    }

    /// Returns `true` if at least one entry matches both `key` and `value` (WHATWG 2-arg `has`).
    pub fn hasValue(key: string, value: string): bool {
        let i: usize = 0;
        while (i < _.len) {
            let entry = _.getEntry(i);
            if (std.mem.eql(u8, entry.key, key) and std.mem.eql(u8, entry.value, value)) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    /// Deletes only entries matching both `key` and `value` (WHATWG 2-arg `delete`).
    pub fn deleteValue(*_, key: string, value: string) {
        let i: usize = 0;
        while (i < _.len) {
            let entry = _.getEntry(i);
            if (std.mem.eql(u8, entry.key, key) and std.mem.eql(u8, entry.value, value)) {
                let j = i;
                while (j + 1 < _.len) {
                    let next_entry = _.getEntry(j + 1);
                    _.setEntry(j, next_entry);
                    j += 1;
                }
                _.len -= 1;
            } else {
                i += 1;
            }
        }
    }

    /// Sets the first matching `key` to `value` and deletes any subsequent duplicates (strict WHATWG `set`).
    pub fn setUnique(*_, key: string, value: string) {
        let found_first = false;
        let i: usize = 0;
        while (i < _.len) {
            let entry = _.getEntry(i);
            if (std.mem.eql(u8, entry.key, key)) {
                if (!found_first) {
                    _.setEntry(i, QueryParam{ .key = key, .value = value });
                    found_first = true;
                    i += 1;
                } else {
                    let j = i;
                    while (j + 1 < _.len) {
                        let next_entry = _.getEntry(j + 1);
                        _.setEntry(j, next_entry);
                        j += 1;
                    }
                    _.len -= 1;
                }
            } else {
                i += 1;
            }
        }
        if (!found_first) {
            _.append(key, value);
        }
    }

    /// Sorts all query parameters in-place by key name lexicographically
    pub fn sort(*_) {
        if (_.len <= 1) return;
        let i: usize = 0;
        while (i < _.len - 1) {
            let j: usize = 0;
            while (j < _.len - 1 - i) {
                let a = _.getEntry(j);
                let b = _.getEntry(j + 1);
                if (std.mem.order(u8, a.key, b.key) == .gt) {
                    _.setEntry(j, b);
                    _.setEntry(j + 1, a);
                }
                j += 1;
            }
            i += 1;
        }
    }

    pub static fn parse(query_str: string, alloc: std.mem.Allocator?): URLSearchParams {
        let params = URLSearchParams.init(alloc);
        let scanner = Scanner.init(query_str);
        while (res_opt := scanner.next()) {
            params.append(res_opt.name, res_opt.value);
        }
        return params;
    }

    /// Serializes parameters to `out` slice in `key=value&...` format
    pub fn toString(*_, out: []u8): string {
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            let entry = _.getEntry(i);
            if (i > 0 and out_idx < out.len) {
                out[out_idx] = 38 as u8; // '&'
                out_idx += 1;
            }
            if (out_idx + entry.key.len <= out.len) {
                @memcpy(out[out_idx .. out_idx + entry.key.len], entry.key);
                out_idx += entry.key.len;
            }
            if (out_idx < out.len) {
                out[out_idx] = 61 as u8; // '='
                out_idx += 1;
            }
            if (out_idx + entry.value.len <= out.len) {
                @memcpy(out[out_idx .. out_idx + entry.value.len], entry.value);
                out_idx += entry.value.len;
            }
            i += 1;
        }
        return out[0..out_idx];
    }

    /// Serializes parameters into newly allocated string
    pub fn toOwnedString(*_, allocator: std.mem.Allocator): !string {
        let total_len: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            let entry = _.getEntry(i);
            if (i > 0) total_len += 1; // '&'
            total_len += entry.key.len + 1 + entry.value.len; // key + '=' + val
            i += 1;
        }
        let buf = allocator.alloc(u8, total_len)?;
        return _.toString(buf);
    }

    /// Serializes parameters into `out` with RFC 3986 percent-encoding for keys and values.
    pub fn toEncodedString(*_, out: []u8): string {
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < _.len) {
            let entry = _.getEntry(i);
            if (i > 0 and out_idx < out.len) {
                out[out_idx] = 38 as u8; // '&'
                out_idx += 1;
            }
            out_idx += PercentEncoding.encodeComponentToBuf(entry.key, out[out_idx..]);
            if (out_idx < out.len) {
                out[out_idx] = 61 as u8; // '='
                out_idx += 1;
            }
            out_idx += PercentEncoding.encodeComponentToBuf(entry.value, out[out_idx..]);
            i += 1;
        }
        return out[0..out_idx];
    }
}

pub const SearchParams = URLSearchParams;
pub const Query = URLSearchParams;
pub const QueryStringMap = URLSearchParams;
