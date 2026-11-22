// Origin: Pure-CP Stdlib (Zero-Alloc RS-274 / Marlin / RepRap G-Code Parser) | Created: 2026-11-22

/// 单个 G-Code 词（字母 + 浮点值，零拷贝原始数值切片）
pub struct GcodeWord {
    letter: u8,
    value: f64,
    raw_value: []const u8,
}

/// 单行结构化 G-Code 指令（零拷贝注释切片）
pub struct GcodeLine {
    command_letter: u8,
    command_code: i32,
    has_command: bool,
    line_number: u32?,
    x: f64,
    y: f64,
    z: f64,
    e: f64,
    f: f64,
    s: f64,
    has_x: bool,
    has_y: bool,
    has_z: bool,
    has_e: bool,
    has_f: bool,
    has_s: bool,
    comment: []const u8,
}

/// 完整 G-Code 程序的刀路与 3D 打印统计汇总
pub struct GcodeStats {
    command_count: u32,
    move_count: u32,
    min_x: f64,
    max_x: f64,
    min_y: f64,
    max_y: f64,
    min_z: f64,
    max_z: f64,
    total_extrusion_e: f64,
    layer_changes: u32,
    is_metric: bool,
}

// ---------------------------------------------------------------------------
// 内部零分配辅助函数
// ---------------------------------------------------------------------------

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn isAlpha(c: u8): bool {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn toUpperAscii(c: u8): u8 {
    if (c >= 'a' && c <= 'z') {
        return c - 32;
    }
    return c;
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
    if (digits == 0 || i != s.len) {
        return null;
    }
    const res = int_part + frac_part;
    return if (neg) -res else res;
}

fn parseU8Decimal(s: []const u8): u8? {
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
        if (acc > 255) {
            return null;
        }
        i += 1;
    }
    return acc as u8;
}

// ---------------------------------------------------------------------------
// 1. RepRap / Marlin 串口校验和验证
// ---------------------------------------------------------------------------

/// 计算 RepRap/Marlin G-Code 逐字节 XOR 校验和（对 `*` 之前全部字节异或）
pub fn gcodeChecksum(prefix: []const u8): u8 {
    let cs: u8 = 0;
    let i: usize = 0;
    while (i < prefix.len) {
        cs = cs ^ prefix[i];
        i += 1;
    }
    return cs;
}

/// 校验 RepRap/Marlin 带 `*<xor>` 校验和的 G-Code 行
pub fn verifyGcodeChecksum(line: []const u8): bool {
    const trimmed = trimAscii(line);
    if (trimmed.len == 0) {
        return false;
    }
    // 查找校验和分隔符 '*'（在行尾注释 ';' 之前）
    let star_pos: usize? = null;
    let i: usize = 0;
    while (i < trimmed.len) {
        const c = trimmed[i];
        if (c == ';') {
            break;
        }
        if (c == '*') {
            star_pos = i;
            break;
        }
        i += 1;
    }
    const sp = star_pos ?? return false;
    if (sp == 0) {
        return false;
    }
    const expected = gcodeChecksum(trimmed[0..sp]);

    // 提取 '*' 之后的数字部分（直到空白或 ';'）
    let num_start = sp + 1;
    let num_end = num_start;
    while (num_end < trimmed.len && !isSpace(trimmed[num_end]) && trimmed[num_end] != ';') {
        num_end += 1;
    }
    const parsed = parseU8Decimal(trimmed[num_start..num_end]) ?? return false;
    return parsed == expected;
}

// ---------------------------------------------------------------------------
// 2. Word（字母 + 浮点值）零分配迭代器
// ---------------------------------------------------------------------------

/// 零分配 G-Code 单行参数词迭代器
/// 自动跳过 `;` 行尾注释、`(...)` 括号注释与 `*checksum`，兼容紧凑写法如 `G1X10.5Y-2.25F1500`
pub struct GcodeWordIter {
    line: []const u8,
    pos: usize,

    pub fn init(line: []const u8): GcodeWordIter {
        return GcodeWordIter {
            line: line,
            pos: 0,
        };
    }

