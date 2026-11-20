// Source: cp/cap/test/test_tui_production_deep.cp
// Deep Production Verification Suite for cap:tui & cap.tui

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    KeyCode,
    KeyModifiers,
    KeyEvent,
    InputParser,
    TuiError,
    Rect,
    Constraint,
    Layout,
    BorderStyle,
    CellStyle,
    Cell,
    TuiBuffer,
    TuiDiff,
} from "cap:tui";

fn strEql(a: string, b: string): bool {
    return std.mem.eql(u8, a, b);
}

fn testInputParserSequences(): !void {
    // 1. Empty input returns null
    expect_eq(InputParser.parseNext("") == null, true);

    // 2. Plain ASCII and UTF-8 multi-byte character
    const ev_a = InputParser.parseNext("abc").?;
    expect_eq(ev_a.code.eql(KeyCode.Char('a')), true);
    expect_eq(ev_a.mods.eql(KeyModifiers.none()), true);
    expect_eq(ev_a.consumed_bytes, 1);

    const ev_utf8 = InputParser.parseNext("中x").?;
    expect_eq(ev_utf8.code.eql(KeyCode.Char(0x4E2D)), true);
    expect_eq(ev_utf8.consumed_bytes, 3);

    const ev_emoji = InputParser.parseNext("🚀").?;
    expect_eq(ev_emoji.code.eql(KeyCode.Char(0x1F680)), true);
    expect_eq(ev_emoji.consumed_bytes, 4);

    // 3. Ctrl+A..Z (0x01..0x1A), Enter, Tab, Backspace, Esc
    const ctrl_c = InputParser.parseNext("\x03").?;
    expect_eq(ctrl_c.code.eql(KeyCode.Char('c')), true);
    expect_eq(ctrl_c.mods.ctrl, true);
    expect_eq(ctrl_c.mods.alt, false);
    expect_eq(ctrl_c.consumed_bytes, 1);

    const ctrl_z = InputParser.parseNext("\x1a").?;
    expect_eq(ctrl_z.code.eql(KeyCode.Char('z')), true);
    expect_eq(ctrl_z.mods.ctrl, true);

    expect_eq(InputParser.parseNext("\r").?.code.eql(KeyCode.Enter), true);
    expect_eq(InputParser.parseNext("\t").?.code.eql(KeyCode.Tab), true);
    expect_eq(InputParser.parseNext("\x7f").?.code.eql(KeyCode.Backspace), true);

    const esc_only = InputParser.parseNext("\x1b").?;
    expect_eq(esc_only.code.eql(KeyCode.Esc), true);
    expect_eq(esc_only.consumed_bytes, 1);

    // 4. Alt+char (\x1b<c>)
    const alt_x = InputParser.parseNext("\x1bx").?;
    expect_eq(alt_x.code.eql(KeyCode.Char('x')), true);
    expect_eq(alt_x.mods.alt, true);
    expect_eq(alt_x.mods.ctrl, false);
    expect_eq(alt_x.consumed_bytes, 2);

    // 5. CSI arrow keys, BackTab, Delete/PageUp/PageDown, Home/End, and modifiers
    const up = InputParser.parseNext("\x1b[A").?;
    expect_eq(up.code.eql(KeyCode.Up), true);
    expect_eq(up.consumed_bytes, 3);

    expect_eq(InputParser.parseNext("\x1b[B").?.code.eql(KeyCode.Down), true);
    expect_eq(InputParser.parseNext("\x1b[C").?.code.eql(KeyCode.Right), true);
    expect_eq(InputParser.parseNext("\x1b[D").?.code.eql(KeyCode.Left), true);
    expect_eq(InputParser.parseNext("\x1b[H").?.code.eql(KeyCode.Home), true);
    expect_eq(InputParser.parseNext("\x1b[F").?.code.eql(KeyCode.End), true);

    const backtab = InputParser.parseNext("\x1b[Z").?;
    expect_eq(backtab.code.eql(KeyCode.BackTab), true);
    expect_eq(backtab.mods.shift, true);

    expect_eq(InputParser.parseNext("\x1b[3~").?.code.eql(KeyCode.Delete), true);
    expect_eq(InputParser.parseNext("\x1b[5~").?.code.eql(KeyCode.PageUp), true);
    expect_eq(InputParser.parseNext("\x1b[6~").?.code.eql(KeyCode.PageDown), true);

    const ctrl_right = InputParser.parseNext("\x1b[1;5C").?;
    expect_eq(ctrl_right.code.eql(KeyCode.Right), true);
    expect_eq(ctrl_right.mods.ctrl, true);
    expect_eq(ctrl_right.mods.alt, false);
    expect_eq(ctrl_right.mods.shift, false);
    expect_eq(ctrl_right.consumed_bytes, 6);

    const alt_shift_up = InputParser.parseNext("\x1b[1;4A").?;
    expect_eq(alt_shift_up.code.eql(KeyCode.Up), true);
    expect_eq(alt_shift_up.mods.shift, true);
    expect_eq(alt_shift_up.mods.alt, true);
    expect_eq(alt_shift_up.mods.ctrl, false);

    // 6. SS3 F1-F4 sequences
    expect_eq(InputParser.parseNext("\x1bOP").?.code.eql(KeyCode.F(1)), true);
    expect_eq(InputParser.parseNext("\x1bOQ").?.code.eql(KeyCode.F(2)), true);
    expect_eq(InputParser.parseNext("\x1bOR").?.code.eql(KeyCode.F(3)), true);
    expect_eq(InputParser.parseNext("\x1bOS").?.code.eql(KeyCode.F(4)), true);
}

