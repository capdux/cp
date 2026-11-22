// Source: Absorbed from /data/dev/bun/src/runtime/node/node_fs_constant.cp | Synced: 2026-11-22T15:51:00Z
// ============================================================================
// lib/sys/fsConstants.cp — Node.js fs.constants, CopyfileFlags & POSIX FileMode
// ============================================================================
// Pure-CP, zero-heap-allocation representation of Node.js `fs.constants`
// (copyFile flags, POSIX S_IF* file types, S_IR*/S_IW*/S_IX* permission bits,
// UV_FS_O_FILEMAP) with safe symbolic `ls -l` mode formatting & octal parsing.

// --- File Copy Constants (fs.copyFile / fs.constants.COPYFILE_*) ---
pub const COPYFILE_EXCL: u32 = 1;
pub const COPYFILE_FICLONE: u32 = 2;
pub const COPYFILE_FICLONE_FORCE: u32 = 4;
pub const COPYFILE_VALID_MASK: u32 = COPYFILE_EXCL | COPYFILE_FICLONE | COPYFILE_FICLONE_FORCE;

// --- libuv Windows Memory Mapping Open Flag ---
pub const UV_FS_O_FILEMAP: u32 = 536870912;

// --- POSIX File Type Masks (S_IF* canonical octal values) ---
pub const FS_IFMT: u32 = 0o170000;
pub const FS_IFIFO: u32 = 0o010000;
pub const FS_IFCHR: u32 = 0o020000;
pub const FS_IFDIR: u32 = 0o040000;
pub const FS_IFBLK: u32 = 0o060000;
pub const FS_IFREG: u32 = 0o100000;
pub const FS_IFLNK: u32 = 0o120000;
pub const FS_IFSOCK: u32 = 0o140000;

// --- POSIX Permission Mode Bits (S_IR* / S_IW* / S_IX* / special bits) ---
pub const S_ISUID: u32 = 0o4000;
pub const S_ISGID: u32 = 0o2000;
pub const S_ISVTX: u32 = 0o1000;

pub const S_IRWXU: u32 = 0o0700;
pub const S_IRUSR: u32 = 0o0400;
pub const S_IWUSR: u32 = 0o0200;
pub const S_IXUSR: u32 = 0o0100;

pub const S_IRWXG: u32 = 0o0070;
pub const S_IRGRP: u32 = 0o0040;
pub const S_IWGRP: u32 = 0o0020;
pub const S_IXGRP: u32 = 0o0010;

pub const S_IRWXO: u32 = 0o0007;
pub const S_IROTH: u32 = 0o0004;
pub const S_IWOTH: u32 = 0o0002;
pub const S_IXOTH: u32 = 0o0001;

pub const FS_PERM_MASK: u32 = 0o7777;

/// Strongly-typed `fs.copyFile` flag bitset (mirrors Bun `node_fs_constant.Copyfile`).
pub struct CopyfileFlags {
    bits: u32 = 0,

    static fn none(): CopyfileFlags {
        return CopyfileFlags{ bits: 0 };
    }

    static fn fromBits(raw: u32): CopyfileFlags {
        return CopyfileFlags{ bits: raw };
    }

    /// Returns true if no unknown bits outside `COPYFILE_VALID_MASK` are set.
    fn isValid(): bool {
        return (_.bits & (~COPYFILE_VALID_MASK)) == 0;
    }

    fn shouldntOverwrite(): bool {
        return (_.bits & COPYFILE_EXCL) != 0;
    }

    fn isForceClone(): bool {
        return (_.bits & COPYFILE_FICLONE_FORCE) != 0;
    }

    fn wantsClone(): bool {
        return (_.bits & (COPYFILE_FICLONE | COPYFILE_FICLONE_FORCE)) != 0;
    }

    fn withExclusive(): CopyfileFlags {
        return CopyfileFlags{ bits: _.bits | COPYFILE_EXCL };
    }

    fn withClone(): CopyfileFlags {
        return CopyfileFlags{ bits: _.bits | COPYFILE_FICLONE };
    }

    fn withForceClone(): CopyfileFlags {
        return CopyfileFlags{ bits: _.bits | COPYFILE_FICLONE_FORCE };
    }
}

/// Zero-allocation POSIX `st_mode` inspector, umask applier, symbolic formatter & octal parser.
pub struct FileMode {
    raw: u32 = 0,

    static fn init(mode: u32): FileMode {
        return FileMode{ raw: mode };
    }

    static fn fromTypeAndPerms(file_type: u32, perms: u32): FileMode {
        return FileMode{ raw: (file_type & FS_IFMT) | (perms & FS_PERM_MASK) };
    }

    fn fileTypeBits(): u32 {
        return _.raw & FS_IFMT;
    }

    fn permBits(): u32 {
        return _.raw & FS_PERM_MASK;
    }

