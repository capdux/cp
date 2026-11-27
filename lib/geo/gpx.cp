// Origin: Pure-CP Stdlib (Zero-Alloc GPX 1.0/1.1 XML Parser & Haversine Stats) | Created: 2026-11-22

import std from "std";

pub const GPX_EARTH_RADIUS_M: f64 = 6371008.8;
const DEG_TO_RAD: f64 = 0.017453292519943295;

pub struct GpxHeader {
    version: []const u8,
    creator: []const u8,
    name: []const u8,
    desc: []const u8,
    time: []const u8,
}

pub struct GpxPoint {
    lat: f64,
    lon: f64,
    ele: f64,
    has_ele: bool,
    time: []const u8,
    name: []const u8,
    sym: []const u8,
}

pub struct GpxTrackPoint {
    track_index: u32,
    segment_index: u32,
    point: GpxPoint,
}

pub struct GpxStats {
    waypoint_count: u64,
    track_count: u64,
    segment_count: u64,
    trackpoint_count: u64,
    total_distance_m: f64,
    elevation_gain_m: f64,
    elevation_loss_m: f64,
    min_lat: f64,
    max_lat: f64,
    min_lon: f64,
    max_lon: f64,
}

type GpxVec16 = @Vector(16, u8);

fn makeGpxClassLut(): [u8; 256] {
    @setEvalBranchQuota(20000);
    let lut: [u8; 256] = [0; 256];
    lut[' '] = 1 | 2;
    lut['\t'] = 1 | 2;
    lut['\r'] = 1 | 2;
    lut['\n'] = 1 | 2;
    lut['>'] = 2;
    lut['/'] = 2;
    return lut;
}

const GPX_CLASS_LUT: [u8; 256] = makeGpxClassLut();

fn isSpace(c: u8): bool {
    return (GPX_CLASS_LUT[@as(usize, c)] & 1) != 0;
}

fn isTagDelim(c: u8): bool {
    return (GPX_CLASS_LUT[@as(usize, c)] & 2) != 0;
}

fn startsWithAt(data: []const u8, pos: usize, prefix: []const u8): bool {
    if (pos > data.len || prefix.len > data.len - pos) {
        return false;
    }
    let i: usize = 0;
    while (i + 16 <= prefix.len) : (i += 16) {
        const va: GpxVec16 = data[pos + i .. pos + i + 16][0..16].*;
        const vb: GpxVec16 = prefix[i .. i + 16][0..16].*;
        if (@reduce(.Or, va ^ vb) != 0) {
            return false;
        }
    }
    while (i < prefix.len) {
        if (data[pos + i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimSpace(s: []const u8): []const u8 {
    let a: usize = 0;
    let b: usize = s.len;
    while (a < b && isSpace(s[a])) {
        a += 1;
    }
    while (b > a && isSpace(s[b - 1])) {
        b -= 1;
    }
    return s[a..b];
}

fn parseF64(raw: []const u8): f64? {
    const s = trimSpace(raw);
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg = false;
    if (s[i] == '-') {
        neg = true;
        i += 1;
    } else if (s[i] == '+') {
        i += 1;
    }

    let val: f64 = 0.0;
    let has_digits = false;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        has_digits = true;
        val = val * 10.0 + ((s[i] - '0') as f64);
        i += 1;
    }

    if (i < s.len && s[i] == '.') {
        i += 1;
        let scale: f64 = 0.1;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            has_digits = true;
            val += ((s[i] - '0') as f64) * scale;
            scale *= 0.1;
            i += 1;
        }
    }

    if (!has_digits) {
        return null;
    }

    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        let exp_neg = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        let exp: i32 = 0;
        let has_exp = false;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            has_exp = true;
            if (exp < 300) {
                exp = exp * 10 + ((s[i] - '0') as i32);
            }
            i += 1;
        }
        if (!has_exp) {
            return null;
        }
        let k: i32 = 0;
        while (k < exp) {
            if (exp_neg) {
                val *= 0.1;
            } else {
                val *= 10.0;
            }
            k += 1;
        }
    }

    if (i != s.len) {
        return null;
    }
    if (neg) {
        val = -val;
    }
    return val;
}

