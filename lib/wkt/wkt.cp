// Source: cp/lib/wkt/wkt.cp (OGC Well-Known Text WKT & Well-Known Binary WKB Geometry Codec)
// Pure CP zero-C-dependency OGC WKT parser/formatter and ISO WKB encoder/decoder
// supporting 2D/Z/M/ZM coordinates, EMPTY geometries, BigEndian/LittleEndian WKB,
// ring validation, and 32-level nesting depth protection.

const std = @import("std");

const MAX_DEPTH: usize = 32;

/// Unified error set for `cap:wkt` WKT/WKB parsing, formatting, and validation.
pub err WktError {
    UnexpectedToken,
    UnexpectedEof,
    InvalidGeometryType,
    InvalidCoordinate,
    InvalidRing,
    InvalidWkbHeader,
    MaxDepthExceeded,
    OutOfMemory,
}

/// ISO WKB byte order marker (`0x00` = BigEndian, `0x01` = LittleEndian).
pub enum WkbByteOrder: u8 {
    BigEndian = 0,
    LittleEndian = 1,
}

/// 2D/3D/4D coordinate tuple (`x`, `y`, optional `z`, optional `m`).
pub struct WktCoord {
    pub x: f64 = 0.0,
    pub y: f64 = 0.0,
    pub z: f64? = null,
    pub m: f64? = null,
}

type OptWktCoord = WktCoord?;

/// Polygon coordinate rings: `rings[0]` is the exterior ring, `rings[1..]` are interior holes.
pub struct WktPolygon {
    pub rings: [][]WktCoord,

    /// Releases all ring slices and the outer ring array owned by this polygon.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.rings.len) {
            if (_.rings[i].len > 0) {
                allocator.free(_.rings[i]);
            }
            i += 1;
        }
        if (_.rings.len > 0) {
            allocator.free(_.rings);
        }
        _.rings = _.rings[0..0];
    }
}

/// OGC WKT/WKB Geometry tagged union.
pub enum WktGeometry {
    point: WktCoord?,
    line_string: []WktCoord,
    polygon: WktPolygon,
    multi_point: []WktCoord?,
    multi_line_string: [][]WktCoord,
    multi_polygon: []WktPolygon,
    geometry_collection: []WktGeometry,

    /// Recursively releases all heap-allocated coordinate slices and child geometries.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        switch (_.*) {
            .point => {},
            .line_string => |pts| {
                if (pts.len > 0) allocator.free(pts);
            },
            .polygon => |*poly| {
                poly.deinit(allocator);
            },
            .multi_point => |pts| {
                if (pts.len > 0) allocator.free(pts);
            },
            .multi_line_string => |lines| {
                let i: usize = 0;
                while (i < lines.len) {
                    if (lines[i].len > 0) allocator.free(lines[i]);
                    i += 1;
                }
                if (lines.len > 0) allocator.free(lines);
            },
            .multi_polygon => |polys| {
                let i: usize = 0;
                while (i < polys.len) {
                    polys[i].deinit(allocator);
                    i += 1;
                }
                if (polys.len > 0) allocator.free(polys);
            },
            .geometry_collection => |geoms| {
                let i: usize = 0;
                while (i < geoms.len) {
                    geoms[i].deinit(allocator);
                    i += 1;
                }
                if (geoms.len > 0) allocator.free(geoms);
            },
        }
    }
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

const DimMode = enum {
    Auto,
    Z,
    M,
    ZM,
};

fn isSpace(c: u8): bool {
    return c == ' ' or c == '\t' or c == '\r' or c == '\n';
}

fn isAlpha(c: u8): bool {
    return (c >= 'a' and c <= 'z') or (c >= 'A' and c <= 'Z') or c == '_';
}

fn isIdentChar(c: u8): bool {
    return isAlpha(c) or (c >= '0' and c <= '9');
}

fn isNumChar(c: u8): bool {
    return (c >= '0' and c <= '9') or c == '-' or c == '+' or c == '.' or c == 'e' or c == 'E';
}

fn asciiEqlIgnoreCase(a: []const u8, b: []const u8): bool {
    return std.ascii.eqlIgnoreCase(a, b);
}

fn emptyCoordSlice(allocator: std.mem.Allocator): ([]WktCoord) ! WktError {
    return allocator.alloc(WktCoord, 0) catch return WktError.OutOfMemory;
}

fn emptyOptCoordSlice(allocator: std.mem.Allocator): ([]WktCoord?) ! WktError {
    return allocator.alloc(OptWktCoord, 0) catch return WktError.OutOfMemory;
}

fn emptyRingSlice(allocator: std.mem.Allocator): ([][]WktCoord) ! WktError {
    return allocator.alloc([]WktCoord, 0) catch return WktError.OutOfMemory;
}

fn emptyPolygonSlice(allocator: std.mem.Allocator): ([]WktPolygon) ! WktError {
    return allocator.alloc(WktPolygon, 0) catch return WktError.OutOfMemory;
}

fn emptyGeomSlice(allocator: std.mem.Allocator): ([]WktGeometry) ! WktError {
    return allocator.alloc(WktGeometry, 0) catch return WktError.OutOfMemory;
}

fn validateRing(ring: []const WktCoord): void ! WktError {
    if (ring.len < 4) {
        return WktError.InvalidRing;
    }
    const first = ring[0];
    const last = ring[ring.len - 1];
    if (first.x != last.x or first.y != last.y) {
        return WktError.InvalidRing;
    }
}

fn freeCoordLines(allocator: std.mem.Allocator, lines: [][]WktCoord) {
    let i: usize = 0;
    while (i < lines.len) {
        if (lines[i].len > 0) allocator.free(lines[i]);
        i += 1;
    }
    if (lines.len > 0) allocator.free(lines);
}

// ---------------------------------------------------------------------------
// WKT Parser
// ---------------------------------------------------------------------------

struct WktParser {
    text: []const u8,
    pos: usize = 0,

