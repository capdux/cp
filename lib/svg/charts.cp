// Source: cp/lib/svg/charts.cp (Declarative Statistical Charts Engine: Bar, Line, Pie/Donut)
// Pure CP zero-C-dependency declarative SVG charting with Heckbert nice-tick scaling,
// multi-series normalization, polar arc geometry, and XSS-safe label escaping.

const std = @import("std");
import {
    SvgError,
    SvgPoint,
    SvgStyle,
    SvgPathBuilder,
    SvgCanvas,
    appendFmtNum,
} from "./canvas.cp";

/// Single labeled datum for bar and pie/donut charts.
pub struct ChartDatum {
    label: string = "";
    value: f64 = 0.0;
    color: string = "";
}

/// Named polyline series for multi-series line charts.
pub struct LineSeries {
    name: string = "";
    points: []const SvgPoint = [];
    color: string = "";
}

const DEFAULT_PALETTE: [8]string = [
    "#3b82f6",
    "#10b981",
    "#f59e0b",
    "#ef4444",
    "#8b5cf6",
    "#06b6d4",
    "#ec4899",
    "#84cc16",
];

fn paletteColor(idx: usize, custom: string): string {
    if (custom.len > 0) return custom;
    return DEFAULT_PALETTE[idx % DEFAULT_PALETTE.len];
}

fn isFiniteNum(v: f64): bool {
    return !std.math.isNan(v) && !std.math.isInf(v);
}

struct NiceScale {
    min: f64;
    max: f64;
    step: f64;
}

fn niceNum(range: f64, round_val: bool): f64 {
    if (range <= 0.0) return 1.0;
    const exp_val = std.math.floor(std.math.log10(range));
    const pow10 = std.math.pow(f64, 10.0, exp_val);
    const frac = range / pow10;
    let nice_frac: f64 = 1.0;
    if (round_val) {
        if (frac < 1.5) {
            nice_frac = 1.0;
        } else if (frac < 3.0) {
            nice_frac = 2.0;
        } else if (frac < 7.0) {
            nice_frac = 5.0;
        } else {
            nice_frac = 10.0;
        }
    } else {
        if (frac <= 1.0) {
            nice_frac = 1.0;
        } else if (frac <= 2.0) {
            nice_frac = 2.0;
        } else if (frac <= 5.0) {
            nice_frac = 5.0;
        } else {
            nice_frac = 10.0;
        }
    }
    return nice_frac * pow10;
}

fn computeNiceScale(raw_min: f64, raw_max: f64, max_ticks: usize): NiceScale {
    let min_v = raw_min;
    let max_v = raw_max;
    if (min_v == max_v) {
        if (min_v == 0.0) {
            return NiceScale{ min: 0.0, max: 1.0, step: 0.25 };
        }
        const pad = @abs(min_v) * 0.2;
        min_v -= pad;
        max_v += pad;
    }
    const ticks_f = f64(if (max_ticks > 1) max_ticks - 1 else 1);
    const range = niceNum(max_v - min_v, false);
    const step = niceNum(range / ticks_f, true);
    const nice_min = std.math.floor(min_v / step) * step;
    const nice_max = std.math.ceil(max_v / step) * step;
    return NiceScale{ min: nice_min, max: nice_max, step: step };
}

fn formatTickLabel(alloc: std.mem.Allocator, val: f64): ([]u8) ! SvgError {
    let buf: std.ArrayList(u8) = .empty;
    errdefer buf.deinit(alloc);
    appendFmtNum(alloc, *buf, val)?;
    return buf.toOwnedSlice(alloc) catch SvgError.OutOfMemory;
}

fn formatPercentLabel(alloc: std.mem.Allocator, label: string, pct: f64): ([]u8) ! SvgError {
    let buf: std.ArrayList(u8) = .empty;
    errdefer buf.deinit(alloc);
    buf.appendSlice(alloc, label) catch return SvgError.OutOfMemory;
    buf.appendSlice(alloc, " (") catch return SvgError.OutOfMemory;
    appendFmtNum(alloc, *buf, pct)?;
    buf.appendSlice(alloc, "%)") catch return SvgError.OutOfMemory;
    return buf.toOwnedSlice(alloc) catch SvgError.OutOfMemory;
}

