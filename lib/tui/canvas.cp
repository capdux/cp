// Source: cp/lib/tui/canvas.cp (2D Cell Buffer, Box Drawing & Zero-Flicker Diff Renderer)
// Pure CP 2D terminal cell canvas, Unicode box-drawing/sparkline primitives,
// and minimal-ANSI zero-flicker diff renderer.

const std = @import("std");
import { TuiError, Rect } from "./layout.cp";

const SPARK_BARS: [8]u21 = [
    0x2581, // ▁
    0x2582, // ▂
    0x2583, // ▃
    0x2584, // ▄
    0x2585, // ▅
    0x2586, // ▆
    0x2587, // ▇
    0x2588, // █
];

/// Box border rendering style.
pub enum BorderStyle {
    Single,
    Double,
    Rounded,
    Ascii,
}

fn optU8Eql(a: u8?, b: u8?): bool {
    if (va := a) {
        if (vb := b) return va == vb;
        return false;
    }
    return b == null;
}

/// Terminal SGR style attributes for a single cell.
pub struct CellStyle {
    fg: u8? = null;
    bg: u8? = null;
    bold: bool = false;
    dim: bool = false;
    underline: bool = false;

    /// Returns the default unstyled `CellStyle`.
    static fn default(): CellStyle {
        return CellStyle{};
    }

    /// Returns `true` if no foreground, background, or text attribute is set.
    fn isDefault(): bool {
        return _.fg == null && _.bg == null && !_.bold && !_.dim && !_.underline;
    }

    /// Returns `true` if all style fields equal `other`.
    fn eql(other: CellStyle): bool {
        return optU8Eql(_.fg, other.fg) && optU8Eql(_.bg, other.bg) && _.bold == other.bold && _.dim == other.dim && _.underline == other.underline;
    }
}

/// Single terminal character cell holding a Unicode scalar value and SGR style.
pub struct Cell {
    ch: u21 = ' ';
    style: CellStyle = CellStyle{};

    /// Returns a blank space cell with default style.
    static fn blank(): Cell {
        return Cell{ ch: ' ', style: CellStyle.default() };
    }

    /// Returns `true` if both character codepoint and style match `other`.
    fn eql(other: Cell): bool {
        return _.ch == other.ch && _.style.eql(other.style);
    }
}

struct BorderChars {
    tl: u21;
    tr: u21;
    bl: u21;
    br: u21;
    h: u21;
    v: u21;
}

fn getBorderChars(style: BorderStyle): BorderChars {
    return switch (style) {
        .Single => BorderChars{ tl: 0x250C, tr: 0x2510, bl: 0x2514, br: 0x2518, h: 0x2500, v: 0x2502 },
        .Double => BorderChars{ tl: 0x2554, tr: 0x2557, bl: 0x255A, br: 0x255D, h: 0x2550, v: 0x2551 },
        .Rounded => BorderChars{ tl: 0x256D, tr: 0x256E, bl: 0x2570, br: 0x256F, h: 0x2500, v: 0x2502 },
        .Ascii => BorderChars{ tl: '+', tr: '+', bl: '+', br: '+', h: '-', v: '|' },
    };
}

fn appendU32Decimal(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u32): void ! TuiError {
    if (val == 0) {
        out.append(alloc, '0') catch return TuiError.OutOfMemory;
    } else {
        let tmp: [16]u8 = [0; 16];
        let n: usize = 0;
        let v = val;
        while (v > 0) {
            tmp[n] = ('0' as u8) + u8(v % 10);
            n += 1;
            v /= 10;
        }
        while (n > 0) {
            n -= 1;
            out.append(alloc, tmp[n]) catch return TuiError.OutOfMemory;
        }
    }
}

fn appendStyleSgr(out: *std.ArrayList(u8), alloc: std.mem.Allocator, style: CellStyle): void ! TuiError {
    if (style.isDefault()) {
        out.appendSlice(alloc, "\x1b[0m") catch return TuiError.OutOfMemory;
    } else {
        out.appendSlice(alloc, "\x1b[0") catch return TuiError.OutOfMemory;
        if (style.bold) out.appendSlice(alloc, ";1") catch return TuiError.OutOfMemory;
        if (style.dim) out.appendSlice(alloc, ";2") catch return TuiError.OutOfMemory;
        if (style.underline) out.appendSlice(alloc, ";4") catch return TuiError.OutOfMemory;
        if (fg_val := style.fg) {
            out.appendSlice(alloc, ";38;5;") catch return TuiError.OutOfMemory;
            appendU32Decimal(out, alloc, u32(fg_val))?;
        }
        if (bg_val := style.bg) {
            out.appendSlice(alloc, ";48;5;") catch return TuiError.OutOfMemory;
            appendU32Decimal(out, alloc, u32(bg_val))?;
        }
        out.append(alloc, 'm') catch return TuiError.OutOfMemory;
    }
}

