// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Apple File System APFS Superblock & Fletcher-64 Parser)

// ---------------------------------------------------------------------------
// APFS Protocol Constants (Little-Endian)
// ---------------------------------------------------------------------------

pub const APFS_OBJ_HEADER_SIZE: usize = 32;
/// Container Superblock magic `"NXSB"` (little-endian `0x4253584E`).
pub const APFS_NX_MAGIC: u32 = 0x4253584E;
/// Volume Superblock magic `"APSB"` (little-endian `0x42535041`).
pub const APFS_VOL_MAGIC: u32 = 0x42535041;
pub const APFS_MIN_BLOCK_SIZE: usize = 4096;
pub const APFS_MAX_FILE_SYSTEMS: u32 = 100;

// Object Types (low 16 bits of o_type)
pub const OBJ_TYPE_NX_SUPERBLOCK: u16 = 0x0001;
pub const OBJ_TYPE_BTREE: u16 = 0x0002;
pub const OBJ_TYPE_BTREE_NODE: u16 = 0x0003;
pub const OBJ_TYPE_SPACEMAN: u16 = 0x0005;
pub const OBJ_TYPE_OMAP: u16 = 0x000B;
pub const OBJ_TYPE_CHECKPOINT_MAP: u16 = 0x000C;
pub const OBJ_TYPE_FS: u16 = 0x000D;

// Storage Type Flags (high 16 bits of o_type, masked by 0xC000)
pub const OBJ_VIRTUAL: u16 = 0x0000;
pub const OBJ_PHYSICAL: u16 = 0x4000;
pub const OBJ_EPHEMERAL: u16 = 0x8000;
const OBJ_STORAGE_MASK: u16 = 0xC000;

// Volume Feature & Incompat Flags
pub const APFS_FS_UNENCRYPTED: u64 = 0x01;
pub const APFS_FS_ONEKEY: u64 = 0x08;
pub const APFS_FS_CASE_INSENSITIVE: u64 = 0x01;
pub const APFS_FS_CASE_SENSITIVE: u64 = 0x08;

// ---------------------------------------------------------------------------
// Internal Little-Endian Helpers
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

fn readBytes256(buf: []const u8, off: usize): [256]u8 {
    let out: [256]u8 = [0; 256];
    let i: usize = 0;
    while (i < 256) {
        out[i] = buf[off + i];
        i += 1;
    }
    return out;
}

fn isPowerOfTwoU32(v: u32): bool {
    return v != 0 && (v & (v - 1)) == 0;
}

// ---------------------------------------------------------------------------
// Apple APFS Fletcher-64 Checksum
// ---------------------------------------------------------------------------

const FLETCHER64_MOD: u64 = 0xFFFFFFFF;

/// Computes the Apple APFS Fletcher-64 checksum over `block[8..block.len]`
/// using 32-bit little-endian words modulo `0xFFFFFFFF`.
pub fn apfsFletcher64(block: []const u8): u64 {
    if (block.len < 8) {
        return 0;
    }
    let sum1: u64 = 0;
    let sum2: u64 = 0;
    let i: usize = 8;
    while (i + 4 <= block.len) {
        const word = readU32Le(block, i) as u64;
        sum1 = (sum1 + word) % FLETCHER64_MOD;
        sum2 = (sum2 + sum1) % FLETCHER64_MOD;
        i += 4;
    }
    const check1: u64 = FLETCHER64_MOD - ((sum1 + sum2) % FLETCHER64_MOD);
    const check2: u64 = FLETCHER64_MOD - ((sum1 + check1) % FLETCHER64_MOD);
    return (check2 << 32) | check1;
}

/// Verifies that the 64-bit `o_cksum` at offset 0 matches the Fletcher-64 checksum of `block`.
pub fn verifyApfsChecksum(block: []const u8): bool {
    if (block.len < 8 || (block.len % 4) != 0) {
        return false;
    }
    const stored = readU64Le(block, 0);
    return stored == apfsFletcher64(block);
}

// ---------------------------------------------------------------------------
// Data Models
// ---------------------------------------------------------------------------

/// 32-byte APFS Object Header (`obj_phys_t`).
pub struct ApfsObjHeader {
    cksum: u64,
    oid: u64,
    xid: u64,
    obj_type: u16,
    obj_flags: u16,
    subtype: u32,

    fn isPhysical(): bool {
        return (_.obj_flags & OBJ_STORAGE_MASK) == OBJ_PHYSICAL;
    }

    fn isEphemeral(): bool {
        return (_.obj_flags & OBJ_STORAGE_MASK) == OBJ_EPHEMERAL;
    }

    fn isVirtual(): bool {
        return (_.obj_flags & OBJ_STORAGE_MASK) == OBJ_VIRTUAL;
    }
}

/// Parsed APFS Container Superblock (`nx_superblock_t`, magic `"NXSB"`).
pub struct ApfsContainerSuper {
    header: ApfsObjHeader,
    block_size: u32,
    block_count: u64,
    features: u64,
    ro_compat_features: u64,
    incompat_features: u64,
    uuid: [16]u8,
    next_oid: u64,
    next_xid: u64,
    xp_desc_blocks: u32,
    xp_data_blocks: u32,
    omap_oid: u64,
    max_file_systems: u32,

    /// Returns total container capacity in bytes (`block_count * block_size`).
    fn totalBytes(): u128 {
        return (_.block_count as u128) * (_.block_size as u128);
    }
}

