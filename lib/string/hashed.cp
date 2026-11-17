// Source: Custom implementation by Antigravity AI
// ============================================================================
// Absorbed from Bun: bun/src/string/hashed.cp
// Upstream Sync: Refer to bun/src/string/hashed.zig for future upstream updates.
// ============================================================================
// lib/string/hashed.cp
// HashedString: Pre-hashed string slice view with O(1) hash comparison rejection
// Absorbed from Bun src/string/HashedString.cp with pure CP semantics

pub fn hashBytes32(data: []const u8): u32 {
    let h: u32 = 2166136261;
    let i: usize = 0;
    while (i < data.len) {
        h = (h ^ (data[i] as u32)) *% 16777619;
        i += 1;
    }
    return if (h == 0) 1 else h;
}

pub struct HashedString {
    data: []const u8 = "",
    hash: u32 = 0,

    pub static fn init(buf: []const u8): HashedString {
        return HashedString {
            data: buf,
            hash: hashBytes32(buf),
        };
    }

    pub static fn initNoHash(buf: []const u8): HashedString {
        return HashedString {
            data: buf,
            hash: 0,
        };
    }

    pub fn len(): usize {
        return _.data.len;
    }

    pub fn slice(): []const u8 {
        return _.data;
    }

    pub fn hashVal(): u32 {
        return _.hash;
    }

    pub fn eql(other: HashedString): bool {
        if (_.hash != 0 && other.hash != 0 && _.hash != other.hash) {
            return false;
        }
        if (_.data.len != other.data.len) {
            return false;
        }
        let eq: bool = false;
        zig {
            eq = std.mem.eql(u8, self.data, other.data);
        }
        return eq;
    }
}
