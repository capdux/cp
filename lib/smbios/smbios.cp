// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc DMTF SMBIOS 2.1 / 3.0+ Table Parser)

pub const SMBIOS2_ANCHOR: []const u8 = "_SM_";
pub const SMBIOS2_DMI_ANCHOR: []const u8 = "_DMI_";
pub const SMBIOS3_ANCHOR: []const u8 = "_SM3_";
pub const SMBIOS2_EP_SIZE: usize = 31;
pub const SMBIOS3_EP_SIZE: usize = 24;
pub const SMBIOS_STRUCT_HEADER_SIZE: usize = 4;

// DMTF SMBIOS Standard Structure Types
pub const TYPE_BIOS: u8 = 0;
pub const TYPE_SYSTEM: u8 = 1;
pub const TYPE_BASEBOARD: u8 = 2;
pub const TYPE_CHASSIS: u8 = 3;
pub const TYPE_PROCESSOR: u8 = 4;
pub const TYPE_MEMORY_DEVICE: u8 = 17;
pub const TYPE_END_OF_TABLE: u8 = 127;

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

fn matchBytes(data: []const u8, off: usize, pat: []const u8): bool {
    if (off + pat.len > data.len) {
        return false;
    }
    let i: usize = 0;
    while (i < pat.len) {
        if (data[off + i] != pat[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Computes the SMBIOS byte-sum checksum modulo 256 (valid structures sum to `0`).
pub fn smbiosChecksum(data: []const u8): u8 {
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < data.len) {
        sum += data[i] as u32;
        i += 1;
    }
    return (sum & 0xFF) as u8;
}

/// Parsed SMBIOS 2.1 (32-bit) Entry Point structure.
pub struct Smbios2EntryPoint {
    major: u8,
    minor: u8,
    max_struct_size: u16,
    table_length: u16,
    table_addr: u32,
    struct_count: u16,
}

/// Strictly parses an SMBIOS 2.1 Entry Point: verifies `"_SM_"`, main checksum,
/// intermediate `"_DMI_"` anchor at offset 16, and 15-byte intermediate checksum.
pub fn parseSmbios2Entry(data: []const u8): Smbios2EntryPoint? {
    if (data.len < SMBIOS2_EP_SIZE) {
        return null;
    }
    if (!matchBytes(data, 0, SMBIOS2_ANCHOR)) {
        return null;
    }
    const ep_len = data[5] as usize;
    if (ep_len < SMBIOS2_EP_SIZE || ep_len > data.len) {
        return null;
    }
    if (smbiosChecksum(data[0..ep_len]) != 0) {
        return null;
    }
    if (!matchBytes(data, 16, SMBIOS2_DMI_ANCHOR)) {
        return null;
    }
    if (smbiosChecksum(data[16..31]) != 0) {
        return null;
    }

    return Smbios2EntryPoint{
        .major = data[6],
        .minor = data[7],
        .max_struct_size = readU16Le(data, 8),
        .table_length = readU16Le(data, 22),
        .table_addr = readU32Le(data, 24),
        .struct_count = readU16Le(data, 28),
    };
}

/// Parsed SMBIOS 3.0+ (64-bit) Entry Point structure.
pub struct Smbios3EntryPoint {
    major: u8,
    minor: u8,
    docrev: u8,
    max_table_size: u32,
    table_addr: u64,
}

/// Strictly parses an SMBIOS 3.0+ Entry Point: verifies `"_SM3_"`, `len >= 24`, and checksum.
pub fn parseSmbios3Entry(data: []const u8): Smbios3EntryPoint? {
    if (data.len < SMBIOS3_EP_SIZE) {
        return null;
    }
    if (!matchBytes(data, 0, SMBIOS3_ANCHOR)) {
        return null;
    }
    const ep_len = data[6] as usize;
    if (ep_len < SMBIOS3_EP_SIZE || ep_len > data.len) {
        return null;
    }
    if (smbiosChecksum(data[0..ep_len]) != 0) {
        return null;
    }

    return Smbios3EntryPoint{
        .major = data[7],
        .minor = data[8],
        .docrev = data[9],
        .max_table_size = readU32Le(data, 12),
        .table_addr = readU64Le(data, 16),
    };
}

/// Single parsed SMBIOS structure with zero-copy formatted and string-set slices.
pub struct SmbiosStruct {
    struct_type: u8,
    length: u8,
    handle: u16,
    formatted: []const u8,
    strings: []const u8,

    fn isEndOfTable(): bool {
        return _.struct_type == TYPE_END_OF_TABLE;
    }

    /// Returns the 1-based string from the unformatted string-set (`index == 0` returns `null`).
    fn stringAt(index: u8): ([]const u8)? {
        if (index == 0 || _.strings.len == 0) {
            return null;
        }
        let cur: u8 = 1;
        let pos: usize = 0;
        while (pos < _.strings.len) {
            let end = pos;
            while (end < _.strings.len && _.strings[end] != 0) {
                end += 1;
            }
            if (cur == index) {
                return _.strings[pos..end];
            }
            if (end >= _.strings.len) {
                break;
            }
            pos = end + 1;
            cur += 1;
        }
        return null;
    }
}

/// Zero-copy iterator over an SMBIOS structure table (`formatted` + `\0\0`-terminated string-set).
/// Stops cleanly after yielding Type 127 (End-of-Table) or reaching table end.
pub struct SmbiosTableIter {
    data: []const u8,
    offset: usize,
    done: bool,
    failed: bool,

    fn init(table: []const u8): SmbiosTableIter {
        return SmbiosTableIter{
            .data = table,
            .offset = 0,
            .done = false,
            .failed = false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn isDone(): bool {
        return _.done;
    }

    fn next(*_): SmbiosStruct? {
        if (_.done || _.failed || _.offset >= _.data.len) {
            return null;
        }
        if (_.offset + SMBIOS_STRUCT_HEADER_SIZE > _.data.len) {
            _.failed = true;
            return null;
        }
        const stype = _.data[_.offset];
        const length = _.data[_.offset + 1];
        const handle = readU16Le(_.data, _.offset + 2);
        const fmt_len = length as usize;
        if (fmt_len < SMBIOS_STRUCT_HEADER_SIZE || _.offset + fmt_len > _.data.len) {
            _.failed = true;
            return null;
        }

        const fmt_slice = _.data[_.offset .. _.offset + fmt_len];
        const str_start = _.offset + fmt_len;

        // Locate double-NUL ("\0\0") string-set terminator
        if (str_start + 2 > _.data.len) {
            _.failed = true;
            return null;
        }

        let str_end: usize = str_start;
        if (_.data[str_start] == 0 && _.data[str_start + 1] == 0) {
            // Empty string-set: immediately "\0\0"
            _.offset = str_start + 2;
        } else {
            let p = str_start;
            let found = false;
            while (p + 1 < _.data.len) {
                if (_.data[p] == 0 && _.data[p + 1] == 0) {
                    str_end = p + 1; // keep NUL of final string
                    _.offset = p + 2;
                    found = true;
                    break;
                }
                p += 1;
            }
            if (!found) {
                _.failed = true;
                return null;
            }
        }

        if (stype == TYPE_END_OF_TABLE) {
            _.done = true;
        }

        return SmbiosStruct{
            .struct_type = stype,
            .length = length,
            .handle = handle,
            .formatted = fmt_slice,
            .strings = _.data[str_start..str_end],
        };
    }
}

/// Typed view for SMBIOS Type 0 (BIOS Information).
pub struct SmbiosBiosInfo {
    vendor: []const u8,
    version: []const u8,
    release_date: []const u8,
}

/// Parses an SMBIOS Type 0 (BIOS Information) structure.
pub fn parseBiosInfo(s: SmbiosStruct): SmbiosBiosInfo? {
    if (s.struct_type != TYPE_BIOS || s.formatted.len < 9) {
        return null;
    }
    const empty = s.formatted[0..0];
    return SmbiosBiosInfo{
        .vendor = s.stringAt(s.formatted[4]) ?? empty,
        .version = s.stringAt(s.formatted[5]) ?? empty,
        .release_date = s.stringAt(s.formatted[8]) ?? empty,
    };
}

/// Typed view for SMBIOS Type 1 (System Information).
pub struct SmbiosSystemInfo {
    manufacturer: []const u8,
    product_name: []const u8,
    version: []const u8,
    serial_number: []const u8,
    uuid: [16]u8,
}

/// Parses an SMBIOS Type 1 (System Information) structure.
pub fn parseSystemInfo(s: SmbiosStruct): SmbiosSystemInfo? {
    if (s.struct_type != TYPE_SYSTEM || s.formatted.len < 8) {
        return null;
    }
    const empty = s.formatted[0..0];
    let uuid: [16]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    if (s.formatted.len >= 24) {
        let i: usize = 0;
        while (i < 16) {
            uuid[i] = s.formatted[8 + i];
            i += 1;
        }
    }
    return SmbiosSystemInfo{
        .manufacturer = s.stringAt(s.formatted[4]) ?? empty,
        .product_name = s.stringAt(s.formatted[5]) ?? empty,
        .version = s.stringAt(s.formatted[6]) ?? empty,
        .serial_number = s.stringAt(s.formatted[7]) ?? empty,
        .uuid = uuid,
    };
}

/// Typed view for SMBIOS Type 17 (Memory Device).
pub struct SmbiosMemoryDevice {
    size_mb: u32,
    speed_mts: u16,
    device_locator: []const u8,
    manufacturer: []const u8,
}

/// Parses an SMBIOS Type 17 (Memory Device) structure, handling KB/MB bit and 0x7FFF 32-bit extended MB size.
pub fn parseMemoryDevice(s: SmbiosStruct): SmbiosMemoryDevice? {
    if (s.struct_type != TYPE_MEMORY_DEVICE || s.formatted.len < 21) {
        return null;
    }
    const empty = s.formatted[0..0];
    const raw_size = readU16Le(s.formatted, 12);
    let size_mb: u32 = 0;
    if (raw_size == 0x0000 || raw_size == 0xFFFF) {
        size_mb = 0;
    } else if (raw_size == 0x7FFF) {
        if (s.formatted.len >= 32) {
            size_mb = readU32Le(s.formatted, 28) & 0x7FFFFFFF;
        }
    } else if ((raw_size & 0x8000) != 0) {
        // Bit 15 set: granularity is KB
        size_mb = ((raw_size & 0x7FFF) as u32) / 1024;
    } else {
        size_mb = raw_size as u32;
    }

    const speed_mts: u16 = if (s.formatted.len >= 23) readU16Le(s.formatted, 21) else 0;
    const locator = s.stringAt(s.formatted[16]) ?? empty;
    const mfg = if (s.formatted.len >= 24) (s.stringAt(s.formatted[23]) ?? empty) else empty;

    return SmbiosMemoryDevice{
        .size_mb = size_mb,
        .speed_mts = speed_mts,
        .device_locator = locator,
        .manufacturer = mfg,
    };
}
