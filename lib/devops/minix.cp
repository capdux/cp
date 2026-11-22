// Origin: Pure-CP Stdlib (Zero-Alloc MINIX v1/v2/v3 Filesystem Superblock, Inode & Dir Parser) | Created: 2026-11-22

// MINIX Layout & Size Constants
pub const MINIX_SUPER_OFFSET: usize = 1024;
pub const MINIX_BLOCK_SIZE: u16 = 1024;
pub const MINIX_V1_INODE_SIZE: usize = 32;
pub const MINIX_V2_INODE_SIZE: usize = 64;

// MINIX Superblock Magic Constants
pub const MINIX_V1_MAGIC: u16 = 0x137F;  // v1, 14-byte names
pub const MINIX_V1_MAGIC2: u16 = 0x138F; // v1, 30-byte names
pub const MINIX_V2_MAGIC: u16 = 0x2468;  // v2, 14-byte names
pub const MINIX_V2_MAGIC2: u16 = 0x2478; // v2, 30-byte names
pub const MINIX_V3_MAGIC: u16 = 0x4D5A;  // v3, 60-byte names

// Filesystem State Bits
pub const MINIX_VALID_FS: u16 = 0x0001;
pub const MINIX_ERROR_FS: u16 = 0x0002;

// POSIX Inode Mode Masks
pub const S_IFMT: u16 = 0xF000;
pub const S_IFDIR: u16 = 0x4000;
pub const S_IFREG: u16 = 0x8000;
pub const S_IFLNK: u16 = 0xA000;

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn trimNul(s: []const u8): []const u8 {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == 0) {
            return s[0..i];
        }
        i += 1;
    }
    return s;
}

fn isV1V2Magic(m: u16): bool {
    return m == MINIX_V1_MAGIC || m == MINIX_V1_MAGIC2 || m == MINIX_V2_MAGIC || m == MINIX_V2_MAGIC2;
}

/// Unified parsed MINIX v1 / v2 / v3 superblock.
pub struct MinixSuperblock {
    version: u8,
    magic: u16,
    ninodes: u32,
    nzones: u32,
    imap_blocks: u16,
    zmap_blocks: u16,
    firstdatazone: u16,
    log_zone_size: u16,
    max_size: u32,
    block_size: u16,
    state: u16,
    name_len: u8,
    dir_entry_size: u8,

    /// Zone size in bytes (`block_size << log_zone_size`).
    fn zoneSizeBytes(): u32 {
        return (_.block_size as u32) << (_.log_zone_size as u5);
    }

    /// On-disk inode size in bytes (`32` for v1, `64` for v2/v3).
    fn inodeSizeBytes(): usize {
        if (_.version == 1) {
            return MINIX_V1_INODE_SIZE;
        }
        return MINIX_V2_INODE_SIZE;
    }

    /// Byte offset of the inode table (`(2 + imap_blocks + zmap_blocks) * block_size`).
    fn inodeTableByteOffset(): u64 {
        const blocks = (2 as u64) + (_.imap_blocks as u64) + (_.zmap_blocks as u64);
        return blocks * (_.block_size as u64);
    }

    /// Converts a zone index to its byte offset in the image (`zone * zoneSizeBytes()`).
    fn zoneToByteOffset(zone: u32): u64 {
        return (zone as u64) * (_.zoneSizeBytes() as u64);
    }

    /// Returns `true` if `MINIX_VALID_FS` is set and `MINIX_ERROR_FS` is clear.
    fn isClean(): bool {
        return (_.state & MINIX_VALID_FS) != 0 && (_.state & MINIX_ERROR_FS) == 0;
    }
}

/// Returns `true` if `sb_bytes` (superblock buffer starting at byte offset 1024) contains a valid MINIX v1/v2/v3 magic.
pub fn isMinixSuperblock(sb_bytes: []const u8): bool {
    if (sb_bytes.len >= 32 && readU16Le(sb_bytes, 24) == MINIX_V3_MAGIC) {
        return true;
    }
    if (sb_bytes.len >= 24 && isV1V2Magic(readU16Le(sb_bytes, 16))) {
        return true;
    }
    return false;
}

