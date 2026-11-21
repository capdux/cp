// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Biovision Hierarchy BVH Motion Capture Parser)

/// BVH 关节通道类型枚举
pub enum BvhChannel: u8 {
    x_pos = 0,
    y_pos = 1,
    z_pos = 2,
    x_rot = 3,
    y_rot = 4,
    z_rot = 5,
    unknown = 255,
}

/// BVH 单个关节节点元信息（零拷贝名称切片）
pub struct BvhJoint {
    name: []const u8,
    depth: u16,
    is_root: bool,
    is_end_site: bool,
    offset_x: f64,
    offset_y: f64,
    offset_z: f64,
    channel_count: u8,
    channels: [6]BvhChannel,
    channel_offset: u32,
}

/// BVH 文件头（HIERARCHY 统计 + MOTION 元信息）
pub struct BvhHeader {
    joint_count: u32,
    end_site_count: u32,
    total_channels: u32,
    frame_count: u32,
    frame_time_sec: f64,
    frames_offset: usize,

    pub fn fps(): f64 {
        if (_.frame_time_sec <= 0.0) {
            return 0.0;
        }
        return 1.0 / _.frame_time_sec;
    }

    pub fn durationSec(): f64 {
        return f64(_.frame_count) * _.frame_time_sec;
    }
}

