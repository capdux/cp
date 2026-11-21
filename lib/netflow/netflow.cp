// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Cisco NetFlow v5 & RFC 3954 v9 Parser)
// Pure CP zero-heap-allocation NetFlow v5 & NetFlow v9 wire protocol parser:
// NetFlow v5 24-byte header + 48-byte Flow Record iterator, NetFlow v9 20-byte header +
// FlowSet iterator + Template FlowSet & FieldSpec zero-copy iterators.

/// Standard NetFlow collector UDP port (`2055`).
pub const NETFLOW_UDP_PORT: u16 = 2055;

/// NetFlow version numbers.
pub const NETFLOW_V5: u16 = 5;
pub const NETFLOW_V9: u16 = 9;

/// NetFlow v5 fixed sizes in bytes.
pub const NF5_HEADER_SIZE: usize = 24;
pub const NF5_RECORD_SIZE: usize = 48;

/// NetFlow v9 fixed sizes & FlowSet IDs (RFC 3954).
pub const NF9_HEADER_SIZE: usize = 20;
pub const NF9_FLOWSET_HEADER_SIZE: usize = 4;
pub const NF9_FLOWSET_TEMPLATE: u16 = 0;
pub const NF9_FLOWSET_OPTIONS_TEMPLATE: u16 = 1;
pub const NF9_FLOWSET_DATA_MIN: u16 = 256;

// ---------------------------------------------------------------------------
// NetFlow v5 Data Models
// ---------------------------------------------------------------------------

/// Decoded 24-byte Cisco NetFlow v5 header.
pub struct Nf5Header {
    version: u16 = NETFLOW_V5,
    count: u16 = 0,
    sys_uptime_ms: u32 = 0,
    unix_secs: u32 = 0,
    unix_nsecs: u32 = 0,
    flow_sequence: u32 = 0,
    engine_type: u8 = 0,
    engine_id: u8 = 0,
    sampling_mode: u8 = 0,
    sampling_interval: u16 = 0,
}

/// Decoded 48-byte Cisco NetFlow v5 Flow Record.
pub struct Nf5Record {
    src_ip: [4]u8 = [0; 4],
    dst_ip: [4]u8 = [0; 4],
    next_hop: [4]u8 = [0; 4],
    input_snmp: u16 = 0,
    output_snmp: u16 = 0,
    packets: u32 = 0,
    octets: u32 = 0,
    first_uptime: u32 = 0,
    last_uptime: u32 = 0,
    src_port: u16 = 0,
    dst_port: u16 = 0,
    tcp_flags: u8 = 0,
    protocol: u8 = 0,
    tos: u8 = 0,
    src_as: u16 = 0,
    dst_as: u16 = 0,
    src_mask: u8 = 0,
    dst_mask: u8 = 0,
}

/// Zero-allocation iterator over 48-byte NetFlow v5 Flow Records.
pub struct Nf5RecordIter {
    data: []const u8 = "",
    pos: usize = 0,
    remaining: u16 = 0,

    pub fn init(records_bytes: []const u8, count: u16): Nf5RecordIter {
        return Nf5RecordIter {
            data: records_bytes,
            pos: 0,
            remaining: count,
        };
    }

    fn next(*_): Nf5Record? {
        if (_.remaining == 0 || _.data.len - _.pos < NF5_RECORD_SIZE) {
            return null;
        }
        const p = _.pos;
        _.pos += NF5_RECORD_SIZE;
        _.remaining -= 1;

        return Nf5Record {
            src_ip: [_.data[p], _.data[p + 1], _.data[p + 2], _.data[p + 3]],
            dst_ip: [_.data[p + 4], _.data[p + 5], _.data[p + 6], _.data[p + 7]],
            next_hop: [_.data[p + 8], _.data[p + 9], _.data[p + 10], _.data[p + 11]],
            input_snmp: readBeU16(_.data, p + 12),
            output_snmp: readBeU16(_.data, p + 14),
            packets: readBeU32(_.data, p + 16),
            octets: readBeU32(_.data, p + 20),
            first_uptime: readBeU32(_.data, p + 24),
            last_uptime: readBeU32(_.data, p + 28),
            src_port: readBeU16(_.data, p + 32),
            dst_port: readBeU16(_.data, p + 34),
            tcp_flags: _.data[p + 37],
            protocol: _.data[p + 38],
            tos: _.data[p + 39],
            src_as: readBeU16(_.data, p + 40),
            dst_as: readBeU16(_.data, p + 42),
            src_mask: _.data[p + 44],
            dst_mask: _.data[p + 45],
        };
    }
}

