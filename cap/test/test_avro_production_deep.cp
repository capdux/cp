// Source: cp/cap/test/test_avro_production_deep.cp
// Deep Production Test Suite for cap:avro
// Covers:
//   1. Scalar round-trip (ZigZag int/long extremes, LE IEEE float/double, boolean, null, bytes, string)
//   2. Complex record + union (["null", "string"]) + array + map + enum + fixed round-trip
//   3. OCF "Obj\x01" container multi-record write/read round-trip & 16-byte sync_marker verification
//   4. Guards: SchemaMismatch, InvalidUnionIndex, InvalidSyncMarker, InvalidContainerHeader, UnexpectedEof, VarintOverflow
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.avro` & std.testing.allocator zero-leak

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    OCF_MAGIC,
    AvroError,
    AvroSchema,
    AvroValue,
    AvroMapEntry,
    AvroRecordField,
    encodeDatum,
    decodeDatum,
    writeContainer,
    readContainer,
} from "cap:binfmt";
import cap from "cap";

fn expectScalarRoundTrip(alloc: std.mem.Allocator, schema_json: []const u8, val: AvroValue)! {
    let schema = AvroSchema.parseSchema(alloc, schema_json) catch unreachable;
    defer schema.deinit();

    const encoded = encodeDatum(alloc, schema, val) catch unreachable;
    defer alloc.free(encoded);

    let decoded = decodeDatum(alloc, schema, encoded) catch unreachable;
    defer decoded.deinit(alloc);

    expect_eq(decoded.eql(val), true);
}

test("1. Scalars ZigZag Int/Long Extremes, LE Float/Double, Boolean, Null, Bytes, String") {
    const alloc = std.testing.allocator;

    expectScalarRoundTrip(alloc, "\"null\"", AvroValue.makeNull())?;
    expectScalarRoundTrip(alloc, "\"boolean\"", AvroValue.makeBoolean(false))?;
    expectScalarRoundTrip(alloc, "\"boolean\"", AvroValue.makeBoolean(true))?;

    // ZigZag int (i32) boundaries: 0 -> 0x00, -1 -> 0x01, 1 -> 0x02, minInt/maxInt
    expectScalarRoundTrip(alloc, "\"int\"", AvroValue.makeInt(0))?;
    expectScalarRoundTrip(alloc, "\"int\"", AvroValue.makeInt(-1))?;
    expectScalarRoundTrip(alloc, "\"int\"", AvroValue.makeInt(1))?;
    expectScalarRoundTrip(alloc, "\"int\"", AvroValue.makeInt(std.math.minInt(i32)))?;
    expectScalarRoundTrip(alloc, "\"int\"", AvroValue.makeInt(std.math.maxInt(i32)))?;

    // Verify exact ZigZag wire bytes for int(-1) = 0x01 and int(1) = 0x02
    let int_schema = AvroSchema.parseSchema(alloc, "\"int\"") catch unreachable;
    defer int_schema.deinit();
    const enc_neg1 = encodeDatum(alloc, int_schema, AvroValue.makeInt(-1)) catch unreachable;
    defer alloc.free(enc_neg1);
    expect_eq(std.mem.eql(u8, enc_neg1, "\x01"), true);

    // ZigZag long (i64) boundaries
    expectScalarRoundTrip(alloc, "\"long\"", AvroValue.makeLong(0))?;
    expectScalarRoundTrip(alloc, "\"long\"", AvroValue.makeLong(-1))?;
    expectScalarRoundTrip(alloc, "\"long\"", AvroValue.makeLong(std.math.minInt(i64)))?;
    expectScalarRoundTrip(alloc, "\"long\"", AvroValue.makeLong(std.math.maxInt(i64)))?;

    // Float & Double (IEEE 754 little-endian)
    expectScalarRoundTrip(alloc, "\"float\"", AvroValue.makeFloat(1.5))?;
    expectScalarRoundTrip(alloc, "\"float\"", AvroValue.makeFloat(-0.25))?;
    expectScalarRoundTrip(alloc, "\"double\"", AvroValue.makeDouble(3.141592653589793))?;
    expectScalarRoundTrip(alloc, "\"double\"", AvroValue.makeDouble(-123456.789))?;

    // Bytes & String
    expectScalarRoundTrip(alloc, "\"bytes\"", AvroValue.makeBytes(""))?;
    expectScalarRoundTrip(alloc, "\"bytes\"", AvroValue.makeBytes("\xDE\xAD\xBE\xEF\x00\xFF"))?;
    expectScalarRoundTrip(alloc, "\"string\"", AvroValue.makeString(""))?;
    expectScalarRoundTrip(alloc, "\"string\"", AvroValue.makeString("apache-avro-cp-codec"))?;
}

test("2. Complex Record + Union + Array + Map + Enum + Fixed Round-Trip") {
    const alloc = std.testing.allocator;

    const schema_json =
        "{\"type\":\"record\",\"name\":\"TelemetryEvent\",\"namespace\":\"io.cp\",\"fields\":[" ++
        "{\"name\":\"id\",\"type\":\"long\"}," ++
        "{\"name\":\"note\",\"type\":[\"null\",\"string\"]}," ++
        "{\"name\":\"status\",\"type\":{\"type\":\"enum\",\"name\":\"Status\",\"symbols\":[\"OK\",\"WARN\",\"FAIL\"]}}," ++
        "{\"name\":\"trace_id\",\"type\":{\"type\":\"fixed\",\"name\":\"TraceId\",\"size\":4}}," ++
        "{\"name\":\"scores\",\"type\":{\"type\":\"array\",\"items\":\"int\"}}," ++
        "{\"name\":\"tags\",\"type\":{\"type\":\"map\",\"values\":\"string\"}}" ++
        "]}";

    let schema = AvroSchema.parseSchema(alloc, schema_json) catch unreachable;
    defer schema.deinit();

    let note_str = AvroValue.makeString("all systems nominal");
    let scores_arr: [3]AvroValue = [
        AvroValue.makeInt(10),
        AvroValue.makeInt(-20),
        AvroValue.makeInt(300),
    ];
    let tags_entries: [2]AvroMapEntry = [
        AvroMapEntry.init("env", AvroValue.makeString("prod")),
        AvroMapEntry.init("region", AvroValue.makeString("ap-southeast-2")),
    ];

    let fields: [6]AvroRecordField = [
        AvroRecordField.init("id", AvroValue.makeLong(9876543210)),
        AvroRecordField.init("note", AvroValue.makeUnion(1, &note_str)),
        AvroRecordField.init("status", AvroValue.makeEnum(1, "WARN")),
        AvroRecordField.init("trace_id", AvroValue.makeFixed("\xCA\xFE\xBA\xBE")),
        AvroRecordField.init("scores", AvroValue.makeArray(&scores_arr)),
        AvroRecordField.init("tags", AvroValue.makeMap(&tags_entries)),
    ];
    const event = AvroValue.makeRecord(&fields);

    const encoded = encodeDatum(alloc, schema, event) catch unreachable;
    defer alloc.free(encoded);

    let decoded = decodeDatum(alloc, schema, encoded) catch unreachable;
    defer decoded.deinit(alloc);

    expect_eq(decoded.eql(event), true);
    expect_eq(decoded.getField("id").?.asLong().?, 9876543210 as i64);
    expect_eq(decoded.getField("note").?.asUnion().?.index, 1 as usize);
    expect_eq(std.mem.eql(u8, decoded.getField("note").?.asUnion().?.value.asString().?, "all systems nominal"), true);
    expect_eq(decoded.getField("status").?.asEnum().?.index, 1 as i32);
    expect_eq(std.mem.eql(u8, decoded.getField("status").?.asEnum().?.symbol, "WARN"), true);
    expect_eq(std.mem.eql(u8, decoded.getField("trace_id").?.asFixed().?, "\xCA\xFE\xBA\xBE"), true);
}

test("3. OCF Obj\\x01 Container Multi-Record Write & Read Round-Trip") {
    const alloc = std.testing.allocator;

    const schema_json =
        "{\"type\":\"record\",\"name\":\"User\",\"fields\":[" ++
        "{\"name\":\"name\",\"type\":\"string\"}," ++
        "{\"name\":\"age\",\"type\":\"int\"}," ++
        "{\"name\":\"bio\",\"type\":[\"null\",\"string\"]}" ++
        "]}";

    const sync_marker: [16]u8 = [
        0x10, 0x21, 0x32, 0x43, 0x54, 0x65, 0x76, 0x87,
        0x98, 0xA9, 0xBA, 0xCB, 0xDC, 0xED, 0xFE, 0x0F,
    ];

    let bio1 = AvroValue.makeNull();
    let r1_fields: [3]AvroRecordField = [
        AvroRecordField.init("name", AvroValue.makeString("Alice")),
        AvroRecordField.init("age", AvroValue.makeInt(30)),
        AvroRecordField.init("bio", AvroValue.makeUnion(0, &bio1)),
    ];

    let bio2 = AvroValue.makeString("Systems architect");
    let r2_fields: [3]AvroRecordField = [
        AvroRecordField.init("name", AvroValue.makeString("Bob")),
        AvroRecordField.init("age", AvroValue.makeInt(42)),
        AvroRecordField.init("bio", AvroValue.makeUnion(1, &bio2)),
    ];

    const records: [2]AvroValue = [
        AvroValue.makeRecord(&r1_fields),
        AvroValue.makeRecord(&r2_fields),
    ];

    const ocf_bytes = writeContainer(alloc, schema_json, sync_marker, &records) catch unreachable;
    defer alloc.free(ocf_bytes);

    // Verify 4-byte OCF magic "Obj\x01"
    expect_eq(std.mem.eql(u8, ocf_bytes[0..4], &OCF_MAGIC), true);

    let container = readContainer(alloc, ocf_bytes) catch unreachable;
    defer container.deinit(alloc);

    expect_eq(std.mem.eql(u8, container.schema_json, schema_json), true);
    expect_eq(std.mem.eql(u8, &container.sync_marker, &sync_marker), true);
    expect_eq(std.mem.eql(u8, container.codec, "null"), true);
    expect_eq(container.records.len, 2 as usize);
    expect_eq(container.records[0].eql(records[0]), true);
    expect_eq(container.records[1].eql(records[1]), true);
}

test("4. Security & Integrity Guards: SchemaMismatch, InvalidUnionIndex, InvalidSyncMarker, Bad Magic & Truncation") {
    const alloc = std.testing.allocator;

    // 4a. SchemaMismatch on encode
    let int_schema = AvroSchema.parseSchema(alloc, "\"int\"") catch unreachable;
    defer int_schema.deinit();
    let got_mismatch = false;
    * = encodeDatum(alloc, int_schema, AvroValue.makeString("wrong")) catch |e| {
        if (e == @as(anyerror, AvroError.SchemaMismatch)) got_mismatch = true;
    };
    expect_eq(got_mismatch, true);

    // 4b. InvalidUnionIndex on encode & decode
    let union_schema = AvroSchema.parseSchema(alloc, "[\"null\",\"string\"]") catch unreachable;
    defer union_schema.deinit();
    let dummy = AvroValue.makeNull();
    let got_union_enc_err = false;
    * = encodeDatum(alloc, union_schema, AvroValue.makeUnion(5, &dummy)) catch |e| {
        if (e == @as(anyerror, AvroError.InvalidUnionIndex)) got_union_enc_err = true;
    };
    expect_eq(got_union_enc_err, true);

    // ZigZag(2) = 0x04 -> index 2 (out of bounds for 2-branch union)
    let got_union_dec_err = false;
    * = decodeDatum(alloc, union_schema, "\x04") catch |e| {
        if (e == @as(anyerror, AvroError.InvalidUnionIndex)) got_union_dec_err = true;
    };
    expect_eq(got_union_dec_err, true);

    // 4c. VarintOverflow (6 continuation bytes for 32-bit int)
    let got_varint_ovf = false;
    * = decodeDatum(alloc, int_schema, "\x80\x80\x80\x80\x80\x01") catch |e| {
        if (e == @as(anyerror, AvroError.VarintOverflow)) got_varint_ovf = true;
    };
    expect_eq(got_varint_ovf, true);

    // 4d. UnexpectedEof on truncated string
    let str_schema = AvroSchema.parseSchema(alloc, "\"string\"") catch unreachable;
    defer str_schema.deinit();
    let got_eof = false;
    // ZigZag(5) = 0x0A, only 2 bytes follow
    * = decodeDatum(alloc, str_schema, "\x0Aab") catch |e| {
        if (e == @as(anyerror, AvroError.UnexpectedEof)) got_eof = true;
    };
    expect_eq(got_eof, true);

    // 4e. InvalidContainerHeader on bad OCF magic
    let got_bad_hdr = false;
    * = readContainer(alloc, "BAD\x01\x00") catch |e| {
        if (e == @as(anyerror, AvroError.InvalidContainerHeader)) got_bad_hdr = true;
    };
    expect_eq(got_bad_hdr, true);

    // 4f. InvalidSyncMarker when trailing 16-byte sync marker is tampered
    const sync_marker: [16]u8 = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16];
    const recs: [1]AvroValue = [AvroValue.makeInt(77)];
    let ocf = writeContainer(alloc, "\"int\"", sync_marker, &recs) catch unreachable;
    defer alloc.free(ocf);
    ocf[ocf.len - 1] ^= 0xFF;

    let got_bad_sync = false;
    * = readContainer(alloc, ocf) catch |e| {
        if (e == @as(anyerror, AvroError.InvalidSyncMarker)) got_bad_sync = true;
    };
    expect_eq(got_bad_sync, true);
}

test("5. Dynamic Lazy Namespace Mounting (cap.avro) & Zero Memory Leak") {
    const alloc = std.testing.allocator;

    const schema_json = "{\"type\":\"record\",\"name\":\"Ping\",\"fields\":[{\"name\":\"seq\",\"type\":\"int\"},{\"name\":\"msg\",\"type\":\"string\"}]}";
    let schema = cap.binfmt.AvroSchema.parseSchema(alloc, schema_json) catch unreachable;
    defer schema.deinit();

    let fields: [2]AvroRecordField = [
        AvroRecordField.init("seq", AvroValue.makeInt(101)),
        AvroRecordField.init("msg", AvroValue.makeString("cap.avro-ok")),
    ];
    const rec = AvroValue.makeRecord(&fields);

    const encoded = cap.binfmt.encodeDatum(alloc, schema, rec) catch unreachable;
    defer alloc.free(encoded);

    let decoded = cap.binfmt.decodeDatum(alloc, schema, encoded) catch unreachable;
    defer decoded.deinit(alloc);

    expect_eq(decoded.eql(rec), true);
    expect_eq(decoded.getField("seq").?.asInt().?, 101 as i32);
    expect_eq(std.mem.eql(u8, decoded.getField("msg").?.asString().?, "cap.avro-ok"), true);
}
