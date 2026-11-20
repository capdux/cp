// Source: cp/lib/svg/canvas.cp (SVG 1.1 Canvas, Path Builder & XSS-Safe Serializer)
// Pure CP zero-C-dependency vector graphics canvas with fluent path builder,
// hierarchical grouping, coordinate validation, and strict XML entity escaping.

const std = @import("std");

/// Error set for SVG canvas, path construction, and chart rendering.
pub err SvgError {
    InvalidDimensions,
    EmptySeries,
    InvalidValue,
    OutOfMemory,
}

/// 2D point in SVG user-space coordinates.
pub struct SvgPoint {
    x: f64 = 0.0;
    y: f64 = 0.0;
}

/// Visual presentation style attributes for SVG primitives.
pub struct SvgStyle {
    fill: string = "none";
    stroke: string = "none";
    stroke_width: f64 = 0.0;
    opacity: f64 = 1.0;
    font_size: f64 = 12.0;
    anchor: string = "";
}

fn isFinite(v: f64): bool {
    return !std.math.isNan(v) && !std.math.isInf(v);
}

fn appendSliceChecked(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: string): void ! SvgError {
    buf.appendSlice(alloc, s) catch return SvgError.OutOfMemory;
}

fn appendByteChecked(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), b: u8): void ! SvgError {
    buf.append(alloc, b) catch return SvgError.OutOfMemory;
}

/// Formats a finite `f64` compactly (up to 2 decimal places, trimming trailing zeros).
pub fn appendFmtNum(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), val: f64): void ! SvgError {
    if (!isFinite(val)) {
        return SvgError.InvalidValue;
    }
    const clean = if (val > -0.000001 && val < 0.000001) 0.0 else val;
    let tmp: [64]u8 = undefined;
    let formatted: string = std.fmt.bufPrint(&tmp, "{d:.2}", .{clean}) catch return SvgError.OutOfMemory;

    // Trim trailing '0' after decimal point, and trailing '.' if whole number.
    let has_dot = false;
    for (ch of formatted) {
        if (ch == '.') {
            has_dot = true;
            break;
        }
    }
    if (has_dot) {
        let end = formatted.len;
        while (end > 0 && formatted[end - 1] == '0') {
            end -= 1;
        }
        if (end > 0 && formatted[end - 1] == '.') {
            end -= 1;
        }
        formatted = formatted[0..end];
    }
    if (std.mem.eql(u8, formatted, "-0")) {
        formatted = "0";
    }
    appendSliceChecked(alloc, buf, formatted)?;
}

/// Escapes XML special characters (`&`, `<`, `>`, `"`, `'`) for safe attribute values.
fn appendEscapedAttr(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: string): void ! SvgError {
    let i: usize = 0;
    let flush_start: usize = 0;
    while (i < s.len) : (i += 1) {
        const c = s[i];
        if (c == '&' || c == '<' || c == '>' || c == '"' || c == '\'') {
            if (i > flush_start) {
                appendSliceChecked(alloc, buf, s[flush_start..i])?;
            }
            switch (c) {
                '&' => appendSliceChecked(alloc, buf, "&amp;")?,
                '<' => appendSliceChecked(alloc, buf, "&lt;")?,
                '>' => appendSliceChecked(alloc, buf, "&gt;")?,
                '"' => appendSliceChecked(alloc, buf, "&quot;")?,
                '\'' => appendSliceChecked(alloc, buf, "&apos;")?,
                else => {},
            }
            flush_start = i + 1;
        }
    }
    if (i > flush_start) {
        appendSliceChecked(alloc, buf, s[flush_start..i])?;
    }
}

/// Escapes XML special characters (`&`, `<`, `>`, `"`, `'`) for safe element text content.
fn appendEscapedText(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), s: string): void ! SvgError {
    appendEscapedAttr(alloc, buf, s)?;
}

