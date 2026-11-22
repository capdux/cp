// Source: cp/lib/qrcode/render.cp (UTF-8 Half-Block Terminal, ASCII & SVG QR Renderer)
// Pure CP QR Code renderers: compact UTF-8 half-block terminal output,
// double-wide ASCII grid, and crisp-edges run-length-merged SVG vector output.

const std = @import("std");
import { QrError, QrMatrix } from "./qrcode_encoder.cp";

fn appendUsizeDecimal(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: usize): ! {
    let tmp: [24]u8 = [0; 24];
    let n: usize = 0;
    let v = val;
    if (v == 0) {
        out.append(alloc, '0') catch return QrError.OutOfMemory;
        return;
    }
    while (v > 0) {
        tmp[n] = ('0' as u8) + ((v % 10) as u8);
        n += 1;
        v /= 10;
    }
    while (n > 0) {
        n -= 1;
        out.append(alloc, tmp[n]) catch return QrError.OutOfMemory;
    }
}

fn paddedModule(mat: QrMatrix, x: usize, y: usize, quiet_zone: usize): bool {
    if (x < quiet_zone || y < quiet_zone) return false;
    const mx = x - quiet_zone;
    const my = y - quiet_zone;
    if (mx >= mat.size || my >= mat.size) return false;
    return mat.get(mx, my);
}

/// QR Code rendering utilities for Terminal UTF-8 half-blocks, ASCII, and SVG.
pub static struct QrRenderer {
    /// Renders `mat` with `quiet_zone` border using UTF-8 half-block characters
    /// (`█` `\u2588`, `▀` `\u2580`, `▄` `\u2584`, ` `), packing two module rows per terminal line.
    static fn toTerminal(
        alloc: std.mem.Allocator,
        mat: QrMatrix,
        quiet_zone: usize,
    ): ([]u8) ! QrError {
        const total = mat.size + 2 * quiet_zone;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let y: usize = 0;
        while (y < total) : (y += 2) {
            let x: usize = 0;
            while (x < total) : (x += 1) {
                const top = paddedModule(mat, x, y, quiet_zone);
                const bot = if (y + 1 < total) paddedModule(mat, x, y + 1, quiet_zone) else false;

                if (top && bot) {
                    out.appendSlice(alloc, "\xe2\x96\x88") catch return QrError.OutOfMemory;
                } else if (top && !bot) {
                    out.appendSlice(alloc, "\xe2\x96\x80") catch return QrError.OutOfMemory;
                } else if (!top && bot) {
                    out.appendSlice(alloc, "\xe2\x96\x84") catch return QrError.OutOfMemory;
                } else {
                    out.append(alloc, ' ') catch return QrError.OutOfMemory;
                }
            }
            out.append(alloc, '\n') catch return QrError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return QrError.OutOfMemory;
    }

    /// Renders `mat` as square-aspect ASCII art (`##` for dark, `  ` for light).
    static fn toAscii(
        alloc: std.mem.Allocator,
        mat: QrMatrix,
        quiet_zone: usize,
    ): ([]u8) ! QrError {
        const total = mat.size + 2 * quiet_zone;
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        let y: usize = 0;
        while (y < total) : (y += 1) {
            let x: usize = 0;
            while (x < total) : (x += 1) {
                if (paddedModule(mat, x, y, quiet_zone)) {
                    out.appendSlice(alloc, "##") catch return QrError.OutOfMemory;
                } else {
                    out.appendSlice(alloc, "  ") catch return QrError.OutOfMemory;
                }
            }
            out.append(alloc, '\n') catch return QrError.OutOfMemory;
        }

        return out.toOwnedSlice(alloc) catch return QrError.OutOfMemory;
    }

    /// Renders `mat` as a clean `<svg xmlns="http://www.w3.org/2000/svg" viewBox="...">` vector image.
    /// Consecutive dark modules in each row are merged into horizontal path segments.
    static fn toSvg(
        alloc: std.mem.Allocator,
        mat: QrMatrix,
        cell_px: usize,
        quiet_zone: usize,
    ): ([]u8) ! QrError {
        const px = if (cell_px == 0) 1 else cell_px;
        const total = mat.size + 2 * quiet_zone;
        const dim = total * px;

        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        out.appendSlice(alloc, "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 ") catch return QrError.OutOfMemory;
        appendUsizeDecimal(&out, alloc, dim) catch return QrError.OutOfMemory;
        out.append(alloc, ' ') catch return QrError.OutOfMemory;
        appendUsizeDecimal(&out, alloc, dim) catch return QrError.OutOfMemory;
        out.appendSlice(alloc, "\" width=\"") catch return QrError.OutOfMemory;
        appendUsizeDecimal(&out, alloc, dim) catch return QrError.OutOfMemory;
        out.appendSlice(alloc, "\" height=\"") catch return QrError.OutOfMemory;
        appendUsizeDecimal(&out, alloc, dim) catch return QrError.OutOfMemory;
        out.appendSlice(alloc, "\" shape-rendering=\"crispEdges\">\n<rect width=\"100%\" height=\"100%\" fill=\"#ffffff\"/>\n<path d=\"") catch return QrError.OutOfMemory;

        let y: usize = 0;
        while (y < mat.size) : (y += 1) {
            let x: usize = 0;
            while (x < mat.size) {
                if (!mat.get(x, y)) {
                    x += 1;
                    continue;
                }
                const run_start = x;
                while (x < mat.size && mat.get(x, y)) : (x += 1) {}
                const run_w = (x - run_start) * px;
                const sx = (run_start + quiet_zone) * px;
                const sy = (y + quiet_zone) * px;

                out.append(alloc, 'M') catch return QrError.OutOfMemory;
                appendUsizeDecimal(&out, alloc, sx) catch return QrError.OutOfMemory;
                out.append(alloc, ' ') catch return QrError.OutOfMemory;
                appendUsizeDecimal(&out, alloc, sy) catch return QrError.OutOfMemory;
                out.append(alloc, 'h') catch return QrError.OutOfMemory;
                appendUsizeDecimal(&out, alloc, run_w) catch return QrError.OutOfMemory;
                out.append(alloc, 'v') catch return QrError.OutOfMemory;
                appendUsizeDecimal(&out, alloc, px) catch return QrError.OutOfMemory;
                out.appendSlice(alloc, "h-") catch return QrError.OutOfMemory;
                appendUsizeDecimal(&out, alloc, run_w) catch return QrError.OutOfMemory;
                out.append(alloc, 'z') catch return QrError.OutOfMemory;
            }
        }

        out.appendSlice(alloc, "\" fill=\"#000000\"/>\n</svg>\n") catch return QrError.OutOfMemory;
        return out.toOwnedSlice(alloc) catch return QrError.OutOfMemory;
    }
}
