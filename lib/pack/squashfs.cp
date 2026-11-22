// Origin: Pure-CP Stdlib (Zero-Alloc Linux SquashFS v4.0 Read-Only Filesystem Parser) | Created: 2026-11-22

// SquashFS v4.0 Core Constants
pub const SQUASHFS_MAGIC: u32 = 0x73717368; // "hsqs" little-endian
pub const SQUASHFS_SUPER_SIZE: usize = 96;
pub const SQUASHFS_MAJOR: u16 = 4;
pub const SQUASHFS_MINOR: u16 = 0;
pub const SQUASHFS_METADATA_MAX_SIZE: usize = 8192;
pub const SQUASHFS_META_UNCOMPRESSED_BIT: u16 = 0x8000;
pub const SQUASHFS_DATA_UNCOMPRESSED_BIT: u32 = 0x01000000;
pub const SQUASHFS_INVALID_OFFSET: u64 = 0xFFFFFFFFFFFFFFFF;

// Compression IDs
pub const COMP_GZIP: u16 = 1;
pub const COMP_LZMA: u16 = 2;
pub const COMP_LZO: u16 = 3;
pub const COMP_XZ: u16 = 4;
pub const COMP_LZ4: u16 = 5;
pub const COMP_ZSTD: u16 = 6;

// Superblock Flags
pub const FLAG_UNCOMPRESSED_INODES: u16 = 0x0001;
pub const FLAG_UNCOMPRESSED_DATA: u16 = 0x0002;
pub const FLAG_UNCOMPRESSED_FRAGMENTS: u16 = 0x0008;
pub const FLAG_NO_FRAGMENTS: u16 = 0x0010;
pub const FLAG_ALWAYS_FRAGMENTS: u16 = 0x0020;
pub const FLAG_DUPLICATES: u16 = 0x0040;
pub const FLAG_EXPORTABLE: u16 = 0x0080;
pub const FLAG_UNCOMPRESSED_XATTRS: u16 = 0x0100;
pub const FLAG_NO_XATTRS: u16 = 0x0200;
pub const FLAG_COMPRESSOR_OPTIONS: u16 = 0x0400;
pub const FLAG_UNCOMPRESSED_IDS: u16 = 0x0800;

// Inode Types
pub const INODE_DIR: u16 = 1;
pub const INODE_REG: u16 = 2;
pub const INODE_SYMLINK: u16 = 3;
pub const INODE_BLKDEV: u16 = 4;
pub const INODE_CHRDEV: u16 = 5;
pub const INODE_FIFO: u16 = 6;
pub const INODE_SOCK: u16 = 7;

fn readU16Le(data: []const u8, off: usize): u16 {
    const b0 = data[off] as u16;
    const b1 = (data[off + 1] as u16) << 8;
    return b0 | b1;
}

fn readU32Le(data: []const u8, off: usize): u32 {
    const b0 = data[off] as u32;
    const b1 = (data[off + 1] as u32) << 8;
    const b2 = (data[off + 2] as u32) << 16;
    const b3 = (data[off + 3] as u32) << 24;
    return b0 | b1 | b2 | b3;
}

fn readU64Le(data: []const u8, off: usize): u64 {
    const lo = readU32Le(data, off) as u64;
    const hi = (readU32Le(data, off + 4) as u64) << 32;
    return lo | hi;
}

/// 96-byte Linux SquashFS v4.0 Superblock.
pub struct SquashfsSuperblock {
    inodes: u32,
    mkfs_time: u32,
    block_size: u32,
    fragments: u32,
    compression_id: u16,
    block_log: u16,
    flags: u16,
    no_ids: u16,
    major: u16,
    minor: u16,
    root_inode_ref: u64,
    bytes_used: u64,
    id_table_start: u64,
    xattr_id_table_start: u64,
    inode_table_start: u64,
    directory_table_start: u64,
    fragment_table_start: u64,
    lookup_table_start: u64,

    fn compressionName(): []const u8 {
        if (_.compression_id == COMP_GZIP) { return "gzip"; }
        if (_.compression_id == COMP_LZMA) { return "lzma"; }
        if (_.compression_id == COMP_LZO) { return "lzo"; }
        if (_.compression_id == COMP_XZ) { return "xz"; }
        if (_.compression_id == COMP_LZ4) { return "lz4"; }
        if (_.compression_id == COMP_ZSTD) { return "zstd"; }
        return "unknown";
    }

    fn hasXattrs(): bool {
        return (_.flags & FLAG_NO_XATTRS) == 0 && _.xattr_id_table_start != SQUASHFS_INVALID_OFFSET;
    }

    fn hasFragments(): bool {
        return (_.flags & FLAG_NO_FRAGMENTS) == 0 && _.fragment_table_start != SQUASHFS_INVALID_OFFSET;
    }

    fn hasExportTable(): bool {
        return (_.flags & FLAG_EXPORTABLE) != 0 && _.lookup_table_start != SQUASHFS_INVALID_OFFSET;
    }

    fn inodesUncompressed(): bool {
        return (_.flags & FLAG_UNCOMPRESSED_INODES) != 0;
    }

    fn dataUncompressed(): bool {
        return (_.flags & FLAG_UNCOMPRESSED_DATA) != 0;
    }
}

/// Returns `true` if `data` starts with the 4-byte SquashFS little-endian magic (`"hsqs"`).
pub fn isSquashfs(data: []const u8): bool {
    return data.len >= 4 && readU32Le(data, 0) == SQUASHFS_MAGIC;
}

