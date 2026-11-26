// @cap/plotters-svg — Zero-heap pure CP SVG vector drawing backend
// Ported from crates.io:plotters-svg (https://github.com/plotters-rs/plotters)

pub struct SvgColor {
    r: u8,
    g: u8,
    b: u8,

    static fn rgb(r: u8, g: u8, b: u8): SvgColor {
        return SvgColor{ r: r, g: g, b: b };
    }

    static fn black(): SvgColor {
        return SvgColor{ r: 0, g: 0, b: 0 };
    }

    static fn white(): SvgColor {
        return SvgColor{ r: 255, g: 255, b: 255 };
    }

    static fn red(): SvgColor {
        return SvgColor{ r: 255, g: 0, b: 0 };
    }

    static fn green(): SvgColor {
        return SvgColor{ r: 0, g: 128, b: 0 };
    }

    static fn blue(): SvgColor {
        return SvgColor{ r: 0, g: 0, b: 255 };
    }
}

fn psvgWriteByte(buf: []u8, pos: usize, b: u8): usize {
    if (pos < buf.len) {
        buf[pos] = b;
        return pos + 1;
    }
    return pos;
}

fn psvgWriteStr(buf: []u8, pos: usize, s: string): usize {
    let w: usize = pos;
    let i: usize = 0;
    while (i < s.len && w < buf.len) {
        buf[w] = s[i];
        w += 1;
        i += 1;
    }
    return w;
}

fn psvgWriteU32(buf: []u8, pos: usize, val: u32): usize {
    if (val == 0) {
        return psvgWriteByte(buf, pos, 0x30);
    }
    const z: u8 = 0;
    let digits: [10]u8 = [z, z, z, z, z, z, z, z, z, z];
    let n: usize = 0;
    let v: u32 = val;
    while (v > 0 && n < 10) {
        digits[n] = (0x30 + (v % 10)) as u8;
        v = v / 10;
        n += 1;
    }
    let w: usize = pos;
    while (n > 0) {
        n -= 1;
        w = psvgWriteByte(buf, w, digits[n]);
    }
    return w;
}

fn psvgWriteI32(buf: []u8, pos: usize, val: i32): usize {
    if (val < 0) {
        const w1 = psvgWriteByte(buf, pos, 0x2D); // '-'
        const mag: u32 = ((0 as i64) - (val as i64)) as u32;
        return psvgWriteU32(buf, w1, mag);
    }
    return psvgWriteU32(buf, pos, val as u32);
}

fn psvgWriteRgb(buf: []u8, pos: usize, c: SvgColor): usize {
    let w: usize = psvgWriteStr(buf, pos, "rgb(");
    w = psvgWriteU32(buf, w, c.r as u32);
    w = psvgWriteByte(buf, w, 0x2C); // ','
    w = psvgWriteU32(buf, w, c.g as u32);
    w = psvgWriteByte(buf, w, 0x2C); // ','
    w = psvgWriteU32(buf, w, c.b as u32);
    w = psvgWriteByte(buf, w, 0x29); // ')'
    return w;
}

fn psvgWriteXmlEscaped(buf: []u8, pos: usize, text: string): usize {
    let w: usize = pos;
    let i: usize = 0;
    while (i < text.len) {
        const ch: u8 = text[i];
        if (ch == 0x26) {
            // '&'
            w = psvgWriteStr(buf, w, "&amp;");
        } else if (ch == 0x3C) {
            // '<'
            w = psvgWriteStr(buf, w, "&lt;");
        } else if (ch == 0x3E) {
            // '>'
            w = psvgWriteStr(buf, w, "&gt;");
        } else if (ch == 0x22) {
            // '"'
            w = psvgWriteStr(buf, w, "&quot;");
        } else if (ch == 0x27) {
            // '\''
            w = psvgWriteStr(buf, w, "&#39;");
        } else {
            w = psvgWriteByte(buf, w, ch);
        }
        i += 1;
    }
    return w;
}

/// Zero-heap SVG vector canvas writing directly into a caller-supplied []u8 buffer.
pub struct SvgCanvas {
    buf: []u8,
    pos: usize,
    overflowed: bool,

    static fn new(buf: []u8): SvgCanvas {
        return SvgCanvas{
            buf: buf,
            pos: 0,
            overflowed: false,
        };
    }

    fn written(): usize {
        return _.pos;
    }

    fn hasOverflow(): bool {
        return _.overflowed;
    }

    fn slice(): string {
        return _.buf[0.._.pos];
    }

    fn begin(*_, width: u32, height: u32): bool {
        _.pos = 0;
        _.overflowed = false;
        let w: usize = 0;
        w = psvgWriteStr(_.buf, w, "<svg width=\"");
        w = psvgWriteU32(_.buf, w, width);
        w = psvgWriteStr(_.buf, w, "\" height=\"");
        w = psvgWriteU32(_.buf, w, height);
        w = psvgWriteStr(_.buf, w, "\" viewBox=\"0 0 ");
        w = psvgWriteU32(_.buf, w, width);
        w = psvgWriteByte(_.buf, w, 0x20);
        w = psvgWriteU32(_.buf, w, height);
        w = psvgWriteStr(_.buf, w, "\" xmlns=\"http://www.w3.org/2000/svg\">\n");
        _.pos = w;
        if (_.pos >= _.buf.len) {
            _.overflowed = true;
            return false;
        }
        return true;
    }

    fn end(*_): usize {
        const before = _.pos;
        _.pos = psvgWriteStr(_.buf, _.pos, "</svg>\n");
        if (_.pos - before < 7) {
            _.overflowed = true;
        }
        return _.pos;
    }

