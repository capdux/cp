// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Microsoft NTFS VBR, MFT & Data Runlist Parser)

// ---------------------------------------------------------------------------
// Protocol Constants (Microsoft NTFS)
// ---------------------------------------------------------------------------

pub const NTFS_VBR_SIZE: usize = 512;
pub const NTFS_OEM_ID: []const u8 = "NTFS    ";
pub const NTFS_BOOT_SIGNATURE: u16 = 0xAA55;
pub const NTFS_FILE_MAGIC: []const u8 = "FILE";
pub const NTFS_ATTR_END: u32 = 0xFFFFFFFF;

// MFT Record Flags
pub const MFT_FLAG_IN_USE: u16 = 0x0001;
pub const MFT_FLAG_DIRECTORY: u16 = 0x0002;

// NTFS Attribute Type Codes
pub const ATTR_STANDARD_INFO: u32 = 0x10;
pub const ATTR_ATTR_LIST: u32 = 0x20;
pub const ATTR_FILE_NAME: u32 = 0x30;
pub const ATTR_VOLUME_NAME: u32 = 0x60;
pub const ATTR_VOLUME_INFO: u32 = 0x70;
pub const ATTR_DATA: u32 = 0x80;
pub const ATTR_INDEX_ROOT: u32 = 0x90;
pub const ATTR_INDEX_ALLOC: u32 = 0xA0;
pub const ATTR_BITMAP: u32 = 0xB0;
pub const ATTR_REPARSE_POINT: u32 = 0xC0;

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

fn isValidSectorSize(bps: u16): bool {
    return bps == 512 || bps == 1024 || bps == 2048 || bps == 4096;
}

fn isPowerOfTwoU8(v: u8): bool {
    return v != 0 && (v & (v - 1)) == 0;
}

// ---------------------------------------------------------------------------
// NTFS Volume Boot Record (VBR)
// ---------------------------------------------------------------------------

/// Decoded 512-byte NTFS Volume Boot Record (VBR).
pub struct NtfsVbr {
    bytes_per_sector: u16,
    sectors_per_cluster: u8,
    total_sectors: u64,
    mft_lcn: u64,
    mftmirr_lcn: u64,
    clusters_per_mft_record: i8,
    clusters_per_index_block: i8,
    volume_serial: u64,

    /// Bytes per allocation cluster (`bytes_per_sector * sectors_per_cluster`).
    fn clusterSizeBytes(): u32 {
        return (_.bytes_per_sector as u32) * (_.sectors_per_cluster as u32);
    }

    /// MFT record size in bytes. Negative values mean `1 << (-v)` (e.g. `-10 -> 1024`).
    fn mftRecordSizeBytes(): u32 {
        if (_.clusters_per_mft_record < 0) {
            const neg = (-(_.clusters_per_mft_record as i32)) as u32;
            if (neg > 30) {
                return 0;
            }
            return (1 as u32) << (neg as u5);
        }
        return (_.clusters_per_mft_record as u32) * _.clusterSizeBytes();
    }

    /// Byte offset of `$MFT` from the start of the volume (`mft_lcn * clusterSizeBytes`).
    fn mftByteOffset(): u64 {
        return _.mft_lcn * (_.clusterSizeBytes() as u64);
    }
}

/// Fast-path validation of a 512-byte NTFS Volume Boot Record.
pub fn isNtfsVbr(data: []const u8): bool {
    return parseNtfsVbr(data) != null;
}

/// Parses and validates a 512-byte NTFS Volume Boot Record (sector 0).
pub fn parseNtfsVbr(sector0: []const u8): NtfsVbr? {
    if (sector0.len < NTFS_VBR_SIZE) {
        return null;
    }
    if (!bytesEq(sector0[3..11], NTFS_OEM_ID)) {
        return null;
    }
    if (readU16Le(sector0, 510) != NTFS_BOOT_SIGNATURE) {
        return null;
    }
    const bps = readU16Le(sector0, 11);
    if (!isValidSectorSize(bps)) {
        return null;
    }
    const spc = sector0[13];
    if (!isPowerOfTwoU8(spc)) {
        return null;
    }
    const total_sectors = readU64Le(sector0, 40);
    if (total_sectors == 0) {
        return null;
    }
    const mft_lcn = readU64Le(sector0, 48);
    const mftmirr_lcn = readU64Le(sector0, 56);
    const c_mft: i8 = @bitCast(sector0[64]);
    if (c_mft == 0) {
        return null;
    }
    const c_idx: i8 = @bitCast(sector0[68]);
    const volume_serial = readU64Le(sector0, 72);

    return NtfsVbr {
        bytes_per_sector: bps,
        sectors_per_cluster: spc,
        total_sectors: total_sectors,
        mft_lcn: mft_lcn,
        mftmirr_lcn: mftmirr_lcn,
        clusters_per_mft_record: c_mft,
        clusters_per_index_block: c_idx,
        volume_serial: volume_serial,
    };
}