    fn rwxBits(): u32 {
        return _.raw & 0o777;
    }

    fn isReg(): bool {
        return _.fileTypeBits() == FS_IFREG;
    }

    fn isDir(): bool {
        return _.fileTypeBits() == FS_IFDIR;
    }

    fn isSymlink(): bool {
        return _.fileTypeBits() == FS_IFLNK;
    }

    fn isFifo(): bool {
        return _.fileTypeBits() == FS_IFIFO;
    }

    fn isSocket(): bool {
        return _.fileTypeBits() == FS_IFSOCK;
    }

    fn isCharDevice(): bool {
        return _.fileTypeBits() == FS_IFCHR;
    }

    fn isBlockDevice(): bool {
        return _.fileTypeBits() == FS_IFBLK;
    }

    fn typeChar(): u8 {
        const ft = _.fileTypeBits();
        if (ft == FS_IFREG) return '-';
        if (ft == FS_IFDIR) return 'd';
        if (ft == FS_IFLNK) return 'l';
        if (ft == FS_IFIFO) return 'p';
        if (ft == FS_IFSOCK) return 's';
        if (ft == FS_IFCHR) return 'c';
        if (ft == FS_IFBLK) return 'b';
        return '?';
    }

    fn canOwnerRead(): bool { return (_.raw & S_IRUSR) != 0; }
    fn canOwnerWrite(): bool { return (_.raw & S_IWUSR) != 0; }
    fn canOwnerExec(): bool { return (_.raw & S_IXUSR) != 0; }

    fn canGroupRead(): bool { return (_.raw & S_IRGRP) != 0; }
    fn canGroupWrite(): bool { return (_.raw & S_IWGRP) != 0; }
    fn canGroupExec(): bool { return (_.raw & S_IXGRP) != 0; }

    fn canOtherRead(): bool { return (_.raw & S_IROTH) != 0; }
    fn canOtherWrite(): bool { return (_.raw & S_IWOTH) != 0; }
    fn canOtherExec(): bool { return (_.raw & S_IXOTH) != 0; }

    fn hasSetuid(): bool { return (_.raw & S_ISUID) != 0; }
    fn hasSetgid(): bool { return (_.raw & S_ISGID) != 0; }
    fn hasSticky(): bool { return (_.raw & S_ISVTX) != 0; }

    /// Applies a POSIX umask to the permission bits while preserving file type bits.
    fn applyUmask(umask: u32): FileMode {
        const ft = _.fileTypeBits();
        const perms = _.permBits() & (~(umask & 0o777));
        return FileMode{ raw: ft | perms };
    }

    /// Formats the 10-character POSIX `ls -l` mode string (e.g. `drwxr-xr-x`, `-rwsr-xr-t`)
    /// into caller-provided `out` buffer. Returns empty slice if `out.len < 10`.
    fn formatSymbolic(out: []u8): []const u8 {
        if (out.len < 10) return out[0..0];
        out[0] = _.typeChar();
        out[1] = if (_.canOwnerRead()) 'r' else '-';
        out[2] = if (_.canOwnerWrite()) 'w' else '-';
        out[3] = if (_.hasSetuid())
            (if (_.canOwnerExec()) 's' else 'S')
        else
            (if (_.canOwnerExec()) 'x' else '-');

        out[4] = if (_.canGroupRead()) 'r' else '-';
        out[5] = if (_.canGroupWrite()) 'w' else '-';
        out[6] = if (_.hasSetgid())
            (if (_.canGroupExec()) 's' else 'S')
        else
            (if (_.canGroupExec()) 'x' else '-');

        out[7] = if (_.canOtherRead()) 'r' else '-';
        out[8] = if (_.canOtherWrite()) 'w' else '-';
        out[9] = if (_.hasSticky())
            (if (_.canOtherExec()) 't' else 'T')
        else
            (if (_.canOtherExec()) 'x' else '-');

        return out[0..10];
    }

    /// Parses an octal permission string such as `"0755"`, `"755"`, `"0o644"`, or `"4755"`.
    /// Rejects empty strings, non-octal digits, or values exceeding `0o7777`.
    static fn parseOctal(input: []const u8): u32? {
        if (input.len == 0) return null;
        let idx: usize = 0;
        if (input.len >= 2 && input[0] == '0' && (input[1] == 'o' || input[1] == 'O')) {
            idx = 2;
            if (idx >= input.len) return null;
        }
        let acc: u32 = 0;
        let digits: usize = 0;
        while (idx < input.len) {
            const c = input[idx];
            if (c < '0' || c > '7') return null;
            acc = (acc << 3) | (@as(u32, c - '0'));
            if (acc > FS_PERM_MASK) return null;
            digits += 1;
            idx += 1;
        }
        if (digits == 0) return null;
        return acc;
    }
}
