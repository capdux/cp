// @cap/wkt — Zero-heap GIS Well-Known Text (WKT) parser, formatter & bounding box.
// Clean-room MIT implementation modeled on crates.io `wkt` v0.11.

import { isSpace, isAlpha, isDigit, toLowerByte, strEq } from "cap:ascii";
import { writeStr, writeU64 } from "cap:fmt";

pub const WKT_POINT: u8 = 1;
pub const WKT_LINESTRING: u8 = 2;
pub const WKT_POLYGON: u8 = 3;
pub const WKT_MULTIPOINT: u8 = 4;
pub const WKT_MAX_COORDS: usize = 32;

/// 2D Cartesian / geographic coordinate pair `(x, y)`.
pub struct Coord2D {
    pub x: f64,
    pub y: f64,
}

/// Zero-heap WKT geometry holding up to 32 2D coordinates.
pub struct WktGeometry {
    pub kind: u8,
    pub is_empty: bool,
    pub coords: [32]Coord2D,
    pub coord_count: usize,
}

/// Axis-aligned 2D bounding box for a `WktGeometry`.
pub struct WktBbox {
    pub min_x: f64,
    pub min_y: f64,
    pub max_x: f64,
    pub max_y: f64,
    pub valid: bool,
}

fn wktZeroCoord(): Coord2D {
    return Coord2D { x: 0.0, y: 0.0 };
}

fn wktEmptyGeom(kind: u8): WktGeometry {
    return WktGeometry {
        kind: kind,
        is_empty: true,
        coords: [
            wktZeroCoord(), wktZeroCoord(), wktZeroCoord(), wktZeroCoord(),
            wktZeroCoord(), wktZeroCoord(), wktZeroCoord(), wktZeroCoord(),
            wktZeroCoord(), wktZeroCoord(), wktZeroCoord(), wktZeroCoord(),
            wktZeroCoord(), wktZeroCoord(), wktZeroCoord(), wktZeroCoord(),
            wktZeroCoord(), wktZeroCoord(), wktZeroCoord(), wktZeroCoord(),
            wktZeroCoord(), wktZeroCoord(), wktZeroCoord(), wktZeroCoord(),
            wktZeroCoord(), wktZeroCoord(), wktZeroCoord(), wktZeroCoord(),
            wktZeroCoord(), wktZeroCoord(), wktZeroCoord(), wktZeroCoord()
        ],
        coord_count: 0,
    };
}

fn wktSkipWs(s: string, pos: *usize) {
    let i: usize = pos.*;
    while (i < s.len && isSpace(s[i])) {
        i = i + 1;
    }
    pos.* = i;
}

fn wktReadWord(s: string, pos: *usize, out: []u8): usize {
    wktSkipWs(s, pos);
    let i: usize = pos.*;
    let n: usize = 0;
    while (i < s.len && isAlpha(s[i])) {
        if (n >= out.len) { return 0; }
        out[n] = toLowerByte(s[i]);
        n = n + 1;
        i = i + 1;
    }
    pos.* = i;
    return n;
}

fn wktParseNumber(s: string, pos: *usize, out: *f64): bool {
    wktSkipWs(s, pos);
    let i: usize = pos.*;
    if (i >= s.len) { return false; }

    let neg: bool = false;
    if (s[i] == 45) {
        neg = true;
        i = i + 1;
    } else if (s[i] == 43) {
        i = i + 1;
    }

    let val: f64 = 0.0;
    let has_digits: bool = false;
    while (i < s.len && isDigit(s[i])) {
        has_digits = true;
        val = val * 10.0 + ((s[i] - 48) as f64);
        i = i + 1;
    }

    if (i < s.len && s[i] == 46) {
        i = i + 1;
        let scale: f64 = 0.1;
        let has_frac: bool = false;
        while (i < s.len && isDigit(s[i])) {
            has_frac = true;
            has_digits = true;
            val = val + ((s[i] - 48) as f64) * scale;
            scale = scale * 0.1;
            i = i + 1;
        }
        if (!has_frac && !has_digits) { return false; }
    }

    if (!has_digits) { return false; }

    // Optional scientific exponent
    if (i < s.len && (s[i] == 101 || s[i] == 69)) {
        i = i + 1;
        let exp_neg: bool = false;
        if (i < s.len && s[i] == 45) {
            exp_neg = true;
            i = i + 1;
        } else if (i < s.len && s[i] == 43) {
            i = i + 1;
        }
        let exp_val: u32 = 0;
        let has_exp: bool = false;
        while (i < s.len && isDigit(s[i])) {
            has_exp = true;
            if (exp_val < 300) {
                exp_val = exp_val * 10 + ((s[i] - 48) as u32);
            }
            i = i + 1;
        }
        if (!has_exp) { return false; }
        let e: u32 = 0;
        while (e < exp_val) {
            if (exp_neg) { val = val * 0.1; } else { val = val * 10.0; }
            e = e + 1;
        }
    }

    pos.* = i;
    out.* = if (neg) { -val } else { val };
    return true;
}