    pub fn next(*_): GcodeWord? {
        const s = _.line;
        while (_.pos < s.len) {
            const c = s[_.pos];
            if (isSpace(c)) {
                _.pos += 1;
                continue;
            }
            if (c == ';' || c == '*') {
                _.pos = s.len;
                return null;
            }
            if (c == '(') {
                _.pos += 1;
                while (_.pos < s.len && s[_.pos] != ')') {
                    _.pos += 1;
                }
                if (_.pos < s.len && s[_.pos] == ')') {
                    _.pos += 1;
                }
                continue;
            }
            if (!isAlpha(c)) {
                _.pos += 1;
                continue;
            }

            const letter = toUpperAscii(c);
            _.pos += 1;

            // 允许字母与数值之间存在可选空格
            while (_.pos < s.len && (s[_.pos] == ' ' || s[_.pos] == '\t')) {
                _.pos += 1;
            }

            const val_start = _.pos;
            if (_.pos < s.len && (s[_.pos] == '+' || s[_.pos] == '-')) {
                _.pos += 1;
            }
            let saw_dot = false;
            while (_.pos < s.len) {
                const ch = s[_.pos];
                if (isDigit(ch)) {
                    _.pos += 1;
                } else if (ch == '.' && !saw_dot) {
                    saw_dot = true;
                    _.pos += 1;
                } else {
                    break;
                }
            }

            const raw_val = s[val_start.._.pos];
            if (v := parseF64(raw_val)) {
                return GcodeWord {
                    letter: letter,
                    value: v,
                    raw_value: raw_val,
                };
            }
            // 畸形数字：跳过当前词继续扫描
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// 3. 单行结构化解析与整程序行迭代器
// ---------------------------------------------------------------------------

fn extractLineComment(line: []const u8): []const u8 {
    let i: usize = 0;
    let paren_comment: []const u8 = "";
    while (i < line.len) {
        const c = line[i];
        if (c == ';') {
            return trimAscii(line[i + 1 ..]);
        }
        if (c == '(' && paren_comment.len == 0) {
            const start = i + 1;
            let j = start;
            while (j < line.len && line[j] != ')') {
                j += 1;
            }
            if (j < line.len && line[j] == ')') {
                paren_comment = trimAscii(line[start..j]);
                i = j + 1;
                continue;
            }
            return trimAscii(line[start..]);
        }
        i += 1;
    }
    return paren_comment;
}

/// 零分配解析单行 G-Code 为结构化 `GcodeLine`
pub fn parseGcodeLine(line: []const u8): GcodeLine {
    let res = GcodeLine {
        command_letter: 0,
        command_code: 0,
        has_command: false,
        line_number: null,
        x: 0.0,
        y: 0.0,
        z: 0.0,
        e: 0.0,
        f: 0.0,
        s: 0.0,
        has_x: false,
        has_y: false,
        has_z: false,
        has_e: false,
        has_f: false,
        has_s: false,
        comment: extractLineComment(line),
    };

    let it = GcodeWordIter.init(line);
    while (w := it.next()) {
        if (w.letter == 'N') {
            if (res.line_number == null && w.value >= 0.0) {
                res.line_number = w.value as u32;
            }
        } else if (w.letter == 'G' || w.letter == 'M' || w.letter == 'T') {
            if (!res.has_command) {
                res.command_letter = w.letter;
                res.command_code = w.value as i32;
                res.has_command = true;
            }
        } else if (w.letter == 'X') {
            res.x = w.value;
            res.has_x = true;
        } else if (w.letter == 'Y') {
            res.y = w.value;
            res.has_y = true;
        } else if (w.letter == 'Z') {
            res.z = w.value;
            res.has_z = true;
        } else if (w.letter == 'E') {
            res.e = w.value;
            res.has_e = true;
        } else if (w.letter == 'F') {
            res.f = w.value;
            res.has_f = true;
        } else if (w.letter == 'S') {
            res.s = w.value;
            res.has_s = true;
        }
    }
    return res;
}

fn isMeaningfulGcodeLine(gl: GcodeLine): bool {
    return gl.has_command || gl.has_x || gl.has_y || gl.has_z || gl.has_e || gl.has_f || gl.has_s || gl.line_number != null;
}

/// 零分配逐行迭代完整 G-Code 程序，自动跳过纯空行与纯注释行，兼容 `\r\n` 与 `\n`
pub struct GcodeIter {
    program: []const u8,
    pos: usize,

    pub fn init(program: []const u8): GcodeIter {
        return GcodeIter {
            program: program,
            pos: 0,
        };
    }

    pub fn next(*_): GcodeLine? {
        const s = _.program;
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
            const line = s[line_start..line_end];
            const parsed = parseGcodeLine(line);
            if (isMeaningfulGcodeLine(parsed)) {
                return parsed;
            }
        }
        return null;
    }
}

// ---------------------------------------------------------------------------
// 4. 刀路包围盒与 3D 打印状态机统计汇总
// ---------------------------------------------------------------------------

/// 单遍解析完整 G-Code 程序，追踪 G90/G91、M82/M83、G92、G20/G21 状态，
/// 计算真实 3D 包围盒、移动指令数、换层数与累计挤出耗材量
pub fn summarizeGcode(program: []const u8): GcodeStats {
    let stats = GcodeStats {
        command_count: 0,
        move_count: 0,
        min_x: 0.0,
        max_x: 0.0,
        min_y: 0.0,
        max_y: 0.0,
        min_z: 0.0,
        max_z: 0.0,
        total_extrusion_e: 0.0,
        layer_changes: 0,
        is_metric: true,
    };

    let abs_xyz = true;
    let abs_e = true;
    let cur_x: f64 = 0.0;
    let cur_y: f64 = 0.0;
    let cur_z: f64 = 0.0;
    let cur_e: f64 = 0.0;
    let prev_z: f64 = 0.0;
    let has_bounds = false;

    let it = GcodeIter.init(program);
    while (gl := it.next()) {
        if (gl.has_command) {
            stats.command_count += 1;
        }

        if (gl.has_command && gl.command_letter == 'G') {
            if (gl.command_code == 20) {
                stats.is_metric = false;
                continue;
            }
            if (gl.command_code == 21) {
                stats.is_metric = true;
                continue;
            }
            if (gl.command_code == 90) {
                abs_xyz = true;
                abs_e = true;
                continue;
            }
            if (gl.command_code == 91) {
                abs_xyz = false;
                abs_e = false;
                continue;
            }
            if (gl.command_code == 92) {
                if (!gl.has_x && !gl.has_y && !gl.has_z && !gl.has_e) {
                    cur_x = 0.0;
                    cur_y = 0.0;
                    cur_z = 0.0;
                    cur_e = 0.0;
                    prev_z = 0.0;
                } else {
                    if (gl.has_x) cur_x = gl.x;
                    if (gl.has_y) cur_y = gl.y;
                    if (gl.has_z) {
                        cur_z = gl.z;
                        prev_z = cur_z;
                    }
                    if (gl.has_e) cur_e = gl.e;
                }
                continue;
            }
            if (gl.command_code == 0 || gl.command_code == 1 || gl.command_code == 2 || gl.command_code == 3) {
                stats.move_count += 1;
                if (gl.has_x) {
                    cur_x = if (abs_xyz) gl.x else (cur_x + gl.x);
                }
                if (gl.has_y) {
                    cur_y = if (abs_xyz) gl.y else (cur_y + gl.y);
                }
                if (gl.has_z) {
                    cur_z = if (abs_xyz) gl.z else (cur_z + gl.z);
                    if (cur_z != prev_z) {
                        stats.layer_changes += 1;
                        prev_z = cur_z;
                    }
                }
                if (gl.has_x || gl.has_y || gl.has_z) {
                    if (!has_bounds) {
                        stats.min_x = cur_x;
                        stats.max_x = cur_x;
                        stats.min_y = cur_y;
                        stats.max_y = cur_y;
                        stats.min_z = cur_z;
                        stats.max_z = cur_z;
                        has_bounds = true;
                    } else {
                        if (cur_x < stats.min_x) stats.min_x = cur_x;
                        if (cur_x > stats.max_x) stats.max_x = cur_x;
                        if (cur_y < stats.min_y) stats.min_y = cur_y;
                        if (cur_y > stats.max_y) stats.max_y = cur_y;
                        if (cur_z < stats.min_z) stats.min_z = cur_z;
                        if (cur_z > stats.max_z) stats.max_z = cur_z;
                    }
                }
                if (gl.has_e) {
                    let delta_e: f64 = 0.0;
                    if (abs_e) {
                        delta_e = gl.e - cur_e;
                        cur_e = gl.e;
                    } else {
                        delta_e = gl.e;
                        cur_e += gl.e;
                    }
                    stats.total_extrusion_e += delta_e;
                }
                continue;
            }
        } else if (gl.has_command && gl.command_letter == 'M') {
            if (gl.command_code == 82) {
                abs_e = true;
            } else if (gl.command_code == 83) {
                abs_e = false;
            }
        }
    }

    return stats;
}
