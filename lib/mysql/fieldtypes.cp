// Source: Absorbed from /data/dev/bun/src/sql/mysql/MySQLTypes.cp | Synced: 2026-11-21T16:59:39Z

/// 常用 MySQL 字符集 ID
pub const CHARSET_LATIN1_SWEDISH_CI: u16 = 8;
pub const CHARSET_UTF8MB3_GENERAL_CI: u16 = 33;
pub const CHARSET_UTF8MB4_GENERAL_CI: u16 = 45;
pub const CHARSET_UTF8MB4_BIN: u16 = 46;
pub const CHARSET_BINARY: u16 = 63;
pub const CHARSET_UTF8MB4_UNICODE_CI: u16 = 224;

/// MySQL 协议字段类型与 ColumnDefinition41 编解码错误集
pub err MySqlWireError {
    UnexpectedEof,
    InvalidFieldType,
    InvalidLengthEncodedInt,
    InvalidFixedFieldsLength,
    BufferTooSmall,
    OutOfMemory,
}

/// MySQL 协议全部 31 种列字段类型（Protocol::MYSQL_TYPE_*）
pub enum FieldType: u8 {
    MYSQL_TYPE_DECIMAL = 0x00,
    MYSQL_TYPE_TINY = 0x01,
    MYSQL_TYPE_SHORT = 0x02,
    MYSQL_TYPE_LONG = 0x03,
    MYSQL_TYPE_FLOAT = 0x04,
    MYSQL_TYPE_DOUBLE = 0x05,
    MYSQL_TYPE_NULL = 0x06,
    MYSQL_TYPE_TIMESTAMP = 0x07,
    MYSQL_TYPE_LONGLONG = 0x08,
    MYSQL_TYPE_INT24 = 0x09,
    MYSQL_TYPE_DATE = 0x0a,
    MYSQL_TYPE_TIME = 0x0b,
    MYSQL_TYPE_DATETIME = 0x0c,
    MYSQL_TYPE_YEAR = 0x0d,
    MYSQL_TYPE_NEWDATE = 0x0e,
    MYSQL_TYPE_VARCHAR = 0x0f,
    MYSQL_TYPE_BIT = 0x10,
    MYSQL_TYPE_TIMESTAMP2 = 0x11,
    MYSQL_TYPE_DATETIME2 = 0x12,
    MYSQL_TYPE_TIME2 = 0x13,
    MYSQL_TYPE_JSON = 0xf5,
    MYSQL_TYPE_NEWDECIMAL = 0xf6,
    MYSQL_TYPE_ENUM = 0xf7,
    MYSQL_TYPE_SET = 0xf8,
    MYSQL_TYPE_TINY_BLOB = 0xf9,
    MYSQL_TYPE_MEDIUM_BLOB = 0xfa,
    MYSQL_TYPE_LONG_BLOB = 0xfb,
    MYSQL_TYPE_BLOB = 0xfc,
    MYSQL_TYPE_VAR_STRING = 0xfd,
    MYSQL_TYPE_STRING = 0xfe,
    MYSQL_TYPE_GEOMETRY = 0xff,

    fn toByte(): u8 {
        return @intFromEnum(_);
    }

    fn isBinaryFormatSupported(): bool {
        return switch (_) {
            .MYSQL_TYPE_TINY,
            .MYSQL_TYPE_SHORT,
            .MYSQL_TYPE_LONG,
            .MYSQL_TYPE_LONGLONG,
            .MYSQL_TYPE_FLOAT,
            .MYSQL_TYPE_DOUBLE,
            .MYSQL_TYPE_TIME,
            .MYSQL_TYPE_DATE,
            .MYSQL_TYPE_DATETIME,
            .MYSQL_TYPE_TIMESTAMP => true,
            else => false,
        };
    }

