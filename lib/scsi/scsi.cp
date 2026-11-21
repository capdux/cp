// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc SCSI SPC-5 / SBC-4 CDB, INQUIRY, CAPACITY & Sense Codec)

// Common SCSI Opcodes (SPC-5 / SBC-4)
pub const OP_TEST_UNIT_READY: u8 = 0x00;
pub const OP_REQUEST_SENSE: u8 = 0x03;
pub const OP_INQUIRY: u8 = 0x12;
pub const OP_READ_CAPACITY_10: u8 = 0x25;
pub const OP_READ_10: u8 = 0x28;
pub const OP_WRITE_10: u8 = 0x2A;
pub const OP_SYNCHRONIZE_CACHE_10: u8 = 0x35;
pub const OP_READ_16: u8 = 0x88;
pub const OP_WRITE_16: u8 = 0x8A;
pub const OP_SERVICE_ACTION_IN_16: u8 = 0x9E;

// SCSI Status Bytes
pub const STATUS_GOOD: u8 = 0x00;
pub const STATUS_CHECK_CONDITION: u8 = 0x02;
pub const STATUS_BUSY: u8 = 0x08;
pub const STATUS_RESERVATION_CONFLICT: u8 = 0x18;
pub const STATUS_TASK_SET_FULL: u8 = 0x28;

// SCSI Sense Keys
pub const SK_NO_SENSE: u8 = 0x0;
pub const SK_RECOVERED_ERROR: u8 = 0x1;
pub const SK_NOT_READY: u8 = 0x2;
pub const SK_MEDIUM_ERROR: u8 = 0x3;
pub const SK_HARDWARE_ERROR: u8 = 0x4;
pub const SK_ILLEGAL_REQUEST: u8 = 0x5;
pub const SK_UNIT_ATTENTION: u8 = 0x6;
pub const SK_DATA_PROTECT: u8 = 0x7;
pub const SK_ABORTED_COMMAND: u8 = 0xB;

// Peripheral Device Types
pub const PDT_DIRECT_ACCESS: u8 = 0x00;
pub const PDT_SEQUENTIAL_ACCESS: u8 = 0x01;
pub const PDT_CD_DVD: u8 = 0x05;
pub const PDT_ENCLOSURE: u8 = 0x0D;

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