/// 2D off-screen terminal cell buffer supporting clipped drawing primitives.
pub struct TuiBuffer {
    alloc: std.mem.Allocator;
    width: u16;
    height: u16;
    cells: []Cell;

    /// Allocates and clears a `w x h` cell buffer.
    static fn init(alloc: std.mem.Allocator, w: u16, h: u16): TuiBuffer ! TuiError {
        const total = usize(w) * usize(h);
        let cells = alloc.alloc(Cell, total) catch return TuiError.OutOfMemory;
        let i: usize = 0;
        while (i < total) : (i += 1) {
            cells[i] = Cell.blank();
        }
        return TuiBuffer{
            alloc: alloc,
            width: w,
            height: h,
            cells: cells,
        };
    }

    /// Frees the underlying cell storage.
    fn deinit(*_) {
        if (_.cells.len > 0) {
            _.alloc.free(_.cells);
        }
        _.cells = [];
        _.width = 0;
        _.height = 0;
    }

    /// Resets all cells in the buffer to blank spaces with default style.
    fn clear(*_) {
        const blank = Cell.blank();
        let i: usize = 0;
        while (i < _.cells.len) : (i += 1) {
            _.cells[i] = blank;
        }
    }

    /// Writes a single cell at `(x, y)` if within buffer bounds.
    fn setCell(*_, x: u16, y: u16, ch: u21, style: CellStyle) {
        if (x >= _.width || y >= _.height) return;
        const idx = usize(y) * usize(_.width) + usize(x);
        _.cells[idx] = Cell{ ch: ch, style: style };
    }

    /// Returns the cell at `(x, y)`, or `null` if out of bounds.
    fn getCell(x: u16, y: u16): Cell? {
        if (x >= _.width || y >= _.height) return null;
        const idx = usize(y) * usize(_.width) + usize(x);
        return _.cells[idx];
    }

    /// Draws UTF-8 `text` clipped strictly inside `rect` and buffer bounds.
    /// Supports `\n` line breaks inside `rect`.
    fn drawText(*_, rect: Rect, text: string, style: CellStyle) {
        if (rect.isEmpty() || _.width == 0 || _.height == 0) return;

        const max_x_rect = u32(rect.x) + u32(rect.w);
        const max_y_rect = u32(rect.y) + u32(rect.h);
        const max_x = if (max_x_rect < u32(_.width)) max_x_rect else u32(_.width);
        const max_y = if (max_y_rect < u32(_.height)) max_y_rect else u32(_.height);

        let cur_x: u32 = u32(rect.x);
        let cur_y: u32 = u32(rect.y);
        if (cur_x >= max_x || cur_y >= max_y) return;

        let i: usize = 0;
        while (i < text.len) {
            const b0 = text[i];
            const seq_len_u3 = std.unicode.utf8ByteSequenceLength(b0) catch 1;
            const seq_len: usize = seq_len_u3;
            let cp: u21 = 0xFFFD;
            let step: usize = 1;

            if (seq_len > 0 && i + seq_len <= text.len) {
                cp = std.unicode.utf8Decode(text[i..i + seq_len]) catch 0xFFFD;
                step = seq_len;
            }
            i += step;

            if (cp == '\r') continue;
            if (cp == '\n') {
                cur_y += 1;
                if (cur_y >= max_y) break;
                cur_x = u32(rect.x);
                continue;
            }

            if (cur_x < max_x && cur_y < max_y) {
                _.setCell(u16(cur_x), u16(cur_y), cp, style);
            }
            cur_x += 1;
        }
    }

    /// Draws a box border around `rect` using `border` style and optional `title` on the top edge.
    fn drawBox(*_, rect: Rect, border: BorderStyle, title: string, style: CellStyle) {
        if (rect.w < 2 || rect.h < 2) return;

        const bc = getBorderChars(border);
        const x0 = rect.x;
        const y0 = rect.y;
        const x1: u16 = u16(u32(rect.x) + u32(rect.w) - 1);
        const y1: u16 = u16(u32(rect.y) + u32(rect.h) - 1);

        _.setCell(x0, y0, bc.tl, style);
        _.setCell(x1, y0, bc.tr, style);
        _.setCell(x0, y1, bc.bl, style);
        _.setCell(x1, y1, bc.br, style);

        let x: u32 = u32(x0) + 1;
        while (x < u32(x1)) : (x += 1) {
            _.setCell(u16(x), y0, bc.h, style);
            _.setCell(u16(x), y1, bc.h, style);
        }

        let y: u32 = u32(y0) + 1;
        while (y < u32(y1)) : (y += 1) {
            _.setCell(x0, u16(y), bc.v, style);
            _.setCell(x1, u16(y), bc.v, style);
        }

        if (title.len > 0 && rect.w > 2) {
            const title_rect = Rect{
                x: x0 + 1,
                y: y0,
                w: rect.w - 2,
                h: 1,
            };
            _.drawText(title_rect, title, style);
        }
    }