fn testLayoutSolverAndRect(): !void {
    const alloc = std.heap.page_allocator;

    // 1. Rect geometry helpers
    const r = Rect.init(10, 20, 30, 10);
    expect_eq(r.area(), 300);
    expect_eq(r.isEmpty(), false);
    expect_eq(r.contains(10, 20), true);
    expect_eq(r.contains(39, 29), true);
    expect_eq(r.contains(40, 29), false);
    expect_eq(r.contains(39, 30), false);

    const inset = r.inner(2, 1);
    expect_eq(inset.eql(Rect.init(12, 21, 26, 8)), true);

    const collapsed = r.inner(20, 10);
    expect_eq(collapsed.isEmpty(), true);
    expect_eq(collapsed.w, 0);
    expect_eq(collapsed.h, 0);

    // 2. Vertical split conservation with Fixed + Percent + Fill remainder sharing
    // Total h=25: Fixed(3)=3, Percent(20)=5, remaining=17 across two Fill(1) -> 9 and 8
    const v_area = Rect.init(2, 4, 80, 25);
    const v_constraints: [4]Constraint = [
        Constraint.Fixed(3),
        Constraint.Percent(20),
        Constraint.Fill(1),
        Constraint.Fill(1),
    ];
    const v_rects = Layout.splitVertical(alloc, v_area, &v_constraints)?;
    defer alloc.free(v_rects);

    expect_eq(v_rects.len, 4);
    expect_eq(v_rects[0].h, 3);
    expect_eq(v_rects[1].h, 5);
    expect_eq(v_rects[2].h, 9);
    expect_eq(v_rects[3].h, 8);
    expect_eq(v_rects[0].y, 4);
    expect_eq(v_rects[1].y, 7);
    expect_eq(v_rects[2].y, 12);
    expect_eq(v_rects[3].y, 21);

    let v_sum: u16 = 0;
    for (vr of v_rects) {
        expect_eq(vr.x, 2);
        expect_eq(vr.w, 80);
        v_sum += vr.h;
    }
    expect_eq(v_sum, 25);

    // 3. Horizontal split with weighted Fill(1), Fill(2), Min(5) and remainder distribution
    // Total w=40: Min(5)=5, remaining=35 across Fill(1)+Fill(2) -> 11 + 23 = 34 + 1 leftover -> 12 and 23
    const h_area = Rect.init(0, 0, 40, 10);
    const h_constraints: [3]Constraint = [
        Constraint.Min(5),
        Constraint.Fill(1),
        Constraint.Fill(2),
    ];
    const h_rects = Layout.splitHorizontal(alloc, h_area, &h_constraints)?;
    defer alloc.free(h_rects);

    expect_eq(h_rects[0].w, 5);
    expect_eq(h_rects[1].w, 12);
    expect_eq(h_rects[2].w, 23);
    expect_eq(h_rects[0].x, 0);
    expect_eq(h_rects[1].x, 5);
    expect_eq(h_rects[2].x, 17);
    expect_eq(h_rects[0].w + h_rects[1].w + h_rects[2].w, 40);

    // 4. Overflow guard: Fixed constraints exceeding parent never overflow
    const small = Rect.init(0, 0, 10, 5);
    const over_c: [3]Constraint = [
        Constraint.Fixed(7),
        Constraint.Fixed(6),
        Constraint.Fill(1),
    ];
    const over_r = Layout.splitHorizontal(alloc, small, &over_c)?;
    defer alloc.free(over_r);
    expect_eq(over_r[0].w, 7);
    expect_eq(over_r[1].w, 3);
    expect_eq(over_r[2].w, 0);
}

