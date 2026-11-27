// @cap/geojson — Zero-heap RFC 7946 GeoJSON parser, formatter & WKT interop.
// Clean-room MIT implementation modeled on crates.io `geojson` v0.24,
// reusing `@cap/wkt` types, parser, formatter, and bounding-box calculator.

import {
    WKT_POINT,
    WKT_LINESTRING,
    WKT_POLYGON,
    WKT_MAX_COORDS,
    Coord2D,
    WktGeometry,
    WktBbox,
    parseWkt,
    formatWkt,
    bboxWkt
} from "../../wkt/src/index.cp";

/// Lightweight GeoJSON geometry wrapper mirroring `WktGeometry` coordinates.
pub struct GeoJsonGeometry {
    pub kind: u8,
    pub coords: [32]Coord2D,
    pub coord_count: usize,

    pub static fn fromWkt(g: WktGeometry): GeoJsonGeometry {
        return GeoJsonGeometry {
            kind: g.kind,
            coords: g.coords,
            coord_count: g.coord_count,
        };
    }

    pub fn toWkt(): WktGeometry {
        return WktGeometry {
            kind: _.kind,
            is_empty: _.coord_count == 0,
            coords: _.coords,
            coord_count: _.coord_count,
        };
    }
}

/// RFC 7946 GeoJSON Feature holding an optional string id, geometry, and optional bbox.
pub struct GeoJsonFeature {
    pub id: string,
    pub geom: WktGeometry,
    pub has_bbox: bool,
    pub bbox: WktBbox,

    pub static fn init(id: string, geom: WktGeometry, include_bbox: bool): GeoJsonFeature {
        let bb: WktBbox = bboxWkt(geom);
        return GeoJsonFeature {
            id: id,
            geom: geom,
            has_bbox: include_bbox && bb.valid,
            bbox: bb,
        };
    }
}

fn gjZeroCoord(): Coord2D {
    return Coord2D { x: 0.0, y: 0.0 };
}

fn gjEmptyGeom(kind: u8): WktGeometry {
    let z: Coord2D = gjZeroCoord();
    return WktGeometry {
        kind: kind,
        is_empty: true,
        coords: [
            z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z,
            z, z, z, z, z, z, z, z, z, z, z, z, z, z, z, z
        ],
        coord_count: 0,
    };
}

fn gjIsSpace(c: u8): bool {
    return c == 32 || c == 9 || c == 10 || c == 13;
}

fn gjIsDigit(c: u8): bool {
    return c >= 48 && c <= 57;
}

fn gjSkipWs(s: string, pos: *usize) {
    let i: usize = pos.*;
    while (i < s.len && gjIsSpace(s[i])) {
        i = i + 1;
    }
    pos.* = i;
}

fn gjConsumeChar(s: string, pos: *usize, ch: u8): bool {
    gjSkipWs(s, pos);
    if (pos.* >= s.len || s[pos.*] != ch) { return false; }
    pos.* = pos.* + 1;
    return true;
}

