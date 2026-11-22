// Origin: Pure-CP Stdlib (Zero-Alloc Microsoft exFAT Boot Sector, Checksum & 32B Directory Entry Parser) | Created: 2026-11-22

// exFAT Core Protocol Constants
pub const EXFAT_OEM_ID: []const u8 = "EXFAT   ";
pub const EXFAT_BOOT_SIGNATURE: u16 = 0xAA55;
pub const EXFAT_MIN_BOOT_SIZE: usize = 512;
pub const EXFAT_DIR_ENTRY_SIZE: usize = 32;

// Directory Entry Types
pub const ENTRY_EOD: u8 = 0x00;
pub const ENTRY_ALLOC_BITMAP: u8 = 0x81;
pub const ENTRY_UPCASE_TABLE: u8 = 0x82;
pub const ENTRY_VOLUME_LABEL: u8 = 0x83;
pub const ENTRY_FILE: u8 = 0x85;
pub const ENTRY_STREAM_EXT: u8 = 0xC0;
pub const ENTRY_FILE_NAME: u8 = 0xC1;

// File Attributes
pub const ATTR_READ_ONLY: u16 = 0x0001;
pub const ATTR_HIDDEN: u16 = 0x0002;
pub const ATTR_SYSTEM: u16 = 0x0004;
pub const ATTR_DIRECTORY: u16 = 0x0010;
pub const ATTR_ARCHIVE: u16 = 0x0020;

// Stream Extension GeneralSecondaryFlags
pub const STREAM_ALLOC_POSSIBLE: u8 = 0x01;
pub const STREAM_NO_FAT_CHAIN: u8 = 0x02;

// Volume Flags
pub const VOLUME_FLAG_ACTIVE_FAT: u16 = 0x0001;
pub const VOLUME_FLAG_DIRTY: u16 = 0x0002;
pub const VOLUME_FLAG_MEDIA_FAILURE: u16 = 0x0004;
pub const VOLUME_FLAG_CLEAR_TO_ZERO: u16 = 0x0008;

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

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// Parsed Microsoft exFAT Main Boot Sector (sector 0, >= 512 bytes).
pub struct ExfatBootSector {
    partition_offset: u64,
    volume_length: u64,
    fat_offset: u32,
    fat_length: u32,
    cluster_heap_offset: u32,
    cluster_count: u32,
    first_cluster_of_root: u32,
    volume_serial: u32,
    fs_revision: u16,
    volume_flags: u16,
    bytes_per_sector_shift: u8,
    sectors_per_cluster_shift: u8,
    number_of_fats: u8,
    percent_in_use: u8,

    /// Sector size in bytes (`1 << bytes_per_sector_shift`, 512..4096).
    fn bytesPerSector(): u32 {
        return (1 as u32) << (_.bytes_per_sector_shift as u5);
    }

    /// Sectors per cluster (`1 << sectors_per_cluster_shift`).
    fn sectorsPerCluster(): u32 {
        return (1 as u32) << (_.sectors_per_cluster_shift as u5);
    }

    /// Cluster size in bytes (`1 << (bytes_per_sector_shift + sectors_per_cluster_shift)`).
    fn clusterSizeBytes(): u32 {
        const total_shift = _.bytes_per_sector_shift + _.sectors_per_cluster_shift;
        return (1 as u32) << (total_shift as u5);
    }

    /// Converts a data cluster index (`>= 2` and `< cluster_count + 2`) to its starting volume LBA (sector).
    fn clusterToLba(cluster: u32): u64? {
        if (cluster < 2) {
            return null;
        }
        const rel = cluster - 2;
        if (rel >= _.cluster_count) {
            return null;
        }
        return (_.cluster_heap_offset as u64) + (rel as u64) * (_.sectorsPerCluster() as u64);
    }

    /// Returns `true` if `VolumeDirty` (bit 1 of `volume_flags`) is set.
    fn isDirty(): bool {
        return (_.volume_flags & VOLUME_FLAG_DIRTY) != 0;
    }
}

/// Returns `true` if `data` (>= 512B) has the `"EXFAT   "` OEM ID at offset 3..11 and `0xAA55` signature at 510..512.
pub fn isExfatBootSector(data: []const u8): bool {
    if (data.len < EXFAT_MIN_BOOT_SIZE) {
        return false;
    }
    return bytesEq(data[3..11], EXFAT_OEM_ID) && readU16Le(data, 510) == EXFAT_BOOT_SIGNATURE;
}

