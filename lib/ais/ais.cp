// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc ITU-R M.1371 / IEC 61162-1 Maritime AIS Decoder)

/// AIS 经度/纬度不可用哨兵值（1/10000 分整数原始值）
pub const AIS_LON_UNAVAILABLE: i32 = 108600000; // 181.0 * 600000
pub const AIS_LAT_UNAVAILABLE: i32 = 54600000;  // 91.0 * 600000

/// NMEA 0183 / IEC 61162-1 AIS 封装句零拷贝视图 (`!AIVDM` / `!AIVDO`)
pub struct AisSentence {
    is_own_vessel: bool,
    frag_count: u8,
    frag_num: u8,
    seq_id: []const u8,
    channel: u8,
    payload: []const u8,
    fill_bits: u8,
}

/// AIS Types 1, 2, 3 — Class A 船舶动态位置报告 (168 bits)
pub struct AisPositionReport {
    msg_type: u8,
    repeat: u8,
    mmsi: u32,
    nav_status: u8,
    rot: i8,
    sog_knots: f64,
    pos_accuracy: bool,
    lon_deg: f64,
    lat_deg: f64,
    lon_available: bool,
    lat_available: bool,
    cog_deg: f64,
    true_heading: u16,
    timestamp_sec: u8,
}

/// AIS Type 18 — Class B 船舶标准位置报告 (168 bits)
pub struct AisClassBPosition {
    msg_type: u8,
    repeat: u8,
    mmsi: u32,
    sog_knots: f64,
    pos_accuracy: bool,
    lon_deg: f64,
    lat_deg: f64,
    lon_available: bool,
    lat_available: bool,
    cog_deg: f64,
    true_heading: u16,
    timestamp_sec: u8,
}

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn parseHexNibble(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn parseHexByte(hi: u8, lo: u8): u8? {
    const h = parseHexNibble(hi) ?? return null;
    const l = parseHexNibble(lo) ?? return null;
    return (h << 4) | l;
}

/// AIS 6-bit ASCII Armor 字符逆映射：`48..87` -> `ch - 48`；`96..119` -> `ch - 56`；其余返回 `null`
pub fn aisUnarmorChar(ch: u8): u8? {
    if (ch >= 48 && ch <= 87) {
        return ch - 48;
    }
    if (ch >= 96 && ch <= 119) {
        return ch - 56;
    }
    return null;
}

/// 快速提取 AIS 装甲载荷的首 6-bit 报文类型编号（1..27）
pub fn aisMessageType(payload: []const u8): u8? {
    if (payload.len == 0) return null;
    return aisUnarmorChar(payload[0]);
}

/// 解析并校验 `!AIVDM` / `!AIVDO` 封装句（严格校验 XOR `*HH` 校验和、分片号与 `fill_bits <= 5`）
pub fn parseAisSentence(line: []const u8): AisSentence? {
    let end = line.len;
    while (end > 0 && (line[end - 1] == '\r' || line[end - 1] == '\n')) {
        end -= 1;
    }
    const s = line[0..end];
    // 最小长度: "!AIVDM,1,1,,A,0,0*00"
    if (s.len < 16 || s[0] != '!') return null;

    // 定位 `*HH` 校验和
    if (s.len < 4 || s[s.len - 3] != '*') return null;
    const star_pos = s.len - 3;
    const expected_chk = parseHexByte(s[star_pos + 1], s[star_pos + 2]) ?? return null;

    // 计算 `!` 与 `*` 之间字节的 XOR 校验和
    let xor_sum: u8 = 0;
    let ci: usize = 1;
    while (ci < star_pos) {
        xor_sum = xor_sum ^ s[ci];
        ci += 1;
    }
    if (xor_sum != expected_chk) return null;

    // 切分 7 个逗号字段
    let fields: [7][]const u8 = undefined;
    let fidx: usize = 0;
    let fstart: usize = 0;
    let i: usize = 0;
    while (i < star_pos) {
        if (s[i] == ',') {
            if (fidx >= 6) return null;
            fields[fidx] = s[fstart..i];
            fidx += 1;
            fstart = i + 1;
        }
        i += 1;
    }
    if (fidx != 6) return null;
    fields[6] = s[fstart..star_pos];

    // 字段 0: !AIVDM / !AIVDO
    let is_own = false;
    if (strEql(fields[0], "!AIVDM")) {
        is_own = false;
    } else if (strEql(fields[0], "!AIVDO")) {
        is_own = true;
    } else {
        return null;
    }

    // 字段 1, 2: frag_count (1..9), frag_num (1..frag_count)
    if (fields[1].len != 1 || fields[2].len != 1) return null;
    if (fields[1][0] < '1' || fields[1][0] > '9') return null;
    if (fields[2][0] < '1' || fields[2][0] > '9') return null;
    const frag_count: u8 = fields[1][0] - '0';
    const frag_num: u8 = fields[2][0] - '0';
    if (frag_num > frag_count) return null;

    // 字段 3: seq_id (可为空或单字符数字)
    const seq_id = fields[3];

    // 字段 4: channel ('A', 'B', '1', '2' 或空)
    let channel: u8 = 0;
    if (fields[4].len == 1) {
        channel = fields[4][0];
    } else if (fields[4].len > 1) {
        return null;
    }

    // 字段 5: payload (非空)
    const payload = fields[5];
    if (payload.len == 0) return null;

    // 字段 6: fill_bits (0..5)
    if (fields[6].len != 1) return null;
    if (fields[6][0] < '0' || fields[6][0] > '5') return null;
    const fill_bits: u8 = fields[6][0] - '0';

    return AisSentence {
        is_own_vessel: is_own,
        frag_count: frag_count,
        frag_num: frag_num,
        seq_id: seq_id,
        channel: channel,
        payload: payload,
        fill_bits: fill_bits,
    };
}

/// AIS 6-bit ASCII Armor 零分配比特流读取器（直接从装甲载荷按位读取，零额外缓冲）
pub struct AisBitReader {
    payload: []const u8,
    total_bits: usize,
    bit_pos: usize = 0,

    pub static fn init(payload: []const u8, fill_bits: u8): AisBitReader? {
        if (fill_bits > 5) return null;
        const raw_bits = payload.len * 6;
        if (raw_bits < usize(fill_bits)) return null;
        return AisBitReader {
            payload: payload,
            total_bits: raw_bits - usize(fill_bits),
            bit_pos: 0,
        };
    }

    pub fn remainingBits(): usize {
        return _.total_bits - _.bit_pos;
    }

    /// 读取 1..32 位无符号整数（大端高位优先）
    pub fn readU32(*_, bits: u8): u32? {
        if (bits == 0) return 0;
        if (bits > 32) return null;
        if (usize(bits) > _.total_bits - _.bit_pos) return null;

        let val: u32 = 0;
        let rem = usize(bits);
        while (rem > 0) {
            const char_idx = _.bit_pos / 6;
            const shift_u8: u8 = (5 - (_.bit_pos % 6)) as u8;
            const six = aisUnarmorChar(_.payload[char_idx]) ?? return null;
            const bit: u32 = u32((six >> (shift_u8 as u3)) & 1);
            val = (val << 1) | bit;
            _.bit_pos += 1;
            rem -= 1;
        }
        return val;
    }

    /// 读取 1..32 位二补码有符号整数（自动符号扩展，用于经纬度与转向率 ROT）
    pub fn readI32(*_, bits: u8): i32? {
        if (bits == 0 || bits > 32) return null;
        const raw = _.readU32(bits) ?? return null;
        if (bits == 32) {
            return raw as i32;
        }
        const shift: u5 = (bits - 1) as u5;
        const sign_bit: u32 = u32(1) << shift;
        if ((raw & sign_bit) != 0) {
            const full: i64 = i64(1) << (bits as u6);
            const signed_val: i64 = i64(raw) - full;
            return signed_val as i32;
        }
        return raw as i32;
    }

    /// 读取 `char_count` 个 6-bit AIS ASCII 字符到 `out`（`0..31` -> `+64`；`32..63` -> 原值；自动剥离末尾 `@` 与空格）
    pub fn readSixbitString(*_, char_count: usize, out: []u8): usize? {
        if (char_count * 6 > _.total_bits - _.bit_pos) return null;
        if (char_count > out.len) return null;

        let i: usize = 0;
        while (i < char_count) {
            const v = _.readU32(6) ?? return null;
            const ch: u8 = if (v < 32) (v + 64) as u8 else v as u8;
            out[i] = ch;
            i += 1;
        }

        let trimmed = char_count;
        while (trimmed > 0 && (out[trimmed - 1] == '@' || out[trimmed - 1] == ' ')) {
            trimmed -= 1;
        }
        return trimmed;
    }
}

/// 解码 AIS Types 1 / 2 / 3 — Class A 船舶动态位置报告（要求 >= 168 bits）
pub fn parseAisPositionReport(payload: []const u8, fill_bits: u8): AisPositionReport? {
    let br = AisBitReader.init(payload, fill_bits) ?? return null;
    if (br.remainingBits() < 168) return null;

    const msg_type = (br.readU32(6) ?? return null) as u8;
    if (msg_type != 1 && msg_type != 2 && msg_type != 3) return null;

    const repeat = (br.readU32(2) ?? return null) as u8;
    const mmsi = br.readU32(30) ?? return null;
    const nav_status = (br.readU32(4) ?? return null) as u8;
    const rot = (br.readI32(8) ?? return null) as i8;
    const sog_raw = br.readU32(10) ?? return null;
    const acc_raw = br.readU32(1) ?? return null;
    const lon_raw = br.readI32(28) ?? return null;
    const lat_raw = br.readI32(27) ?? return null;
    const cog_raw = br.readU32(12) ?? return null;
    const true_heading = (br.readU32(9) ?? return null) as u16;
    const timestamp_sec = (br.readU32(6) ?? return null) as u8;

    // 校验剩余 25 位装甲字符合法性
    * = br.readU32(25) ?? return null;

    const lon_avail = lon_raw != AIS_LON_UNAVAILABLE && lon_raw >= -108000000 && lon_raw <= 108000000;
    const lat_avail = lat_raw != AIS_LAT_UNAVAILABLE && lat_raw >= -54000000 && lat_raw <= 54000000;

    return AisPositionReport {
        msg_type: msg_type,
        repeat: repeat,
        mmsi: mmsi,
        nav_status: nav_status,
        rot: rot,
        sog_knots: (sog_raw as f64) / 10.0,
        pos_accuracy: acc_raw != 0,
        lon_deg: (lon_raw as f64) / 600000.0,
        lat_deg: (lat_raw as f64) / 600000.0,
        lon_available: lon_avail,
        lat_available: lat_avail,
        cog_deg: (cog_raw as f64) / 10.0,
        true_heading: true_heading,
        timestamp_sec: timestamp_sec,
    };
}

/// 解码 AIS Type 18 — Class B 船舶标准位置报告（要求 >= 168 bits）
pub fn parseAisClassBPosition(payload: []const u8, fill_bits: u8): AisClassBPosition? {
    let br = AisBitReader.init(payload, fill_bits) ?? return null;
    if (br.remainingBits() < 168) return null;

    const msg_type = (br.readU32(6) ?? return null) as u8;
    if (msg_type != 18) return null;

    const repeat = (br.readU32(2) ?? return null) as u8;
    const mmsi = br.readU32(30) ?? return null;
    // 跳过 8-bit regional reserved
    * = br.readU32(8) ?? return null;
    const sog_raw = br.readU32(10) ?? return null;
    const acc_raw = br.readU32(1) ?? return null;
    const lon_raw = br.readI32(28) ?? return null;
    const lat_raw = br.readI32(27) ?? return null;
    const cog_raw = br.readU32(12) ?? return null;
    const true_heading = (br.readU32(9) ?? return null) as u16;
    const timestamp_sec = (br.readU32(6) ?? return null) as u8;

    // 校验剩余 29 位装甲字符合法性
    * = br.readU32(29) ?? return null;

    const lon_avail = lon_raw != AIS_LON_UNAVAILABLE && lon_raw >= -108000000 && lon_raw <= 108000000;
    const lat_avail = lat_raw != AIS_LAT_UNAVAILABLE && lat_raw >= -54000000 && lat_raw <= 54000000;

    return AisClassBPosition {
        msg_type: msg_type,
        repeat: repeat,
        mmsi: mmsi,
        sog_knots: (sog_raw as f64) / 10.0,
        pos_accuracy: acc_raw != 0,
        lon_deg: (lon_raw as f64) / 600000.0,
        lat_deg: (lat_raw as f64) / 600000.0,
        lon_available: lon_avail,
        lat_available: lat_avail,
        cog_deg: (cog_raw as f64) / 10.0,
        true_heading: true_heading,
        timestamp_sec: timestamp_sec,
    };
}