fn appendStyleAttrs(alloc: std.mem.Allocator, buf: *std.ArrayList(u8), style: SvgStyle, include_text_attrs: bool): void ! SvgError {
    if (!isFinite(style.stroke_width) || !isFinite(style.opacity) || !isFinite(style.font_size)) {
        return SvgError.InvalidValue;
    }
    if (style.fill.len > 0) {
        appendSliceChecked(alloc, buf, " fill=\"")?;
        appendEscapedAttr(alloc, buf, style.fill)?;
        appendByteChecked(alloc, buf, '"')?;
    }
    if (style.stroke.len > 0 && !std.mem.eql(u8, style.stroke, "none")) {
        appendSliceChecked(alloc, buf, " stroke=\"")?;
        appendEscapedAttr(alloc, buf, style.stroke)?;
        appendByteChecked(alloc, buf, '"')?;
        if (style.stroke_width > 0.0) {
            appendSliceChecked(alloc, buf, " stroke-width=\"")?;
            appendFmtNum(alloc, buf, style.stroke_width)?;
            appendByteChecked(alloc, buf, '"')?;
        }
    }
    if (style.opacity < 0.9999) {
        appendSliceChecked(alloc, buf, " opacity=\"")?;
        appendFmtNum(alloc, buf, style.opacity)?;
        appendByteChecked(alloc, buf, '"')?;
    }
    if (include_text_attrs) {
        if (style.font_size > 0.0) {
            appendSliceChecked(alloc, buf, " font-size=\"")?;
            appendFmtNum(alloc, buf, style.font_size)?;
            appendByteChecked(alloc, buf, '"')?;
        }
        if (style.anchor.len > 0) {
            appendSliceChecked(alloc, buf, " text-anchor=\"")?;
            appendEscapedAttr(alloc, buf, style.anchor)?;
            appendByteChecked(alloc, buf, '"')?;
        }
    }
}

/// Fluent builder for SVG `<path d="...">` command strings.
pub struct SvgPathBuilder {
    alloc: std.mem.Allocator;
    buf: std.ArrayList(u8) = .empty;

    /// Initializes an empty `SvgPathBuilder`.
    pub static fn init(alloc: std.mem.Allocator): SvgPathBuilder {
        return SvgPathBuilder{
            alloc: alloc,
            buf: .empty,
        };
    }

    /// Releases the internal path command buffer.
    pub fn deinit(*_) {
        _.buf.deinit(_.alloc);
    }

    fn appendSep(*_): void ! SvgError {
        if (_.buf.items.len > 0) {
            appendByteChecked(_.alloc, *_.buf, ' ')?;
        }
    }

    /// Appends an absolute `M x y` move-to command.
    pub fn moveTo(*_, x: f64, y: f64): void ! SvgError {
        if (!isFinite(x) || !isFinite(y)) return SvgError.InvalidValue;
        _.appendSep()?;
        appendSliceChecked(_.alloc, *_.buf, "M ")?;
        appendFmtNum(_.alloc, *_.buf, x)?;
        appendByteChecked(_.alloc, *_.buf, ' ')?;
        appendFmtNum(_.alloc, *_.buf, y)?;
    }

    /// Appends an absolute `L x y` line-to command.
    pub fn lineTo(*_, x: f64, y: f64): void ! SvgError {
        if (!isFinite(x) || !isFinite(y)) return SvgError.InvalidValue;
        _.appendSep()?;
        appendSliceChecked(_.alloc, *_.buf, "L ")?;
        appendFmtNum(_.alloc, *_.buf, x)?;
        appendByteChecked(_.alloc, *_.buf, ' ')?;
        appendFmtNum(_.alloc, *_.buf, y)?;
    }