    /// Draws a Unicode sparkline (`▁▂▃▄▅▆▇█`) for `values` inside `rect`.
    fn drawSparkline(*_, rect: Rect, values: []const f64, style: CellStyle) {
        if (rect.isEmpty() || values.len == 0) return;

        const count = if (values.len < usize(rect.w)) values.len else usize(rect.w);
        let min_v: f64 = 0.0;
        let max_v: f64 = 0.0;
        let has_valid = false;

        let i: usize = 0;
        while (i < count) : (i += 1) {
            const v = values[i];
            if (std.math.isNan(v) || std.math.isInf(v)) continue;
            if (!has_valid) {
                min_v = v;
                max_v = v;
                has_valid = true;
            } else {
                if (v < min_v) min_v = v;
                if (v > max_v) max_v = v;
            }
        }
        if (!has_valid) return;

        const span = max_v - min_v;
        const bar_y = u16(u32(rect.y) + u32(rect.h) - 1);

        let col: usize = 0;
        while (col < count) : (col += 1) {
            const v = values[col];
            const clean_v = if (std.math.isNan(v) || std.math.isInf(v)) min_v else v;
            let level: usize = 0;
            if (span > 0.0) {
                const norm = (clean_v - min_v) / span;
                const scaled = norm * 7.0;
                const rounded = @as(i32, @intFromFloat(@round(scaled)));
                if (rounded < 0) {
                    level = 0;
                } else if (rounded > 7) {
                    level = 7;
                } else {
                    level = usize(rounded);
                }
            } else if (clean_v > 0.0) {
                level = 4;
            }
            const cx = u32(rect.x) + u32(col);
            if (cx < u32(_.width) && u32(bar_y) < u32(_.height)) {
                _.setCell(u16(cx), bar_y, SPARK_BARS[level], style);
            }
        }
    }
}

/// Zero-flicker 2D buffer diff renderer emitting minimal ANSI cursor movement and SGR codes.
pub static struct TuiDiff {
    /// Computes the minimal ANSI byte stream required to transform `prev` into `curr`.
    /// Emits an empty slice when both buffers have identical cells.
    /// Caller owns the returned slice and must free it with `alloc.free(...)`.
    static fn renderDiff(
        alloc: std.mem.Allocator,
        prev: *const TuiBuffer,
        curr: *const TuiBuffer,
    ): ([]u8) ! TuiError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let cursor_x: i32 = -1;
        let cursor_y: i32 = -1;
        let active_style = CellStyle.default();
        let style_dirty = false;

        let y: u16 = 0;
        while (y < curr.height) : (y += 1) {
            let x: u16 = 0;
            while (x < curr.width) : (x += 1) {
                const c_cell = curr.getCell(x, y) orelse continue;
                if (p_cell := prev.getCell(x, y)) {
                    if (p_cell.eql(c_cell)) continue;
                }

                // Emit cursor positioning only if not already positioned at (x, y)
                if (cursor_y != i32(y) || cursor_x != i32(x)) {
                    out.appendSlice(alloc, "\x1b[") catch return TuiError.OutOfMemory;
                    appendU32Decimal(&out, alloc, u32(y) + 1)?;
                    out.append(alloc, ';') catch return TuiError.OutOfMemory;
                    appendU32Decimal(&out, alloc, u32(x) + 1)?;
                    out.append(alloc, 'H') catch return TuiError.OutOfMemory;
                }

                // Emit SGR sequence only when cell style differs from current terminal state
                if (!c_cell.style.eql(active_style)) {
                    appendStyleSgr(&out, alloc, c_cell.style)?;
                    active_style = c_cell.style;
                    style_dirty = !active_style.isDefault();
                }

                let utf8_buf: [4]u8 = [' ', 0, 0, 0];
                const n_bytes: usize = std.unicode.utf8Encode(c_cell.ch, &utf8_buf) catch @as(usize, 1);
                out.appendSlice(alloc, utf8_buf[0..n_bytes]) catch return TuiError.OutOfMemory;

                cursor_x = i32(x) + 1;
                cursor_y = i32(y);
            }
        }

        if (style_dirty) {
            out.appendSlice(alloc, "\x1b[0m") catch return TuiError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return TuiError.OutOfMemory;
    }
}
