// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc VirtualBox VDI 1.1 Disk Image Header & Block Map Parser)

// ---------------------------------------------------------------------------
// VDI 1.1 Protocol Constants
// ---------------------------------------------------------------------------

/// VDI magic signature at byte offset 64 (little-endian `0xBEDA107F`).
pub const VDI_SIGNATURE: u32 = 0xBEDA107F;

pub const VDI_MIN_FILE_HEADER_SIZE: usize = 512;
pub const VDI_V1_1_MIN_HEADER_SIZE: u32 = 400;
pub const VDI_DEFAULT_SECTOR_SIZE: u32 = 512;
pub const VDI_DEFAULT_BLOCK_SIZE: u32 = 1048576;

// Image Types
pub const TYPE_DYNAMIC: u32 = 1;
pub const TYPE_STATIC: u32 = 2;
pub const TYPE_UNDO: u32 = 3;
pub const TYPE_DIFF: u32 = 4;

// Block Allocation Map Special Entries
pub const VDI_BLOCK_FREE: u32 = 0xFFFFFFFF;
pub const VDI_BLOCK_ZERO: u32 = 0xFFFFFFFE;

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

fn isNonZeroUuid(u: [16]u8): bool {
    let i: usize = 0;
    while (i < 16) {
        if (u[i] != 0) {
            return true;
        }
        i += 1;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Data Models
// ---------------------------------------------------------------------------

/// Parsed VirtualBox VDI 1.1 virtual disk image header.
pub struct VdiHeader {
    version_minor: u16,
    version_major: u16,
    header_size: u32,
    image_type: u32,
    image_flags: u32,
    offset_blocks: u32,
    offset_data: u32,
    sector_size: u32,
    disk_size: u64,
    block_size: u32,
    block_extra_data: u32,
    blocks_in_hdd: u32,
    blocks_allocated: u32,
    uuid_create: [16]u8,
    uuid_modify: [16]u8,
    uuid_linkage: [16]u8,
    uuid_parent_modify: [16]u8,
    comment: [256]u8,

    fn isDynamic(): bool {
        return _.image_type == TYPE_DYNAMIC;
    }

    fn isStatic(): bool {
        return _.image_type == TYPE_STATIC;
    }

    fn isDiff(): bool {
        return _.image_type == TYPE_DIFF;
    }

    /// Returns `true` if `uuid_linkage` is non-zero (indicating a differencing/child image with a parent).
    fn hasParent(): bool {
        return isNonZeroUuid(_.uuid_linkage);
    }

    /// Returns the NUL-terminated comment slice from the 256-byte comment buffer.
    fn commentSlice(): []const u8 {
        let end: usize = 0;
        while (end < 256 && _.comment[end] != 0) {
            end += 1;
        }
        return _.comment[0..end];
    }

    /// Returns the total byte size of the Block Allocation Map (`blocks_in_hdd * 4`).
    fn blockMapBytes(): u64 {
        return (_.blocks_in_hdd as u64) * 4;
    }
}

/// Decoded mapping for a single logical block in the VDI Block Allocation Map.
pub struct VdiBlockMapping {
    is_allocated: bool,
    is_zero: bool,
    phys_block: u32,
    file_offset: u64,
}

// ---------------------------------------------------------------------------
// Parsing & Block Map Lookup Functions
// ---------------------------------------------------------------------------

/// Returns `true` if `data` has at least 68 bytes and contains `VDI_SIGNATURE` at offset 64.
pub fn isVdi(data: []const u8): bool {
    if (data.len < 68) {
        return false;
    }
    return readU32Le(data, 64) == VDI_SIGNATURE;
}

/// Parses and validates a VirtualBox VDI 1.1 image header from the first 512+ bytes of an image.
pub fn parseVdiHeader(data: []const u8): VdiHeader? {
    if (data.len < VDI_MIN_FILE_HEADER_SIZE) {
        return null;
    }
    if (readU32Le(data, 64) != VDI_SIGNATURE) {
        return null;
    }

    const version_minor = readU16Le(data, 68);
    const version_major = readU16Le(data, 70);
    if (version_major != 1 || version_minor != 1) {
        return null;
    }

    const header_size = readU32Le(data, 72);
    if (header_size < VDI_V1_1_MIN_HEADER_SIZE) {
        return null;
    }

    const image_type = readU32Le(data, 76);
    if (image_type < TYPE_DYNAMIC || image_type > TYPE_DIFF) {
        return null;
    }

    const sector_size = readU32Le(data, 360);
    if (sector_size == 0) {
        return null;
    }

    const block_size = readU32Le(data, 376);
    if (block_size == 0) {
        return null;
    }

    return VdiHeader {
        version_minor: version_minor,
        version_major: version_major,
        header_size: header_size,
        image_type: image_type,
        image_flags: readU32Le(data, 80),
        offset_blocks: readU32Le(data, 340),
        offset_data: readU32Le(data, 344),
        sector_size: sector_size,
        disk_size: readU64Le(data, 368),
        block_size: block_size,
        block_extra_data: readU32Le(data, 380),
        blocks_in_hdd: readU32Le(data, 384),
        blocks_allocated: readU32Le(data, 388),
        uuid_create: readBytes16(data, 392),
        uuid_modify: readBytes16(data, 408),
        uuid_linkage: readBytes16(data, 424),
        uuid_parent_modify: readBytes16(data, 440),
        comment: readBytes256(data, 84),
    };
}

/// Decodes a single 32-bit VDI Block Allocation Map entry using `header` geometry.
pub fn decodeVdiBlockEntry(header: VdiHeader, raw_entry: u32): VdiBlockMapping {
    if (raw_entry == VDI_BLOCK_FREE) {
        return VdiBlockMapping {
            is_allocated: false,
            is_zero: false,
            phys_block: 0,
            file_offset: 0,
        };
    }
    if (raw_entry == VDI_BLOCK_ZERO) {
        return VdiBlockMapping {
            is_allocated: false,
            is_zero: true,
            phys_block: 0,
            file_offset: 0,
        };
    }
    const stride: u64 = (header.block_size as u64) + (header.block_extra_data as u64);
    const file_offset: u64 = (header.offset_data as u64) + (raw_entry as u64) * stride;
    return VdiBlockMapping {
        is_allocated: true,
        is_zero: false,
        phys_block: raw_entry,
        file_offset: file_offset,
    };
}

/// Looks up `logical_block` in `block_map` (the raw slice starting at `header.offset_blocks`).
/// Returns `null` if `logical_block >= header.blocks_in_hdd` or `block_map` is truncated.
pub fn lookupVdiBlock(header: VdiHeader, block_map: []const u8, logical_block: u32): VdiBlockMapping? {
    if (logical_block >= header.blocks_in_hdd) {
        return null;
    }
    const byte_off: usize = (logical_block as usize) * 4;
    if (byte_off + 4 > block_map.len) {
        return null;
    }
    const raw_entry = readU32Le(block_map, byte_off);
    return decodeVdiBlockEntry(header, raw_entry);
}