    fn skipWs(*_) {
        while (_.pos < _.text.len and isSpace(_.text[_.pos])) {
            _.pos += 1;
        }
    }

    fn peekChar(*_): u8? {
        _.skipWs();
        if (_.pos >= _.text.len) return null;
        return _.text[_.pos];
    }

    fn consumeChar(*_, expected: u8): void ! WktError {
        _.skipWs();
        if (_.pos >= _.text.len) return WktError.UnexpectedEof;
        if (_.text[_.pos] != expected) return WktError.UnexpectedToken;
        _.pos += 1;
    }

    fn readIdent(*_): ([]const u8) ! WktError {
        _.skipWs();
        if (_.pos >= _.text.len) return WktError.UnexpectedEof;
        if (!isAlpha(_.text[_.pos])) return WktError.UnexpectedToken;
        const start = _.pos;
        while (_.pos < _.text.len and isIdentChar(_.text[_.pos])) {
            _.pos += 1;
        }
        return _.text[start.._.pos];
    }

    fn tryConsumeEmpty(*_): bool {
        _.skipWs();
        const saved = _.pos;
        if (_.pos + 5 <= _.text.len and asciiEqlIgnoreCase(_.text[_.pos.._.pos + 5], "EMPTY")) {
            const after = _.pos + 5;
            if (after == _.text.len or !isIdentChar(_.text[after])) {
                _.pos = after;
                return true;
            }
        }
        _.pos = saved;
        return false;
    }

    fn tryReadDimMode(*_): DimMode {
        _.skipWs();
        const saved = _.pos;
        if (_.pos >= _.text.len or !isAlpha(_.text[_.pos])) {
            return .Auto;
        }
        let end = _.pos;
        while (end < _.text.len and isIdentChar(_.text[end])) {
            end += 1;
        }
        const tok = _.text[_.pos..end];
        if (asciiEqlIgnoreCase(tok, "ZM")) {
            _.pos = end;
            return .ZM;
        }
        if (asciiEqlIgnoreCase(tok, "Z")) {
            _.pos = end;
            return .Z;
        }
        if (asciiEqlIgnoreCase(tok, "M")) {
            _.pos = end;
            return .M;
        }
        _.pos = saved;
        return .Auto;
    }

    fn readNumber(*_): f64 ! WktError {
        _.skipWs();
        if (_.pos >= _.text.len) return WktError.UnexpectedEof;
        const start = _.pos;
        while (_.pos < _.text.len and isNumChar(_.text[_.pos])) {
            _.pos += 1;
        }
        if (_.pos == start) return WktError.InvalidCoordinate;
        const tok = _.text[start.._.pos];
        const val = std.fmt.parseFloat(f64, tok) catch return WktError.InvalidCoordinate;
        if (!std.math.isFinite(val)) return WktError.InvalidCoordinate;
        return val;
    }

    fn readCoord(*_, dim: DimMode): WktCoord ! WktError {
        const x = _.readNumber()?;
        const y = _.readNumber()?;
        let z: f64? = null;
        let m: f64? = null;

        let extra_count: usize = 0;
        let v3: f64 = 0.0;
        let v4: f64 = 0.0;

        _.skipWs();
        if (_.pos < _.text.len and isNumChar(_.text[_.pos])) {
            v3 = _.readNumber()?;
            extra_count = 1;
            _.skipWs();
            if (_.pos < _.text.len and isNumChar(_.text[_.pos])) {
                v4 = _.readNumber()?;
                extra_count = 2;
                _.skipWs();
                if (_.pos < _.text.len and isNumChar(_.text[_.pos])) {
                    return WktError.InvalidCoordinate;
                }
            }
        }

        switch (dim) {
            .Auto => {
                if (extra_count == 1) {
                    z = v3;
                } else if (extra_count == 2) {
                    z = v3;
                    m = v4;
                }
            },
            .Z => {
                if (extra_count != 1) return WktError.InvalidCoordinate;
                z = v3;
            },
            .M => {
                if (extra_count != 1) return WktError.InvalidCoordinate;
                m = v3;
            },
            .ZM => {
                if (extra_count != 2) return WktError.InvalidCoordinate;
                z = v3;
                m = v4;
            },
        }

        return WktCoord{ x: x, y: y, z: z, m: m };
    }

    fn readCoordList(*_, allocator: std.mem.Allocator, dim: DimMode): ([]WktCoord) ! WktError {
        _.consumeChar('(')?;
        let list: std.ArrayList(WktCoord) = .empty;
        errdefer list.deinit(allocator);

        let first = true;
        while (true) {
            if (!first) {
                const ch = _.peekChar() orelse return WktError.UnexpectedEof;
                if (ch == ')') {
                    _.pos += 1;
                    break;
                }
                if (ch != ',') return WktError.UnexpectedToken;
                _.pos += 1;
            }
            first = false;
            const c = _.readCoord(dim)?;
            list.append(allocator, c) catch return WktError.OutOfMemory;
        }

        return list.toOwnedSlice(allocator) catch return WktError.OutOfMemory;
    }

    fn readPolygonRings(*_, allocator: std.mem.Allocator, dim: DimMode): WktPolygon ! WktError {
        _.consumeChar('(')?;
        let rings: std.ArrayList([]WktCoord) = .empty;
        errdefer {
            let i: usize = 0;
            while (i < rings.items.len) {
                if (rings.items[i].len > 0) allocator.free(rings.items[i]);
                i += 1;
            }
            rings.deinit(allocator);
        }

        let first = true;
        while (true) {
            if (!first) {
                const ch = _.peekChar() orelse return WktError.UnexpectedEof;
                if (ch == ')') {
                    _.pos += 1;
                    break;
                }
                if (ch != ',') return WktError.UnexpectedToken;
                _.pos += 1;
            }
            first = false;
            const ring = _.readCoordList(allocator, dim)?;
            errdefer {
                if (ring.len > 0) allocator.free(ring);
            }
            validateRing(ring)?;
            rings.append(allocator, ring) catch return WktError.OutOfMemory;
        }

        const slice = rings.toOwnedSlice(allocator) catch return WktError.OutOfMemory;
        return WktPolygon{ rings: slice };
    }

