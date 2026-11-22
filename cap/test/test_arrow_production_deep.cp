// Source: cp/cap/test/test_arrow_production_deep.cp (Production deep tests for cap:arrow)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    ARROW_MAGIC,
    ARROW_FILE_HEADER_SIZE,
    ARROW_FILE_TRAILER_SIZE,
    ARROW_FILE_MIN_SIZE,
    ARROW_CONTINUATION,
    ArrowMetadataVersion,
    ArrowHeaderType,
    ArrowMessageMeta,
    ArrowMessageHeader,
    ArrowFooterInfo,
    ArrowMessageIter,
    readFbU32,
    readFbRootTable,
    readFbVtableOffset,
    parseArrowMessageMeta,
    isArrowFile,
    isArrowStream,
    parseArrowFileFooter,
} from "cap:binfmt";

fn writeU16LE(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeU32LE(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeU64LE(buf: []u8, off: usize, v: u64) {
    writeU32LE(buf, off, (v & 0xFFFF_FFFF) as u32);
    writeU32LE(buf, off + 4, ((v >> 32) & 0xFFFF_FFFF) as u32);
}

// Build a valid 32-byte FlatBuffers Arrow Message table:
//   [0..4]   root_offset = 16 (table starts at 16)
//   [4..16]  vtable (12 bytes):
//              vtable_size=12, table_size=16,
//              slot0(version)=4, slot1(header_type)=6, slot2(header)=0, slot3(bodyLength)=8
//   [16..32] table (16 bytes):
//              soffset32 = 12 (16 - 12 = 4 -> vtable at 4),
//              +4: version (i16),
//              +6: header_type (u8),
//              +8: bodyLength (i64)
fn encodeFbMessage(out: []u8, version: u16, header_type: u8, body_length: u64) {
    writeU32LE(out, 0, 16); // root table at offset 16
    writeU16LE(out, 4, 12); // vtable size = 12
    writeU16LE(out, 6, 16); // table size = 16
    writeU16LE(out, 8, 4);  // voffset 4 -> version at table+4
    writeU16LE(out, 10, 6); // voffset 6 -> header_type at table+6
    writeU16LE(out, 12, 0); // voffset 8 -> header absent
    writeU16LE(out, 14, 8); // voffset 10 -> bodyLength at table+8

    writeU32LE(out, 16, 12); // soffset32 = 12 (16 - 12 = 4)
    writeU16LE(out, 20, version);
    out[22] = header_type;
    out[23] = 0;
    writeU64LE(out, 24, body_length);
}

fn writeMagic(buf: []u8, off: usize) {
    buf[off] = 'A';
    buf[off + 1] = 'R';
    buf[off + 2] = 'R';
    buf[off + 3] = 'O';
    buf[off + 4] = 'W';
    buf[off + 5] = '1';
}

test("1. Arrow File magic validation and parseArrowFileFooter extraction", () => {
    // Layout: [0..8] ARROW1+pad | [8..48] 1 encapsulated Schema msg (8+32=40B) | [48..64] 16B footer | [64..68] footer_len=16 | [68..74] ARROW1
    let file: [74]u8 = std.mem.zeroes([74]u8);
    writeMagic(file[0..], 0);
    writeU32LE(file[0..], 8, ARROW_CONTINUATION);
    writeU32LE(file[0..], 12, 32);
    encodeFbMessage(file[16..48], 4, 1, 0); // V5, Schema, body=0

    // Footer bytes (16 bytes)
    file[48] = 'F'; file[49] = 'O'; file[50] = 'O'; file[51] = 'T';
    writeU32LE(file[0..], 64, 16);
    writeMagic(file[0..], 68);

    expect_eq(isArrowFile(file[0..]), true);
    expect_eq(isArrowStream(file[0..]), false);

    const footer = parseArrowFileFooter(file[0..]) ?? unreachable;
    expect_eq(footer.footer_len, 16);
    expect_eq(footer.footer_offset, 48);
    expect_eq(footer.footer_flatbuf.len, 16);
    expect_eq(std.mem.eql(u8, footer.footer_flatbuf[0..4], "FOOT"), true);
    expect_eq(footer.stream_start, 8);
    expect_eq(footer.stream_end, 48);
    expect_eq(footer.stream_slice.len, 40);

    let it = ArrowMessageIter.init(footer.stream_slice);
    const msg = it.next() ?? unreachable;
    expect_eq(msg.is_eos, false);
    expect_eq(msg.metadata_len, 32);
    const msg_meta = msg.parseMeta() ?? unreachable;
    expect_eq(msg_meta.version == ArrowMetadataVersion.v5, true);
    expect_eq(msg_meta.header_type == ArrowHeaderType.schema, true);
    expect_eq(it.next() == null, true);
});

test("2. Arrow IPC Stream (0.15+) multi-message iteration with body alignment and EOS", () => {
    // Msg 0: Schema (8B hdr + 32B meta, body=0) -> 40B [0..40]
    // Msg 1: RecordBatch (8B hdr + 32B meta, body=5B + 3B pad to 8B) -> 48B [40..88]
    // Msg 2: EOS (0xFFFFFFFF + 0) -> 8B [88..96]
    let stream: [96]u8 = std.mem.zeroes([96]u8);

    writeU32LE(stream[0..], 0, ARROW_CONTINUATION);
    writeU32LE(stream[0..], 4, 32);
    encodeFbMessage(stream[8..40], 4, 1, 0); // V5, Schema

    writeU32LE(stream[0..], 40, ARROW_CONTINUATION);
    writeU32LE(stream[0..], 44, 32);
    encodeFbMessage(stream[48..80], 4, 3, 5); // V5, RecordBatch, bodyLength=5
    stream[80] = 'H'; stream[81] = 'E'; stream[82] = 'L'; stream[83] = 'L'; stream[84] = 'O';

    writeU32LE(stream[0..], 88, ARROW_CONTINUATION);
    writeU32LE(stream[0..], 92, 0); // EOS

    expect_eq(isArrowStream(stream[0..]), true);

    let it = ArrowMessageIter.init(stream[0..]);

    const m0 = it.next() ?? unreachable;
    expect_eq(m0.is_eos, false);
    expect_eq(m0.header_bytes, 40);
    expect_eq((m0.parseMeta() ?? unreachable).header_type == ArrowHeaderType.schema, true);

    const m1 = it.next() ?? unreachable;
    expect_eq(m1.is_eos, false);
    expect_eq(m1.body_length, 5);
    expect_eq(std.mem.eql(u8, m1.body, "HELLO"), true);
    expect_eq((m1.parseMeta() ?? unreachable).header_type == ArrowHeaderType.record_batch, true);

    const m2 = it.next() ?? unreachable;
    expect_eq(m2.is_eos, true);
    expect_eq(m2.metadata_len, 0);
    expect_eq(m2.header_bytes, 8);

    expect_eq(it.next() == null, true);
});

test("3. FlatBuffers helpers and ArrowMessageMeta decode V4/V5 and all MessageHeader types", () => {
    let fb: [32]u8 = std.mem.zeroes([32]u8);
    encodeFbMessage(fb[0..], 3, 2, 1024); // V4, DictionaryBatch, body=1024

    expect_eq(readFbU32(fb[0..], 0) ?? 0, 16);
    const root = readFbRootTable(fb[0..]) ?? unreachable;
    expect_eq(root, 16);
    expect_eq(readFbVtableOffset(fb[0..], root, 4) ?? 0, 20);
    expect_eq(readFbVtableOffset(fb[0..], root, 6) ?? 0, 22);
    expect_eq(readFbVtableOffset(fb[0..], root, 8) == null, true); // slot 2 absent
    expect_eq(readFbVtableOffset(fb[0..], root, 10) ?? 0, 24);

    const meta1 = parseArrowMessageMeta(fb[0..]) ?? unreachable;
    expect_eq(meta1.version == ArrowMetadataVersion.v4, true);
    expect_eq(meta1.header_type == ArrowHeaderType.dictionary_batch, true);
    expect_eq(meta1.body_length, 1024);

    encodeFbMessage(fb[0..], 4, 4, 64); // V5, Tensor
    const meta2 = parseArrowMessageMeta(fb[0..]) ?? unreachable;
    expect_eq(meta2.version == ArrowMetadataVersion.v5, true);
    expect_eq(meta2.header_type == ArrowHeaderType.tensor, true);

    encodeFbMessage(fb[0..], 4, 5, 128); // V5, SparseTensor
    const meta3 = parseArrowMessageMeta(fb[0..]) ?? unreachable;
    expect_eq(meta3.header_type == ArrowHeaderType.sparse_tensor, true);
});

test("4. Legacy pre-0.15 Arrow IPC Stream without continuation marker and manual skipBody", () => {
    // Legacy format: [0..4] meta_len=32 | [4..36] Schema | [36..40] EOS (0)
    let legacy: [40]u8 = std.mem.zeroes([40]u8);
    writeU32LE(legacy[0..], 0, 32);
    encodeFbMessage(legacy[4..36], 3, 1, 0); // V4, Schema
    writeU32LE(legacy[0..], 36, 0); // legacy EOS

    expect_eq(isArrowStream(legacy[0..]), true);

    let it = ArrowMessageIter.init(legacy[0..]);
    const m0 = it.next() ?? unreachable;
    expect_eq(m0.is_eos, false);
    expect_eq(m0.header_bytes, 36);
    expect_eq((m0.parseMeta() ?? unreachable).header_type == ArrowHeaderType.schema, true);

    const eos = it.next() ?? unreachable;
    expect_eq(eos.is_eos, true);
    expect_eq(eos.header_bytes, 4);
    expect_eq(it.next() == null, true);

    // Test explicit skipBody with 8-byte alignment
    let raw_stream: [32]u8 = std.mem.zeroes([32]u8);
    writeU32LE(raw_stream[0..], 0, ARROW_CONTINUATION);
    writeU32LE(raw_stream[0..], 4, 8); // non-FB raw 8B metadata
    // 8B body at 16..24, then EOS at 24..32
    writeU32LE(raw_stream[0..], 24, ARROW_CONTINUATION);
    writeU32LE(raw_stream[0..], 28, 0);

    let it2 = ArrowMessageIter.init(raw_stream[0..]);
    const raw_msg = it2.next() ?? unreachable;
    expect_eq(raw_msg.metadata_len, 8);
    expect_eq(it2.skipBody(5), true); // aligns 16+5=21 -> 24
    const eos2 = it2.next() ?? unreachable;
    expect_eq(eos2.is_eos, true);
});

test("5. Malformed magic, out-of-bounds footer_len/metadata_len, and invalid FlatBuffers guard", () => {
    let bad: [24]u8 = std.mem.zeroes([24]u8);
    expect_eq(isArrowFile(bad[0..17]), false);
    expect_eq(parseArrowFileFooter(bad[0..]) == null, true);

    // Valid magic but footer_len exceeds available space (24 - 18 = 6 max, claim 20)
    writeMagic(bad[0..], 0);
    writeU32LE(bad[0..], 14, 20);
    writeMagic(bad[0..], 18);
    expect_eq(isArrowFile(bad[0..]), true);
    expect_eq(parseArrowFileFooter(bad[0..]) == null, true);

    // Stream with metadata_len exceeding buffer
    let oob_stream: [16]u8 = std.mem.zeroes([16]u8);
    writeU32LE(oob_stream[0..], 0, ARROW_CONTINUATION);
    writeU32LE(oob_stream[0..], 4, 9999);
    expect_eq(isArrowStream(oob_stream[0..]), false);
    let it = ArrowMessageIter.init(oob_stream[0..]);
    expect_eq(it.next() == null, true);

    // FlatBuffers with out-of-bounds root offset and negative bodyLength
    let bad_fb: [32]u8 = std.mem.zeroes([32]u8);
    writeU32LE(bad_fb[0..], 0, 500);
    expect_eq(readFbRootTable(bad_fb[0..]) == null, true);
    expect_eq(parseArrowMessageMeta(bad_fb[0..]) == null, true);

    encodeFbMessage(bad_fb[0..], 4, 3, 0xFFFF_FFFF_FFFF_FFFF); // negative i64 (-1)
    expect_eq(parseArrowMessageMeta(bad_fb[0..]) == null, true);
});
