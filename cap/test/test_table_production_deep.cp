// Source: cp/cap/test/test_table_production_deep.cp
// Deep Production Test Suite for cap:table & cap.table
// Covers:
//   1. DisplayWidth: ASCII, CJK wide chars, ANSI CSI colors & UTF-8 safe truncate
//   2. Ascii, Box, Rounded, Borderless rendering, alignment & footer separator
//   3. Markdown GFM rendering with :---, :---:, ---: alignment markers
//   4. Max-width wrapping, sortByColumn, sortNumericByColumn & ColumnMismatch
//   5. Dynamic lazy namespace mounting via `import cap from "cap"; cap.table`

const std = @import("std");
import { test, expect_eq } from "cap:test";
import {
    TextAlign,
    TableError,
    DisplayWidth,
    TableStyle,
    ColumnSpec,
    Table,
} from "cap:text";
import cap from "cap";

fn contains(haystack: string, needle: string): bool {
    return std.mem.indexOf(u8, haystack, needle) != null;
}

fn testDisplayWidthAndTruncate()! {
    const alloc = std.heap.page_allocator;

    expect_eq(DisplayWidth.visibleWidth("hello"), 5 as usize);
    expect_eq(DisplayWidth.visibleWidth("你好CP"), 6 as usize);
    expect_eq(DisplayWidth.visibleWidth("\x1b[32mOK\x1b[0m"), 2 as usize);
    expect_eq(DisplayWidth.visibleWidth("\x1b[1;36m状态: 正常\x1b[0m"), 10 as usize);

    // Safe UTF-8 truncation without splitting multi-byte CJK characters
    const t1 = DisplayWidth.truncate(alloc, "你好世界", 6, "..")?;
    defer alloc.free(t1);
    expect_eq(std.mem.eql(u8, t1, "你好.."), true);

    const t2 = DisplayWidth.truncate(alloc, "你好世界", 5, "..")?;
    defer alloc.free(t2);
    expect_eq(std.mem.eql(u8, t2, "你.."), true);
    expect_eq(DisplayWidth.visibleWidth(t2) <= 5, true);

    // ANSI reset preservation on truncate
    const t3 = DisplayWidth.truncate(alloc, "\x1b[31mHelloWorld\x1b[0m", 8, "...")?;
    defer alloc.free(t3);
    expect_eq(std.mem.eql(u8, t3, "\x1b[31mHello...\x1b[0m"), true);
    expect_eq(DisplayWidth.visibleWidth(t3), 8 as usize);

    // Center padding
    const p = DisplayWidth.pad(alloc, "中", 4, .Center)?;
    defer alloc.free(p);
    expect_eq(std.mem.eql(u8, p, " 中 "), true);
}

fn testStylesAlignmentAndFooter()! {
    const alloc = std.heap.page_allocator;

    let tbl = Table.init(alloc);
    defer tbl.deinit();

    tbl.setColumns([
        ColumnSpec{ title: "Name", alignment: .Left },
        ColumnSpec{ title: "State", alignment: .Center },
        ColumnSpec{ title: "Cost", alignment: .Right },
    ])?;
    tbl.addRow(["alpha", "\x1b[32mOK\x1b[0m", "12"])?;
    tbl.addRow(["测试", "WARN", "105"])?;
    tbl.setFooter(["Total", "2", "117"])?;

    tbl.setStyle(.Ascii);
    const ascii_out = tbl.render()?;
    defer alloc.free(ascii_out);
    expect_eq(contains(ascii_out, "+-------+-------+------+"), true);
    expect_eq(contains(ascii_out, "| alpha |  \x1b[32mOK\x1b[0m   |   12 |"), true);
    expect_eq(contains(ascii_out, "| Total |   2   |  117 |"), true);

    tbl.setStyle(.Box);
    const box_out = tbl.render()?;
    defer alloc.free(box_out);
    expect_eq(contains(box_out, "┌───────┬───────┬──────┐"), true);
    expect_eq(contains(box_out, "├───────┼───────┼──────┤"), true);
    expect_eq(contains(box_out, "└───────┴───────┴──────┘"), true);

    tbl.setStyle(.Rounded);
    const round_out = tbl.render()?;
    defer alloc.free(round_out);
    expect_eq(contains(round_out, "╭───────┬───────┬──────╮"), true);
    expect_eq(contains(round_out, "╰───────┴───────┴──────╯"), true);

    tbl.setStyle(.Borderless);
    const borderless_out = tbl.render()?;
    defer alloc.free(borderless_out);
    expect_eq(contains(borderless_out, "│"), false);
    expect_eq(contains(borderless_out, "─────"), true);
}

