// Source: Absorbed from /data/dev/bun/src/paths/EnvPath.cp | Synced: 2026-11-22T12:49:17Z
//! Cross-platform `PATH` environment variable builder & zero-allocation entry iterator
//! with `~init()` deterministic RAII cleanup.

import std from "std";

export const DEFAULT_DELIMITER: u8 = std.fs.path.delimiter;

fn trimDelimiters(input: []const u8, delim: u8): []const u8 {
    let s = input;
    while (s.len > 0 && s[0] == delim) {
        s = s[1..];
    }
    while (s.len > 0 && s[s.len - 1] == delim) {
        s = s[0 .. s.len - 1];
    }
    return s;
}

fn withoutTrailingSlash(input: []const u8): []const u8 {
    let s = input;
    while (s.len > 1 && (s[s.len - 1] == '/' || s[s.len - 1] == '\\')) {
        s = s[0 .. s.len - 1];
    }
    return s;
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

/// Zero-allocation iterator over entries in a delimited `PATH` string.
export struct EnvPathIterator {
    raw: []const u8,
    pos: usize = 0,
    delimiter: u8 = DEFAULT_DELIMITER,

    pub static fn init(raw: []const u8, delimiter: u8): EnvPathIterator {
        return EnvPathIterator{ raw: raw, pos: 0, delimiter: delimiter };
    }

    pub fn next(*_): ([]const u8)? {
        while (_.pos < _.raw.len) {
            const start = _.pos;
            while (_.pos < _.raw.len && _.raw[_.pos] != _.delimiter) : (_.pos += 1) {}
            const entry = _.raw[start.._.pos];
            if (_.pos < _.raw.len && _.raw[_.pos] == _.delimiter) {
                _.pos += 1;
            }
            if (entry.len > 0) {
                return entry;
            }
        }
        return null;
    }
}

/// Dynamic `PATH` environment string builder with `~init()` RAII cleanup.
export struct EnvPath {
    allocator: std.mem.Allocator,
    buf: std.ArrayList(u8),
    delimiter: u8 = DEFAULT_DELIMITER,

    pub static fn init(allocator: std.mem.Allocator): EnvPath {
        return EnvPath{
            allocator: allocator,
            buf: std.ArrayList(u8).empty,
            delimiter: DEFAULT_DELIMITER,
        };
    }

    pub static fn initWithDelimiter(allocator: std.mem.Allocator, delimiter: u8): EnvPath {
        return EnvPath{
            allocator: allocator,
            buf: std.ArrayList(u8).empty,
            delimiter: delimiter,
        };
    }

    ~init() {
        _.buf.deinit(_.allocator);
    }

    pub fn slice(): []const u8 {
        return _.buf.items;
    }

    pub fn len(): usize {
        return _.buf.items.len;
    }

    pub fn isEmpty(): bool {
        return _.buf.items.len == 0;
    }

    pub fn clear(*_) {
        _.buf.shrinkRetainingCapacity(0);
    }

    pub fn iterator(): EnvPathIterator {
        return EnvPathIterator.init(_.buf.items, _.delimiter);
    }

    pub fn contains(entry: []const u8): bool {
        const needle = withoutTrailingSlash(trimDelimiters(entry, _.delimiter));
        if (needle.len == 0) return false;
        let it = _.iterator();
        while (item := it.next()) {
            if (eqlBytes(withoutTrailingSlash(item), needle)) return true;
        }
        return false;
    }

    /// Append a path entry (trims redundant delimiters & trailing slashes).
    pub fn append(*_, input: []const u8): void! {
        const trimmed = withoutTrailingSlash(trimDelimiters(input, _.delimiter));
        if (trimmed.len == 0) return;

        if (_.buf.items.len != 0) {
            _.buf.ensureUnusedCapacity(_.allocator, trimmed.len + 1)?;
            _.buf.appendAssumeCapacity(_.delimiter);
            _.buf.appendSliceAssumeCapacity(trimmed);
        } else {
            _.buf.appendSlice(_.allocator, trimmed)?;
        }
    }

    /// Append only if not already present.
    pub fn appendUnique(*_, input: []const u8): bool! {
        if (_.contains(input)) return false;
        _.append(input)?;
        return true;
    }

    /// Prepend a path entry at the front (higher lookup priority).
    pub fn prepend(*_, input: []const u8): void! {
        const trimmed = withoutTrailingSlash(trimDelimiters(input, _.delimiter));
        if (trimmed.len == 0) return;

        if (_.buf.items.len == 0) {
            _.buf.appendSlice(_.allocator, trimmed)?;
            return;
        }

        const old_len = _.buf.items.len;
        const shift = trimmed.len + 1;
        _.buf.resize(_.allocator, old_len + shift)?;
        let i = old_len;
        while (i > 0) {
            i -= 1;
            _.buf.items[i + shift] = _.buf.items[i];
        }
        @memcpy(_.buf.items[0..trimmed.len], trimmed);
        _.buf.items[trimmed.len] = _.delimiter;
    }
}