    fn drawRect(*_, x: i32, y: i32, w: u32, h: u32, fill: SvgColor, stroke_width: u32): bool {
        let p: usize = _.pos;
        p = psvgWriteStr(_.buf, p, "<rect x=\"");
        p = psvgWriteI32(_.buf, p, x);
        p = psvgWriteStr(_.buf, p, "\" y=\"");
        p = psvgWriteI32(_.buf, p, y);
        p = psvgWriteStr(_.buf, p, "\" width=\"");
        p = psvgWriteU32(_.buf, p, w);
        p = psvgWriteStr(_.buf, p, "\" height=\"");
        p = psvgWriteU32(_.buf, p, h);
        p = psvgWriteStr(_.buf, p, "\" fill=\"");
        p = psvgWriteRgb(_.buf, p, fill);
        p = psvgWriteByte(_.buf, p, 0x22);
        if (stroke_width > 0) {
            p = psvgWriteStr(_.buf, p, " stroke=\"rgb(0,0,0)\" stroke-width=\"");
            p = psvgWriteU32(_.buf, p, stroke_width);
            p = psvgWriteByte(_.buf, p, 0x22);
        }
        p = psvgWriteStr(_.buf, p, "/>\n");
        _.pos = p;
        if (_.pos >= _.buf.len) {
            _.overflowed = true;
            return false;
        }
        return true;
    }

    fn drawLine(*_, x1: i32, y1: i32, x2: i32, y2: i32, stroke: SvgColor, stroke_width: u32): bool {
        let p: usize = _.pos;
        p = psvgWriteStr(_.buf, p, "<line x1=\"");
        p = psvgWriteI32(_.buf, p, x1);
        p = psvgWriteStr(_.buf, p, "\" y1=\"");
        p = psvgWriteI32(_.buf, p, y1);
        p = psvgWriteStr(_.buf, p, "\" x2=\"");
        p = psvgWriteI32(_.buf, p, x2);
        p = psvgWriteStr(_.buf, p, "\" y2=\"");
        p = psvgWriteI32(_.buf, p, y2);
        p = psvgWriteStr(_.buf, p, "\" stroke=\"");
        p = psvgWriteRgb(_.buf, p, stroke);
        p = psvgWriteStr(_.buf, p, "\" stroke-width=\"");
        p = psvgWriteU32(_.buf, p, stroke_width);
        p = psvgWriteStr(_.buf, p, "\"/>\n");
        _.pos = p;
        if (_.pos >= _.buf.len) {
            _.overflowed = true;
            return false;
        }
        return true;
    }

    fn drawCircle(*_, cx: i32, cy: i32, r: u32, fill: SvgColor): bool {
        let p: usize = _.pos;
        p = psvgWriteStr(_.buf, p, "<circle cx=\"");
        p = psvgWriteI32(_.buf, p, cx);
        p = psvgWriteStr(_.buf, p, "\" cy=\"");
        p = psvgWriteI32(_.buf, p, cy);
        p = psvgWriteStr(_.buf, p, "\" r=\"");
        p = psvgWriteU32(_.buf, p, r);
        p = psvgWriteStr(_.buf, p, "\" fill=\"");
        p = psvgWriteRgb(_.buf, p, fill);
        p = psvgWriteStr(_.buf, p, "\"/>\n");
        _.pos = p;
        if (_.pos >= _.buf.len) {
            _.overflowed = true;
            return false;
        }
        return true;
    }

    fn drawPolyline(*_, xs: []const i32, ys: []const i32, stroke: SvgColor, stroke_width: u32): bool {
        let n: usize = xs.len;
        if (ys.len < n) n = ys.len;
        let p: usize = _.pos;
        p = psvgWriteStr(_.buf, p, "<polyline fill=\"none\" stroke=\"");
        p = psvgWriteRgb(_.buf, p, stroke);
        p = psvgWriteStr(_.buf, p, "\" stroke-width=\"");
        p = psvgWriteU32(_.buf, p, stroke_width);
        p = psvgWriteStr(_.buf, p, "\" points=\"");
        let i: usize = 0;
        while (i < n) {
            if (i > 0) {
                p = psvgWriteByte(_.buf, p, 0x20);
            }
            p = psvgWriteI32(_.buf, p, xs[i]);
            p = psvgWriteByte(_.buf, p, 0x2C);
            p = psvgWriteI32(_.buf, p, ys[i]);
            i += 1;
        }
        p = psvgWriteStr(_.buf, p, "\"/>\n");
        _.pos = p;
        if (_.pos >= _.buf.len) {
            _.overflowed = true;
            return false;
        }
        return true;
    }

    fn drawText(*_, x: i32, y: i32, text: string, font_size: u32, fill: SvgColor): bool {
        let p: usize = _.pos;
        p = psvgWriteStr(_.buf, p, "<text x=\"");
        p = psvgWriteI32(_.buf, p, x);
        p = psvgWriteStr(_.buf, p, "\" y=\"");
        p = psvgWriteI32(_.buf, p, y);
        p = psvgWriteStr(_.buf, p, "\" font-size=\"");
        p = psvgWriteU32(_.buf, p, font_size);
        p = psvgWriteStr(_.buf, p, "\" fill=\"");
        p = psvgWriteRgb(_.buf, p, fill);
        p = psvgWriteStr(_.buf, p, "\">");
        p = psvgWriteXmlEscaped(_.buf, p, text);
        p = psvgWriteStr(_.buf, p, "</text>\n");
        _.pos = p;
        if (_.pos >= _.buf.len) {
            _.overflowed = true;
            return false;
        }
        return true;
    }
}