/// Declarative statistical SVG chart generator.
pub static class SvgCharts {
    /// Generates a vertical bar chart SVG supporting positive, negative, and zero values.
    pub static fn barChart(
        alloc: std.mem.Allocator,
        title: string,
        data: []const ChartDatum,
        width: f64,
        height: f64
    ): ([]u8) ! SvgError {
        if (!isFiniteNum(width) || !isFiniteNum(height) || width <= 0.0 || height <= 0.0) {
            return SvgError.InvalidDimensions;
        }
        if (data.len == 0) {
            return SvgError.EmptySeries;
        }

        let raw_min: f64 = 0.0;
        let raw_max: f64 = 0.0;
        for (d of data) {
            if (!isFiniteNum(d.value)) {
                return SvgError.InvalidValue;
            }
            if (d.value < raw_min) raw_min = d.value;
            if (d.value > raw_max) raw_max = d.value;
        }

        const scale = computeNiceScale(raw_min, raw_max, 5);
        const span = if (scale.max - scale.min > 0.0) scale.max - scale.min else 1.0;

        const pad_left: f64 = 64.0;
        const pad_right: f64 = 24.0;
        const pad_top: f64 = if (title.len > 0) 48.0 else 24.0;
        const pad_bottom: f64 = 48.0;
        const plot_w = if (width - pad_left - pad_right > 20.0) width - pad_left - pad_right else 20.0;
        const plot_h = if (height - pad_top - pad_bottom > 20.0) height - pad_top - pad_bottom else 20.0;

        let canvas = SvgCanvas.init(alloc, width, height)?;
        defer canvas.deinit();
        canvas.setBackground("#ffffff");

        if (title.len > 0) {
            canvas.text(
                width * 0.5,
                28.0,
                title,
                SvgStyle{ fill: "#0f172a", font_size: 16.0, anchor: "middle" }
            )?;
        }

        // Grid lines & Y-axis ticks
        canvas.beginGroup("y-grid", "")?;
        let tick = scale.min;
        let guard: usize = 0;
        while (tick <= scale.max + scale.step * 0.001 && guard < 32) {
            const ratio = (tick - scale.min) / span;
            const y = pad_top + plot_h - ratio * plot_h;
            canvas.line(
                pad_left,
                y,
                pad_left + plot_w,
                y,
                SvgStyle{ stroke: "#e2e8f0", stroke_width: 1.0 }
            )?;
            const lbl = formatTickLabel(alloc, tick)?;
            defer alloc.free(lbl);
            canvas.text(
                pad_left - 8.0,
                y + 4.0,
                lbl,
                SvgStyle{ fill: "#64748b", font_size: 11.0, anchor: "end" }
            )?;
            tick += scale.step;
            guard += 1;
        }
        canvas.endGroup()?;

        // Baseline (y = 0) and left axis
        const zero_ratio = (0.0 - scale.min) / span;
        const y_zero = pad_top + plot_h - zero_ratio * plot_h;
        canvas.line(pad_left, pad_top, pad_left, pad_top + plot_h, SvgStyle{ stroke: "#94a3b8", stroke_width: 1.5 })?;
        canvas.line(pad_left, y_zero, pad_left + plot_w, y_zero, SvgStyle{ stroke: "#475569", stroke_width: 1.5 })?;

        // Bars & X labels
        canvas.beginGroup("bars", "")?;
        const slot_w = plot_w / f64(data.len);
        const bar_w = slot_w * 0.6;
        const bar_offset = (slot_w - bar_w) * 0.5;

        let idx: usize = 0;
        while (idx < data.len) : (idx += 1) {
            const d = data[idx];
            const bx = pad_left + f64(idx) * slot_w + bar_offset;
            const val_ratio = (d.value - scale.min) / span;
            const y_val = pad_top + plot_h - val_ratio * plot_h;
            const bar_top = if (d.value >= 0.0) y_val else y_zero;
            const bar_h = @abs(y_zero - y_val);
            const col = paletteColor(idx, d.color);

            canvas.rect(
                bx,
                bar_top,
                bar_w,
                bar_h,
                2.0,
                SvgStyle{ fill: col }
            )?;

            if (d.label.len > 0) {
                canvas.text(
                    bx + bar_w * 0.5,
                    pad_top + plot_h + 20.0,
                    d.label,
                    SvgStyle{ fill: "#334155", font_size: 11.0, anchor: "middle" }
                )?;
            }
        }
        canvas.endGroup()?;

        return canvas.render(alloc);
    }

