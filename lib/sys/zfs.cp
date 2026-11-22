// Origin: Pure-CP Stdlib (Zero-Alloc OpenZFS On-Disk Format Parser) | Created: 2026-11-22

pub const ZFS_UBERBLOCK_MAGIC: u64 = 0x00BAB10C;
pub const ZFS_UBERBLOCK_MAGIC_SWAPPED: u64 = 0x0CB1BA00;
pub const ZFS_UBERBLOCK_SIZE: usize = 1024;
pub const ZFS_BLKPTR_SIZE: usize = 128;
pub const ZFS_SECTOR_SHIFT: u6 = 9;
pub const ZFS_VDEV_LABEL_SIZE: usize = 262144;
pub const ZFS_VDEV_BOOT_OFFSET: u64 = 0x400000;

// Compression IDs (zio_compress)
pub const COMP_INHERIT: u8 = 0;
pub const COMP_ON: u8 = 1;
pub const COMP_OFF: u8 = 2;
pub const COMP_LZJB: u8 = 3;
pub const COMP_EMPTY: u8 = 4;
pub const COMP_GZIP_1: u8 = 5;
pub const COMP_LZ4: u8 = 15;
pub const COMP_ZSTD: u8 = 16;

// Checksum IDs (zio_checksum)
pub const CKSUM_INHERIT: u8 = 0;
pub const CKSUM_ON: u8 = 1;
pub const CKSUM_OFF: u8 = 2;
pub const CKSUM_FLETCHER_2: u8 = 6;
pub const CKSUM_FLETCHER_4: u8 = 7;
pub const CKSUM_SHA256: u8 = 8;
pub const CKSUM_SHA512: u8 = 11;
pub const CKSUM_SKEIN: u8 = 12;
pub const CKSUM_EDONR: u8 = 13;
pub const CKSUM_BLAKE3: u8 = 14;