/// Parses a 96-byte SquashFS v4.0 Superblock with strict validation of magic, version (4.0),
/// `block_size == (1 << block_log)` (`12 <= block_log <= 20`), and `compression_id` (1..6).
pub fn parseSquashfsSuperblock(data: []const u8): SquashfsSuperblock? {
    if (data.len < SQUASHFS_SUPER_SIZE || !isSquashfs(data)) {
        return null;
    }
    const inodes = readU32Le(data, 4);
    const mkfs_time = readU32Le(data, 8);
    const block_size = readU32Le(data, 12);
    const fragments = readU32Le(data, 16);
    const compression_id = readU16Le(data, 20);
    const block_log = readU16Le(data, 22);
    const flags = readU16Le(data, 24);
    const no_ids = readU16Le(data, 26);
    const major = readU16Le(data, 28);
    const minor = readU16Le(data, 30);

    if (major != SQUASHFS_MAJOR || minor != SQUASHFS_MINOR) {
        return null;
    }
    if (compression_id < COMP_GZIP || compression_id > COMP_ZSTD) {
        return null;
    }
    if (block_log < 12 || block_log > 20) {
        return null;
    }
    const expected_bs: u32 = (1 as u32) << (block_log as u5);
    if (block_size != expected_bs) {
        return null;
    }

    return SquashfsSuperblock{
        .inodes = inodes,
        .mkfs_time = mkfs_time,
        .block_size = block_size,
        .fragments = fragments,
        .compression_id = compression_id,
        .block_log = block_log,
        .flags = flags,
        .no_ids = no_ids,
        .major = major,
        .minor = minor,
        .root_inode_ref = readU64Le(data, 32),
        .bytes_used = readU64Le(data, 40),
        .id_table_start = readU64Le(data, 48),
        .xattr_id_table_start = readU64Le(data, 56),
        .inode_table_start = readU64Le(data, 64),
        .directory_table_start = readU64Le(data, 72),
        .fragment_table_start = readU64Le(data, 80),
        .lookup_table_start = readU64Le(data, 88),
    };
}

/// Decoded 64-bit SquashFS Inode Reference (48-bit metadata block offset + 13-bit inner offset).
pub struct SquashfsInodeRef {
    block_offset: u64,
    inner_offset: u16,
}

/// Decodes a 64-bit SquashFS inode reference into `(block_offset, inner_offset)`.
pub fn decodeInodeRef(inode_ref: u64): SquashfsInodeRef {
    return SquashfsInodeRef{
        .block_offset = (inode_ref >> 16) & 0x0000FFFFFFFFFFFF,
        .inner_offset = (inode_ref & 0x1FFF) as u16,
    };
}

/// Encodes a 48-bit `block_offset` and 13-bit `inner_offset` (`< 8192`) into a 64-bit SquashFS inode reference.
pub fn encodeInodeRef(block_offset: u64, inner_offset: u16): u64? {
    if (block_offset > 0x0000FFFFFFFFFFFF || inner_offset >= 8192) {
        return null;
    }
    return (block_offset << 16) | (inner_offset as u64);
}

/// Parsed 2-byte SquashFS Metadata Block header and zero-copy payload slice.
pub struct SquashfsMetaHeader {
    is_compressed: bool,
    size: u16,
    payload: []const u8,
    consumed: usize,
}

/// Parses a SquashFS Metadata Block (`2B header + payload`), enforcing `1 <= size <= 8192` and buffer bounds.
pub fn parseMetaBlock(data: []const u8): SquashfsMetaHeader? {
    if (data.len < 2) {
        return null;
    }
    const raw = readU16Le(data, 0);
    const is_compressed = (raw & SQUASHFS_META_UNCOMPRESSED_BIT) == 0;
    const size = raw & 0x7FFF;
    if (size == 0 || (size as usize) > SQUASHFS_METADATA_MAX_SIZE) {
        return null;
    }
    const sz = size as usize;
    if (sz > data.len - 2) {
        return null;
    }
    return SquashfsMetaHeader{
        .is_compressed = is_compressed,
        .size = size,
        .payload = data[2..(2 + sz)],
        .consumed = 2 + sz,
    };
}

/// Decoded 32-bit SquashFS Data Block size descriptor (24-bit size + uncompressed bit + sparse check).
pub struct SquashfsDataBlockSize {
    is_compressed: bool,
    size: u32,
    is_sparse: bool,
}

/// Decodes a 32-bit SquashFS data block size field.
pub fn decodeDataBlockSize(raw: u32): SquashfsDataBlockSize {
    const size = raw & 0x00FFFFFF;
    const is_sparse = size == 0;
    const is_compressed = !is_sparse && (raw & SQUASHFS_DATA_UNCOMPRESSED_BIT) == 0;
    return SquashfsDataBlockSize{
        .is_compressed = is_compressed,
        .size = size,
        .is_sparse = is_sparse,
    };
}

/// 16-byte SquashFS Fragment Table Entry (`start_block`, `size_raw`, decoded `is_compressed` & `size`).
pub struct SquashfsFragmentEntry {
    start_block: u64,
    size_raw: u32,
    is_compressed: bool,
    size: u32,
}

/// Parses a 16-byte SquashFS Fragment Entry.
pub fn parseFragmentEntry(data: []const u8): SquashfsFragmentEntry? {
    if (data.len < 16) {
        return null;
    }
    const start_block = readU64Le(data, 0);
    const size_raw = readU32Le(data, 8);
    const decoded = decodeDataBlockSize(size_raw);
    return SquashfsFragmentEntry{
        .start_block = start_block,
        .size_raw = size_raw,
        .is_compressed = decoded.is_compressed,
        .size = decoded.size,
    };
}