    /// Generates a multi-series line chart SVG with unified X/Y domain normalization and legend.
    pub static fn lineChart(
        alloc: std.mem.Allocator,
        title: string,
        series: []const LineSeries,
        width: f64,
        height: f64
    ): ([]u8) ! SvgError {
        if (!isFiniteNum(width) || !isFiniteNum(height) || width <= 0.0 || height <= 0.0) {
            return SvgError.InvalidDimensions;
        }
        if (series.len == 0) {
            return SvgError.EmptySeries;
        }

        let has_points = false;
        let x_min: f64 = 0.0;
        let x_max: f64 = 0.0;
        let y_min: f64 = 0.0;
        let y_max: f64 = 0.0;

        for (s of series) {
            for (pt of s.points) {
                if (!isFiniteNum(pt.x) || !isFiniteNum(pt.y)) {
                    return SvgError.InvalidValue;
                }
                if (!has_points) {
                    x_min = pt.x;
                    x_max = pt.x;
                    y_min = pt.y;
                    y_max = pt.y;
                    has_points = true;
                } else {
                    if (pt.x < x_min) x_min = pt.x;
                    if (pt.x > x_max) x_max = pt.x;
                    if (pt.y < y_min) y_min = pt.y;
                    if (pt.y > y_max) y_max = pt.y;
                }
            }
        }
        if (!has_points) {
            return SvgError.EmptySeries;
        }

        const x_scale = computeNiceScale(x_min, x_max, 5);
        const y_scale = computeNiceScale(y_min, y_max, 5);
        const x_span = if (x_scale.max - x_scale.min > 0.0) x_scale.max - x_scale.min else 1.0;
        const y_span = if (y_scale.max - y_scale.min > 0.0) y_scale.max - y_scale.min else 1.0;

        const pad_left: f64 = 64.0;
        const pad_right: f64 = 32.0;
        const pad_top: f64 = if (title.len > 0) 68.0 else 44.0;
        const pad_bottom: f64 = 48.0;
        const plot_w = if (width - pad_left - pad_right > 20.0) width - pad_left - pad_right else 20.0;
        const plot_h = if (height - pad_top - pad_bottom > 20.0) height - pad_top - pad_bottom else 20.0;

        let canvas = SvgCanvas.init(alloc, width, height)?;
        defer canvas.deinit();
        canvas.setBackground("#ffffff");

        if (title.len > 0) {
            canvas.text(
                width * 0.5,
                26.0,
                title,
                SvgStyle{ fill: "#0f172a", font_size: 16.0, anchor: "middle" }
            )?;
        }

        // Y-axis grid & labels
        canvas.beginGroup("grid", "")?;
        let y_tick = y_scale.min;
        let yg: usize = 0;
        while (y_tick <= y_scale.max + y_scale.step * 0.001 && yg < 32) {
            const ry = (y_tick - y_scale.min) / y_span;
            const py = pad_top + plot_h - ry * plot_h;
            canvas.line(pad_left, py, pad_left + plot_w, py, SvgStyle{ stroke: "#e2e8f0", stroke_width: 1.0 })?;
            const lbl = formatTickLabel(alloc, y_tick)?;
            defer alloc.free(lbl);
            canvas.text(pad_left - 8.0, py + 4.0, lbl, SvgStyle{ fill: "#64748b", font_size: 11.0, anchor: "end" })?;
            y_tick += y_scale.step;
            yg += 1;
        }

        // X-axis grid & labels
        let x_tick = x_scale.min;
        let xg: usize = 0;
        while (x_tick <= x_scale.max + x_scale.step * 0.001 && xg < 32) {
            const rx = (x_tick - x_scale.min) / x_span;
            const px = pad_left + rx * plot_w;
            canvas.line(px, pad_top, px, pad_top + plot_h, SvgStyle{ stroke: "#f1f5f9", stroke_width: 1.0 })?;
            const lbl = formatTickLabel(alloc, x_tick)?;
            defer alloc.free(lbl);
            canvas.text(px, pad_top + plot_h + 18.0, lbl, SvgStyle{ fill: "#64748b", font_size: 11.0, anchor: "middle" })?;
            x_tick += x_scale.step;
            xg += 1;
        }
        canvas.endGroup()?;

        // Axes
        canvas.line(pad_left, pad_top, pad_left, pad_top + plot_h, SvgStyle{ stroke: "#94a3b8", stroke_width: 1.5 })?;
        canvas.line(pad_left, pad_top + plot_h, pad_left + plot_w, pad_top + plot_h, SvgStyle{ stroke: "#94a3b8", stroke_width: 1.5 })?;

        // Series polylines & markers
        canvas.beginGroup("series", "")?;
        let mapped: std.ArrayList(SvgPoint) = .empty;
        defer mapped.deinit(alloc);

        let s_idx: usize = 0;
        while (s_idx < series.len) : (s_idx += 1) {
            const s = series[s_idx];
            const col = paletteColor(s_idx, s.color);
            mapped.clearRetainingCapacity();

            for (pt of s.points) {
                const sx = pad_left + ((pt.x - x_scale.min) / x_span) * plot_w;
                const sy = pad_top + plot_h - ((pt.y - y_scale.min) / y_span) * plot_h;
                mapped.append(alloc, SvgPoint{ x: sx, y: sy }) catch return SvgError.OutOfMemory;
            }

            if (mapped.items.len >= 2) {
                canvas.polyline(mapped.items, SvgStyle{ fill: "none", stroke: col, stroke_width: 2.0 })?;
            }
            for (mpt of mapped.items) {
                canvas.circle(mpt.x, mpt.y, 3.5, SvgStyle{ fill: col, stroke: "#ffffff", stroke_width: 1.0 })?;
            }
        }
        canvas.endGroup()?;

        // Legend
        canvas.beginGroup("legend", "")?;
        let leg_x = pad_left;
        const leg_y: f64 = if (title.len > 0) 44.0 else 18.0;
        let l_idx: usize = 0;
        while (l_idx < series.len) : (l_idx += 1) {
            const s = series[l_idx];
            const col = paletteColor(l_idx, s.color);
            canvas.rect(leg_x, leg_y - 8.0, 12.0, 12.0, 2.0, SvgStyle{ fill: col })?;
            canvas.text(leg_x + 16.0, leg_y + 2.0, s.name, SvgStyle{ fill: "#334155", font_size: 11.0, anchor: "start" })?;
            leg_x += 28.0 + f64(s.name.len) * 7.0;
            if (leg_x > width - 60.0) {
                leg_x = pad_left;
            }
        }
        canvas.endGroup()?;

        return canvas.render(alloc);
    }