    fn isInteger(): bool {
        return switch (_) {
            .MYSQL_TYPE_TINY,
            .MYSQL_TYPE_SHORT,
            .MYSQL_TYPE_LONG,
            .MYSQL_TYPE_LONGLONG,
            .MYSQL_TYPE_INT24,
            .MYSQL_TYPE_YEAR,
            .MYSQL_TYPE_BIT => true,
            else => false,
        };
    }

    fn isFloat(): bool {
        return switch (_) {
            .MYSQL_TYPE_FLOAT,
            .MYSQL_TYPE_DOUBLE,
            .MYSQL_TYPE_DECIMAL,
            .MYSQL_TYPE_NEWDECIMAL => true,
            else => false,
        };
    }

    fn isString(): bool {
        return switch (_) {
            .MYSQL_TYPE_VARCHAR,
            .MYSQL_TYPE_VAR_STRING,
            .MYSQL_TYPE_STRING,
            .MYSQL_TYPE_ENUM,
            .MYSQL_TYPE_SET,
            .MYSQL_TYPE_JSON,
            .MYSQL_TYPE_TINY_BLOB,
            .MYSQL_TYPE_MEDIUM_BLOB,
            .MYSQL_TYPE_LONG_BLOB,
            .MYSQL_TYPE_BLOB => true,
            else => false,
        };
    }

    fn isTemporal(): bool {
        return switch (_) {
            .MYSQL_TYPE_TIMESTAMP,
            .MYSQL_TYPE_DATE,
            .MYSQL_TYPE_TIME,
            .MYSQL_TYPE_DATETIME,
            .MYSQL_TYPE_YEAR,
            .MYSQL_TYPE_NEWDATE,
            .MYSQL_TYPE_TIMESTAMP2,
            .MYSQL_TYPE_DATETIME2,
            .MYSQL_TYPE_TIME2 => true,
            else => false,
        };
    }

    fn name(): string {
        return switch (_) {
            .MYSQL_TYPE_DECIMAL => "DECIMAL",
            .MYSQL_TYPE_TINY => "TINY",
            .MYSQL_TYPE_SHORT => "SHORT",
            .MYSQL_TYPE_LONG => "LONG",
            .MYSQL_TYPE_FLOAT => "FLOAT",
            .MYSQL_TYPE_DOUBLE => "DOUBLE",
            .MYSQL_TYPE_NULL => "NULL",
            .MYSQL_TYPE_TIMESTAMP => "TIMESTAMP",
            .MYSQL_TYPE_LONGLONG => "LONGLONG",
            .MYSQL_TYPE_INT24 => "INT24",
            .MYSQL_TYPE_DATE => "DATE",
            .MYSQL_TYPE_TIME => "TIME",
            .MYSQL_TYPE_DATETIME => "DATETIME",
            .MYSQL_TYPE_YEAR => "YEAR",
            .MYSQL_TYPE_NEWDATE => "NEWDATE",
            .MYSQL_TYPE_VARCHAR => "VARCHAR",
            .MYSQL_TYPE_BIT => "BIT",
            .MYSQL_TYPE_TIMESTAMP2 => "TIMESTAMP2",
            .MYSQL_TYPE_DATETIME2 => "DATETIME2",
            .MYSQL_TYPE_TIME2 => "TIME2",
            .MYSQL_TYPE_JSON => "JSON",
            .MYSQL_TYPE_NEWDECIMAL => "NEWDECIMAL",
            .MYSQL_TYPE_ENUM => "ENUM",
            .MYSQL_TYPE_SET => "SET",
            .MYSQL_TYPE_TINY_BLOB => "TINY_BLOB",
            .MYSQL_TYPE_MEDIUM_BLOB => "MEDIUM_BLOB",
            .MYSQL_TYPE_LONG_BLOB => "LONG_BLOB",
            .MYSQL_TYPE_BLOB => "BLOB",
            .MYSQL_TYPE_VAR_STRING => "VAR_STRING",
            .MYSQL_TYPE_STRING => "STRING",
            .MYSQL_TYPE_GEOMETRY => "GEOMETRY",
        };
    }
}

