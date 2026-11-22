// Origin: Pure-CP Stdlib (Zero-Alloc VESA EDID 1.3 / 1.4 Block & Timing Parser) | Created: 2026-11-22

// VESA EDID Constants & Descriptor Tags
pub const EDID_BLOCK_SIZE: usize = 128;
pub const EDID_HEADER_MAGIC: [8]u8 = [0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00];
pub const DESC_SERIAL: u8 = 0xFF;
pub const DESC_UNSPECIFIED_TEXT: u8 = 0xFE;
pub const DESC_RANGE_LIMITS: u8 = 0xFD;
pub const DESC_MONITOR_NAME: u8 = 0xFC;
pub const DESC_DUMMY: u8 = 0x10;
pub const CEA_EXT_TAG: u8 = 0x02;

fn readU16Le(data: []const u8, off: usize): u16 {
    const b0 = data[off] as u16;
    const b1 = (data[off + 1] as u16) << 8;
    return b0 | b1;
}

fn readU16Be(data: []const u8, off: usize): u16 {
    const hi = (data[off] as u16) << 8;
    const lo = data[off + 1] as u16;
    return hi | lo;
}

fn readU32Le(data: []const u8, off: usize): u32 {
    const b0 = data[off] as u32;
    const b1 = (data[off + 1] as u32) << 8;
    const b2 = (data[off + 2] as u32) << 16;
    const b3 = (data[off + 3] as u32) << 24;
    return b0 | b1 | b2 | b3;
}

/// Computes the 8-bit modular checksum over the first 128 bytes of an EDID block.
/// A valid EDID block always produces `0`. Returns `1` if `block128.len < 128`.
pub fn edidChecksum(block128: []const u8): u8 {
    if (block128.len < EDID_BLOCK_SIZE) {
        return 1;
    }
    let sum: u32 = 0;
    let i: usize = 0;
    while (i < EDID_BLOCK_SIZE) {
        sum += block128[i] as u32;
        i += 1;
    }
    return (sum & 0xFF) as u8;
}

/// Returns `true` if `block128` has at least 128 bytes and its modular checksum equals `0`.
pub fn verifyEdidChecksum(block128: []const u8): bool {
    return block128.len >= EDID_BLOCK_SIZE && edidChecksum(block128) == 0;
}

/// Decodes a 16-bit big-endian compressed 5-bit ASCII PNP manufacturer ID (e.g. `"DEL"`, `"SAM"`, `"APP"`).
pub fn decodePnpId(raw_be: u16): [3]u8 {
    const c0 = (((raw_be >> 10) & 0x1F) + 0x40) as u8;
    const c1 = (((raw_be >> 5) & 0x1F) + 0x40) as u8;
    const c2 = ((raw_be & 0x1F) + 0x40) as u8;
    return [c0, c1, c2];
}

/// Encodes a 3-letter uppercase ASCII PNP manufacturer ID (`'A'..'Z'`) into a 16-bit big-endian value.
pub fn encodePnpId(id: [3]u8): u16? {
    if (id[0] < 'A' || id[0] > 'Z' || id[1] < 'A' || id[1] > 'Z' || id[2] < 'A' || id[2] > 'Z') {
        return null;
    }
    const v0 = ((id[0] - 0x40) as u16) << 10;
    const v1 = ((id[1] - 0x40) as u16) << 5;
    const v2 = (id[2] - 0x40) as u16;
    return v0 | v1 | v2;
}

/// VESA EDID 18-byte Detailed Timing Descriptor (DTD).
pub struct EdidDetailedTiming {
    pixel_clock_khz: u32,
    h_active: u16,
    h_blank: u16,
    v_active: u16,
    v_blank: u16,
    h_sync_offset: u16,
    h_sync_pulse: u16,
    v_sync_offset: u16,
    v_sync_pulse: u16,
    h_size_mm: u16,
    v_size_mm: u16,
    interlaced: bool,

    /// Computes vertical refresh rate in Hz: `pixel_clock_hz / ((h_active + h_blank) * (v_active + v_blank))`.
    fn refreshRateHz(): f64 {
        const h_total = (_.h_active as u64) + (_.h_blank as u64);
        const v_total = (_.v_active as u64) + (_.v_blank as u64);
        if (h_total == 0 || v_total == 0) {
            return 0.0;
        }
        const clk_hz = (_.pixel_clock_khz as f64) * 1000.0;
        return clk_hz / ((h_total as f64) * (v_total as f64));
    }
}