    fn parseGeometry(*_, allocator: std.mem.Allocator, depth: usize): WktGeometry ! WktError {
        if (depth >= MAX_DEPTH) return WktError.MaxDepthExceeded;

        const geom_type = _.readIdent()?;
        const dim = _.tryReadDimMode();

        if (asciiEqlIgnoreCase(geom_type, "POINT")) {
            if (_.tryConsumeEmpty()) {
                return WktGeometry{ .point = null };
            }
            _.consumeChar('(')?;
            const c = _.readCoord(dim)?;
            _.consumeChar(')')?;
            return WktGeometry{ .point = c };
        }

        if (asciiEqlIgnoreCase(geom_type, "LINESTRING")) {
            if (_.tryConsumeEmpty()) {
                return WktGeometry{ .line_string = emptyCoordSlice(allocator)? };
            }
            const pts = _.readCoordList(allocator, dim)?;
            return WktGeometry{ .line_string = pts };
        }

        if (asciiEqlIgnoreCase(geom_type, "POLYGON")) {
            if (_.tryConsumeEmpty()) {
                return WktGeometry{ .polygon = WktPolygon{ rings: emptyRingSlice(allocator)? } };
            }
            const poly = _.readPolygonRings(allocator, dim)?;
            return WktGeometry{ .polygon = poly };
        }

        if (asciiEqlIgnoreCase(geom_type, "MULTIPOINT")) {
            if (_.tryConsumeEmpty()) {
                return WktGeometry{ .multi_point = emptyOptCoordSlice(allocator)? };
            }
            _.consumeChar('(')?;
            let pts: std.ArrayList(OptWktCoord) = .empty;
            errdefer pts.deinit(allocator);

            const first_ch = _.peekChar() orelse return WktError.UnexpectedEof;
            const parenthesized = first_ch == '(' or first_ch == 'E' or first_ch == 'e';

            let first = true;
            while (true) {
                if (!first) {
                    const ch = _.peekChar() orelse return WktError.UnexpectedEof;
                    if (ch == ')') {
                        _.pos += 1;
                        break;
                    }
                    if (ch != ',') return WktError.UnexpectedToken;
                    _.pos += 1;
                }
                first = false;
                if (parenthesized) {
                    if (_.tryConsumeEmpty()) {
                        pts.append(allocator, null) catch return WktError.OutOfMemory;
                    } else {
                        _.consumeChar('(')?;
                        const c = _.readCoord(dim)?;
                        _.consumeChar(')')?;
                        pts.append(allocator, c) catch return WktError.OutOfMemory;
                    }
                } else {
                    const c = _.readCoord(dim)?;
                    pts.append(allocator, c) catch return WktError.OutOfMemory;
                }
            }

            const slice = pts.toOwnedSlice(allocator) catch return WktError.OutOfMemory;
            return WktGeometry{ .multi_point = slice };
        }

        if (asciiEqlIgnoreCase(geom_type, "MULTILINESTRING")) {
            if (_.tryConsumeEmpty()) {
                return WktGeometry{ .multi_line_string = emptyRingSlice(allocator)? };
            }
            _.consumeChar('(')?;
            let lines: std.ArrayList([]WktCoord) = .empty;
            errdefer {
                let i: usize = 0;
                while (i < lines.items.len) {
                    if (lines.items[i].len > 0) allocator.free(lines.items[i]);
                    i += 1;
                }
                lines.deinit(allocator);
            }

            let first = true;
            while (true) {
                if (!first) {
                    const ch = _.peekChar() orelse return WktError.UnexpectedEof;
                    if (ch == ')') {
                        _.pos += 1;
                        break;
                    }
                    if (ch != ',') return WktError.UnexpectedToken;
                    _.pos += 1;
                }
                first = false;
                const line = _.readCoordList(allocator, dim)?;
                lines.append(allocator, line) catch {
                    if (line.len > 0) allocator.free(line);
                    return WktError.OutOfMemory;
                };
            }

            const slice = lines.toOwnedSlice(allocator) catch return WktError.OutOfMemory;
            return WktGeometry{ .multi_line_string = slice };
        }

        if (asciiEqlIgnoreCase(geom_type, "MULTIPOLYGON")) {
            if (_.tryConsumeEmpty()) {
                return WktGeometry{ .multi_polygon = emptyPolygonSlice(allocator)? };
            }
            _.consumeChar('(')?;
            let polys: std.ArrayList(WktPolygon) = .empty;
            errdefer {
                let i: usize = 0;
                while (i < polys.items.len) {
                    polys.items[i].deinit(allocator);
                    i += 1;
                }
                polys.deinit(allocator);
            }

            let first = true;
            while (true) {
                if (!first) {
                    const ch = _.peekChar() orelse return WktError.UnexpectedEof;
                    if (ch == ')') {
                        _.pos += 1;
                        break;
                    }
                    if (ch != ',') return WktError.UnexpectedToken;
                    _.pos += 1;
                }
                first = false;
                let poly = _.readPolygonRings(allocator, dim)?;
                polys.append(allocator, poly) catch {
                    poly.deinit(allocator);
                    return WktError.OutOfMemory;
                };
            }

            const slice = polys.toOwnedSlice(allocator) catch return WktError.OutOfMemory;
            return WktGeometry{ .multi_polygon = slice };
        }

        if (asciiEqlIgnoreCase(geom_type, "GEOMETRYCOLLECTION")) {
            if (_.tryConsumeEmpty()) {
                return WktGeometry{ .geometry_collection = emptyGeomSlice(allocator)? };
            }
            _.consumeChar('(')?;
            let geoms: std.ArrayList(WktGeometry) = .empty;
            errdefer {
                let i: usize = 0;
                while (i < geoms.items.len) {
                    geoms.items[i].deinit(allocator);
                    i += 1;
                }
                geoms.deinit(allocator);
            }

            let first = true;
            while (true) {
                if (!first) {
                    const ch = _.peekChar() orelse return WktError.UnexpectedEof;
                    if (ch == ')') {
                        _.pos += 1;
                        break;
                    }
                    if (ch != ',') return WktError.UnexpectedToken;
                    _.pos += 1;
                }
                first = false;
                let child = _.parseGeometry(allocator, depth + 1)?;
                geoms.append(allocator, child) catch {
                    child.deinit(allocator);
                    return WktError.OutOfMemory;
                };
            }

            const slice = geoms.toOwnedSlice(allocator) catch return WktError.OutOfMemory;
            return WktGeometry{ .geometry_collection = slice };
        }

        return WktError.InvalidGeometryType;
    }
}

