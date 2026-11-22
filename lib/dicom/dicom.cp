// Origin: Pure-CP Stdlib (Zero-Alloc DICOM PS3.10 Medical Imaging Data Element Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation DICOM PS3.10 (.dcm) parser:
// 128B preamble + "DICM" magic, Group 0x0002 Explicit VR LE File Meta Information,
// automatic Transfer Syntax switching (Explicit VR LE / Implicit VR LE / Explicit VR BE),
// long/short VR decoding, and 0xFFFFFFFF undefined-length SQ / PixelData delimitation.

/// Standard DICOM PS3.10 file preamble length in bytes.
pub const DICOM_PREAMBLE_SIZE: usize = 128;

/// Standard 4-byte DICOM prefix magic at byte offset 128..132.
pub const DICOM_MAGIC: []const u8 = "DICM";

/// Byte offset where the first DICOM Data Element begins (128 + 4).
pub const DICOM_DATA_OFFSET: usize = 132;

/// Sentinel value length (`0xFFFFFFFF`) indicating Undefined Length (SQ / encapsulated PixelData).
pub const DICOM_UNDEFINED_LEN: u32 = 0xFFFFFFFF;

/// Implicit VR Little Endian Transfer Syntax UID (Default DICOM Transfer Syntax).
pub const TS_IMPLICIT_VR_LE: []const u8 = "1.2.840.10008.1.2";

/// Explicit VR Little Endian Transfer Syntax UID.
pub const TS_EXPLICIT_VR_LE: []const u8 = "1.2.840.10008.1.2.1";

/// Explicit VR Big Endian Transfer Syntax UID (Retired).
pub const TS_EXPLICIT_VR_BE: []const u8 = "1.2.840.10008.1.2.2";

/// DICOM Data Element Tag pair `(group, element)`.
pub struct DicomTag {
    group: u16 = 0,
    element: u16 = 0,
}

/// Transfer Syntax UID `(0x0002, 0x0010)`.
pub const TAG_TRANSFER_SYNTAX: DicomTag = DicomTag { group: 0x0002, element: 0x0010 };

/// Patient's Name `(0x0010, 0x0010)`.
pub const TAG_PATIENT_NAME: DicomTag = DicomTag { group: 0x0010, element: 0x0010 };

/// Patient ID `(0x0010, 0x0020)`.
pub const TAG_PATIENT_ID: DicomTag = DicomTag { group: 0x0010, element: 0x0020 };

/// Modality `(0x0008, 0x0060)`.
pub const TAG_MODALITY: DicomTag = DicomTag { group: 0x0008, element: 0x0060 };

/// Image Rows `(0x0028, 0x0010)`.
pub const TAG_ROWS: DicomTag = DicomTag { group: 0x0028, element: 0x0010 };

/// Image Columns `(0x0028, 0x0011)`.
pub const TAG_COLUMNS: DicomTag = DicomTag { group: 0x0028, element: 0x0011 };

/// Bits Allocated `(0x0028, 0x0100)`.
pub const TAG_BITS_ALLOCATED: DicomTag = DicomTag { group: 0x0028, element: 0x0100 };

/// Pixel Data `(0x7FE0, 0x0010)`.
pub const TAG_PIXEL_DATA: DicomTag = DicomTag { group: 0x7FE0, element: 0x0010 };

const GROUP_META: u16 = 0x0002;
const GROUP_DELIM: u16 = 0xFFFE;
const ELEM_ITEM: u16 = 0xE000;
const ELEM_ITEM_DELIM: u16 = 0xE00D;
const ELEM_SEQ_DELIM: u16 = 0xE0DD;

/// Zero-copy decoded DICOM Data Element.
pub struct DicomElement {
    group: u16 = 0,
    element: u16 = 0,
    vr: [2]u8 = [' ', ' '],
    value: []const u8 = "",
    is_undefined_len: bool = false,

    /// Returns `true` if this element matches `(g, e)`.
    fn isTag(g: u16, e: u16): bool {
        return _.group == g && _.element == e;
    }

    /// Returns `true` if this element matches a `DicomTag` constant.
    fn matchesTag(tag: DicomTag): bool {
        return _.group == tag.group && _.element == tag.element;
    }