    /// Appends an absolute cubic Bezier `C cx1 cy1 cx2 cy2 x y` command.
    pub fn curveTo(*_, cx1: f64, cy1: f64, cx2: f64, cy2: f64, x: f64, y: f64): void ! SvgError {
        if (!isFinite(cx1) || !isFinite(cy1) || !isFinite(cx2) || !isFinite(cy2) || !isFinite(x) || !isFinite(y)) {
            return SvgError.InvalidValue;
        }
        _.appendSep()?;
        appendSliceChecked(_.alloc, *_.buf, "C ")?;
        appendFmtNum(_.alloc, *_.buf, cx1)?;
        appendByteChecked(_.alloc, *_.buf, ' ')?;
        appendFmtNum(_.alloc, *_.buf, cy1)?;
        appendByteChecked(_.alloc, *_.buf, ' ')?;
        appendFmtNum(_.alloc, *_.buf, cx2)?;
        appendByteChecked(_.alloc, *_.buf, ' ')?;
        appendFmtNum(_.alloc, *_.buf, cy2)?;
        appendByteChecked(_.alloc, *_.buf, ' ')?;
        appendFmtNum(_.alloc, *_.buf, x)?;
        appendByteChecked(_.alloc, *_.buf, ' ')?;
        appendFmtNum(_.alloc, *_.buf, y)?;
    }

    /// Appends an absolute elliptical arc `A rx ry rot large sweep x y` command.
    pub fn arcTo(*_, rx: f64, ry: f64, x_axis_rotation: f64, large_arc: bool, sweep: bool, x: f64, y: f64): void ! SvgError {
        if (!isFinite(rx) || !isFinite(ry) || !isFinite(x_axis_rotation) || !isFinite(x) || !isFinite(y)) {
            return SvgError.InvalidValue;
        }
        _.appendSep()?;
        appendSliceChecked(_.alloc, *_.buf, "A ")?;
        appendFmtNum(_.alloc, *_.buf, rx)?;
        appendByteChecked(_.alloc, *_.buf, ' ')?;
        appendFmtNum(_.alloc, *_.buf, ry)?;
        appendByteChecked(_.alloc, *_.buf, ' ')?;
        appendFmtNum(_.alloc, *_.buf, x_axis_rotation)?;
        appendSliceChecked(_.alloc, *_.buf, if (large_arc) " 1 " else " 0 ")?;
        appendSliceChecked(_.alloc, *_.buf, if (sweep) "1 " else "0 ")?;
        appendFmtNum(_.alloc, *_.buf, x)?;
        appendByteChecked(_.alloc, *_.buf, ' ')?;
        appendFmtNum(_.alloc, *_.buf, y)?;
    }

    /// Appends a `Z` close-path command.
    pub fn close(*_): void ! SvgError {
        _.appendSep()?;
        appendByteChecked(_.alloc, *_.buf, 'Z')?;
    }

    /// Returns a read-only slice view of the built path data.
    pub fn slice(): string {
        return _.buf.items;
    }

    /// Transfers ownership of the built path data buffer to the caller.
    pub fn toOwnedSlice(*_): ([]u8) ! SvgError {
        return _.buf.toOwnedSlice(_.alloc) catch SvgError.OutOfMemory;
    }
}

/// Vector SVG 1.1 drawing canvas with group hierarchy and XSS-safe XML output.
pub struct SvgCanvas {
    alloc: std.mem.Allocator;
    width: f64;
    height: f64;
    background: string = "";
    body: std.ArrayList(u8) = .empty;
    group_depth: usize = 0;

    /// Initializes a new `SvgCanvas` with positive finite dimensions `width` x `height`.
    pub static fn init(alloc: std.mem.Allocator, width: f64, height: f64): SvgCanvas ! SvgError {
        if (!isFinite(width) || !isFinite(height) || width <= 0.0 || height <= 0.0) {
            return SvgError.InvalidDimensions;
        }
        return SvgCanvas{
            alloc: alloc,
            width: width,
            height: height,
            background: "",
            body: .empty,
            group_depth: 0,
        };
    }

    /// Releases internal canvas buffers.
    pub fn deinit(*_) {
        _.body.deinit(_.alloc);
        _.group_depth = 0;
    }

    /// Sets an optional solid background color (rendered as a full-canvas `<rect>`).
    pub fn setBackground(*_, color: string) {
        _.background = color;
    }

