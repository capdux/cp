// Origin: Pure-CP Stdlib (Zero-Alloc UEFI GPT & Protective MBR Parser + CRC32) | Created: 2026-11-22

pub const GPT_SIGNATURE: []const u8 = "EFI PART";
pub const GPT_REVISION_1_0: u32 = 0x00010000;
pub const GPT_MIN_HEADER_SIZE: usize = 92;
pub const GPT_MIN_ENTRY_SIZE: usize = 128;
pub const MBR_SECTOR_SIZE: usize = 512;
pub const MBR_BOOT_SIGNATURE: u16 = 0xAA55;
pub const MBR_TYPE_GPT_PROTECTIVE: u8 = 0xEE;

// Well-known UEFI Partition Type GUIDs (16-byte UEFI mixed-endian layout)
pub const GUID_UNUSED: [16]u8 = [
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
];
// C12A7328-F81F-11D2-BA4B-00A0C93EC93B
pub const GUID_EFI_SYSTEM: [16]u8 = [
    0x28, 0x73, 0x2A, 0xC1, 0x1F, 0xF8, 0xD2, 0x11,
    0xBA, 0x4B, 0x00, 0xA0, 0xC9, 0x3E, 0xC9, 0x3B,
];
// 0FC63DAF-8483-4772-8E79-3D69D8477DE4
pub const GUID_LINUX_FS: [16]u8 = [
    0xAF, 0x3D, 0xC6, 0x0F, 0x83, 0x84, 0x72, 0x47,
    0x8E, 0x79, 0x3D, 0x69, 0xD8, 0x47, 0x7D, 0xE4,
];
// 0657FD6D-A4AB-43C4-84E5-0933C84B4F4F
pub const GUID_LINUX_SWAP: [16]u8 = [
    0x6D, 0xFD, 0x57, 0x06, 0xAB, 0xA4, 0xC4, 0x43,
    0x84, 0xE5, 0x09, 0x33, 0xC8, 0x4B, 0x4F, 0x4F,
];
// EBD0A0A2-B9E5-4433-87C0-68B6B72699C7
pub const GUID_MS_BASIC_DATA: [16]u8 = [
    0xA2, 0xA0, 0xD0, 0xEB, 0xE5, 0xB9, 0x33, 0x44,
    0x87, 0xC0, 0x68, 0xB6, 0xB7, 0x26, 0x99, 0xC7,
];

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

fn copy72(src: []const u8, off: usize): [72]u8 {
    let out: [72]u8 = undefined;
    let i: usize = 0;
    while (i < 72) {
        out[i] = src[off + i];
        i += 1;
    }
    return out;
}