fn testTuiBufferDrawing(): !void {
    const alloc = std.heap.page_allocator;

    let buf = TuiBuffer.init(alloc, 12, 6)?;
    defer buf.deinit();

    // 1. Rounded box with title
    const box_rect = Rect.init(0, 0, 10, 4);
    const box_style = CellStyle{ fg: 14, bold: true };
    buf.drawBox(box_rect, BorderStyle.Rounded, "Agent", box_style);

    expect_eq(buf.getCell(0, 0).?.ch, 0x256D); // ╭
    expect_eq(buf.getCell(9, 0).?.ch, 0x256E); // ╮
    expect_eq(buf.getCell(0, 3).?.ch, 0x2570); // ╰
    expect_eq(buf.getCell(9, 3).?.ch, 0x256F); // ╯
    expect_eq(buf.getCell(0, 1).?.ch, 0x2502); // │
    expect_eq(buf.getCell(1, 3).?.ch, 0x2500); // ─
    expect_eq(buf.getCell(1, 0).?.ch, 'A');
    expect_eq(buf.getCell(5, 0).?.ch, 't');
    expect_eq(buf.getCell(6, 0).?.ch, 0x2500); // ─

    // 2. Clipped drawText inside inner box and out-of-bounds protection
    buf.drawText(box_rect.inner(1, 1), "HelloWorldExtra\nLine2", CellStyle.default());
    expect_eq(buf.getCell(1, 1).?.ch, 'H');
    expect_eq(buf.getCell(8, 1).?.ch, 'r');
    expect_eq(buf.getCell(9, 1).?.ch, 0x2502); // right border preserved
    expect_eq(buf.getCell(1, 2).?.ch, 'L');
    expect_eq(buf.getCell(5, 2).?.ch, '2');
    expect_eq(buf.getCell(1, 3).?.ch, 0x2500); // bottom border preserved

    // 3. Sparkline rendering across min..max range
    const spark_rect = Rect.init(0, 5, 5, 1);
    const vals: [5]f64 = [0.0, 25.0, 50.0, 75.0, 100.0];
    buf.drawSparkline(spark_rect, &vals, CellStyle.default());
    expect_eq(buf.getCell(0, 5).?.ch, 0x2581); // ▁
    expect_eq(buf.getCell(4, 5).?.ch, 0x2588); // █
    expect_eq(buf.getCell(2, 5).?.ch > 0x2581 && buf.getCell(2, 5).?.ch < 0x2588, true);

    // Out-of-bounds getCell returns null
    expect_eq(buf.getCell(12, 0) == null, true);
    expect_eq(buf.getCell(0, 6) == null, true);
}

