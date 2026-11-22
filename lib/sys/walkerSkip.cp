// Source: Absorbed from /data/dev/bun/src/sys/walker_skippable.cp | Synced: 2026-11-22T16:35:32Z
// Pure-CP zero-allocation hash-accelerated directory walker skip filter & path stack.
// Mirrors Bun's `Walker` skip_dirnames/skip_filenames/skip_all hash tables, DT_UNKNOWN
// fallback policy, and single-buffer `name_buffer` shrink-and-append traversal stack.

pub const WALKER_MAX_SKIP_RULES: usize = 64;
pub const WALKER_MAX_DEPTH: usize = 64;
pub const WALKER_PATH_BUF_CAP: usize = 4096;

pub enum WalkerEntryKind {
    File,
    Directory,
    SymLink,
    Unknown,
}

pub enum WalkerDecision {
    Visit,
    Descend,
    Skip,
    ResolveUnknown,
}

pub err WalkerSkipError {
    RuleTableFull,
    PathTooLong,
    MaxDepthExceeded,
    EmptyStack,
}

/// Seeded 64-bit FNV-1a hash for entry basenames (zero-allocation, deterministic).
pub fn walkerHashName(seed: u64, name: []const u8): u64 {
    let h: u64 = @as(u64, 0xcbf29ce484222325) ^ seed;
    let i: usize = 0;
    while (i < name.len) {
        h ^= @as(u64, name[i]);
        h = h *% @as(u64, 0x100000001b3);
        i += 1;
    }
    h ^= h >> 33;
    h = h *% @as(u64, 0xff51afd7ed558ccd);
    h ^= h >> 33;
    return h;
}

fn containsHash(table: []const u64, count: usize, target: u64): bool {
    let i: usize = 0;
    while (i < count) {
        if (table[i] == target) return true;
        i += 1;
    }
    return false;
}

fn insertHash(table: []u64, count: *usize, hash_val: u64): ! {
    if (containsHash(table[0..], count.*, hash_val)) return;
    if (count.* >= table.len) return WalkerSkipError.RuleTableFull;
    table[count.*] = hash_val;
    count.* += 1;
}

/// Hash-accelerated skip filter for directory traversal.
/// Avoids computing hashes altogether when the relevant rule table is empty.
pub struct WalkerSkipFilter {
    skip_dirnames: [WALKER_MAX_SKIP_RULES]u64 = [0; WALKER_MAX_SKIP_RULES],
    skip_dir_count: usize = 0,
    skip_filenames: [WALKER_MAX_SKIP_RULES]u64 = [0; WALKER_MAX_SKIP_RULES],
    skip_file_count: usize = 0,
    skip_all: [WALKER_MAX_SKIP_RULES]u64 = [0; WALKER_MAX_SKIP_RULES],
    skip_all_count: usize = 0,
    seed: u64 = 0,
    resolve_unknown_entry_types: bool = false,
    skip_hidden: bool = false,

    static fn init(seed: u64): WalkerSkipFilter {
        return WalkerSkipFilter { seed: seed };
    }

    static fn withCommonIgnores(seed: u64): WalkerSkipFilter {
        let f = WalkerSkipFilter { seed: seed };
        f.addSkipDir(".git") catch {};
        f.addSkipDir("node_modules") catch {};
        f.addSkipDir(".cap") catch {};
        f.addSkipFile(".DS_Store") catch {};
        return f;
    }

    fn setResolveUnknown(*_, enabled: bool) {
        _.resolve_unknown_entry_types = enabled;
    }

    fn setSkipHidden(*_, enabled: bool) {
        _.skip_hidden = enabled;
    }

    fn dirRuleCount(): usize {
        return _.skip_dir_count;
    }

    fn fileRuleCount(): usize {
        return _.skip_file_count;
    }

    fn allRuleCount(): usize {
        return _.skip_all_count;
    }

    /// Registers a directory basename to skip (also added to `skip_all` for symlinks).
    fn addSkipDir(*_, name: []const u8): ! {
        const h = walkerHashName(_.seed, name);
        insertHash(_.skip_dirnames[0..], &_.skip_dir_count, h)?;
        insertHash(_.skip_all[0..], &_.skip_all_count, h)?;
    }

    /// Registers a file basename to skip (also added to `skip_all` for symlinks).
    fn addSkipFile(*_, name: []const u8): ! {
        const h = walkerHashName(_.seed, name);
        insertHash(_.skip_filenames[0..], &_.skip_file_count, h)?;
        insertHash(_.skip_all[0..], &_.skip_all_count, h)?;
    }