/// Parses and strictly validates a MINIX v1, v2, or v3 superblock:
/// - Recognizes all 5 MINIX magic numbers (`0x137F`, `0x138F`, `0x2468`, `0x2478`, `0x4D5A`)
/// - Validates `ninodes > 0`, `nzones > 0`, `log_zone_size <= 16`
/// - For v3, validates `block_size >= 1024` and a power of 2
pub fn parseMinixSuperblock(sb_bytes: []const u8): MinixSuperblock? {
    // Try MINIX v3 first (magic 0x4D5A at offset 24)
    if (sb_bytes.len >= 32 && readU16Le(sb_bytes, 24) == MINIX_V3_MAGIC) {
        const ninodes = readU32Le(sb_bytes, 0);
        const imap_blocks = readU16Le(sb_bytes, 6);
        const zmap_blocks = readU16Le(sb_bytes, 8);
        const firstdatazone = readU16Le(sb_bytes, 10);
        const log_zone_size = readU16Le(sb_bytes, 12);
        const max_size = readU32Le(sb_bytes, 16);
        const nzones = readU32Le(sb_bytes, 20);
        const block_size = readU16Le(sb_bytes, 28);

        if (ninodes == 0 || nzones == 0 || log_zone_size > 16) {
            return null;
        }
        if (block_size < MINIX_BLOCK_SIZE || (block_size & (block_size - 1)) != 0) {
            return null;
        }
        return MinixSuperblock{
            .version = 3,
            .magic = MINIX_V3_MAGIC,
            .ninodes = ninodes,
            .nzones = nzones,
            .imap_blocks = imap_blocks,
            .zmap_blocks = zmap_blocks,
            .firstdatazone = firstdatazone,
            .log_zone_size = log_zone_size,
            .max_size = max_size,
            .block_size = block_size,
            .state = MINIX_VALID_FS,
            .name_len = 60,
            .dir_entry_size = 64,
        };
    }

    // MINIX v1 / v2 (magic at offset 16)
    if (sb_bytes.len < 24) {
        return null;
    }
    const magic = readU16Le(sb_bytes, 16);
    if (!isV1V2Magic(magic)) {
        return null;
    }

    const ninodes = readU16Le(sb_bytes, 0) as u32;
    const nzones_v1 = readU16Le(sb_bytes, 2) as u32;
    const imap_blocks = readU16Le(sb_bytes, 4);
    const zmap_blocks = readU16Le(sb_bytes, 6);
    const firstdatazone = readU16Le(sb_bytes, 8);
    const log_zone_size = readU16Le(sb_bytes, 10);
    const max_size = readU32Le(sb_bytes, 12);
    const state = readU16Le(sb_bytes, 18);
    const nzones_v2 = readU32Le(sb_bytes, 20);

    let version: u8 = 1;
    let nzones = nzones_v1;
    let name_len: u8 = 14;
    let dir_entry_size: u8 = 16;

    if (magic == MINIX_V1_MAGIC) {
        version = 1;
        nzones = nzones_v1;
        name_len = 14;
        dir_entry_size = 16;
    } else if (magic == MINIX_V1_MAGIC2) {
        version = 1;
        nzones = nzones_v1;
        name_len = 30;
        dir_entry_size = 32;
    } else if (magic == MINIX_V2_MAGIC) {
        version = 2;
        nzones = nzones_v2;
        name_len = 14;
        dir_entry_size = 16;
    } else {
        version = 2;
        nzones = nzones_v2;
        name_len = 30;
        dir_entry_size = 32;
    }

    if (ninodes == 0 || nzones == 0 || log_zone_size > 16) {
        return null;
    }

    return MinixSuperblock{
        .version = version,
        .magic = magic,
        .ninodes = ninodes,
        .nzones = nzones,
        .imap_blocks = imap_blocks,
        .zmap_blocks = zmap_blocks,
        .firstdatazone = firstdatazone,
        .log_zone_size = log_zone_size,
        .max_size = max_size,
        .block_size = MINIX_BLOCK_SIZE,
        .state = state,
        .name_len = name_len,
        .dir_entry_size = dir_entry_size,
    };
}

/// Unified MINIX v1 (32B) and v2/v3 (64B) inode representation.
pub struct MinixInode {
    mode: u16,
    nlinks: u16,
    uid: u16,
    gid: u16,
    size: u32,
    atime: u32,
    mtime: u32,
    ctime: u32,
    direct_zones: [7]u32,
    indirect_zone: u32,
    double_indirect: u32,
    triple_indirect: u32,

    fn isDir(): bool {
        return (_.mode & S_IFMT) == S_IFDIR;
    }

    fn isRegular(): bool {
        return (_.mode & S_IFMT) == S_IFREG;
    }

    fn isSymlink(): bool {
        return (_.mode & S_IFMT) == S_IFLNK;
    }

    /// Lower 12 POSIX permission & special bits (`mode & 0x0FFF`).
    fn permissions(): u16 {
        return _.mode & 0x0FFF;
    }
}

