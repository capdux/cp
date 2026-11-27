// cap/test/testCapTable.cp
// Comprehensive unit tests for @cap/table:
// 1. Display width with ASCII, ANSI CSI/OSC escape sequences, UTF-8 & CJK wide characters
// 2. Truncation (`truncateDisplay`) & alignment padding (`padCell`) with ANSI/CJK safety
// 3. Unicode & Rounded box-drawing border presets with Left/Center/Right alignment
// 4. Ascii & Compact border presets
// 5. Markdown (GFM) border preset with alignment markers
// 6. Empty tables, jagged multi-column rows, and `setMaxColWidth` truncation
// 7. CJK + ANSI styled table alignment integrity

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    BorderStyle,
    Align,
    displayWidth,
    truncateDisplay,
    padCell,
    Table,
} from "../../packages/cli-table3/src/index.cp";

fn main() {
    test("1. @cap/table — displayWidth (ASCII, ANSI CSI, UTF-8 & CJK)", () => {
        expect_eq(displayWidth(""), 0 as usize);
        expect_eq(displayWidth("hello"), 5 as usize);
        // ANSI SGR color sequences contribute 0 display columns
        expect_eq(displayWidth("\x1b[31mERROR\x1b[0m"), 5 as usize);
        expect_eq(displayWidth("\x1b[1;32;40mOK\x1b[0m"), 2 as usize);
        // CJK ideographs count as 2 columns each
        expect_eq(displayWidth("中文"), 4 as usize);
        expect_eq(displayWidth("CP语言2026"), 10 as usize);
        // Mixed ANSI + CJK
        expect_eq(displayWidth("\x1b[36m北京\x1b[0m/BJ"), 7 as usize);
    });

    test("2. @cap/table — truncateDisplay & padCell (ANSI + CJK)", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        const t1 = truncateDisplay(alloc, "Hello World", 5) catch unreachable;
        defer alloc.free(t1);
        expect_eq(std.mem.eql(u8, t1, "Hell…"), true);
        expect_eq(displayWidth(t1), 5 as usize);

        // No-op when within max_width
        const t2 = truncateDisplay(alloc, "short", 10) catch unreachable;
        defer alloc.free(t2);
        expect_eq(std.mem.eql(u8, t2, "short"), true);

        // CJK truncation: "测试文本" (width 8) -> max 5 => target 4 => "测试…" (width 5)
        const t3 = truncateDisplay(alloc, "测试文本", 5) catch unreachable;
        defer alloc.free(t3);
        expect_eq(std.mem.eql(u8, t3, "测试…"), true);
        expect_eq(displayWidth(t3), 5 as usize);

        // ANSI-colored truncation appends reset sequence and preserves visual width
        const t4 = truncateDisplay(alloc, "\x1b[31mLongError\x1b[0m", 5) catch unreachable;
        defer alloc.free(t4);
        expect_eq(std.mem.eql(u8, t4, "\x1b[31mLong…\x1b[0m"), true);
        expect_eq(displayWidth(t4), 5 as usize);

        // padCell Left / Center / Right
        const p_left = padCell(alloc, "hi", 6, Align.Left) catch unreachable;
        defer alloc.free(p_left);
        expect_eq(std.mem.eql(u8, p_left, "hi    "), true);

        const p_center = padCell(alloc, "hi", 6, Align.Center) catch unreachable;
        defer alloc.free(p_center);
        expect_eq(std.mem.eql(u8, p_center, "  hi  "), true);

        const p_right = padCell(alloc, "中文", 6, Align.Right) catch unreachable;
        defer alloc.free(p_right);
        expect_eq(std.mem.eql(u8, p_right, "  中文"), true);
    });

    test("3. @cap/table — Unicode & Rounded Borders with Alignment", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let tbl = Table.init(alloc);
        defer tbl.deinit();

        const hdrs: [2]string = [ "Name", "Score" ];
        const r1: [2]string = [ "Alice", "95" ];
        const r2: [2]string = [ "Bob", "100" ];
        tbl.setHeaders(hdrs[0..]) catch unreachable;
        tbl.addRow(r1[0..]) catch unreachable;
        tbl.addRow(r2[0..]) catch unreachable;
        tbl.setAlign(0, Align.Left) catch unreachable;
        tbl.setAlign(1, Align.Right) catch unreachable;

        // Default style is BorderStyle.Unicode
        const out_uni = tbl.render(alloc) catch unreachable;
        defer alloc.free(out_uni);
        const expected_uni = "┌───────┬───────┐\n│ Name  │ Score │\n├───────┼───────┤\n│ Alice │    95 │\n│ Bob   │   100 │\n└───────┴───────┘";
        expect_eq(std.mem.eql(u8, out_uni, expected_uni), true);

        // Switch to BorderStyle.Rounded with Center alignment on column 0
        tbl.setStyle(BorderStyle.Rounded);
        tbl.setAlign(0, Align.Center) catch unreachable;
        const out_rnd = tbl.render(alloc) catch unreachable;
        defer alloc.free(out_rnd);
        const expected_rnd = "╭───────┬───────╮\n│ Name  │ Score │\n├───────┼───────┤\n│ Alice │    95 │\n│  Bob  │   100 │\n╰───────┴───────╯";
        expect_eq(std.mem.eql(u8, out_rnd, expected_rnd), true);
    });

    test("4. @cap/table — Ascii & Compact Border Presets", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let tbl = Table.init(alloc);
        defer tbl.deinit();

        const hdrs: [2]string = [ "Key", "Val" ];
        const r1: [2]string = [ "a", "1" ];
        tbl.setHeaders(hdrs[0..]) catch unreachable;
        tbl.addRow(r1[0..]) catch unreachable;
        tbl.setStyle(BorderStyle.Ascii);

        const out_ascii = tbl.render(alloc) catch unreachable;
        defer alloc.free(out_ascii);
        const expected_ascii = "+-----+-----+\n| Key | Val |\n+-----+-----+\n| a   | 1   |\n+-----+-----+";
        expect_eq(std.mem.eql(u8, out_ascii, expected_ascii), true);

        tbl.setStyle(BorderStyle.Compact);
        const out_compact = tbl.render(alloc) catch unreachable;
        defer alloc.free(out_compact);
        const expected_compact = "Key  Val\n───  ───\na    1  ";
        expect_eq(std.mem.eql(u8, out_compact, expected_compact), true);
    });

    test("5. @cap/table — Markdown (GFM) Preset with Column Alignment", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let tbl = Table.init(alloc);
        defer tbl.deinit();

        const hdrs: [3]string = [ "Lang", "Type", "Year" ];
        const r1: [3]string = [ "CP", "AOT", "2026" ];
        tbl.setHeaders(hdrs[0..]) catch unreachable;
        tbl.addRow(r1[0..]) catch unreachable;
        tbl.setStyle(BorderStyle.Markdown);
        tbl.setAlign(0, Align.Left) catch unreachable;
        tbl.setAlign(1, Align.Center) catch unreachable;
        tbl.setAlign(2, Align.Right) catch unreachable;

        const out_md = tbl.render(alloc) catch unreachable;
        defer alloc.free(out_md);
        const expected_md = "| Lang | Type | Year |\n| :--- | :--: | ---: |\n| CP   | AOT  | 2026 |";
        expect_eq(std.mem.eql(u8, out_md, expected_md), true);
    });

    test("6. @cap/table — Empty Table, Jagged Multi-Column Rows & MaxColWidth", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;

        // Empty table renders to ""
        let empty_tbl = Table.init(alloc);
        defer empty_tbl.deinit();
        const empty_out = empty_tbl.render(alloc) catch unreachable;
        defer alloc.free(empty_out);
        expect_eq(empty_out.len, 0 as usize);

        // Jagged rows (header has 2 cols, row 1 has 1 col, row 2 has 3 cols) + maxColWidth = 5
        let tbl = Table.init(alloc);
        defer tbl.deinit();
        const hdrs: [2]string = [ "ID", "Description" ];
        const r1: [1]string = [ "1" ];
        const r2: [3]string = [ "2", "VeryLongText", "ExtraCol" ];
        tbl.setHeaders(hdrs[0..]) catch unreachable;
        tbl.addRow(r1[0..]) catch unreachable;
        tbl.addRow(r2[0..]) catch unreachable;
        tbl.setStyle(BorderStyle.Ascii);
        tbl.setMaxColWidth(5);

        const out = tbl.render(alloc) catch unreachable;
        defer alloc.free(out);
        const expected = "+----+-------+-------+\n| ID | Desc… |       |\n+----+-------+-------+\n| 1  |       |       |\n| 2  | Very… | Extr… |\n+----+-------+-------+";
        expect_eq(std.mem.eql(u8, out, expected), true);
    });

    test("7. @cap/table — CJK + ANSI Styled Table Alignment Integrity", () => {
        const alloc: std.mem.Allocator = std.heap.page_allocator;
        let tbl = Table.init(alloc);
        defer tbl.deinit();

        const hdrs: [2]string = [ "城市", "状态" ];
        const r1: [2]string = [ "北京", "\x1b[32m正常\x1b[0m" ];
        const r2: [2]string = [ "NYC", "\x1b[31m告警\x1b[0m" ];
        tbl.setHeaders(hdrs[0..]) catch unreachable;
        tbl.addRow(r1[0..]) catch unreachable;
        tbl.addRow(r2[0..]) catch unreachable;
        tbl.setStyle(BorderStyle.Ascii);

        const out = tbl.render(alloc) catch unreachable;
        defer alloc.free(out);
        const expected = "+------+------+\n| 城市 | 状态 |\n+------+------+\n| 北京 | \x1b[32m正常\x1b[0m |\n| NYC  | \x1b[31m告警\x1b[0m |\n+------+------+";
        expect_eq(std.mem.eql(u8, out, expected), true);
    });
}