fn wktParseCoord(s: string, pos: *usize, out: *Coord2D): bool {
    let x: f64 = 0.0;
    let y: f64 = 0.0;
    if (!wktParseNumber(s, pos, &x)) { return false; }
    let after_x: usize = pos.*;
    wktSkipWs(s, pos);
    if (pos.* == after_x) { return false; }
    if (!wktParseNumber(s, pos, &y)) { return false; }
    out.* = Coord2D { x: x, y: y };
    return true;
}

fn wktConsumeChar(s: string, pos: *usize, ch: u8): bool {
    wktSkipWs(s, pos);
    if (pos.* >= s.len || s[pos.*] != ch) { return false; }
    pos.* = pos.* + 1;
    return true;
}

fn wktWriteF64(out: []u8, pos: usize, val: f64): usize {
    let p: usize = pos;
    let v: f64 = val;
    if (v == 0.0) {
        return writeStr("0", out, p);
    }
    if (v < 0.0) {
        p = writeStr("-", out, p);
        v = -v;
    }
    let scaled: u64 = ((v * 1000000.0) + 0.5) as u64;
    let ipart: u64 = scaled / 1000000;
    let fpart: u64 = scaled % 1000000;
    p = writeU64(ipart, out, p);
    if (fpart > 0) {
        p = writeStr(".", out, p);
        let fbuf: [6]u8 = undefined;
        let rem: u64 = fpart;
        let idx: usize = 6;
        while (idx > 0) {
            idx = idx - 1;
            fbuf[idx] = (48 + (rem % 10)) as u8;
            rem = rem / 10;
        }
        let trim_len: usize = 6;
        while (trim_len > 0 && fbuf[trim_len - 1] == 48) {
            trim_len = trim_len - 1;
        }
        p = writeStr(fbuf[0..trim_len], out, p);
    }
    return p;
}

fn wktWriteCoord(out: []u8, pos: usize, c: Coord2D): usize {
    let p: usize = wktWriteF64(out, pos, c.x);
    p = writeStr(" ", out, p);
    p = wktWriteF64(out, p, c.y);
    return p;
}

/// Parses a WKT geometry string (`POINT`, `LINESTRING`, `POLYGON`, `MULTIPOINT`, plus `EMPTY`),
/// returning `null` on unknown geometry types, malformed brackets, empty coordinate lists, or trailing garbage.
pub fn parseWkt(input: string): WktGeometry? {
    let pos: usize = 0;
    let wbuf: [16]u8 = undefined;
    let wlen: usize = wktReadWord(input, &pos, wbuf[0..16]);
    if (wlen == 0) { return null; }

    let kind: u8 = 0;
    if (strEq(wbuf[0..wlen], "point")) { kind = WKT_POINT; }
    else if (strEq(wbuf[0..wlen], "linestring")) { kind = WKT_LINESTRING; }
    else if (strEq(wbuf[0..wlen], "polygon")) { kind = WKT_POLYGON; }
    else if (strEq(wbuf[0..wlen], "multipoint")) { kind = WKT_MULTIPOINT; }
    else { return null; }

    wktSkipWs(input, &pos);
    if (pos >= input.len) { return null; }

    // Check for EMPTY
    if (isAlpha(input[pos])) {
        let ebuf: [16]u8 = undefined;
        let elen: usize = wktReadWord(input, &pos, ebuf[0..16]);
        if (!strEq(ebuf[0..elen], "empty")) { return null; }
        wktSkipWs(input, &pos);
        if (pos != input.len) { return null; }
        return wktEmptyGeom(kind);
    }

    let geom: WktGeometry = wktEmptyGeom(kind);
    geom.is_empty = false;

    if (kind == WKT_POINT) {
        if (!wktConsumeChar(input, &pos, 40)) { return null; }
        let pt: Coord2D = wktZeroCoord();
        if (!wktParseCoord(input, &pos, &pt)) { return null; }
        if (!wktConsumeChar(input, &pos, 41)) { return null; }
        geom.coords[0] = pt;
        geom.coord_count = 1;
    } else if (kind == WKT_LINESTRING) {
        if (!wktConsumeChar(input, &pos, 40)) { return null; }
        let done: bool = false;
        while (!done) {
            if (geom.coord_count >= WKT_MAX_COORDS) { return null; }
            let pt: Coord2D = wktZeroCoord();
            if (!wktParseCoord(input, &pos, &pt)) { return null; }
            geom.coords[geom.coord_count] = pt;
            geom.coord_count = geom.coord_count + 1;
            wktSkipWs(input, &pos);
            if (pos < input.len && input[pos] == 44) {
                pos = pos + 1;
            } else {
                done = true;
            }
        }
        if (geom.coord_count == 0) { return null; }
        if (!wktConsumeChar(input, &pos, 41)) { return null; }
    } else if (kind == WKT_POLYGON) {
        if (!wktConsumeChar(input, &pos, 40)) { return null; }
        if (!wktConsumeChar(input, &pos, 40)) { return null; }
        let done: bool = false;
        while (!done) {
            if (geom.coord_count >= WKT_MAX_COORDS) { return null; }
            let pt: Coord2D = wktZeroCoord();
            if (!wktParseCoord(input, &pos, &pt)) { return null; }
            geom.coords[geom.coord_count] = pt;
            geom.coord_count = geom.coord_count + 1;
            wktSkipWs(input, &pos);
            if (pos < input.len && input[pos] == 44) {
                pos = pos + 1;
            } else {
                done = true;
            }
        }
        if (geom.coord_count < 3) { return null; }
        if (!wktConsumeChar(input, &pos, 41)) { return null; }
        if (!wktConsumeChar(input, &pos, 41)) { return null; }
    } else if (kind == WKT_MULTIPOINT) {
        if (!wktConsumeChar(input, &pos, 40)) { return null; }
        wktSkipWs(input, &pos);
        if (pos >= input.len) { return null; }
        let nested: bool = input[pos] == 40;
        let done: bool = false;
        while (!done) {
            if (geom.coord_count >= WKT_MAX_COORDS) { return null; }
            if (nested) {
                if (!wktConsumeChar(input, &pos, 40)) { return null; }
            }
            let pt: Coord2D = wktZeroCoord();
            if (!wktParseCoord(input, &pos, &pt)) { return null; }
            if (nested) {
                if (!wktConsumeChar(input, &pos, 41)) { return null; }
            }
            geom.coords[geom.coord_count] = pt;
            geom.coord_count = geom.coord_count + 1;
            wktSkipWs(input, &pos);
            if (pos < input.len && input[pos] == 44) {
                pos = pos + 1;
            } else {
                done = true;
            }
        }
        if (geom.coord_count == 0) { return null; }
        if (!wktConsumeChar(input, &pos, 41)) { return null; }
    }

    wktSkipWs(input, &pos);
    if (pos != input.len) { return null; }
    return geom;
}

