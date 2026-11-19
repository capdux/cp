// ============================================================================
// cap/test/test_sourcemap_patch_hash.cp — cap:sourcemap, cap:patch, cap:hash
// 第三阶段三大核心标准库端到端全量回归测试套件
// ============================================================================

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    VLQ,
    LineOffsetTable,
    DynamicLineOffsetTable,
    SourceMapping,
    ParsedSourceMap,
    InternalSourceMap,
    InternalSourceMapBuilder,
    zigzagEncode,
    zigzagDecode,
    writeVarint,
    readVarint,
    Chunk,
    ParsedSourceMapRecord,
    SourceMapGenerator,
    SourceMapConsumer,
} from "cap:sourcemap";
import {
    DiffOpKind,
    HunkRange,
    UnifiedHunkHeader,
    LineDiffStats,
    UnifiedDiff,
    DiffOp,
    LineDiffer,
} from "cap:patch";
import {
    Hash,
    WyhashStream,
    XxHash3Stream,
} from "cap:hash";

fn runGeneratorConsumerRoundtrip(): !void {
    const allocator = std.heap.page_allocator;
    let gen = SourceMapGenerator.init(allocator, "app.bundle.js");
    defer gen.deinit();

    const s0 = try gen.addSourceWithContent("src/core.cp", "pub fn compute(v: i32): i32 { return v * 2; }");
    const n0 = try gen.addName("compute");

    try gen.addMapping(0, 0, 0, 0, s0, -1);
    try gen.addMapping(0, 12, 0, 7, s0, n0);
    try gen.addMapping(1, 4, 0, 30, s0, -1);

    const json_str = try gen.generateJSON(allocator, true);
    defer allocator.free(json_str);

    let consumer = try SourceMapConsumer.init(allocator, json_str);
    defer consumer.deinit();

    const pos_opt = consumer.originalPositionFor(0, 15);
    expect_eq(pos_opt != null, true);
    if (pos := pos_opt) {
        expect_eq(std.mem.eql(u8, pos.source, "src/core.cp"), true);
        expect_eq(pos.line, 0);
        expect_eq(pos.column, 7);
        expect_eq(std.mem.eql(u8, pos.name, "compute"), true);
    }

    // InternalSourceMap binary checkpoint search
    let builder = InternalSourceMapBuilder.init();
    defer builder.deinit(allocator);
    try builder.addMapping(allocator, SourceMapping{
        generated_line: 0,
        generated_column: 0,
        source_index: 0,
        original_line: 0,
        original_column: 0,
        name_index: -1,
    });
    try builder.addMapping(allocator, SourceMapping{
        generated_line: 2,
        generated_column: 8,
        source_index: 0,
        original_line: 10,
        original_column: 4,
        name_index: -1,
    });
    let ism = try builder.build(allocator);
    defer ism.deinit(allocator);

    const found_ism = ism.find(2, 12);
    expect_eq(found_ism != null, true);
    expect_eq(found_ism.?.original_line, 10);
    expect_eq(found_ism.?.original_column, 4);
}

