// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Linux F2FS Superblock, Checkpoint & CRC32 Parser)

// ---------------------------------------------------------------------------
// F2FS Protocol Constants (Linux include/linux/f2fs_fs.h)
// ---------------------------------------------------------------------------

pub const F2FS_SUPER_OFFSET: usize = 1024;
pub const F2FS_SUPER_MAGIC: u32 = 0xF2F52010;
pub const F2FS_BLKSIZE: usize = 4096;
pub const F2FS_SB_MIN_SIZE: usize = 1024;
pub const F2FS_CP_MIN_SIZE: usize = 192;
pub const F2FS_SB_FEATURE_OFFSET: usize = 2180;
pub const F2FS_SB_CRC_OFFSET: usize = 3068;

// Superblock Feature Flags
pub const FEAT_ENCRYPT: u32 = 0x0001;
pub const FEAT_BLKZONED: u32 = 0x0002;
pub const FEAT_ATOMIC_WRITE: u32 = 0x0004;
pub const FEAT_EXTRA_ATTR: u32 = 0x0008;
pub const FEAT_PRJQUOTA: u32 = 0x0010;
pub const FEAT_INODE_CHKSUM: u32 = 0x0020;
pub const FEAT_FLEXIBLE_INLINE_XATTR: u32 = 0x0040;
pub const FEAT_QUOTA_INO: u32 = 0x0080;
pub const FEAT_INODE_CRTIME: u32 = 0x0100;
pub const FEAT_LOST_FOUND: u32 = 0x0200;
pub const FEAT_VERITY: u32 = 0x0400;
pub const FEAT_SB_CHKSUM: u32 = 0x0800;
pub const FEAT_CASEFOLD: u32 = 0x1000;
pub const FEAT_COMPRESSION: u32 = 0x2000;

// Checkpoint Flags
pub const CP_UMOUNT_FLAG: u32 = 0x00000001;
pub const CP_ORPHAN_PRESENT_FLAG: u32 = 0x00000002;
pub const CP_COMPACT_SUM_FLAG: u32 = 0x00000004;
pub const CP_ERROR_FLAG: u32 = 0x00000008;
pub const CP_FSCK_FLAG: u32 = 0x00000010;
pub const CP_FASTBOOT_FLAG: u32 = 0x00000020;
pub const CP_CRC_RECOVERY_FLAG: u32 = 0x00000040;
pub const CP_TRIMMED_FLAG: u32 = 0x00000100;

// ---------------------------------------------------------------------------
// Internal Little-Endian & Shift Helpers
// ---------------------------------------------------------------------------

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

fn readBytes16(buf: []const u8, off: usize): [16]u8 {
    let out: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        out[i] = buf[off + i];
        i += 1;
    }
    return out;
}

fn readBytes512(buf: []const u8, off: usize): [512]u8 {
    let out: [512]u8 = [0; 512];
    let i: usize = 0;
    while (i < 512) {
        out[i] = buf[off + i];
        i += 1;
    }
    return out;
}

fn pow2U32(exp: u32): u32 {
    if (exp >= 32) {
        return 0;
    }
    let v: u32 = 1;
    let i: u32 = 0;
    while (i < exp) {
        v = v * 2;
        i += 1;
    }
    return v;
}

fn pow2U64(exp: u32): u64 {
    if (exp >= 64) {
        return 0;
    }
    let v: u64 = 1;
    let i: u32 = 0;
    while (i < exp) {
        v = v * 2;
        i += 1;
    }
    return v;
}

// ---------------------------------------------------------------------------
// F2FS CRC32 (Linux crc32_le: IEEE 802.3 poly 0xEDB88320, no final XOR)
// ---------------------------------------------------------------------------

const CRC32_IEEE_POLY: u32 = 0xEDB88320;

/// Computes Linux F2FS `crc32_le(seed, data)` using reflected polynomial `0xEDB88320`
/// with no final inversion. Passing `F2FS_SUPER_MAGIC` matches kernel `f2fs_crc32()`.
pub fn f2fsCrc32(seed: u32, data: []const u8): u32 {
    let crc: u32 = seed;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc ^ (data[i] as u32);
        let bit: usize = 0;
        while (bit < 8) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ CRC32_IEEE_POLY;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc;
}

// ---------------------------------------------------------------------------
// Data Models
// ---------------------------------------------------------------------------