    /// Generates a pie chart (`donut_ratio == 0`) or donut chart (`0 < donut_ratio < 1`) SVG.
    pub static fn pieChart(
        alloc: std.mem.Allocator,
        title: string,
        data: []const ChartDatum,
        width: f64,
        height: f64,
        donut_ratio: f64
    ): ([]u8) ! SvgError {
        if (!isFiniteNum(width) || !isFiniteNum(height) || width <= 0.0 || height <= 0.0) {
            return SvgError.InvalidDimensions;
        }
        if (data.len == 0) {
            return SvgError.EmptySeries;
        }
        if (!isFiniteNum(donut_ratio) || donut_ratio < 0.0 || donut_ratio >= 1.0) {
            return SvgError.InvalidValue;
        }

        let total: f64 = 0.0;
        for (d of data) {
            if (!isFiniteNum(d.value) || d.value < 0.0) {
                return SvgError.InvalidValue;
            }
            total += d.value;
        }
        if (total <= 0.0) {
            return SvgError.InvalidValue;
        }

        let canvas = SvgCanvas.init(alloc, width, height)?;
        defer canvas.deinit();
        canvas.setBackground("#ffffff");

        if (title.len > 0) {
            canvas.text(
                width * 0.5,
                28.0,
                title,
                SvgStyle{ fill: "#0f172a", font_size: 16.0, anchor: "middle" }
            )?;
        }

        const top_offset: f64 = if (title.len > 0) 44.0 else 16.0;
        const avail_h = if (height - top_offset - 16.0 > 20.0) height - top_offset - 16.0 else 20.0;
        const chart_w = width * 0.62;
        const cx = chart_w * 0.5;
        const cy = top_offset + avail_h * 0.5;
        const min_dim = if (chart_w < avail_h) chart_w else avail_h;
        const outer_r = min_dim * 0.42;
        const inner_r = outer_r * donut_ratio;

        canvas.beginGroup("slices", "")?;
        let pb = SvgPathBuilder.init(alloc);
        defer pb.deinit();

        let angle: f64 = -1.0 * std.math.pi * 0.5;
        let idx: usize = 0;
        while (idx < data.len) : (idx += 1) {
            const d = data[idx];
            if (d.value <= 0.0) continue;

            const frac = d.value / total;
            const col = paletteColor(idx, d.color);
            pb.buf.clearRetainingCapacity();

            if (frac >= 0.999999) {
                // Full-circle 100% slice: split into two 180-degree arcs so SVG A-command never degenerates.
                pb.moveTo(cx, cy - outer_r)?;
                pb.arcTo(outer_r, outer_r, 0.0, false, true, cx, cy + outer_r)?;
                pb.arcTo(outer_r, outer_r, 0.0, false, true, cx, cy - outer_r)?;
                pb.close()?;
                if (inner_r > 0.0) {
                    pb.moveTo(cx, cy - inner_r)?;
                    pb.arcTo(inner_r, inner_r, 0.0, false, false, cx, cy + inner_r)?;
                    pb.arcTo(inner_r, inner_r, 0.0, false, false, cx, cy - inner_r)?;
                    pb.close()?;
                }
            } else {
                const sweep_angle = frac * 2.0 * std.math.pi;
                const next_angle = angle + sweep_angle;
                const large_arc = sweep_angle > std.math.pi;

                const ox1 = cx + outer_r * std.math.cos(angle);
                const oy1 = cy + outer_r * std.math.sin(angle);
                const ox2 = cx + outer_r * std.math.cos(next_angle);
                const oy2 = cy + outer_r * std.math.sin(next_angle);

                if (inner_r > 0.0) {
                    const ix1 = cx + inner_r * std.math.cos(angle);
                    const iy1 = cy + inner_r * std.math.sin(angle);
                    const ix2 = cx + inner_r * std.math.cos(next_angle);
                    const iy2 = cy + inner_r * std.math.sin(next_angle);

                    pb.moveTo(ox1, oy1)?;
                    pb.arcTo(outer_r, outer_r, 0.0, large_arc, true, ox2, oy2)?;
                    pb.lineTo(ix2, iy2)?;
                    pb.arcTo(inner_r, inner_r, 0.0, large_arc, false, ix1, iy1)?;
                    pb.close()?;
                } else {
                    pb.moveTo(cx, cy)?;
                    pb.lineTo(ox1, oy1)?;
                    pb.arcTo(outer_r, outer_r, 0.0, large_arc, true, ox2, oy2)?;
                    pb.close()?;
                }
                angle = next_angle;
            }

            canvas.path(pb.slice(), SvgStyle{ fill: col, stroke: "#ffffff", stroke_width: 1.5 })?;
        }
        canvas.endGroup()?;

        // Legend on the right side
        canvas.beginGroup("legend", "")?;
        const leg_x = chart_w + 8.0;
        let leg_y = top_offset + 16.0;
        let l_idx: usize = 0;
        while (l_idx < data.len) : (l_idx += 1) {
            const d = data[l_idx];
            const col = paletteColor(l_idx, d.color);
            const pct = (d.value / total) * 100.0;
            const lbl = formatPercentLabel(alloc, d.label, pct)?;
            defer alloc.free(lbl);

            canvas.rect(leg_x, leg_y - 10.0, 12.0, 12.0, 2.0, SvgStyle{ fill: col })?;
            canvas.text(leg_x + 18.0, leg_y, lbl, SvgStyle{ fill: "#334155", font_size: 11.0, anchor: "start" })?;
            leg_y += 22.0;
        }
        canvas.endGroup()?;

        return canvas.render(alloc);
    }
}