/// Formats `g` into standard WKT text in `out`, returning the number of bytes written.
pub fn formatWkt(g: WktGeometry, out: []u8): usize {
    let p: usize = 0;
    if (g.kind == WKT_POINT) { p = writeStr("POINT", out, p); }
    else if (g.kind == WKT_LINESTRING) { p = writeStr("LINESTRING", out, p); }
    else if (g.kind == WKT_POLYGON) { p = writeStr("POLYGON", out, p); }
    else if (g.kind == WKT_MULTIPOINT) { p = writeStr("MULTIPOINT", out, p); }
    else { return 0; }

    if (g.is_empty || g.coord_count == 0) {
        return writeStr(" EMPTY", out, p);
    }

    if (g.kind == WKT_POINT) {
        p = writeStr(" (", out, p);
        p = wktWriteCoord(out, p, g.coords[0]);
        p = writeStr(")", out, p);
        return p;
    }

    if (g.kind == WKT_LINESTRING) {
        p = writeStr(" (", out, p);
        let i: usize = 0;
        while (i < g.coord_count) {
            if (i > 0) { p = writeStr(", ", out, p); }
            p = wktWriteCoord(out, p, g.coords[i]);
            i = i + 1;
        }
        p = writeStr(")", out, p);
        return p;
    }

    if (g.kind == WKT_POLYGON) {
        p = writeStr(" ((", out, p);
        let i: usize = 0;
        while (i < g.coord_count) {
            if (i > 0) { p = writeStr(", ", out, p); }
            p = wktWriteCoord(out, p, g.coords[i]);
            i = i + 1;
        }
        p = writeStr("))", out, p);
        return p;
    }

    // WKT_MULTIPOINT
    p = writeStr(" (", out, p);
    let i: usize = 0;
    while (i < g.coord_count) {
        if (i > 0) { p = writeStr(", ", out, p); }
        p = writeStr("(", out, p);
        p = wktWriteCoord(out, p, g.coords[i]);
        p = writeStr(")", out, p);
        i = i + 1;
    }
    p = writeStr(")", out, p);
    return p;
}

/// Computes the axis-aligned bounding box of `g`. Returns `valid = false` for empty geometries.
pub fn bboxWkt(g: WktGeometry): WktBbox {
    if (g.is_empty || g.coord_count == 0) {
        return WktBbox { min_x: 0.0, min_y: 0.0, max_x: 0.0, max_y: 0.0, valid: false };
    }
    let min_x: f64 = g.coords[0].x;
    let min_y: f64 = g.coords[0].y;
    let max_x: f64 = g.coords[0].x;
    let max_y: f64 = g.coords[0].y;
    let i: usize = 1;
    while (i < g.coord_count) {
        let c: Coord2D = g.coords[i];
        if (c.x < min_x) { min_x = c.x; }
        if (c.y < min_y) { min_y = c.y; }
        if (c.x > max_x) { max_x = c.x; }
        if (c.y > max_y) { max_y = c.y; }
        i = i + 1;
    }
    return WktBbox {
        min_x: min_x,
        min_y: min_y,
        max_x: max_x,
        max_y: max_y,
        valid: true,
    };
}
