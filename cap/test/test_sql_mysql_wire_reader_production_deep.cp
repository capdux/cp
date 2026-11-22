// Production Deep Tests for cap:sql MySQL Wire Reader (lib/sql/mysql_wire_reader.cp)
import { test, expect_eq } from "cap:test";
import {
    MYSQL_LENENC_NULL_MARKER,
    MySqlWireReaderError,
    MySqlWireReader,
} from "cap:sql";

test("sql mysql_wire_reader: little-endian integers and IEEE-754 floats", fn() {
    const wire: [34]u8 = [
        0x7f,                                           // u8: 127
        0x34, 0x12,                                     // u16: 0x1234
        0x03, 0x02, 0x01,                               // u24: 0x010203
        0x78, 0x56, 0x34, 0x12,                         // u32: 0x12345678
        0xfe, 0xff, 0xff, 0xff,                         // i32: -2
        0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, // u64: 0x0100000000000001
        0x00, 0x00, 0xc0, 0x3f,                         // f32: 1.5 (LE 0x3fc00000)
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0xc0, // f64: -2.25 (LE 0xc002000000000000)
    ];

    let r = MySqlWireReader.init(wire[0..]);
    expect_eq(r.remaining(), @as(usize, 34));
    expect_eq(r.readByte() catch unreachable, @as(u8, 0x7f));
    expect_eq(r.readU16() catch unreachable, @as(u16, 0x1234));
    expect_eq(r.readU24() catch unreachable, @as(u32, 0x010203));
    expect_eq(r.readU32() catch unreachable, @as(u32, 0x12345678));
    expect_eq(r.readI32() catch unreachable, @as(i32, -2));
    expect_eq(r.readU64() catch unreachable, @as(u64, 0x0100000000000001));
    expect_eq(r.readF32() catch unreachable, @as(f32, 1.5));
    expect_eq(r.readF64() catch unreachable, @as(f64, -2.25));
    expect_eq(r.remaining(), @as(usize, 0));
});

test("sql mysql_wire_reader: zero-copy readZ, peek, skip and ensureCapacity", fn() {
    const wire: [13]u8 = [
        '8', '.', '0', '.', '3', '6', 0, // "8.0.36\0"
        0,                                // ""
        'a', 'b', 0,                      // "ab\0"
        0xde, 0xad,                       // trailing bytes
    ];

    let r = MySqlWireReader.init(wire[0..]);
    expect_eq(r.ensureCapacity(13) catch unreachable, @as(usize, 13));
    expect_eq(r.readZ() catch unreachable, "8.0.36");
    expect_eq(r.readZ() catch unreachable, "");
    expect_eq(r.readZ() catch unreachable, "ab");
    expect_eq(r.peek().len, @as(usize, 2));
    expect_eq(r.peek()[0], @as(u8, 0xde));
    // skip clamps to buffer end
    expect_eq(r.skip(100), @as(usize, 2));
    expect_eq(r.remaining(), @as(usize, 0));
});

test("sql mysql_wire_reader: lenenc-int, lenenc-str and NULL marker (0xfb)", fn() {
    const wire: [20]u8 = [
        0x2a,                               // lenenc-int: 42 (1 byte)
        0xfc, 0x00, 0x01,                   // lenenc-int: 256 (3 bytes)
        0x03, 'd', 'e', 'f',                // lenenc-str: "def"
        0x00,                               // lenenc-str: ""
        MYSQL_LENENC_NULL_MARKER,           // NULL (0xfb)
        0x05, 'u', 's', 'e', 'r', 's',      // lenenc-str: "users"
        0xfd, 0x01, 0x00, 0x01,             // lenenc-int: 65537 (4 bytes)
    ];

    let r = MySqlWireReader.init(wire[0..]);
    expect_eq(r.readLenEncInt() catch unreachable, @as(u64, 42));

    const v2 = r.readLenEncIntWithSize() catch unreachable;
    expect_eq(v2.value, @as(u64, 256));
    expect_eq(v2.bytes_read, @as(usize, 3));

    expect_eq(r.readLenEncString() catch unreachable, "def");
    expect_eq(r.readLenEncString() catch unreachable, "");

    const null_col = r.readLenEncStringOrNull() catch unreachable;
    expect_eq(null_col == null, true);

    const non_null_col = r.readLenEncStringOrNull() catch unreachable;
    expect_eq(non_null_col ?? "", "users");

    expect_eq(r.readLenEncInt() catch unreachable, @as(u64, 65537));
    expect_eq(r.remaining(), @as(usize, 0));
});