// ---------------------------------------------------------------------------
// NTFS Data Runlist Iterator
// ---------------------------------------------------------------------------

/// A single decoded NTFS Data Run extent.
pub struct NtfsDataRun {
    cluster_count: u64,
    lcn: i64,
    is_sparse: bool,
}

/// Zero-allocation iterator decoding NTFS variable-length Data Runlists.
/// Header byte: `len_sz = hdr & 0x0F` (unsigned cluster count), `off_sz = hdr >> 4` (signed delta LCN; `0` = sparse).
pub struct NtfsRunIter {
    data: []const u8,
    offset: usize,
    prev_lcn: i64,
    done: bool,
    failed: bool,

    pub fn init(runlist_bytes: []const u8): NtfsRunIter {
        return NtfsRunIter {
            data: runlist_bytes,
            offset: 0,
            prev_lcn: 0,
            done: false,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): NtfsDataRun? {
        if (_.done || _.failed || _.offset >= _.data.len) {
            _.done = true;
            return null;
        }
        const hdr = _.data[_.offset];
        if (hdr == 0) {
            _.done = true;
            return null;
        }
        const len_sz = (hdr & 0x0F) as usize;
        const off_sz = ((hdr >> 4) & 0x0F) as usize;
        if (len_sz == 0 || len_sz > 8 || off_sz > 8) {
            _.failed = true;
            return null;
        }
        if (_.offset + 1 + len_sz + off_sz > _.data.len) {
            _.failed = true;
            return null;
        }

        // Decode unsigned little-endian cluster_count
        let cluster_count: u64 = 0;
        let i: usize = 0;
        while (i < len_sz) {
            const b = _.data[_.offset + 1 + i] as u64;
            cluster_count = cluster_count | (b << ((i * 8) as u6));
            i += 1;
        }

        if (off_sz == 0) {
            _.offset += 1 + len_sz;
            return NtfsDataRun {
                cluster_count: cluster_count,
                lcn: 0,
                is_sparse: true,
            };
        }

        // Decode signed little-endian delta LCN
        let raw_delta: u64 = 0;
        let j: usize = 0;
        while (j < off_sz) {
            const b = _.data[_.offset + 1 + len_sz + j] as u64;
            raw_delta = raw_delta | (b << ((j * 8) as u6));
            j += 1;
        }
        // Sign-extend if MSB of highest delta byte is 1
        const msb_byte = _.data[_.offset + 1 + len_sz + off_sz - 1];
        if ((msb_byte & 0x80) != 0 && off_sz < 8) {
            let k: usize = off_sz;
            while (k < 8) {
                raw_delta = raw_delta | ((0xFF as u64) << ((k * 8) as u6));
                k += 1;
            }
        }
        const delta: i64 = @bitCast(raw_delta);
        _.prev_lcn = _.prev_lcn + delta;
        _.offset += 1 + len_sz + off_sz;

        return NtfsDataRun {
            cluster_count: cluster_count,
            lcn: _.prev_lcn,
            is_sparse: false,
        };
    }
}

// ---------------------------------------------------------------------------
// NTFS Attribute & MFT Record Models
// ---------------------------------------------------------------------------

/// Zero-copy view of a resident or non-resident NTFS MFT attribute.
pub struct NtfsAttr {
    attr_type: u32,
    total_len: u32,
    non_resident: bool,
    name_utf16le: []const u8,
    flags: u16,
    attr_id: u16,
    resident_value: []const u8,
    real_size: u64,
    alloc_size: u64,
    runlist_bytes: []const u8,

