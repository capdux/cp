// Source: Bun src/http/Headers.zig (name-matched, needs review)
// lib/http/headers.cp
// High-performance case-insensitive HTTP Headers collection

pub struct HeaderEntry {
    pub name: []const u8 = "";
    pub value: []const u8 = "";
}

pub struct Headers {
    pub entries: [32]HeaderEntry = undefined;
    pub dynamic_entries: ([]HeaderEntry)? = null;
    pub len: usize = 0;
    pub allocator: std.mem.Allocator = undefined;

    pub fn init(allocator: std.mem.Allocator): Headers {
        return Headers{
            entries: undefined,
            dynamic_entries: null,
            len: 0,
            allocator: allocator,
        };
    }

    pub fn deinit(*_) {
        if (dynamic_buf := _.dynamic_entries) {
            _.allocator.free(dynamic_buf);
            _.dynamic_entries = null;
        }
        _.len = 0;
    }

    pub fn count(*_): usize {
        return _.len;
    }

    pub fn get(*_, name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.len) {
            if (dynamic_buf := _.dynamic_entries) {
                if (std.ascii.eqlIgnoreCase(dynamic_buf[i].name, name)) {
                    return dynamic_buf[i].value;
                }
            } else {
                if (std.ascii.eqlIgnoreCase(_.entries[i].name, name)) {
                    return _.entries[i].value;
                }
            }
            i += 1;
        }
        return null;
    }

    pub fn has(*_, name: []const u8): bool {
        return _.get(name) != null;
    }

    pub fn set(*_, name: []const u8, value: []const u8) {
        let i: usize = 0;
        while (i < _.len) {
            if (dynamic_buf_opt := _.dynamic_entries) {
                let dynamic_buf = dynamic_buf_opt;
                if (std.ascii.eqlIgnoreCase(dynamic_buf[i].name, name)) {
                    dynamic_buf[i].value = value;
                    return;
                }
            } else {
                if (std.ascii.eqlIgnoreCase(_.entries[i].name, name)) {
                    _.entries[i].value = value;
                    return;
                }
            }
            i += 1;
        }
        _.append(name, value);
    }

    pub fn append(*_, name: []const u8, value: []const u8) {
        if (_.len < 32 && _.dynamic_entries == null) {
            _.entries[_.len] = HeaderEntry{ name: name, value: value };
            _.len += 1;
            return;
        }
        if (_.dynamic_entries == null) {
            let new_slice = _.allocator.alloc(HeaderEntry, 64) catch return;
            let i: usize = 0;
            while (i < _.len) {
                new_slice[i] = _.entries[i];
                i += 1;
            }
            new_slice[_.len] = HeaderEntry{ name: name, value: value };
            _.dynamic_entries = new_slice;
            _.len += 1;
        } else if (dynamic_buf_opt := _.dynamic_entries) {
            let dynamic_buf = dynamic_buf_opt;
            if (_.len < dynamic_buf.len) {
                dynamic_buf[_.len] = HeaderEntry{ name: name, value: value };
                _.len += 1;
            } else {
                let new_slice = _.allocator.realloc(dynamic_buf, dynamic_buf.len * 2) catch return;
                new_slice[_.len] = HeaderEntry{ name: name, value: value };
                _.dynamic_entries = new_slice;
                _.len += 1;
            }
        }
    }

    pub fn delete(*_, name: []const u8) {
        let i: usize = 0;
        while (i < _.len) {
            let matches = false;
            if (dynamic_buf := _.dynamic_entries) {
                matches = std.ascii.eqlIgnoreCase(dynamic_buf[i].name, name);
            } else {
                matches = std.ascii.eqlIgnoreCase(_.entries[i].name, name);
            }

            if (matches) {
                let j = i;
                while (j + 1 < _.len) {
                    if (dynamic_buf_opt := _.dynamic_entries) {
                        let dynamic_buf = dynamic_buf_opt;
                        dynamic_buf[j] = dynamic_buf[j + 1];
                    } else {
                        _.entries[j] = _.entries[j + 1];
                    }
                    j += 1;
                }
                _.len -= 1;
            } else {
                i += 1;
            }
        }
    }

    pub fn at(*_, index: usize): HeaderEntry? {
        if (index >= _.len) {
            return null;
        }
        if (dynamic_buf := _.dynamic_entries) {
            return dynamic_buf[index];
        }
        return _.entries[index];
    }
}