/// Zero-copy decoded NetFlow v5 packet.
pub struct Nf5Packet {
    header: Nf5Header = Nf5Header {},
    records_bytes: []const u8 = "",

    fn records(): Nf5RecordIter {
        return Nf5RecordIter.init(_.records_bytes, _.header.count);
    }
}

// ---------------------------------------------------------------------------
// NetFlow v9 Data Models (RFC 3954)
// ---------------------------------------------------------------------------

/// Decoded 20-byte RFC 3954 NetFlow v9 header.
pub struct Nf9Header {
    version: u16 = NETFLOW_V9,
    count: u16 = 0,
    sys_uptime_ms: u32 = 0,
    unix_secs: u32 = 0,
    sequence_number: u32 = 0,
    source_id: u32 = 0,
}

/// Decoded NetFlow v9 Template Field Specifier (`2B type + 2B length`).
pub struct Nf9FieldSpec {
    field_type: u16 = 0,
    field_len: u16 = 0,
}

/// Zero-allocation iterator over `Nf9FieldSpec` entries inside a Template Record.
pub struct Nf9FieldSpecIter {
    data: []const u8 = "",
    pos: usize = 0,
    remaining: u16 = 0,

    pub fn init(fields_bytes: []const u8, field_count: u16): Nf9FieldSpecIter {
        return Nf9FieldSpecIter {
            data: fields_bytes,
            pos: 0,
            remaining: field_count,
        };
    }

    fn next(*_): Nf9FieldSpec? {
        if (_.remaining == 0 || _.data.len - _.pos < 4) {
            return null;
        }
        const ftype = readBeU16(_.data, _.pos);
        const flen = readBeU16(_.data, _.pos + 2);
        _.pos += 4;
        _.remaining -= 1;
        return Nf9FieldSpec {
            field_type: ftype,
            field_len: flen,
        };
    }
}

/// Zero-copy decoded NetFlow v9 Template Record.
pub struct Nf9TemplateRecord {
    template_id: u16 = 0,
    field_count: u16 = 0,
    fields_bytes: []const u8 = "",

    fn fields(): Nf9FieldSpecIter {
        return Nf9FieldSpecIter.init(_.fields_bytes, _.field_count);
    }
}

/// Zero-allocation iterator over Template Records inside a Template FlowSet (`flowset_id == 0`).
pub struct Nf9TemplateIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,

    pub fn init(flowset_body: []const u8): Nf9TemplateIter {
        return Nf9TemplateIter {
            data: flowset_body,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): Nf9TemplateRecord? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        const rem = _.data.len - _.pos;
        // RFC 3954 allows up to 3 trailing padding bytes at the end of a FlowSet
        if (rem < 4) {
            return null;
        }
        const template_id = readBeU16(_.data, _.pos);
        const field_count = readBeU16(_.data, _.pos + 2);
        if (template_id == 0 && field_count == 0) {
            _.pos = _.data.len;
            return null;
        }
        if (template_id < NF9_FLOWSET_DATA_MIN) {
            _.failed = true;
            return null;
        }
        const fields_len = (field_count as usize) * 4;
        if (rem - 4 < fields_len) {
            _.failed = true;
            return null;
        }
        const fields_bytes = _.data[(_.pos + 4)..(_.pos + 4 + fields_len)];
        _.pos += 4 + fields_len;
        return Nf9TemplateRecord {
            template_id: template_id,
            field_count: field_count,
            fields_bytes: fields_bytes,
        };
    }
}

/// Zero-copy decoded NetFlow v9 FlowSet (`4B header + body`).
pub struct Nf9FlowSet {
    flowset_id: u16 = 0,
    length: u16 = 4,
    body: []const u8 = "",

    fn isTemplate(): bool {
        return _.flowset_id == NF9_FLOWSET_TEMPLATE;
    }

    fn isOptionsTemplate(): bool {
        return _.flowset_id == NF9_FLOWSET_OPTIONS_TEMPLATE;
    }

    fn isData(): bool {
        return _.flowset_id >= NF9_FLOWSET_DATA_MIN;
    }

    fn templates(): Nf9TemplateIter {
        if (!_.isTemplate()) {
            return Nf9TemplateIter.init("");
        }
        return Nf9TemplateIter.init(_.body);
    }
}

/// Zero-allocation iterator over FlowSets in a NetFlow v9 packet.
pub struct Nf9FlowSetIter {
    data: []const u8 = "",
    pos: usize = 0,
    failed: bool = false,

    pub fn init(flowsets_bytes: []const u8): Nf9FlowSetIter {
        return Nf9FlowSetIter {
            data: flowsets_bytes,
            pos: 0,
            failed: false,
        };
    }