/// Parses and strictly validates an exFAT Main Boot Sector:
/// - `data.len >= 512`
/// - OEM ID `"EXFAT   "` at offset 3..11 and boot signature `0xAA55` at offset 510..512
/// - `9 <= bytes_per_sector_shift <= 12` (512..4096 B)
/// - `bytes_per_sector_shift + sectors_per_cluster_shift <= 25` (max 32 MiB cluster)
/// - `number_of_fats` in `1..=2`
pub fn parseExfatBootSector(sector0: []const u8): ExfatBootSector? {
    if (!isExfatBootSector(sector0)) {
        return null;
    }
    const bps_shift = sector0[108];
    const spc_shift = sector0[109];
    const num_fats = sector0[110];

    if (bps_shift < 9 || bps_shift > 12) {
        return null;
    }
    if ((bps_shift as u16) + (spc_shift as u16) > 25) {
        return null;
    }
    if (num_fats < 1 || num_fats > 2) {
        return null;
    }

    const partition_offset = readU64Le(sector0, 64);
    const volume_length = readU64Le(sector0, 72);
    const fat_offset = readU32Le(sector0, 80);
    const fat_length = readU32Le(sector0, 84);
    const cluster_heap_offset = readU32Le(sector0, 88);
    const cluster_count = readU32Le(sector0, 92);
    const first_cluster_of_root = readU32Le(sector0, 96);
    const volume_serial = readU32Le(sector0, 100);
    const fs_revision = readU16Le(sector0, 104);
    const volume_flags = readU16Le(sector0, 106);
    const percent_in_use = sector0[112];

    if (first_cluster_of_root < 2) {
        return null;
    }

    return ExfatBootSector{
        .partition_offset = partition_offset,
        .volume_length = volume_length,
        .fat_offset = fat_offset,
        .fat_length = fat_length,
        .cluster_heap_offset = cluster_heap_offset,
        .cluster_count = cluster_count,
        .first_cluster_of_root = first_cluster_of_root,
        .volume_serial = volume_serial,
        .fs_revision = fs_revision,
        .volume_flags = volume_flags,
        .bytes_per_sector_shift = bps_shift,
        .sectors_per_cluster_shift = spc_shift,
        .number_of_fats = num_fats,
        .percent_in_use = percent_in_use,
    };
}

/// Computes the Microsoft exFAT 32-bit rotate-and-add Boot Region Checksum over the first 11 sectors,
/// skipping volatile fields at byte indices `106`, `107` (`VolumeFlags`) and `112` (`PercentInUse`).
pub fn exfatBootChecksum(boot_region_11_sectors: []const u8): u32 {
    let cs: u32 = 0;
    let i: usize = 0;
    while (i < boot_region_11_sectors.len) {
        if (i != 106 && i != 107 && i != 112) {
            let hi: u64 = 0;
            if ((cs & 1) != 0) {
                hi = 0x80000000 as u64;
            }
            const next = (hi + ((cs >> 1) as u64) + (boot_region_11_sectors[i] as u64)) & 0xFFFFFFFF;
            cs = next as u32;
        }
        i += 1;
    }
    return cs;
}

/// Computes the Microsoft exFAT 16-bit rotate-and-add Directory Entry Set Checksum,
/// skipping the `SetChecksum` field at byte indices `2` and `3` of the primary entry.
pub fn exfatEntrySetChecksum(entries_bytes: []const u8): u16 {
    let cs: u16 = 0;
    let i: usize = 0;
    while (i < entries_bytes.len) {
        if (i != 2 && i != 3) {
            let hi: u32 = 0;
            if ((cs & 1) != 0) {
                hi = 0x8000 as u32;
            }
            const next = (hi + ((cs >> 1) as u32) + (entries_bytes[i] as u32)) & 0xFFFF;
            cs = next as u16;
        }
        i += 1;
    }
    return cs;
}

/// Parsed primary File Directory Entry (`EntryType == 0x85`).
pub struct ExfatFileEntry {
    secondary_count: u8,
    set_checksum: u16,
    file_attributes: u16,
    create_ts: u32,
    last_mod_ts: u32,
    last_acc_ts: u32,

    /// Returns `true` if `ATTR_DIRECTORY` (`0x0010`) is set.
    fn isDirectory(): bool {
        return (_.file_attributes & ATTR_DIRECTORY) != 0;
    }
}

