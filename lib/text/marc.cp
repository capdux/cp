// Origin: Pure-CP Stdlib (Zero-Alloc ISO 2709 / MARC 21 Binary Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation ISO 2709 / Library of Congress MARC 21 bibliographic parser:
// 24-byte ASCII Leader, 12-byte Directory entries, control (001..009) & data (>=010) fields,
// 0x1F subfield splitting, and multi-record .mrc streaming iteration.

/// Fixed ISO 2709 / MARC 21 Leader length in bytes.
pub const MARC_LEADER_SIZE: usize = 24;

/// Fixed MARC 21 Directory entry length in bytes (3B tag + 4B len + 5B start).
pub const MARC_DIR_ENTRY_SIZE: usize = 12;

/// ISO 2709 Record Terminator byte (`0x1D`, ASCII GS).
pub const MARC_RECORD_TERMINATOR: u8 = 0x1D;

/// ISO 2709 Field Terminator byte (`0x1E`, ASCII RS).
pub const MARC_FIELD_TERMINATOR: u8 = 0x1E;

/// ISO 2709 Subfield Delimiter byte (`0x1F`, ASCII US).
pub const MARC_SUBFIELD_DELIM: u8 = 0x1F;

/// Decoded 24-byte MARC 21 Leader.
pub struct MarcLeader {
    record_length: usize = 0,
    record_status: u8 = ' ',
    record_type: u8 = ' ',
    bib_level: u8 = ' ',
    char_encoding: u8 = ' ',
    base_addr_of_data: usize = 0,
    encoding_level: u8 = ' ',

    /// Returns `true` if the record uses UTF-8 (`char_encoding == 'a'`).
    fn isUtf8(): bool {
        return _.char_encoding == 'a';
    }

    /// Returns the number of 12-byte Directory entries in this record.
    fn fieldCount(): usize {
        if (_.base_addr_of_data < 25) {
            return 0;
        }
        return (_.base_addr_of_data - 25) / MARC_DIR_ENTRY_SIZE;
    }
}

/// Zero-copy subfield (`0x1F <code> <value>`) inside a MARC 21 data field.
pub struct MarcSubfield {
    code: u8 = 0,
    value: []const u8 = "",
}

/// Zero-allocation iterator over `0x1F`-delimited subfields in a MARC 21 data field.
pub struct MarcSubfieldIter {
    data: []const u8 = "",
    pos: usize = 0,

    pub fn init(field_data: []const u8): MarcSubfieldIter {
        return MarcSubfieldIter {
            data: field_data,
            pos: 0,
        };
    }

    fn next(*_): MarcSubfield? {
        // Advance to next 0x1F delimiter
        while (_.pos < _.data.len && _.data[_.pos] != MARC_SUBFIELD_DELIM) {
            _.pos += 1;
        }
        if (_.pos >= _.data.len) {
            return null;
        }
        // Consume 0x1F
        _.pos += 1;
        if (_.pos >= _.data.len) {
            return null;
        }
        const code = _.data[_.pos];
        _.pos += 1;

        const val_start = _.pos;
        while (_.pos < _.data.len && _.data[_.pos] != MARC_SUBFIELD_DELIM) {
            _.pos += 1;
        }
        return MarcSubfield {
            code: code,
            value: _.data[val_start.._.pos],
        };
    }
}

/// Zero-copy decoded MARC 21 Control (`001..009`) or Data (`>= 010`) field.
pub struct MarcField {
    tag: [3]u8 = ['0', '0', '0'],
    ind1: u8 = ' ',
    ind2: u8 = ' ',
    data: []const u8 = "",

    /// Returns `true` if `tag` matches the 3-character string `t3` (e.g. `"245"`).
    fn isTag(t3: []const u8): bool {
        return t3.len == 3 &&
            _.tag[0] == t3[0] &&
            _.tag[1] == t3[1] &&
            _.tag[2] == t3[2];
    }

    /// Returns `true` for MARC 21 Control Fields (`001..009`).
    fn isControlField(): bool {
        return _.tag[0] == '0' && _.tag[1] == '0' && _.tag[2] >= '1' && _.tag[2] <= '9';
    }

    /// Returns a zero-allocation iterator over subfields (empty for control fields).
    fn subfields(): MarcSubfieldIter {
        if (_.isControlField()) {
            return MarcSubfieldIter.init("");
        }
        return MarcSubfieldIter.init(_.data);
    }

    /// Finds the value of the first subfield with matching `code` (e.g. `'a'`, `'b'`, `'c'`).
    fn findSubfield(code: u8): ([]const u8)? {
        let it = _.subfields();
        while (sf := it.next()) {
            if (sf.code == code) {
                return sf.value;
            }
        }
        return null;
    }
}

/// Zero-copy view of a single ISO 2709 / MARC 21 record yielded by `MarcRecordIter`.
pub struct MarcRecord {
    leader: MarcLeader = MarcLeader {},
    raw: []const u8 = "",

    /// Returns a zero-allocation `MarcFieldIter` over all fields in this record.
    fn fields(): MarcFieldIter {
        return MarcFieldIter {
            record: _.raw,
            leader: _.leader,
            idx: 0,
            total: _.leader.fieldCount(),
            failed: false,
        };
    }
}

// ---------------------------------------------------------------------------
// Internal ASCII Decimal Helper
// ---------------------------------------------------------------------------

fn parseFixedDecimal(s: []const u8): usize? {
    if (s.len == 0) {
        return null;
    }
    let acc: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const ch = s[i];
        if (ch < '0' || ch > '9') {
            return null;
        }
        acc = acc * 10 + ((ch - '0') as usize);
        i += 1;
    }
    return acc;
}

