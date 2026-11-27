// @cap/svg-badge — Zero-heap Shields.io-style SVG status badge generator
// Clean-room MIT implementation in pure CP (replaces npm `badge-maker` / `gh-badges`).

import { writeStr, writeU64 } from "cap:fmt";

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

struct BadgeWriter {
    pub out: []u8,
    pub pos: usize,

    pub fn putStr(*_, s: string) {
        _.pos = writeStr(s, _.out, _.pos);
    }

    pub fn writeXmlEscaped(*_, s: string) {
        let i: usize = 0;
        while (i < s.len) {
            const c: u8 = s[i];
            if (c == '&') {
                _.putStr("&amp;");
            } else if (c == '<') {
                _.putStr("&lt;");
            } else if (c == '>') {
                _.putStr("&gt;");
            } else if (c == '"') {
                _.putStr("&quot;");
            } else if (c == '\'') {
                _.putStr("&#39;");
            } else if (_.pos < _.out.len) {
                _.out[_.pos] = c;
                _.pos += 1;
            }
            i += 1;
        }
    }

    pub fn writeUsize(*_, val: usize) {
        _.pos = writeU64(val as u64, _.out, _.pos);
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

    let w: BadgeWriter = BadgeWriter { out: out, pos: 0 };

    w.putStr("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"");
    w.writeUsize(totalW);
    w.putStr("\" height=\"");
    w.writeUsize(height);
    w.putStr("\" role=\"img\" aria-label=\"");
    if (hasLabel) {
        w.writeXmlEscaped(opts.label);
        w.putStr(": ");
    }
    w.writeXmlEscaped(opts.message);
    w.putStr("\"><title>");
    if (hasLabel) {
        w.writeXmlEscaped(opts.label);
        w.putStr(": ");
    }
    w.writeXmlEscaped(opts.message);
    w.putStr("</title>");

    if (!isSquare) {
        if (isPlastic) {
            w.putStr("<linearGradient id=\"s\" x2=\"0\" y2=\"100%\"><stop offset=\"0\" stop-color=\"#fff\" stop-opacity=\".7\"/><stop offset=\".1\" stop-color=\"#aaa\" stop-opacity=\".1\"/><stop offset=\".9\" stop-color=\"#000\" stop-opacity=\".3\"/><stop offset=\"1\" stop-color=\"#000\" stop-opacity=\".5\"/></linearGradient>");
        } else {
            w.putStr("<linearGradient id=\"s\" x2=\"0\" y2=\"100%\"><stop offset=\"0\" stop-color=\"#bbb\" stop-opacity=\".1\"/><stop offset=\"1\" stop-opacity=\".1\"/></linearGradient>");
        }
        w.putStr("<clipPath id=\"r\"><rect width=\"");
        w.writeUsize(totalW);
        w.putStr("\" height=\"20\" rx=\"");
        w.putStr(rx);
        w.putStr("\" fill=\"#fff\"/></clipPath><g clip-path=\"url(#r)\">");
    } else {
        w.putStr("<g shape-rendering=\"crispEdges\">");
    }

    if (hasLabel) {
        w.putStr("<rect width=\"");
        w.writeUsize(labelW);
        w.putStr("\" height=\"20\" fill=\"");
        w.writeXmlEscaped(labelCol);
        w.putStr("\"/>");
    }

    w.putStr("<rect x=\"");
    w.writeUsize(labelW);
    w.putStr("\" width=\"");
    w.writeUsize(msgW);
    w.putStr("\" height=\"20\" fill=\"");
    w.writeXmlEscaped(msgCol);
    w.putStr("\"/>");

    if (!isSquare) {
        w.putStr("<rect width=\"");
        w.writeUsize(totalW);
        w.putStr("\" height=\"20\" fill=\"url(#s)\"/>");
    }

    w.putStr("</g><g fill=\"#fff\" text-anchor=\"middle\" font-family=\"Verdana,Geneva,DejaVu Sans,sans-serif\" font-size=\"11\">");

    if (hasLabel) {
        const lx: usize = labelW / 2;
        w.putStr("<text x=\"");
        w.writeUsize(lx);
        w.putStr("\" y=\"15\" fill=\"#010101\" fill-opacity=\".3\">");
        w.writeXmlEscaped(opts.label);
        w.putStr("</text><text x=\"");
        w.writeUsize(lx);
        w.putStr("\" y=\"14\">");
        w.writeXmlEscaped(opts.label);
        w.putStr("</text>");
    }

    const mx: usize = labelW + (msgW / 2);
    w.putStr("<text x=\"");
    w.writeUsize(mx);
    w.putStr("\" y=\"15\" fill=\"#010101\" fill-opacity=\".3\">");
    w.writeXmlEscaped(opts.message);
    w.putStr("</text><text x=\"");
    w.writeUsize(mx);
    w.putStr("\" y=\"14\">");
    w.writeXmlEscaped(opts.message);
    w.putStr("</text></g></svg>");

    return out[0..w.pos];
}

/// Convenience helper for generating a flat status badge.
pub fn makeBadge(label: string, message: string, color: string, out: []u8) -> string {
    return renderBadge(defaultBadge(label, message, color), out);
}
