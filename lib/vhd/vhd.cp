// Origin: Pure-CP Stdlib (Zero-Alloc Microsoft VHD 1.0 Footer, Dynamic Header & BAT Parser) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// VHD 1.0 Protocol Constants (Big-Endian)
// ---------------------------------------------------------------------------

pub const VHD_FOOTER_COOKIE: []const u8 = "conectix";
pub const VHD_DYNAMIC_COOKIE: []const u8 = "cxsparse";

pub const VHD_FOOTER_SIZE: usize = 512;
pub const VHD_DYNAMIC_HEADER_SIZE: usize = 1024;
pub const VHD_FORMAT_VERSION: u32 = 0x00010000;
pub const VHD_FIXED_DATA_OFFSET: u64 = 0xFFFFFFFFFFFFFFFF;
/// Seconds between 1970-01-01T00:00:00Z (Unix epoch) and 2000-01-01T00:00:00Z (VHD epoch).
pub const VHD_EPOCH_UNIX_DELTA: u64 = 946684800;
pub const VHD_BAT_UNUSED: u32 = 0xFFFFFFFF;

// Disk Types
pub const DISK_TYPE_FIXED: u32 = 2;
pub const DISK_TYPE_DYNAMIC: u32 = 3;
pub const DISK_TYPE_DIFFERENCING: u32 = 4;

// Feature Flags
pub const FEATURE_TEMPORARY: u32 = 0x01;
pub const FEATURE_RESERVED: u32 = 0x02;

// ---------------------------------------------------------------------------
// Internal Big-Endian & Byte Helpers
// ---------------------------------------------------------------------------

fn readU16Be(buf: []const u8, off: usize): u16 {
    const b0 = buf[off] as u16;
    const b1 = buf[off + 1] as u16;
    return (b0 << 8) | b1;
}

fn readU32Be(buf: []const u8, off: usize): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn readU64Be(buf: []const u8, off: usize): u64 {
    const hi = readU32Be(buf, off) as u64;
    const lo = readU32Be(buf, off + 4) as u64;
    return (hi << 32) | lo;
}