/// Parses an OGC WKT text string into a `WktGeometry`.
pub fn parseWkt(allocator: std.mem.Allocator, text: []const u8): WktGeometry ! WktError {
    let parser = WktParser{ text: text, pos: 0 };
    parser.skipWs();
    if (parser.pos >= text.len) {
        return WktError.UnexpectedEof;
    }
    let geom = parser.parseGeometry(allocator, 0)?;
    errdefer geom.deinit(allocator);
    parser.skipWs();
    if (parser.pos != text.len) {
        return WktError.UnexpectedToken;
    }
    return geom;
}

// ---------------------------------------------------------------------------
// WKT Formatter
// ---------------------------------------------------------------------------

struct DimFlags {
    has_z: bool = false,
    has_m: bool = false,
}

fn scanCoordDim(c: WktCoord, flags: *DimFlags) {
    if (c.z != null) flags.has_z = true;
    if (c.m != null) flags.has_m = true;
}

fn scanCoordsDim(coords: []const WktCoord, flags: *DimFlags) {
    let i: usize = 0;
    while (i < coords.len) {
        scanCoordDim(coords[i], flags);
        i += 1;
    }
}

fn appendDimSuffix(out: *std.ArrayList(u8), alloc: std.mem.Allocator, flags: DimFlags): void ! WktError {
    if (flags.has_z and flags.has_m) {
        out.appendSlice(alloc, " ZM") catch return WktError.OutOfMemory;
    } else if (flags.has_z) {
        out.appendSlice(alloc, " Z") catch return WktError.OutOfMemory;
    } else if (flags.has_m) {
        out.appendSlice(alloc, " M") catch return WktError.OutOfMemory;
    }
}

fn appendF64Text(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: f64): void ! WktError {
    let buf: [64]u8 = undefined;
    const norm = if (val == 0.0) 0.0 else val;
    const s = std.fmt.bufPrint(&buf, "{d}", .{norm}) catch return WktError.OutOfMemory;
    out.appendSlice(alloc, s) catch return WktError.OutOfMemory;
}

fn appendCoordText(out: *std.ArrayList(u8), alloc: std.mem.Allocator, c: WktCoord, flags: DimFlags): void ! WktError {
    appendF64Text(out, alloc, c.x)?;
    out.append(alloc, ' ') catch return WktError.OutOfMemory;
    appendF64Text(out, alloc, c.y)?;
    if (flags.has_z) {
        out.append(alloc, ' ') catch return WktError.OutOfMemory;
        appendF64Text(out, alloc, c.z orelse 0.0)?;
    }
    if (flags.has_m) {
        out.append(alloc, ' ') catch return WktError.OutOfMemory;
        appendF64Text(out, alloc, c.m orelse 0.0)?;
    }
}

fn appendCoordListText(out: *std.ArrayList(u8), alloc: std.mem.Allocator, coords: []const WktCoord, flags: DimFlags): void ! WktError {
    out.append(alloc, '(') catch return WktError.OutOfMemory;
    let i: usize = 0;
    while (i < coords.len) {
        if (i > 0) out.appendSlice(alloc, ", ") catch return WktError.OutOfMemory;
        appendCoordText(out, alloc, coords[i], flags)?;
        i += 1;
    }
    out.append(alloc, ')') catch return WktError.OutOfMemory;
}

fn appendPolygonBodyText(out: *std.ArrayList(u8), alloc: std.mem.Allocator, poly: WktPolygon, flags: DimFlags): void ! WktError {
    out.append(alloc, '(') catch return WktError.OutOfMemory;
    let i: usize = 0;
    while (i < poly.rings.len) {
        if (i > 0) out.appendSlice(alloc, ", ") catch return WktError.OutOfMemory;
        appendCoordListText(out, alloc, poly.rings[i], flags)?;
        i += 1;
    }
    out.append(alloc, ')') catch return WktError.OutOfMemory;
}

