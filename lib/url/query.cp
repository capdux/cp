// lib/url/query.cp
// URLSearchParams query string parser and serializer

pub const QueryParam = struct {
    key: []const u8,
    value: []const u8,
};

pub const URLSearchParams = struct {
    entries: [32]QueryParam = undefined,
    dynamic_entries: ([]QueryParam)? = null,
    len: usize = 0,
    allocator: std.mem.Allocator = undefined,

    pub fn init(allocator: std.mem.Allocator): URLSearchParams {
        return URLSearchParams{
            .entries = undefined,
            .dynamic_entries = null,
            .len = 0,
            .allocator = allocator,
        };
    }

    pub fn deinit(*_) {
        if (dyn := _.dynamic_entries) {
            _.allocator.free(dyn);
            _.dynamic_entries = null;
        }
        _.len = 0;
    }

    pub fn count(*_): usize {
        return _.len;
    }

    pub fn get(*_, key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const entry = if (dyn := _.dynamic_entries) dyn[i] else _.entries[i];
            if (std.mem.eql(u8, entry.key, key)) {
                return entry.value;
            }
        }
        return null;
    }

    pub fn has(*_, key: []const u8): bool {
        return _.get(key) != null;
    }

    pub fn append(*_, key: []const u8, value: []const u8) {
        if (_.len < 32 and _.dynamic_entries == null) {
            _.entries[_.len] = QueryParam{ .key = key, .value = value };
            _.len += 1;
            return;
        }
        if (_.dynamic_entries == null) {
            const new_slice = _.allocator.alloc(QueryParam, 64) catch return;
            let i: usize = 0;
            while (i < _.len) : (i += 1) {
                new_slice[i] = _.entries[i];
            }
            new_slice[_.len] = QueryParam{ .key = key, .value = value };
            _.dynamic_entries = new_slice;
            _.len += 1;
        } else if (dyn := _.dynamic_entries) {
            if (_.len < dyn.len) {
                dyn[_.len] = QueryParam{ .key = key, .value = value };
                _.len += 1;
            } else {
                const new_slice = _.allocator.realloc(dyn, dyn.len * 2) catch return;
                new_slice[_.len] = QueryParam{ .key = key, .value = value };
                _.dynamic_entries = new_slice;
                _.len += 1;
            }
        }
    }

    pub fn set(*_, key: []const u8, value: []const u8) {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (dyn := _.dynamic_entries) {
                if (std.mem.eql(u8, dyn[i].key, key)) {
                    dyn[i].value = value;
                    return;
                }
            } else {
                if (std.mem.eql(u8, _.entries[i].key, key)) {
                    _.entries[i].value = value;
                    return;
                }
            }
        }
        _.append(key, value);
    }

    pub fn delete(*_, key: []const u8) {
        let i: usize = 0;
        while (i < _.len) {
            const matches = if (dyn := _.dynamic_entries)
                std.mem.eql(u8, dyn[i].key, key)
            else
                std.mem.eql(u8, _.entries[i].key, key);

            if (matches) {
                let j = i;
                while (j + 1 < _.len) : (j += 1) {
                    if (dyn := _.dynamic_entries) {
                        dyn[j] = dyn[j + 1];
                    } else {
                        _.entries[j] = _.entries[j + 1];
                    }
                }
                _.len -= 1;
            } else {
                i += 1;
            }
        }
    }

    pub fn parse(query_str: []const u8, allocator: std.mem.Allocator): URLSearchParams {
        let params = URLSearchParams.init(allocator);
        let q = query_str;
        if (q.len > 0 and q[0] == '?') q = q[1..];

        let iter = std.mem.splitScalar(u8, q, '&');
        while (iter.next()) |pair| {
            if (pair.len == 0) continue;
            let eq_idx: usize? = null;
            let i: usize = 0;
            while (i < pair.len) : (i += 1) {
                if (pair[i] == '=') {
                    eq_idx = i;
                    break;
                }
            }
            if (idx := eq_idx) {
                params.append(pair[0..idx], pair[idx + 1 ..]);
            } else {
                params.append(pair, "");
            }
        }
        return params;
    }

    pub fn toString(*_, buf: *[1024]u8): []const u8 {
        let out_idx: usize = 0;
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const entry = if (dyn := _.dynamic_entries) dyn[i] else _.entries[i];
            if (i > 0 and out_idx < 1024) {
                buf[out_idx] = '&';
                out_idx += 1;
            }
            @memcpy(buf[out_idx .. out_idx + entry.key.len], entry.key);
            out_idx += entry.key.len;
            buf[out_idx] = '=';
            out_idx += 1;
            @memcpy(buf[out_idx .. out_idx + entry.value.len], entry.value);
            out_idx += entry.value.len;
        }
        return buf[0..out_idx];
    }
};
