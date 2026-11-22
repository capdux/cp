// Origin: Pure-CP Stdlib (Zero-Alloc QEMU QCOW2 v2/v3 Disk Image Header & Table Parser) | Created: 2026-11-22

// QCOW2 Core Constants
pub const QCOW2_MAGIC: u32 = 0x514649FB; // "QFI\xFB" big-endian
pub const QCOW2_V2_HEADER_SIZE: usize = 72;
pub const QCOW2_V3_MIN_HEADER_SIZE: usize = 104;
pub const QCOW2_MIN_CLUSTER_BITS: u32 = 9;
pub const QCOW2_MAX_CLUSTER_BITS: u32 = 21;

// Encryption Methods
pub const CRYPT_NONE: u32 = 0;
pub const CRYPT_AES: u32 = 1;
pub const CRYPT_LUKS: u32 = 2;

// QCOW2 v3 Incompatible Feature Flags
pub const INCOMPAT_DIRTY: u64 = 0x01;
pub const INCOMPAT_CORRUPT: u64 = 0x02;
pub const INCOMPAT_EXTERNAL_DATA: u64 = 0x04;
pub const INCOMPAT_COMPRESSION_TYPE: u64 = 0x08;
pub const INCOMPAT_EXTENDED_L2: u64 = 0x10;

// QCOW2 Header Extension Types
pub const EXT_END: u32 = 0x00000000;
pub const EXT_BACKING_FORMAT: u32 = 0xE2792ACA;
pub const EXT_FEATURE_NAME_TABLE: u32 = 0x6803F857;
pub const EXT_BITMAPS: u32 = 0x23852875;
pub const EXT_FULL_DISK_ENCRYPTION: u32 = 0x0537BE77;

fn readU32Be(data: []const u8, off: usize): u32 {
    const b0 = (data[off] as u32) << 24;
    const b1 = (data[off + 1] as u32) << 16;
    const b2 = (data[off + 2] as u32) << 8;
    const b3 = data[off + 3] as u32;
    return b0 | b1 | b2 | b3;
}

fn readU64Be(data: []const u8, off: usize): u64 {
    const hi = (readU32Be(data, off) as u64) << 32;
    const lo = readU32Be(data, off + 4) as u64;
    return hi | lo;
}

/// Parsed QEMU QCOW2 v2 (72B) or v3 (>= 104B) disk image header.
pub struct Qcow2Header {
    version: u32,
    backing_file_offset: u64,
    backing_file_size: u32,
    cluster_bits: u32,
    size: u64,
    crypt_method: u32,
    l1_size: u32,
    l1_table_offset: u64,
    refcount_table_offset: u64,
    refcount_table_clusters: u32,
    nb_snapshots: u32,
    snapshots_offset: u64,
    incompat_features: u64,
    compat_features: u64,
    autoclear_features: u64,
    refcount_order: u32,
    header_length: u32,

    /// Cluster size in bytes (`1 << cluster_bits`, 512B..2MiB).
    fn clusterSize(): u64 {
        return (1 as u64) << (_.cluster_bits as u6);
    }

    /// Refcount width in bits (`1 << refcount_order`, 16 for v2).
    fn refcountBits(): u32 {
        return (1 as u32) << (_.refcount_order as u5);
    }

    fn hasBackingFile(): bool {
        return _.backing_file_offset != 0 && _.backing_file_size != 0;
    }

    fn isDirty(): bool {
        return (_.incompat_features & INCOMPAT_DIRTY) != 0;
    }

    fn isCorrupt(): bool {
        return (_.incompat_features & INCOMPAT_CORRUPT) != 0;
    }

    fn hasExtendedL2(): bool {
        return (_.incompat_features & INCOMPAT_EXTENDED_L2) != 0;
    }

    /// Zero-copy extraction of the backing file path from the image buffer.
    fn backingFile(image: []const u8): ([]const u8)? {
        if (!_.hasBackingFile()) {
            return null;
        }
        const off = _.backing_file_offset as usize;
        const sz = _.backing_file_size as usize;
        if (off > image.len || sz > image.len - off) {
            return null;
        }
        return image[off..(off + sz)];
    }
}

/// Returns `true` if `data` starts with the 4-byte QCOW2 magic (`0x514649FB`).
pub fn isQcow2(data: []const u8): bool {
    return data.len >= 4 && readU32Be(data, 0) == QCOW2_MAGIC;
}