fn formatWktInto(out: *std.ArrayList(u8), alloc: std.mem.Allocator, geom: WktGeometry, depth: usize): void ! WktError {
    if (depth >= MAX_DEPTH) return WktError.MaxDepthExceeded;

    switch (geom) {
        .point => |pt_opt| {
            if (pt := pt_opt) {
                let flags = DimFlags{};
                scanCoordDim(pt, &flags);
                out.appendSlice(alloc, "POINT") catch return WktError.OutOfMemory;
                appendDimSuffix(out, alloc, flags)?;
                out.appendSlice(alloc, " (") catch return WktError.OutOfMemory;
                appendCoordText(out, alloc, pt, flags)?;
                out.append(alloc, ')') catch return WktError.OutOfMemory;
            } else {
                out.appendSlice(alloc, "POINT EMPTY") catch return WktError.OutOfMemory;
            }
        },
        .line_string => |pts| {
            if (pts.len == 0) {
                out.appendSlice(alloc, "LINESTRING EMPTY") catch return WktError.OutOfMemory;
                return;
            }
            let flags = DimFlags{};
            scanCoordsDim(pts, &flags);
            out.appendSlice(alloc, "LINESTRING") catch return WktError.OutOfMemory;
            appendDimSuffix(out, alloc, flags)?;
            out.append(alloc, ' ') catch return WktError.OutOfMemory;
            appendCoordListText(out, alloc, pts, flags)?;
        },
        .polygon => |poly| {
            if (poly.rings.len == 0) {
                out.appendSlice(alloc, "POLYGON EMPTY") catch return WktError.OutOfMemory;
                return;
            }
            let flags = DimFlags{};
            let r: usize = 0;
            while (r < poly.rings.len) {
                scanCoordsDim(poly.rings[r], &flags);
                r += 1;
            }
            out.appendSlice(alloc, "POLYGON") catch return WktError.OutOfMemory;
            appendDimSuffix(out, alloc, flags)?;
            out.append(alloc, ' ') catch return WktError.OutOfMemory;
            appendPolygonBodyText(out, alloc, poly, flags)?;
        },
        .multi_point => |pts| {
            if (pts.len == 0) {
                out.appendSlice(alloc, "MULTIPOINT EMPTY") catch return WktError.OutOfMemory;
                return;
            }
            let flags = DimFlags{};
            let i: usize = 0;
            while (i < pts.len) {
                if (pt := pts[i]) {
                    scanCoordDim(pt, &flags);
                }
                i += 1;
            }
            out.appendSlice(alloc, "MULTIPOINT") catch return WktError.OutOfMemory;
            appendDimSuffix(out, alloc, flags)?;
            out.appendSlice(alloc, " (") catch return WktError.OutOfMemory;
            i = 0;
            while (i < pts.len) {
                if (i > 0) out.appendSlice(alloc, ", ") catch return WktError.OutOfMemory;
                if (pt := pts[i]) {
                    out.append(alloc, '(') catch return WktError.OutOfMemory;
                    appendCoordText(out, alloc, pt, flags)?;
                    out.append(alloc, ')') catch return WktError.OutOfMemory;
                } else {
                    out.appendSlice(alloc, "EMPTY") catch return WktError.OutOfMemory;
                }
                i += 1;
            }
            out.append(alloc, ')') catch return WktError.OutOfMemory;
        },
        .multi_line_string => |lines| {
            if (lines.len == 0) {
                out.appendSlice(alloc, "MULTILINESTRING EMPTY") catch return WktError.OutOfMemory;
                return;
            }
            let flags = DimFlags{};
            let i: usize = 0;
            while (i < lines.len) {
                scanCoordsDim(lines[i], &flags);
                i += 1;
            }
            out.appendSlice(alloc, "MULTILINESTRING") catch return WktError.OutOfMemory;
            appendDimSuffix(out, alloc, flags)?;
            out.appendSlice(alloc, " (") catch return WktError.OutOfMemory;
            i = 0;
            while (i < lines.len) {
                if (i > 0) out.appendSlice(alloc, ", ") catch return WktError.OutOfMemory;
                appendCoordListText(out, alloc, lines[i], flags)?;
                i += 1;
            }
            out.append(alloc, ')') catch return WktError.OutOfMemory;
        },
        .multi_polygon => |polys| {
            if (polys.len == 0) {
                out.appendSlice(alloc, "MULTIPOLYGON EMPTY") catch return WktError.OutOfMemory;
                return;
            }
            let flags = DimFlags{};
            let i: usize = 0;
            while (i < polys.len) {
                let r: usize = 0;
                while (r < polys[i].rings.len) {
                    scanCoordsDim(polys[i].rings[r], &flags);
                    r += 1;
                }
                i += 1;
            }
            out.appendSlice(alloc, "MULTIPOLYGON") catch return WktError.OutOfMemory;
            appendDimSuffix(out, alloc, flags)?;
            out.appendSlice(alloc, " (") catch return WktError.OutOfMemory;
            i = 0;
            while (i < polys.len) {
                if (i > 0) out.appendSlice(alloc, ", ") catch return WktError.OutOfMemory;
                appendPolygonBodyText(out, alloc, polys[i], flags)?;
                i += 1;
            }
            out.append(alloc, ')') catch return WktError.OutOfMemory;
        },
        .geometry_collection => |geoms| {
            if (geoms.len == 0) {
                out.appendSlice(alloc, "GEOMETRYCOLLECTION EMPTY") catch return WktError.OutOfMemory;
                return;
            }
            out.appendSlice(alloc, "GEOMETRYCOLLECTION (") catch return WktError.OutOfMemory;
            let i: usize = 0;
            while (i < geoms.len) {
                if (i > 0) out.appendSlice(alloc, ", ") catch return WktError.OutOfMemory;
                formatWktInto(out, alloc, geoms[i], depth + 1)?;
                i += 1;
            }
            out.append(alloc, ')') catch return WktError.OutOfMemory;
        },
    }
}

/// Formats a `WktGeometry` into canonical OGC WKT text.
pub fn formatWkt(allocator: std.mem.Allocator, geom: WktGeometry): ([]u8) ! WktError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    formatWktInto(&out, allocator, geom, 0)?;
    return out.toOwnedSlice(allocator) catch return WktError.OutOfMemory;
}

// ---------------------------------------------------------------------------
// WKB Encoder
// ---------------------------------------------------------------------------

fn appendU32Wkb(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: u32, order: WkbByteOrder): void ! WktError {
    let b: [4]u8 = [0, 0, 0, 0];
    if (order == .LittleEndian) {
        b[0] = (val & 0xFF) as u8;
        b[1] = ((val >> 8) & 0xFF) as u8;
        b[2] = ((val >> 16) & 0xFF) as u8;
        b[3] = ((val >> 24) & 0xFF) as u8;
    } else {
        b[0] = ((val >> 24) & 0xFF) as u8;
        b[1] = ((val >> 16) & 0xFF) as u8;
        b[2] = ((val >> 8) & 0xFF) as u8;
        b[3] = (val & 0xFF) as u8;
    }
    out.appendSlice(alloc, &b) catch return WktError.OutOfMemory;
}

