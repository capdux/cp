// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Linux ext2/ext3/ext4 Filesystem Parser)

pub const EXT4_SUPER_OFFSET: usize = 1024;
pub const EXT4_SUPER_SIZE: usize = 1024;
pub const EXT4_SUPER_MAGIC: u16 = 0xEF53;
pub const EXT4_EXTENT_MAGIC: u16 = 0xF30A;
pub const EXT4_ROOT_INO: u32 = 2;

// Superblock Feature Flags
pub const COMPAT_HAS_JOURNAL: u32 = 0x0004;
pub const INCOMPAT_FILETYPE: u32 = 0x0002;
pub const INCOMPAT_EXTENTS: u32 = 0x0040;
pub const INCOMPAT_64BIT: u32 = 0x0080;
pub const RO_COMPAT_SPARSE_SUPER: u32 = 0x0001;
pub const RO_COMPAT_LARGE_FILE: u32 = 0x0002;
pub const RO_COMPAT_METADATA_CSUM: u32 = 0x0400;

// Inode Flags and POSIX Mode Bits
pub const EXT4_EXTENTS_FL: u32 = 0x00080000;
pub const S_IFMT: u16 = 0xF000;
pub const S_IFREG: u16 = 0x8000;
pub const S_IFDIR: u16 = 0x4000;
pub const S_IFLNK: u16 = 0xA000;

// Directory Entry File Types
pub const FT_UNKNOWN: u8 = 0;
pub const FT_REG_FILE: u8 = 1;
pub const FT_DIR: u8 = 2;
pub const FT_CHRDEV: u8 = 3;
pub const FT_BLKDEV: u8 = 4;
pub const FT_FIFO: u8 = 5;
pub const FT_SOCK: u8 = 6;
pub const FT_SYMLINK: u8 = 7;

