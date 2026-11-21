// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Linux Btrfs Filesystem Parser)

pub const BTRFS_SUPER_OFFSET: usize = 65536;
pub const BTRFS_SUPER_SIZE: usize = 4096;
pub const BTRFS_MAGIC: []const u8 = "_BHRfS_M";
pub const BTRFS_TREE_HEADER_SIZE: usize = 101;
pub const BTRFS_KEY_SIZE: usize = 17;
pub const BTRFS_SYS_CHUNK_ARRAY_OFFSET: usize = 811;
pub const BTRFS_SYS_CHUNK_ARRAY_MAX: usize = 2048;
pub const BTRFS_CHUNK_FIXED_SIZE: usize = 48;
pub const BTRFS_STRIPE_SIZE: usize = 32;

// Checksum Types (btrfs_csum_type)
pub const CSUM_CRC32C: u16 = 0;
pub const CSUM_XXHASH64: u16 = 1;
pub const CSUM_SHA256: u16 = 2;
pub const CSUM_BLAKE2B: u16 = 3;

// Common B-Tree Key Types
pub const KEY_INODE_ITEM: u8 = 1;
pub const KEY_DIR_ITEM: u8 = 84;
pub const KEY_EXTENT_DATA: u8 = 108;
pub const KEY_ROOT_ITEM: u8 = 132;
pub const KEY_CHUNK_ITEM: u8 = 228;

// Block Group / Chunk Profile Flags
pub const BLOCK_GROUP_DATA: u64 = 0x01;
pub const BLOCK_GROUP_SYSTEM: u64 = 0x02;
pub const BLOCK_GROUP_METADATA: u64 = 0x04;
pub const BLOCK_GROUP_RAID0: u64 = 0x08;
pub const BLOCK_GROUP_RAID1: u64 = 0x10;
pub const BLOCK_GROUP_DUP: u64 = 0x20;
pub const BLOCK_GROUP_RAID10: u64 = 0x40;
pub const BLOCK_GROUP_RAID5: u64 = 0x80;
pub const BLOCK_GROUP_RAID6: u64 = 0x100;

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

fn copy32(src: []const u8, off: usize): [32]u8 {
    let out: [32]u8 = undefined;
    let i: usize = 0;
    while (i < 32) {
        out[i] = src[off + i];
        i += 1;
    }
    return out;
}

fn copy256(src: []const u8, off: usize): [256]u8 {
    let out: [256]u8 = undefined;
    let i: usize = 0;
    while (i < 256) {
        out[i] = src[off + i];
        i += 1;
    }
    return out;
}

fn isPowerOfTwoU32(v: u32): bool {
    return v != 0 && (v & (v - 1)) == 0;
}