fn readU32Le(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn readU64Le(buf: []const u8, off: usize): u64 {
    const lo = readU32Le(buf, off) as u64;
    const hi = readU32Le(buf, off + 4) as u64;
    return lo | (hi << 32);
}

fn readU64Be(buf: []const u8, off: usize): u64 {
    const hi = readU32Be(buf, off) as u64;
    const lo = readU32Be(buf, off + 4) as u64;
    return (hi << 32) | lo;
}

fn readU64Endian(buf: []const u8, off: usize, is_big_endian: bool): u64 {
    if (is_big_endian) {
        return readU64Be(buf, off);
    }
    return readU64Le(buf, off);
}

/// 256-bit OpenZFS checksum (`zio_cksum_t`: 4 x `u64`).
pub struct ZfsChecksum {
    words: [4]u64,
}

/// Computes OpenZFS Fletcher-4 256-bit checksum over little-endian 32-bit words.
pub fn zfsFletcher4Le(data: []const u8): ZfsChecksum {
    let a: u64 = 0;
    let b: u64 = 0;
    let c: u64 = 0;
    let d: u64 = 0;

    let off: usize = 0;
    while (off + 4 <= data.len) {
        const w = readU32Le(data, off) as u64;
        a +%= w;
        b +%= a;
        c +%= b;
        d +%= c;
        off += 4;
    }

    let out: [4]u64 = undefined;
    out[0] = a;
    out[1] = b;
    out[2] = c;
    out[3] = d;
    return ZfsChecksum { words: out };
}

/// Computes OpenZFS Fletcher-4 256-bit checksum over big-endian 32-bit words.
pub fn zfsFletcher4Be(data: []const u8): ZfsChecksum {
    let a: u64 = 0;
    let b: u64 = 0;
    let c: u64 = 0;
    let d: u64 = 0;

    let off: usize = 0;
    while (off + 4 <= data.len) {
        const w = readU32Be(data, off) as u64;
        a +%= w;
        b +%= a;
        c +%= b;
        d +%= c;
        off += 4;
    }

    let out: [4]u64 = undefined;
    out[0] = a;
    out[1] = b;
    out[2] = c;
    out[3] = d;
    return ZfsChecksum { words: out };
}

/// 16-byte OpenZFS Data Virtual Address (`dva_t`).
pub struct ZfsDva {
    vdev: u32,
    asize_sectors: u32,
    grid: u8,
    gang: bool,
    offset_sectors: u64,

    /// Converts sector offset to physical VDEV byte offset: `(offset_sectors << 9) + 0x400000` (4 MiB VDEV label offset).
    fn byteOffset(): u64 {
        return (_.offset_sectors << ZFS_SECTOR_SHIFT) + ZFS_VDEV_BOOT_OFFSET;
    }

    /// Returns allocated size in bytes (`asize_sectors << 9`).
    fn asizeBytes(): u64 {
        return (_.asize_sectors as u64) << ZFS_SECTOR_SHIFT;
    }

    /// Returns true if this DVA points to allocated space (`asize_sectors != 0`).
    fn isValid(): bool {
        return _.asize_sectors != 0;
    }
}

fn parseDva(buf: []const u8, off: usize, is_big_endian: bool): ZfsDva {
    const w0 = readU64Endian(buf, off, is_big_endian);
    const w1 = readU64Endian(buf, off + 8, is_big_endian);
    return ZfsDva {
        vdev: ((w0 >> 32) & 0xFFFFFFFF) as u32,
        asize_sectors: (w0 & 0x00FFFFFF) as u32,
        grid: ((w0 >> 24) & 0xFF) as u8,
        gang: (w1 >> 63) != 0,
        offset_sectors: w1 & 0x7FFFFFFFFFFFFFFF,
    };
}

/// 128-byte OpenZFS Block Pointer (`blkptr_t`).
pub struct ZfsBlkPtr {
    dvas: [3]ZfsDva,
    lsize_bytes: u32,
    psize_bytes: u32,
    compression: u8,
    is_embedded: bool,
    checksum_type: u8,
    dmu_type: u8,
    level: u8,
    is_encrypted: bool,
    is_dedup: bool,
    is_big_endian: bool,
    logical_birth_txg: u64,
    phys_birth_txg: u64,
    fill_count: u64,
    cksum: ZfsChecksum,

    /// Returns true when this block pointer represents an unallocated hole (`logical_birth_txg == 0 && !is_embedded`).
    fn isHole(): bool {
        return _.logical_birth_txg == 0 && !_.is_embedded;
    }
}

/// Parsed OpenZFS Uberblock (`uberblock_t`).
pub struct ZfsUberblock {
    is_big_endian: bool,
    version: u64,
    txg: u64,
    guid_sum: u64,
    timestamp: u64,
    rootbp: ZfsBlkPtr,
    software_version: u64,
}

/// Parses a 128-byte OpenZFS `blkptr_t` in the given byte order.
pub fn parseZfsBlkPtr(data: []const u8, is_big_endian: bool): ZfsBlkPtr? {
    if (data.len < ZFS_BLKPTR_SIZE) {
        return null;
    }

    let dvas: [3]ZfsDva = undefined;
    dvas[0] = parseDva(data, 0, is_big_endian);
    dvas[1] = parseDva(data, 16, is_big_endian);
    dvas[2] = parseDva(data, 32, is_big_endian);

    const prop = readU64Endian(data, 48, is_big_endian);
    const phys_birth_txg = readU64Endian(data, 72, is_big_endian);
    const logical_birth_txg = readU64Endian(data, 80, is_big_endian);
    const fill_count = readU64Endian(data, 88, is_big_endian);

    const is_embedded = ((prop >> 39) & 1) != 0;
    const is_hole = logical_birth_txg == 0 && !is_embedded;

    let lsize_bytes: u32 = 0;
    let psize_bytes: u32 = 0;
    let bp_is_be = is_big_endian;
    if (!is_hole) {
        const lsize_field = (prop & 0xFFFF) as u32;
        const psize_field = ((prop >> 16) & 0xFFFF) as u32;
        lsize_bytes = (lsize_field + 1) << 9;
        psize_bytes = (psize_field + 1) << 9;
        // In OpenZFS blk_prop bit 63: 1 = Little-Endian, 0 = Big-Endian
        bp_is_be = ((prop >> 63) & 1) == 0;
    }

    let cksum_words: [4]u64 = undefined;
    cksum_words[0] = readU64Endian(data, 96, is_big_endian);
    cksum_words[1] = readU64Endian(data, 104, is_big_endian);
    cksum_words[2] = readU64Endian(data, 112, is_big_endian);
    cksum_words[3] = readU64Endian(data, 120, is_big_endian);

    return ZfsBlkPtr {
        dvas: dvas,
        lsize_bytes: lsize_bytes,
        psize_bytes: psize_bytes,
        compression: ((prop >> 32) & 0x7F) as u8,
        is_embedded: is_embedded,
        checksum_type: ((prop >> 40) & 0xFF) as u8,
        dmu_type: ((prop >> 48) & 0xFF) as u8,
        level: ((prop >> 56) & 0x1F) as u8,
        is_encrypted: ((prop >> 61) & 1) != 0,
        is_dedup: ((prop >> 62) & 1) != 0,
        is_big_endian: bp_is_be,
        logical_birth_txg: logical_birth_txg,
        phys_birth_txg: phys_birth_txg,
        fill_count: fill_count,
        cksum: ZfsChecksum { words: cksum_words },
    };
}

/// Fast magic and size check for an OpenZFS Uberblock (little-endian or big-endian).
pub fn isZfsUberblock(data: []const u8): bool {
    if (data.len < ZFS_UBERBLOCK_SIZE) {
        return false;
    }
    const le = readU64Le(data, 0);
    const be = readU64Be(data, 0);
    return le == ZFS_UBERBLOCK_MAGIC || be == ZFS_UBERBLOCK_MAGIC || le == ZFS_UBERBLOCK_MAGIC_SWAPPED;
}

/// Parses and validates an OpenZFS Uberblock, auto-detecting little-endian vs big-endian byte order.
/// Validates `version >= 1 && txg >= 1` and decodes the embedded 128-byte `rootbp`.
pub fn parseZfsUberblock(data: []const u8): ZfsUberblock? {
    if (data.len < ZFS_UBERBLOCK_SIZE) {
        return null;
    }

    const le = readU64Le(data, 0);
    const be = readU64Be(data, 0);
    let is_big_endian = false;
    if (le == ZFS_UBERBLOCK_MAGIC) {
        is_big_endian = false;
    } else if (be == ZFS_UBERBLOCK_MAGIC || le == ZFS_UBERBLOCK_MAGIC_SWAPPED) {
        is_big_endian = true;
    } else {
        return null;
    }

    const version = readU64Endian(data, 8, is_big_endian);
    const txg = readU64Endian(data, 16, is_big_endian);
    if (version < 1 || txg < 1) {
        return null;
    }

    if (rootbp := parseZfsBlkPtr(data[40..168], is_big_endian)) {
        return ZfsUberblock {
            is_big_endian: is_big_endian,
            version: version,
            txg: txg,
            guid_sum: readU64Endian(data, 24, is_big_endian),
            timestamp: readU64Endian(data, 32, is_big_endian),
            rootbp: rootbp,
            software_version: readU64Endian(data, 168, is_big_endian),
        };
    }
    return null;
}

/// Selects the active Uberblock with the higher transaction group (`txg`).
pub fn selectActiveUberblock(ub1: ZfsUberblock?, ub2: ZfsUberblock?): ZfsUberblock? {
    if (a := ub1) {
        if (b := ub2) {
            if (b.txg > a.txg) {
                return b;
            }
            return a;
        }
        return a;
    }
    return ub2;
}