    /// Opens a `<g>` group element with optional `id` and `transform` attributes.
    pub fn beginGroup(*_, id: string, transform: string): void ! SvgError {
        appendSliceChecked(_.alloc, *_.body, "  <g")?;
        if (id.len > 0) {
            appendSliceChecked(_.alloc, *_.body, " id=\"")?;
            appendEscapedAttr(_.alloc, *_.body, id)?;
            appendByteChecked(_.alloc, *_.body, '"')?;
        }
        if (transform.len > 0) {
            appendSliceChecked(_.alloc, *_.body, " transform=\"")?;
            appendEscapedAttr(_.alloc, *_.body, transform)?;
            appendByteChecked(_.alloc, *_.body, '"')?;
        }
        appendSliceChecked(_.alloc, *_.body, ">\n")?;
        _.group_depth += 1;
    }

    /// Closes the most recently opened `<g>` group element.
    pub fn endGroup(*_): void ! SvgError {
        if (_.group_depth > 0) {
            appendSliceChecked(_.alloc, *_.body, "  </g>\n")?;
            _.group_depth -= 1;
        }
    }

    /// Draws a rectangle `<rect>` with optional corner radius `rx`.
    pub fn rect(*_, x: f64, y: f64, w: f64, h: f64, rx: f64, style: SvgStyle): void ! SvgError {
        if (!isFinite(x) || !isFinite(y) || !isFinite(w) || !isFinite(h) || !isFinite(rx)) {
            return SvgError.InvalidValue;
        }
        appendSliceChecked(_.alloc, *_.body, "  <rect x=\"")?;
        appendFmtNum(_.alloc, *_.body, x)?;
        appendSliceChecked(_.alloc, *_.body, "\" y=\"")?;
        appendFmtNum(_.alloc, *_.body, y)?;
        appendSliceChecked(_.alloc, *_.body, "\" width=\"")?;
        appendFmtNum(_.alloc, *_.body, w)?;
        appendSliceChecked(_.alloc, *_.body, "\" height=\"")?;
        appendFmtNum(_.alloc, *_.body, h)?;
        appendByteChecked(_.alloc, *_.body, '"')?;
        if (rx > 0.0) {
            appendSliceChecked(_.alloc, *_.body, " rx=\"")?;
            appendFmtNum(_.alloc, *_.body, rx)?;
            appendByteChecked(_.alloc, *_.body, '"')?;
        }
        appendStyleAttrs(_.alloc, *_.body, style, false)?;
        appendSliceChecked(_.alloc, *_.body, "/>\n")?;
    }

    /// Draws a circle `<circle>`.
    pub fn circle(*_, cx: f64, cy: f64, r: f64, style: SvgStyle): void ! SvgError {
        if (!isFinite(cx) || !isFinite(cy) || !isFinite(r) || r < 0.0) {
            return SvgError.InvalidValue;
        }
        appendSliceChecked(_.alloc, *_.body, "  <circle cx=\"")?;
        appendFmtNum(_.alloc, *_.body, cx)?;
        appendSliceChecked(_.alloc, *_.body, "\" cy=\"")?;
        appendFmtNum(_.alloc, *_.body, cy)?;
        appendSliceChecked(_.alloc, *_.body, "\" r=\"")?;
        appendFmtNum(_.alloc, *_.body, r)?;
        appendByteChecked(_.alloc, *_.body, '"')?;
        appendStyleAttrs(_.alloc, *_.body, style, false)?;
        appendSliceChecked(_.alloc, *_.body, "/>\n")?;
    }

    /// Draws a straight line `<line>`.
    pub fn line(*_, x1: f64, y1: f64, x2: f64, y2: f64, style: SvgStyle): void ! SvgError {
        if (!isFinite(x1) || !isFinite(y1) || !isFinite(x2) || !isFinite(y2)) {
            return SvgError.InvalidValue;
        }
        appendSliceChecked(_.alloc, *_.body, "  <line x1=\"")?;
        appendFmtNum(_.alloc, *_.body, x1)?;
        appendSliceChecked(_.alloc, *_.body, "\" y1=\"")?;
        appendFmtNum(_.alloc, *_.body, y1)?;
        appendSliceChecked(_.alloc, *_.body, "\" x2=\"")?;
        appendFmtNum(_.alloc, *_.body, x2)?;
        appendSliceChecked(_.alloc, *_.body, "\" y2=\"")?;
        appendFmtNum(_.alloc, *_.body, y2)?;
        appendByteChecked(_.alloc, *_.body, '"')?;
        appendStyleAttrs(_.alloc, *_.body, style, false)?;
        appendSliceChecked(_.alloc, *_.body, "/>\n")?;
    }

