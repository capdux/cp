// Origin: Pure-CP Stdlib (Zero-Alloc ECMA-167 / ISO 13346 UDF Parser & OSTA CS0 Decoder) | Created: 2026-11-22

// UDF Core Protocol Constants
pub const UDF_SECTOR_SIZE: usize = 2048;
pub const UDF_VRS_OFFSET: usize = 32768;
pub const UDF_AVDP_SECTOR: u32 = 256;
pub const UDF_TAG_SIZE: usize = 16;

// Volume Recognition Sequence (VRS) Standard Identifiers
pub const VRS_BEA01: []const u8 = "BEA01";
pub const VRS_NSR02: []const u8 = "NSR02";
pub const VRS_NSR03: []const u8 = "NSR03";
pub const VRS_TEA01: []const u8 = "TEA01";

// ECMA-167 Descriptor Tag Identifiers
pub const TAG_PVD: u16 = 1;
pub const TAG_AVDP: u16 = 2;
pub const TAG_VDP: u16 = 3;
pub const TAG_IUVD: u16 = 4;
pub const TAG_PD: u16 = 5;
pub const TAG_LVD: u16 = 6;
pub const TAG_USD: u16 = 7;
pub const TAG_TD: u16 = 8;
pub const TAG_LVID: u16 = 9;
pub const TAG_FSD: u16 = 256;
pub const TAG_FID: u16 = 257;
pub const TAG_FE: u16 = 261;
pub const TAG_EFE: u16 = 266;

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

fn copySlice(dst: []u8, src: []const u8, src_off: usize) {
    let i: usize = 0;
    while (i < dst.len) {
        dst[i] = src[src_off + i];
        i += 1;
    }
}

/// Computes the ECMA-167 8-bit Descriptor Tag checksum over the first 16 bytes,
/// skipping byte index 4 (`TagChecksum` itself), modulo 256.
pub fn udfTagChecksum(tag16: []const u8): u8 {
    if (tag16.len < UDF_TAG_SIZE) {
        return 0;
    }
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < UDF_TAG_SIZE) {
        if (i != 4) {
            sum += tag16[i] as u32;
        }
        i += 1;
    }
    return (sum & 0xFF) as u8;
}

/// Computes the ECMA-167 CRC-ITU-T (polynomial `0x1021`, non-reflected, init `0x0000`, xorout `0x0000`).
pub fn udfCrc16(data: []const u8): u16 {
    let crc: u16 = 0;
    let i: usize = 0;
    while (i < data.len) {
        crc = crc ^ ((data[i] as u16) << 8);
        let bit: usize = 0;
        while (bit < 8) {
            if ((crc & 0x8000) != 0) {
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF;
            } else {
                crc = (crc << 1) & 0xFFFF;
            }
            bit += 1;
        }
        i += 1;
    }
    return crc;
}

/// ECMA-167 Extent Allocation Descriptor (`extent_ad`: 8 bytes LE).
pub struct UdfExtentAd {
    length: u32,
    location: u32,
}

/// Parsed 16-byte ECMA-167 Descriptor Tag.
pub struct UdfTag {
    tag_id: u16,
    desc_version: u16,
    checksum: u8,
    serial_num: u16,
    desc_crc: u16,
    desc_crc_len: u16,
    tag_location: u32,
}

/// Parses and validates a 16-byte ECMA-167 Descriptor Tag.
/// Strictly checks `udfTagChecksum` and, when `16 + desc_crc_len <= data.len`,
/// verifies `udfCrc16` over the descriptor body matches `desc_crc`.
pub fn parseUdfTag(data: []const u8): UdfTag? {
    if (data.len < UDF_TAG_SIZE) {
        return null;
    }
    const checksum = data[4];
    if (udfTagChecksum(data[0..UDF_TAG_SIZE]) != checksum) {
        return null;
    }
    const tag_id = readU16Le(data, 0);
    const desc_version = readU16Le(data, 2);
    const serial_num = readU16Le(data, 6);
    const desc_crc = readU16Le(data, 8);
    const desc_crc_len = readU16Le(data, 10);
    const tag_location = readU32Le(data, 12);

    const crc_len = desc_crc_len as usize;
    if (crc_len > 0 && UDF_TAG_SIZE + crc_len <= data.len) {
        const calc_crc = udfCrc16(data[UDF_TAG_SIZE..(UDF_TAG_SIZE + crc_len)]);
        if (calc_crc != desc_crc) {
            return null;
        }
    }

    return UdfTag{
        .tag_id = tag_id,
        .desc_version = desc_version,
        .checksum = checksum,
        .serial_num = serial_num,
        .desc_crc = desc_crc,
        .desc_crc_len = desc_crc_len,
        .tag_location = tag_location,
    };
}

/// Parsed Volume Recognition Sequence (VRS) descriptor entry (at 2048B sector boundary from offset 32768).
pub struct UdfVrsEntry {
    struct_type: u8,
    ident: [5]u8,
    version: u8,

    fn isBea01(): bool {
        return bytesEq(_.ident[0..], VRS_BEA01);
    }

    fn isNsr02(): bool {
        return bytesEq(_.ident[0..], VRS_NSR02);
    }

