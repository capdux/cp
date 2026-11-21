// Source: Absorbed from /data/dev/bun/src/string/PathString.cp | Synced: 2026-11-20T04:56:52Z
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/string/PathString.cp
// Upstream Sync: Refer to /data/dev/bun/src/string/PathString.zig for future upstream updates.
// ============================================================================
// lib/string/path.cp
// PathString: Lightweight zero-alloc path slice view
// Absorbed from Bun src/string/PathString.cp with pure CP semantics

pub struct PathString {
    data: []const u8 = "",

    pub static fn init(p: []const u8): PathString {
        return PathString {
            data: p,
        };
    }

    pub fn slice(): []const u8 {
        return _.data;
    }

    pub fn len(): usize {
        return _.data.len;
    }

    pub fn isEmpty(): bool {
        return _.data.len == 0;
    }

    pub fn isAbsolute(): bool {
        if (_.data.len == 0) {
            return false;
        }
        return _.data[0] == '/' as u8;
    }

    pub fn extension(): []const u8 {
        if (_.data.len == 0) {
            return "";
        }
        let i = _.data.len;
        while (i > 0) {
            i -= 1;
            const c = _.data[i];
            if (c == '/' as u8) {
                return "";
            }
            if (c == '.' as u8) {
                return _.data[i.._.data.len];
            }
        }
        return "";
    }

    pub fn basename(): []const u8 {
        if (_.data.len == 0) {
            return "";
        }
        let i = _.data.len;
        while (i > 0) {
            i -= 1;
            if (_.data[i] == '/' as u8) {
                return _.data[(i + 1).._.data.len];
            }
        }
        return _.data;
    }

    pub fn dirname(): []const u8 {
        if (_.data.len == 0) {
            return ".";
        }
        let i = _.data.len;
        while (i > 0) {
            i -= 1;
            if (_.data[i] == '/' as u8) {
                if (i == 0) {
                    return "/";
                }
                return _.data[0..i];
            }
        }
        return ".";
    }

    pub fn hasExtension(ext: []const u8): bool {
        const actual_ext = _.extension();
        if (actual_ext.len != ext.len) {
            return false;
        }
        let eq: bool = false;
        zig {
            eq = std.mem.eql(u8, actual_ext, ext);
        }
        return eq;
    }
}
