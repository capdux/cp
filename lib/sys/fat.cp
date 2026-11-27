// Origin: Pure-CP Stdlib (Zero-Alloc Microsoft FAT12/FAT16/FAT32 BPB & DirEntry Parser) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// Protocol Constants (Microsoft FAT12 / FAT16 / FAT32)
// ---------------------------------------------------------------------------

pub const FAT_BOOT_SECTOR_SIZE: usize = 512;
pub const FAT_BOOT_SIGNATURE: u16 = 0xAA55;
pub const FAT_DIR_ENTRY_SIZE: usize = 32;

// Directory Entry Attribute Flags
pub const ATTR_READ_ONLY: u8 = 0x01;
pub const ATTR_HIDDEN: u8 = 0x02;
pub const ATTR_SYSTEM: u8 = 0x04;
pub const ATTR_VOLUME_ID: u8 = 0x08;
pub const ATTR_DIRECTORY: u8 = 0x10;
pub const ATTR_ARCHIVE: u8 = 0x20;
pub const ATTR_LFN: u8 = 0x0F;

pub enum FatKind: u8 {
    fat12 = 12,
    fat16 = 16,
    fat32 = 32,
}

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

fn isValidSectorSize(bps: u16): bool {
    return bps == 512 || bps == 1024 || bps == 2048 || bps == 4096;
}

fn isPowerOfTwoU8(v: u8): bool {
    return v != 0 && (v & (v - 1)) == 0;
}

// ---------------------------------------------------------------------------
// DOS Date/Time & LFN Checksum
// ---------------------------------------------------------------------------

/// Decoded MS-DOS FAT date and time (2-second resolution, epoch 1980).
pub struct FatDateTime {
    year: u16,
    month: u8,
    day: u8,
    hour: u8,
    minute: u8,
    second: u8,
}

/// Decodes packed 16-bit DOS date and 16-bit DOS time fields.
pub fn decodeDosDateTime(date: u16, time: u16): FatDateTime {
    return FatDateTime {
        year: 1980 + ((date >> 9) & 0x7F),
        month: ((date >> 5) & 0x0F) as u8,
        day: (date & 0x1F) as u8,
        hour: ((time >> 11) & 0x1F) as u8,
        minute: ((time >> 5) & 0x3F) as u8,
        second: (((time & 0x1F) as u8) * 2),
    };
}

/// Computes the Microsoft FAT Long File Name (LFN) 8-bit rotate-and-add checksum over an 11-byte 8.3 name.
pub fn fatShortNameChecksum(short_name: [11]u8): u8 {
    let sum: u8 = 0;
    let i: usize = 0;
    while (i < 11) {
        const rot: u8 = ((sum & 0x01) << 7) | (sum >> 1);
        sum = (((rot as u16) + (short_name[i] as u16)) & 0xFF) as u8;
        i += 1;
    }
    return sum;
}

// ---------------------------------------------------------------------------
// BIOS Parameter Block (BPB)
// ---------------------------------------------------------------------------

/// Decoded FAT12 / FAT16 / FAT32 BIOS Parameter Block (BPB).
pub struct FatBpb {
    kind: FatKind,
    oem_name: [8]u8,
    bytes_per_sector: u16,
    sectors_per_cluster: u8,
    reserved_sectors: u16,
    num_fats: u8,
    root_entry_count: u16,
    total_sectors: u32,
    fat_size_sectors: u32,
    root_cluster: u32,
    volume_id: u32,
    volume_label: [11]u8,
    cluster_count: u32,

    /// LBA sector offset of the first File Allocation Table (FAT).
    fn fatStartLba(): u32 {
        return _.reserved_sectors as u32;
    }

    /// Number of sectors occupied by the fixed root directory (0 on FAT32).
    fn rootDirSectors(): u32 {
        const bps = _.bytes_per_sector as u32;
        return (((_.root_entry_count as u32) * 32) + (bps - 1)) / bps;
    }

    /// LBA sector offset of the first data sector (cluster 2).
    fn dataStartLba(): u32 {
        return (_.reserved_sectors as u32) + ((_.num_fats as u32) * _.fat_size_sectors) + _.rootDirSectors();
    }

    /// Converts a valid data cluster number (`2 .. cluster_count + 1`) to its starting LBA sector.
    fn clusterToLba(cluster: u32): u32? {
        if (cluster < 2 || cluster - 2 >= _.cluster_count) {
            return null;
        }
        return _.dataStartLba() + ((cluster - 2) * (_.sectors_per_cluster as u32));
    }