/// Parses a 32-byte primary File Directory Entry (`0x85`).
pub fn parseFileEntry(raw: []const u8): ExfatFileEntry? {
    if (raw.len < EXFAT_DIR_ENTRY_SIZE || raw[0] != ENTRY_FILE) {
        return null;
    }
    const secondary_count = raw[1];
    if (secondary_count < 2) {
        return null;
    }
    return ExfatFileEntry{
        .secondary_count = secondary_count,
        .set_checksum = readU16Le(raw, 2),
        .file_attributes = readU16Le(raw, 4),
        .create_ts = readU32Le(raw, 8),
        .last_mod_ts = readU32Le(raw, 12),
        .last_acc_ts = readU32Le(raw, 16),
    };
}

/// Parsed Stream Extension Directory Entry (`EntryType == 0xC0`).
pub struct ExfatStreamExt {
    general_flags: u8,
    name_length: u8,
    name_hash: u16,
    valid_data_length: u64,
    first_cluster: u32,
    data_length: u64,

    /// Returns `true` if `STREAM_NO_FAT_CHAIN` (`0x02`) is set (contiguous cluster allocation).
    fn isContiguous(): bool {
        return (_.general_flags & STREAM_NO_FAT_CHAIN) != 0;
    }

    /// Returns `true` if `STREAM_ALLOC_POSSIBLE` (`0x01`) is set.
    fn isAllocationPossible(): bool {
        return (_.general_flags & STREAM_ALLOC_POSSIBLE) != 0;
    }
}

/// Parses a 32-byte Stream Extension Directory Entry (`0xC0`).
pub fn parseStreamExt(raw: []const u8): ExfatStreamExt? {
    if (raw.len < EXFAT_DIR_ENTRY_SIZE || raw[0] != ENTRY_STREAM_EXT) {
        return null;
    }
    return ExfatStreamExt{
        .general_flags = raw[1],
        .name_length = raw[3],
        .name_hash = readU16Le(raw, 4),
        .valid_data_length = readU64Le(raw, 8),
        .first_cluster = readU32Le(raw, 20),
        .data_length = readU64Le(raw, 24),
    };
}

/// Extracts up to 15 UTF-16LE characters from a 32-byte File Name Directory Entry (`0xC1`) into `out` as ASCII.
/// Non-ASCII code units (`> 0x7F`) are mapped to `'?'`. Stops on NUL (`0x0000`) or when `out` is full.
/// Returns the number of bytes written.
pub fn decodeFileNameAscii(raw: []const u8, out: []u8): usize {
    if (raw.len < EXFAT_DIR_ENTRY_SIZE || raw[0] != ENTRY_FILE_NAME) {
        return 0;
    }
    let written: usize = 0;
    let i: usize = 0;
    while (i < 15 && written < out.len) {
        const ch = readU16Le(raw, 2 + i * 2);
        if (ch == 0) {
            break;
        }
        if (ch <= 0x7F) {
            out[written] = ch as u8;
        } else {
            out[written] = 0x3F; // '?'
        }
        written += 1;
        i += 1;
    }
    return written;
}

/// Raw 32-byte exFAT directory entry snapshot.
pub struct ExfatRawEntry {
    entry_type: u8,
    raw: [32]u8,

    fn isInUse(): bool {
        return (_.entry_type & 0x80) != 0;
    }

    fn isFile(): bool {
        return _.entry_type == ENTRY_FILE;
    }

    fn isStreamExt(): bool {
        return _.entry_type == ENTRY_STREAM_EXT;
    }

    fn isFileName(): bool {
        return _.entry_type == ENTRY_FILE_NAME;
    }

    fn isVolumeLabel(): bool {
        return _.entry_type == ENTRY_VOLUME_LABEL;
    }
}

/// Zero-alloc iterator over 32-byte exFAT directory entries.
/// Stops on `ENTRY_EOD (0x00)` and skips deleted entries (where bit 7 `InUse` is 0).
pub struct ExfatDirIter {
    data: []const u8,
    pos: usize,
    done: bool,

    pub fn init(data: []const u8): ExfatDirIter {
        return ExfatDirIter{
            .data = data,
            .pos = 0,
            .done = false,
        };
    }

    fn next(*_): ExfatRawEntry? {
        while (!_.done && _.data.len - _.pos >= EXFAT_DIR_ENTRY_SIZE) {
            const off = _.pos;
            _.pos += EXFAT_DIR_ENTRY_SIZE;
            const entry_type = _.data[off];
            if (entry_type == ENTRY_EOD) {
                _.done = true;
                return null;
            }
            if ((entry_type & 0x80) == 0) {
                continue;
            }
            let raw: [32]u8 = [0; 32];
            let i: usize = 0;
            while (i < 32) {
                raw[i] = _.data[off + i];
                i += 1;
            }
            return ExfatRawEntry{
                .entry_type = entry_type,
                .raw = raw,
            };
        }
        _.done = true;
        return null;
    }
}
