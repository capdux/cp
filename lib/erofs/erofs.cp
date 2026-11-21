// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Linux EROFS Filesystem Parser)

pub const EROFS_SUPER_OFFSET: usize = 1024;
pub const EROFS_SUPER_SIZE: usize = 128;
pub const EROFS_SUPER_MAGIC: u32 = 0xE0F5E1E2;
pub const EROFS_INODE_COMPACT_SIZE: usize = 32;
pub const EROFS_INODE_EXTENDED_SIZE: usize = 64;
pub const EROFS_DIRENT_SIZE: usize = 12;

// Feature Compat Flags
pub const FEATURE_COMPAT_SB_CHKSUM: u32 = 0x00000001;

// Feature Incompat Flags
pub const FEATURE_INCOMPAT_ZERO_PADDING: u32 = 0x0001;
pub const FEATURE_INCOMPAT_COMPR_CFGS: u32 = 0x0002;
pub const FEATURE_INCOMPAT_BIG_PCLUSTER: u32 = 0x0002;
pub const FEATURE_INCOMPAT_CHUNKED_FILE: u32 = 0x0004;
pub const FEATURE_INCOMPAT_DEVICE_TABLE: u32 = 0x0008;
pub const FEATURE_INCOMPAT_ZTAILPACKING: u32 = 0x0010;
pub const FEATURE_INCOMPAT_FRAGMENTS: u32 = 0x0020;

// Inode Data Layout ((i_format >> 1) & 0x7)
pub const LAYOUT_FLAT_PLAIN: u8 = 0;
pub const LAYOUT_FLAT_COMPRESSION_LEGACY: u8 = 1;
pub const LAYOUT_FLAT_INLINE: u8 = 2;
pub const LAYOUT_FLAT_COMPRESSION: u8 = 3;
pub const LAYOUT_CHUNK_BASED: u8 = 4;

// Directory Entry File Types
pub const FT_UNKNOWN: u8 = 0;
pub const FT_REG_FILE: u8 = 1;
pub const FT_DIR: u8 = 2;
pub const FT_CHRDEV: u8 = 3;
pub const FT_BLKDEV: u8 = 4;
pub const FT_FIFO: u8 = 5;
pub const FT_SOCK: u8 = 6;
pub const FT_SYMLINK: u8 = 7;

// POSIX Mode Bits
pub const S_IFMT: u16 = 0xF000;
pub const S_IFREG: u16 = 0x8000;
pub const S_IFDIR: u16 = 0x4000;

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

