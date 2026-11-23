import { test, expect_eq } from "cap:test";
import {
    END_OF_MESSAGE,
    SchemaError,
    SchemaReader,
    SchemaWriter,
    FixedSchemaWriter,
    CompactUint64,
} from "cap:binfmt";

enum PlatformOS: u8 {
    unknown = 0,
    linux = 1,
    macos = 2,
    windows = 3,
}

enum EventType: u16 {
    build_start = 100,
    build_ok = 200,
    build_fail = 500,
}

struct TelemetryRecord {
    os: PlatformOS = PlatformOS.unknown,
    event: EventType = EventType.build_start,
    session_id: u32 = 0,
    timestamp: CompactUint64 = CompactUint64{},
    label: string = "",
    active: bool = false,

    fn encode(w: *SchemaWriter): void ! SchemaError {
        w.writeFieldId(1)?;
        w.writeEnum(PlatformOS, _.os)?;
        w.writeFieldId(2)?;
        w.writeEnum(EventType, _.event)?;
        w.writeFieldId(3)?;
        w.writeU32(_.session_id)?;
        w.writeFieldId(4)?;
        _.timestamp.encode(w)?;
        if (_.label.len > 0) {
            w.writeFieldId(5)?;
            w.writeString(_.label)?;
        }
        w.writeFieldId(6)?;
        w.writeBool(_.active)?;
        return w.endMessage();
    }

    static fn decode(r: *SchemaReader): TelemetryRecord ! SchemaError {
        let rec = TelemetryRecord{};
        while (true) {
            const fid = r.readFieldId()?;
            if (fid == END_OF_MESSAGE) {
                break;
            }
            switch (fid) {
                1 => { rec.os = r.readEnum(PlatformOS)?; },
                2 => { rec.event = r.readEnum(EventType)?; },
                3 => { rec.session_id = r.readU32()?; },
                4 => { rec.timestamp = CompactUint64.decode(r)?; },
                5 => { rec.label = r.readString()?; },
                6 => { rec.active = r.readBool()?; },
                else => return err.InvalidFieldId,
            }
        }
        return rec;
    }
}

test("1. Primitive integers, booleans, byte arrays, strings & CompactUint64 round-trip (zero leak)", () => {
    const alloc = std.testing.allocator;
    let w = SchemaWriter.init(alloc);

    w.writeBool(true) catch unreachable;
    w.writeBool(false) catch unreachable;
    w.writeByte(0xAB) catch unreachable;
    w.writeI8(-42) catch unreachable;
    w.writeU16(65530) catch unreachable;
    w.writeI16(-12345) catch unreachable;
    w.writeU32(0xDEADBEEF) catch unreachable;
    w.writeI32(-987654321) catch unreachable;
    w.writeU64(0x0123456789ABCDEF) catch unreachable;
    w.writeI64(-1122334455667788) catch unreachable;
    w.writeString("cp-binary-schema") catch unreachable;
    w.writeString("") catch unreachable;

    const raw_bytes: [4]u8 = [0x01, 0x02, 0x03, 0x04];
    w.writeByteArray(raw_bytes[0..]) catch unreachable;

    const ts = CompactUint64.fromU64(0x1122334455667788);
    ts.encode(&w) catch unreachable;

    let r = SchemaReader.init(w.bytes());
    expect_eq(r.readBool() catch unreachable, true);
    expect_eq(r.readBool() catch unreachable, false);
    expect_eq(r.readByte() catch unreachable, u8(0xAB));
    expect_eq(r.readI8() catch unreachable, i8(-42));
    expect_eq(r.readU16() catch unreachable, u16(65530));
    expect_eq(r.readI16() catch unreachable, i16(-12345));
    expect_eq(r.readU32() catch unreachable, u32(0xDEADBEEF));
    expect_eq(r.readI32() catch unreachable, i32(-987654321));
    expect_eq(r.readU64() catch unreachable, u64(0x0123456789ABCDEF));
    expect_eq(r.readI64() catch unreachable, i64(-1122334455667788));
    expect_eq(std.mem.eql(u8, r.readString() catch unreachable, "cp-binary-schema"), true);
    expect_eq(std.mem.eql(u8, r.readString() catch unreachable, ""), true);
    expect_eq(std.mem.eql(u8, r.readByteArray() catch unreachable, raw_bytes[0..]), true);

    const dec_ts = CompactUint64.decode(&r) catch unreachable;
    expect_eq(dec_ts.toU64(), u64(0x1122334455667788));
    expect_eq(r.isEof(), true);
    expect_eq(r.remaining(), usize(0));
});