/// 从单字节解析标准 MySQL `FieldType`，未定义区间（0x14..0xf4）返回 `err.InvalidFieldType`。
pub fn fieldTypeFromByte(b: u8): FieldType ! MySqlWireError {
    return switch (b) {
        0x00 => FieldType.MYSQL_TYPE_DECIMAL,
        0x01 => FieldType.MYSQL_TYPE_TINY,
        0x02 => FieldType.MYSQL_TYPE_SHORT,
        0x03 => FieldType.MYSQL_TYPE_LONG,
        0x04 => FieldType.MYSQL_TYPE_FLOAT,
        0x05 => FieldType.MYSQL_TYPE_DOUBLE,
        0x06 => FieldType.MYSQL_TYPE_NULL,
        0x07 => FieldType.MYSQL_TYPE_TIMESTAMP,
        0x08 => FieldType.MYSQL_TYPE_LONGLONG,
        0x09 => FieldType.MYSQL_TYPE_INT24,
        0x0a => FieldType.MYSQL_TYPE_DATE,
        0x0b => FieldType.MYSQL_TYPE_TIME,
        0x0c => FieldType.MYSQL_TYPE_DATETIME,
        0x0d => FieldType.MYSQL_TYPE_YEAR,
        0x0e => FieldType.MYSQL_TYPE_NEWDATE,
        0x0f => FieldType.MYSQL_TYPE_VARCHAR,
        0x10 => FieldType.MYSQL_TYPE_BIT,
        0x11 => FieldType.MYSQL_TYPE_TIMESTAMP2,
        0x12 => FieldType.MYSQL_TYPE_DATETIME2,
        0x13 => FieldType.MYSQL_TYPE_TIME2,
        0xf5 => FieldType.MYSQL_TYPE_JSON,
        0xf6 => FieldType.MYSQL_TYPE_NEWDECIMAL,
        0xf7 => FieldType.MYSQL_TYPE_ENUM,
        0xf8 => FieldType.MYSQL_TYPE_SET,
        0xf9 => FieldType.MYSQL_TYPE_TINY_BLOB,
        0xfa => FieldType.MYSQL_TYPE_MEDIUM_BLOB,
        0xfb => FieldType.MYSQL_TYPE_LONG_BLOB,
        0xfc => FieldType.MYSQL_TYPE_BLOB,
        0xfd => FieldType.MYSQL_TYPE_VAR_STRING,
        0xfe => FieldType.MYSQL_TYPE_STRING,
        0xff => FieldType.MYSQL_TYPE_GEOMETRY,
        else => return err.InvalidFieldType,
    };
}

/// MySQL 16 位列标志位域（与 ColumnDefinition41 协议逐位对齐）
pub struct ColumnFlags {
    not_null: bool = false,
    pri_key: bool = false,
    unique_key: bool = false,
    multiple_key: bool = false,
    blob: bool = false,
    unsigned: bool = false,
    zerofill: bool = false,
    binary: bool = false,
    is_enum: bool = false,
    auto_increment: bool = false,
    timestamp: bool = false,
    is_set: bool = false,
    no_default_value: bool = false,
    on_update_now: bool = false,
    num: bool = false,
    part_key: bool = false,

    fn toInt(): u16 {
        let bits: u16 = 0;
        if (_.not_null) { bits |= 0x0001; }
        if (_.pri_key) { bits |= 0x0002; }
        if (_.unique_key) { bits |= 0x0004; }
        if (_.multiple_key) { bits |= 0x0008; }
        if (_.blob) { bits |= 0x0010; }
        if (_.unsigned) { bits |= 0x0020; }
        if (_.zerofill) { bits |= 0x0040; }
        if (_.binary) { bits |= 0x0080; }
        if (_.is_enum) { bits |= 0x0100; }
        if (_.auto_increment) { bits |= 0x0200; }
        if (_.timestamp) { bits |= 0x0400; }
        if (_.is_set) { bits |= 0x0800; }
        if (_.no_default_value) { bits |= 0x1000; }
        if (_.on_update_now) { bits |= 0x2000; }
        if (_.num) { bits |= 0x4000; }
        if (_.part_key) { bits |= 0x8000; }
        return bits;
    }

