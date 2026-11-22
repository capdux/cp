// Origin: Pure-CP Stdlib (Zero-Alloc FAI IGC Flight Recorder Parser) | Created: 2026-11-22

/// FAI IGC 文件头（A 记录与 H 头记录，零拷贝切片指向原始文本）
pub struct IgcHeader {
    manufacturer: []const u8,
    logger_id: []const u8,
    date_ddmmyy: []const u8,
    pilot: []const u8,
    glider_type: []const u8,
    glider_id: []const u8,
    fr_type: []const u8,
    gps_datum: []const u8,
}

/// FAI IGC B 记录定位点（至少 35 字节定宽 ASCII）
pub struct IgcFix {
    hour: u8,
    minute: u8,
    second: u8,
    lat_deg: f64,
    lon_deg: f64,
    validity: u8,
    pressure_alt_m: i32,
    gnss_alt_m: i32,

    /// 返回当日 UTC 秒数（0..86399）
    pub fn timeSeconds(): u32 {
        return u32(_.hour) * 3600 + u32(_.minute) * 60 + u32(_.second);
    }
}

/// 完整 IGC 飞行轨迹单遍统计结果
pub struct IgcStats {
    fix_count: u32,
    valid_3d_count: u32,
    min_alt_m: i32,
    max_alt_m: i32,
    duration_sec: u32,
    min_lat: f64,
    max_lat: f64,
    min_lon: f64,
    max_lon: f64,
}

// ---------------------------------------------------------------------------
// 内部零分配辅助函数
// ---------------------------------------------------------------------------

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isAlnum(c: u8): bool {
    return isDigit(c) || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn eqlBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) {
        start += 1;
    }
    let end = s.len;
    while (end > start && isSpace(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn stripCr(line: []const u8): []const u8 {
    if (line.len > 0 && line[line.len - 1] == '\r') {
        return line[0 .. line.len - 1];
    }
    return line;
}

fn parseFixedU32(s: []const u8): u32? {
    if (s.len == 0) {
        return null;
    }
    let acc: u32 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return null;
        }
        acc = acc * 10 + u32(s[i] - '0');
        i += 1;
    }
    return acc;
}

fn parseFixedAlt5(s: []const u8): i32? {
    if (s.len != 5) {
        return null;
    }
    let neg = false;
    let start: usize = 0;
    if (s[0] == '-') {
        neg = true;
        start = 1;
    } else if (s[0] == '+') {
        start = 1;
    }
    const mag = parseFixedU32(s[start..5]) ?? return null;
    const val = mag as i32;
    return if (neg) -val else val;
}

fn findColon(s: []const u8): usize? {
    let i: usize = 0;
    while (i < s.len) {
        if (s[i] == ':') {
            return i;
        }
        i += 1;
    }
    return null;
}

fn extractHeaderValue(line: []const u8): []const u8 {
    if (colon := findColon(line)) {
        return trimAscii(line[colon + 1 ..]);
    }
    if (line.len > 5) {
        return trimAscii(line[5..]);
    }
    return "";
}

fn extractDateDdmmyy(line: []const u8): []const u8 {
    if (colon := findColon(line)) {
        const after = trimAscii(line[colon + 1 ..]);
        // 兼容 HFDTEDATE:DDMMYY,NN
        let end: usize = 0;
        while (end < after.len && end < 6 && isDigit(after[end])) {
            end += 1;
        }
        if (end == 6) {
            return after[0..6];
        }
        return "";
    }
    // 旧版 HFDTEDDMMYY
    if (line.len >= 11) {
        let i: usize = 5;
        while (i < 11) {
            if (!isDigit(line[i])) {
                return "";
            }
            i += 1;
        }
        return line[5..11];
    }
    return "";
}

struct LineSlice {
    line: []const u8,
    next_pos: usize,
}

fn nextLine(data: []const u8, pos: usize): LineSlice? {
    if (pos >= data.len) {
        return null;
    }
    let end = pos;
    while (end < data.len && data[end] != '\n') {
        end += 1;
    }
    const next_pos = if (end < data.len && data[end] == '\n') (end + 1) else end;
    return LineSlice {
        line: stripCr(data[pos..end]),
        next_pos: next_pos,
    };
}