    /// Starting LBA of the root directory (fixed area for FAT12/16, `root_cluster` LBA for FAT32).
    fn rootDirStartLba(): u32 {
        if (_.kind == FatKind.fat32) {
            return _.clusterToLba(_.root_cluster) ?? _.dataStartLba();
        }
        return (_.reserved_sectors as u32) + ((_.num_fats as u32) * _.fat_size_sectors);
    }

    /// Bytes per allocation cluster (`bytes_per_sector * sectors_per_cluster`).
    fn clusterSizeBytes(): u32 {
        return (_.bytes_per_sector as u32) * (_.sectors_per_cluster as u32);
    }
}

/// Fast-path validation of a 512-byte FAT boot sector.
pub fn isFatBootSector(data: []const u8): bool {
    return parseFatBpb(data) != null;
}

/// Parses and validates a 512-byte FAT12/FAT16/FAT32 Boot Sector BPB per Microsoft fatgen103.
/// Strictly checks `0xAA55` boot signature, valid sector size `{512,1024,2048,4096}`,
/// power-of-two `sectors_per_cluster`, `reserved_sectors >= 1`, `num_fats >= 1`, `fat_size > 0`,
/// and classifies FAT12 (`<4085`), FAT16 (`<65525`), or FAT32 (`>=65525`) by cluster count.
pub fn parseFatBpb(sector0: []const u8): FatBpb? {
    if (sector0.len < FAT_BOOT_SECTOR_SIZE) {
        return null;
    }
    if (readU16Le(sector0, 510) != FAT_BOOT_SIGNATURE) {
        return null;
    }

    const bytes_per_sector = readU16Le(sector0, 11);
    if (!isValidSectorSize(bytes_per_sector)) {
        return null;
    }
    const sectors_per_cluster = sector0[13];
    if (!isPowerOfTwoU8(sectors_per_cluster)) {
        return null;
    }
    const reserved_sectors = readU16Le(sector0, 14);
    if (reserved_sectors == 0) {
        return null;
    }
    const num_fats = sector0[16];
    if (num_fats == 0) {
        return null;
    }

    const root_entry_count = readU16Le(sector0, 17);
    const tot_sec16 = readU16Le(sector0, 19);
    const fat_sz16 = readU16Le(sector0, 22);
    const tot_sec32 = readU32Le(sector0, 32);
    const fat_sz32 = readU32Le(sector0, 36);

    const total_sectors: u32 = if (tot_sec16 != 0) tot_sec16 as u32 else tot_sec32;
    const fat_size_sectors: u32 = if (fat_sz16 != 0) fat_sz16 as u32 else fat_sz32;
    if (total_sectors == 0 || fat_size_sectors == 0) {
        return null;
    }

    const bps_u32 = bytes_per_sector as u32;
    const root_dir_sectors = (((root_entry_count as u32) * 32) + (bps_u32 - 1)) / bps_u32;
    const fats_total = (num_fats as u64) * (fat_size_sectors as u64);
    const first_data_u64 = (reserved_sectors as u64) + fats_total + (root_dir_sectors as u64);
    if (first_data_u64 >= (total_sectors as u64)) {
        return null;
    }
    const data_sectors = (total_sectors as u64) - first_data_u64;
    const cluster_count = (data_sectors / (sectors_per_cluster as u64)) as u32;

    let kind = FatKind.fat32;
    if (cluster_count < 4085) {
        kind = FatKind.fat12;
    } else if (cluster_count < 65525) {
        kind = FatKind.fat16;
    }

    let oem_name: [8]u8 = [0; 8];
    let i: usize = 0;
    while (i < 8) {
        oem_name[i] = sector0[3 + i];
        i += 1;
    }

    let root_cluster: u32 = 0;
    let volume_id: u32 = 0;
    let volume_label: [11]u8 = [0; 11];
    if (kind == FatKind.fat32) {
        root_cluster = readU32Le(sector0, 44);
        if (root_cluster < 2) {
            return null;
        }
        volume_id = readU32Le(sector0, 67);
        let j: usize = 0;
        while (j < 11) {
            volume_label[j] = sector0[71 + j];
            j += 1;
        }
    } else {
        volume_id = readU32Le(sector0, 39);
        let j: usize = 0;
        while (j < 11) {
            volume_label[j] = sector0[43 + j];
            j += 1;
        }
    }

    return FatBpb {
        kind: kind,
        oem_name: oem_name,
        bytes_per_sector: bytes_per_sector,
        sectors_per_cluster: sectors_per_cluster,
        reserved_sectors: reserved_sectors,
        num_fats: num_fats,
        root_entry_count: root_entry_count,
        total_sectors: total_sectors,
        fat_size_sectors: fat_size_sectors,
        root_cluster: root_cluster,
        volume_id: volume_id,
        volume_label: volume_label,
        cluster_count: cluster_count,
    };
}

