// lib/http/headers.cp
// High-performance case-insensitive HTTP Headers collection

pub const HeaderEntry = struct {
    name: []const u8,
    value: []const u8,
};

pub const Headers = struct {
    entries: [32]HeaderEntry = undefined,
    dynamic_entries: ([]HeaderEntry)? = null,
    len: usize = 0,
    allocator: std.mem.Allocator = undefined,

    pub fn init(allocator: std.mem.Allocator): Headers {
        return Headers{
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

    pub fn get(*_, name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            const entry = if (dyn := _.dynamic_entries) dyn[i] else _.entries[i];
            if (std.ascii.eqlIgnoreCase(entry.name, name)) {
                return entry.value;
            }
        }
        return null;
    }

    pub fn has(*_, name: []const u8): bool {
        return _.get(name) != null;
    }

    pub fn set(*_, name: []const u8, value: []const u8) {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (dyn := _.dynamic_entries) {
                if (std.ascii.eqlIgnoreCase(dyn[i].name, name)) {
                    dyn[i].value = value;
                    return;
                }
            } else {
                if (std.ascii.eqlIgnoreCase(_.entries[i].name, name)) {
                    _.entries[i].value = value;
                    return;
                }
            }
        }
        _.append(name, value);
    }

    pub fn append(*_, name: []const u8, value: []const u8) {
        if (_.len < 32 and _.dynamic_entries == null) {
            _.entries[_.len] = HeaderEntry{ .name = name, .value = value };
            _.len += 1;
            return;
        }
        if (_.dynamic_entries == null) {
            const new_slice = _.allocator.alloc(HeaderEntry, 64) catch return;
            let i: usize = 0;
            while (i < _.len) : (i += 1) {
                new_slice[i] = _.entries[i];
            }
            new_slice[_.len] = HeaderEntry{ .name = name, .value = value };
            _.dynamic_entries = new_slice;
            _.len += 1;
        } else if (dyn := _.dynamic_entries) {
            if (_.len < dyn.len) {
                dyn[_.len] = HeaderEntry{ .name = name, .value = value };
                _.len += 1;
            } else {
                const new_slice = _.allocator.realloc(dyn, dyn.len * 2) catch return;
                new_slice[_.len] = HeaderEntry{ .name = name, .value = value };
                _.dynamic_entries = new_slice;
                _.len += 1;
            }
        }
    }

    pub fn delete(*_, name: []const u8) {
        let i: usize = 0;
        while (i < _.len) {
            const matches = if (dyn := _.dynamic_entries)
                std.ascii.eqlIgnoreCase(dyn[i].name, name)
            else
                std.ascii.eqlIgnoreCase(_.entries[i].name, name);

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

    pub fn at(*_, index: usize): HeaderEntry? {
        if (index >= _.len) return null;
        if (dyn := _.dynamic_entries) {
            return dyn[index];
        }
        return _.entries[index];
    }
};