/// Parses a 32-byte MINIX v1 inode (`struct minix_inode`).
pub fn parseMinixV1Inode(bytes32: []const u8): MinixInode? {
    if (bytes32.len < MINIX_V1_INODE_SIZE) {
        return null;
    }
    const mode = readU16Le(bytes32, 0);
    const uid = readU16Le(bytes32, 2);
    const size = readU32Le(bytes32, 4);
    const mtime = readU32Le(bytes32, 8);
    const gid = bytes32[12] as u16;
    const nlinks = bytes32[13] as u16;

    let direct: [7]u32 = [0; 7];
    let i: usize = 0;
    while (i < 7) {
        direct[i] = readU16Le(bytes32, 14 + i * 2) as u32;
        i += 1;
    }
    const indirect = readU16Le(bytes32, 28) as u32;
    const dbl_indirect = readU16Le(bytes32, 30) as u32;

    return MinixInode{
        .mode = mode,
        .nlinks = nlinks,
        .uid = uid,
        .gid = gid,
        .size = size,
        .atime = mtime,
        .mtime = mtime,
        .ctime = mtime,
        .direct_zones = direct,
        .indirect_zone = indirect,
        .double_indirect = dbl_indirect,
        .triple_indirect = 0,
    };
}

/// Parses a 64-byte MINIX v2/v3 inode (`struct minix2_inode`).
pub fn parseMinixV2Inode(bytes64: []const u8): MinixInode? {
    if (bytes64.len < MINIX_V2_INODE_SIZE) {
        return null;
    }
    const mode = readU16Le(bytes64, 0);
    const nlinks = readU16Le(bytes64, 2);
    const uid = readU16Le(bytes64, 4);
    const gid = readU16Le(bytes64, 6);
    const size = readU32Le(bytes64, 8);
    const atime = readU32Le(bytes64, 12);
    const mtime = readU32Le(bytes64, 16);
    const ctime = readU32Le(bytes64, 20);

    let direct: [7]u32 = [0; 7];
    let i: usize = 0;
    while (i < 7) {
        direct[i] = readU32Le(bytes64, 24 + i * 4);
        i += 1;
    }
    const indirect = readU32Le(bytes64, 52);
    const dbl_indirect = readU32Le(bytes64, 56);
    const tri_indirect = readU32Le(bytes64, 60);

    return MinixInode{
        .mode = mode,
        .nlinks = nlinks,
        .uid = uid,
        .gid = gid,
        .size = size,
        .atime = atime,
        .mtime = mtime,
        .ctime = ctime,
        .direct_zones = direct,
        .indirect_zone = indirect,
        .double_indirect = dbl_indirect,
        .triple_indirect = tri_indirect,
    };
}

/// Single parsed MINIX directory entry (zero-copy NUL-trimmed `name` slice).
pub struct MinixDirEntry {
    inode: u32,
    name: []const u8,
}

/// Zero-copy fixed-width directory block iterator (16B/32B for v1/v2 with `u16` inode, 64B for v3 with `u32` inode).
/// Automatically skips unused slots where `inode == 0`.
pub struct MinixDirIter {
    data: []const u8,
    pos: usize,
    entry_size: usize,
    is_v3: bool,

    pub fn init(sb: MinixSuperblock, dir_bytes: []const u8): MinixDirIter {
        return MinixDirIter{
            .data = dir_bytes,
            .pos = 0,
            .entry_size = sb.dir_entry_size as usize,
            .is_v3 = sb.version == 3,
        };
    }

    fn next(*_): MinixDirEntry? {
        if (_.entry_size < 4) {
            return null;
        }
        while (_.data.len - _.pos >= _.entry_size) {
            const off = _.pos;
            _.pos += _.entry_size;
            let ino: u32 = 0;
            let name_start: usize = off + 2;
            if (_.is_v3) {
                ino = readU32Le(_.data, off);
                name_start = off + 4;
            } else {
                ino = readU16Le(_.data, off) as u32;
                name_start = off + 2;
            }
            if (ino == 0) {
                continue;
            }
            const name = trimNul(_.data[name_start..(off + _.entry_size)]);
            return MinixDirEntry{
                .inode = ino,
                .name = name,
            };
        }
        return null;
    }
}

/// Creates a zero-copy directory iterator matching the superblock's version and `dir_entry_size`.
pub fn iterMinixDir(sb: MinixSuperblock, dir_bytes: []const u8): MinixDirIter {
    return MinixDirIter.init(sb, dir_bytes);
}
