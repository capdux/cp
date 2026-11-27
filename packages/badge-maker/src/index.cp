// @cap/svg-badge — Zero-heap Shields.io-style SVG status badge generator
// Clean-room MIT implementation in pure CP (replaces npm `badge-maker` / `gh-badges`).

pub enum BadgeStyle {
    Flat,
    FlatSquare,
    Plastic,
}

pub struct BadgeOptions {
    pub label: string,
    pub message: string,
    pub labelColor: string,
    pub color: string,
    pub style: BadgeStyle,
}

pub fn defaultBadge(label: string, message: string, color: string) -> BadgeOptions {
    return BadgeOptions {
        label: label,
        message: message,
        labelColor: "#555",
        color: color,
        style: BadgeStyle.Flat,
    };
}

/// Resolve Shields.io named badge colors to hex codes; pass through `#hex` values unchanged.
pub fn resolveBadgeColor(name: string) -> string {
    if (name == "brightgreen") { return "#4c1"; }
    if (name == "green") { return "#97ca00"; }
    if (name == "yellowgreen") { return "#a4a61d"; }
    if (name == "yellow") { return "#dfb317"; }
    if (name == "orange") { return "#fe7d37"; }
    if (name == "red") { return "#e05d44"; }
    if (name == "blue") { return "#007ec6"; }
    if (name == "lightgrey" || name == "lightgray") { return "#9f9f9f"; }
    if (name == "grey" || name == "gray") { return "#555"; }
    if (name == "blueviolet") { return "#8a2be2"; }
    if (name == "success") { return "#4c1"; }
    if (name == "important") { return "#fe7d37"; }
    if (name == "critical") { return "#e05d44"; }
    if (name == "informational") { return "#007ec6"; }
    if (name == "inactive") { return "#9f9f9f"; }
    if (name.len == 0) { return "#007ec6"; }
    return name;
}

/// Approximate Verdana 11px text width in pixels (integer).
pub fn estimateTextWidth(s: string) -> usize {
    let w: usize = 0;
    let i: usize = 0;
    while (i < s.len) {
        const c: u8 = s[i];
        if (c == 'i' || c == 'l' || c == 'I' || c == '.' || c == ',' || c == ':' || c == ';' || c == '!' || c == '|') {
            w += 4;
        } else if (c == 'f' || c == 'j' || c == 'r' || c == 't' || c == '(' || c == ')' || c == ' ') {
            w += 5;
        } else if (c == 'm' || c == 'w' || c == 'M' || c == 'W' || c == '@' || c == '%') {
            w += 10;
        } else if (c >= 'A' && c <= 'Z') {
            w += 8;
        } else if (c >= 0x80) {
            // Skip UTF-8 continuation bytes, count multi-byte glyph as ~11px
            if ((c & 0xC0) != 0x80) {
                w += 11;
            }
        } else {
            w += 7;
        }
        i += 1;
    }
    return w;
}

struct BufWriter {
    pub out: []u8,
    pub pos: usize,

    pub fn writeStr(*_, s: string) {
        let i: usize = 0;
        while (i < s.len && _.pos < _.out.len) {
            _.out[_.pos] = s[i];
            _.pos += 1;
            i += 1;
        }
    }

    pub fn writeXmlEscaped(*_, s: string) {
        let i: usize = 0;
        while (i < s.len) {
            const c: u8 = s[i];
            if (c == '&') {
                _.writeStr("&amp;");
            } else if (c == '<') {
                _.writeStr("&lt;");
            } else if (c == '>') {
                _.writeStr("&gt;");
            } else if (c == '"') {
                _.writeStr("&quot;");
            } else if (c == '\'') {
                _.writeStr("&#39;");
            } else if (_.pos < _.out.len) {
                _.out[_.pos] = c;
                _.pos += 1;
            }
            i += 1;
        }
    }

    pub fn writeUsize(*_, val: usize) {
        if (val == 0) {
            _.writeStr("0");
            return;
        }
        let tmp: [24]u8 = undefined;
        let n: usize = 0;
        let v: usize = val;
        while (v > 0 && n < 24) {
            const d: u8 = (v % 10) as u8;
            tmp[n] = ('0' as u8) + d;
            v = v / 10;
            n += 1;
        }
        while (n > 0) {
            n -= 1;
            if (_.pos < _.out.len) {
                _.out[_.pos] = tmp[n];
                _.pos += 1;
            }
        }
    }
}