fn emptyTiming(): EdidDetailedTiming {
    return EdidDetailedTiming{
        .pixel_clock_khz = 0,
        .h_active = 0,
        .h_blank = 0,
        .v_active = 0,
        .v_blank = 0,
        .h_sync_offset = 0,
        .h_sync_pulse = 0,
        .v_sync_offset = 0,
        .v_sync_pulse = 0,
        .h_size_mm = 0,
        .v_size_mm = 0,
        .interlaced = false,
    };
}

fn parseDetailedTiming(d: []const u8): EdidDetailedTiming {
    const pixel_clock_khz = (readU16Le(d, 0) as u32) * 10;
    const h_active = (d[2] as u16) | (((d[4] >> 4) as u16) << 8);
    const h_blank = (d[3] as u16) | (((d[4] & 0x0F) as u16) << 8);
    const v_active = (d[5] as u16) | (((d[7] >> 4) as u16) << 8);
    const v_blank = (d[6] as u16) | (((d[7] & 0x0F) as u16) << 8);
    const h_sync_offset = (d[8] as u16) | (((d[11] >> 6) as u16) << 8);
    const h_sync_pulse = (d[9] as u16) | ((((d[11] >> 4) & 0x03) as u16) << 8);
    const v_sync_offset = ((d[10] >> 4) as u16) | ((((d[11] >> 2) & 0x03) as u16) << 4);
    const v_sync_pulse = ((d[10] & 0x0F) as u16) | (((d[11] & 0x03) as u16) << 4);
    const h_size_mm = (d[12] as u16) | (((d[14] >> 4) as u16) << 8);
    const v_size_mm = (d[13] as u16) | (((d[14] & 0x0F) as u16) << 8);
    const interlaced = (d[17] & 0x80) != 0;

    return EdidDetailedTiming{
        .pixel_clock_khz = pixel_clock_khz,
        .h_active = h_active,
        .h_blank = h_blank,
        .v_active = v_active,
        .v_blank = v_blank,
        .h_sync_offset = h_sync_offset,
        .h_sync_pulse = h_sync_pulse,
        .v_sync_offset = v_sync_offset,
        .v_sync_pulse = v_sync_pulse,
        .h_size_mm = h_size_mm,
        .v_size_mm = v_size_mm,
        .interlaced = interlaced,
    };
}

fn trimEdidText(raw: []const u8): []const u8 {
    let end = raw.len;
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == 0x0A || raw[i] == 0x00) {
            end = i;
            break;
        }
        i += 1;
    }
    while (end > 0 && raw[end - 1] == 0x20) {
        end -= 1;
    }
    return raw[0..end];
}

/// One of the 4 18-byte descriptor blocks (Detailed Timing or Monitor Descriptor).
pub struct EdidDescriptor {
    is_timing: bool,
    tag: u8,
    timing: EdidDetailedTiming,
    text: []const u8,
}

/// Decoded 2-byte Standard Timing entry (`width`, `height`, `refresh_hz`).
pub struct EdidStandardTiming {
    width: u16,
    height: u16,
    refresh_hz: u8,
}

/// Parsed 128-byte VESA EDID 1.3 / 1.4 base block with zero-copy descriptor accessors.
pub struct EdidInfo {
    manufacturer: [3]u8,
    product_code: u16,
    serial_number: u32,
    mfg_week: u8,
    mfg_year: u16,
    version: u8,
    revision: u8,
    is_digital: bool,
    color_bit_depth: u8,
    width_cm: u8,
    height_cm: u8,
    gamma_raw: u8,
    feature_flags: u8,
    extension_count: u8,
    raw: []const u8,

    /// Returns the display gamma value (`(gamma_raw + 100) / 100`), or `0.0` if undefined (`0xFF`).
    fn gamma(): f64 {
        if (_.gamma_raw == 0xFF) {
            return 0.0;
        }
        return ((_.gamma_raw as f64) + 100.0) / 100.0;
    }

    /// Parses descriptor slot `idx` (`0..3`) at byte offset `54 + idx * 18`.
    fn descriptorAt(idx: usize): EdidDescriptor? {
        if (idx >= 4 || _.raw.len < EDID_BLOCK_SIZE) {
            return null;
        }
        const off = 54 + idx * 18;
        const d = _.raw[off..(off + 18)];
        if (d[0] != 0 || d[1] != 0) {
            return EdidDescriptor{
                .is_timing = true,
                .tag = 0,
                .timing = parseDetailedTiming(d),
                .text = d[0..0],
            };
        }
        const tag = d[3];
        let text: []const u8 = d[0..0];
        if (tag == DESC_MONITOR_NAME || tag == DESC_SERIAL || tag == DESC_UNSPECIFIED_TEXT) {
            text = trimEdidText(d[5..18]);
        }
        return EdidDescriptor{
            .is_timing = false,
            .tag = tag,
            .timing = emptyTiming(),
            .text = text,
        };
    }