fn findBytes(data: []const u8, start: usize, needle: []const u8): usize? {
    if (needle.len == 0 || start > data.len || needle.len > data.len - start) {
        return null;
    }
    let i: usize = start;
    const limit = data.len - needle.len;
    const v_first: GpxVec16 = @splat(needle[0]);
    while (i <= limit) {
        if (i + 16 <= limit + 1) {
            const chunk: GpxVec16 = data[i .. i + 16][0..16].*;
            if (!@reduce(.Or, chunk == v_first)) {
                i += 16;
                continue;
            }
        }
        if (startsWithAt(data, i, needle)) {
            return i;
        }
        i += 1;
    }
    return null;
}

fn findTagEnd(data: []const u8, lt_pos: usize): usize? {
    let i = lt_pos + 1;
    let quote: u8 = 0;
    const v_dq: GpxVec16 = @splat('"' as u8);
    const v_sq: GpxVec16 = @splat('\'' as u8);
    const v_gt: GpxVec16 = @splat('>' as u8);
    while (i < data.len) {
        if (quote == 0) {
            while (i + 16 <= data.len) {
                const v: GpxVec16 = data[i .. i + 16][0..16].*;
                const hit = (v == v_dq) | (v == v_sq) | (v == v_gt);
                if (@reduce(.Or, hit)) break;
                i += 16;
            }
        } else {
            const v_q: GpxVec16 = @splat(quote);
            while (i + 16 <= data.len) {
                const v: GpxVec16 = data[i .. i + 16][0..16].*;
                if (@reduce(.Or, v == v_q)) break;
                i += 16;
            }
        }
        if (i >= data.len) break;
        const c = data[i];
        if (quote != 0) {
            if (c == quote) {
                quote = 0;
            }
        } else {
            if (c == '"' || c == '\'') {
                quote = c;
            } else if (c == '>') {
                return i;
            }
        }
        i += 1;
    }
    return null;
}

fn skipProlog(data: []const u8): usize {
    let pos: usize = 0;
    // Skip UTF-8 BOM
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        pos = 3;
    }
    while (pos < data.len) {
        while (pos < data.len && isSpace(data[pos])) {
            pos += 1;
        }
        if (startsWithAt(data, pos, "<?")) {
            const end = findBytes(data, pos + 2, "?>") ?? return data.len;
            pos = end + 2;
        } else if (startsWithAt(data, pos, "<!--")) {
            const end = findBytes(data, pos + 4, "-->") ?? return data.len;
            pos = end + 3;
        } else if (startsWithAt(data, pos, "<!")) {
            const end = findTagEnd(data, pos) ?? return data.len;
            pos = end + 1;
        } else {
            break;
        }
    }
    return pos;
}

fn getAttr(tag: []const u8, name: []const u8): ([]const u8)? {
    // Advance past tag name
    let i: usize = 0;
    if (i < tag.len && tag[i] == '<') i += 1;
    while (i < tag.len && !isTagDelim(tag[i])) {
        i += 1;
    }

    while (i < tag.len) {
        while (i < tag.len && isSpace(tag[i])) i += 1;
        if (i >= tag.len || tag[i] == '>' || tag[i] == '/') break;

        const name_start = i;
        while (i < tag.len && !isSpace(tag[i]) && tag[i] != '=' && tag[i] != '>' && tag[i] != '/') {
            i += 1;
        }
        const cur_name = tag[name_start..i];
        while (i < tag.len && isSpace(tag[i])) i += 1;
        if (i >= tag.len || tag[i] != '=') {
            continue;
        }
        i += 1;
        while (i < tag.len && isSpace(tag[i])) i += 1;
        if (i >= tag.len || (tag[i] != '"' && tag[i] != '\'')) {
            return null;
        }
        const q = tag[i];
        i += 1;
        const val_start = i;
        while (i < tag.len && tag[i] != q) {
            i += 1;
        }
        if (i >= tag.len) {
            return null;
        }
        const val = tag[val_start..i];
        i += 1;

        if (cur_name.len == name.len && startsWithAt(cur_name, 0, name)) {
            return val;
        }
    }
    return null;
}

fn isSelfClosingTag(data: []const u8, gt_pos: usize): bool {
    if (gt_pos == 0) return false;
    let j = gt_pos - 1;
    while (j > 0 && isSpace(data[j])) {
        j -= 1;
    }
    return data[j] == '/';
}