/// Parsed Linux F2FS Superblock (`struct f2fs_super_block`).
pub struct F2fsSuperblock {
    major_ver: u16,
    minor_ver: u16,
    log_sectorsize: u32,
    log_sectors_per_block: u32,
    log_blocksize: u32,
    log_blocks_per_seg: u32,
    segs_per_sec: u32,
    secs_per_zone: u32,
    checksum_offset: u32,
    block_count: u64,
    section_count: u32,
    segment_count: u32,
    segment_count_ckpt: u32,
    segment_count_sit: u32,
    segment_count_nat: u32,
    segment_count_ssa: u32,
    segment_count_main: u32,
    segment0_blkaddr: u32,
    cp_blkaddr: u32,
    sit_blkaddr: u32,
    nat_blkaddr: u32,
    ssa_blkaddr: u32,
    main_blkaddr: u32,
    root_ino: u32,
    node_ino: u32,
    meta_ino: u32,
    uuid: [16]u8,
    volume_name_utf16le: [512]u8,
    feature: u32,

    fn blockSize(): u32 {
        return pow2U32(_.log_blocksize);
    }

    fn sectorSize(): u32 {
        return pow2U32(_.log_sectorsize);
    }

    fn segmentSizeBytes(): u64 {
        return pow2U64(_.log_blocks_per_seg) * (_.blockSize() as u64);
    }

    fn totalBytes(): u128 {
        return (_.block_count as u128) * (_.blockSize() as u128);
    }

    fn hasEncrypt(): bool {
        return (_.feature & FEAT_ENCRYPT) != 0;
    }

    fn hasCompression(): bool {
        return (_.feature & FEAT_COMPRESSION) != 0;
    }

    fn hasSbChecksum(): bool {
        return (_.feature & FEAT_SB_CHKSUM) != 0;
    }

    /// Decodes the NUL-terminated UTF-16LE volume name into ASCII bytes in `out`.
    /// Non-ASCII code units are replaced with `'?'`. Returns the number of bytes written.
    fn decodeVolumeNameAscii(out: []u8): usize {
        let written: usize = 0;
        let i: usize = 0;
        while (i + 1 < 512 && written < out.len) {
            const unit = readU16Le(_.volume_name_utf16le[0..], i);
            if (unit == 0) {
                break;
            }
            out[written] = if (unit < 0x80) (unit as u8) else 0x3F;
            written += 1;
            i += 2;
        }
        return written;
    }
}

/// Parsed Linux F2FS Checkpoint (`struct f2fs_checkpoint`).
pub struct F2fsCheckpoint {
    checkpoint_ver: u64,
    user_block_count: u64,
    valid_block_count: u64,
    rsvd_segment_count: u32,
    overprov_segment_count: u32,
    free_segment_count: u32,
    ckpt_flags: u32,
    cp_pack_total_block_count: u32,
    cp_pack_start_sum: u32,
    valid_node_count: u32,
    valid_inode_count: u32,
    next_free_nid: u32,
    checksum_offset: u32,

    fn isCleanUnmount(): bool {
        return (_.ckpt_flags & CP_UMOUNT_FLAG) != 0;
    }

    fn hasOrphans(): bool {
        return (_.ckpt_flags & CP_ORPHAN_PRESENT_FLAG) != 0;
    }

    fn isFastboot(): bool {
        return (_.ckpt_flags & CP_FASTBOOT_FLAG) != 0;
    }

    fn hasError(): bool {
        return (_.ckpt_flags & CP_ERROR_FLAG) != 0;
    }
}

// ---------------------------------------------------------------------------
// Parsing & Selection Functions
// ---------------------------------------------------------------------------

/// Returns `true` if `data` starts with the little-endian `F2FS_SUPER_MAGIC` (`0xF2F52010`).
pub fn isF2fsSuperblock(data: []const u8): bool {
    if (data.len < 4) {
        return false;
    }
    return readU32Le(data, 0) == F2FS_SUPER_MAGIC;
}