    /// Draws a `<polyline>` connecting `points`.
    pub fn polyline(*_, points: []const SvgPoint, style: SvgStyle): void ! SvgError {
        appendSliceChecked(_.alloc, *_.body, "  <polyline points=\"")?;
        let i: usize = 0;
        while (i < points.len) : (i += 1) {
            const pt = points[i];
            if (!isFinite(pt.x) || !isFinite(pt.y)) {
                return SvgError.InvalidValue;
            }
            if (i > 0) {
                appendByteChecked(_.alloc, *_.body, ' ')?;
            }
            appendFmtNum(_.alloc, *_.body, pt.x)?;
            appendByteChecked(_.alloc, *_.body, ',')?;
            appendFmtNum(_.alloc, *_.body, pt.y)?;
        }
        appendByteChecked(_.alloc, *_.body, '"')?;
        appendStyleAttrs(_.alloc, *_.body, style, false)?;
        appendSliceChecked(_.alloc, *_.body, "/>\n")?;
    }

    /// Draws a `<path>` with raw `d` command string (attribute-escaped).
    pub fn path(*_, d: string, style: SvgStyle): void ! SvgError {
        appendSliceChecked(_.alloc, *_.body, "  <path d=\"")?;
        appendEscapedAttr(_.alloc, *_.body, d)?;
        appendByteChecked(_.alloc, *_.body, '"')?;
        appendStyleAttrs(_.alloc, *_.body, style, false)?;
        appendSliceChecked(_.alloc, *_.body, "/>\n")?;
    }

    /// Draws a `<text>` element with strict XML escaping on `content` to prevent XSS.
    pub fn text(*_, x: f64, y: f64, content: string, style: SvgStyle): void ! SvgError {
        if (!isFinite(x) || !isFinite(y)) {
            return SvgError.InvalidValue;
        }
        appendSliceChecked(_.alloc, *_.body, "  <text x=\"")?;
        appendFmtNum(_.alloc, *_.body, x)?;
        appendSliceChecked(_.alloc, *_.body, "\" y=\"")?;
        appendFmtNum(_.alloc, *_.body, y)?;
        appendByteChecked(_.alloc, *_.body, '"')?;
        appendStyleAttrs(_.alloc, *_.body, style, true)?;
        appendByteChecked(_.alloc, *_.body, '>')?;
        appendEscapedText(_.alloc, *_.body, content)?;
        appendSliceChecked(_.alloc, *_.body, "</text>\n")?;
    }

    /// Renders a complete, valid SVG 1.1 document string owned by the caller.
    pub fn render(alloc: std.mem.Allocator): ([]u8) ! SvgError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(alloc);

        appendSliceChecked(alloc, *out, "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"")?;
        appendFmtNum(alloc, *out, _.width)?;
        appendSliceChecked(alloc, *out, "\" height=\"")?;
        appendFmtNum(alloc, *out, _.height)?;
        appendSliceChecked(alloc, *out, "\" viewBox=\"0 0 ")?;
        appendFmtNum(alloc, *out, _.width)?;
        appendByteChecked(alloc, *out, ' ')?;
        appendFmtNum(alloc, *out, _.height)?;
        appendSliceChecked(alloc, *out, "\">\n")?;

        if (_.background.len > 0) {
            appendSliceChecked(alloc, *out, "  <rect width=\"100%\" height=\"100%\" fill=\"")?;
            appendEscapedAttr(alloc, *out, _.background)?;
            appendSliceChecked(alloc, *out, "\"/>\n")?;
        }

        appendSliceChecked(alloc, *out, _.body.items)?;

        let rem = _.group_depth;
        while (rem > 0) : (rem -= 1) {
            appendSliceChecked(alloc, *out, "  </g>\n")?;
        }

        appendSliceChecked(alloc, *out, "</svg>\n")?;
        return out.toOwnedSlice(alloc) catch SvgError.OutOfMemory;
    }
}
