// Source: cp/lib/geojson/geojson.cp (RFC 7946 GeoJSON Codec, Geofence & Simplifier)
// Pure CP zero-C-dependency RFC 7946 GeoJSON parser, canonical serializer,
// bounding box calculator, ray-casting point-in-polygon with hole support,
// and iterative Douglas-Peucker polyline simplifier.

const std = @import("std");

const BOUNDARY_EPS: f64 = 0.000000000001;

/// Unified error set for `cap:geojson` parsing, validation, and simplification.
pub err GeoJsonError {
    InvalidJson,
    InvalidType,
    InvalidCoordinates,
    InvalidRing,
    OutOfMemory,
}

/// Geographic coordinate position (`lon`, `lat`, optional `alt`).
pub struct Position {
    pub lon: f64 = 0.0,
    pub lat: f64 = 0.0,
    pub alt: f64? = null,
}

/// Axis-aligned 2D bounding box `[min_lon, min_lat, max_lon, max_lat]`.
pub struct BBox {
    pub min_lon: f64 = 0.0,
    pub min_lat: f64 = 0.0,
    pub max_lon: f64 = 0.0,
    pub max_lat: f64 = 0.0,
}

/// Polygon coordinate rings: `rings[0]` is the exterior ring, `rings[1..]` are interior holes.
pub struct PolygonData {
    pub rings: [][]Position,

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

/// RFC 7946 GeoJSON Geometry tagged union.
pub enum Geometry {
    point: Position,
    multi_point: []Position,
    line_string: []Position,
    multi_line_string: [][]Position,
    polygon: PolygonData,
    multi_polygon: []PolygonData,
    geometry_collection: []Geometry,

    /// Recursively releases all heap-allocated coordinate slices and child geometries.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        switch (_.*) {
            .point => {},
            .multi_point => |pts| {
                if (pts.len > 0) allocator.free(pts);
            },
            .line_string => |pts| {
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
            .polygon => |*poly| {
                poly.deinit(allocator);
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

/// RFC 7946 GeoJSON Feature object.
pub struct Feature {
    pub id: ([]const u8)? = null,
    pub geometry: Geometry? = null,
    pub properties_json: []const u8 = "null",

    /// Releases owned `id`, `geometry`, and `properties_json` buffers.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        if (id_val := _.id) {
            allocator.free(id_val);
            _.id = null;
        }
        if (_.geometry != null) {
            _.geometry.?.deinit(allocator);
            _.geometry = null;
        }
        if (_.properties_json.len > 0) {
            allocator.free(_.properties_json);
            _.properties_json = "";
        }
    }
}

/// RFC 7946 GeoJSON FeatureCollection object.
pub struct FeatureCollection {
    pub features: []Feature,

    /// Releases all owned `Feature` items and the `features` slice.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        let i: usize = 0;
        while (i < _.features.len) {
            _.features[i].deinit(allocator);
            i += 1;
        }
        if (_.features.len > 0) allocator.free(_.features);
        _.features = _.features[0..0];
    }
}

/// Top-level RFC 7946 GeoJSON document tagged union.
pub enum GeoJsonDoc {
    geometry: Geometry,
    feature: Feature,
    feature_collection: FeatureCollection,