    /// Registers a basename to skip regardless of entry kind.
    fn addSkipAny(*_, name: []const u8): ! {
        const h = walkerHashName(_.seed, name);
        insertHash(_.skip_dirnames[0..], &_.skip_dir_count, h)?;
        insertHash(_.skip_filenames[0..], &_.skip_file_count, h)?;
        insertHash(_.skip_all[0..], &_.skip_all_count, h)?;
    }

    /// Returns true if `(kind, name)` should be skipped.
    fn shouldSkip(kind: WalkerEntryKind, name: []const u8): bool {
        if (_.skip_hidden && name.len > 0 && name[0] == '.') {
            return true;
        }
        if (kind == WalkerEntryKind.Directory) {
            if (_.skip_dir_count == 0) return false;
            return containsHash(_.skip_dirnames[0..], _.skip_dir_count, walkerHashName(_.seed, name));
        }
        if (kind == WalkerEntryKind.File) {
            if (_.skip_file_count == 0) return false;
            return containsHash(_.skip_filenames[0..], _.skip_file_count, walkerHashName(_.seed, name));
        }
        if (kind == WalkerEntryKind.SymLink) {
            if (_.skip_all_count == 0) return false;
            return containsHash(_.skip_all[0..], _.skip_all_count, walkerHashName(_.seed, name));
        }
        return false;
    }

    /// Evaluates traversal action for `(kind, name)`, honoring `resolve_unknown_entry_types`.
    fn evaluate(kind: WalkerEntryKind, name: []const u8): WalkerDecision {
        if (kind == WalkerEntryKind.Unknown && _.resolve_unknown_entry_types) {
            return WalkerDecision.ResolveUnknown;
        }
        if (_.shouldSkip(kind, name)) {
            return WalkerDecision.Skip;
        }
        if (kind == WalkerEntryKind.Directory) {
            return WalkerDecision.Descend;
        }
        return WalkerDecision.Visit;
    }
}

/// Single-buffer zero-allocation directory path stack mirroring Bun's `name_buffer` + `dirname_len`.
pub struct WalkerPathStack {
    buf: [WALKER_PATH_BUF_CAP]u8 = [0; WALKER_PATH_BUF_CAP],
    cur_len: usize = 0,
    dir_len: usize = 0,
    parent_lens: [WALKER_MAX_DEPTH]usize = [0; WALKER_MAX_DEPTH],
    depth_count: usize = 0,
    sep: u8 = '/',

    static fn initPosix(): WalkerPathStack {
        return WalkerPathStack { sep: '/' };
    }

    static fn initWithSep(sep: u8): WalkerPathStack {
        return WalkerPathStack { sep: sep };
    }

    fn depth(): usize {
        return _.depth_count;
    }

    fn currentDir(): []const u8 {
        return _.buf[0.._.dir_len];
    }

    fn currentPath(): []const u8 {
        return _.buf[0.._.cur_len];
    }

    fn reset(*_) {
        _.cur_len = 0;
        _.dir_len = 0;
        _.depth_count = 0;
    }

    /// Joins an entry basename under the active directory without advancing the directory stack.
    fn joinEntry(*_, basename: []const u8): ([]const u8)? {
        let base_len = _.dir_len;
        const need_sep = base_len > 0 && _.buf[base_len - 1] != _.sep;
        const extra = (if (need_sep) @as(usize, 1) else @as(usize, 0)) + basename.len;
        if (base_len + extra > WALKER_PATH_BUF_CAP) return null;

        if (need_sep) {
            _.buf[base_len] = _.sep;
            base_len += 1;
        }
        let i: usize = 0;
        while (i < basename.len) {
            _.buf[base_len + i] = basename[i];
            i += 1;
        }
        _.cur_len = base_len + basename.len;
        return _.buf[0.._.cur_len];
    }

    /// Pushes a subdirectory onto the stack, saving the previous `dir_len`.
    fn pushDir(*_, dirname: []const u8): ! {
        if (_.depth_count >= WALKER_MAX_DEPTH) return WalkerSkipError.MaxDepthExceeded;
        const prev_dir_len = _.dir_len;
        const joined = _.joinEntry(dirname) ?? return WalkerSkipError.PathTooLong;
        _.parent_lens[_.depth_count] = prev_dir_len;
        _.depth_count += 1;
        _.dir_len = joined.len;
        _.cur_len = joined.len;
    }

    /// Pops the top directory, restoring the parent `dir_len`.
    fn popDir(*_): bool {
        if (_.depth_count == 0) return false;
        _.depth_count -= 1;
        _.dir_len = _.parent_lens[_.depth_count];
        _.cur_len = _.dir_len;
        return true;
    }
}
