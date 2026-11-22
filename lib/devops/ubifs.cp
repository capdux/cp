// Origin: Pure-CP Stdlib (Zero-Alloc Linux UBI EC/VID Header & UBIFS Node/Superblock/Master Parser) | Created: 2026-11-22

// ---------------------------------------------------------------------------
// UBI & UBIFS Protocol Constants (Linux ubi-media.h / ubifs-media.h)
// ---------------------------------------------------------------------------

/// UBI Erase Counter Header magic `"UBI#"` (big-endian `0x55424923`).
pub const UBI_EC_HDR_MAGIC: u32 = 0x55424923;
/// UBI Volume Identifier Header magic `"UBI!"` (big-endian `0x55424921`).
pub const UBI_VID_HDR_MAGIC: u32 = 0x55424921;
pub const UBI_HDR_SIZE: usize = 64;

/// UBIFS Common Node Header magic (little-endian `0x06101831`).
pub const UBIFS_NODE_MAGIC: u32 = 0x06101831;
pub const UBIFS_CH_SIZE: usize = 24;
pub const UBIFS_CRC32_INIT: u32 = 0xFFFFFFFF;

// UBIFS Node Types
pub const UBIFS_INO_NODE: u8 = 0;
pub const UBIFS_DATA_NODE: u8 = 1;
pub const UBIFS_DENT_NODE: u8 = 2;
pub const UBIFS_XENT_NODE: u8 = 3;
pub const UBIFS_TRUN_NODE: u8 = 4;
pub const UBIFS_PAD_NODE: u8 = 5;
pub const UBIFS_SB_NODE: u8 = 6;
pub const UBIFS_MST_NODE: u8 = 7;
pub const UBIFS_REF_NODE: u8 = 8;
pub const UBIFS_IDX_NODE: u8 = 9;
pub const UBIFS_CS_NODE: u8 = 10;
pub const UBIFS_ORPH_NODE: u8 = 11;

// UBIFS Compression Types
pub const COMPR_NONE: u16 = 0;
pub const COMPR_LZO: u16 = 1;
pub const COMPR_ZLIB: u16 = 2;
pub const COMPR_ZSTD: u16 = 3;

// ---------------------------------------------------------------------------
// Internal Endian Helpers
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

fn readBytes16(buf: []const u8, off: usize): [16]u8 {
    let out: [16]u8 = [0; 16];
    let i: usize = 0;
    while (i < 16) {
        out[i] = buf[off + i];
        i += 1;
    }
    return out;
}

// ---------------------------------------------------------------------------
// UBI / UBIFS CRC32 (IEEE 802.3 0xEDB88320, init 0xFFFFFFFF, no final XOR)
// ---------------------------------------------------------------------------

const CRC32_IEEE_POLY: u32 = 0xEDB88320;

