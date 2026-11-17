// Source: UNVERIFIED (origin to be confirmed: Bun / Zig std / custom)
// lib/router/param_map.cp
// High-performance route parameter dictionary with inline stack storage

pub const Param = struct {
    name: []const u8,
    value: []const u8,
};

pub const ParamMap = struct {
    entries: [16]Param = undefined,
    len: usize = 0,

    pub fn init(): ParamMap {
        return ParamMap{
            .entries = undefined,
            .len = 0,
        };
    }

    pub fn count(*_): usize {
        return _.len;
    }

    pub fn isEmpty(*_): bool {
        return _.len == 0;
    }

    pub fn get(*_, name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.len) : (i += 1) {
            if (std.mem.eql(u8, _.entries[i].name, name)) {
                return _.entries[i].value;
            }
        }
        return null;
    }

    pub fn at(*_, index: usize): Param? {
        if (index >= _.len) return null;
        return _.entries[index];
    }

    pub fn has(*_, name: []const u8): bool {
        return _.get(name) != null;
    }

    pub fn append(*_, name: []const u8, value: []const u8) {
        if (_.len < 16) {
            _.entries[_.len] = Param{ .name = name, .value = value };
            _.len += 1;
        }
    }
};