fn appendF64Wkb(out: *std.ArrayList(u8), alloc: std.mem.Allocator, val: f64, order: WkbByteOrder): void ! WktError {
    const bits: u64 = @bitCast(val);
    let b: [8]u8 = [0, 0, 0, 0, 0, 0, 0, 0];
    if (order == .LittleEndian) {
        let i: usize = 0;
        while (i < 8) {
            const shift: u6 = @intCast(i * 8);
            b[i] = ((bits >> shift) & 0xFF) as u8;
            i += 1;
        }
    } else {
        let i: usize = 0;
        while (i < 8) {
            const shift: u6 = @intCast((7 - i) * 8);
            b[i] = ((bits >> shift) & 0xFF) as u8;
            i += 1;
        }
    }
    out.appendSlice(alloc, &b) catch return WktError.OutOfMemory;
}

fn appendWkbHeader(out: *std.ArrayList(u8), alloc: std.mem.Allocator, base_type: u32, flags: DimFlags, order: WkbByteOrder): void ! WktError {
    const bo_byte: u8 = if (order == .LittleEndian) 1 else 0;
    out.append(alloc, bo_byte) catch return WktError.OutOfMemory;
    let wkb_type: u32 = base_type;
    if (flags.has_z) wkb_type += 1000;
    if (flags.has_m) wkb_type += 2000;
    appendU32Wkb(out, alloc, wkb_type, order)?;
}

fn appendCoordWkb(out: *std.ArrayList(u8), alloc: std.mem.Allocator, c: WktCoord, flags: DimFlags, order: WkbByteOrder): void ! WktError {
    appendF64Wkb(out, alloc, c.x, order)?;
    appendF64Wkb(out, alloc, c.y, order)?;
    if (flags.has_z) appendF64Wkb(out, alloc, c.z orelse 0.0, order)?;
    if (flags.has_m) appendF64Wkb(out, alloc, c.m orelse 0.0, order)?;
}

fn appendPolygonWkbBody(out: *std.ArrayList(u8), alloc: std.mem.Allocator, poly: WktPolygon, flags: DimFlags, order: WkbByteOrder): void ! WktError {
    appendU32Wkb(out, alloc, poly.rings.len as u32, order)?;
    let r: usize = 0;
    while (r < poly.rings.len) {
        const ring = poly.rings[r];
        appendU32Wkb(out, alloc, ring.len as u32, order)?;
        let j: usize = 0;
        while (j < ring.len) {
            appendCoordWkb(out, alloc, ring[j], flags, order)?;
            j += 1;
        }
        r += 1;
    }
}

fn encodeWkbInto(out: *std.ArrayList(u8), alloc: std.mem.Allocator, geom: WktGeometry, order: WkbByteOrder, depth: usize): void ! WktError {
    if (depth >= MAX_DEPTH) return WktError.MaxDepthExceeded;

    switch (geom) {
        .point => |pt_opt| {
            let flags = DimFlags{};
            if (pt := pt_opt) {
                scanCoordDim(pt, &flags);
                appendWkbHeader(out, alloc, 1, flags, order)?;
                appendCoordWkb(out, alloc, pt, flags, order)?;
            } else {
                appendWkbHeader(out, alloc, 1, flags, order)?;
                const nan_val = std.math.nan(f64);
                appendF64Wkb(out, alloc, nan_val, order)?;
                appendF64Wkb(out, alloc, nan_val, order)?;
            }
        },
        .line_string => |pts| {
            let flags = DimFlags{};
            scanCoordsDim(pts, &flags);
            appendWkbHeader(out, alloc, 2, flags, order)?;
            appendU32Wkb(out, alloc, pts.len as u32, order)?;
            let i: usize = 0;
            while (i < pts.len) {
                appendCoordWkb(out, alloc, pts[i], flags, order)?;
                i += 1;
            }
        },
        .polygon => |poly| {
            let flags = DimFlags{};
            let r: usize = 0;
            while (r < poly.rings.len) {
                scanCoordsDim(poly.rings[r], &flags);
                r += 1;
            }
            appendWkbHeader(out, alloc, 3, flags, order)?;
            appendPolygonWkbBody(out, alloc, poly, flags, order)?;
        },
        .multi_point => |pts| {
            let flags = DimFlags{};
            let i: usize = 0;
            while (i < pts.len) {
                if (pt := pts[i]) scanCoordDim(pt, &flags);
                i += 1;
            }
            appendWkbHeader(out, alloc, 4, flags, order)?;
            appendU32Wkb(out, alloc, pts.len as u32, order)?;
            i = 0;
            while (i < pts.len) {
                appendWkbHeader(out, alloc, 1, flags, order)?;
                if (pt := pts[i]) {
                    appendCoordWkb(out, alloc, pt, flags, order)?;
                } else {
                    const nan_val = std.math.nan(f64);
                    appendF64Wkb(out, alloc, nan_val, order)?;
                    appendF64Wkb(out, alloc, nan_val, order)?;
                    if (flags.has_z) appendF64Wkb(out, alloc, nan_val, order)?;
                    if (flags.has_m) appendF64Wkb(out, alloc, nan_val, order)?;
                }
                i += 1;
            }
        },
        .multi_line_string => |lines| {
            let flags = DimFlags{};
            let i: usize = 0;
            while (i < lines.len) {
                scanCoordsDim(lines[i], &flags);
                i += 1;
            }
            appendWkbHeader(out, alloc, 5, flags, order)?;
            appendU32Wkb(out, alloc, lines.len as u32, order)?;
            i = 0;
            while (i < lines.len) {
                appendWkbHeader(out, alloc, 2, flags, order)?;
                appendU32Wkb(out, alloc, lines[i].len as u32, order)?;
                let j: usize = 0;
                while (j < lines[i].len) {
                    appendCoordWkb(out, alloc, lines[i][j], flags, order)?;
                    j += 1;
                }
                i += 1;
            }
        },
        .multi_polygon => |polys| {
            let flags = DimFlags{};
            let i: usize = 0;
            while (i < polys.len) {
                let r: usize = 0;
                while (r < polys[i].rings.len) {
                    scanCoordsDim(polys[i].rings[r], &flags);
                    r += 1;
                }
                i += 1;
            }
            appendWkbHeader(out, alloc, 6, flags, order)?;
            appendU32Wkb(out, alloc, polys.len as u32, order)?;
            i = 0;
            while (i < polys.len) {
                appendWkbHeader(out, alloc, 3, flags, order)?;
                appendPolygonWkbBody(out, alloc, polys[i], flags, order)?;
                i += 1;
            }
        },
        .geometry_collection => |geoms| {
            appendWkbHeader(out, alloc, 7, DimFlags{}, order)?;
            appendU32Wkb(out, alloc, geoms.len as u32, order)?;
            let i: usize = 0;
            while (i < geoms.len) {
                encodeWkbInto(out, alloc, geoms[i], order, depth + 1)?;
                i += 1;
            }
        },
    }
}