/// Parses a QCOW2 v2 or v3 header with strict validation of magic, version (2..3),
/// `cluster_bits` (9..21), `crypt_method` (0..2), `refcount_order` (0..6), and `header_length`.
pub fn parseQcow2Header(data: []const u8): Qcow2Header? {
    if (data.len < QCOW2_V2_HEADER_SIZE || !isQcow2(data)) {
        return null;
    }
    const version = readU32Be(data, 4);
    if (version != 2 && version != 3) {
        return null;
    }
    const backing_file_offset = readU64Be(data, 8);
    const backing_file_size = readU32Be(data, 16);
    const cluster_bits = readU32Be(data, 20);
    if (cluster_bits < QCOW2_MIN_CLUSTER_BITS || cluster_bits > QCOW2_MAX_CLUSTER_BITS) {
        return null;
    }
    const size = readU64Be(data, 24);
    const crypt_method = readU32Be(data, 32);
    if (crypt_method > CRYPT_LUKS) {
        return null;
    }
    const l1_size = readU32Be(data, 36);
    const l1_table_offset = readU64Be(data, 40);
    const refcount_table_offset = readU64Be(data, 48);
    const refcount_table_clusters = readU32Be(data, 56);
    const nb_snapshots = readU32Be(data, 60);
    const snapshots_offset = readU64Be(data, 64);

    let incompat_features: u64 = 0;
    let compat_features: u64 = 0;
    let autoclear_features: u64 = 0;
    let refcount_order: u32 = 4; // v2 default = 16-bit refcounts
    let header_length: u32 = QCOW2_V2_HEADER_SIZE as u32;

    if (version == 3) {
        if (data.len < QCOW2_V3_MIN_HEADER_SIZE) {
            return null;
        }
        incompat_features = readU64Be(data, 72);
        compat_features = readU64Be(data, 80);
        autoclear_features = readU64Be(data, 88);
        refcount_order = readU32Be(data, 96);
        header_length = readU32Be(data, 100);
        if (refcount_order > 6) {
            return null;
        }
        if (header_length < (QCOW2_V3_MIN_HEADER_SIZE as u32) || (header_length as usize) > data.len) {
            return null;
        }
    }

    return Qcow2Header{
        .version = version,
        .backing_file_offset = backing_file_offset,
        .backing_file_size = backing_file_size,
        .cluster_bits = cluster_bits,
        .size = size,
        .crypt_method = crypt_method,
        .l1_size = l1_size,
        .l1_table_offset = l1_table_offset,
        .refcount_table_offset = refcount_table_offset,
        .refcount_table_clusters = refcount_table_clusters,
        .nb_snapshots = nb_snapshots,
        .snapshots_offset = snapshots_offset,
        .incompat_features = incompat_features,
        .compat_features = compat_features,
        .autoclear_features = autoclear_features,
        .refcount_order = refcount_order,
        .header_length = header_length,
    };
}

/// Single QCOW2 Header Extension entry (`ext_type`, zero-copy `data` without 8B alignment padding).
pub struct Qcow2Ext {
    ext_type: u32,
    data: []const u8,
}

/// Zero-copy iterator over 8-byte aligned QCOW2 Header Extensions starting at `header.header_length`.
pub struct Qcow2ExtIter {
    image: []const u8,
    pos: usize,
    done: bool,
    saw_end: bool,
    malformed: bool,

    pub fn init(header: Qcow2Header, data: []const u8): Qcow2ExtIter {
        return Qcow2ExtIter{
            .image = data,
            .pos = header.header_length as usize,
            .done = false,
            .saw_end = false,
            .malformed = false,
        };
    }

    fn next(*_): Qcow2Ext? {
        if (_.done || _.malformed || _.pos >= _.image.len) {
            return null;
        }
        if (_.image.len - _.pos < 8) {
            _.malformed = true;
            _.done = true;
            return null;
        }
        const ext_type = readU32Be(_.image, _.pos);
        const ext_len = readU32Be(_.image, _.pos + 4) as usize;
        const data_start = _.pos + 8;

        if (ext_type == EXT_END) {
            _.pos = data_start;
            _.saw_end = true;
            _.done = true;
            return null;
        }

        if (ext_len > _.image.len - data_start) {
            _.malformed = true;
            _.done = true;
            return null;
        }
        const payload = _.image[data_start..(data_start + ext_len)];

        // Advance to next 8-byte aligned boundary
        const rem = ext_len & 7;
        let pad: usize = 0;
        if (rem != 0) {
            pad = (8 as usize) - rem;
        }
        if (pad > _.image.len - (data_start + ext_len)) {
            _.malformed = true;
            _.done = true;
            return null;
        }
        _.pos = data_start + ext_len + pad;

        return Qcow2Ext{
            .ext_type = ext_type,
            .data = payload,
        };
    }

    fn sawEnd(): bool {
        return _.saw_end;
    }

    fn isMalformed(): bool {
        return _.malformed;
    }
}

/// Creates a zero-copy iterator over QCOW2 Header Extensions.
pub fn iterQcow2Exts(header: Qcow2Header, data: []const u8): Qcow2ExtIter {
    return Qcow2ExtIter.init(header, data);
}

/// Decoded 64-bit QCOW2 L1 table entry.
pub struct Qcow2L1Entry {
    raw: u64,

    /// Bit 63: `1` if refcount == 1 (copied / no COW needed).
    fn isCopied(): bool {
        return (_.raw & 0x8000000000000000) != 0;
    }

    /// Bits 9..55: host offset of the L2 table (512B aligned).
    fn l2Offset(): u64 {
        return _.raw & 0x00FFFFFFFFFFFE00;
    }
}

/// Decodes a 64-bit QCOW2 L1 table entry.
pub fn decodeL1Entry(raw: u64): Qcow2L1Entry {
    return Qcow2L1Entry{ .raw = raw };
}

/// Decoded 64-bit QCOW2 L2 table entry.
pub struct Qcow2L2Entry {
    raw: u64,

    /// Bit 63: `1` if refcount == 1 (copied / no COW needed).
    fn isCopied(): bool {
        return (_.raw & 0x8000000000000000) != 0;
    }

    /// Bit 62: `1` if this cluster is compressed.
    fn isCompressed(): bool {
        return (_.raw & 0x4000000000000000) != 0;
    }

    /// Standard non-compressed cluster with bit 0 set reads as all zeroes.
    fn isZeroCluster(): bool {
        return !_.isCompressed() && (_.raw & 1) != 0;
    }

    /// Bits 9..55: host cluster offset for standard non-compressed clusters (0 if compressed).
    fn hostOffset(): u64 {
        if (_.isCompressed()) {
            return 0;
        }
        return _.raw & 0x00FFFFFFFFFFFE00;
    }
}

/// Decodes a 64-bit QCOW2 L2 table entry.
pub fn decodeL2Entry(raw: u64): Qcow2L2Entry {
    return Qcow2L2Entry{ .raw = raw };
}