fn writeU16Be(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeU32Be(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeU64Be(buf: []u8, off: usize, v: u64) {
    writeU32Be(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    writeU32Be(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn trimRightSpace(buf: []const u8): []const u8 {
    let end = buf.len;
    while (end > 0 && (buf[end - 1] == ' ' || buf[end - 1] == 0)) {
        end -= 1;
    }
    return buf[0..end];
}

/// Returns the standard CDB byte length for a SCSI opcode group (6/10/12/16),
/// or `null` for reserved/vendor-specific groups.
pub fn cdbLengthForOpcode(opcode: u8): u8? {
    const grp = opcode >> 5;
    if (grp == 0) return 6;
    if (grp == 1 || grp == 2) return 10;
    if (grp == 4) return 16;
    if (grp == 5) return 12;
    return null;
}

/// Parsed 36-byte Standard SCSI INQUIRY response data.
pub struct ScsiInquiryData {
    peripheral_qualifier: u8,
    device_type: u8,
    removable: bool,
    version: u8,
    response_format: u8,
    vendor_id: [8]u8,
    product_id: [16]u8,
    product_rev: [4]u8,

    fn vendorSlice(): []const u8 {
        return trimRightSpace(_.vendor_id[0..]);
    }

    fn productSlice(): []const u8 {
        return trimRightSpace(_.product_id[0..]);
    }

    fn revSlice(): []const u8 {
        return trimRightSpace(_.product_rev[0..]);
    }
}

/// Parses a standard SCSI INQUIRY payload (requires `data.len >= 36`).
pub fn parseInquiry(data: []const u8): ScsiInquiryData? {
    if (data.len < 36) {
        return null;
    }
    let vendor: [8]u8 = undefined;
    let i: usize = 0;
    while (i < 8) {
        vendor[i] = data[8 + i];
        i += 1;
    }
    let product: [16]u8 = undefined;
    let j: usize = 0;
    while (j < 16) {
        product[j] = data[16 + j];
        j += 1;
    }
    let rev: [4]u8 = undefined;
    let k: usize = 0;
    while (k < 4) {
        rev[k] = data[32 + k];
        k += 1;
    }

    return ScsiInquiryData{
        .peripheral_qualifier = (data[0] >> 5) & 0x07,
        .device_type = data[0] & 0x1F,
        .removable = (data[1] & 0x80) != 0,
        .version = data[2],
        .response_format = data[3] & 0x0F,
        .vendor_id = vendor,
        .product_id = product,
        .product_rev = rev,
    };
}

/// Parsed SCSI READ CAPACITY(10) / READ CAPACITY(16) geometry.
pub struct ScsiCapacity {
    last_lba: u64,
    block_size: u32,

    fn sectorCount(): u64 {
        return _.last_lba + 1;
    }

    fn totalBytes(): u128 {
        return ((_.last_lba as u128) + 1) * (_.block_size as u128);
    }
}

/// Parses an 8-byte READ CAPACITY(10) response.
pub fn parseReadCapacity10(data: []const u8): ScsiCapacity? {
    if (data.len < 8) {
        return null;
    }
    return ScsiCapacity{
        .last_lba = readU32Be(data, 0) as u64,
        .block_size = readU32Be(data, 4),
    };
}

/// Parses a READ CAPACITY(16) response (requires `data.len >= 12`).
pub fn parseReadCapacity16(data: []const u8): ScsiCapacity? {
    if (data.len < 12) {
        return null;
    }
    return ScsiCapacity{
        .last_lba = readU64Be(data, 0),
        .block_size = readU32Be(data, 8),
    };
}

/// Parsed SCSI Fixed (`0x70`/`0x71`) or Descriptor (`0x72`/`0x73`) Sense Data.
pub struct ScsiSense {
    response_code: u8,
    is_descriptor: bool,
    is_deferred: bool,
    sense_key: u8,
    asc: u8,
    ascq: u8,
    valid_info: bool,
    info: u32,

    fn isCurrent(): bool {
        return !_.is_deferred;
    }

    fn isOk(): bool {
        return _.sense_key == SK_NO_SENSE;
    }
}

/// Parses SCSI Fixed (`0x70`/`0x71`, >=14B) or Descriptor (`0x72`/`0x73`, >=8B) Sense Data.
pub fn parseSense(data: []const u8): ScsiSense? {
    if (data.len < 8) {
        return null;
    }
    const resp_code = data[0] & 0x7F;

    if (resp_code == 0x70 || resp_code == 0x71) {
        if (data.len < 14) {
            return null;
        }
        return ScsiSense{
            .response_code = resp_code,
            .is_descriptor = false,
            .is_deferred = resp_code == 0x71,
            .sense_key = data[2] & 0x0F,
            .asc = data[12],
            .ascq = data[13],
            .valid_info = (data[0] & 0x80) != 0,
            .info = readU32Be(data, 3),
        };
    }

    if (resp_code == 0x72 || resp_code == 0x73) {
        return ScsiSense{
            .response_code = resp_code,
            .is_descriptor = true,
            .is_deferred = resp_code == 0x73,
            .sense_key = data[1] & 0x0F,
            .asc = data[2],
            .ascq = data[3],
            .valid_info = false,
            .info = 0,
        };
    }

    return null;
}

/// Zero-alloc encoder for 6-byte SCSI INQUIRY CDB (`0x12`).
pub fn encodeInquiryCdb(alloc_len: u16, out: []u8): usize? {
    if (out.len < 6) {
        return null;
    }
    out[0] = OP_INQUIRY;
    out[1] = 0;
    out[2] = 0;
    writeU16Be(out, 3, alloc_len);
    out[5] = 0;
    return 6;
}

/// Zero-alloc encoder for 10-byte SCSI READ(10) CDB (`0x28`).
pub fn encodeRead10Cdb(lba: u32, blocks: u16, out: []u8): usize? {
    if (out.len < 10) {
        return null;
    }
    out[0] = OP_READ_10;
    out[1] = 0;
    writeU32Be(out, 2, lba);
    out[6] = 0;
    writeU16Be(out, 7, blocks);
    out[9] = 0;
    return 10;
}

/// Zero-alloc encoder for 10-byte SCSI WRITE(10) CDB (`0x2A`).
pub fn encodeWrite10Cdb(lba: u32, blocks: u16, out: []u8): usize? {
    if (out.len < 10) {
        return null;
    }
    out[0] = OP_WRITE_10;
    out[1] = 0;
    writeU32Be(out, 2, lba);
    out[6] = 0;
    writeU16Be(out, 7, blocks);
    out[9] = 0;
    return 10;
}

/// Zero-alloc encoder for 16-byte SCSI READ(16) CDB (`0x88`).
pub fn encodeRead16Cdb(lba: u64, blocks: u32, out: []u8): usize? {
    if (out.len < 16) {
        return null;
    }
    out[0] = OP_READ_16;
    out[1] = 0;
    writeU64Be(out, 2, lba);
    writeU32Be(out, 10, blocks);
    out[14] = 0;
    out[15] = 0;
    return 16;
}

/// Zero-alloc encoder for 16-byte SCSI WRITE(16) CDB (`0x8A`).
pub fn encodeWrite16Cdb(lba: u64, blocks: u32, out: []u8): usize? {
    if (out.len < 16) {
        return null;
    }
    out[0] = OP_WRITE_16;
    out[1] = 0;
    writeU64Be(out, 2, lba);
    writeU32Be(out, 10, blocks);
    out[14] = 0;
    out[15] = 0;
    return 16;
}