/// Computes Btrfs on-disk CRC32C using reflected Castagnoli polynomial 0x82F63B78,
/// initial value 0xFFFFFFFF, and NO final bitwise inversion (standard CRC32C = ~btrfsCrc32c(data)).
pub fn btrfsCrc32c(data: []const u8): u32 {
    let crc: u32 = 0xFFFFFFFF;
    let i: usize = 0;
    while (i < data.len) {
        crc ^= data[i] as u32;
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

/// Btrfs 17-byte disk key (`btrfs_disk_key`).
pub struct BtrfsKey {
    objectid: u64,
    key_type: u8,
    offset: u64,
}

/// Decoded System Chunk entry from `superblock.sys_chunk_array`.
pub struct BtrfsSysChunk {
    key: BtrfsKey,
    length: u64,
    owner: u64,
    stripe_len: u64,
    chunk_type: u64,
    num_stripes: u16,
    first_devid: u64,
    first_offset: u64,
}

/// Zero-copy iterator over `(btrfs_disk_key, btrfs_chunk)` records in `sys_chunk_array`.
pub struct BtrfsSysChunkIter {
    data: []const u8,
    pos: usize,

    fn next(*_): BtrfsSysChunk? {
        const min_entry = BTRFS_KEY_SIZE + BTRFS_CHUNK_FIXED_SIZE;
        if (_.pos >= _.data.len || _.data.len - _.pos < min_entry) {
            _.pos = _.data.len;
            return null;
        }

        const key_off = _.pos;
        const key = BtrfsKey {
            objectid: readU64Le(_.data, key_off),
            key_type: _.data[key_off + 8],
            offset: readU64Le(_.data, key_off + 9),
        };
        if (key.key_type != KEY_CHUNK_ITEM) {
            _.pos = _.data.len;
            return null;
        }

        const chunk_off = key_off + BTRFS_KEY_SIZE;
        const length = readU64Le(_.data, chunk_off + 0);
        const owner = readU64Le(_.data, chunk_off + 8);
        const stripe_len = readU64Le(_.data, chunk_off + 16);
        const chunk_type = readU64Le(_.data, chunk_off + 24);
        const num_stripes = readU16Le(_.data, chunk_off + 44);
        if (num_stripes == 0) {
            _.pos = _.data.len;
            return null;
        }

        const stripes_bytes = (num_stripes as usize) * BTRFS_STRIPE_SIZE;
        const entry_size = min_entry + stripes_bytes;
        if (_.data.len - _.pos < entry_size) {
            _.pos = _.data.len;
            return null;
        }

        const stripe0_off = chunk_off + BTRFS_CHUNK_FIXED_SIZE;
        const first_devid = readU64Le(_.data, stripe0_off + 0);
        const first_offset = readU64Le(_.data, stripe0_off + 8);

        _.pos += entry_size;
        return BtrfsSysChunk {
            key: key,
            length: length,
            owner: owner,
            stripe_len: stripe_len,
            chunk_type: chunk_type,
            num_stripes: num_stripes,
            first_devid: first_devid,
            first_offset: first_offset,
        };
    }
}

/// Parsed Linux Btrfs 4KB Superblock (`btrfs_super_block`).
pub struct BtrfsSuperblock {
    csum: [32]u8,
    fsid: [16]u8,
    bytenr: u64,
    flags: u64,
    generation: u64,
    root: u64,
    chunk_root: u64,
    log_root: u64,
    total_bytes: u64,
    bytes_used: u64,
    root_dir_objectid: u64,
    num_devices: u64,
    sectorsize: u32,
    nodesize: u32,
    stripesize: u32,
    sys_chunk_array_size: u32,
    chunk_root_generation: u64,
    compat_flags: u64,
    compat_ro_flags: u64,
    incompat_flags: u64,
    csum_type: u16,
    root_level: u8,
    chunk_root_level: u8,
    label: [256]u8,
    sys_chunk_array: []const u8,

    /// Returns the NUL-terminated filesystem label slice from `label[0..256]`.
    fn labelSlice(): []const u8 {
        let end: usize = 0;
        while (end < 256 && _.label[end] != 0) {
            end += 1;
        }
        return _.label[0..end];
    }

    /// Returns the human-readable checksum algorithm name.
    fn csumName(): []const u8 {
        if (_.csum_type == CSUM_CRC32C) {
            return "crc32c";
        }
        if (_.csum_type == CSUM_XXHASH64) {
            return "xxhash64";
        }
        if (_.csum_type == CSUM_SHA256) {
            return "sha256";
        }
        if (_.csum_type == CSUM_BLAKE2B) {
            return "blake2b";
        }
        return "unknown";
    }

    /// Returns a zero-copy iterator over system chunks embedded in `sys_chunk_array`.
    fn sysChunks(): BtrfsSysChunkIter {
        return BtrfsSysChunkIter {
            data: _.sys_chunk_array,
            pos: 0,
        };
    }
}

/// Parsed Btrfs 101-byte B-Tree node/leaf header (`btrfs_header`).
pub struct BtrfsTreeHeader {
    csum: [32]u8,
    fsid: [16]u8,
    bytenr: u64,
    flags: u64,
    chunk_tree_uuid: [16]u8,
    generation: u64,
    owner: u64,
    nritems: u32,
    level: u8,

    /// Returns true when this B-Tree node is a leaf (`level == 0`).
    fn isLeaf(): bool {
        return _.level == 0;
    }
}

/// Fast magic and size check for a 4KB Btrfs superblock (`"_BHRfS_M"` at offset 64).
pub fn isBtrfsSuperblock(sb4k: []const u8): bool {
    if (sb4k.len < BTRFS_SUPER_SIZE) {
        return false;
    }
    let i: usize = 0;
    while (i < 8) {
        if (sb4k[64 + i] != BTRFS_MAGIC[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Parses and validates a 4KB Btrfs superblock.
/// Checks magic `"_BHRfS_M"` at offset 64, power-of-two `sectorsize`/`nodesize`,
/// `sys_chunk_array_size <= 2048`, and verifies CRC32C over `[32..4096]` when `csum_type == CSUM_CRC32C`.
pub fn parseBtrfsSuperblock(sb4k: []const u8): BtrfsSuperblock? {
    if (!isBtrfsSuperblock(sb4k)) {
        return null;
    }

    const sectorsize = readU32Le(sb4k, 144);
    const nodesize = readU32Le(sb4k, 148);
    if (!isPowerOfTwoU32(sectorsize) || !isPowerOfTwoU32(nodesize)) {
        return null;
    }

    const sys_chunk_array_size = readU32Le(sb4k, 160);
    if ((sys_chunk_array_size as usize) > BTRFS_SYS_CHUNK_ARRAY_MAX) {
        return null;
    }

    const csum_type = readU16Le(sb4k, 196);
    if (csum_type == CSUM_CRC32C) {
        const expected = readU32Le(sb4k, 0);
        const actual = btrfsCrc32c(sb4k[32..BTRFS_SUPER_SIZE]);
        if (actual != expected) {
            return null;
        }
    }

    const array_end = BTRFS_SYS_CHUNK_ARRAY_OFFSET + (sys_chunk_array_size as usize);

    return BtrfsSuperblock {
        csum: copy32(sb4k, 0),
        fsid: copy16(sb4k, 32),
        bytenr: readU64Le(sb4k, 48),
        flags: readU64Le(sb4k, 56),
        generation: readU64Le(sb4k, 72),
        root: readU64Le(sb4k, 80),
        chunk_root: readU64Le(sb4k, 88),
        log_root: readU64Le(sb4k, 96),
        total_bytes: readU64Le(sb4k, 112),
        bytes_used: readU64Le(sb4k, 120),
        root_dir_objectid: readU64Le(sb4k, 128),
        num_devices: readU64Le(sb4k, 136),
        sectorsize: sectorsize,
        nodesize: nodesize,
        stripesize: readU32Le(sb4k, 156),
        sys_chunk_array_size: sys_chunk_array_size,
        chunk_root_generation: readU64Le(sb4k, 164),
        compat_flags: readU64Le(sb4k, 172),
        compat_ro_flags: readU64Le(sb4k, 180),
        incompat_flags: readU64Le(sb4k, 188),
        csum_type: csum_type,
        root_level: sb4k[198],
        chunk_root_level: sb4k[199],
        label: copy256(sb4k, 299),
        sys_chunk_array: sb4k[BTRFS_SYS_CHUNK_ARRAY_OFFSET..array_end],
    };
}

/// Parses a 17-byte Btrfs B-Tree disk key.
pub fn parseBtrfsKey(data: []const u8): BtrfsKey? {
    if (data.len < BTRFS_KEY_SIZE) {
        return null;
    }
    return BtrfsKey {
        objectid: readU64Le(data, 0),
        key_type: data[8],
        offset: readU64Le(data, 9),
    };
}

/// Parses a 101-byte Btrfs B-Tree node/leaf header.
pub fn parseBtrfsTreeHeader(node: []const u8): BtrfsTreeHeader? {
    if (node.len < BTRFS_TREE_HEADER_SIZE) {
        return null;
    }
    return BtrfsTreeHeader {
        csum: copy32(node, 0),
        fsid: copy16(node, 32),
        bytenr: readU64Le(node, 48),
        flags: readU64Le(node, 56),
        chunk_tree_uuid: copy16(node, 64),
        generation: readU64Le(node, 80),
        owner: readU64Le(node, 88),
        nritems: readU32Le(node, 96),
        level: node[100],
    };
}