fn readBytes4(buf: []const u8, off: usize): [4]u8 {
    let out: [4]u8 = [0; 4];
    out[0] = buf[off];
    out[1] = buf[off + 1];
    out[2] = buf[off + 2];
    out[3] = buf[off + 3];
    return out;
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

fn matchBytes(buf: []const u8, off: usize, sig: []const u8): bool {
    if (buf.len < off + sig.len) {
        return false;
    }
    let i: usize = 0;
    while (i < sig.len) {
        if (buf[off + i] != sig[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn isPowerOfTwoU32(v: u32): bool {
    return v != 0 && (v & (v - 1)) == 0;
}

// ---------------------------------------------------------------------------
// One's Complement Checksum (MS VHD 1.0 Spec)
// ---------------------------------------------------------------------------

/// Computes the VHD 1.0 one's-complement byte sum checksum over `data`,
/// skipping the 4-byte checksum field at `[checksum_offset .. checksum_offset + 4)`.
pub fn vhdChecksum(data: []const u8, checksum_offset: usize): u32 {
    let sum: u64 = 0;
    let i: usize = 0;
    while (i < data.len) {
        if (i < checksum_offset || i >= checksum_offset + 4) {
            sum += data[i] as u64;
        }
        i += 1;
    }
    const low32: u32 = (sum & 0xFFFFFFFF) as u32;
    return low32 ^ 0xFFFFFFFF;
}

// ---------------------------------------------------------------------------
// Data Models
// ---------------------------------------------------------------------------

/// CHS Disk Geometry stored in the VHD Footer.
pub struct VhdGeometry {
    cylinders: u16,
    heads: u8,
    sectors_per_track: u8,
}

/// Parsed 512-byte Microsoft VHD 1.0 Hard Disk Footer.
pub struct VhdFooter {
    features: u32,
    format_version: u32,
    data_offset: u64,
    timestamp: u32,
    creator_app: [4]u8,
    creator_version: u32,
    creator_host_os: [4]u8,
    original_size: u64,
    current_size: u64,
    geometry: VhdGeometry,
    disk_type: u32,
    checksum: u32,
    unique_id: [16]u8,
    saved_state: bool,

    fn isFixed(): bool {
        return _.disk_type == DISK_TYPE_FIXED;
    }

    fn isDynamic(): bool {
        return _.disk_type == DISK_TYPE_DYNAMIC;
    }

    fn isDifferencing(): bool {
        return _.disk_type == DISK_TYPE_DIFFERENCING;
    }

    /// Converts the VHD timestamp (seconds since 2000-01-01T00:00:00Z) to Unix epoch seconds.
    fn unixTimestamp(): u64 {
        return (_.timestamp as u64) + VHD_EPOCH_UNIX_DELTA;
    }
}

/// Parsed 1024-byte Microsoft VHD 1.0 Dynamic Disk Header.
pub struct VhdDynamicHeader {
    data_offset: u64,
    table_offset: u64,
    header_version: u32,
    max_table_entries: u32,
    block_size: u32,
    checksum: u32,
    parent_unique_id: [16]u8,
    parent_timestamp: u32,
    parent_unicode_name: [512]u8,

    /// Returns the number of 512-byte sectors per data block.
    fn blockSectorCount(): u32 {
        return _.block_size / 512;
    }

    /// Returns the byte length of the Block Allocation Table (`max_table_entries * 4`).
    fn batSizeBytes(): u64 {
        return (_.max_table_entries as u64) * 4;
    }
}

// ---------------------------------------------------------------------------
// Parsing & BAT Lookup Functions
// ---------------------------------------------------------------------------

/// Returns `true` if `data` starts with the 8-byte `"conectix"` VHD footer cookie.
pub fn isVhdFooter(data: []const u8): bool {
    return matchBytes(data, 0, VHD_FOOTER_COOKIE);
}

/// Parses and validates a 512-byte VHD 1.0 Hard Disk Footer.
pub fn parseVhdFooter(footer512: []const u8): VhdFooter? {
    if (footer512.len < VHD_FOOTER_SIZE) {
        return null;
    }
    if (!matchBytes(footer512, 0, VHD_FOOTER_COOKIE)) {
        return null;
    }
    const format_version = readU32Be(footer512, 12);
    if (format_version != VHD_FORMAT_VERSION) {
        return null;
    }
    const disk_type = readU32Be(footer512, 60);
    if (disk_type < DISK_TYPE_FIXED || disk_type > DISK_TYPE_DIFFERENCING) {
        return null;
    }
    const stored_checksum = readU32Be(footer512, 64);
    const computed_checksum = vhdChecksum(footer512[0..VHD_FOOTER_SIZE], 64);
    if (stored_checksum != computed_checksum) {
        return null;
    }

    return VhdFooter {
        features: readU32Be(footer512, 8),
        format_version: format_version,
        data_offset: readU64Be(footer512, 16),
        timestamp: readU32Be(footer512, 24),
        creator_app: readBytes4(footer512, 28),
        creator_version: readU32Be(footer512, 32),
        creator_host_os: readBytes4(footer512, 36),
        original_size: readU64Be(footer512, 40),
        current_size: readU64Be(footer512, 48),
        geometry: VhdGeometry {
            cylinders: readU16Be(footer512, 56),
            heads: footer512[58],
            sectors_per_track: footer512[59],
        },
        disk_type: disk_type,
        checksum: stored_checksum,
        unique_id: readBytes16(footer512, 68),
        saved_state: footer512[84] != 0,
    };
}

/// Parses and validates a 1024-byte VHD 1.0 Dynamic Disk Header.
pub fn parseVhdDynamicHeader(hdr1024: []const u8): VhdDynamicHeader? {
    if (hdr1024.len < VHD_DYNAMIC_HEADER_SIZE) {
        return null;
    }
    if (!matchBytes(hdr1024, 0, VHD_DYNAMIC_COOKIE)) {
        return null;
    }
    const header_version = readU32Be(hdr1024, 24);
    if (header_version != VHD_FORMAT_VERSION) {
        return null;
    }
    const block_size = readU32Be(hdr1024, 32);
    if (block_size < 512 || !isPowerOfTwoU32(block_size)) {
        return null;
    }
    const stored_checksum = readU32Be(hdr1024, 36);
    const computed_checksum = vhdChecksum(hdr1024[0..VHD_DYNAMIC_HEADER_SIZE], 36);
    if (stored_checksum != computed_checksum) {
        return null;
    }

    return VhdDynamicHeader {
        data_offset: readU64Be(hdr1024, 8),
        table_offset: readU64Be(hdr1024, 16),
        header_version: header_version,
        max_table_entries: readU32Be(hdr1024, 28),
        block_size: block_size,
        checksum: stored_checksum,
        parent_unique_id: readBytes16(hdr1024, 40),
        parent_timestamp: readU32Be(hdr1024, 56),
        parent_unicode_name: readBytes512(hdr1024, 64),
    };
}

/// Looks up the big-endian 32-bit absolute sector offset for `block_idx` in `bat_bytes`.
/// Returns `null` if out of bounds or if the entry is `VHD_BAT_UNUSED` (`0xFFFFFFFF`).
pub fn lookupVhdBatEntry(bat_bytes: []const u8, block_idx: u32): u32? {
    const off: usize = (block_idx as usize) * 4;
    if (off + 4 > bat_bytes.len) {
        return null;
    }
    const sector_offset = readU32Be(bat_bytes, off);
    if (sector_offset == VHD_BAT_UNUSED) {
        return null;
    }
    return sector_offset;
}