/// Computes Linux UBI/UBIFS CRC32 (`crc32(UBIFS_CRC32_INIT, data)` with no final inversion).
pub fn ubifsCrc32(data: []const u8): u32 {
    let crc: u32 = UBIFS_CRC32_INIT;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc ^ (data[i] as u32);
        let bit: usize = 0;
        while (bit < 8) {
            if ((crc & 1) != 0) {
                crc = (crc >> 1) ^ CRC32_IEEE_POLY;
            } else {
                crc = crc >> 1;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc;
}

// ---------------------------------------------------------------------------
// Data Models
// ---------------------------------------------------------------------------

/// Parsed 64-byte big-endian UBI Erase Counter Header (`struct ubi_ec_hdr`).
pub struct UbiEcHeader {
    version: u8,
    ec: u64,
    vid_hdr_offset: u32,
    data_offset: u32,
    image_seq: u32,
    hdr_crc: u32,
}

/// Parsed 64-byte big-endian UBI Volume Identifier Header (`struct ubi_vid_hdr`).
pub struct UbiVidHeader {
    version: u8,
    vol_type: u8,
    copy_flag: u8,
    compat: u8,
    vol_id: u32,
    lnum: u32,
    data_size: u32,
    used_ebs: u32,
    data_pad: u32,
    data_crc: u32,
    sqnum: u64,
    hdr_crc: u32,
}

/// Parsed 24-byte little-endian UBIFS Common Node Header (`struct ubifs_ch`) plus payload slice.
pub struct UbifsCommonHeader {
    crc: u32,
    sqnum: u64,
    len: u32,
    node_type: u8,
    group_type: u8,
    body: []const u8,
}

/// Parsed UBIFS Superblock Node (`UBIFS_SB_NODE`, type 6).
pub struct UbifsSuperblock {
    ch: UbifsCommonHeader,
    key_hash: u8,
    key_fmt: u8,
    flags: u32,
    min_io_size: u32,
    leb_size: u32,
    leb_cnt: u32,
    max_leb_cnt: u32,
    max_bud_bytes: u64,
    log_lebs: u32,
    lpt_lebs: u32,
    orph_lebs: u32,
    jhead_cnt: u32,
    fanout: u32,
    lsave_cnt: u32,
    fmt_version: u32,
    default_compr: u16,
    uuid: [16]u8,

    /// Returns total filesystem capacity in bytes (`leb_cnt * leb_size`).
    fn totalBytes(): u64 {
        return (_.leb_cnt as u64) * (_.leb_size as u64);
    }
}

/// Parsed UBIFS Master Node (`UBIFS_MST_NODE`, type 7).
pub struct UbifsMasterNode {
    ch: UbifsCommonHeader,
    highest_inum: u64,
    cmt_no: u64,
    flags: u32,
    log_lnum: u32,
    root_lnum: u32,
    root_offs: u32,
    root_len: u32,
    total_free: u64,
    total_dirty: u64,
    total_used: u64,
}

// ---------------------------------------------------------------------------
// Parsing Functions
// ---------------------------------------------------------------------------

/// Parses and validates a 64-byte big-endian UBI EC Header (`"UBI#"`, `version == 1`, CRC32 over `[0..60]`).
pub fn parseUbiEcHeader(data: []const u8): UbiEcHeader? {
    if (data.len < UBI_HDR_SIZE) {
        return null;
    }
    if (readU32Be(data, 0) != UBI_EC_HDR_MAGIC) {
        return null;
    }
    const version = data[4];
    if (version != 1) {
        return null;
    }
    const hdr_crc = readU32Be(data, 60);
    if (ubifsCrc32(data[0..60]) != hdr_crc) {
        return null;
    }
    return UbiEcHeader {
        version: version,
        ec: readU64Be(data, 8),
        vid_hdr_offset: readU32Be(data, 16),
        data_offset: readU32Be(data, 20),
        image_seq: readU32Be(data, 24),
        hdr_crc: hdr_crc,
    };
}

/// Parses and validates a 64-byte big-endian UBI VID Header (`"UBI!"`, `version == 1`, CRC32 over `[0..60]`).
pub fn parseUbiVidHeader(data: []const u8): UbiVidHeader? {
    if (data.len < UBI_HDR_SIZE) {
        return null;
    }
    if (readU32Be(data, 0) != UBI_VID_HDR_MAGIC) {
        return null;
    }
    const version = data[4];
    if (version != 1) {
        return null;
    }
    const hdr_crc = readU32Be(data, 60);
    if (ubifsCrc32(data[0..60]) != hdr_crc) {
        return null;
    }
    return UbiVidHeader {
        version: version,
        vol_type: data[5],
        copy_flag: data[6],
        compat: data[7],
        vol_id: readU32Be(data, 8),
        lnum: readU32Be(data, 12),
        data_size: readU32Be(data, 20),
        used_ebs: readU32Be(data, 24),
        data_pad: readU32Be(data, 28),
        data_crc: readU32Be(data, 32),
        sqnum: readU64Be(data, 40),
        hdr_crc: hdr_crc,
    };
}

/// Returns `true` if `data` starts with the little-endian `UBIFS_NODE_MAGIC` (`0x06101831`).
pub fn isUbifsNode(data: []const u8): bool {
    if (data.len < 4) {
        return false;
    }
    return readU32Le(data, 0) == UBIFS_NODE_MAGIC;
}

/// Parses and validates a UBIFS Common Node Header (`0x06101831`, `24 <= len <= data.len`,
/// and `ubifsCrc32(data[8..len]) == crc`).
pub fn parseUbifsNode(data: []const u8): UbifsCommonHeader? {
    if (data.len < UBIFS_CH_SIZE) {
        return null;
    }
    if (readU32Le(data, 0) != UBIFS_NODE_MAGIC) {
        return null;
    }
    const crc = readU32Le(data, 4);
    const sqnum = readU64Le(data, 8);
    const len = readU32Le(data, 16);
    if ((len as usize) < UBIFS_CH_SIZE || (len as usize) > data.len) {
        return null;
    }
    const node_len: usize = len as usize;
    if (ubifsCrc32(data[8..node_len]) != crc) {
        return null;
    }
    return UbifsCommonHeader {
        crc: crc,
        sqnum: sqnum,
        len: len,
        node_type: data[20],
        group_type: data[21],
        body: data[UBIFS_CH_SIZE..node_len],
    };
}

/// Parses and validates a UBIFS Superblock Node (`UBIFS_SB_NODE`, type 6).
pub fn parseUbifsSuperblock(data: []const u8): UbifsSuperblock? {
    const ch = parseUbifsNode(data) ?? return null;
    if (ch.node_type != UBIFS_SB_NODE || (ch.len as usize) < 124) {
        return null;
    }
    return UbifsSuperblock {
        ch: ch,
        key_hash: data[26],
        key_fmt: data[27],
        flags: readU32Le(data, 28),
        min_io_size: readU32Le(data, 32),
        leb_size: readU32Le(data, 36),
        leb_cnt: readU32Le(data, 40),
        max_leb_cnt: readU32Le(data, 44),
        max_bud_bytes: readU64Le(data, 48),
        log_lebs: readU32Le(data, 56),
        lpt_lebs: readU32Le(data, 60),
        orph_lebs: readU32Le(data, 64),
        jhead_cnt: readU32Le(data, 68),
        fanout: readU32Le(data, 72),
        lsave_cnt: readU32Le(data, 76),
        fmt_version: readU32Le(data, 80),
        default_compr: readU16Le(data, 84),
        uuid: readBytes16(data, 108),
    };
}

/// Parses and validates a UBIFS Master Node (`UBIFS_MST_NODE`, type 7).
pub fn parseUbifsMaster(data: []const u8): UbifsMasterNode? {
    const ch = parseUbifsNode(data) ?? return null;
    if (ch.node_type != UBIFS_MST_NODE || (ch.len as usize) < 104) {
        return null;
    }
    return UbifsMasterNode {
        ch: ch,
        highest_inum: readU64Le(data, 24),
        cmt_no: readU64Le(data, 32),
        flags: readU32Le(data, 40),
        log_lnum: readU32Le(data, 44),
        root_lnum: readU32Le(data, 48),
        root_offs: readU32Le(data, 52),
        root_len: readU32Le(data, 56),
        total_free: readU64Le(data, 80),
        total_dirty: readU64Le(data, 88),
        total_used: readU64Le(data, 96),
    };
}