    /// Releases all heap-allocated memory owned by this GeoJSON document.
    pub fn deinit(*_, allocator: std.mem.Allocator) {
        switch (_.*) {
            .geometry => |*g| g.deinit(allocator),
            .feature => |*f| f.deinit(allocator),
            .feature_collection => |*fc| fc.deinit(allocator),
        }
    }
}

fn isFiniteF64(v: f64): bool {
    return !std.math.isNan(v) && !std.math.isInf(v);
}

fn optF64Equal(a: f64?, b: f64?): bool {
    if (va := a) {
        if (vb := b) {
            return va == vb;
        }
        return false;
    }
    return b == null;
}

fn positionsEqual(a: Position, b: Position): bool {
    return a.lon == b.lon && a.lat == b.lat && optF64Equal(a.alt, b.alt);
}

fn parseJsonF64(val: std.json.Value): f64 ! GeoJsonError {
    switch (val) {
        .integer => |i| {
            return @floatFromInt(i);
        },
        .float => |f| {
            if (!isFiniteF64(f)) return GeoJsonError.InvalidCoordinates;
            return f;
        },
        .number_string => |s| {
            const parsed = std.fmt.parseFloat(f64, s) catch return GeoJsonError.InvalidCoordinates;
            if (!isFiniteF64(parsed)) return GeoJsonError.InvalidCoordinates;
            return parsed;
        },
        else => return GeoJsonError.InvalidCoordinates,
    }
}

fn parsePosition(val: std.json.Value): Position ! GeoJsonError {
    let arr = switch (val) {
        .array => |a| a,
        else => return GeoJsonError.InvalidCoordinates,
    };
    if (arr.items.len < 2) {
        return GeoJsonError.InvalidCoordinates;
    }
    const lon = parseJsonF64(arr.items[0])?;
    const lat = parseJsonF64(arr.items[1])?;
    let alt: f64? = null;
    if (arr.items.len >= 3) {
        alt = parseJsonF64(arr.items[2])?;
    }
    return Position{ lon: lon, lat: lat, alt: alt };
}

fn parsePositionList(allocator: std.mem.Allocator, val: std.json.Value): ([]Position) ! GeoJsonError {
    let arr = switch (val) {
        .array => |a| a,
        else => return GeoJsonError.InvalidCoordinates,
    };
    let out = allocator.alloc(Position, arr.items.len) catch return GeoJsonError.OutOfMemory;
    errdefer allocator.free(out);
    let i: usize = 0;
    while (i < arr.items.len) {
        out[i] = parsePosition(arr.items[i])?;
        i += 1;
    }
    return out;
}

fn parseMultiPositionList(allocator: std.mem.Allocator, val: std.json.Value): ([][]Position) ! GeoJsonError {
    let arr = switch (val) {
        .array => |a| a,
        else => return GeoJsonError.InvalidCoordinates,
    };
    let out = allocator.alloc([]Position, arr.items.len) catch return GeoJsonError.OutOfMemory;
    let count: usize = 0;
    errdefer {
        let j: usize = 0;
        while (j < count) {
            if (out[j].len > 0) allocator.free(out[j]);
            j += 1;
        }
        allocator.free(out);
    }
    while (count < arr.items.len) {
        out[count] = parsePositionList(allocator, arr.items[count])?;
        count += 1;
    }
    return out;
}

fn parseRing(allocator: std.mem.Allocator, val: std.json.Value): ([]Position) ! GeoJsonError {
    let arr = switch (val) {
        .array => |a| a,
        else => return GeoJsonError.InvalidCoordinates,
    };
    if (arr.items.len < 4) {
        return GeoJsonError.InvalidRing;
    }
    let pts = allocator.alloc(Position, arr.items.len) catch return GeoJsonError.OutOfMemory;
    errdefer allocator.free(pts);
    let i: usize = 0;
    while (i < arr.items.len) {
        pts[i] = parsePosition(arr.items[i])?;
        i += 1;
    }
    if (!positionsEqual(pts[0], pts[pts.len - 1])) {
        return GeoJsonError.InvalidRing;
    }
    return pts;
}

fn parsePolygonData(allocator: std.mem.Allocator, val: std.json.Value): PolygonData ! GeoJsonError {
    let arr = switch (val) {
        .array => |a| a,
        else => return GeoJsonError.InvalidCoordinates,
    };
    if (arr.items.len == 0) {
        return GeoJsonError.InvalidRing;
    }
    let rings = allocator.alloc([]Position, arr.items.len) catch return GeoJsonError.OutOfMemory;
    let count: usize = 0;
    errdefer {
        let j: usize = 0;
        while (j < count) {
            if (rings[j].len > 0) allocator.free(rings[j]);
            j += 1;
        }
        allocator.free(rings);
    }
    while (count < arr.items.len) {
        rings[count] = parseRing(allocator, arr.items[count])?;
        count += 1;
    }
    return PolygonData{ rings: rings };
}

fn parseMultiPolygonData(allocator: std.mem.Allocator, val: std.json.Value): ([]PolygonData) ! GeoJsonError {
    let arr = switch (val) {
        .array => |a| a,
        else => return GeoJsonError.InvalidCoordinates,
    };
    let polys = allocator.alloc(PolygonData, arr.items.len) catch return GeoJsonError.OutOfMemory;
    let count: usize = 0;
    errdefer {
        let j: usize = 0;
        while (j < count) {
            polys[j].deinit(allocator);
            j += 1;
        }
        allocator.free(polys);
    }
    while (count < arr.items.len) {
        polys[count] = parsePolygonData(allocator, arr.items[count])?;
        count += 1;
    }
    return polys;
}

fn getTypeString(obj: std.json.ObjectMap): ([]const u8) ! GeoJsonError {
    if (type_val := obj.get("type")) {
        switch (type_val) {
            .string => |s| return s,
            else => return GeoJsonError.InvalidType,
        }
    }
    return GeoJsonError.InvalidType;
}

fn getCoordinatesField(obj: std.json.ObjectMap): std.json.Value ! GeoJsonError {
    if (coords := obj.get("coordinates")) {
        return coords;
    }
    return GeoJsonError.InvalidCoordinates;
}

fn parseGeometry(allocator: std.mem.Allocator, obj: std.json.ObjectMap): Geometry ! GeoJsonError {
    const type_str = getTypeString(obj)?;
    if (std.mem.eql(u8, type_str, "Point")) {
        const coords = getCoordinatesField(obj)?;
        const pos = parsePosition(coords)?;
        return Geometry{ .point = pos };
    }
    if (std.mem.eql(u8, type_str, "MultiPoint")) {
        const coords = getCoordinatesField(obj)?;
        const pts = parsePositionList(allocator, coords)?;
        return Geometry{ .multi_point = pts };
    }
    if (std.mem.eql(u8, type_str, "LineString")) {
        const coords = getCoordinatesField(obj)?;
        const pts = parsePositionList(allocator, coords)?;
        return Geometry{ .line_string = pts };
    }
    if (std.mem.eql(u8, type_str, "MultiLineString")) {
        const coords = getCoordinatesField(obj)?;
        const lines = parseMultiPositionList(allocator, coords)?;
        return Geometry{ .multi_line_string = lines };
    }
    if (std.mem.eql(u8, type_str, "Polygon")) {
        const coords = getCoordinatesField(obj)?;
        const poly = parsePolygonData(allocator, coords)?;
        return Geometry{ .polygon = poly };
    }
    if (std.mem.eql(u8, type_str, "MultiPolygon")) {
        const coords = getCoordinatesField(obj)?;
        const polys = parseMultiPolygonData(allocator, coords)?;
        return Geometry{ .multi_polygon = polys };
    }
    if (std.mem.eql(u8, type_str, "GeometryCollection")) {
        const geoms_val = obj.get("geometries") orelse return GeoJsonError.InvalidType;
        const arr = switch (geoms_val) {
            .array => |a| a,
            else => return GeoJsonError.InvalidType,
        };
        let geoms = allocator.alloc(Geometry, arr.items.len) catch return GeoJsonError.OutOfMemory;
        let count: usize = 0;
        errdefer {
            let j: usize = 0;
            while (j < count) {
                geoms[j].deinit(allocator);
                j += 1;
            }
            allocator.free(geoms);
        }
        while (count < arr.items.len) {
            const g_obj = switch (arr.items[count]) {
                .object => |o| o,
                else => return GeoJsonError.InvalidType,
            };
            geoms[count] = parseGeometry(allocator, g_obj)?;
            count += 1;
        }
        return Geometry{ .geometry_collection = geoms };
    }
    return GeoJsonError.InvalidType;
}

fn parseFeature(allocator: std.mem.Allocator, obj: std.json.ObjectMap): Feature ! GeoJsonError {
    let owned_id: ([]const u8)? = null;
    errdefer {
        if (id_v := owned_id) {
            allocator.free(id_v);
        }
    }
    if (id_val := obj.get("id")) {
        switch (id_val) {
            .null => {},
            .string => |s| {
                owned_id = allocator.dupe(u8, s) catch return GeoJsonError.OutOfMemory;
            },
            .integer => |i| {
                owned_id = std.fmt.allocPrint(allocator, "{d}", .{i}) catch return GeoJsonError.OutOfMemory;
            },
            .float => |f| {
                if (!isFiniteF64(f)) return GeoJsonError.InvalidJson;
                owned_id = std.fmt.allocPrint(allocator, "{d}", .{f}) catch return GeoJsonError.OutOfMemory;
            },
            .number_string => |s| {
                owned_id = allocator.dupe(u8, s) catch return GeoJsonError.OutOfMemory;
            },
            else => return GeoJsonError.InvalidType,
        }
    }

    let geom: Geometry? = null;
    errdefer {
        if (geom != null) {
            geom.?.deinit(allocator);
        }
    }
    if (geom_val := obj.get("geometry")) {
        switch (geom_val) {
            .null => {},
            .object => |g_obj| {
                geom = parseGeometry(allocator, g_obj)?;
            },
            else => return GeoJsonError.InvalidType,
        }
    }

    let props_json: []const u8 = undefined;
    if (props_val := obj.get("properties")) {
        props_json = std.json.Stringify.valueAlloc(allocator, props_val, .{}) catch return GeoJsonError.OutOfMemory;
    } else {
        props_json = allocator.dupe(u8, "null") catch return GeoJsonError.OutOfMemory;
    }

    return Feature{
        id: owned_id,
        geometry: geom,
        properties_json: props_json,
    };
}

fn parseFeatureCollection(allocator: std.mem.Allocator, obj: std.json.ObjectMap): FeatureCollection ! GeoJsonError {
    const feats_val = obj.get("features") orelse return GeoJsonError.InvalidType;
    const arr = switch (feats_val) {
        .array => |a| a,
        else => return GeoJsonError.InvalidType,
    };
    let features = allocator.alloc(Feature, arr.items.len) catch return GeoJsonError.OutOfMemory;
    let count: usize = 0;
    errdefer {
        let j: usize = 0;
        while (j < count) {
            features[j].deinit(allocator);
            j += 1;
        }
        allocator.free(features);
    }
    while (count < arr.items.len) {
        const f_obj = switch (arr.items[count]) {
            .object => |o| o,
            else => return GeoJsonError.InvalidType,
        };
        const f_type = getTypeString(f_obj)?;
        if (!std.mem.eql(u8, f_type, "Feature")) {
            return GeoJsonError.InvalidType;
        }
        features[count] = parseFeature(allocator, f_obj)?;
        count += 1;
    }
    return FeatureCollection{ features: features };
}

/// Parses an RFC 7946 GeoJSON string into a strongly-typed `GeoJsonDoc`.
/// Validates `"type"`, coordinate finite numbers (>= 2 elements), and closed Polygon rings (>= 4 points).
pub fn parse(allocator: std.mem.Allocator, json_str: []const u8): GeoJsonDoc ! GeoJsonError {
    let parsed = std.json.parseFromSlice(std.json.Value, allocator, json_str, .{}) catch e => {
        if (e == error.OutOfMemory) return GeoJsonError.OutOfMemory;
        return GeoJsonError.InvalidJson;
    };
    defer parsed.deinit();

    const root_obj = switch (parsed.value) {
        .object => |o| o,
        else => return GeoJsonError.InvalidType,
    };

    const type_str = getTypeString(root_obj)?;
    if (std.mem.eql(u8, type_str, "Feature")) {
        const feat = parseFeature(allocator, root_obj)?;
        return GeoJsonDoc{ .feature = feat };
    }
    if (std.mem.eql(u8, type_str, "FeatureCollection")) {
        const fc = parseFeatureCollection(allocator, root_obj)?;
        return GeoJsonDoc{ .feature_collection = fc };
    }
    const geom = parseGeometry(allocator, root_obj)?;
    return GeoJsonDoc{ .geometry = geom };
}

fn appendStr(out: *std.ArrayList(u8), alloc: std.mem.Allocator, s: []const u8): void ! GeoJsonError {
    out.appendSlice(alloc, s) catch return GeoJsonError.OutOfMemory;
}

fn appendF64(out: *std.ArrayList(u8), alloc: std.mem.Allocator, v: f64): void ! GeoJsonError {
    if (!isFiniteF64(v)) return GeoJsonError.InvalidCoordinates;
    let buf: [64]u8 = undefined;
    const formatted = std.fmt.bufPrint(&buf, "{d}", .{v}) catch return GeoJsonError.OutOfMemory;
    appendStr(out, alloc, formatted)?;
}

fn appendPosition(out: *std.ArrayList(u8), alloc: std.mem.Allocator, pos: Position): void ! GeoJsonError {
    appendStr(out, alloc, "[")?;
    appendF64(out, alloc, pos.lon)?;
    appendStr(out, alloc, ",")?;
    appendF64(out, alloc, pos.lat)?;
    if (alt := pos.alt) {
        appendStr(out, alloc, ",")?;
        appendF64(out, alloc, alt)?;
    }
    appendStr(out, alloc, "]")?;
}

fn appendPositionSlice(out: *std.ArrayList(u8), alloc: std.mem.Allocator, pts: []const Position): void ! GeoJsonError {
    appendStr(out, alloc, "[")?;
    let i: usize = 0;
    while (i < pts.len) {
        if (i > 0) appendStr(out, alloc, ",")?;
        appendPosition(out, alloc, pts[i])?;
        i += 1;
    }
    appendStr(out, alloc, "]")?;
}

fn appendPolygonRings(out: *std.ArrayList(u8), alloc: std.mem.Allocator, poly: PolygonData): void ! GeoJsonError {
    appendStr(out, alloc, "[")?;
    let i: usize = 0;
    while (i < poly.rings.len) {
        if (i > 0) appendStr(out, alloc, ",")?;
        appendPositionSlice(out, alloc, poly.rings[i])?;
        i += 1;
    }
    appendStr(out, alloc, "]")?;
}

fn appendBBoxField(out: *std.ArrayList(u8), alloc: std.mem.Allocator, bbox: BBox): void ! GeoJsonError {
    appendStr(out, alloc, ",\"bbox\":[")?;
    appendF64(out, alloc, bbox.min_lon)?;
    appendStr(out, alloc, ",")?;
    appendF64(out, alloc, bbox.min_lat)?;
    appendStr(out, alloc, ",")?;
    appendF64(out, alloc, bbox.max_lon)?;
    appendStr(out, alloc, ",")?;
    appendF64(out, alloc, bbox.max_lat)?;
    appendStr(out, alloc, "]")?;
}

fn appendGeometry(out: *std.ArrayList(u8), alloc: std.mem.Allocator, geom: Geometry, include_bbox: bool): void ! GeoJsonError {
    switch (geom) {
        .point => |pt| {
            appendStr(out, alloc, "{\"type\":\"Point\",\"coordinates\":")?;
            appendPosition(out, alloc, pt)?;
        },
        .multi_point => |pts| {
            appendStr(out, alloc, "{\"type\":\"MultiPoint\",\"coordinates\":")?;
            appendPositionSlice(out, alloc, pts)?;
        },
        .line_string => |pts| {
            appendStr(out, alloc, "{\"type\":\"LineString\",\"coordinates\":")?;
            appendPositionSlice(out, alloc, pts)?;
        },
        .multi_line_string => |lines| {
            appendStr(out, alloc, "{\"type\":\"MultiLineString\",\"coordinates\":[")?;
            let i: usize = 0;
            while (i < lines.len) {
                if (i > 0) appendStr(out, alloc, ",")?;
                appendPositionSlice(out, alloc, lines[i])?;
                i += 1;
            }
            appendStr(out, alloc, "]")?;
        },
        .polygon => |poly| {
            appendStr(out, alloc, "{\"type\":\"Polygon\",\"coordinates\":")?;
            appendPolygonRings(out, alloc, poly)?;
        },
        .multi_polygon => |polys| {
            appendStr(out, alloc, "{\"type\":\"MultiPolygon\",\"coordinates\":[")?;
            let i: usize = 0;
            while (i < polys.len) {
                if (i > 0) appendStr(out, alloc, ",")?;
                appendPolygonRings(out, alloc, polys[i])?;
                i += 1;
            }
            appendStr(out, alloc, "]")?;
        },
        .geometry_collection => |geoms| {
            appendStr(out, alloc, "{\"type\":\"GeometryCollection\",\"geometries\":[")?;
            let i: usize = 0;
            while (i < geoms.len) {
                if (i > 0) appendStr(out, alloc, ",")?;
                appendGeometry(out, alloc, geoms[i], false)?;
                i += 1;
            }
            appendStr(out, alloc, "]")?;
        },
    }
    if (include_bbox) {
        if (bbox := computeBBox(geom)) {
            appendBBoxField(out, alloc, bbox)?;
        }
    }
    appendStr(out, alloc, "}")?;
}

fn appendFeature(out: *std.ArrayList(u8), alloc: std.mem.Allocator, feat: Feature, include_bbox: bool): void ! GeoJsonError {
    appendStr(out, alloc, "{\"type\":\"Feature\"")?;
    if (id_val := feat.id) {
        const quoted = std.json.Stringify.valueAlloc(alloc, id_val, .{}) catch return GeoJsonError.OutOfMemory;
        defer alloc.free(quoted);
        appendStr(out, alloc, ",\"id\":")?;
        appendStr(out, alloc, quoted)?;
    }
    appendStr(out, alloc, ",\"geometry\":")?;
    if (g := feat.geometry) {
        appendGeometry(out, alloc, g, false)?;
    } else {
        appendStr(out, alloc, "null")?;
    }
    appendStr(out, alloc, ",\"properties\":")?;
    if (feat.properties_json.len > 0) {
        appendStr(out, alloc, feat.properties_json)?;
    } else {
        appendStr(out, alloc, "null")?;
    }
    if (include_bbox) {
        if (g := feat.geometry) {
            if (bbox := computeBBox(g)) {
                appendBBoxField(out, alloc, bbox)?;
            }
        }
    }
    appendStr(out, alloc, "}")?;
}

/// Canonicalizes and serializes a `GeoJsonDoc` into an owned JSON byte slice,
/// automatically embedding `"bbox"` when a non-empty bounding box exists.
pub fn stringify(allocator: std.mem.Allocator, doc: GeoJsonDoc): ([]u8) ! GeoJsonError {
    let out: std.ArrayList(u8) = .empty;
    errdefer out.deinit(allocator);

    switch (doc) {
        .geometry => |g| {
            appendGeometry(&out, allocator, g, true)?;
        },
        .feature => |f| {
            appendFeature(&out, allocator, f, true)?;
        },
        .feature_collection => |fc| {
            appendStr(&out, allocator, "{\"type\":\"FeatureCollection\",\"features\":[")?;
            let i: usize = 0;
            while (i < fc.features.len) {
                if (i > 0) appendStr(&out, allocator, ",")?;
                appendFeature(&out, allocator, fc.features[i], false)?;
                i += 1;
            }
            appendStr(&out, allocator, "]")?;
            if (bbox := computeDocBBox(doc)) {
                appendBBoxField(&out, allocator, bbox)?;
            }
            appendStr(&out, allocator, "}")?;
        },
    }

    return out.toOwnedSlice(allocator) catch return GeoJsonError.OutOfMemory;
}

fn extendBBox(bbox: *BBox?, pos: Position) {
    if (b := bbox.*) {
        let updated = b;
        if (pos.lon < updated.min_lon) updated.min_lon = pos.lon;
        if (pos.lat < updated.min_lat) updated.min_lat = pos.lat;
        if (pos.lon > updated.max_lon) updated.max_lon = pos.lon;
        if (pos.lat > updated.max_lat) updated.max_lat = pos.lat;
        bbox.* = updated;
    } else {
        bbox.* = BBox{
            min_lon: pos.lon,
            min_lat: pos.lat,
            max_lon: pos.lon,
            max_lat: pos.lat,
        };
    }
}

fn mergeBBox(dst: *BBox?, src: BBox?) {
    if (s := src) {
        extendBBox(dst, Position{ lon: s.min_lon, lat: s.min_lat, alt: null });
        extendBBox(dst, Position{ lon: s.max_lon, lat: s.max_lat, alt: null });
    }
}

/// Computes the 2D axis-aligned `BBox` of a `Geometry`, or `null` if it contains no positions.
pub fn computeBBox(geom: Geometry): BBox? {
    let bbox: BBox? = null;
    switch (geom) {
        .point => |pt| {
            extendBBox(&bbox, pt);
        },
        .multi_point => |pts| {
            for (pt of pts) extendBBox(&bbox, pt);
        },
        .line_string => |pts| {
            for (pt of pts) extendBBox(&bbox, pt);
        },
        .multi_line_string => |lines| {
            for (line of lines) {
                for (pt of line) extendBBox(&bbox, pt);
            }
        },
        .polygon => |poly| {
            for (ring of poly.rings) {
                for (pt of ring) extendBBox(&bbox, pt);
            }
        },
        .multi_polygon => |polys| {
            for (poly of polys) {
                for (ring of poly.rings) {
                    for (pt of ring) extendBBox(&bbox, pt);
                }
            }
        },
        .geometry_collection => |geoms| {
            for (g of geoms) {
                mergeBBox(&bbox, computeBBox(g));
            }
        },
    }
    return bbox;
}

/// Computes the 2D axis-aligned `BBox` of any `GeoJsonDoc`, or `null` if empty.
pub fn computeDocBBox(doc: GeoJsonDoc): BBox? {
    switch (doc) {
        .geometry => |g| return computeBBox(g),
        .feature => |f| {
            if (g := f.geometry) return computeBBox(g);
            return null;
        },
        .feature_collection => |fc| {
            let bbox: BBox? = null;
            for (f of fc.features) {
                if (g := f.geometry) {
                    mergeBBox(&bbox, computeBBox(g));
                }
            }
            return bbox;
        },
    }
}

fn pointOnSegment(px: f64, py: f64, ax: f64, ay: f64, bx: f64, by: f64): bool {
    const min_x = if (ax < bx) ax else bx;
    const max_x = if (ax > bx) ax else bx;
    const min_y = if (ay < by) ay else by;
    const max_y = if (ay > by) ay else by;

    if (px < min_x - BOUNDARY_EPS || px > max_x + BOUNDARY_EPS) return false;
    if (py < min_y - BOUNDARY_EPS || py > max_y + BOUNDARY_EPS) return false;

    const dx = bx - ax;
    const dy = by - ay;
    const len_sq = dx * dx + dy * dy;
    if (len_sq <= BOUNDARY_EPS * BOUNDARY_EPS) {
        return @abs(px - ax) <= BOUNDARY_EPS && @abs(py - ay) <= BOUNDARY_EPS;
    }

    const cross = (px - ax) * dy - (py - ay) * dx;
    return (cross * cross) <= (BOUNDARY_EPS * BOUNDARY_EPS) * len_sq;
}

fn isPointOnRing(pt: Position, ring: []const Position): bool {
    if (ring.len < 2) return false;
    let i: usize = 0;
    let j: usize = ring.len - 1;
    while (i < ring.len) {
        if (pointOnSegment(pt.lon, pt.lat, ring[j].lon, ring[j].lat, ring[i].lon, ring[i].lat)) {
            return true;
        }
        j = i;
        i += 1;
    }
    return false;
}

fn rayCastRing(pt: Position, ring: []const Position): bool {
    if (ring.len < 3) return false;
    let inside = false;
    let i: usize = 0;
    let j: usize = ring.len - 1;
    while (i < ring.len) {
        const xi = ring[i].lon;
        const yi = ring[i].lat;
        const xj = ring[j].lon;
        const yj = ring[j].lat;
        const intersects = ((yi > pt.lat) != (yj > pt.lat)) &&
            (pt.lon < (xj - xi) * (pt.lat - yi) / (yj - yi) + xi);
        if (intersects) {
            inside = !inside;
        }
        j = i;
        i += 1;
    }
    return inside;
}

/// Ray-casting + boundary geofence predicate for a `PolygonData` with optional interior holes.
/// Returns `true` if `pt` lies inside or on the exterior boundary and is NOT strictly inside any hole.
pub fn pointInPolygon(pt: Position, poly: PolygonData): bool {
    if (poly.rings.len == 0) return false;
    const outer = poly.rings[0];
    if (!isPointOnRing(pt, outer) && !rayCastRing(pt, outer)) {
        return false;
    }
    let h: usize = 1;
    while (h < poly.rings.len) {
        const hole = poly.rings[h];
        if (!isPointOnRing(pt, hole) && rayCastRing(pt, hole)) {
            return false;
        }
        h += 1;
    }
    return true;
}

fn perpendicularDistance(p: Position, a: Position, b: Position): f64 {
    const dx = b.lon - a.lon;
    const dy = b.lat - a.lat;
    const len_sq = dx * dx + dy * dy;
    if (len_sq == 0.0) {
        const px = p.lon - a.lon;
        const py = p.lat - a.lat;
        return @sqrt(px * px + py * py);
    }
    const cross = @abs((p.lon - a.lon) * dy - (p.lat - a.lat) * dx);
    return cross / @sqrt(len_sq);
}

struct SegRange {
    start: usize,
    end: usize,
}

/// Simplifies a polyline using the classic iterative Douglas-Peucker algorithm.
/// Preserves the first and last positions and any inflection points whose perpendicular
/// distance exceeds `epsilon` (in coordinate units).
pub fn simplifyDouglasPeucker(allocator: std.mem.Allocator, pts: []const Position, epsilon: f64): ([]Position) ! GeoJsonError {
    if (pts.len == 0) {
        return allocator.alloc(Position, 0) catch return GeoJsonError.OutOfMemory;
    }
    if (pts.len <= 2 || epsilon <= 0.0 || !isFiniteF64(epsilon)) {
        return allocator.dupe(Position, pts) catch return GeoJsonError.OutOfMemory;
    }

    let keep = allocator.alloc(bool, pts.len) catch return GeoJsonError.OutOfMemory;
    defer allocator.free(keep);
    @memset(keep, false);
    keep[0] = true;
    keep[pts.len - 1] = true;

    let stack: std.ArrayList(SegRange) = .empty;
    defer stack.deinit(allocator);
    stack.append(allocator, SegRange{ start: 0, end: pts.len - 1 }) catch return GeoJsonError.OutOfMemory;

    while (stack.items.len > 0) {
        const seg = stack.pop().?;
        if (seg.end <= seg.start + 1) continue;

        let max_dist: f64 = 0.0;
        let max_idx: usize = seg.start;
        let k: usize = seg.start + 1;
        while (k < seg.end) {
            const d = perpendicularDistance(pts[k], pts[seg.start], pts[seg.end]);
            if (d > max_dist) {
                max_dist = d;
                max_idx = k;
            }
            k += 1;
        }

        if (max_dist > epsilon) {
            keep[max_idx] = true;
            stack.append(allocator, SegRange{ start: seg.start, end: max_idx }) catch return GeoJsonError.OutOfMemory;
            stack.append(allocator, SegRange{ start: max_idx, end: seg.end }) catch return GeoJsonError.OutOfMemory;
        }
    }

    let kept_count: usize = 0;
    for (flag of keep) {
        if (flag) kept_count += 1;
    }

    let out = allocator.alloc(Position, kept_count) catch return GeoJsonError.OutOfMemory;
    let write_idx: usize = 0;
    let i: usize = 0;
    while (i < pts.len) {
        if (keep[i]) {
            out[write_idx] = pts[i];
            write_idx += 1;
        }
        i += 1;
    }
    return out;
}