    static fn fromInt(raw: u16): ColumnFlags {
        return ColumnFlags{
            not_null: (raw & 0x0001) != 0,
            pri_key: (raw & 0x0002) != 0,
            unique_key: (raw & 0x0004) != 0,
            multiple_key: (raw & 0x0008) != 0,
            blob: (raw & 0x0010) != 0,
            unsigned: (raw & 0x0020) != 0,
            zerofill: (raw & 0x0040) != 0,
            binary: (raw & 0x0080) != 0,
            is_enum: (raw & 0x0100) != 0,
            auto_increment: (raw & 0x0200) != 0,
            timestamp: (raw & 0x0400) != 0,
            is_set: (raw & 0x0800) != 0,
            no_default_value: (raw & 0x1000) != 0,
            on_update_now: (raw & 0x2000) != 0,
            num: (raw & 0x4000) != 0,
            part_key: (raw & 0x8000) != 0,
        };
    }
}

/// MySQL Length-Encoded Integer 解码结果
pub struct DecodedLenEncInt {
    value: u64,
    bytes_read: usize,
}

/// MySQL Length-Encoded String 解码结果
pub struct DecodedLenEncString {
    value: string,
    bytes_read: usize,
}

/// 计算 MySQL length-encoded integer 编码长度（1 / 3 / 4 / 9 字节）
pub fn lenEncIntSize(value: u64): usize {
    if (value < 251) {
        return 1;
    }
    if (value < 65536) {
        return 3;
    }
    if (value < 16777216) {
        return 4;
    }
    return 9;
}

/// 编码 MySQL length-encoded integer（小端字节序）
pub fn encodeLenEncInt(buf: []u8, value: u64): usize ! MySqlWireError {
    const need = lenEncIntSize(value);
    if (buf.len < need) {
        return err.BufferTooSmall;
    }
    if (need == 1) {
        buf[0] = value as u8;
        return 1;
    }
    if (need == 3) {
        buf[0] = 0xfc;
        buf[1] = (value & 0xFF) as u8;
        buf[2] = ((value >> 8) & 0xFF) as u8;
        return 3;
    }
    if (need == 4) {
        buf[0] = 0xfd;
        buf[1] = (value & 0xFF) as u8;
        buf[2] = ((value >> 8) & 0xFF) as u8;
        buf[3] = ((value >> 16) & 0xFF) as u8;
        return 4;
    }
    buf[0] = 0xfe;
    buf[1] = (value & 0xFF) as u8;
    buf[2] = ((value >> 8) & 0xFF) as u8;
    buf[3] = ((value >> 16) & 0xFF) as u8;
    buf[4] = ((value >> 24) & 0xFF) as u8;
    buf[5] = ((value >> 32) & 0xFF) as u8;
    buf[6] = ((value >> 40) & 0xFF) as u8;
    buf[7] = ((value >> 48) & 0xFF) as u8;
    buf[8] = ((value >> 56) & 0xFF) as u8;
    return 9;
}

/// 解码 MySQL length-encoded integer（0xfb/0xff 为非法前缀）
pub fn decodeLenEncInt(data: []const u8): DecodedLenEncInt ! MySqlWireError {
    if (data.len < 1) {
        return err.UnexpectedEof;
    }
    const first = data[0];
    if (first < 251) {
        return DecodedLenEncInt{ value: first as u64, bytes_read: 1 };
    }
    if (first == 0xfc) {
        if (data.len < 3) {
            return err.UnexpectedEof;
        }
        const v: u64 = (data[1] as u64) | ((data[2] as u64) << 8);
        return DecodedLenEncInt{ value: v, bytes_read: 3 };
    }
    if (first == 0xfd) {
        if (data.len < 4) {
            return err.UnexpectedEof;
        }
        const v: u64 = (data[1] as u64) | ((data[2] as u64) << 8) | ((data[3] as u64) << 16);
        return DecodedLenEncInt{ value: v, bytes_read: 4 };
    }
    if (first == 0xfe) {
        if (data.len < 9) {
            return err.UnexpectedEof;
        }
        const v: u64 = (data[1] as u64)
            | ((data[2] as u64) << 8)
            | ((data[3] as u64) << 16)
            | ((data[4] as u64) << 24)
            | ((data[5] as u64) << 32)
            | ((data[6] as u64) << 40)
            | ((data[7] as u64) << 48)
            | ((data[8] as u64) << 56);
        return DecodedLenEncInt{ value: v, bytes_read: 9 };
    }
    return err.InvalidLengthEncodedInt;
}