/// Encodes a `WktGeometry` into ISO WKB binary bytes with the specified byte order.
pub fn encodeWkb(allocator: std.mem.Allocator, geom: WktGeometry, byte_order: WkbByteOrder): ([]u8) ! WktError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);
    encodeWkbInto(&out, allocator, geom, byte_order, 0)?;
    return out.toOwnedSlice(allocator) catch return WktError.OutOfMemory;
}

// ---------------------------------------------------------------------------
// WKB Decoder
// ---------------------------------------------------------------------------

struct WkbReader {
    data: []const u8,
    pos: usize = 0,

    fn readByte(*_): u8 ! WktError {
        if (_.pos >= _.data.len) return WktError.UnexpectedEof;
        const b = _.data[_.pos];
        _.pos += 1;
        return b;
    }

    fn readByteOrder(*_): WkbByteOrder ! WktError {
        const b = _.readByte()?;
        if (b == 0) return .BigEndian;
        if (b == 1) return .LittleEndian;
        return WktError.InvalidWkbHeader;
    }

    fn readU32(*_, order: WkbByteOrder): u32 ! WktError {
        if (_.pos + 4 > _.data.len) return WktError.UnexpectedEof;
        const d = _.data;
        const p = _.pos;
        _.pos += 4;
        if (order == .LittleEndian) {
            return (d[p] as u32) |
                ((d[p + 1] as u32) << 8) |
                ((d[p + 2] as u32) << 16) |
                ((d[p + 3] as u32) << 24);
        } else {
            return ((d[p] as u32) << 24) |
                ((d[p + 1] as u32) << 16) |
                ((d[p + 2] as u32) << 8) |
                (d[p + 3] as u32);
        }
    }

    fn readF64(*_, order: WkbByteOrder): f64 ! WktError {
        if (_.pos + 8 > _.data.len) return WktError.UnexpectedEof;
        const d = _.data;
        const p = _.pos;
        _.pos += 8;
        let bits: u64 = 0;
        if (order == .LittleEndian) {
            let i: usize = 0;
            while (i < 8) {
                const shift: u6 = @intCast(i * 8);
                bits |= (d[p + i] as u64) << shift;
                i += 1;
            }
        } else {
            let i: usize = 0;
            while (i < 8) {
                const shift: u6 = @intCast((7 - i) * 8);
                bits |= (d[p + i] as u64) << shift;
                i += 1;
            }
        }
        return @bitCast(bits);
    }

    fn readCoord(*_, order: WkbByteOrder, flags: DimFlags, allow_nan: bool): WktCoord? ! WktError {
        const x = _.readF64(order)?;
        const y = _.readF64(order)?;
        let z: f64? = null;
        let m: f64? = null;
        if (flags.has_z) z = _.readF64(order)?;
        if (flags.has_m) m = _.readF64(order)?;

        if (allow_nan and std.math.isNan(x) and std.math.isNan(y)) {
            return null;
        }
        if (!std.math.isFinite(x) or !std.math.isFinite(y)) {
            return WktError.InvalidCoordinate;
        }
        if (zv := z) {
            if (!std.math.isFinite(zv)) return WktError.InvalidCoordinate;
        }
        if (mv := m) {
            if (!std.math.isFinite(mv)) return WktError.InvalidCoordinate;
        }
        return WktCoord{ x: x, y: y, z: z, m: m };
    }

    fn readCoordSlice(*_, alloc: std.mem.Allocator, order: WkbByteOrder, flags: DimFlags): ([]WktCoord) ! WktError {
        const count = _.readU32(order)? as usize;
        let coord_dims: usize = 2;
        if (flags.has_z) coord_dims += 1;
        if (flags.has_m) coord_dims += 1;
        const stride: usize = coord_dims * 8;
        if (count > (_.data.len - _.pos) / stride) {
            return WktError.UnexpectedEof;
        }
        if (count == 0) return emptyCoordSlice(alloc)?;
        let pts = alloc.alloc(WktCoord, count) catch return WktError.OutOfMemory;
        errdefer alloc.free(pts);
        let i: usize = 0;
        while (i < count) {
            const c = _.readCoord(order, flags, false)?;
            pts[i] = c.?;
            i += 1;
        }
        return pts;
    }

    fn readPolygonBody(*_, alloc: std.mem.Allocator, order: WkbByteOrder, flags: DimFlags): WktPolygon ! WktError {
        const num_rings = _.readU32(order)? as usize;
        if (num_rings > (_.data.len - _.pos) / 4) {
            return WktError.UnexpectedEof;
        }
        if (num_rings == 0) return WktPolygon{ rings: emptyRingSlice(alloc)? };
        let rings = alloc.alloc([]WktCoord, num_rings) catch return WktError.OutOfMemory;
        let initialized: usize = 0;
        errdefer {
            let j: usize = 0;
            while (j < initialized) {
                if (rings[j].len > 0) alloc.free(rings[j]);
                j += 1;
            }
            alloc.free(rings);
        }

        while (initialized < num_rings) {
            const ring = _.readCoordSlice(alloc, order, flags)?;
            errdefer {
                if (ring.len > 0) alloc.free(ring);
            }
            validateRing(ring)?;
            rings[initialized] = ring;
            initialized += 1;
        }

        return WktPolygon{ rings: rings };
    }