    fn hasError(): bool {
        return _.failed;
    }

    fn next(*_): Nf9FlowSet? {
        if (_.failed || _.pos >= _.data.len) {
            return null;
        }
        const rem = _.data.len - _.pos;
        if (rem < NF9_FLOWSET_HEADER_SIZE) {
            _.failed = true;
            return null;
        }
        const flowset_id = readBeU16(_.data, _.pos);
        const length = readBeU16(_.data, _.pos + 2);
        const len_usize = length as usize;
        if (len_usize < NF9_FLOWSET_HEADER_SIZE || len_usize > rem) {
            _.failed = true;
            return null;
        }
        const body = _.data[(_.pos + NF9_FLOWSET_HEADER_SIZE)..(_.pos + len_usize)];
        _.pos += len_usize;
        return Nf9FlowSet {
            flowset_id: flowset_id,
            length: length,
            body: body,
        };
    }
}

/// Zero-copy decoded NetFlow v9 packet.
pub struct Nf9Packet {
    header: Nf9Header = Nf9Header {},
    flowsets_bytes: []const u8 = "",

    fn flowsets(): Nf9FlowSetIter {
        return Nf9FlowSetIter.init(_.flowsets_bytes);
    }
}

// ---------------------------------------------------------------------------
// Internal Big-Endian Helpers
// ---------------------------------------------------------------------------

fn readBeU16(data: []const u8, off: usize): u16 {
    return ((data[off] as u16) << 8) | (data[off + 1] as u16);
}

fn readBeU32(data: []const u8, off: usize): u32 {
    return ((data[off] as u32) << 24)
        | ((data[off + 1] as u32) << 16)
        | ((data[off + 2] as u32) << 8)
        | (data[off + 3] as u32);
}

// ---------------------------------------------------------------------------
// Public NetFlow Parsing API
// ---------------------------------------------------------------------------

/// Peeks the 16-bit big-endian NetFlow version field (`5` or `9`), or `null` if `< 2` bytes.
pub fn peekNetflowVersion(data: []const u8): u16? {
    if (data.len < 2) {
        return null;
    }
    return readBeU16(data, 0);
}

/// Strictly parses a Cisco NetFlow v5 packet (`version == 5`, `1 <= count <= 30`, `24 + count * 48 <= data.len`).
pub fn parseNetflowV5(data: []const u8): Nf5Packet? {
    if (data.len < NF5_HEADER_SIZE) {
        return null;
    }
    const version = readBeU16(data, 0);
    if (version != NETFLOW_V5) {
        return null;
    }
    const count = readBeU16(data, 2);
    if (count == 0 || count > 30) {
        return null;
    }
    const records_len = (count as usize) * NF5_RECORD_SIZE;
    if (data.len - NF5_HEADER_SIZE < records_len) {
        return null;
    }
    const sampling_raw = readBeU16(data, 22);
    return Nf5Packet {
        header: Nf5Header {
            version: version,
            count: count,
            sys_uptime_ms: readBeU32(data, 4),
            unix_secs: readBeU32(data, 8),
            unix_nsecs: readBeU32(data, 12),
            flow_sequence: readBeU32(data, 16),
            engine_type: data[20],
            engine_id: data[21],
            sampling_mode: ((sampling_raw >> 14) & 0x03) as u8,
            sampling_interval: sampling_raw & 0x3FFF,
        },
        records_bytes: data[NF5_HEADER_SIZE..(NF5_HEADER_SIZE + records_len)],
    };
}

/// Strictly parses an RFC 3954 NetFlow v9 packet (`version == 9`, `data.len >= 20`).
pub fn parseNetflowV9(data: []const u8): Nf9Packet? {
    if (data.len < NF9_HEADER_SIZE) {
        return null;
    }
    const version = readBeU16(data, 0);
    if (version != NETFLOW_V9) {
        return null;
    }
    return Nf9Packet {
        header: Nf9Header {
            version: version,
            count: readBeU16(data, 2),
            sys_uptime_ms: readBeU32(data, 4),
            unix_secs: readBeU32(data, 8),
            sequence_number: readBeU32(data, 12),
            source_id: readBeU32(data, 16),
        },
        flowsets_bytes: data[NF9_HEADER_SIZE..data.len],
    };
}

/// Returns `true` if `data` is a structurally valid NetFlow v5 or NetFlow v9 packet.
pub fn isNetflow(data: []const u8): bool {
    if (v5 := parseNetflowV5(data)) {
        * = v5;
        return true;
    }
    if (v9 := parseNetflowV9(data)) {
        * = v9;
        return true;
    }
    return false;
}