fn extractChildText(body: []const u8, tag_name: []const u8): []const u8 {
    let pos: usize = 0;
    while (pos < body.len) {
        const lt = findBytes(body, pos, "<") ?? return body[0..0];
        if (startsWithAt(body, lt, "<!--")) {
            const cend = findBytes(body, lt + 4, "-->") ?? return body[0..0];
            pos = cend + 3;
            continue;
        }
        const gt = findTagEnd(body, lt) ?? return body[0..0];
        if (startsWithAt(body, lt + 1, tag_name)) {
            const after_name = lt + 1 + tag_name.len;
            if (after_name < body.len && isTagDelim(body[after_name])) {
                if (isSelfClosingTag(body, gt)) {
                    return body[0..0];
                }
                const content_start = gt + 1;
                // Find matching </tag_name>
                let scan = content_start;
                while (scan < body.len) {
                    const clt = findBytes(body, scan, "</") ?? return body[0..0];
                    if (startsWithAt(body, clt + 2, tag_name)) {
                        const cafter = clt + 2 + tag_name.len;
                        if (cafter < body.len && (isSpace(body[cafter]) || body[cafter] == '>')) {
                            return trimSpace(body[content_start..clt]);
                        }
                    }
                    scan = clt + 2;
                }
                return body[0..0];
            }
        }
        pos = gt + 1;
    }
    return body[0..0];
}

struct ElementMatch {
    open_tag: []const u8,
    body: []const u8,
    next_pos: usize,
}

fn readElement(data: []const u8, lt_pos: usize, tag_name: []const u8): ElementMatch? {
    const gt = findTagEnd(data, lt_pos) ?? return null;
    const open_tag = data[lt_pos..(gt + 1)];
    if (isSelfClosingTag(data, gt)) {
        return ElementMatch{
            .open_tag = open_tag,
            .body = data[0..0],
            .next_pos = gt + 1,
        };
    }

    const body_start = gt + 1;
    let scan = body_start;
    while (scan < data.len) {
        const clt = findBytes(data, scan, "</") ?? return null;
        if (startsWithAt(data, clt + 2, tag_name)) {
            const after = clt + 2 + tag_name.len;
            if (after < data.len && (isSpace(data[after]) || data[after] == '>')) {
                const cgt = findTagEnd(data, clt) ?? return null;
                return ElementMatch{
                    .open_tag = open_tag,
                    .body = data[body_start..clt],
                    .next_pos = cgt + 1,
                };
            }
        }
        scan = clt + 2;
    }
    return null;
}

fn parsePointElement(elem: ElementMatch): GpxPoint? {
    const lat_s = getAttr(elem.open_tag, "lat") ?? return null;
    const lon_s = getAttr(elem.open_tag, "lon") ?? return null;
    const lat = parseF64(lat_s) ?? return null;
    const lon = parseF64(lon_s) ?? return null;
    if (lat < -90.0 || lat > 90.0 || lon < -180.0 || lon > 180.0) {
        return null;
    }

    let ele: f64 = 0.0;
    let has_ele = false;
    const ele_s = extractChildText(elem.body, "ele");
    if (ele_s.len > 0) {
        if (e := parseF64(ele_s)) {
            ele = e;
            has_ele = true;
        }
    }

    const time_s = extractChildText(elem.body, "time");
    const name_s = extractChildText(elem.body, "name");
    const sym_s = extractChildText(elem.body, "sym");

    return GpxPoint{
        .lat = lat,
        .lon = lon,
        .ele = ele,
        .has_ele = has_ele,
        .time = time_s,
        .name = name_s,
        .sym = sym_s,
    };
}

pub fn isGpx(data: []const u8): bool {
    const pos = skipProlog(data);
    if (!startsWithAt(data, pos, "<gpx")) {
        return false;
    }
    const after = pos + 4;
    if (after >= data.len || !isTagDelim(data[after])) {
        return false;
    }
    if (_gt := findTagEnd(data, pos)) {
        return true;
    }
    return false;
}

pub fn parseGpxHeader(data: []const u8): GpxHeader? {
    const pos = skipProlog(data);
    if (!startsWithAt(data, pos, "<gpx")) {
        return null;
    }
    const after = pos + 4;
    if (after >= data.len || !isTagDelim(data[after])) {
        return null;
    }
    const gt = findTagEnd(data, pos) ?? return null;
    const root_tag = data[pos..(gt + 1)];
    const empty = data[0..0];

    const version = getAttr(root_tag, "version") ?? empty;
    const creator = getAttr(root_tag, "creator") ?? empty;

    let name = empty;
    let desc = empty;
    let time = empty;

    // Locate optional <metadata>...</metadata>
    let scan = gt + 1;
    while (scan < data.len) {
        const lt = findBytes(data, scan, "<") ?? break;
        if (startsWithAt(data, lt, "<!--")) {
            const cend = findBytes(data, lt + 4, "-->") ?? break;
            scan = cend + 3;
            continue;
        }
        if (startsWithAt(data, lt, "<metadata") && lt + 9 < data.len && isTagDelim(data[lt + 9])) {
            if (meta_elem := readElement(data, lt, "metadata")) {
                name = extractChildText(meta_elem.body, "name");
                desc = extractChildText(meta_elem.body, "desc");
                time = extractChildText(meta_elem.body, "time");
            }
            break;
        }
        const t_end = findTagEnd(data, lt) ?? break;
        scan = t_end + 1;
    }

    return GpxHeader{
        .version = version,
        .creator = creator,
        .name = name,
        .desc = desc,
        .time = time,
    };
}