// ---------------------------------------------------------------------------
// 1. 格式识别与文件头解析 (A + H 记录)
// ---------------------------------------------------------------------------

/// 校验首行是否为合法 FAI IGC `A` 记录（`'A'` + 3B 厂商码 + 3B 记录仪 ID，至少 7 字节）
pub fn isIgc(data: []const u8): bool {
    const first = nextLine(data, 0) ?? return false;
    const line = first.line;
    if (line.len < 7 || line[0] != 'A') {
        return false;
    }
    let i: usize = 1;
    while (i < 7) {
        if (!isAlnum(line[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 零分配解析 IGC `A` 记录与全部 `H` 头记录（兼容新旧 `HFDTE` 日期格式）
pub fn parseIgcHeader(data: []const u8): IgcHeader? {
    if (!isIgc(data)) {
        return null;
    }
    const first = nextLine(data, 0) ?? return null;
    let hdr = IgcHeader {
        manufacturer: first.line[1..4],
        logger_id: first.line[4..7],
        date_ddmmyy: "",
        pilot: "",
        glider_type: "",
        glider_id: "",
        fr_type: "",
        gps_datum: "",
    };

    let pos = first.next_pos;
    while (ls := nextLine(data, pos)) {
        pos = ls.next_pos;
        const line = ls.line;
        if (line.len == 0) {
            continue;
        }
        if (line[0] == 'B') {
            break;
        }
        if (line[0] == 'H' && line.len >= 5) {
            const code = line[2..5];
            if (eqlBytes(code, "DTE")) {
                if (hdr.date_ddmmyy.len == 0) {
                    hdr.date_ddmmyy = extractDateDdmmyy(line);
                }
            } else if (eqlBytes(code, "PLT")) {
                if (hdr.pilot.len == 0) {
                    hdr.pilot = extractHeaderValue(line);
                }
            } else if (eqlBytes(code, "GTY")) {
                if (hdr.glider_type.len == 0) {
                    hdr.glider_type = extractHeaderValue(line);
                }
            } else if (eqlBytes(code, "GID")) {
                if (hdr.glider_id.len == 0) {
                    hdr.glider_id = extractHeaderValue(line);
                }
            } else if (eqlBytes(code, "FTY")) {
                if (hdr.fr_type.len == 0) {
                    hdr.fr_type = extractHeaderValue(line);
                }
            } else if (eqlBytes(code, "DTM")) {
                if (hdr.gps_datum.len == 0) {
                    hdr.gps_datum = extractHeaderValue(line);
                }
            }
        }
    }

    return hdr;
}

// ---------------------------------------------------------------------------
// 2. B 记录定位点解析 (35 字节定宽 ASCII)
// ---------------------------------------------------------------------------

/// 零分配解析单条 FAI IGC `B` 定位记录（至少 35 字节）
pub fn parseIgcBRecord(raw_line: []const u8): IgcFix? {
    const line = stripCr(raw_line);
    if (line.len < 35 || line[0] != 'B') {
        return null;
    }

    // UTC time: HHMMSS (1..7)
    const hh = parseFixedU32(line[1..3]) ?? return null;
    const mm = parseFixedU32(line[3..5]) ?? return null;
    const ss = parseFixedU32(line[5..7]) ?? return null;
    if (hh > 23 || mm > 59 || ss > 59) {
        return null;
    }

    // Latitude: DDMMmmm + N/S (7..15)
    const lat_dd = parseFixedU32(line[7..9]) ?? return null;
    const lat_mm = parseFixedU32(line[9..11]) ?? return null;
    const lat_mmm = parseFixedU32(line[11..14]) ?? return null;
    const lat_hemi = line[14];
    if (lat_dd > 90 || lat_mm > 59 || (lat_dd == 90 && (lat_mm != 0 || lat_mmm != 0))) {
        return null;
    }
    if (lat_hemi != 'N' && lat_hemi != 'S') {
        return null;
    }
    let lat_deg = f64(lat_dd) + (f64(lat_mm) + f64(lat_mmm) / 1000.0) / 60.0;
    if (lat_hemi == 'S') {
        lat_deg = -lat_deg;
    }

    // Longitude: DDDMMmmm + E/W (15..24)
    const lon_ddd = parseFixedU32(line[15..18]) ?? return null;
    const lon_mm = parseFixedU32(line[18..20]) ?? return null;
    const lon_mmm = parseFixedU32(line[20..23]) ?? return null;
    const lon_hemi = line[23];
    if (lon_ddd > 180 || lon_mm > 59 || (lon_ddd == 180 && (lon_mm != 0 || lon_mmm != 0))) {
        return null;
    }
    if (lon_hemi != 'E' && lon_hemi != 'W') {
        return null;
    }
    let lon_deg = f64(lon_ddd) + (f64(lon_mm) + f64(lon_mmm) / 1000.0) / 60.0;
    if (lon_hemi == 'W') {
        lon_deg = -lon_deg;
    }

    // Validity: 'A' (3D) or 'V' (2D) (24)
    const validity = line[24];
    if (validity != 'A' && validity != 'V') {
        return null;
    }

    // Pressure & GNSS altitudes (25..30, 30..35)
    const pressure_alt_m = parseFixedAlt5(line[25..30]) ?? return null;
    const gnss_alt_m = parseFixedAlt5(line[30..35]) ?? return null;

    return IgcFix {
        hour: hh as u8,
        minute: mm as u8,
        second: ss as u8,
        lat_deg: lat_deg,
        lon_deg: lon_deg,
        validity: validity,
        pressure_alt_m: pressure_alt_m,
        gnss_alt_m: gnss_alt_m,
    };
}

// ---------------------------------------------------------------------------
// 3. B 记录零分配迭代器与轨迹统计汇总
// ---------------------------------------------------------------------------

/// 零分配迭代 IGC 文件中全部合法 `B` 定位记录（兼容 `\r\n` 与 `\n`）
pub struct IgcFixIter {
    data: []const u8,
    pos: usize,

    pub fn init(data: []const u8): IgcFixIter {
        return IgcFixIter {
            data: data,
            pos: 0,
        };
    }

    pub fn next(*_): IgcFix? {
        while (ls := nextLine(_.data, _.pos)) {
            _.pos = ls.next_pos;
            if (ls.line.len >= 35 && ls.line[0] == 'B') {
                if (fix := parseIgcBRecord(ls.line)) {
                    return fix;
                }
            }
        }
        return null;
    }
}

/// 单遍统计 IGC 飞行轨迹（定位数、3D 有效数、高度极值、经纬度包围盒与跨午夜 UTC 总时长）
pub fn summarizeIgc(data: []const u8): IgcStats {
    let stats = IgcStats {
        fix_count: 0,
        valid_3d_count: 0,
        min_alt_m: 0,
        max_alt_m: 0,
        duration_sec: 0,
        min_lat: 0.0,
        max_lat: 0.0,
        min_lon: 0.0,
        max_lon: 0.0,
    };

    let it = IgcFixIter.init(data);
    let prev_sec: u32 = 0;

    while (fix := it.next()) {
        const alt = if (fix.validity == 'A') fix.gnss_alt_m else fix.pressure_alt_m;
        const cur_sec = fix.timeSeconds();

        if (fix.validity == 'A') {
            stats.valid_3d_count += 1;
        }

        if (stats.fix_count == 0) {
            stats.min_alt_m = alt;
            stats.max_alt_m = alt;
            stats.min_lat = fix.lat_deg;
            stats.max_lat = fix.lat_deg;
            stats.min_lon = fix.lon_deg;
            stats.max_lon = fix.lon_deg;
        } else {
            if (alt < stats.min_alt_m) stats.min_alt_m = alt;
            if (alt > stats.max_alt_m) stats.max_alt_m = alt;
            if (fix.lat_deg < stats.min_lat) stats.min_lat = fix.lat_deg;
            if (fix.lat_deg > stats.max_lat) stats.max_lat = fix.lat_deg;
            if (fix.lon_deg < stats.min_lon) stats.min_lon = fix.lon_deg;
            if (fix.lon_deg > stats.max_lon) stats.max_lon = fix.lon_deg;

            // 处理跨午夜 UTC 翻转（23:59:59 -> 00:00:00）
            const delta = if (cur_sec >= prev_sec) (cur_sec - prev_sec) else ((86400 - prev_sec) + cur_sec);
            stats.duration_sec += delta;
        }

        prev_sec = cur_sec;
        stats.fix_count += 1;
    }

    return stats;
}