    struct WkbHeaderInfo {
        order: WkbByteOrder,
        base_type: u32,
        flags: DimFlags,
    }

    fn readHeader(*_): WkbHeaderInfo ! WktError {
        const order = _.readByteOrder()?;
        const raw_type = _.readU32(order)?;
        const base_type = raw_type % 1000;
        const dim_code = raw_type / 1000;
        if (dim_code > 3 or base_type < 1 or base_type > 7) {
            return WktError.InvalidGeometryType;
        }
        const flags = DimFlags{
            has_z: dim_code == 1 or dim_code == 3,
            has_m: dim_code == 2 or dim_code == 3,
        };
        return WkbHeaderInfo{ order: order, base_type: base_type, flags: flags };
    }

    fn decodeGeometry(*_, alloc: std.mem.Allocator, depth: usize): WktGeometry ! WktError {
        if (depth >= MAX_DEPTH) return WktError.MaxDepthExceeded;

        const hdr = _.readHeader()?;
        const order = hdr.order;
        const flags = hdr.flags;

        switch (hdr.base_type) {
            1 => {
                const pt = _.readCoord(order, flags, true)?;
                return WktGeometry{ .point = pt };
            },
            2 => {
                const pts = _.readCoordSlice(alloc, order, flags)?;
                return WktGeometry{ .line_string = pts };
            },
            3 => {
                const poly = _.readPolygonBody(alloc, order, flags)?;
                return WktGeometry{ .polygon = poly };
            },
            4 => {
                const count = _.readU32(order)? as usize;
                if (count > (_.data.len - _.pos) / 5) return WktError.UnexpectedEof;
                if (count == 0) return WktGeometry{ .multi_point = emptyOptCoordSlice(alloc)? };
                let pts = alloc.alloc(OptWktCoord, count) catch return WktError.OutOfMemory;
                errdefer alloc.free(pts);
                let i: usize = 0;
                while (i < count) {
                    const sub_hdr = _.readHeader()?;
                    if (sub_hdr.base_type != 1) return WktError.InvalidGeometryType;
                    pts[i] = _.readCoord(sub_hdr.order, sub_hdr.flags, true)?;
                    i += 1;
                }
                return WktGeometry{ .multi_point = pts };
            },
            5 => {
                const count = _.readU32(order)? as usize;
                if (count > (_.data.len - _.pos) / 5) return WktError.UnexpectedEof;
                if (count == 0) return WktGeometry{ .multi_line_string = emptyRingSlice(alloc)? };
                let lines = alloc.alloc([]WktCoord, count) catch return WktError.OutOfMemory;
                let initialized: usize = 0;
                errdefer {
                    let j: usize = 0;
                    while (j < initialized) {
                        if (lines[j].len > 0) alloc.free(lines[j]);
                        j += 1;
                    }
                    alloc.free(lines);
                }
                while (initialized < count) {
                    const sub_hdr = _.readHeader()?;
                    if (sub_hdr.base_type != 2) return WktError.InvalidGeometryType;
                    lines[initialized] = _.readCoordSlice(alloc, sub_hdr.order, sub_hdr.flags)?;
                    initialized += 1;
                }
                return WktGeometry{ .multi_line_string = lines };
            },
            6 => {
                const count = _.readU32(order)? as usize;
                if (count > (_.data.len - _.pos) / 5) return WktError.UnexpectedEof;
                if (count == 0) return WktGeometry{ .multi_polygon = emptyPolygonSlice(alloc)? };
                let polys = alloc.alloc(WktPolygon, count) catch return WktError.OutOfMemory;
                let initialized: usize = 0;
                errdefer {
                    let j: usize = 0;
                    while (j < initialized) {
                        polys[j].deinit(alloc);
                        j += 1;
                    }
                    alloc.free(polys);
                }
                while (initialized < count) {
                    const sub_hdr = _.readHeader()?;
                    if (sub_hdr.base_type != 3) return WktError.InvalidGeometryType;
                    polys[initialized] = _.readPolygonBody(alloc, sub_hdr.order, sub_hdr.flags)?;
                    initialized += 1;
                }
                return WktGeometry{ .multi_polygon = polys };
            },
            7 => {
                const count = _.readU32(order)? as usize;
                if (count > (_.data.len - _.pos) / 5) return WktError.UnexpectedEof;
                if (count == 0) return WktGeometry{ .geometry_collection = emptyGeomSlice(alloc)? };
                let geoms = alloc.alloc(WktGeometry, count) catch return WktError.OutOfMemory;
                let initialized: usize = 0;
                errdefer {
                    let j: usize = 0;
                    while (j < initialized) {
                        geoms[j].deinit(alloc);
                        j += 1;
                    }
                    alloc.free(geoms);
                }
                while (initialized < count) {
                    geoms[initialized] = _.decodeGeometry(alloc, depth + 1)?;
                    initialized += 1;
                }
                return WktGeometry{ .geometry_collection = geoms };
            },
            else => return WktError.InvalidGeometryType,
        }
    }
}

/// Decodes ISO WKB binary bytes into a `WktGeometry`.
pub fn decodeWkb(allocator: std.mem.Allocator, wkb_bytes: []const u8): WktGeometry ! WktError {
    let reader = WkbReader{ data: wkb_bytes, pos: 0 };
    let geom = reader.decodeGeometry(allocator, 0)?;
    errdefer geom.deinit(allocator);
    if (reader.pos != wkb_bytes.len) {
        return WktError.InvalidWkbHeader;
    }
    return geom;
}