/// Parsed APFS Volume Superblock (`apfs_superblock_t`, magic `"APSB"`).
pub struct ApfsVolumeSuper {
    header: ApfsObjHeader,
    fs_index: u32,
    features: u64,
    ro_compat_features: u64,
    incompat_features: u64,
    omap_oid: u64,
    root_tree_oid: u64,
    extentref_tree_oid: u64,
    snap_meta_tree_oid: u64,
    num_files: u64,
    num_directories: u64,
    num_symlinks: u64,
    vol_uuid: [16]u8,
    volname: [256]u8,

    /// Returns the NUL-terminated UTF-8 volume name slice.
    fn volnameSlice(): []const u8 {
        let end: usize = 0;
        while (end < 256 && _.volname[end] != 0) {
            end += 1;
        }
        return _.volname[0..end];
    }

    fn isUnencrypted(): bool {
        return (_.features & APFS_FS_UNENCRYPTED) != 0;
    }

    fn isCaseInsensitive(): bool {
        return (_.incompat_features & APFS_FS_CASE_INSENSITIVE) != 0;
    }
}

// ---------------------------------------------------------------------------
// Parsing Functions
// ---------------------------------------------------------------------------

/// Parses a 32-byte APFS Object Header from `data[0..32]`.
pub fn parseApfsObjHeader(data: []const u8): ApfsObjHeader? {
    if (data.len < APFS_OBJ_HEADER_SIZE) {
        return null;
    }
    return ApfsObjHeader {
        cksum: readU64Le(data, 0),
        oid: readU64Le(data, 8),
        xid: readU64Le(data, 16),
        obj_type: readU16Le(data, 24),
        obj_flags: readU16Le(data, 26),
        subtype: readU32Le(data, 28),
    };
}

/// Returns `true` if `block` has at least 36 bytes and `"NXSB"` magic at offset 32.
pub fn isApfsContainer(block: []const u8): bool {
    if (block.len < 36) {
        return false;
    }
    return readU32Le(block, 32) == APFS_NX_MAGIC;
}

/// Parses and validates an APFS Container Superblock (`"NXSB"`, `OBJ_TYPE_NX_SUPERBLOCK`,
/// power-of-two `block_size >= 4096`, and Fletcher-64 checksum).
pub fn parseApfsContainer(block: []const u8): ApfsContainerSuper? {
    if (block.len < APFS_MIN_BLOCK_SIZE) {
        return null;
    }
    if (readU32Le(block, 32) != APFS_NX_MAGIC) {
        return null;
    }
    const header = parseApfsObjHeader(block) ?? return null;
    if (header.obj_type != OBJ_TYPE_NX_SUPERBLOCK) {
        return null;
    }
    const block_size = readU32Le(block, 36);
    if ((block_size as usize) < APFS_MIN_BLOCK_SIZE || !isPowerOfTwoU32(block_size)) {
        return null;
    }
    const bsize: usize = block_size as usize;
    if (block.len < bsize) {
        return null;
    }
    if (!verifyApfsChecksum(block[0..bsize])) {
        return null;
    }

    return ApfsContainerSuper {
        header: header,
        block_size: block_size,
        block_count: readU64Le(block, 40),
        features: readU64Le(block, 48),
        ro_compat_features: readU64Le(block, 56),
        incompat_features: readU64Le(block, 64),
        uuid: readBytes16(block, 72),
        next_oid: readU64Le(block, 88),
        next_xid: readU64Le(block, 96),
        xp_desc_blocks: readU32Le(block, 104),
        xp_data_blocks: readU32Le(block, 108),
        omap_oid: readU64Le(block, 160),
        max_file_systems: readU32Le(block, 180),
    };
}

/// Returns `true` if `block` has at least 36 bytes and `"APSB"` magic at offset 32.
pub fn isApfsVolume(block: []const u8): bool {
    if (block.len < 36) {
        return false;
    }
    return readU32Le(block, 32) == APFS_VOL_MAGIC;
}

/// Parses and validates an APFS Volume Superblock (`"APSB"`, `OBJ_TYPE_FS`, and Fletcher-64 checksum).
pub fn parseApfsVolume(block: []const u8): ApfsVolumeSuper? {
    if (block.len < APFS_MIN_BLOCK_SIZE) {
        return null;
    }
    if (readU32Le(block, 32) != APFS_VOL_MAGIC) {
        return null;
    }
    const header = parseApfsObjHeader(block) ?? return null;
    if (header.obj_type != OBJ_TYPE_FS) {
        return null;
    }
    if (!verifyApfsChecksum(block)) {
        return null;
    }

    return ApfsVolumeSuper {
        header: header,
        fs_index: readU32Le(block, 36),
        features: readU64Le(block, 40),
        ro_compat_features: readU64Le(block, 48),
        incompat_features: readU64Le(block, 56),
        omap_oid: readU64Le(block, 128),
        root_tree_oid: readU64Le(block, 136),
        extentref_tree_oid: readU64Le(block, 144),
        snap_meta_tree_oid: readU64Le(block, 152),
        num_files: readU64Le(block, 184),
        num_directories: readU64Le(block, 192),
        num_symlinks: readU64Le(block, 200),
        vol_uuid: readBytes16(block, 240),
        volname: readBytes256(block, 704),
    };
}
