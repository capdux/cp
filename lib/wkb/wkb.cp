// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc OGC WKB / PostGIS EWKB Spatial Geometry Codec)

pub const WKB_XDR: u8 = 0;
pub const WKB_NDR: u8 = 1;

pub const EWKB_FLAG_Z: u32 = 0x8000_0000;
pub const EWKB_FLAG_M: u32 = 0x4000_0000;
pub const EWKB_FLAG_SRID: u32 = 0x2000_0000;

pub const MAX_WKB_DEPTH: u32 = 32;

pub enum WkbGeomType: u32 {
    unknown = 0,
    point = 1,
    linestring = 2,
    polygon = 3,
    multipoint = 4,
    multilinestring = 5,
    multipolygon = 6,
    geometrycollection = 7,
}

pub fn wkbGeomTypeFromCode(code: u32): WkbGeomType {
    if (code == 1) return WkbGeomType.point;
    if (code == 2) return WkbGeomType.linestring;
    if (code == 3) return WkbGeomType.polygon;
    if (code == 4) return WkbGeomType.multipoint;
    if (code == 5) return WkbGeomType.multilinestring;
    if (code == 6) return WkbGeomType.multipolygon;
    if (code == 7) return WkbGeomType.geometrycollection;
    return WkbGeomType.unknown;
}

pub struct WkbPoint {
    x: f64,
    y: f64,
    z: f64,
    m: f64,
    has_z: bool,
    has_m: bool,

    fn xy(x: f64, y: f64): WkbPoint {
        return WkbPoint{ .x = x, .y = y, .z = 0.0, .m = 0.0, .has_z = false, .has_m = false };
    }

    fn xyz(x: f64, y: f64, z: f64): WkbPoint {
        return WkbPoint{ .x = x, .y = y, .z = z, .m = 0.0, .has_z = true, .has_m = false };
    }

    fn xym(x: f64, y: f64, m: f64): WkbPoint {
        return WkbPoint{ .x = x, .y = y, .z = 0.0, .m = m, .has_z = false, .has_m = true };
    }

    fn xyzm(x: f64, y: f64, z: f64, m: f64): WkbPoint {
        return WkbPoint{ .x = x, .y = y, .z = z, .m = m, .has_z = true, .has_m = true };
    }
}

pub struct WkbHeader {
    little_endian: bool,
    geom_type: WkbGeomType,
    has_z: bool,
    has_m: bool,
    srid: u32?,
    coord_dims: u8,
    body_offset: usize,
}

fn readU32(buf: []const u8, off: usize, little: bool): u32 {
    const b0 = buf[off] as u32;
    const b1 = buf[off + 1] as u32;
    const b2 = buf[off + 2] as u32;
    const b3 = buf[off + 3] as u32;
    if (little) {
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }
    return (b0 << 24) | (b1 << 16) | (b2 << 8) | b3;
}

fn readF64(buf: []const u8, off: usize, little: bool): f64 {
    let bits: u64 = 0;
    if (little) {
        const lo = readU32(buf, off, true) as u64;
        const hi = readU32(buf, off + 4, true) as u64;
        bits = lo | (hi << 32);
    } else {
        const hi = readU32(buf, off, false) as u64;
        const lo = readU32(buf, off + 4, false) as u64;
        bits = (hi << 32) | lo;
    }
    unsafe {
        const v: f64 = @bitCast(bits);
        return v;
    }
}

fn readPointAt(buf: []const u8, off: usize, little: bool, has_z: bool, has_m: bool): WkbPoint {
    const x = readF64(buf, off, little);
    const y = readF64(buf, off + 8, little);
    let cur = off + 16;
    let z: f64 = 0.0;
    let m: f64 = 0.0;
    if (has_z) {
        z = readF64(buf, cur, little);
        cur += 8;
    }
    if (has_m) {
        m = readF64(buf, cur, little);
    }
    return WkbPoint{
        .x = x,
        .y = y,
        .z = z,
        .m = m,
        .has_z = has_z,
        .has_m = has_m,
    };
}