fn testMarkdownGfmRendering()! {
    const alloc = std.heap.page_allocator;

    let tbl = Table.init(alloc);
    defer tbl.deinit();

    tbl.setStyle(.Markdown);
    tbl.addColumn("Key", .Left)?;
    tbl.addColumn("Mode", .Center)?;
    tbl.addColumn("Count", .Right)?;
    tbl.addRow(["cpu", "fast", "8"])?;

    const md_out = tbl.render()?;
    defer alloc.free(md_out);

    expect_eq(contains(md_out, "| Key  | Mode  | Count |"), true);
    expect_eq(contains(md_out, "| :--- | :---: | ----: |"), true);
    expect_eq(contains(md_out, "| cpu  | fast  |     8 |"), true);
}

fn testWrapSortAndMismatch()! {
    const alloc = std.heap.page_allocator;

    let tbl = Table.init(alloc);
    defer tbl.deinit();

    tbl.setStyle(.Ascii);
    tbl.setColumns([
        ColumnSpec{ title: "Task", alignment: .Left, max_width: 10 },
        ColumnSpec{ title: "Score", alignment: .Right },
    ])?;

    // ColumnMismatch on addRow and setFooter
    let row_mismatch = false;
    tbl.addRow(["only_one"]) catch |e| {
        if (e == @as(anyerror, err.ColumnMismatch)) {
            row_mismatch = true;
        }
    };
    expect_eq(row_mismatch, true);

    let footer_mismatch = false;
    tbl.setFooter(["a", "b", "c"]) catch |e| {
        if (e == @as(anyerror, err.ColumnMismatch)) {
            footer_mismatch = true;
        }
    };
    expect_eq(footer_mismatch, true);

    tbl.addRow(["zebra task", "100"])?;
    tbl.addRow(["alpha long message", "20"])?;
    tbl.addRow(["beta\nline2", "\x1b[32m3.5\x1b[0m"])?;

    // Numeric ascending sort: 3.5 -> 20 -> 100
    tbl.sortNumericByColumn(1, true);
    const num_sorted = tbl.render()?;
    defer alloc.free(num_sorted);
    const idx_beta = std.mem.indexOf(u8, num_sorted, "beta").?;
    const idx_alpha = std.mem.indexOf(u8, num_sorted, "alpha").?;
    const idx_zebra = std.mem.indexOf(u8, num_sorted, "zebra").?;
    expect_eq(idx_beta < idx_alpha && idx_alpha < idx_zebra, true);

    // Lexical descending sort: zebra -> beta -> alpha
    tbl.sortByColumn(0, false);
    const lex_sorted = tbl.render()?;
    defer alloc.free(lex_sorted);
    const l_zebra = std.mem.indexOf(u8, lex_sorted, "zebra").?;
    const l_beta = std.mem.indexOf(u8, lex_sorted, "beta").?;
    const l_alpha = std.mem.indexOf(u8, lex_sorted, "alpha").?;
    expect_eq(l_zebra < l_beta && l_beta < l_alpha, true);

    // Max-width wrapping occurred for "alpha long message" (width > 10)
    expect_eq(contains(lex_sorted, "| alpha long |"), true);
    expect_eq(contains(lex_sorted, "| message    |"), true);
}

fn testDynamicCapTableNamespace()! {
    const alloc = std.heap.page_allocator;

    expect_eq(cap.text.DisplayWidth.visibleWidth("测试"), 4 as usize);

    let t = cap.text.Table.init(alloc);
    defer t.deinit();

    t.setStyle(cap.text.TableStyle.Rounded);
    t.addColumn("Agent", cap.text.TextAlign.Left)?;
    t.addColumn("Width", cap.text.TextAlign.Right)?;
    t.addRow(["测试", "4"])?;

    const out = t.render()?;
    defer alloc.free(out);
    expect_eq(contains(out, "╭"), true);
    expect_eq(contains(out, "测试"), true);
}

pub fn main() {
    test("1. DisplayWidth: ASCII, CJK wide chars, ANSI colors & safe UTF-8 truncate", () => {
        testDisplayWidthAndTruncate() catch unreachable;
    });
    test("2. Ascii, Box, Rounded, Borderless rendering, alignment & footer", () => {
        testStylesAlignmentAndFooter() catch unreachable;
    });
    test("3. Markdown GFM rendering with :---, :---:, ---: alignment markers", () => {
        testMarkdownGfmRendering() catch unreachable;
    });
    test("4. Max-width wrapping, sortByColumn, sortNumericByColumn & ColumnMismatch", () => {
        testWrapSortAndMismatch() catch unreachable;
    });
    test("5. Dynamic lazy namespace mounting via `import cap from \"cap\"; cap.table`", () => {
        testDynamicCapTableNamespace() catch unreachable;
    });
}