    /// Returns a zero-allocation Data Runlist iterator for non-resident attributes.
    fn runs(): NtfsRunIter {
        return NtfsRunIter.init(_.runlist_bytes);
    }
}

/// Zero-allocation iterator over attributes inside an NTFS MFT `"FILE"` record.
pub struct NtfsAttrIter {
    raw: []const u8,
    offset: usize,
    limit: usize,
    done: bool,
    failed: bool,

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): NtfsAttr? {
        if (_.done || _.failed) {
            return null;
        }
        if (_.offset + 4 > _.limit) {
            _.failed = true;
            return null;
        }
        const attr_type = readU32Le(_.raw, _.offset);
        if (attr_type == NTFS_ATTR_END) {
            _.done = true;
            return null;
        }
        if (_.offset + 16 > _.limit) {
            _.failed = true;
            return null;
        }
        const total_len = readU32Le(_.raw, _.offset + 4);
        if (total_len < 16 || (total_len as usize) > _.limit - _.offset) {
            _.failed = true;
            return null;
        }

        const attr_slice = _.raw[_.offset.._.offset + (total_len as usize)];
        const non_resident = attr_slice[8] != 0;
        const name_len = attr_slice[9] as usize;
        const name_offset = readU16Le(attr_slice, 10) as usize;
        const flags = readU16Le(attr_slice, 12);
        const attr_id = readU16Le(attr_slice, 14);

        let name_utf16le: []const u8 = "";
        if (name_len > 0) {
            const name_bytes = name_len * 2;
            if (name_offset > attr_slice.len || name_bytes > attr_slice.len - name_offset) {
                _.failed = true;
                return null;
            }
            name_utf16le = attr_slice[name_offset..name_offset + name_bytes];
        }

        let resident_value: []const u8 = "";
        let real_size: u64 = 0;
        let alloc_size: u64 = 0;
        let runlist_bytes: []const u8 = "";

        if (!non_resident) {
            if (attr_slice.len < 24) {
                _.failed = true;
                return null;
            }
            const val_len = readU32Le(attr_slice, 16) as usize;
            const val_off = readU16Le(attr_slice, 20) as usize;
            if (val_off > attr_slice.len || val_len > attr_slice.len - val_off) {
                _.failed = true;
                return null;
            }
            resident_value = attr_slice[val_off..val_off + val_len];
            real_size = val_len as u64;
            alloc_size = val_len as u64;
        } else {
            if (attr_slice.len < 64) {
                _.failed = true;
                return null;
            }
            const run_off = readU16Le(attr_slice, 32) as usize;
            alloc_size = readU64Le(attr_slice, 40);
            real_size = readU64Le(attr_slice, 48);
            if (run_off > attr_slice.len) {
                _.failed = true;
                return null;
            }
            runlist_bytes = attr_slice[run_off..attr_slice.len];
        }

        _.offset += total_len as usize;
        return NtfsAttr {
            attr_type: attr_type,
            total_len: total_len,
            non_resident: non_resident,
            name_utf16le: name_utf16le,
            flags: flags,
            attr_id: attr_id,
            resident_value: resident_value,
            real_size: real_size,
            alloc_size: alloc_size,
            runlist_bytes: runlist_bytes,
        };
    }
}

/// Zero-copy view of a parsed NTFS MFT `"FILE"` record.
pub struct NtfsMftRecord {
    usa_offset: u16,
    usa_count: u16,
    lsn: u64,
    seq_num: u16,
    hard_link_count: u16,
    first_attr_offset: u16,
    flags: u16,
    used_size: u32,
    alloc_size: u32,
    base_record: u64,
    raw: []const u8,

    fn isInUse(): bool {
        return (_.flags & MFT_FLAG_IN_USE) != 0;
    }

    fn isDirectory(): bool {
        return (_.flags & MFT_FLAG_DIRECTORY) != 0;
    }

    fn attrs(): NtfsAttrIter {
        const limit: usize = if ((_.used_size as usize) <= _.raw.len) _.used_size as usize else _.raw.len;
        return NtfsAttrIter {
            raw: _.raw,
            offset: _.first_attr_offset as usize,
            limit: limit,
            done: false,
            failed: false,
        };
    }
}

/// Parses an NTFS MFT `"FILE"` record header (validating `"FILE"` magic and attribute bounds).
pub fn parseNtfsMftRecord(record_bytes: []const u8): NtfsMftRecord? {
    if (record_bytes.len < 48) {
        return null;
    }
    if (!bytesEq(record_bytes[0..4], NTFS_FILE_MAGIC)) {
        return null;
    }
    const usa_offset = readU16Le(record_bytes, 4);
    const usa_count = readU16Le(record_bytes, 6);
    const lsn = readU64Le(record_bytes, 8);
    const seq_num = readU16Le(record_bytes, 16);
    const hard_link_count = readU16Le(record_bytes, 18);
    const first_attr_offset = readU16Le(record_bytes, 20);
    const flags = readU16Le(record_bytes, 22);
    const used_size = readU32Le(record_bytes, 24);
    const alloc_size = readU32Le(record_bytes, 28);
    const base_record = readU64Le(record_bytes, 32);

    if (first_attr_offset < 42 || (used_size as usize) > record_bytes.len || (first_attr_offset as usize) > (used_size as usize)) {
        return null;
    }

    return NtfsMftRecord {
        usa_offset: usa_offset,
        usa_count: usa_count,
        lsn: lsn,
        seq_num: seq_num,
        hard_link_count: hard_link_count,
        first_attr_offset: first_attr_offset,
        flags: flags,
        used_size: used_size,
        alloc_size: alloc_size,
        base_record: base_record,
        raw: record_bytes,
    };
}