    /// Returns `value` with trailing ASCII space (`0x20`) and NUL (`0x00`) padding stripped.
    fn asTrimmedString(): []const u8 {
        return trimDicomTrailing(_.value);
    }

    /// Decodes the first 2 bytes of `value` as an unsigned 16-bit integer.
    fn asU16(is_be: bool): u16? {
        if (_.value.len < 2) {
            return null;
        }
        return readU16(_.value, 0, is_be);
    }

    /// Decodes the first 4 bytes of `value` as an unsigned 32-bit integer.
    fn asU32(is_be: bool): u32? {
        if (_.value.len < 4) {
            return null;
        }
        return readU32(_.value, 0, is_be);
    }
}

/// High-level metadata summary extracted in a single zero-allocation pass.
pub struct DicomSummary {
    transfer_syntax: []const u8 = "",
    modality: []const u8 = "",
    patient_name: []const u8 = "",
    patient_id: []const u8 = "",
    rows: u16 = 0,
    columns: u16 = 0,
    bits_allocated: u16 = 0,
    has_pixel_data: bool = false,
    element_count: u32 = 0,
}

struct UndefinedScanResult {
    value_end: usize,
    next_pos: usize,
}

// ---------------------------------------------------------------------------
// Internal Byte & VR Helpers
// ---------------------------------------------------------------------------

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readU16Be(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readU16(data: []const u8, off: usize, is_be: bool): u16 {
    if (is_be) {
        return readU16Be(data, off);
    }
    return readU16Le(data, off);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn readU32Be(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24) |
        ((data[off + 1] as u32) << 16) |
        ((data[off + 2] as u32) << 8) |
        (data[off + 3] as u32);
}

fn readU32(data: []const u8, off: usize, is_be: bool): u32 {
    if (is_be) {
        return readU32Be(data, off);
    }
    return readU32Le(data, off);
}

fn bytesEqual(a: []const u8, b: []const u8): bool {
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

fn trimDicomTrailing(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0) {
        const ch = s[end - 1];
        if (ch == ' ' || ch == 0) {
            end -= 1;
        } else {
            break;
        }
    }
    return s[0..end];
}

/// Returns `true` if `vr` is a DICOM Explicit VR with 2 reserved bytes + 4-byte length:
/// `OB`, `OD`, `OF`, `OL`, `OV`, `OW`, `SQ`, `SV`, `UC`, `UN`, `UR`, `UT`, `UV`.
fn isLongVr(vr: [2]u8): bool {
    const a = vr[0];
    const b = vr[1];
    if (a == 'O') {
        return b == 'B' || b == 'D' || b == 'F' || b == 'L' || b == 'V' || b == 'W';
    }
    if (a == 'S') {
        return b == 'Q' || b == 'V';
    }
    if (a == 'U') {
        return b == 'C' || b == 'N' || b == 'R' || b == 'T' || b == 'V';
    }
    return false;
}

/// Scans an undefined-length (`0xFFFFFFFF`) element (such as `SQ` or encapsulated `PixelData`)
/// forward until its matching Sequence Delimitation Item `(0xFFFE, 0xE0DD)`.
fn skipUndefinedLength(data: []const u8, start: usize, explicit: bool, is_be: bool): UndefinedScanResult? {
    let pos = start;
    let seq_depth: usize = 1;

    while (seq_depth > 0) {
        if (pos + 4 > data.len) {
            return null;
        }
        const tag_pos = pos;
        const g = readU16(data, pos, is_be);
        const e = readU16(data, pos + 2, is_be);
        pos += 4;

        if (g == GROUP_DELIM) {
            if (pos + 4 > data.len) {
                return null;
            }
            const len = readU32(data, pos, is_be);
            pos += 4;

            if (e == ELEM_SEQ_DELIM) {
                seq_depth -= 1;
                if (seq_depth == 0) {
                    return UndefinedScanResult {
                        value_end: tag_pos,
                        next_pos: pos,
                    };
                }
            } else if (e == ELEM_ITEM) {
                if (len != DICOM_UNDEFINED_LEN) {
                    const len_usize = len as usize;
                    if (len_usize > data.len - pos) {
                        return null;
                    }
                    pos += len_usize;
                }
            }
            // ELEM_ITEM_DELIM (0xE00D): 4-byte length already consumed, continue scanning.
        } else {
            let el_len: u32 = 0;
            if (explicit) {
                if (pos + 2 > data.len) {
                    return null;
                }
                const vr: [2]u8 = [data[pos], data[pos + 1]];
                pos += 2;
                if (isLongVr(vr)) {
                    if (pos + 6 > data.len) {
                        return null;
                    }
                    el_len = readU32(data, pos + 2, is_be);
                    pos += 6;
                } else {
                    if (pos + 2 > data.len) {
                        return null;
                    }
                    el_len = readU16(data, pos, is_be) as u32;
                    pos += 2;
                }
            } else {
                if (pos + 4 > data.len) {
                    return null;
                }
                el_len = readU32(data, pos, is_be);
                pos += 4;
            }

            if (el_len == DICOM_UNDEFINED_LEN) {
                seq_depth += 1;
            } else {
                const len_usize = el_len as usize;
                if (len_usize > data.len - pos) {
                    return null;
                }
                pos += len_usize;
            }
        }
    }

    return null;
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

/// Validates that `data` is at least 132 bytes and has the `"DICM"` magic at offset `128..132`.
pub fn isDicom(data: []const u8): bool {
    if (data.len < DICOM_DATA_OFFSET) {
        return false;
    }
    return data[128] == 'D' &&
        data[129] == 'I' &&
        data[130] == 'C' &&
        data[131] == 'M';
}

/// Zero-allocation streaming DICOM PS3.10 Data Element iterator.
/// Always decodes Group `0x0002` (File Meta Information) as Explicit VR Little Endian,
/// and automatically switches dataset decoding mode upon reading `(0x0002, 0x0010)`.
pub struct DicomIter {
    data: []const u8 = "",
    pos: usize = 0,
    in_meta: bool = true,
    dataset_explicit_vr: bool = true,
    dataset_big_endian: bool = false,
    transfer_syntax: []const u8 = "",
    failed: bool = false,

    /// Initializes a streaming iterator over a valid DICOM PS3.10 buffer.
    /// Returns `null` if `isDicom(data)` is `false`.
    pub fn init(data: []const u8): DicomIter? {
        if (!isDicom(data)) {
            return null;
        }
        return DicomIter {
            data: data,
            pos: DICOM_DATA_OFFSET,
            in_meta: true,
            dataset_explicit_vr: true,
            dataset_big_endian: false,
            transfer_syntax: "",
            failed: false,
        };
    }

    /// Returns `true` if the dataset (Group > `0x0002`) is currently decoded as Big Endian.
    fn isBigEndian(): bool {
        return _.dataset_big_endian;
    }

    /// Returns `true` if the dataset (Group > `0x0002`) is currently decoded as Explicit VR.
    fn isExplicitVr(): bool {
        return _.dataset_explicit_vr;
    }

    /// Returns `true` if iteration terminated early due to truncated or malformed input.
    fn hasError(): bool {
        return _.failed;
    }

    /// Parses and returns the next DICOM Data Element, or `null` at EOF / error.
    fn next(*_): DicomElement? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        if (_.pos + 4 > _.data.len) {
            _.failed = true;
            return null;
        }

        // DICOM PS3.10: Group 0x0002 (File Meta Information) is ALWAYS Explicit VR Little Endian.
        let use_explicit = _.dataset_explicit_vr;
        let use_be = _.dataset_big_endian;
        if (_.in_meta) {
            const peek_group_le = readU16Le(_.data, _.pos);
            if (peek_group_le == GROUP_META) {
                use_explicit = true;
                use_be = false;
            } else {
                _.in_meta = false;
            }
        }

        const group = readU16(_.data, _.pos, use_be);
        const element = readU16(_.data, _.pos + 2, use_be);
        _.pos += 4;

        let vr: [2]u8 = [' ', ' '];
        let val_len: u32 = 0;

        if (use_explicit && group != GROUP_DELIM) {
            if (_.pos + 2 > _.data.len) {
                _.failed = true;
                return null;
            }
            vr = [_.data[_.pos], _.data[_.pos + 1]];
            _.pos += 2;

            if (isLongVr(vr)) {
                if (_.pos + 6 > _.data.len) {
                    _.failed = true;
                    return null;
                }
                val_len = readU32(_.data, _.pos + 2, use_be);
                _.pos += 6;
            } else {
                if (_.pos + 2 > _.data.len) {
                    _.failed = true;
                    return null;
                }
                val_len = readU16(_.data, _.pos, use_be) as u32;
                _.pos += 2;
            }
        } else {
            if (_.pos + 4 > _.data.len) {
                _.failed = true;
                return null;
            }
            val_len = readU32(_.data, _.pos, use_be);
            _.pos += 4;
        }

        let val_slice: []const u8 = "";
        let is_undef = false;

        if (val_len == DICOM_UNDEFINED_LEN) {
            is_undef = true;
            const val_start = _.pos;
            if (scan := skipUndefinedLength(_.data, val_start, use_explicit, use_be)) {
                val_slice = _.data[val_start..scan.value_end];
                _.pos = scan.next_pos;
            } else {
                _.failed = true;
                return null;
            }
        } else {
            const len_usize = val_len as usize;
            if (len_usize > _.data.len - _.pos) {
                _.failed = true;
                return null;
            }
            val_slice = _.data[_.pos.._.pos + len_usize];
            _.pos += len_usize;
        }

        // Automatically switch dataset transfer syntax upon reading (0x0002, 0x0010).
        if (group == TAG_TRANSFER_SYNTAX.group && element == TAG_TRANSFER_SYNTAX.element) {
            const ts = trimDicomTrailing(val_slice);
            _.transfer_syntax = ts;
            if (bytesEqual(ts, TS_IMPLICIT_VR_LE)) {
                _.dataset_explicit_vr = false;
                _.dataset_big_endian = false;
            } else if (bytesEqual(ts, TS_EXPLICIT_VR_BE)) {
                _.dataset_explicit_vr = true;
                _.dataset_big_endian = true;
            } else {
                _.dataset_explicit_vr = true;
                _.dataset_big_endian = false;
            }
        }

        return DicomElement {
            group: group,
            element: element,
            vr: vr,
            value: val_slice,
            is_undefined_len: is_undef,
        };
    }
}

/// Parses a DICOM PS3.10 buffer in a single zero-allocation pass and extracts core metadata.
/// Returns `null` if the magic is invalid or a truncated/malformed element is encountered.
pub fn parseDicomSummary(data: []const u8): DicomSummary? {
    if (it_val := DicomIter.init(data)) {
        let it = it_val;
        let summary = DicomSummary {};

        while (elem := it.next()) {
            summary.element_count += 1;
            const elem_be = if (elem.group == GROUP_META) false else it.isBigEndian();

            if (elem.matchesTag(TAG_TRANSFER_SYNTAX)) {
                summary.transfer_syntax = elem.asTrimmedString();
            } else if (elem.matchesTag(TAG_MODALITY)) {
                summary.modality = elem.asTrimmedString();
            } else if (elem.matchesTag(TAG_PATIENT_NAME)) {
                summary.patient_name = elem.asTrimmedString();
            } else if (elem.matchesTag(TAG_PATIENT_ID)) {
                summary.patient_id = elem.asTrimmedString();
            } else if (elem.matchesTag(TAG_ROWS)) {
                if (v := elem.asU16(elem_be)) {
                    summary.rows = v;
                }
            } else if (elem.matchesTag(TAG_COLUMNS)) {
                if (v := elem.asU16(elem_be)) {
                    summary.columns = v;
                }
            } else if (elem.matchesTag(TAG_BITS_ALLOCATED)) {
                if (v := elem.asU16(elem_be)) {
                    summary.bits_allocated = v;
                }
            } else if (elem.matchesTag(TAG_PIXEL_DATA)) {
                summary.has_pixel_data = true;
            }
        }

        if (it.hasError()) {
            return null;
        }
        return summary;
    }
    return null;
}