pub fn parseWkbHeader(data: []const u8): WkbHeader? {
    if (data.len < 5) {
        return null;
    }
    const order = data[0];
    if (order != WKB_XDR && order != WKB_NDR) {
        return null;
    }
    const little = (order == WKB_NDR);
    const raw_type = readU32(data, 1, little);

    let has_z = (raw_type & EWKB_FLAG_Z) != 0;
    let has_m = (raw_type & EWKB_FLAG_M) != 0;
    const has_srid = (raw_type & EWKB_FLAG_SRID) != 0;
    const base_code = raw_type & 0x1FFF_FFFF;

    let geom_code: u32 = base_code;
    if (base_code >= 3000 && base_code < 4000) {
        has_z = true;
        has_m = true;
        geom_code = base_code - 3000;
    } else if (base_code >= 2000 && base_code < 3000) {
        has_m = true;
        geom_code = base_code - 2000;
    } else if (base_code >= 1000 && base_code < 2000) {
        has_z = true;
        geom_code = base_code - 1000;
    }

    const geom_type = wkbGeomTypeFromCode(geom_code);
    if (geom_type == WkbGeomType.unknown) {
        return null;
    }

    let dims: u8 = 2;
    if (has_z) dims += 1;
    if (has_m) dims += 1;

    let srid: u32? = null;
    let body_offset: usize = 5;
    if (has_srid) {
        if (data.len < 9) {
            return null;
        }
        srid = readU32(data, 5, little);
        body_offset = 9;
    }

    return WkbHeader{
        .little_endian = little,
        .geom_type = geom_type,
        .has_z = has_z,
        .has_m = has_m,
        .srid = srid,
        .coord_dims = dims,
        .body_offset = body_offset,
    };
}

fn wkbByteLengthDepth(data: []const u8, depth: u32): usize? {
    if (depth > MAX_WKB_DEPTH) {
        return null;
    }
    const hdr = parseWkbHeader(data) ?? return null;
    const pt_bytes: usize = (hdr.coord_dims as usize) * 8;
    let pos = hdr.body_offset;

    if (hdr.geom_type == WkbGeomType.point) {
        if (pt_bytes > data.len - pos) {
            return null;
        }
        return pos + pt_bytes;
    }

    if (hdr.geom_type == WkbGeomType.linestring) {
        if (data.len - pos < 4) {
            return null;
        }
        const npts: usize = readU32(data, pos, hdr.little_endian) as usize;
        pos += 4;
        if (npts > (data.len - pos) / pt_bytes) {
            return null;
        }
        return pos + npts * pt_bytes;
    }

    if (hdr.geom_type == WkbGeomType.polygon) {
        if (data.len - pos < 4) {
            return null;
        }
        const nrings: usize = readU32(data, pos, hdr.little_endian) as usize;
        pos += 4;
        let r: usize = 0;
        while (r < nrings) {
            if (data.len - pos < 4) {
                return null;
            }
            const npts: usize = readU32(data, pos, hdr.little_endian) as usize;
            pos += 4;
            if (npts > (data.len - pos) / pt_bytes) {
                return null;
            }
            pos += npts * pt_bytes;
            r += 1;
        }
        return pos;
    }

    // MultiPoint / MultiLineString / MultiPolygon / GeometryCollection
    if (depth + 1 > MAX_WKB_DEPTH) {
        return null;
    }
    if (data.len - pos < 4) {
        return null;
    }
    const ngeoms: usize = readU32(data, pos, hdr.little_endian) as usize;
    pos += 4;
    let i: usize = 0;
    while (i < ngeoms) {
        const child_len = wkbByteLengthDepth(data[pos..], depth + 1) ?? return null;
        if (child_len == 0 || child_len > data.len - pos) {
            return null;
        }
        pos += child_len;
        i += 1;
    }
    return pos;
}

pub fn wkbByteLength(data: []const u8): usize? {
    return wkbByteLengthDepth(data, 0);
}

pub struct WkbPointIter {
    data: []const u8,
    pos: usize,
    remaining_points: usize,
    little_endian: bool,
    has_z: bool,
    has_m: bool,
    point_stride: usize,

    fn len(): usize {
        return _.remaining_points;
    }

    fn next(*_): WkbPoint? {
        if (_.remaining_points == 0 || _.point_stride > _.data.len - _.pos) {
            return null;
        }
        const pt = readPointAt(_.data, _.pos, _.little_endian, _.has_z, _.has_m);
        _.pos += _.point_stride;
        _.remaining_points -= 1;
        return pt;
    }
}

pub struct WkbRingIter {
    data: []const u8,
    pos: usize,
    remaining_rings: usize,
    little_endian: bool,
    has_z: bool,
    has_m: bool,
    point_stride: usize,

    fn init(data: []const u8): WkbRingIter? {
        const hdr = parseWkbHeader(data) ?? return null;
        if (hdr.geom_type != WkbGeomType.polygon) {
            return null;
        }
        if (data.len - hdr.body_offset < 4) {
            return null;
        }
        const nrings: usize = readU32(data, hdr.body_offset, hdr.little_endian) as usize;
        return WkbRingIter{
            .data = data,
            .pos = hdr.body_offset + 4,
            .remaining_rings = nrings,
            .little_endian = hdr.little_endian,
            .has_z = hdr.has_z,
            .has_m = hdr.has_m,
            .point_stride = (hdr.coord_dims as usize) * 8,
        };
    }

    fn len(): usize {
        return _.remaining_rings;
    }