    fn isNsr03(): bool {
        return bytesEq(_.ident[0..], VRS_NSR03);
    }

    fn isTea01(): bool {
        return bytesEq(_.ident[0..], VRS_TEA01);
    }
}

/// Parses a VRS entry from a 2048-byte sector (validates `struct_type == 0` and `version == 1`).
pub fn parseUdfVrsEntry(sector2k: []const u8): UdfVrsEntry? {
    if (sector2k.len < 7) {
        return null;
    }
    const struct_type = sector2k[0];
    const version = sector2k[6];
    if (struct_type != 0 || version != 1) {
        return null;
    }
    let ident: [5]u8 = [0; 5];
    copySlice(ident[0..], sector2k, 1);
    return UdfVrsEntry{
        .struct_type = struct_type,
        .ident = ident,
        .version = version,
    };
}

/// Parsed Anchor Volume Descriptor Pointer (AVDP, Tag ID 2).
pub struct UdfAvdp {
    tag: UdfTag,
    main_vds: UdfExtentAd,
    reserve_vds: UdfExtentAd,
}

/// Parses an Anchor Volume Descriptor Pointer (AVDP) and verifies `tag_id == TAG_AVDP (2)`.
pub fn parseUdfAvdp(sector: []const u8): UdfAvdp? {
    if (sector.len < 32) {
        return null;
    }
    const tag = parseUdfTag(sector) ?? return null;
    if (tag.tag_id != TAG_AVDP) {
        return null;
    }
    return UdfAvdp{
        .tag = tag,
        .main_vds = UdfExtentAd{
            .length = readU32Le(sector, 16),
            .location = readU32Le(sector, 20),
        },
        .reserve_vds = UdfExtentAd{
            .length = readU32Le(sector, 24),
            .location = readU32Le(sector, 28),
        },
    };
}

/// Decodes an OSTA UDF CS0 compressed Unicode `dstring` into `out` as ASCII.
/// - Last byte of `dstring` holds the byte length (including the 1-byte `comp_id`).
/// - `comp_id == 8`: 8-bit characters; `comp_id == 16`: big-endian UTF-16BE characters.
/// - Non-ASCII code points (`> 0x7F`) are mapped to `'?'`.
/// Returns the number of bytes written, or `null` if `dstring` is malformed.
pub fn decodeOstaCs0Ascii(dstring: []const u8, out: []u8): usize? {
    if (dstring.len == 0) {
        return null;
    }
    const data_len = dstring[dstring.len - 1] as usize;
    if (data_len == 0) {
        return 0;
    }
    if (data_len >= dstring.len) {
        return null;
    }
    const comp_id = dstring[0];
    if (comp_id == 8) {
        let written: usize = 0;
        let i: usize = 1;
        while (i < data_len && written < out.len) {
            const c = dstring[i];
            if (c <= 0x7F) {
                out[written] = c;
            } else {
                out[written] = 0x3F; // '?'
            }
            written += 1;
            i += 1;
        }
        return written;
    } else if (comp_id == 16) {
        const payload_bytes = data_len - 1;
        if ((payload_bytes & 1) != 0) {
            return null;
        }
        let written: usize = 0;
        let i: usize = 1;
        while (i + 1 < data_len && written < out.len) {
            const ch = ((dstring[i] as u16) << 8) | (dstring[i + 1] as u16);
            if (ch <= 0x7F) {
                out[written] = ch as u8;
            } else {
                out[written] = 0x3F; // '?'
            }
            written += 1;
            i += 2;
        }
        return written;
    }
    return null;
}

/// Parsed Primary Volume Descriptor (PVD, Tag ID 1).
pub struct UdfPvd {
    tag: UdfTag,
    vds_seq_num: u32,
    pvd_num: u32,
    vol_id_raw: [32]u8,
    vol_seq_num: u16,
    max_vol_seq_num: u16,
    interchange_level: u16,
    vol_set_id_raw: [128]u8,

    /// Decodes the 32-byte OSTA CS0 `vol_id_raw` dstring into `out` as ASCII.
    fn decodeVolIdAscii(out: []u8): usize {
        return decodeOstaCs0Ascii(_.vol_id_raw[0..], out) ?? 0;
    }
}

/// Parses a Primary Volume Descriptor (PVD) and verifies `tag_id == TAG_PVD (1)`.
pub fn parseUdfPvd(sector: []const u8): UdfPvd? {
    if (sector.len < 200) {
        return null;
    }
    const tag = parseUdfTag(sector) ?? return null;
    if (tag.tag_id != TAG_PVD) {
        return null;
    }
    let vol_id_raw: [32]u8 = [0; 32];
    let vol_set_id_raw: [128]u8 = [0; 128];
    copySlice(vol_id_raw[0..], sector, 24);
    copySlice(vol_set_id_raw[0..], sector, 72);

    return UdfPvd{
        .tag = tag,
        .vds_seq_num = readU32Le(sector, 16),
        .pvd_num = readU32Le(sector, 20),
        .vol_id_raw = vol_id_raw,
        .vol_seq_num = readU16Le(sector, 56),
        .max_vol_seq_num = readU16Le(sector, 58),
        .interchange_level = readU16Le(sector, 60),
        .vol_set_id_raw = vol_set_id_raw,
    };
}