fn testTuiDiffZeroFlicker(): !void {
    const alloc = std.heap.page_allocator;

    let prev = TuiBuffer.init(alloc, 10, 5)?;
    defer prev.deinit();
    let curr = TuiBuffer.init(alloc, 10, 5)?;
    defer curr.deinit();

    // 1. Identical buffers emit empty diff
    const empty_diff = TuiDiff.renderDiff(alloc, &prev, &curr)?;
    defer alloc.free(empty_diff);
    expect_eq(empty_diff.len, 0);

    // 2. Single unstyled cell change at (x=4, y=2) -> 1-based row 3, col 5: "\x1b[3;5HX"
    curr.setCell(4, 2, 'X', CellStyle.default());
    const single_diff = TuiDiff.renderDiff(alloc, &prev, &curr)?;
    defer alloc.free(single_diff);
    expect_eq(strEql(single_diff, "\x1b[3;5HX"), true);

    // 3. Adjacent horizontal changes omit redundant cursor positioning
    prev.setCell(4, 2, 'X', CellStyle.default());
    curr.setCell(1, 1, 'A', CellStyle.default());
    curr.setCell(2, 1, 'B', CellStyle.default());
    const adj_diff = TuiDiff.renderDiff(alloc, &prev, &curr)?;
    defer alloc.free(adj_diff);
    expect_eq(strEql(adj_diff, "\x1b[2;2HAB"), true);

    // 4. Styled cell emits SGR + trailing reset
    prev.setCell(1, 1, 'A', CellStyle.default());
    prev.setCell(2, 1, 'B', CellStyle.default());
    const styled = CellStyle{ fg: 196, bold: true };
    curr.setCell(0, 0, '!', styled);
    const styled_diff = TuiDiff.renderDiff(alloc, &prev, &curr)?;
    defer alloc.free(styled_diff);
    expect_eq(strEql(styled_diff, "\x1b[1;1H\x1b[0;1;38;5;196m!\x1b[0m"), true);
}

fn testCapTuiDynamicMount(): !void {
    const alloc = std.heap.page_allocator;

    const ev = cap.tui.InputParser.parseNext("\x1bOP").?;
    expect_eq(ev.code.eql(cap.tui.KeyCode.makeF(1)), true);

    const root = cap.tui.Rect.init(0, 0, 20, 6);
    const cs: [2]cap.tui.Constraint = [
        cap.tui.Constraint.fixed(3),
        cap.tui.Constraint.fill(1),
    ];
    const panes = cap.tui.Layout.splitVertical(alloc, root, &cs)?;
    defer alloc.free(panes);
    expect_eq(panes[0].h, 3);
    expect_eq(panes[1].h, 3);

    let a = cap.tui.TuiBuffer.init(alloc, 20, 6)?;
    defer a.deinit();
    let b = cap.tui.TuiBuffer.init(alloc, 20, 6)?;
    defer b.deinit();

    b.drawBox(panes[0], cap.tui.BorderStyle.Single, "OK", cap.tui.CellStyle.default());
    const out = cap.tui.TuiDiff.renderDiff(alloc, &a, &b)?;
    defer alloc.free(out);
    expect_eq(out.len > 0, true);
}

fn main() {
    test("1. InputParser ASCII/UTF-8/Ctrl/Alt/CSI/Modified/SS3 Sequences", () => {
        testInputParserSequences()?;
    });
    test("2. Layout Vertical/Horizontal Split Conservation & Rect Helpers", () => {
        testLayoutSolverAndRect()?;
    });
    test("3. TuiBuffer drawBox Rounded/Title, Clipped drawText & drawSparkline", () => {
        testTuiBufferDrawing()?;
    });
    test("4. TuiDiff.renderDiff Empty Diff & Minimal Cursor/SGR Emission", () => {
        testTuiDiffZeroFlicker()?;
    });
    test("5. import cap from \"cap\"; cap.tui Automatic Dynamic Mounting", () => {
        testCapTuiDynamicMount()?;
    });
}