fn readU64Le(buf: []const u8, off: usize): u64 {
    const lo = readU32Le(buf, off) as u64;
    const hi = readU32Le(buf, off + 4) as u64;
    return lo | (hi << 32);
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

/// Computes Linux EROFS superblock CRC32C using reflected Castagnoli polynomial 0x82F63B78,
/// initial value 0xFFFFFFFF, NO final bitwise inversion (`crc32c(~0, ...)`), and treating
/// bytes `[4..8]` (the superblock `checksum` field) as zero.
pub fn erofsCrc32c(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        let b = data[i];
        if (i >= 4 && i < 8) {
            b = 0;
        }
        crc ^= b as u32;
        let bit: u8 = 0;
        while (bit < 8) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0x82F63B78;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc;
}

/// Parsed Linux EROFS 128-byte Superblock (`erofs_super_block`).
pub struct ErofsSuperblock {
    checksum: u32,
    feature_compat: u32,
    blkszbits: u8,
    sb_extslots: u8,
    root_nid: u16,
    inos: u64,
    build_time: u64,
    build_time_nsec: u32,
    blocks: u32,
    meta_blkaddr: u32,
    xattr_blkaddr: u32,
    uuid: [16]u8,
    volume_name: [16]u8,
    feature_incompat: u32,

    /// Returns filesystem block size in bytes (`1 << blkszbits`).
    fn blockSize(): u32 {
        return (1 as u32) << (_.blkszbits as u5);
    }

    /// Returns true if `FEATURE_COMPAT_SB_CHKSUM` is enabled.
    fn hasChecksum(): bool {
        return (_.feature_compat & FEATURE_COMPAT_SB_CHKSUM) != 0;
    }

    /// Returns true if `FEATURE_INCOMPAT_CHUNKED_FILE` is enabled.
    fn hasChunkedFile(): bool {
        return (_.feature_incompat & FEATURE_INCOMPAT_CHUNKED_FILE) != 0;
    }

    /// Returns true if `FEATURE_INCOMPAT_FRAGMENTS` is enabled.
    fn hasFragments(): bool {
        return (_.feature_incompat & FEATURE_INCOMPAT_FRAGMENTS) != 0;
    }

    /// Returns the NUL-terminated volume label slice from `volume_name[0..16]`.
    fn volumeNameSlice(): []const u8 {
        let end: usize = 0;
        while (end < 16 && _.volume_name[end] != 0) {
            end += 1;
        }
        return _.volume_name[0..end];
    }

    /// Translates an inode NID to its byte offset in the EROFS image:
    /// `meta_blkaddr * blockSize() + nid * 32`.
    fn nidToOffset(nid: u64): u64 {
        return (_.meta_blkaddr as u64) * (_.blockSize() as u64) + nid * 32;
    }
}

/// Unified EROFS Inode view supporting both 32-byte Compact (`version == 0`)
/// and 64-byte Extended (`version == 1`) on-disk formats.
pub struct ErofsInode {
    is_extended: bool,
    data_layout: u8,
    xattr_icount: u16,
    mode: u16,
    nlink: u32,
    size: u64,
    uid: u32,
    gid: u32,
    ino: u32,
    raw_blkaddr: u32,
    mtime: u64,

    fn isCompressed(): bool {
        return _.data_layout == LAYOUT_FLAT_COMPRESSION_LEGACY || _.data_layout == LAYOUT_FLAT_COMPRESSION;
    }

    fn isInline(): bool {
        return _.data_layout == LAYOUT_FLAT_INLINE;
    }

    fn isRegular(): bool {
        return (_.mode & S_IFMT) == S_IFREG;
    }

    fn isDirectory(): bool {
        return (_.mode & S_IFMT) == S_IFDIR;
    }
}

/// Decoded EROFS Directory Entry (`erofs_dirent`) with zero-copy name slice.
pub struct ErofsDirent {
    nid: u64,
    nameoff: u16,
    file_type: u8,
    name: []const u8,
}

/// Zero-copy iterator over entries in a single EROFS directory block.
pub struct ErofsDirIter {
    block: []const u8,
    count: usize,
    name_area_start: usize,
    idx: usize,

    fn next(*_): ErofsDirent? {
        if (_.idx >= _.count) {
            return null;
        }

        const i = _.idx;
        const off = i * EROFS_DIRENT_SIZE;
        const nid = readU64Le(_.block, off);
        const nameoff = readU16Le(_.block, off + 8);
        const file_type = _.block[off + 10];

        const start = nameoff as usize;
        if (start < _.name_area_start || start > _.block.len) {
            _.idx = _.count;
            return null;
        }

        let end = _.block.len;
        if (i + 1 < _.count) {
            const next_off = readU16Le(_.block, (i + 1) * EROFS_DIRENT_SIZE + 8) as usize;
            if (next_off < start || next_off > _.block.len) {
                _.idx = _.count;
                return null;
            }
            end = next_off;
        }

        // Trim NUL padding (e.g. trailing zeroes in last entry of a block)
        let actual_end = start;
        while (actual_end < end && _.block[actual_end] != 0) {
            actual_end += 1;
        }

        _.idx = i + 1;
        return ErofsDirent {
            nid: nid,
            nameoff: nameoff,
            file_type: file_type,
            name: _.block[start..actual_end],
        };
    }
}

/// Fast magic and length check for an EROFS 128-byte superblock (`0xE0F5E1E2` LE at offset 0).
pub fn isErofsSuperblock(data: []const u8): bool {
    if (data.len < EROFS_SUPER_SIZE) {
        return false;
    }
    return readU32Le(data, 0) == EROFS_SUPER_MAGIC;
}

/// Parses and validates an EROFS superblock.
/// Strictly checks magic `0xE0F5E1E2`, `9 <= blkszbits <= 16`, and when
/// `FEATURE_COMPAT_SB_CHKSUM` is set, verifies `erofsCrc32c(sb_bytes) == checksum`.
pub fn parseErofsSuperblock(sb_bytes: []const u8): ErofsSuperblock? {
    if (!isErofsSuperblock(sb_bytes)) {
        return null;
    }

    const checksum = readU32Le(sb_bytes, 4);
    const feature_compat = readU32Le(sb_bytes, 8);
    const blkszbits = sb_bytes[12];
    if (blkszbits < 9 || blkszbits > 16) {
        return null;
    }

    if ((feature_compat & FEATURE_COMPAT_SB_CHKSUM) != 0) {
        if (erofsCrc32c(sb_bytes) != checksum) {
            return null;
        }
    }

    return ErofsSuperblock {
        checksum: checksum,
        feature_compat: feature_compat,
        blkszbits: blkszbits,
        sb_extslots: sb_bytes[13],
        root_nid: readU16Le(sb_bytes, 14),
        inos: readU64Le(sb_bytes, 16),
        build_time: readU64Le(sb_bytes, 24),
        build_time_nsec: readU32Le(sb_bytes, 32),
        blocks: readU32Le(sb_bytes, 36),
        meta_blkaddr: readU32Le(sb_bytes, 40),
        xattr_blkaddr: readU32Le(sb_bytes, 44),
        uuid: copy16(sb_bytes, 48),
        volume_name: copy16(sb_bytes, 64),
        feature_incompat: readU32Le(sb_bytes, 80),
    };
}

/// Parses either a 32-byte Compact (`version == 0`) or 64-byte Extended (`version == 1`) EROFS inode.
pub fn parseErofsInode(data: []const u8): ErofsInode? {
    if (data.len < EROFS_INODE_COMPACT_SIZE) {
        return null;
    }
    const i_format = readU16Le(data, 0);
    const version = (i_format & 0x0001) as u8;
    const data_layout = ((i_format >> 1) & 0x0007) as u8;
    const xattr_icount = readU16Le(data, 2);
    const mode = readU16Le(data, 4);

    if (version == 0) {
        return ErofsInode {
            is_extended: false,
            data_layout: data_layout,
            xattr_icount: xattr_icount,
            mode: mode,
            nlink: readU16Le(data, 6) as u32,
            size: readU32Le(data, 8) as u64,
            uid: readU16Le(data, 24) as u32,
            gid: readU16Le(data, 26) as u32,
            ino: readU32Le(data, 20),
            raw_blkaddr: readU32Le(data, 16),
            mtime: 0,
        };
    }

    if (data.len < EROFS_INODE_EXTENDED_SIZE) {
        return null;
    }
    return ErofsInode {
        is_extended: true,
        data_layout: data_layout,
        xattr_icount: xattr_icount,
        mode: mode,
        nlink: readU32Le(data, 44),
        size: readU64Le(data, 8),
        uid: readU32Le(data, 24),
        gid: readU32Le(data, 28),
        ino: readU32Le(data, 20),
        raw_blkaddr: readU32Le(data, 16),
        mtime: readU64Le(data, 32),
    };
}

/// Creates a zero-copy iterator over an EROFS directory block.
/// Validates that `dirent[0].nameoff` is a non-zero multiple of 12 within `block_bytes.len`.
pub fn iterErofsDirBlock(block_bytes: []const u8): ErofsDirIter? {
    if (block_bytes.len < EROFS_DIRENT_SIZE) {
        return null;
    }
    const nameoff0 = readU16Le(block_bytes, 8) as usize;
    if (nameoff0 < EROFS_DIRENT_SIZE || nameoff0 > block_bytes.len || (nameoff0 % EROFS_DIRENT_SIZE) != 0) {
        return null;
    }
    return ErofsDirIter {
        block: block_bytes,
        count: nameoff0 / EROFS_DIRENT_SIZE,
        name_area_start: nameoff0,
        idx: 0,
    };
}