fn crc32Update(init_crc: u32, data: []const u8): u32 {
    let crc = init_crc;
    let i: usize = 0;
    while (i < data.len) {
        crc ^= data[i] as u32;
        let bit: usize = 0;
        while (bit < 8) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc;
}

/// Computes IEEE 802.3 CRC-32 (`0xEDB88320`, init `0xFFFFFFFF`, final XOR `0xFFFFFFFF`).
pub fn gptCrc32(data: []const u8): u32 {
    return crc32Update(0xFFFFFFFF, data) ^ 0xFFFFFFFF;
}

fn hexNibbleUpper(n: u8): u8 {
    if (n < 10) {
        return ('0' as u8) + n;
    }
    return ('A' as u8) + (n - 10);
}

fn writeHexByte(out: []u8, pos: usize, b: u8) {
    out[pos] = hexNibbleUpper((b >> 4) & 0x0F);
    out[pos + 1] = hexNibbleUpper(b & 0x0F);
}

/// Formats a 16-byte UEFI mixed-endian GUID into standard 36-byte uppercase UUID string
/// (`XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX`).
pub fn formatGuid(guid: [16]u8, out: []u8): usize? {
    if (out.len < 36) {
        return null;
    }
    // Data1 (4B LE: bytes 3, 2, 1, 0)
    writeHexByte(out, 0, guid[3]);
    writeHexByte(out, 2, guid[2]);
    writeHexByte(out, 4, guid[1]);
    writeHexByte(out, 6, guid[0]);
    out[8] = '-';

    // Data2 (2B LE: bytes 5, 4)
    writeHexByte(out, 9, guid[5]);
    writeHexByte(out, 11, guid[4]);
    out[13] = '-';

    // Data3 (2B LE: bytes 7, 6)
    writeHexByte(out, 14, guid[7]);
    writeHexByte(out, 16, guid[6]);
    out[18] = '-';

    // Data4a (2B BE: bytes 8, 9)
    writeHexByte(out, 19, guid[8]);
    writeHexByte(out, 21, guid[9]);
    out[23] = '-';

    // Data4b (6B BE: bytes 10..15)
    writeHexByte(out, 24, guid[10]);
    writeHexByte(out, 26, guid[11]);
    writeHexByte(out, 28, guid[12]);
    writeHexByte(out, 30, guid[13]);
    writeHexByte(out, 32, guid[14]);
    writeHexByte(out, 34, guid[15]);
    return 36;
}

/// Parsed 16-byte legacy MBR partition table record.
pub struct MbrPartition {
    boot_indicator: u8,
    part_type: u8,
    start_lba: u32,
    sector_count: u32,
}

pub type MbrPartitionTable = [4]MbrPartition;

/// Parses the 4 legacy MBR partition records from sector 0 (requires `len >= 512` and `0xAA55` signature).
pub fn parseMbrPartitions(sector0: []const u8): MbrPartitionTable? {
    if (sector0.len < MBR_SECTOR_SIZE) {
        return null;
    }
    if (readU16Le(sector0, 510) != MBR_BOOT_SIGNATURE) {
        return null;
    }
    let parts: MbrPartitionTable = undefined;
    let i: usize = 0;
    while (i < 4) {
        const base = 446 + i * 16;
        parts[i] = MbrPartition{
            .boot_indicator = sector0[base],
            .part_type = sector0[base + 4],
            .start_lba = readU32Le(sector0, base + 8),
            .sector_count = readU32Le(sector0, base + 12),
        };
        i += 1;
    }
    return parts;
}

/// Returns true if sector 0 is a valid UEFI Protective MBR (`0xEE` partition starting at LBA 1).
pub fn isProtectiveMbr(sector0: []const u8): bool {
    const parts = parseMbrPartitions(sector0) ?? return false;
    let has_protective = false;
    let i: usize = 0;
    while (i < 4) {
        const pt = parts[i].part_type;
        if (pt == MBR_TYPE_GPT_PROTECTIVE) {
            if (parts[i].start_lba == 1 && parts[i].sector_count > 0) {
                has_protective = true;
            }
        } else if (pt != 0) {
            return false;
        }
        i += 1;
    }
    return has_protective;
}

/// Parsed UEFI GPT Header (92+ bytes).
pub struct GptHeader {
    revision: u32,
    header_size: u32,
    header_crc32: u32,
    current_lba: u64,
    backup_lba: u64,
    first_usable_lba: u64,
    last_usable_lba: u64,
    disk_guid: [16]u8,
    part_entry_lba: u64,
    num_part_entries: u32,
    part_entry_size: u32,
    part_array_crc32: u32,
}

fn hasGptSignature(data: []const u8): bool {
    if (data.len < 8) {
        return false;
    }
    let i: usize = 0;
    while (i < 8) {
        if (data[i] != GPT_SIGNATURE[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Fast signature and minimum size check for a GPT Header.
pub fn isGptHeader(data: []const u8): bool {
    if (data.len < GPT_MIN_HEADER_SIZE) {
        return false;
    }
    return hasGptSignature(data);
}

/// Strictly parses a GPT Header: validates `"EFI PART"` signature, `header_size >= 92`,
/// `part_entry_size >= 128`, and verifies the IEEE CRC-32 with bytes 16..20 zeroed.
pub fn parseGptHeader(data: []const u8): GptHeader? {
    if (data.len < GPT_MIN_HEADER_SIZE) {
        return null;
    }
    if (!hasGptSignature(data)) {
        return null;
    }

    const revision = readU32Le(data, 8);
    const header_size = readU32Le(data, 12);
    const hdr_len = header_size as usize;
    if (hdr_len < GPT_MIN_HEADER_SIZE || hdr_len > data.len) {
        return null;
    }

    const header_crc32 = readU32Le(data, 16);
    const part_entry_size = readU32Le(data, 84);
    if ((part_entry_size as usize) < GPT_MIN_ENTRY_SIZE) {
        return null;
    }

    // Recompute IEEE CRC-32 over header[0..hdr_len] with header_crc32 (bytes 16..20) zeroed
    const zero4: [4]u8 = [0, 0, 0, 0];
    let crc = crc32Update(0xFFFFFFFF, data[0..16]);
    crc = crc32Update(crc, zero4[0..]);
    crc = crc32Update(crc, data[20..hdr_len]);
    const computed_crc = crc ^ 0xFFFFFFFF;
    if (computed_crc != header_crc32) {
        return null;
    }

    return GptHeader{
        .revision = revision,
        .header_size = header_size,
        .header_crc32 = header_crc32,
        .current_lba = readU64Le(data, 24),
        .backup_lba = readU64Le(data, 32),
        .first_usable_lba = readU64Le(data, 40),
        .last_usable_lba = readU64Le(data, 48),
        .disk_guid = copy16(data, 56),
        .part_entry_lba = readU64Le(data, 72),
        .num_part_entries = readU32Le(data, 80),
        .part_entry_size = part_entry_size,
        .part_array_crc32 = readU32Le(data, 88),
    };
}

/// Verifies the partition entry array CRC-32 against `header.part_array_crc32`.
pub fn verifyGptEntriesCrc(header: GptHeader, entries_bytes: []const u8): bool {
    const count = header.num_part_entries as usize;
    const esize = header.part_entry_size as usize;
    if (esize < GPT_MIN_ENTRY_SIZE) {
        return false;
    }
    const total = count * esize;
    if (entries_bytes.len < total) {
        return false;
    }
    return gptCrc32(entries_bytes[0..total]) == header.part_array_crc32;
}

/// Parsed 128-byte UEFI GPT Partition Entry.
pub struct GptEntry {
    type_guid: [16]u8,
    unique_guid: [16]u8,
    first_lba: u64,
    last_lba: u64,
    attributes: u64,
    name_utf16le: [72]u8,

    fn isUnused(): bool {
        let i: usize = 0;
        while (i < 16) {
            if (_.type_guid[i] != 0) {
                return false;
            }
            i += 1;
        }
        return true;
    }

    fn sectorCount(): u64 {
        if (_.isUnused() || _.last_lba < _.first_lba) {
            return 0;
        }
        return _.last_lba - _.first_lba + 1;
    }

    fn isRequired(): bool {
        return (_.attributes & 0x01) != 0;
    }

    fn isNoBlockIo(): bool {
        return (_.attributes & 0x02) != 0;
    }

    /// Decodes the UTF-16LE partition name (up to 36 code units, NUL-terminated) into ASCII.
    fn decodeNameAscii(out: []u8): usize {
        let out_pos: usize = 0;
        let i: usize = 0;
        while (i + 1 < 72 && out_pos < out.len) {
            const lo = _.name_utf16le[i] as u16;
            const hi = _.name_utf16le[i + 1] as u16;
            const cu = lo | (hi << 8);
            if (cu == 0) {
                break;
            }
            if (cu >= 0x20 && cu <= 0x7E) {
                out[out_pos] = cu as u8;
            } else {
                out[out_pos] = '?';
            }
            out_pos += 1;
            i += 2;
        }
        return out_pos;
    }
}

/// Zero-copy iterator over a GPT partition entry array.
pub struct GptEntryIter {
    data: []const u8,
    entry_size: usize,
    total_entries: u32,
    index: u32,
    skip_unused: bool,

    static fn init(entries_bytes: []const u8, num_entries: u32, entry_size: u32, skip_unused: bool): GptEntryIter {
        return GptEntryIter{
            .data = entries_bytes,
            .entry_size = entry_size as usize,
            .total_entries = num_entries,
            .index = 0,
            .skip_unused = skip_unused,
        };
    }

    fn initFromHeader(header: GptHeader, entries_bytes: []const u8, skip_unused: bool): GptEntryIter {
        return GptEntryIter{
            .data = entries_bytes,
            .entry_size = header.part_entry_size as usize,
            .total_entries = header.num_part_entries,
            .index = 0,
            .skip_unused = skip_unused,
        };
    }

    fn next(*_): GptEntry? {
        if (_.entry_size < GPT_MIN_ENTRY_SIZE) {
            return null;
        }
        while (_.index < _.total_entries) {
            const off = (_.index as usize) * _.entry_size;
            if (off + GPT_MIN_ENTRY_SIZE > _.data.len) {
                return null;
            }
            _.index += 1;

            const entry = GptEntry{
                .type_guid = copy16(_.data, off),
                .unique_guid = copy16(_.data, off + 16),
                .first_lba = readU64Le(_.data, off + 32),
                .last_lba = readU64Le(_.data, off + 40),
                .attributes = readU64Le(_.data, off + 48),
                .name_utf16le = copy72(_.data, off + 56),
            };
            if (_.skip_unused && entry.isUnused()) {
                continue;
            }
            return entry;
        }
        return null;
    }
}