    /// Returns the first Detailed Timing Descriptor (Preferred Timing Mode), if present.
    fn preferredTiming(): EdidDetailedTiming? {
        let i: usize = 0;
        while (i < 4) {
            if (desc := _.descriptorAt(i)) {
                if (desc.is_timing) {
                    return desc.timing;
                }
            }
            i += 1;
        }
        return null;
    }

    /// Returns the monitor name string from descriptor tag `0xFC` (`DESC_MONITOR_NAME`), if present.
    fn monitorName(): ([]const u8)? {
        let i: usize = 0;
        while (i < 4) {
            if (desc := _.descriptorAt(i)) {
                if (!desc.is_timing && desc.tag == DESC_MONITOR_NAME) {
                    return desc.text;
                }
            }
            i += 1;
        }
        return null;
    }

    /// Returns the monitor serial string from descriptor tag `0xFF` (`DESC_SERIAL`), if present.
    fn serialText(): ([]const u8)? {
        let i: usize = 0;
        while (i < 4) {
            if (desc := _.descriptorAt(i)) {
                if (!desc.is_timing && desc.tag == DESC_SERIAL) {
                    return desc.text;
                }
            }
            i += 1;
        }
        return null;
    }

    /// Parses standard timing slot `idx` (`0..7`) at byte offset `38 + idx * 2`.
    fn standardTimingAt(idx: usize): EdidStandardTiming? {
        if (idx >= 8 || _.raw.len < EDID_BLOCK_SIZE) {
            return null;
        }
        const off = 38 + idx * 2;
        const b0 = _.raw[off];
        const b1 = _.raw[off + 1];
        if ((b0 == 0x01 && b1 == 0x01) || (b0 == 0x00 && b1 == 0x00)) {
            return null;
        }
        const width = ((b0 as u16) + 31) * 8;
        const aspect = (b1 >> 6) & 0x03;
        let height: u16 = width;
        if (aspect == 0) {
            if (_.revision >= 3) {
                height = (width * 10) / 16;
            }
        } else if (aspect == 1) {
            height = (width * 3) / 4;
        } else if (aspect == 2) {
            height = (width * 4) / 5;
        } else {
            height = (width * 9) / 16;
        }
        const refresh_hz = (b1 & 0x3F) + 60;
        return EdidStandardTiming{
            .width = width,
            .height = height,
            .refresh_hz = refresh_hz,
        };
    }
}

fn hasEdidMagic(data: []const u8): bool {
    if (data.len < 8) {
        return false;
    }
    let i: usize = 0;
    while (i < 8) {
        if (data[i] != EDID_HEADER_MAGIC[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn decodeColorBitDepth(video_input: u8, revision: u8): u8 {
    if ((video_input & 0x80) == 0 || revision < 4) {
        return 0;
    }
    const code = (video_input >> 4) & 0x07;
    if (code == 1) { return 6; }
    if (code == 2) { return 8; }
    if (code == 3) { return 10; }
    if (code == 4) { return 12; }
    if (code == 5) { return 14; }
    if (code == 6) { return 16; }
    return 0;
}

/// Returns `true` if `data` is a valid 128-byte VESA EDID 1.x block (magic, version == 1, checksum == 0).
pub fn isEdid(data: []const u8): bool {
    return data.len >= EDID_BLOCK_SIZE
        && hasEdidMagic(data)
        && data[18] == 1
        && verifyEdidChecksum(data);
}

/// Parses a 128-byte VESA EDID 1.3 / 1.4 base block.
pub fn parseEdid(data: []const u8): EdidInfo? {
    if (!isEdid(data)) {
        return null;
    }
    const version = data[18];
    const revision = data[19];
    const video_input = data[20];
    const is_digital = (video_input & 0x80) != 0;

    return EdidInfo{
        .manufacturer = decodePnpId(readU16Be(data, 8)),
        .product_code = readU16Le(data, 10),
        .serial_number = readU32Le(data, 12),
        .mfg_week = data[16],
        .mfg_year = 1990 + (data[17] as u16),
        .version = version,
        .revision = revision,
        .is_digital = is_digital,
        .color_bit_depth = decodeColorBitDepth(video_input, revision),
        .width_cm = data[21],
        .height_cm = data[22],
        .gamma_raw = data[23],
        .feature_flags = data[24],
        .extension_count = data[126],
        .raw = data[0..EDID_BLOCK_SIZE],
    };
}