pub struct GpxWaypointIter {
    data: []const u8,
    pos: usize,

    static fn init(data: []const u8): GpxWaypointIter {
        return GpxWaypointIter{
            .data = data,
            .pos = skipProlog(data),
        };
    }

    fn next(*_): GpxPoint? {
        while (_.pos < _.data.len) {
            const lt = findBytes(_.data, _.pos, "<") ?? return null;
            if (startsWithAt(_.data, lt, "<!--")) {
                const cend = findBytes(_.data, lt + 4, "-->") ?? return null;
                _.pos = cend + 3;
                continue;
            }
            if (startsWithAt(_.data, lt, "<wpt") && lt + 4 < _.data.len && isTagDelim(_.data[lt + 4])) {
                const elem = readElement(_.data, lt, "wpt") ?? return null;
                _.pos = elem.next_pos;
                if (pt := parsePointElement(elem)) {
                    return pt;
                }
                continue;
            }
            const gt = findTagEnd(_.data, lt) ?? return null;
            _.pos = gt + 1;
        }
        return null;
    }
}

pub struct GpxTrackPointIter {
    data: []const u8,
    pos: usize,
    track_count: u32,
    segment_count: u32,

    static fn init(data: []const u8): GpxTrackPointIter {
        return GpxTrackPointIter{
            .data = data,
            .pos = skipProlog(data),
            .track_count = 0,
            .segment_count = 0,
        };
    }

    fn next(*_): GpxTrackPoint? {
        while (_.pos < _.data.len) {
            const lt = findBytes(_.data, _.pos, "<") ?? return null;
            if (startsWithAt(_.data, lt, "<!--")) {
                const cend = findBytes(_.data, lt + 4, "-->") ?? return null;
                _.pos = cend + 3;
                continue;
            }
            if (startsWithAt(_.data, lt, "<trkpt") && lt + 6 < _.data.len && isTagDelim(_.data[lt + 6])) {
                const elem = readElement(_.data, lt, "trkpt") ?? return null;
                _.pos = elem.next_pos;
                if (pt := parsePointElement(elem)) {
                    let ti: u32 = 0;
                    if (_.track_count > 0) ti = _.track_count - 1;
                    let si: u32 = 0;
                    if (_.segment_count > 0) si = _.segment_count - 1;
                    return GpxTrackPoint{
                        .track_index = ti,
                        .segment_index = si,
                        .point = pt,
                    };
                }
                continue;
            }
            if (startsWithAt(_.data, lt, "<trkseg") && lt + 7 < _.data.len && isTagDelim(_.data[lt + 7])) {
                _.segment_count += 1;
            } else if (startsWithAt(_.data, lt, "<trk") && lt + 4 < _.data.len && isTagDelim(_.data[lt + 4])) {
                _.track_count += 1;
                _.segment_count = 0;
            }
            const gt = findTagEnd(_.data, lt) ?? return null;
            _.pos = gt + 1;
        }
        return null;
    }
}

pub fn haversineMeters(lat1: f64, lon1: f64, lat2: f64, lon2: f64): f64 {
    const phi1 = lat1 * DEG_TO_RAD;
    const phi2 = lat2 * DEG_TO_RAD;
    const dphi = (lat2 - lat1) * DEG_TO_RAD;
    const dlam = (lon2 - lon1) * DEG_TO_RAD;

    const sp = std.math.sin(dphi * 0.5);
    const sl = std.math.sin(dlam * 0.5);
    let a = sp * sp + std.math.cos(phi1) * std.math.cos(phi2) * sl * sl;
    if (a < 0.0) a = 0.0;
    if (a > 1.0) a = 1.0;
    const c = 2.0 * std.math.atan2(std.math.sqrt(a), std.math.sqrt(1.0 - a));
    return GPX_EARTH_RADIUS_M * c;
}