/// Render a Shields.io-compatible SVG badge into `out` and return the written slice.
pub fn renderBadge(opts: BadgeOptions, out: []u8) -> string {
    const labelCol: string = resolveBadgeColor(opts.labelColor);
    const msgCol: string = resolveBadgeColor(opts.color);

    const hasLabel: bool = opts.label.len > 0;
    const labelTextW: usize = estimateTextWidth(opts.label);
    const msgTextW: usize = estimateTextWidth(opts.message);
    const labelW: usize = if (hasLabel) labelTextW + 12 else (0 as usize);
    const msgW: usize = msgTextW + 12;
    const totalW: usize = labelW + msgW;
    const height: usize = 20;

    let rx: string = "3";
    let isSquare: bool = false;
    let isPlastic: bool = false;
    switch (opts.style) {
        BadgeStyle.Flat => { rx = "3"; },
        BadgeStyle.FlatSquare => { rx = "0"; isSquare = true; },
        BadgeStyle.Plastic => { rx = "4"; isPlastic = true; },
    }

    let w: BufWriter = BufWriter { out: out, pos: 0 };

    w.writeStr("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"");
    w.writeUsize(totalW);
    w.writeStr("\" height=\"");
    w.writeUsize(height);
    w.writeStr("\" role=\"img\" aria-label=\"");
    if (hasLabel) {
        w.writeXmlEscaped(opts.label);
        w.writeStr(": ");
    }
    w.writeXmlEscaped(opts.message);
    w.writeStr("\"><title>");
    if (hasLabel) {
        w.writeXmlEscaped(opts.label);
        w.writeStr(": ");
    }
    w.writeXmlEscaped(opts.message);
    w.writeStr("</title>");

    if (!isSquare) {
        if (isPlastic) {
            w.writeStr("<linearGradient id=\"s\" x2=\"0\" y2=\"100%\"><stop offset=\"0\" stop-color=\"#fff\" stop-opacity=\".7\"/><stop offset=\".1\" stop-color=\"#aaa\" stop-opacity=\".1\"/><stop offset=\".9\" stop-color=\"#000\" stop-opacity=\".3\"/><stop offset=\"1\" stop-color=\"#000\" stop-opacity=\".5\"/></linearGradient>");
        } else {
            w.writeStr("<linearGradient id=\"s\" x2=\"0\" y2=\"100%\"><stop offset=\"0\" stop-color=\"#bbb\" stop-opacity=\".1\"/><stop offset=\"1\" stop-opacity=\".1\"/></linearGradient>");
        }
        w.writeStr("<clipPath id=\"r\"><rect width=\"");
        w.writeUsize(totalW);
        w.writeStr("\" height=\"20\" rx=\"");
        w.writeStr(rx);
        w.writeStr("\" fill=\"#fff\"/></clipPath><g clip-path=\"url(#r)\">");
    } else {
        w.writeStr("<g shape-rendering=\"crispEdges\">");
    }

    if (hasLabel) {
        w.writeStr("<rect width=\"");
        w.writeUsize(labelW);
        w.writeStr("\" height=\"20\" fill=\"");
        w.writeXmlEscaped(labelCol);
        w.writeStr("\"/>");
    }

    w.writeStr("<rect x=\"");
    w.writeUsize(labelW);
    w.writeStr("\" width=\"");
    w.writeUsize(msgW);
    w.writeStr("\" height=\"20\" fill=\"");
    w.writeXmlEscaped(msgCol);
    w.writeStr("\"/>");

    if (!isSquare) {
        w.writeStr("<rect width=\"");
        w.writeUsize(totalW);
        w.writeStr("\" height=\"20\" fill=\"url(#s)\"/>");
    }

    w.writeStr("</g><g fill=\"#fff\" text-anchor=\"middle\" font-family=\"Verdana,Geneva,DejaVu Sans,sans-serif\" font-size=\"11\">");

    if (hasLabel) {
        const lx: usize = labelW / 2;
        w.writeStr("<text x=\"");
        w.writeUsize(lx);
        w.writeStr("\" y=\"15\" fill=\"#010101\" fill-opacity=\".3\">");
        w.writeXmlEscaped(opts.label);
        w.writeStr("</text><text x=\"");
        w.writeUsize(lx);
        w.writeStr("\" y=\"14\">");
        w.writeXmlEscaped(opts.label);
        w.writeStr("</text>");
    }

    const mx: usize = labelW + (msgW / 2);
    w.writeStr("<text x=\"");
    w.writeUsize(mx);
    w.writeStr("\" y=\"15\" fill=\"#010101\" fill-opacity=\".3\">");
    w.writeXmlEscaped(opts.message);
    w.writeStr("</text><text x=\"");
    w.writeUsize(mx);
    w.writeStr("\" y=\"14\">");
    w.writeXmlEscaped(opts.message);
    w.writeStr("</text></g></svg>");

    return out[0..w.pos];
}

/// Convenience helper for generating a flat status badge.
pub fn makeBadge(label: string, message: string, color: string, out: []u8) -> string {
    return renderBadge(defaultBadge(label, message, color), out);
}