    fn next(*_): WkbPointIter? {
        if (_.remaining_rings == 0 || _.data.len - _.pos < 4) {
            return null;
        }
        const npts: usize = readU32(_.data, _.pos, _.little_endian) as usize;
        const pts_start = _.pos + 4;
        if (npts > (_.data.len - pts_start) / _.point_stride) {
            _.remaining_rings = 0;
            return null;
        }
        const pts_bytes = npts * _.point_stride;
        _.pos = pts_start + pts_bytes;
        _.remaining_rings -= 1;
        return WkbPointIter{
            .data = _.data[pts_start.._.pos],
            .pos = 0,
            .remaining_points = npts,
            .little_endian = _.little_endian,
            .has_z = _.has_z,
            .has_m = _.has_m,
            .point_stride = _.point_stride,
        };
    }
}

pub struct WkbChildIter {
    data: []const u8,
    pos: usize,
    remaining_children: usize,

    fn init(data: []const u8): WkbChildIter? {
        const hdr = parseWkbHeader(data) ?? return null;
        const is_multi = hdr.geom_type == WkbGeomType.multipoint ||
            hdr.geom_type == WkbGeomType.multilinestring ||
            hdr.geom_type == WkbGeomType.multipolygon ||
            hdr.geom_type == WkbGeomType.geometrycollection;
        if (!is_multi) {
            return null;
        }
        if (data.len - hdr.body_offset < 4) {
            return null;
        }
        const ngeoms: usize = readU32(data, hdr.body_offset, hdr.little_endian) as usize;
        return WkbChildIter{
            .data = data,
            .pos = hdr.body_offset + 4,
            .remaining_children = ngeoms,
        };
    }

    fn len(): usize {
        return _.remaining_children;
    }

    fn next(*_): ([]const u8)? {
        if (_.remaining_children == 0 || _.pos >= _.data.len) {
            return null;
        }
        const clen = wkbByteLength(_.data[_.pos..]) ?? return null;
        const start = _.pos;
        _.pos += clen;
        _.remaining_children -= 1;
        return _.data[start.._.pos];
    }
}

pub struct WkbReader {
    fn readPoint(data: []const u8): WkbPoint? {
        const hdr = parseWkbHeader(data) ?? return null;
        if (hdr.geom_type != WkbGeomType.point) {
            return null;
        }
        const pt_bytes: usize = (hdr.coord_dims as usize) * 8;
        if (pt_bytes > data.len - hdr.body_offset) {
            return null;
        }
        return readPointAt(data, hdr.body_offset, hdr.little_endian, hdr.has_z, hdr.has_m);
    }

    fn linePoints(data: []const u8): WkbPointIter? {
        const hdr = parseWkbHeader(data) ?? return null;
        if (hdr.geom_type != WkbGeomType.linestring) {
            return null;
        }
        if (data.len - hdr.body_offset < 4) {
            return null;
        }
        const pt_stride: usize = (hdr.coord_dims as usize) * 8;
        const npts: usize = readU32(data, hdr.body_offset, hdr.little_endian) as usize;
        const start = hdr.body_offset + 4;
        if (npts > (data.len - start) / pt_stride) {
            return null;
        }
        const end = start + npts * pt_stride;
        return WkbPointIter{
            .data = data[start..end],
            .pos = 0,
            .remaining_points = npts,
            .little_endian = hdr.little_endian,
            .has_z = hdr.has_z,
            .has_m = hdr.has_m,
            .point_stride = pt_stride,
        };
    }

    fn polygonRings(data: []const u8): WkbRingIter? {
        return WkbRingIter.init(data);
    }

    fn children(data: []const u8): WkbChildIter? {
        return WkbChildIter.init(data);
    }
}

pub struct WkbWriter {
    buf: []u8,
    pos: usize,
    little_endian: bool,

    fn init(buf: []u8, little_endian: bool): WkbWriter {
        return WkbWriter{
            .buf = buf,
            .pos = 0,
            .little_endian = little_endian,
        };
    }

    fn initNdr(buf: []u8): WkbWriter {
        return WkbWriter.init(buf, true);
    }

    fn initXdr(buf: []u8): WkbWriter {
        return WkbWriter.init(buf, false);
    }

    fn written(): []const u8 {
        return _.buf[0.._.pos];
    }

    fn remaining(): usize {
        if (_.pos >= _.buf.len) {
            return 0;
        }
        return _.buf.len - _.pos;
    }

    fn writeU8(*_, v: u8): bool {
        if (_.remaining() < 1) return false;
        _.buf[_.pos] = v;
        _.pos += 1;
        return true;
    }