pub fn summarizeGpx(data: []const u8): GpxStats {
    let waypoint_count: u64 = 0;
    let track_count: u64 = 0;
    let segment_count: u64 = 0;
    let trackpoint_count: u64 = 0;
    let total_distance_m: f64 = 0.0;
    let elevation_gain_m: f64 = 0.0;
    let elevation_loss_m: f64 = 0.0;

    let has_bbox = false;
    let min_lat: f64 = 0.0;
    let max_lat: f64 = 0.0;
    let min_lon: f64 = 0.0;
    let max_lon: f64 = 0.0;

    let has_prev = false;
    let prev_lat: f64 = 0.0;
    let prev_lon: f64 = 0.0;
    let prev_ele: f64 = 0.0;
    let prev_has_ele = false;

    let pos = skipProlog(data);
    while (pos < data.len) {
        const lt = findBytes(data, pos, "<") ?? break;
        if (startsWithAt(data, lt, "<!--")) {
            const cend = findBytes(data, lt + 4, "-->") ?? break;
            pos = cend + 3;
            continue;
        }
        if (startsWithAt(data, lt, "<wpt") && lt + 4 < data.len && isTagDelim(data[lt + 4])) {
            const elem = readElement(data, lt, "wpt") ?? break;
            pos = elem.next_pos;
            if (pt := parsePointElement(elem)) {
                waypoint_count += 1;
                if (!has_bbox) {
                    min_lat = pt.lat; max_lat = pt.lat;
                    min_lon = pt.lon; max_lon = pt.lon;
                    has_bbox = true;
                } else {
                    if (pt.lat < min_lat) min_lat = pt.lat;
                    if (pt.lat > max_lat) max_lat = pt.lat;
                    if (pt.lon < min_lon) min_lon = pt.lon;
                    if (pt.lon > max_lon) max_lon = pt.lon;
                }
            }
            continue;
        }
        if (startsWithAt(data, lt, "<trkpt") && lt + 6 < data.len && isTagDelim(data[lt + 6])) {
            const elem = readElement(data, lt, "trkpt") ?? break;
            pos = elem.next_pos;
            if (pt := parsePointElement(elem)) {
                trackpoint_count += 1;
                if (!has_bbox) {
                    min_lat = pt.lat; max_lat = pt.lat;
                    min_lon = pt.lon; max_lon = pt.lon;
                    has_bbox = true;
                } else {
                    if (pt.lat < min_lat) min_lat = pt.lat;
                    if (pt.lat > max_lat) max_lat = pt.lat;
                    if (pt.lon < min_lon) min_lon = pt.lon;
                    if (pt.lon > max_lon) max_lon = pt.lon;
                }
                if (has_prev) {
                    total_distance_m += haversineMeters(prev_lat, prev_lon, pt.lat, pt.lon);
                    if (prev_has_ele && pt.has_ele) {
                        const diff = pt.ele - prev_ele;
                        if (diff > 0.0) {
                            elevation_gain_m += diff;
                        } else if (diff < 0.0) {
                            elevation_loss_m += -diff;
                        }
                    }
                }
                has_prev = true;
                prev_lat = pt.lat;
                prev_lon = pt.lon;
                prev_ele = pt.ele;
                prev_has_ele = pt.has_ele;
            }
            continue;
        }
        if (startsWithAt(data, lt, "<trkseg") && lt + 7 < data.len && isTagDelim(data[lt + 7])) {
            segment_count += 1;
            has_prev = false;
        } else if (startsWithAt(data, lt, "</trkseg") || startsWithAt(data, lt, "</trk")) {
            has_prev = false;
        } else if (startsWithAt(data, lt, "<trk") && lt + 4 < data.len && isTagDelim(data[lt + 4])) {
            track_count += 1;
            has_prev = false;
        }

        const gt = findTagEnd(data, lt) ?? break;
        pos = gt + 1;
    }

    return GpxStats{
        .waypoint_count = waypoint_count,
        .track_count = track_count,
        .segment_count = segment_count,
        .trackpoint_count = trackpoint_count,
        .total_distance_m = total_distance_m,
        .elevation_gain_m = elevation_gain_m,
        .elevation_loss_m = elevation_loss_m,
        .min_lat = min_lat,
        .max_lat = max_lat,
        .min_lon = min_lon,
        .max_lon = max_lon,
    };
}
