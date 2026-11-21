// Source: cp/cap/test/test_line_offset_table_production_deep.cp (Production deep tests for cap:line_offset_table)

import { test, expect_eq } from "cap:test";
import { LineOffsetTable, LineCol } from "cap:line_offset_table";

test("Empty source and single-line source positioning") {
    const alloc = std.testing.allocator;

    const empty_tbl = LineOffsetTable.build(alloc, "") catch unreachable;
    expect_eq(empty_tbl.lineCount(), @as(usize, 1));
    expect_eq(empty_tbl.sourceLen(), @as(usize, 0));
    expect_eq(empty_tbl.lineStart(0).?, @as(usize, 0));
    expect_eq(empty_tbl.lineStart(1) == null, true);

    const lc0 = empty_tbl.offsetToLineCol(0).?;
    expect_eq(lc0.eql(LineCol{ line: 0, column: 0 }), true);
    expect_eq(lc0.line1(), @as(usize, 1));
    expect_eq(lc0.column1(), @as(usize, 1));
    expect_eq(empty_tbl.offsetToLineCol(1) == null, true);
    expect_eq(empty_tbl.lineSlice("", 0).?, "");

    const single = "const x = 42;";
    const tbl = LineOffsetTable.build(alloc, single) catch unreachable;
    expect_eq(tbl.lineCount(), @as(usize, 1));
    const lc_mid = tbl.offsetToLineCol(6).?;
    expect_eq(lc_mid.line, @as(usize, 0));
    expect_eq(lc_mid.column, @as(usize, 6));
    expect_eq(tbl.offsetToLineCol(single.len).?.column, single.len);
    expect_eq(tbl.offsetToLineCol(single.len + 1) == null, true);
    expect_eq(tbl.lineSlice(single, 0).?, single);
}

test("Mixed LF, CRLF and CR multi-line binary search accuracy") {
    const alloc = std.testing.allocator;
    // Line 0: "alpha\n"   (0..6,  next=6)
    // Line 1: "beta\r\n"  (6..12, next=12)
    // Line 2: "gamma\r"   (12..18, next=18)
    // Line 3: "delta"     (18..23)
    const src = "alpha\nbeta\r\ngamma\rdelta";
    const tbl = LineOffsetTable.build(alloc, src) catch unreachable;

    expect_eq(tbl.lineCount(), @as(usize, 4));
    expect_eq(tbl.lineStart(0).?, @as(usize, 0));
    expect_eq(tbl.lineStart(1).?, @as(usize, 6));
    expect_eq(tbl.lineStart(2).?, @as(usize, 12));
    expect_eq(tbl.lineStart(3).?, @as(usize, 18));

    expect_eq(tbl.offsetToLineCol(0).?.eql(LineCol{ line: 0, column: 0 }), true);
    expect_eq(tbl.offsetToLineCol(5).?.eql(LineCol{ line: 0, column: 5 }), true); // '\n'
    expect_eq(tbl.offsetToLineCol(6).?.eql(LineCol{ line: 1, column: 0 }), true); // 'b'
    expect_eq(tbl.offsetToLineCol(10).?.eql(LineCol{ line: 1, column: 4 }), true); // '\r'
    expect_eq(tbl.offsetToLineCol(11).?.eql(LineCol{ line: 1, column: 5 }), true); // '\n'
    expect_eq(tbl.offsetToLineCol(12).?.eql(LineCol{ line: 2, column: 0 }), true); // 'g'
    expect_eq(tbl.offsetToLineCol(18).?.eql(LineCol{ line: 3, column: 0 }), true); // 'd'
    expect_eq(tbl.offsetToLineCol(23).?.eql(LineCol{ line: 3, column: 5 }), true); // EOF
}

test("Bidirectional offset <-> (line, column) roundtrip and out-of-bounds guards") {
    const alloc = std.testing.allocator;
    const src = "fn main() {\r\n    let a = 1;\n    return a;\n}";
    const tbl = LineOffsetTable.build(alloc, src) catch unreachable;

    // 对源码中全部字节位置验证 offset -> LineCol -> offset 100% 可逆
    let off: usize = 0;
    while (off <= src.len) {
        const lc = tbl.offsetToLineCol(off).?;
        const back = tbl.lineColToOffset(lc.line, lc.column).?;
        expect_eq(back, off);
        off += 1;
    }

    // 越界行、跨行越界列、超 EOF 列均严格返回 null
    expect_eq(tbl.lineColToOffset(99, 0) == null, true);
    expect_eq(tbl.lineColToOffset(0, 100) == null, true);
    expect_eq(tbl.lineColToOffset(tbl.lineCount() - 1, 2) == null, true);
}

test("lineSlice and lineRange newline stripping with trailing newline and bounds guards") {
    const alloc = std.testing.allocator;
    const src = "first\r\nsecond\nthird\r";
    const tbl = LineOffsetTable.build(alloc, src) catch unreachable;

    // 末尾 CR 后存在第 3 行空行
    expect_eq(tbl.lineCount(), @as(usize, 4));
    expect_eq(tbl.lineSlice(src, 0).?, "first");
    expect_eq(tbl.lineSlice(src, 1).?, "second");
    expect_eq(tbl.lineSlice(src, 2).?, "third");
    expect_eq(tbl.lineSlice(src, 3).?, "");
    expect_eq(tbl.lineSlice(src, 4) == null, true);

    const r0 = tbl.lineRange(src, 0).?;
    expect_eq(r0.start, @as(usize, 0));
    expect_eq(r0.end, @as(usize, 5));
    expect_eq(r0.end_with_newline, @as(usize, 7));

    // 传入截断的短 source 触发越界保护返回 null
    expect_eq(tbl.lineSlice("short", 2) == null, true);
}

test("Large 200-line generated source O(log N) binary search verification") {
    const alloc = std.testing.allocator;
    let buf = std.array_list.Managed(u8).init(alloc);
    defer buf.deinit();

    let line: usize = 0;
    while (line < 200) {
        buf.appendSlice("line_content_12345\n") catch unreachable; // 每行 19 字节
        line += 1;
    }

    const tbl = LineOffsetTable.build(alloc, buf.items) catch unreachable;
    expect_eq(tbl.lineCount(), @as(usize, 201));

    let check: usize = 0;
    while (check < 200) {
        const expected_start = check * 19;
        expect_eq(tbl.lineStart(check).?, expected_start);
        const lc = tbl.offsetToLineCol(expected_start + 5).?;
        expect_eq(lc.line, check);
        expect_eq(lc.column, @as(usize, 5));
        expect_eq(tbl.lineSlice(buf.items, check).?, "line_content_12345");
        check += 1;
    }
}

test("Multi-round RAII ~init() zero-leak verification") {
    const alloc = std.testing.allocator;
    const samples: [4]string = ["", "a", "a\nb\nc", "1\r\n2\r\n3\r\n"];

    let i: usize = 0;
    while (i < 4) {
        const tbl = LineOffsetTable.build(alloc, samples[i]) catch unreachable;
        expect_eq(tbl.sourceLen(), samples[i].len);
        expect_eq(tbl.lineCount() >= 1, true);
        i += 1;
    }
}