fn main() {
    test("1. cap:sourcemap — Base64 VLQ, ZigZag/Varint, LineOffsetTable & ParsedSourceMap<CAP>", () => {
        // VLQ single & batch encode/decode
        let v_neg = VLQ.encode(-2048);
        const d_neg = VLQ.decode(v_neg.slice(), 0);
        expect_eq(d_neg.valid, true);
        expect_eq(d_neg.value, -2048);

        let batch: [4]i32 = [0, 1, -1, 16];
        let vlq_buf: [32]u8 = undefined;
        const vlq_len = VLQ.encodeValues(&batch, &vlq_buf);
        expect_eq(vlq_buf[0..vlq_len] == "ACDgB", true);

        // ZigZag & Varint
        expect_eq(zigzagDecode(zigzagEncode(-4096)), -4096);
        let vbuf: [16]u8 = undefined;
        const vn = writeVarint(&vbuf, 0, -65535);
        let vpos: usize = 0;
        expect_eq(readVarint(&vbuf, &vpos), -65535);
        expect_eq(vpos, vn);

        // LineOffsetTable O(log N) binary lookup
        const src = "line0\nline1_longer\nline2_end\n";
        const ltable = LineOffsetTable<16>.build(src);
        expect_eq(ltable.line_count, 4 as usize);
        const loc1 = ltable.resolveLocation(8); // inside "line1_longer" (starts at 6)
        expect_eq(loc1.line, 1 as u32);
        expect_eq(loc1.column, 2 as u32);
        expect_eq(ltable.resolveByteOffset(1, 2), 8 as u32);

        // Zero-heap ParsedSourceMap<CAP>
        const sm = ParsedSourceMap<16>.parseMappings("AAAA,IAAIA;ECAA");
        expect_eq(sm.count, 3 as usize);
        const m = sm.findMapping(1, 4);
        expect_eq(m != null, true);
        expect_eq(m.?.generated_line, 1);
        expect_eq(m.?.generated_column, 2);
        expect_eq(m.?.source_index, 1);
    });

    test("2. cap:sourcemap — SourceMapGenerator, SourceMapConsumer & InternalSourceMap binary roundtrip", () => {
        try runGeneratorConsumerRoundtrip();
    });

    test("3. cap:patch — UnifiedHunkHeader, UnifiedDiff analysis/application & LineDiffer Myers diff", () => {
        const hdr_opt = UnifiedHunkHeader.parse("@@ -10,5 +12,7 @@ fn update()");
        expect_eq(hdr_opt != null, true);
        if (hdr := hdr_opt) {
            expect_eq(hdr.old_range.start_line, 10 as usize);
            expect_eq(hdr.old_range.line_count, 5 as usize);
            expect_eq(hdr.new_range.start_line, 12 as usize);
            expect_eq(hdr.new_range.line_count, 7 as usize);
        }

        const patch_doc =
            "--- a/app.cp\n" ++
            "+++ b/app.cp\n" ++
            "@@ -1,3 +1,4 @@\n" ++
            " const a = 1;\n" ++
            "-const b = 2;\n" ++
            "+const b = 20;\n" ++
            "+const c = 30;\n" ++
            " return a + b;\n";

        const stats = UnifiedDiff.analyzePatch(patch_doc);
        expect_eq(stats.hunk_count, 1 as usize);
        expect_eq(stats.equal_lines, 2 as usize);
        expect_eq(stats.deleted_lines, 1 as usize);
        expect_eq(stats.inserted_lines, 2 as usize);

        let rendered_buf: [256]u8 = undefined;
        const new_side = UnifiedDiff.renderHunkNewSide(patch_doc, rendered_buf[0..256]);
        expect_eq(new_side == "const a = 1;\nconst b = 20;\nconst c = 30;\nreturn a + b;\n", true);

        // LineDiffer prefix/suffix trimming, DiffOp generation, editDistance & unified formatting
        const old_text = "const a = 1;\nconst b = 2;\nreturn a + b;\n";
        const new_text = "const a = 1;\nconst b = 20;\nconst c = 30;\nreturn a + b;\n";
        let ops: [16]DiffOp = undefined;
        const op_count = LineDiffer.diffLines(old_text, new_text, ops[0..16]);
        expect_eq(op_count, 5 as usize);
        expect_eq(LineDiffer.editDistance(ops[0..op_count]), 3 as usize); // 1 Delete + 2 Inserts

        let fmt_buf: [256]u8 = undefined;
        const fmt_diff = LineDiffer.formatUnifiedToBuf(ops[0..op_count], fmt_buf[0..256]);
        const roundtrip = UnifiedDiff.renderHunkNewSide(fmt_diff, rendered_buf[0..256]);
        expect_eq(roundtrip == new_text, true);
    });

    test("4. cap:hash — Wyhash, XxHash3/128, Murmur3, CityHash64, FNV-1a, djb2, SipHash, CRC32 & Adler32", () => {
        const msg = "CP High-Speed Non-Cryptographic Hash Matrix Verification";

        // 1. Wyhash one-shot & streaming consistency
        const wy1 = Hash.wyhash(msg, 42);
        expect_eq(wy1 != 0, true);
        let wy_stream = WyhashStream.init(42);
        wy_stream.update("CP High-Speed Non-Cryptographic ");
        wy_stream.update("Hash Matrix Verification");
        expect_eq(wy_stream.digest(), wy1);
        expect_eq(WyhashStream.hash(42, msg), wy1);

        // 2. XxHash3 & XxHash128 one-shot & streaming consistency
        const xx1 = Hash.xxhash3(msg, 0);
        expect_eq(xx1 != 0, true);
        let xx_stream = XxHash3Stream.init(0);
        xx_stream.update("CP High-Speed ");
        xx_stream.update("Non-Cryptographic Hash Matrix Verification");
        expect_eq(xx_stream.digest(), xx1);
        expect_eq(Hash.xxhash128(msg, 0) != 0, true);

        // 3. MurmurHash3, CityHash64, FNV-1a (64 & 32), djb2, hashInt
        expect_eq(Hash.murmur3(msg, 0) != 0, true);
        expect_eq(Hash.cityhash64(msg) != 0, true);
        expect_eq(Hash.fnv1a(msg) != 0, true);
        expect_eq(Hash.fnv1a32(msg) != 0, true);
        expect_eq(Hash.djb2("hello"), 261238937 as u32);
        expect_eq(Hash.hashInt(12345) != 12345, true);

        // 4. SipHash-2-4 (64-bit & 128-bit)
        let key: [16]u8 = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15];
        expect_eq(Hash.siphash(msg, key) != 0, true);
        expect_eq(Hash.siphash128(msg, key) != 0, true);

        // 5. IEEE 802.3 CRC32 & RFC 1950 Adler32 standard vectors
        expect_eq(Hash.crc32("123456789"), 0xCBF43926 as u32);
        expect_eq(Hash.adler32("example"), 0x0BC002ED as u32);
    });
}