    fn writeU32(*_, v: u32): bool {
        if (_.remaining() < 4) return false;
        if (_.little_endian) {
            _.buf[_.pos] = (v & 0xFF) as u8;
            _.buf[_.pos + 1] = ((v >> 8) & 0xFF) as u8;
            _.buf[_.pos + 2] = ((v >> 16) & 0xFF) as u8;
            _.buf[_.pos + 3] = ((v >> 24) & 0xFF) as u8;
        } else {
            _.buf[_.pos] = ((v >> 24) & 0xFF) as u8;
            _.buf[_.pos + 1] = ((v >> 16) & 0xFF) as u8;
            _.buf[_.pos + 2] = ((v >> 8) & 0xFF) as u8;
            _.buf[_.pos + 3] = (v & 0xFF) as u8;
        }
        _.pos += 4;
        return true;
    }

    fn writeF64(*_, v: f64): bool {
        if (_.remaining() < 8) return false;
        let bits: u64 = 0;
        unsafe { bits = @bitCast(v); }
        const lo = (bits & 0xFFFF_FFFF) as u32;
        const hi = ((bits >> 32) & 0xFFFF_FFFF) as u32;
        if (_.little_endian) {
            if (!_.writeU32(lo)) return false;
            return _.writeU32(hi);
        }
        if (!_.writeU32(hi)) return false;
        return _.writeU32(lo);
    }

    fn writeHeader(*_, geom_type: WkbGeomType, has_z: bool, has_m: bool, srid: u32?): bool {
        const orig = _.pos;
        let order = WKB_XDR;
        if (_.little_endian) order = WKB_NDR;
        if (!_.writeU8(order)) {
            _.pos = orig;
            return false;
        }
        let code: u32 = geom_type as u32;
        if (has_z) code = code | EWKB_FLAG_Z;
        if (has_m) code = code | EWKB_FLAG_M;
        if (srid != null) code = code | EWKB_FLAG_SRID;
        if (!_.writeU32(code)) {
            _.pos = orig;
            return false;
        }
        if (s := srid) {
            if (!_.writeU32(s)) {
                _.pos = orig;
                return false;
            }
        }
        return true;
    }

    fn writeCoord(*_, pt: WkbPoint, has_z: bool, has_m: bool): bool {
        const orig = _.pos;
        if (!_.writeF64(pt.x) || !_.writeF64(pt.y)) {
            _.pos = orig;
            return false;
        }
        if (has_z) {
            if (!_.writeF64(pt.z)) {
                _.pos = orig;
                return false;
            }
        }
        if (has_m) {
            if (!_.writeF64(pt.m)) {
                _.pos = orig;
                return false;
            }
        }
        return true;
    }

    fn writePoint(*_, pt: WkbPoint, srid: u32?): bool {
        const orig = _.pos;
        if (!_.writeHeader(WkbGeomType.point, pt.has_z, pt.has_m, srid)) {
            _.pos = orig;
            return false;
        }
        if (!_.writeCoord(pt, pt.has_z, pt.has_m)) {
            _.pos = orig;
            return false;
        }
        return true;
    }

    fn writeLineString(*_, pts: []const WkbPoint, has_z: bool, has_m: bool, srid: u32?): bool {
        const orig = _.pos;
        if (!_.writeHeader(WkbGeomType.linestring, has_z, has_m, srid)) {
            _.pos = orig;
            return false;
        }
        if (!_.writeU32(pts.len as u32)) {
            _.pos = orig;
            return false;
        }
        let i: usize = 0;
        while (i < pts.len) {
            if (!_.writeCoord(pts[i], has_z, has_m)) {
                _.pos = orig;
                return false;
            }
            i += 1;
        }
        return true;
    }

    fn writePolygonHeader(*_, num_rings: u32, has_z: bool, has_m: bool, srid: u32?): bool {
        const orig = _.pos;
        if (!_.writeHeader(WkbGeomType.polygon, has_z, has_m, srid)) {
            _.pos = orig;
            return false;
        }
        if (!_.writeU32(num_rings)) {
            _.pos = orig;
            return false;
        }
        return true;
    }

    fn writeRing(*_, pts: []const WkbPoint, has_z: bool, has_m: bool): bool {
        const orig = _.pos;
        if (!_.writeU32(pts.len as u32)) {
            _.pos = orig;
            return false;
        }
        let i: usize = 0;
        while (i < pts.len) {
            if (!_.writeCoord(pts[i], has_z, has_m)) {
                _.pos = orig;
                return false;
            }
            i += 1;
        }
        return true;
    }

    fn writeCollectionHeader(*_, geom_type: WkbGeomType, num_geoms: u32, srid: u32?): bool {
        const orig = _.pos;
        if (!_.writeHeader(geom_type, false, false, srid)) {
            _.pos = orig;
            return false;
        }
        if (!_.writeU32(num_geoms)) {
            _.pos = orig;
            return false;
        }
        return true;
    }
}