// ---------------------------------------------------------------------------
// 32-Byte Directory Entry & Zero-Alloc Iterator
// ---------------------------------------------------------------------------

/// Decoded 32-byte FAT directory entry.
pub struct FatDirEntry {
    short_name: [11]u8,
    attr: u8,
    first_cluster: u32,
    file_size: u32,
    wrt_date: u16,
    wrt_time: u16,

    fn isDirectory(): bool {
        return (_.attr & ATTR_DIRECTORY) != 0;
    }

    fn isVolumeId(): bool {
        return (_.attr & ATTR_VOLUME_ID) != 0 && !_.isLfn();
    }

    fn isLfn(): bool {
        return (_.attr & 0x3F) == ATTR_LFN;
    }

    fn isReadOnly(): bool {
        return (_.attr & ATTR_READ_ONLY) != 0;
    }

    fn isHidden(): bool {
        return (_.attr & ATTR_HIDDEN) != 0;
    }

    fn writeTime(): FatDateTime {
        return decodeDosDateTime(_.wrt_date, _.wrt_time);
    }

    /// Formats the 11-byte 8.3 short name into `out` (e.g. `"README  TXT"` -> `"README.TXT"`).
    /// Returns the number of bytes written.
    fn formatShortName(out: []u8): usize {
        // Trim trailing spaces from 8-byte base name
        let name_len: usize = 8;
        while (name_len > 0 && _.short_name[name_len - 1] == ' ') {
            name_len -= 1;
        }
        // Trim trailing spaces from 3-byte extension
        let ext_len: usize = 3;
        while (ext_len > 0 && _.short_name[8 + ext_len - 1] == ' ') {
            ext_len -= 1;
        }

        let pos: usize = 0;
        let i: usize = 0;
        while (i < name_len && pos < out.len) {
            const b = _.short_name[i];
            out[pos] = if (i == 0 && b == 0x05) 0xE5 else b;
            pos += 1;
            i += 1;
        }
        if (ext_len > 0 && pos < out.len) {
            out[pos] = '.';
            pos += 1;
            let j: usize = 0;
            while (j < ext_len && pos < out.len) {
                out[pos] = _.short_name[8 + j];
                pos += 1;
                j += 1;
            }
        }
        return pos;
    }
}

/// Zero-allocation iterator over 32-byte FAT directory entries in a directory sector/cluster buffer.
/// Terminates at first entry starting with `0x00`, skips `0xE5` deleted entries,
/// and optionally skips Long File Name (`ATTR_LFN`) entries when `skip_lfn == true`.
pub struct FatDirIter {
    data: []const u8,
    offset: usize,
    skip_lfn: bool,
    done: bool,

    pub static fn init(data: []const u8, skip_lfn: bool): FatDirIter {
        return FatDirIter {
            data: data,
            offset: 0,
            skip_lfn: skip_lfn,
            done: false,
        };
    }

    fn next(*_): FatDirEntry? {
        if (_.done) {
            return null;
        }
        while (_.offset + FAT_DIR_ENTRY_SIZE <= _.data.len) {
            const off = _.offset;
            _.offset += FAT_DIR_ENTRY_SIZE;

            const first_byte = _.data[off];
            if (first_byte == 0x00) {
                _.done = true;
                return null;
            }
            if (first_byte == 0xE5) {
                continue;
            }

            const attr = _.data[off + 11];
            if (_.skip_lfn && (attr & 0x3F) == ATTR_LFN) {
                continue;
            }

            let short_name: [11]u8 = [0; 11];
            let i: usize = 0;
            while (i < 11) {
                short_name[i] = _.data[off + i];
                i += 1;
            }
            const clus_hi = readU16Le(_.data, off + 20) as u32;
            const wrt_time = readU16Le(_.data, off + 22);
            const wrt_date = readU16Le(_.data, off + 24);
            const clus_lo = readU16Le(_.data, off + 26) as u32;
            const file_size = readU32Le(_.data, off + 28);

            return FatDirEntry {
                short_name: short_name,
                attr: attr,
                first_cluster: (clus_hi << 16) | clus_lo,
                file_size: file_size,
                wrt_date: wrt_date,
                wrt_time: wrt_time,
            };
        }
        _.done = true;
        return null;
    }
}