// ---------------------------------------------------------------------------
// 内部零分配词法与数值解析辅助函数
// ---------------------------------------------------------------------------

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isLineSpace(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn toLowerAscii(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
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

fn eqlAsciiIgnoreCase(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (toLowerAscii(a[i]) != toLowerAscii(b[i])) {
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

fn parseU32(s: []const u8): u32? {
    if (s.len == 0) {
        return null;
    }
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < s.len) {
        if (!isDigit(s[i])) {
            return null;
        }
        acc = acc * 10 + u64(s[i] - '0');
        if (acc > 0xFFFFFFFF) {
            return null;
        }
        i += 1;
    }
    return acc as u32;
}

fn parseF64(s: []const u8): f64? {
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
    let int_part: f64 = 0.0;
    let digits: usize = 0;
    while (i < s.len && isDigit(s[i])) {
        int_part = int_part * 10.0 + f64(s[i] - '0');
        digits += 1;
        i += 1;
    }
    let frac_part: f64 = 0.0;
    let frac_scale: f64 = 1.0;
    if (i < s.len && s[i] == '.') {
        i += 1;
        while (i < s.len && isDigit(s[i])) {
            frac_scale *= 0.1;
            frac_part += f64(s[i] - '0') * frac_scale;
            digits += 1;
            i += 1;
        }
    }
    if (digits == 0) {
        return null;
    }
    let val = int_part + frac_part;
    if (i < s.len && (s[i] == 'e' || s[i] == 'E')) {
        i += 1;
        let exp_neg = false;
        if (i < s.len && s[i] == '-') {
            exp_neg = true;
            i += 1;
        } else if (i < s.len && s[i] == '+') {
            i += 1;
        }
        let exp_val: u32 = 0;
        let exp_digits: usize = 0;
        while (i < s.len && isDigit(s[i])) {
            if (exp_val < 1000) {
                exp_val = exp_val * 10 + u32(s[i] - '0');
            }
            exp_digits += 1;
            i += 1;
        }
        if (exp_digits == 0 || exp_val > 308) {
            return null;
        }
        let e: u32 = 0;
        while (e < exp_val) {
            if (exp_neg) {
                val *= 0.1;
            } else {
                val *= 10.0;
            }
            e += 1;
        }
    }
    if (i != s.len) {
        return null;
    }
    return if (neg) -val else val;
}

struct Token {
    text: []const u8,
    next_pos: usize,
}

fn peekToken(data: []const u8, pos: usize): Token? {
    let i = pos;
    while (i < data.len && isSpace(data[i])) {
        i += 1;
    }
    if (i >= data.len) {
        return null;
    }
    if (data[i] == '{' || data[i] == '}') {
        return Token {
            text: data[i .. i + 1],
            next_pos: i + 1,
        };
    }
    const start = i;
    while (i < data.len && !isSpace(data[i]) && data[i] != '{' && data[i] != '}') {
        i += 1;
    }
    return Token {
        text: data[start..i],
        next_pos: i,
    };
}

// ---------------------------------------------------------------------------
// 1. 通道解析与格式快速探测
// ---------------------------------------------------------------------------

/// 解析 BVH 通道名称（兼容大小写）为 `BvhChannel`
pub fn parseBvhChannel(name: []const u8): BvhChannel {
    if (eqlAsciiIgnoreCase(name, "Xposition")) return BvhChannel.x_pos;
    if (eqlAsciiIgnoreCase(name, "Yposition")) return BvhChannel.y_pos;
    if (eqlAsciiIgnoreCase(name, "Zposition")) return BvhChannel.z_pos;
    if (eqlAsciiIgnoreCase(name, "Xrotation")) return BvhChannel.x_rot;
    if (eqlAsciiIgnoreCase(name, "Yrotation")) return BvhChannel.y_rot;
    if (eqlAsciiIgnoreCase(name, "Zrotation")) return BvhChannel.z_rot;
    return BvhChannel.unknown;
}

/// 跳过前导空白后检测是否以 `"HIERARCHY"` 开头
pub fn isBvh(data: []const u8): bool {
    if (tok := peekToken(data, 0)) {
        return eqlBytes(tok.text, "HIERARCHY");
    }
    return false;
}

// ---------------------------------------------------------------------------
// 2. HIERARCHY 关节树深度优先零分配迭代器
// ---------------------------------------------------------------------------

/// 零分配深度优先遍历 BVH `HIERARCHY` 中的 `ROOT` / `JOINT` / `End Site`
pub struct BvhJointIter {
    data: []const u8,
    pos: usize,
    brace_depth: u16,
    channel_offset: u32,
    started: bool,
    done: bool,
    found_motion: bool,
    motion_after_pos: usize,
    malformed: bool,

    pub fn init(data: []const u8): BvhJointIter {
        return BvhJointIter {
            data: data,
            pos: 0,
            brace_depth: 0,
            channel_offset: 0,
            started: false,
            done: false,
            found_motion: false,
            motion_after_pos: 0,
            malformed: false,
        };
    }

    fn fail(*_): BvhJoint? {
        _.malformed = true;
        _.done = true;
        return null;
    }

    pub fn next(*_): BvhJoint? {
        if (_.done) {
            return null;
        }
        if (!_.started) {
            _.started = true;
            const hdr = peekToken(_.data, _.pos) ?? return _.fail();
            if (!eqlBytes(hdr.text, "HIERARCHY")) {
                return _.fail();
            }
            _.pos = hdr.next_pos;
        }

        while (true) {
            const tok_opt = peekToken(_.data, _.pos);
            if (tok_opt == null) {
                if (_.brace_depth != 0) {
                    _.malformed = true;
                }
                _.done = true;
                return null;
            }
            const tok = tok_opt ?? unreachable;
            _.pos = tok.next_pos;

            if (eqlBytes(tok.text, "}")) {
                if (_.brace_depth == 0) {
                    return _.fail();
                }
                _.brace_depth -= 1;
                continue;
            }

            if (eqlBytes(tok.text, "MOTION")) {
                if (_.brace_depth != 0) {
                    _.malformed = true;
                }
                _.found_motion = true;
                _.motion_after_pos = _.pos;
                _.done = true;
                return null;
            }

            const is_root = eqlBytes(tok.text, "ROOT");
            const is_joint = eqlBytes(tok.text, "JOINT");
            const is_end = eqlBytes(tok.text, "End");
            if (!is_root && !is_joint && !is_end) {
                return _.fail();
            }

            const node_depth = _.brace_depth;
            let joint_name: []const u8 = "";
            if (is_end) {
                const site_tok = peekToken(_.data, _.pos) ?? return _.fail();
                if (!eqlBytes(site_tok.text, "Site")) {
                    return _.fail();
                }
                _.pos = site_tok.next_pos;
                joint_name = "End Site";
            } else {
                const name_tok = peekToken(_.data, _.pos) ?? return _.fail();
                if (eqlBytes(name_tok.text, "{") || eqlBytes(name_tok.text, "}")) {
                    return _.fail();
                }
                _.pos = name_tok.next_pos;
                joint_name = name_tok.text;
            }

            const open_brace = peekToken(_.data, _.pos) ?? return _.fail();
            if (!eqlBytes(open_brace.text, "{")) {
                return _.fail();
            }
            _.pos = open_brace.next_pos;
            _.brace_depth += 1;

            let offset_x: f64 = 0.0;
            let offset_y: f64 = 0.0;
            let offset_z: f64 = 0.0;
            let channel_count: u8 = 0;
            let channels: [6]BvhChannel = [
                BvhChannel.unknown,
                BvhChannel.unknown,
                BvhChannel.unknown,
                BvhChannel.unknown,
                BvhChannel.unknown,
                BvhChannel.unknown,
            ];

            // 读取当前节点的 OFFSET 与 CHANNELS 属性，直到子块或 '}'
            while (true) {
                const next_tok = peekToken(_.data, _.pos) ?? return _.fail();
                if (eqlBytes(next_tok.text, "OFFSET")) {
                    _.pos = next_tok.next_pos;
                    const tx = peekToken(_.data, _.pos) ?? return _.fail();
                    offset_x = parseF64(tx.text) ?? return _.fail();
                    _.pos = tx.next_pos;

                    const ty = peekToken(_.data, _.pos) ?? return _.fail();
                    offset_y = parseF64(ty.text) ?? return _.fail();
                    _.pos = ty.next_pos;

                    const tz = peekToken(_.data, _.pos) ?? return _.fail();
                    offset_z = parseF64(tz.text) ?? return _.fail();
                    _.pos = tz.next_pos;
                    continue;
                }
                if (eqlBytes(next_tok.text, "CHANNELS")) {
                    if (is_end) {
                        return _.fail();
                    }
                    _.pos = next_tok.next_pos;
                    const cnt_tok = peekToken(_.data, _.pos) ?? return _.fail();
                    const cnt = parseU32(cnt_tok.text) ?? return _.fail();
                    if (cnt == 0 || cnt > 6) {
                        return _.fail();
                    }
                    _.pos = cnt_tok.next_pos;
                    channel_count = cnt as u8;

                    let ci: usize = 0;
                    while (ci < usize(cnt)) {
                        const ch_tok = peekToken(_.data, _.pos) ?? return _.fail();
                        const ch = parseBvhChannel(ch_tok.text);
                        if (ch == BvhChannel.unknown) {
                            return _.fail();
                        }
                        channels[ci] = ch;
                        _.pos = ch_tok.next_pos;
                        ci += 1;
                    }
                    continue;
                }
                // 遇到子块关键词或 '}'：不消耗，留给下一次 next() 处理
                break;
            }

            const my_offset = _.channel_offset;
            _.channel_offset += u32(channel_count);

            return BvhJoint {
                name: joint_name,
                depth: node_depth,
                is_root: is_root,
                is_end_site: is_end,
                offset_x: offset_x,
                offset_y: offset_y,
                offset_z: offset_z,
                channel_count: channel_count,
                channels: channels,
                channel_offset: my_offset,
            };
        }
    }
}

// ---------------------------------------------------------------------------
// 3. 完整 BVH 头解析 (HIERARCHY + MOTION Header)
// ---------------------------------------------------------------------------

fn skipLineEnd(data: []const u8, pos: usize): usize {
    let i = pos;
    while (i < data.len && isLineSpace(data[i])) {
        i += 1;
    }
    if (i < data.len && data[i] == '\r') {
        i += 1;
    }
    if (i < data.len && data[i] == '\n') {
        i += 1;
    }
    return i;
}

/// 完整扫描 HIERARCHY 与 MOTION 头，返回关节统计、总通道数、帧数、帧间隔及帧数据起始偏移
pub fn parseBvhHeader(data: []const u8): BvhHeader? {
    let it = BvhJointIter.init(data);
    let joint_count: u32 = 0;
    let end_site_count: u32 = 0;

    while (j := it.next()) {
        if (j.is_end_site) {
            end_site_count += 1;
        } else {
            joint_count += 1;
        }
    }

    if (it.malformed || !it.found_motion || joint_count == 0) {
        return null;
    }

    let pos = it.motion_after_pos;

    // 解析 "Frames: N"
    const t_frames = peekToken(data, pos) ?? return null;
    if (!eqlBytes(t_frames.text, "Frames:")) {
        return null;
    }
    pos = t_frames.next_pos;
    const t_fc = peekToken(data, pos) ?? return null;
    const frame_count = parseU32(t_fc.text) ?? return null;
    pos = t_fc.next_pos;

    // 解析 "Frame Time: T"
    const t_frame = peekToken(data, pos) ?? return null;
    if (!eqlBytes(t_frame.text, "Frame")) {
        return null;
    }
    pos = t_frame.next_pos;
    const t_time = peekToken(data, pos) ?? return null;
    if (!eqlBytes(t_time.text, "Time:")) {
        return null;
    }
    pos = t_time.next_pos;
    const t_ft = peekToken(data, pos) ?? return null;
    const frame_time_sec = parseF64(t_ft.text) ?? return null;
    if (frame_time_sec <= 0.0) {
        return null;
    }
    const frames_offset = skipLineEnd(data, t_ft.next_pos);

    return BvhHeader {
        joint_count: joint_count,
        end_site_count: end_site_count,
        total_channels: it.channel_offset,
        frame_count: frame_count,
        frame_time_sec: frame_time_sec,
        frames_offset: frames_offset,
    };
}

// ---------------------------------------------------------------------------
// 4. 帧数据零分配迭代器 (BvhFloatIter & BvhFrameIter)
// ---------------------------------------------------------------------------

/// 对单帧文本切片零分配逐个解码 f64 通道值
pub struct BvhFloatIter {
    line: []const u8,
    pos: usize,
    malformed: bool,

    pub fn init(frame_line: []const u8): BvhFloatIter {
        return BvhFloatIter {
            line: frame_line,
            pos: 0,
            malformed: false,
        };
    }

    pub fn next(*_): f64? {
        const s = _.line;
        while (_.pos < s.len && isSpace(s[_.pos])) {
            _.pos += 1;
        }
        if (_.pos >= s.len) {
            return null;
        }
        const start = _.pos;
        while (_.pos < s.len && !isSpace(s[_.pos])) {
            _.pos += 1;
        }
        const tok = s[start.._.pos];
        if (v := parseF64(tok)) {
            return v;
        }
        _.malformed = true;
        _.pos = s.len;
        return null;
    }
}

/// 按行零分配产出每帧文本切片（跳过空行，并校验每帧浮点通道数严格等于 `total_channels`）
pub struct BvhFrameIter {
    data: []const u8,
    pos: usize,
    expected_channels: u32,
    emitted: u32,
    max_frames: u32,
    malformed: bool,

    pub fn init(data: []const u8, header: BvhHeader): BvhFrameIter {
        return BvhFrameIter {
            data: data,
            pos: header.frames_offset,
            expected_channels: header.total_channels,
            emitted: 0,
            max_frames: header.frame_count,
            malformed: false,
        };
    }

    pub fn next(*_): ([]const u8)? {
        if (_.emitted >= _.max_frames) {
            return null;
        }
        const s = _.data;
        while (_.pos < s.len) {
            const line_start = _.pos;
            while (_.pos < s.len && s[_.pos] != '\n') {
                _.pos += 1;
            }
            let line_end = _.pos;
            if (_.pos < s.len && s[_.pos] == '\n') {
                _.pos += 1;
            }
            if (line_end > line_start && s[line_end - 1] == '\r') {
                line_end -= 1;
            }
            const trimmed = trimAscii(s[line_start..line_end]);
            if (trimmed.len == 0) {
                continue;
            }
            // 校验该帧通道浮点计数与 total_channels 一致
            let fit = BvhFloatIter.init(trimmed);
            let cnt: u32 = 0;
            while (v := fit.next()) {
                * = v;
                cnt += 1;
            }
            if (fit.malformed || cnt != _.expected_channels) {
                _.malformed = true;
                _.pos = s.len;
                return null;
            }
            _.emitted += 1;
            return trimmed;
        }
        return null;
    }
}