fn gjStrEq(a: string, b: string): bool {
    if (a.len != b.len) { return false; }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn gjWriteByte(out: []u8, pos: usize, b: u8): usize {
    if (pos >= out.len) { return pos; }
    out[pos] = b;
    return pos + 1;
}

fn gjWriteStr(out: []u8, pos: usize, s: string): usize {
    let i: usize = 0;
    let p: usize = pos;
    while (i < s.len && p < out.len) {
        out[p] = s[i];
        p = p + 1;
        i = i + 1;
    }
    return p;
}

fn gjWriteJsonEscaped(out: []u8, pos: usize, s: string): usize {
    let p: usize = gjWriteByte(out, pos, 34);
    let i: usize = 0;
    while (i < s.len) {
        let c: u8 = s[i];
        if (c == 34 || c == 92) {
            p = gjWriteByte(out, p, 92);
            p = gjWriteByte(out, p, c);
        } else if (c == 10) {
            p = gjWriteStr(out, p, "\\n");
        } else if (c == 13) {
            p = gjWriteStr(out, p, "\\r");
        } else if (c == 9) {
            p = gjWriteStr(out, p, "\\t");
        } else {
            p = gjWriteByte(out, p, c);
        }
        i = i + 1;
    }
    p = gjWriteByte(out, p, 34);
    return p;
}

fn gjWriteU64(out: []u8, pos: usize, val: u64): usize {
    let tmp: [24]u8 = [0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0];
    let n: usize = 0;
    let v: u64 = val;
    if (v == 0) {
        tmp[0] = 48;
        n = 1;
    } else {
        while (v > 0) {
            tmp[n] = (48 + (v % 10)) as u8;
            v = v / 10;
            n = n + 1;
        }
    }
    let p: usize = pos;
    let i: usize = n;
    while (i > 0 && p < out.len) {
        i = i - 1;
        out[p] = tmp[i];
        p = p + 1;
    }
    return p;
}

fn gjWriteF64(out: []u8, pos: usize, val: f64): usize {
    let p: usize = pos;
    let v: f64 = val;
    if (v == 0.0) {
        return gjWriteStr(out, p, "0");
    }
    if (v < 0.0) {
        p = gjWriteStr(out, p, "-");
        v = -v;
    }
    let scaled: u64 = ((v * 1000000.0) + 0.5) as u64;
    let ipart: u64 = scaled / 1000000;
    let fpart: u64 = scaled % 1000000;
    p = gjWriteU64(out, p, ipart);
    if (fpart > 0) {
        p = gjWriteStr(out, p, ".");
        let fbuf: [6]u8 = [48, 48, 48, 48, 48, 48];
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
        let k: usize = 0;
        while (k < trim_len && p < out.len) {
            out[p] = fbuf[k];
            p = p + 1;
            k = k + 1;
        }
    }
    return p;
}

fn gjWriteCoord(out: []u8, pos: usize, c: Coord2D): usize {
    let p: usize = gjWriteByte(out, pos, 91);
    p = gjWriteF64(out, p, c.x);
    p = gjWriteByte(out, p, 44);
    p = gjWriteF64(out, p, c.y);
    p = gjWriteByte(out, p, 93);
    return p;
}

fn gjParseNumber(s: string, pos: *usize, out: *f64): bool {
    gjSkipWs(s, pos);
    let i: usize = pos.*;
    if (i >= s.len) { return false; }

    let neg: bool = false;
    if (s[i] == 45) {
        neg = true;
        i = i + 1;
    }
    if (i >= s.len || !gjIsDigit(s[i])) { return false; }

    let val: f64 = 0.0;
    while (i < s.len && gjIsDigit(s[i])) {
        val = val * 10.0 + ((s[i] - 48) as f64);
        i = i + 1;
    }

    if (i < s.len && s[i] == 46) {
        i = i + 1;
        let scale: f64 = 0.1;
        let has_frac: bool = false;
        while (i < s.len && gjIsDigit(s[i])) {
            has_frac = true;
            val = val + ((s[i] - 48) as f64) * scale;
            scale = scale * 0.1;
            i = i + 1;
        }
        if (!has_frac) { return false; }
    }

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
        while (i < s.len && gjIsDigit(s[i])) {
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

fn gjParseStringSlice(s: string, pos: *usize, start_out: *usize, end_out: *usize): bool {
    if (!gjConsumeChar(s, pos, 34)) { return false; }
    let start: usize = pos.*;
    let i: usize = start;
    while (i < s.len) {
        let c: u8 = s[i];
        if (c == 92) {
            if (i + 1 >= s.len) { return false; }
            i = i + 2;
        } else if (c == 34) {
            start_out.* = start;
            end_out.* = i;
            pos.* = i + 1;
            return true;
        } else {
            i = i + 1;
        }
    }
    return false;
}

fn gjSkipValue(s: string, pos: *usize, depth: u32): bool {
    if (depth > 32) { return false; }
    gjSkipWs(s, pos);
    if (pos.* >= s.len) { return false; }
    let c: u8 = s[pos.*];
    if (c == 34) {
        let st: usize = 0;
        let en: usize = 0;
        return gjParseStringSlice(s, pos, &st, &en);
    }
    if (c == 91) {
        pos.* = pos.* + 1;
        gjSkipWs(s, pos);
        if (pos.* < s.len && s[pos.*] == 93) {
            pos.* = pos.* + 1;
            return true;
        }
        let done: bool = false;
        while (!done) {
            if (!gjSkipValue(s, pos, depth + 1)) { return false; }
            gjSkipWs(s, pos);
            if (pos.* < s.len && s[pos.*] == 44) {
                pos.* = pos.* + 1;
            } else {
                done = true;
            }
        }
        return gjConsumeChar(s, pos, 93);
    }
    if (c == 123) {
        pos.* = pos.* + 1;
        gjSkipWs(s, pos);
        if (pos.* < s.len && s[pos.*] == 125) {
            pos.* = pos.* + 1;
            return true;
        }
        let done: bool = false;
        while (!done) {
            let ks: usize = 0;
            let ke: usize = 0;
            if (!gjParseStringSlice(s, pos, &ks, &ke)) { return false; }
            if (!gjConsumeChar(s, pos, 58)) { return false; }
            if (!gjSkipValue(s, pos, depth + 1)) { return false; }
            gjSkipWs(s, pos);
            if (pos.* < s.len && s[pos.*] == 44) {
                pos.* = pos.* + 1;
            } else {
                done = true;
            }
        }
        return gjConsumeChar(s, pos, 125);
    }
    if (c == 116) {
        if (pos.* + 4 > s.len || !gjStrEq(s[pos.*..pos.* + 4], "true")) { return false; }
        pos.* = pos.* + 4;
        return true;
    }
    if (c == 102) {
        if (pos.* + 5 > s.len || !gjStrEq(s[pos.*..pos.* + 5], "false")) { return false; }
        pos.* = pos.* + 5;
        return true;
    }
    if (c == 110) {
        if (pos.* + 4 > s.len || !gjStrEq(s[pos.*..pos.* + 4], "null")) { return false; }
        pos.* = pos.* + 4;
        return true;
    }
    let num: f64 = 0.0;
    return gjParseNumber(s, pos, &num);
}

fn gjParsePosition(s: string, pos: *usize, out: *Coord2D): bool {
    if (!gjConsumeChar(s, pos, 91)) { return false; }
    let x: f64 = 0.0;
    let y: f64 = 0.0;
    if (!gjParseNumber(s, pos, &x)) { return false; }
    if (!gjConsumeChar(s, pos, 44)) { return false; }
    if (!gjParseNumber(s, pos, &y)) { return false; }
    gjSkipWs(s, pos);
    // Optional 3rd altitude component per RFC 7946
    if (pos.* < s.len && s[pos.*] == 44) {
        pos.* = pos.* + 1;
        let z: f64 = 0.0;
        if (!gjParseNumber(s, pos, &z)) { return false; }
    }
    if (!gjConsumeChar(s, pos, 93)) { return false; }
    out.* = Coord2D { x: x, y: y };
    return true;
}

fn gjParsePosList(s: string, pos: *usize, geom: *WktGeometry): bool {
    if (!gjConsumeChar(s, pos, 91)) { return false; }
    gjSkipWs(s, pos);
    if (pos.* >= s.len || s[pos.*] == 93) { return false; }
    let done: bool = false;
    while (!done) {
        if (geom.*.coord_count >= WKT_MAX_COORDS) { return false; }
        let pt: Coord2D = gjZeroCoord();
        if (!gjParsePosition(s, pos, &pt)) { return false; }
        geom.*.coords[geom.*.coord_count] = pt;
        geom.*.coord_count = geom.*.coord_count + 1;
        gjSkipWs(s, pos);
        if (pos.* < s.len && s[pos.*] == 44) {
            pos.* = pos.* + 1;
        } else {
            done = true;
        }
    }
    return gjConsumeChar(s, pos, 93);
}

/// Formats `g` into RFC 7946 GeoJSON geometry text in `out`, returning bytes written.
/// Returns `0` for empty or unsupported geometry kinds.
pub fn formatGeoJsonGeometry(g: WktGeometry, out: []u8): usize {
    if (g.is_empty || g.coord_count == 0) { return 0; }
    let p: usize = 0;

    if (g.kind == WKT_POINT) {
        p = gjWriteStr(out, p, "{\"type\":\"Point\",\"coordinates\":");
        p = gjWriteCoord(out, p, g.coords[0]);
        p = gjWriteByte(out, p, 125);
        return p;
    }

    if (g.kind == WKT_LINESTRING) {
        p = gjWriteStr(out, p, "{\"type\":\"LineString\",\"coordinates\":[");
        let i: usize = 0;
        while (i < g.coord_count) {
            if (i > 0) { p = gjWriteByte(out, p, 44); }
            p = gjWriteCoord(out, p, g.coords[i]);
            i = i + 1;
        }
        p = gjWriteStr(out, p, "]}");
        return p;
    }

    if (g.kind == WKT_POLYGON) {
        p = gjWriteStr(out, p, "{\"type\":\"Polygon\",\"coordinates\":[[");
        let i: usize = 0;
        while (i < g.coord_count) {
            if (i > 0) { p = gjWriteByte(out, p, 44); }
            p = gjWriteCoord(out, p, g.coords[i]);
            i = i + 1;
        }
        p = gjWriteStr(out, p, "]]}");
        return p;
    }

    return 0;
}

/// Formats `f` into RFC 7946 GeoJSON Feature text in `out`, returning bytes written.
pub fn formatGeoJsonFeature(f: GeoJsonFeature, out: []u8): usize {
    let p: usize = 0;
    p = gjWriteStr(out, p, "{\"type\":\"Feature\"");
    if (f.id.len > 0) {
        p = gjWriteStr(out, p, ",\"id\":");
        p = gjWriteJsonEscaped(out, p, f.id);
    }
    if (f.has_bbox && f.bbox.valid) {
        p = gjWriteStr(out, p, ",\"bbox\":[");
        p = gjWriteF64(out, p, f.bbox.min_x);
        p = gjWriteByte(out, p, 44);
        p = gjWriteF64(out, p, f.bbox.min_y);
        p = gjWriteByte(out, p, 44);
        p = gjWriteF64(out, p, f.bbox.max_x);
        p = gjWriteByte(out, p, 44);
        p = gjWriteF64(out, p, f.bbox.max_y);
        p = gjWriteByte(out, p, 93);
    }
    p = gjWriteStr(out, p, ",\"geometry\":");
    if (f.geom.is_empty || f.geom.coord_count == 0) {
        p = gjWriteStr(out, p, "null");
    } else {
        let written: usize = formatGeoJsonGeometry(f.geom, out[p..out.len]);
        if (written == 0) {
            p = gjWriteStr(out, p, "null");
        } else {
            p = p + written;
        }
    }
    p = gjWriteStr(out, p, ",\"properties\":{}}");
    return p;
}

/// Parses a GeoJSON geometry object (`Point`, `LineString`, or `Polygon`) into `WktGeometry`.
/// Rejects unknown types, missing keys, empty coordinate arrays, and malformed JSON.
pub fn parseGeoJsonGeometry(json: string): WktGeometry? {
    let pos: usize = 0;
    if (!gjConsumeChar(json, &pos, 123)) { return null; }
    gjSkipWs(json, &pos);
    if (pos >= json.len || json[pos] == 125) { return null; }

    let kind: u8 = 0;
    let has_type: bool = false;
    let has_coords: bool = false;
    let coords_pos: usize = 0;

    let done: bool = false;
    while (!done) {
        let ks: usize = 0;
        let ke: usize = 0;
        if (!gjParseStringSlice(json, &pos, &ks, &ke)) { return null; }
        let key: string = json[ks..ke];
        if (!gjConsumeChar(json, &pos, 58)) { return null; }
        gjSkipWs(json, &pos);

        if (gjStrEq(key, "type")) {
            let vs: usize = 0;
            let ve: usize = 0;
            if (!gjParseStringSlice(json, &pos, &vs, &ve)) { return null; }
            let tname: string = json[vs..ve];
            if (gjStrEq(tname, "Point")) { kind = WKT_POINT; }
            else if (gjStrEq(tname, "LineString")) { kind = WKT_LINESTRING; }
            else if (gjStrEq(tname, "Polygon")) { kind = WKT_POLYGON; }
            else { return null; }
            has_type = true;
        } else if (gjStrEq(key, "coordinates")) {
            coords_pos = pos;
            if (!gjSkipValue(json, &pos, 0)) { return null; }
            has_coords = true;
        } else {
            if (!gjSkipValue(json, &pos, 0)) { return null; }
        }

        gjSkipWs(json, &pos);
        if (pos < json.len && json[pos] == 44) {
            pos = pos + 1;
        } else {
            done = true;
        }
    }

    if (!gjConsumeChar(json, &pos, 125)) { return null; }
    gjSkipWs(json, &pos);
    if (pos != json.len) { return null; }
    if (!has_type || !has_coords) { return null; }

    let geom: WktGeometry = gjEmptyGeom(kind);
    geom.is_empty = false;
    let cp: usize = coords_pos;

    if (kind == WKT_POINT) {
        let pt: Coord2D = gjZeroCoord();
        if (!gjParsePosition(json, &cp, &pt)) { return null; }
        geom.coords[0] = pt;
        geom.coord_count = 1;
        return geom;
    }

    if (kind == WKT_LINESTRING) {
        if (!gjParsePosList(json, &cp, &geom)) { return null; }
        if (geom.coord_count == 0) { return null; }
        return geom;
    }

    if (kind == WKT_POLYGON) {
        if (!gjConsumeChar(json, &cp, 91)) { return null; }
        gjSkipWs(json, &cp);
        if (cp >= json.len || json[cp] == 93) { return null; }
        if (!gjParsePosList(json, &cp, &geom)) { return null; }
        if (geom.coord_count < 3) { return null; }
        // Skip optional interior hole rings if present
        gjSkipWs(json, &cp);
        while (cp < json.len && json[cp] == 44) {
            cp = cp + 1;
            if (!gjSkipValue(json, &cp, 0)) { return null; }
            gjSkipWs(json, &cp);
        }
        if (!gjConsumeChar(json, &cp, 93)) { return null; }
        return geom;
    }

    return null;
}

/// Parses a GeoJSON `Point` geometry object, returning `null` for any non-Point or invalid input.
pub fn parseGeoJsonPoint(json: string): WktGeometry? {
    let maybe = parseGeoJsonGeometry(json);
    if (maybe == null) { return null; }
    let g: WktGeometry = maybe ?? gjEmptyGeom(0);
    if (g.kind != WKT_POINT || g.is_empty || g.coord_count != 1) { return null; }
    return g;
}

/// Converts a WKT geometry string directly to RFC 7946 GeoJSON in `out` in one step.
/// Returns `0` on invalid or empty WKT input.
pub fn wktToGeoJson(wkt_str: string, out: []u8): usize {
    let maybe = parseWkt(wkt_str);
    if (maybe == null) { return 0; }
    let g: WktGeometry = maybe ?? gjEmptyGeom(0);
    if (g.is_empty || g.coord_count == 0) { return 0; }
    return formatGeoJsonGeometry(g, out);
}

/// Converts a GeoJSON geometry string directly to canonical WKT in `out` in one step.
/// Returns `0` on invalid GeoJSON input.
pub fn geoJsonToWkt(json: string, out: []u8): usize {
    let maybe = parseGeoJsonGeometry(json);
    if (maybe == null) { return 0; }
    let g: WktGeometry = maybe ?? gjEmptyGeom(0);
    return formatWkt(g, out);
}