/// 编码 MySQL length-encoded string
pub fn encodeLenEncString(buf: []u8, s: string): usize ! MySqlWireError {
    const prefix_len = lenEncIntSize(s.len as u64);
    const total = prefix_len + s.len;
    if (buf.len < total) {
        return err.BufferTooSmall;
    }
    const pos = encodeLenEncInt(buf, s.len as u64)?;
    if (s.len > 0) {
        @memcpy(buf[pos .. pos + s.len], s);
    }
    return pos + s.len;
}

/// 解码 MySQL length-encoded string（零拷贝切片）
pub fn decodeLenEncString(data: []const u8): DecodedLenEncString ! MySqlWireError {
    const hdr = decodeLenEncInt(data)?;
    const str_len: usize = hdr.value as usize;
    if (str_len > data.len - hdr.bytes_read) {
        return err.UnexpectedEof;
    }
    return DecodedLenEncString{
        value: data[hdr.bytes_read .. hdr.bytes_read + str_len],
        bytes_read: hdr.bytes_read + str_len,
    };
}

/// 带 RAII 析构的 MySQL 线协议字节缓冲区
pub struct MySqlWireBuffer {
    data: []u8,
    allocator: std.mem.Allocator,

    fn slice(): []const u8 {
        return _.data;
    }

    fn len(): usize {
        return _.data.len;
    }

    ~init() {
        if (_.data.len > 0) {
            _.allocator.free(_.data);
            _.data = [];
        }
    }
}

/// MySQL 4.1+ `ColumnDefinition41` 列元数据报文结构
pub struct ColumnDefinition41 {
    catalog: string = "def",
    schema: string = "",
    table: string = "",
    org_table: string = "",
    name: string = "",
    org_name: string = "",
    character_set: u16 = CHARSET_UTF8MB4_GENERAL_CI,
    column_length: u32 = 0,
    column_type: FieldType = FieldType.MYSQL_TYPE_NULL,
    flags: ColumnFlags = ColumnFlags{},
    decimals: u8 = 0,

    fn encodedSize(): usize {
        return lenEncIntSize(_.catalog.len as u64) + _.catalog.len
            + lenEncIntSize(_.schema.len as u64) + _.schema.len
            + lenEncIntSize(_.table.len as u64) + _.table.len
            + lenEncIntSize(_.org_table.len as u64) + _.org_table.len
            + lenEncIntSize(_.name.len as u64) + _.name.len
            + lenEncIntSize(_.org_name.len as u64) + _.org_name.len
            + usize(13);
    }
}

/// `ColumnDefinition41` 解码结果
pub struct DecodedColumnDef41 {
    column: ColumnDefinition41,
    bytes_read: usize,
}