test("sql mysql_wire_reader: message bookmarking, relative seek and rewind", fn() {
    const wire: [10]u8 = [0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a];
    let r = MySqlWireReader.init(wire[0..]);

    * = r.readBytes(2) catch unreachable;
    r.markMessageStart();
    expect_eq(r.offset(), @as(usize, 2));
    expect_eq(r.offsetFromStart(), @as(usize, 0));

    * = r.readU32() catch unreachable;
    expect_eq(r.offsetFromStart(), @as(usize, 4));

    // Seek relative to message_start
    expect_eq(r.setOffsetFromStart(2) catch unreachable, @as(usize, 4));
    expect_eq(r.readByte() catch unreachable, @as(u8, 0x05));

    // Rewind back to message_start
    r.rewindToMessageStart();
    expect_eq(r.offset(), @as(usize, 2));
    expect_eq(r.readByte() catch unreachable, @as(u8, 0x03));
});

test("sql mysql_wire_reader: MySQL 4.1 packet header + ColumnDefinition41 prefix decode", fn() {
    // 4-byte MySQL packet header (payload_len=19, seq_id=2) + catalog/schema/table + charset/len/type
    const pkt: [23]u8 = [
        0x13, 0x00, 0x00, 0x02,       // len=19, seq=2
        0x03, 'd', 'e', 'f',          // catalog = "def"
        0x02, 'd', 'b',               // schema = "db"
        0x01, 't',                    // table = "t"
        0x0c,                         // fixed_fields_len = 12
        0x2d, 0x00,                   // charset = 45 (utf8mb4_general_ci)
        0x10, 0x00, 0x00, 0x00,       // column_length = 16
        0xfd,                         // MYSQL_TYPE_VAR_STRING (253)
        0x01, 0x00,                   // flags = NOT_NULL (1)
    ];

    let r = MySqlWireReader.init(pkt[0..]);
    r.markMessageStart();
    const payload_len = r.readU24() catch unreachable;
    const seq_id = r.readByte() catch unreachable;
    expect_eq(payload_len, @as(u32, 19));
    expect_eq(seq_id, @as(u8, 2));

    expect_eq(r.readLenEncString() catch unreachable, "def");
    expect_eq(r.readLenEncString() catch unreachable, "db");
    expect_eq(r.readLenEncString() catch unreachable, "t");
    expect_eq(r.readLenEncInt() catch unreachable, @as(u64, 12));
    expect_eq(r.readU16() catch unreachable, @as(u16, 45));
    expect_eq(r.readU32() catch unreachable, @as(u32, 16));
    expect_eq(r.readByte() catch unreachable, @as(u8, 0xfd));
    expect_eq(r.readU16() catch unreachable, @as(u16, 1));
    expect_eq(r.remaining(), @as(usize, 0));
});

test("sql mysql_wire_reader: ShortRead, UnterminatedString, InvalidEncodedLength and InvalidOffset guards", fn() {
    // 1. ShortRead on truncated u32
    const short_u32: [2]u8 = [0x01, 0x02];
    let r1 = MySqlWireReader.init(short_u32[0..]);
    let got_short = false;
    * = r1.readU32() catch |e| {
        got_short = (e == MySqlWireReaderError.ShortRead);
        @as(u32, 0)
    };
    expect_eq(got_short, true);
    expect_eq(r1.offset(), @as(usize, 0));

    // 2. UnterminatedString on readZ without NUL
    const no_nul: [3]u8 = ['a', 'b', 'c'];
    let r2 = MySqlWireReader.init(no_nul[0..]);
    let got_unterm = false;
    * = r2.readZ() catch |e| {
        got_unterm = (e == MySqlWireReaderError.UnterminatedString);
        ""
    };
    expect_eq(got_unterm, true);

    // 3. InvalidEncodedInteger on truncated 0xfc marker (needs 3 bytes, has 2)
    const trunc_int: [2]u8 = [0xfc, 0x01];
    let r3 = MySqlWireReader.init(trunc_int[0..]);
    let got_bad_int = false;
    * = r3.readLenEncInt() catch |e| {
        got_bad_int = (e == MySqlWireReaderError.InvalidEncodedInteger);
        @as(u64, 0)
    };
    expect_eq(got_bad_int, true);

    // 4. ShortRead when lenenc-str declares length 10 but only 2 bytes follow (pos stays 0)
    const trunc_str: [3]u8 = [0x0a, 'h', 'i'];
    let r4 = MySqlWireReader.init(trunc_str[0..]);
    let got_str_short = false;
    * = r4.readLenEncString() catch |e| {
        got_str_short = (e == MySqlWireReaderError.ShortRead);
        ""
    };
    expect_eq(got_str_short, true);
    expect_eq(r4.offset(), @as(usize, 0));

    // 5. InvalidOffset when seeking past buffer end
    let got_bad_off = false;
    * = r4.setOffsetFromStart(99) catch |e| {
        got_bad_off = (e == MySqlWireReaderError.InvalidOffset);
        @as(usize, 0)
    };
    expect_eq(got_bad_off, true);
});
