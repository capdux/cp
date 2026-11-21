// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc OGC KML 2.2 Placemark & Coordinates Parser)

/// OGC KML 2.2 几何类型枚举
pub enum KmlGeomKind: u8 {
    point = 0,
    linestring = 1,
    polygon = 2,
    multigeometry = 3,
    unknown = 4,
}

/// KML 地理坐标（KML 规范顺序：`lon,lat[,alt]`）
pub struct KmlCoord {
    lon: f64,
    lat: f64,
    alt: f64,
    has_alt: bool,
}

/// 零堆分配 `<coordinates>` 内部 `lon,lat[,alt]` 三元组流式迭代器
pub struct KmlCoordIter {
    data: []const u8,
    pos: usize = 0,

    static fn init(raw_coords: []const u8): KmlCoordIter {
        return KmlCoordIter {
            data: raw_coords,
            pos: 0,
        };
    }

    /// 解析下一组 `lon,lat[,alt]`（严格校验 `-180 <= lon <= 180` 与 `-90 <= lat <= 90`）；到达末尾或遇到非法坐标时返回 `null`
    fn next(*_): KmlCoord? {
        while (_.pos < _.data.len && isWs(_.data[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= _.data.len) return null;

        const start = _.pos;
        while (_.pos < _.data.len && !isWs(_.data[_.pos])) {
            _.pos += 1;
        }
        const tok = _.data[start.._.pos];

        const c1 = findByte(tok, ',', 0) ?? return null;
        const lon = parseF64(tok[0..c1]) ?? return null;
        if (lon < -180.0 || lon > 180.0) return null;

        const rest = tok[c1 + 1 .. tok.len];
        if (c2 := findByte(rest, ',', 0)) {
            const lat = parseF64(rest[0..c2]) ?? return null;
            if (lat < -90.0 || lat > 90.0) return null;
            const alt_str = rest[c2 + 1 .. rest.len];
            // 禁止出现第三个逗号
            if (findByte(alt_str, ',', 0) != null) return null;
            const alt = parseF64(alt_str) ?? return null;
            return KmlCoord {
                lon: lon,
                lat: lat,
                alt: alt,
                has_alt: true,
            };
        }

        const lat = parseF64(rest) ?? return null;
        if (lat < -90.0 || lat > 90.0) return null;
        return KmlCoord {
            lon: lon,
            lat: lat,
            alt: 0.0,
            has_alt: false,
        };
    }
}

/// 零拷贝 `<Placemark>` 视图
pub struct KmlPlacemark {
    id: []const u8,
    name: []const u8,
    description: []const u8,
    style_url: []const u8,
    geom_kind: KmlGeomKind,
    raw_coordinates: []const u8,

    /// 创建 `<coordinates>` 零拷贝坐标迭代器
    fn coords(): KmlCoordIter {
        return KmlCoordIter.init(_.raw_coordinates);
    }
}

/// 零堆分配 KML `<Placemark>` 流式迭代器（支持嵌套 `<Document>`/`<Folder>` 与 `<![CDATA[...]]>`）
pub struct KmlPlacemarkIter {
    data: []const u8,
    pos: usize = 0,

    static fn init(data: []const u8): KmlPlacemarkIter {
        return KmlPlacemarkIter {
            data: data,
            pos: 0,
        };
    }

    /// 解析并返回下一条 `<Placemark>`；到达末尾或遇到未闭合标签时安全返回 `null`
    fn next(*_): KmlPlacemark? {
        while (_.pos < _.data.len) {
            const open_pos = findOpenTag(_.data, _.pos, "Placemark") ?? return null;
            const gt_pos = findByte(_.data, '>', open_pos + 10) ?? return null;

            const close_tag = "</Placemark>";
            const close_pos = findSubstrSkippingSpecial(_.data, gt_pos + 1, close_tag) ?? return null;

            const attr_slice = _.data[open_pos + 10 .. gt_pos];
            const body = _.data[gt_pos + 1 .. close_pos];
            _.pos = close_pos + close_tag.len;

            const empty = _.data[0..0];
            const id = extractAttr(attr_slice, "id") ?? empty;
            const name = extractTagText(body, "name") ?? empty;
            const desc = extractTagText(body, "description") ?? empty;
            const style_url = extractTagText(body, "styleUrl") ?? empty;
            const geom_kind = detectGeomKind(body);
            const raw_coords = extractTagText(body, "coordinates") ?? empty;

            return KmlPlacemark {
                id: id,
                name: name,
                description: desc,
                style_url: style_url,
                geom_kind: geom_kind,
                raw_coordinates: raw_coords,
            };
        }
        return null;
    }
}

/// 单遍汇总的 KML 统计信息与全局经纬度包络盒
pub struct KmlStats {
    placemark_count: u32,
    point_count: u32,
    linestring_count: u32,
    polygon_count: u32,
    coord_count: u64,
    has_bbox: bool,
    min_lon: f64,
    max_lon: f64,
    min_lat: f64,
    max_lat: f64,
}

fn isWs(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn startsWithAt(data: []const u8, pos: usize, pat: []const u8): bool {
    if (pat.len > data.len - pos) return false;
    let i: usize = 0;
    while (i < pat.len) {
        if (data[pos + i] != pat[i]) return false;
        i += 1;
    }
    return true;
}

fn findByte(s: []const u8, ch: u8, start: usize): usize? {
    let i = start;
    while (i < s.len) {
        if (s[i] == ch) return i;
        i += 1;
    }
    return null;
}

fn findSubstr(data: []const u8, start: usize, pat: []const u8): usize? {
    if (pat.len == 0 || pat.len > data.len) return null;
    let i = start;
    const limit = data.len - pat.len;
    while (i <= limit) {
        if (startsWithAt(data, i, pat)) return i;
        i += 1;
    }
    return null;
}

fn findSubstrSkippingSpecial(data: []const u8, start: usize, pat: []const u8): usize? {
    if (pat.len == 0 || pat.len > data.len) return null;
    let i = start;
    while (i + pat.len <= data.len) {
        if (startsWithAt(data, i, "<![CDATA[")) {
            const end_cd = findSubstr(data, i + 9, "]]>") ?? return null;
            i = end_cd + 3;
            continue;
        }
        if (startsWithAt(data, i, "<!--")) {
            const end_cm = findSubstr(data, i + 4, "-->") ?? return null;
            i = end_cm + 3;
            continue;
        }
        if (startsWithAt(data, i, pat)) return i;
        i += 1;
    }
    return null;
}

fn findOpenTag(data: []const u8, start: usize, tag_name: []const u8): usize? {
    let i = start;
    while (i < data.len) {
        if (startsWithAt(data, i, "<![CDATA[")) {
            const end_cd = findSubstr(data, i + 9, "]]>") ?? return null;
            i = end_cd + 3;
            continue;
        }
        if (startsWithAt(data, i, "<!--")) {
            const end_cm = findSubstr(data, i + 4, "-->") ?? return null;
            i = end_cm + 3;
            continue;
        }
        if (data[i] == '<' && i + 1 + tag_name.len < data.len) {
            if (startsWithAt(data, i + 1, tag_name)) {
                const after = data[i + 1 + tag_name.len];
                if (after == '>' || after == '/' || isWs(after)) {
                    return i;
                }
            }
        }
        i += 1;
    }
    return null;
}

fn trimWs(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isWs(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isWs(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn unwrapCdata(s: []const u8): []const u8 {
    const t = trimWs(s);
    if (t.len >= 12 && startsWithAt(t, 0, "<![CDATA[") && startsWithAt(t, t.len - 3, "]]>")) {
        return t[9 .. t.len - 3];
    }
    return t;
}

fn extractAttr(attrs: []const u8, key: []const u8): ([]const u8)? {
    let i: usize = 0;
    while (i < attrs.len) {
        while (i < attrs.len && isWs(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len) break;
        const k_start = i;
        while (i < attrs.len && attrs[i] != '=' && !isWs(attrs[i])) {
            i += 1;
        }
        const k_end = i;
        while (i < attrs.len && isWs(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len || attrs[i] != '=') break;
        i += 1;
        while (i < attrs.len && isWs(attrs[i])) {
            i += 1;
        }
        if (i >= attrs.len) break;
        const quote = attrs[i];
        if (quote != '"' && quote != '\'') break;
        i += 1;
        const v_start = i;
        while (i < attrs.len && attrs[i] != quote) {
            i += 1;
        }
        if (i >= attrs.len) break;
        const v_end = i;
        i += 1;
        if (k_end - k_start == key.len && startsWithAt(attrs, k_start, key)) {
            return attrs[v_start..v_end];
        }
    }
    return null;
}

fn extractTagText(body: []const u8, tag_name: []const u8): ([]const u8)? {
    const open_pos = findOpenTag(body, 0, tag_name) ?? return null;
    const gt_pos = findByte(body, '>', open_pos + 1 + tag_name.len) ?? return null;
    // 自闭合标签如 <name/>
    if (gt_pos > open_pos && body[gt_pos - 1] == '/') {
        return body[gt_pos..gt_pos];
    }

    // 查找 </tag_name>
    let i: usize = gt_pos + 1;
    while (i < body.len) {
        if (startsWithAt(body, i, "<![CDATA[")) {
            const end_cd = findSubstr(body, i + 9, "]]>") ?? return null;
            i = end_cd + 3;
            continue;
        }
        if (startsWithAt(body, i, "<!--")) {
            const end_cm = findSubstr(body, i + 4, "-->") ?? return null;
            i = end_cm + 3;
            continue;
        }
        if (body[i] == '<' && i + 2 + tag_name.len < body.len && body[i + 1] == '/') {
            if (startsWithAt(body, i + 2, tag_name) && body[i + 2 + tag_name.len] == '>') {
                return unwrapCdata(body[gt_pos + 1 .. i]);
            }
        }
        i += 1;
    }
    return null;
}

fn detectGeomKind(body: []const u8): KmlGeomKind {
    if (findOpenTag(body, 0, "MultiGeometry") != null) return KmlGeomKind.multigeometry;
    if (findOpenTag(body, 0, "Polygon") != null) return KmlGeomKind.polygon;
    if (findOpenTag(body, 0, "LineString") != null) return KmlGeomKind.linestring;
    if (findOpenTag(body, 0, "Point") != null) return KmlGeomKind.point;
    return KmlGeomKind.unknown;
}

fn parseF64(s: []const u8): f64? {
    if (s.len == 0) return null;
    let i: usize = 0;
    let neg: bool = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) return null;

    let int_part: f64 = 0.0;
    let has_digits: bool = false;
    while (i < s.len && s[i] >= '0' && s[i] <= '9') {
        int_part = int_part * 10.0 + ((s[i] - '0') as f64);
        has_digits = true;
        i += 1;
    }

    let frac_part: f64 = 0.0;
    let frac_scale: f64 = 0.1;
    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            frac_part += ((s[i] - '0') as f64) * frac_scale;
            frac_scale *= 0.1;
            has_digits = true;
            i += 1;
        }
    }
    if (!has_digits) return null;

    let val: f64 = int_part + frac_part;
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        let exp_neg: bool = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        if (i >= s.len || s[i] < '0' || s[i] > '9') return null;
        let exp_val: i32 = 0;
        while (i < s.len && s[i] >= '0' && s[i] <= '9') {
            if (exp_val < 308) {
                exp_val = exp_val * 10 + ((s[i] - '0') as i32);
            }
            i += 1;
        }
        let step: i32 = 0;
        while (step < exp_val) {
            val = if (exp_neg) (val * 0.1) else (val * 10.0);
            step += 1;
        }
    }

    if (i != s.len) return null;
    return if (neg) -val else val;
}

/// 跳过可选 UTF-8 BOM、XML 声明与注释，探测根标签是否为 `<kml`
pub fn isKml(data: []const u8): bool {
    let pos: usize = 0;
    if (data.len >= 3 && data[0] == 0xEF && data[1] == 0xBB && data[2] == 0xBF) {
        pos = 3;
    }

    while (pos < data.len) {
        while (pos < data.len && isWs(data[pos])) {
            pos += 1;
        }
        if (pos >= data.len) return false;

        if (startsWithAt(data, pos, "<?")) {
            const end_pi = findSubstr(data, pos + 2, "?>") ?? return false;
            pos = end_pi + 2;
            continue;
        }
        if (startsWithAt(data, pos, "<!--")) {
            const end_cm = findSubstr(data, pos + 4, "-->") ?? return false;
            pos = end_cm + 3;
            continue;
        }
        if (startsWithAt(data, pos, "<kml")) {
            if (pos + 4 >= data.len) return false;
            const next_ch = data[pos + 4];
            return next_ch == '>' || next_ch == ':' || isWs(next_ch);
        }
        return false;
    }
    return false;
}

/// 单遍汇总 KML 文档的 Placemark 分布、坐标总数与全局经纬度 BBox
pub fn summarizeKml(data: []const u8): KmlStats {
    let stats = KmlStats {
        placemark_count: 0,
        point_count: 0,
        linestring_count: 0,
        polygon_count: 0,
        coord_count: 0,
        has_bbox: false,
        min_lon: 0.0,
        max_lon: 0.0,
        min_lat: 0.0,
        max_lat: 0.0,
    };

    let pit = KmlPlacemarkIter.init(data);
    while (pm := pit.next()) {
        stats.placemark_count += 1;
        if (pm.geom_kind == KmlGeomKind.point) {
            stats.point_count += 1;
        } else if (pm.geom_kind == KmlGeomKind.linestring) {
            stats.linestring_count += 1;
        } else if (pm.geom_kind == KmlGeomKind.polygon) {
            stats.polygon_count += 1;
        }

        let cit = pm.coords();
        while (c := cit.next()) {
            stats.coord_count += 1;
            if (!stats.has_bbox) {
                stats.has_bbox = true;
                stats.min_lon = c.lon;
                stats.max_lon = c.lon;
                stats.min_lat = c.lat;
                stats.max_lat = c.lat;
            } else {
                if (c.lon < stats.min_lon) stats.min_lon = c.lon;
                if (c.lon > stats.max_lon) stats.max_lon = c.lon;
                if (c.lat < stats.min_lat) stats.min_lat = c.lat;
                if (c.lat > stats.max_lat) stats.max_lat = c.lat;
            }
        }
    }

    return stats;
}