/// Parses and validates an F2FS Superblock slice (starting at `F2FS_SUPER_OFFSET`).
/// Validates `F2FS_SUPER_MAGIC`, `log_blocksize == 12` (4KB),
/// `log_sectorsize + log_sectors_per_block == log_blocksize`, and `FEAT_SB_CHKSUM` CRC32 if enabled.
pub fn parseF2fsSuperblock(sb_bytes: []const u8): F2fsSuperblock? {
    if (sb_bytes.len < F2FS_SB_MIN_SIZE) {
        return null;
    }
    if (readU32Le(sb_bytes, 0) != F2FS_SUPER_MAGIC) {
        return null;
    }

    const log_sectorsize = readU32Le(sb_bytes, 8);
    const log_sectors_per_block = readU32Le(sb_bytes, 12);
    const log_blocksize = readU32Le(sb_bytes, 16);
    if (log_blocksize != 12) {
        return null;
    }
    if (log_sectorsize + log_sectors_per_block != log_blocksize) {
        return null;
    }

    const log_blocks_per_seg = readU32Le(sb_bytes, 20);
    if (log_blocks_per_seg >= 32) {
        return null;
    }

    const checksum_offset = readU32Le(sb_bytes, 32);
    const feature: u32 = if (sb_bytes.len >= F2FS_SB_FEATURE_OFFSET + 4) readU32Le(sb_bytes, F2FS_SB_FEATURE_OFFSET) else 0;

    if ((feature & FEAT_SB_CHKSUM) != 0) {
        const cksum_off: usize = checksum_offset as usize;
        if (cksum_off < F2FS_SB_MIN_SIZE || cksum_off + 4 > sb_bytes.len) {
            return null;
        }
        const stored_crc = readU32Le(sb_bytes, cksum_off);
        const computed_crc = f2fsCrc32(F2FS_SUPER_MAGIC, sb_bytes[0..cksum_off]);
        if (stored_crc != computed_crc) {
            return null;
        }
    }

    return F2fsSuperblock {
        major_ver: readU16Le(sb_bytes, 4),
        minor_ver: readU16Le(sb_bytes, 6),
        log_sectorsize: log_sectorsize,
        log_sectors_per_block: log_sectors_per_block,
        log_blocksize: log_blocksize,
        log_blocks_per_seg: log_blocks_per_seg,
        segs_per_sec: readU32Le(sb_bytes, 24),
        secs_per_zone: readU32Le(sb_bytes, 28),
        checksum_offset: checksum_offset,
        block_count: readU64Le(sb_bytes, 36),
        section_count: readU32Le(sb_bytes, 44),
        segment_count: readU32Le(sb_bytes, 48),
        segment_count_ckpt: readU32Le(sb_bytes, 52),
        segment_count_sit: readU32Le(sb_bytes, 56),
        segment_count_nat: readU32Le(sb_bytes, 60),
        segment_count_ssa: readU32Le(sb_bytes, 64),
        segment_count_main: readU32Le(sb_bytes, 68),
        segment0_blkaddr: readU32Le(sb_bytes, 72),
        cp_blkaddr: readU32Le(sb_bytes, 76),
        sit_blkaddr: readU32Le(sb_bytes, 80),
        nat_blkaddr: readU32Le(sb_bytes, 84),
        ssa_blkaddr: readU32Le(sb_bytes, 88),
        main_blkaddr: readU32Le(sb_bytes, 92),
        root_ino: readU32Le(sb_bytes, 96),
        node_ino: readU32Le(sb_bytes, 100),
        meta_ino: readU32Le(sb_bytes, 104),
        uuid: readBytes16(sb_bytes, 108),
        volume_name_utf16le: readBytes512(sb_bytes, 124),
        feature: feature,
    };
}

/// Parses and validates an F2FS Checkpoint block (`checksum_offset >= 192`, bounds check, and `f2fsCrc32`).
pub fn parseF2fsCheckpoint(cp_block: []const u8): F2fsCheckpoint? {
    if (cp_block.len < F2FS_CP_MIN_SIZE + 4) {
        return null;
    }
    const checksum_offset = readU32Le(cp_block, 164);
    const cksum_off: usize = checksum_offset as usize;
    if (cksum_off < F2FS_CP_MIN_SIZE || cksum_off + 4 > cp_block.len) {
        return null;
    }
    const stored_crc = readU32Le(cp_block, cksum_off);
    const computed_crc = f2fsCrc32(F2FS_SUPER_MAGIC, cp_block[0..cksum_off]);
    if (stored_crc != computed_crc) {
        return null;
    }

    return F2fsCheckpoint {
        checkpoint_ver: readU64Le(cp_block, 0),
        user_block_count: readU64Le(cp_block, 8),
        valid_block_count: readU64Le(cp_block, 16),
        rsvd_segment_count: readU32Le(cp_block, 24),
        overprov_segment_count: readU32Le(cp_block, 28),
        free_segment_count: readU32Le(cp_block, 32),
        ckpt_flags: readU32Le(cp_block, 132),
        cp_pack_total_block_count: readU32Le(cp_block, 136),
        cp_pack_start_sum: readU32Le(cp_block, 140),
        valid_node_count: readU32Le(cp_block, 144),
        valid_inode_count: readU32Le(cp_block, 148),
        next_free_nid: readU32Le(cp_block, 152),
        checksum_offset: checksum_offset,
    };
}

/// Selects the active F2FS Checkpoint from the two redundant checkpoint packs.
/// Returns the valid checkpoint with the higher `checkpoint_ver` (or `null` if neither is valid).
pub fn selectActiveCheckpoint(c1: F2fsCheckpoint?, c2: F2fsCheckpoint?): F2fsCheckpoint? {
    if (a := c1) {
        if (b := c2) {
            if (b.checkpoint_ver > a.checkpoint_ver) {
                return b;
            }
            return a;
        }
        return a;
    }
    return c2;
}