/// 将 `ColumnDefinition41` 编码为标准 MySQL 4.1+ 线协议字节包（RAII `MySqlWireBuffer`）。
pub fn encodeColumnDefinition41(
    allocator: std.mem.Allocator,
    col: ColumnDefinition41,
): MySqlWireBuffer ! MySqlWireError {
    const total = col.encodedSize();
    let buf = allocator.alloc(u8, total) catch return err.OutOfMemory;
    errdefer allocator.free(buf);

    let pos: usize = 0;
    pos += encodeLenEncString(buf[pos..], col.catalog)?;
    pos += encodeLenEncString(buf[pos..], col.schema)?;
    pos += encodeLenEncString(buf[pos..], col.table)?;
    pos += encodeLenEncString(buf[pos..], col.org_table)?;
    pos += encodeLenEncString(buf[pos..], col.name)?;
    pos += encodeLenEncString(buf[pos..], col.org_name)?;

    // fixed_length_fields_length = 0x0c (12 bytes follow)
    buf[pos] = 0x0c;
    pos += 1;

    // character_set (u16 LE)
    buf[pos] = (col.character_set & 0xFF) as u8;
    buf[pos + 1] = ((col.character_set >> 8) & 0xFF) as u8;
    pos += 2;

    // column_length (u32 LE)
    buf[pos] = (col.column_length & 0xFF) as u8;
    buf[pos + 1] = ((col.column_length >> 8) & 0xFF) as u8;
    buf[pos + 2] = ((col.column_length >> 16) & 0xFF) as u8;
    buf[pos + 3] = ((col.column_length >> 24) & 0xFF) as u8;
    pos += 4;

    // column_type (u8)
    buf[pos] = col.column_type.toByte();
    pos += 1;

    // flags (u16 LE)
    const raw_flags = col.flags.toInt();
    buf[pos] = (raw_flags & 0xFF) as u8;
    buf[pos + 1] = ((raw_flags >> 8) & 0xFF) as u8;
    pos += 2;

    // decimals (u8) + 2 filler bytes (0x00, 0x00)
    buf[pos] = col.decimals;
    buf[pos + 1] = 0;
    buf[pos + 2] = 0;

    return MySqlWireBuffer{
        data: buf,
        allocator: allocator,
    };
}

/// 从字节流解码 MySQL `ColumnDefinition41`（零拷贝切片引用字符串，严格校验固定字段长度 0x0c 与字段类型）。
pub fn decodeColumnDefinition41(data: []const u8): DecodedColumnDef41 ! MySqlWireError {
    let pos: usize = 0;

    const cat = decodeLenEncString(data[pos..])?;
    pos += cat.bytes_read;
    const sch = decodeLenEncString(data[pos..])?;
    pos += sch.bytes_read;
    const tbl = decodeLenEncString(data[pos..])?;
    pos += tbl.bytes_read;
    const org_tbl = decodeLenEncString(data[pos..])?;
    pos += org_tbl.bytes_read;
    const col_name = decodeLenEncString(data[pos..])?;
    pos += col_name.bytes_read;
    const org_col_name = decodeLenEncString(data[pos..])?;
    pos += org_col_name.bytes_read;

    const fixed_len = decodeLenEncInt(data[pos..])?;
    pos += fixed_len.bytes_read;
    if (fixed_len.value != 12) {
        return err.InvalidFixedFieldsLength;
    }
    if (usize(12) > data.len - pos) {
        return err.UnexpectedEof;
    }

    const charset: u16 = (data[pos] as u16) | ((data[pos + 1] as u16) << 8);
    pos += 2;

    const col_len: u32 = (data[pos] as u32)
        | ((data[pos + 1] as u32) << 8)
        | ((data[pos + 2] as u32) << 16)
        | ((data[pos + 3] as u32) << 24);
    pos += 4;

    const col_type = fieldTypeFromByte(data[pos])?;
    pos += 1;

    const raw_flags: u16 = (data[pos] as u16) | ((data[pos + 1] as u16) << 8);
    pos += 2;

    const decimals = data[pos];
    pos += 3; // decimals (1) + filler (2)

    return DecodedColumnDef41{
        column: ColumnDefinition41{
            catalog: cat.value,
            schema: sch.value,
            table: tbl.value,
            org_table: org_tbl.value,
            name: col_name.value,
            org_name: org_col_name.value,
            character_set: charset,
            column_length: col_len,
            column_type: col_type,
            flags: ColumnFlags.fromInt(raw_flags),
            decimals: decimals,
        },
        bytes_read: pos,
    };
}