test("2. Enum encode/decode round-trip & invalid enum value rejection", () => {
    const alloc = std.testing.allocator;
    let w = SchemaWriter.initCapacity(alloc, 16) catch unreachable;

    w.writeEnum(PlatformOS, PlatformOS.linux) catch unreachable;
    w.writeEnum(EventType, EventType.build_fail) catch unreachable;
    // Inject invalid u8 enum value (99) and invalid u16 enum value (999)
    w.writeByte(99) catch unreachable;
    w.writeU16(999) catch unreachable;

    let r = SchemaReader.init(w.bytes());
    expect_eq((r.readEnum(PlatformOS) catch unreachable) == PlatformOS.linux, true);
    expect_eq((r.readEnum(EventType) catch unreachable) == EventType.build_fail, true);

    std.testing.expectError(SchemaError.InvalidEnumValue, r.readEnum(PlatformOS)) catch unreachable;
    std.testing.expectError(SchemaError.InvalidEnumValue, r.readEnum(EventType)) catch unreachable;
    expect_eq(r.isEof(), true);
});

test("3. Field-ID tagged compact struct record round-trip & unknown field ID guard", () => {
    const alloc = std.testing.allocator;
    let w = SchemaWriter.init(alloc);

    const orig = TelemetryRecord{
        os: PlatformOS.macos,
        event: EventType.build_ok,
        session_id: 778899,
        timestamp: CompactUint64.fromU64(1732190000123),
        label: "release-v1",
        active: true,
    };
    orig.encode(&w) catch unreachable;

    let r = SchemaReader.init(w.bytes());
    const decoded = TelemetryRecord.decode(&r) catch unreachable;
    expect_eq(decoded.os == PlatformOS.macos, true);
    expect_eq(decoded.event == EventType.build_ok, true);
    expect_eq(decoded.session_id, u32(778899));
    expect_eq(decoded.timestamp.toU64(), u64(1732190000123));
    expect_eq(std.mem.eql(u8, decoded.label, "release-v1"), true);
    expect_eq(decoded.active, true);
    expect_eq(r.isEof(), true);

    // Unknown field ID (99) must fail with InvalidFieldId
    w.reset();
    w.writeFieldId(99) catch unreachable;
    w.endMessage() catch unreachable;
    let r2 = SchemaReader.init(w.bytes());
    std.testing.expectError(SchemaError.InvalidFieldId, TelemetryRecord.decode(&r2)) catch unreachable;
});

test("4. Truncated & EOF boundary protection across all reader methods", () => {
    const empty: [0]u8 = [];
    let r0 = SchemaReader.init(empty[0..]);
    expect_eq(r0.isEof(), true);
    std.testing.expectError(SchemaError.UnexpectedEof, r0.readByte()) catch unreachable;
    std.testing.expectError(SchemaError.UnexpectedEof, r0.readU32()) catch unreachable;
    std.testing.expectError(SchemaError.UnexpectedEof, r0.readString()) catch unreachable;

    // Length prefix claims 100 bytes but only 3 bytes remain
    const bad_str: [7]u8 = [100, 0, 0, 0, 'a', 'b', 'c'];
    let r1 = SchemaReader.init(bad_str[0..]);
    std.testing.expectError(SchemaError.UnexpectedEof, r1.readString()) catch unreachable;

    // Truncated u64 (only 6 bytes)
    const short_u64: [6]u8 = [1, 2, 3, 4, 5, 6];
    let r2 = SchemaReader.init(short_u64[0..]);
    std.testing.expectError(SchemaError.UnexpectedEof, r2.readU64()) catch unreachable;
});

test("5. FixedSchemaWriter zero-allocation encoding & BufferTooSmall guard + SchemaWriter growth", () => {
    let stack_buf: [16]u8 = std.mem.zeroes([16]u8);
    let fw = FixedSchemaWriter.init(stack_buf[0..]);
    fw.writeFieldId(1) catch unreachable;
    fw.writeString("ok") catch unreachable; // 4 + 2 = 6 bytes
    fw.endMessage() catch unreachable;
    expect_eq(fw.written(), usize(8));

    // Attempting to write 20 bytes into remaining 8 bytes -> BufferTooSmall
    const too_long = "01234567890123456789";
    std.testing.expectError(SchemaError.BufferTooSmall, fw.writeString(too_long)) catch unreachable;

    // SchemaWriter dynamic growth across multiple doublings with std.testing.allocator (zero leak)
    const alloc = std.testing.allocator;
    let gw = SchemaWriter.initCapacity(alloc, 4) catch unreachable;
    let i: u32 = 0;
    while (i < 64) {
        gw.writeU32(i) catch unreachable;
        i += 1;
    }
    expect_eq(gw.written(), usize(256));
    let gr = SchemaReader.init(gw.bytes());
    let j: u32 = 0;
    while (j < 64) {
        expect_eq(gr.readU32() catch unreachable, j);
        j += 1;
    }
    expect_eq(gr.isEof(), true);
});