fn readU16Le(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return b0 | (b1 << 8);
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn copy16(src: []const u8, off: usize): [16]u8 {
    let out: [16]u8 = undefined;
    let i: usize = 0;
    while (i < 16) {
        out[i] = src[off + i];
        i += 1;
    }
    return out;
}

fn copy60(src: []const u8, off: usize): [60]u8 {
    let out: [60]u8 = undefined;
    let i: usize = 0;
    while (i < 60) {
        out[i] = src[off + i];
        i += 1;
    }
    return out;
}

/// Parsed Linux ext2 / ext3 / ext4 Superblock (1024 bytes).
pub struct Ext4Superblock {
    inodes_count: u32,
    blocks_count: u64,
    free_blocks_count: u64,
    free_inodes_count: u32,
    first_data_block: u32,
    block_size: u32,
    blocks_per_group: u32,
    inodes_per_group: u32,
    rev_level: u32,
    inode_size: u16,
    desc_size: u16,
    feature_compat: u32,
    feature_incompat: u32,
    feature_ro_compat: u32,
    uuid: [16]u8,
    volume_name: [16]u8,

    fn hasJournal(): bool {
        return (_.feature_compat & COMPAT_HAS_JOURNAL) != 0;
    }

    fn hasExtents(): bool {
        return (_.feature_incompat & INCOMPAT_EXTENTS) != 0;
    }

    fn is64Bit(): bool {
        return (_.feature_incompat & INCOMPAT_64BIT) != 0;
    }

    fn groupCount(): u64 {
        const first = _.first_data_block as u64;
        if (_.blocks_count <= first || _.blocks_per_group == 0) {
            return 0;
        }
        const bpg = _.blocks_per_group as u64;
        return ((_.blocks_count - first) + bpg - 1) / bpg;
    }

    /// Returns the NUL-terminated volume label slice from `volume_name[0..16]`.
    fn volumeNameSlice(): []const u8 {
        let end: usize = 0;
        while (end < 16 && _.volume_name[end] != 0) {
            end += 1;
        }
        return _.volume_name[0..end];
    }
}

/// Fast magic and size check for an ext2/ext3/ext4 superblock.
pub fn isExt4Superblock(sb_bytes: []const u8): bool {
    if (sb_bytes.len < EXT4_SUPER_SIZE) {
        return false;
    }
    return readU16Le(sb_bytes, 0x38) == EXT4_SUPER_MAGIC;
}

/// Strictly parses an ext2/ext3/ext4 superblock (`magic == 0xEF53`, `log_block_size <= 6`,
/// `blocks_per_group > 0`, `inodes_per_group > 0`).
pub fn parseExt4Superblock(sb_bytes: []const u8): Ext4Superblock? {
    if (sb_bytes.len < EXT4_SUPER_SIZE) {
        return null;
    }
    if (readU16Le(sb_bytes, 0x38) != EXT4_SUPER_MAGIC) {
        return null;
    }

    const log_block_size = readU32Le(sb_bytes, 0x18);
    if (log_block_size > 6) {
        return null;
    }
    const blocks_per_group = readU32Le(sb_bytes, 0x20);
    const inodes_per_group = readU32Le(sb_bytes, 0x28);
    if (blocks_per_group == 0 || inodes_per_group == 0) {
        return null;
    }

    const block_size: u32 = (1024 as u32) << (log_block_size as u5);
    const rev_level = readU32Le(sb_bytes, 0x4C);
    const feature_compat = readU32Le(sb_bytes, 0x5C);
    const feature_incompat = readU32Le(sb_bytes, 0x60);
    const feature_ro_compat = readU32Le(sb_bytes, 0x64);
    const is_64 = (feature_incompat & INCOMPAT_64BIT) != 0;

    let inode_size: u16 = 128;
    if (rev_level >= 1) {
        const raw_isize = readU16Le(sb_bytes, 0x58);
        if (raw_isize < 128) {
            return null;
        }
        inode_size = raw_isize;
    }

    let desc_size: u16 = 32;
    if (is_64) {
        const raw_dsize = readU16Le(sb_bytes, 0xFE);
        desc_size = if (raw_dsize >= 64) raw_dsize else 64;
    }

    const blocks_lo = readU32Le(sb_bytes, 0x04) as u64;
    const blocks_hi: u64 = if (is_64) (readU32Le(sb_bytes, 0x150) as u64) else 0;
    const free_lo = readU32Le(sb_bytes, 0x0C) as u64;
    const free_hi: u64 = if (is_64) (readU32Le(sb_bytes, 0x158) as u64) else 0;

    return Ext4Superblock{
        .inodes_count = readU32Le(sb_bytes, 0x00),
        .blocks_count = blocks_lo | (blocks_hi << 32),
        .free_blocks_count = free_lo | (free_hi << 32),
        .free_inodes_count = readU32Le(sb_bytes, 0x10),
        .first_data_block = readU32Le(sb_bytes, 0x14),
        .block_size = block_size,
        .blocks_per_group = blocks_per_group,
        .inodes_per_group = inodes_per_group,
        .rev_level = rev_level,
        .inode_size = inode_size,
        .desc_size = desc_size,
        .feature_compat = feature_compat,
        .feature_incompat = feature_incompat,
        .feature_ro_compat = feature_ro_compat,
        .uuid = copy16(sb_bytes, 0x68),
        .volume_name = copy16(sb_bytes, 0x78),
    };
}

/// Parsed ext2/ext3/ext4 Inode (128+ bytes).
pub struct Ext4Inode {
    mode: u16,
    uid: u32,
    gid: u32,
    size: u64,
    atime: u32,
    ctime: u32,
    mtime: u32,
    links_count: u16,
    flags: u32,
    block_area: [60]u8,

    fn isRegular(): bool {
        return (_.mode & S_IFMT) == S_IFREG;
    }

    fn isDirectory(): bool {
        return (_.mode & S_IFMT) == S_IFDIR;
    }

    fn isSymlink(): bool {
        return (_.mode & S_IFMT) == S_IFLNK;
    }

    fn usesExtents(): bool {
        return (_.flags & EXT4_EXTENTS_FL) != 0;
    }

    fn permissions(): u16 {
        return _.mode & 0x0FFF;
    }
}

/// Parses an ext2/ext3/ext4 Inode from a buffer of at least 128 bytes.
pub fn parseExt4Inode(bytes: []const u8): Ext4Inode? {
    if (bytes.len < 128) {
        return null;
    }
    const mode = readU16Le(bytes, 0x00);
    const uid_lo = readU16Le(bytes, 0x02) as u32;
    const uid_hi = readU16Le(bytes, 0x78) as u32;
    const gid_lo = readU16Le(bytes, 0x18) as u32;
    const gid_hi = readU16Le(bytes, 0x7A) as u32;

    const size_lo = readU32Le(bytes, 0x04) as u64;
    const is_reg = (mode & S_IFMT) == S_IFREG;
    const size_hi: u64 = if (is_reg) (readU32Le(bytes, 0x6C) as u64) else 0;

    return Ext4Inode{
        .mode = mode,
        .uid = uid_lo | (uid_hi << 16),
        .gid = gid_lo | (gid_hi << 16),
        .size = size_lo | (size_hi << 32),
        .atime = readU32Le(bytes, 0x08),
        .ctime = readU32Le(bytes, 0x0C),
        .mtime = readU32Le(bytes, 0x10),
        .links_count = readU16Le(bytes, 0x1A),
        .flags = readU32Le(bytes, 0x20),
        .block_area = copy60(bytes, 0x28),
    };
}

/// Parsed 12-byte ext4 Extent Tree Node Header (`eh_magic == 0xF30A`).
pub struct Ext4ExtentHeader {
    entries: u16,
    max_entries: u16,
    depth: u16,
    generation: u32,
}

/// Parses an ext4 Extent Header from a 60-byte `block_area` or extent block.
pub fn parseExt4ExtentHeader(block_area: []const u8): Ext4ExtentHeader? {
    if (block_area.len < 12) {
        return null;
    }
    if (readU16Le(block_area, 0) != EXT4_EXTENT_MAGIC) {
        return null;
    }
    const entries = readU16Le(block_area, 2);
    const max_entries = readU16Le(block_area, 4);
    if (entries > max_entries) {
        return null;
    }
    return Ext4ExtentHeader{
        .entries = entries,
        .max_entries = max_entries,
        .depth = readU16Le(block_area, 6),
        .generation = readU32Le(block_area, 8),
    };
}

/// Parsed 12-byte ext4 Leaf Extent (48-bit physical block + uninitialized flag).
pub struct Ext4Extent {
    logical_block: u32,
    raw_len: u16,
    physical_block: u64,

    fn isUninitialized(): bool {
        return _.raw_len > 32768;
    }

    fn blockLen(): u16 {
        if (_.raw_len > 32768) {
            return _.raw_len - 32768;
        }
        return _.raw_len;
    }
}

/// Zero-copy iterator over leaf extents following an `Ext4ExtentHeader` (`depth == 0`).
pub struct Ext4ExtentIter {
    data: []const u8,
    offset: usize,
    remaining: u16,

    fn next(*_): Ext4Extent? {
        if (_.remaining == 0 || _.offset + 12 > _.data.len) {
            return null;
        }
        const off = _.offset;
        const logical_block = readU32Le(_.data, off);
        const raw_len = readU16Le(_.data, off + 4);
        const start_hi = readU16Le(_.data, off + 6) as u64;
        const start_lo = readU32Le(_.data, off + 8) as u64;
        _.offset += 12;
        _.remaining -= 1;
        return Ext4Extent{
            .logical_block = logical_block,
            .raw_len = raw_len,
            .physical_block = (start_hi << 32) | start_lo,
        };
    }
}

/// Creates a leaf extent iterator from a 60-byte `block_area` or extent block (`depth == 0`).
pub fn iterExt4Extents(block_area: []const u8): Ext4ExtentIter? {
    const hdr = parseExt4ExtentHeader(block_area) ?? return null;
    if (hdr.depth != 0) {
        return null;
    }
    const needed: usize = (12 as usize) + (hdr.entries as usize) * 12;
    if (needed > block_area.len) {
        return null;
    }
    return Ext4ExtentIter{
        .data = block_area,
        .offset = 12,
        .remaining = hdr.entries,
    };
}

/// Single parsed ext4 linear directory entry (`ext4_dir_entry_2`).
pub struct Ext4DirEntry {
    inode: u32,
    rec_len: u16,
    file_type: u8,
    name: []const u8,
}

/// Zero-copy iterator over an ext4 linear directory block, skipping `inode == 0` holes
/// and validating `rec_len >= 8`, 4-byte alignment, and bounds.
pub struct Ext4DirIter {
    data: []const u8,
    offset: usize,
    failed: bool,

    fn init(dir_block: []const u8): Ext4DirIter {
        return Ext4DirIter{
            .data = dir_block,
            .offset = 0,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): Ext4DirEntry? {
        while (!_.failed && _.offset < _.data.len) {
            if (_.offset + 8 > _.data.len) {
                _.failed = true;
                return null;
            }
            const inode = readU32Le(_.data, _.offset);
            const rec_len = readU16Le(_.data, _.offset + 4);
            const rlen = rec_len as usize;
            if (rlen < 8 || (rlen & 3) != 0 || _.offset + rlen > _.data.len) {
                _.failed = true;
                return null;
            }
            const name_len = _.data[_.offset + 6] as usize;
            const file_type = _.data[_.offset + 7];
            if (8 + name_len > rlen) {
                _.failed = true;
                return null;
            }
            const name = _.data[_.offset + 8 .. _.offset + 8 + name_len];
            _.offset += rlen;

            if (inode == 0) {
                continue;
            }
            return Ext4DirEntry{
                .inode = inode,
                .rec_len = rec_len,
                .file_type = file_type,
                .name = name,
            };
        }
        return null;
    }
}