// ---------------------------------------------------------------------------
// Public Detection, Leader, Field & Record Stream API
// ---------------------------------------------------------------------------

/// Validates the MARC 21 Leader prefix, record length, trailing `0x1D` record terminator,
/// and MARC 21 indicator/subfield counts (`'2'`, `'2'`).
pub fn isMarcRecord(data: []const u8): bool {
    if (data.len < MARC_LEADER_SIZE) {
        return false;
    }
    if (data[10] != '2' || data[11] != '2') {
        return false;
    }
    if (rec_len := parseFixedDecimal(data[0..5])) {
        if (rec_len < MARC_LEADER_SIZE || rec_len > data.len) {
            return false;
        }
        return data[rec_len - 1] == MARC_RECORD_TERMINATOR;
    }
    return false;
}

/// Parses and strictly validates the 24-byte MARC 21 Leader and Directory boundary.
/// Enforces `base_addr_of_data >= 25`, `base_addr_of_data < record_length`,
/// `data[base_addr_of_data - 1] == 0x1E`, and `(base_addr_of_data - 25) % 12 == 0`.
pub fn parseMarcLeader(data: []const u8): MarcLeader? {
    if (!isMarcRecord(data)) {
        return null;
    }
    let rec_len: usize = 0;
    if (v := parseFixedDecimal(data[0..5])) { rec_len = v; } else { return null; }

    let base_addr: usize = 0;
    if (v := parseFixedDecimal(data[12..17])) { base_addr = v; } else { return null; }

    if (base_addr < 25 || base_addr >= rec_len) {
        return null;
    }
    if (data[base_addr - 1] != MARC_FIELD_TERMINATOR) {
        return null;
    }
    if (((base_addr - 25) % MARC_DIR_ENTRY_SIZE) != 0) {
        return null;
    }

    return MarcLeader {
        record_length: rec_len,
        record_status: data[5],
        record_type: data[6],
        bib_level: data[7],
        char_encoding: data[9],
        base_addr_of_data: base_addr,
        encoding_level: data[17],
    };
}

/// Zero-allocation iterator over all 12-byte Directory entries and their variable fields.
pub struct MarcFieldIter {
    record: []const u8 = "",
    leader: MarcLeader = MarcLeader {},
    idx: usize = 0,
    total: usize = 0,
    failed: bool = false,

    /// Initializes a field iterator from a single MARC 21 record buffer.
    /// Returns `null` if the 24-byte Leader or Directory header is invalid.
    pub fn init(data: []const u8): MarcFieldIter? {
        if (ldr := parseMarcLeader(data)) {
            return MarcFieldIter {
                record: data[0..ldr.record_length],
                leader: ldr,
                idx: 0,
                total: ldr.fieldCount(),
                failed: false,
            };
        }
        return null;
    }

    /// Returns `true` if a directory entry had invalid digits, out-of-bounds length, or missing `0x1E`.
    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): MarcField? {
        if (_.failed || _.idx >= _.total) {
            return null;
        }
        const entry_off = MARC_LEADER_SIZE + _.idx * MARC_DIR_ENTRY_SIZE;
        _.idx += 1;

        const tag: [3]u8 = [
            _.record[entry_off],
            _.record[entry_off + 1],
            _.record[entry_off + 2],
        ];

        let field_len: usize = 0;
        let start_pos: usize = 0;
        if (v := parseFixedDecimal(_.record[entry_off + 3..entry_off + 7])) {
            field_len = v;
        } else {
            _.failed = true;
            return null;
        }
        if (v := parseFixedDecimal(_.record[entry_off + 7..entry_off + 12])) {
            start_pos = v;
        } else {
            _.failed = true;
            return null;
        }

        const data_limit = _.leader.record_length - 1; // exclude trailing 0x1D
        if (field_len == 0 || start_pos > data_limit - _.leader.base_addr_of_data) {
            _.failed = true;
            return null;
        }
        const abs_start = _.leader.base_addr_of_data + start_pos;
        if (field_len > data_limit - abs_start) {
            _.failed = true;
            return null;
        }
        const abs_end = abs_start + field_len;
        if (_.record[abs_end - 1] != MARC_FIELD_TERMINATOR) {
            _.failed = true;
            return null;
        }

        const body = _.record[abs_start..abs_end - 1];
        const is_ctrl = tag[0] == '0' && tag[1] == '0' && tag[2] >= '1' && tag[2] <= '9';
        if (is_ctrl) {
            return MarcField {
                tag: tag,
                ind1: ' ',
                ind2: ' ',
                data: body,
            };
        }

        if (body.len < 2) {
            _.failed = true;
            return null;
        }
        return MarcField {
            tag: tag,
            ind1: body[0],
            ind2: body[1],
            data: body[2..],
        };
    }
}

/// Zero-allocation streaming iterator over concatenated ISO 2709 / MARC 21 (`.mrc`) records.
pub struct MarcRecordIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,

    pub fn init(data: []const u8): MarcRecordIter {
        return MarcRecordIter {
            data: data,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): MarcRecord? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        const rem = _.data[_.pos..];
        if (ldr := parseMarcLeader(rem)) {
            const rec_slice = rem[0..ldr.record_length];
            _.pos += ldr.record_length;
            return MarcRecord {
                leader: ldr,
                raw: rec_slice,
            };
        }
        _.failed = true;
        return null;
    }
}
