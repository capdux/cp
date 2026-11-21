// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc RCSB Protein Data Bank .pdb Structure Parser)

/// RCSB PDB 文件头部元数据（零拷贝切片）
pub struct PdbHeader {
    classification: []const u8,
    dep_date: []const u8,
    pdb_id: []const u8,
    title: []const u8,
    expdta: []const u8,
}

/// PDB v3.3 80 列定宽 ATOM / HETATM 原子记录（零拷贝切片）
pub struct PdbAtom {
    is_hetatm: bool,
    serial: i32,
    name: []const u8,
    alt_loc: u8,
    res_name: []const u8,
    chain_id: u8,
    res_seq: i32,
    x: f64,
    y: f64,
    z: f64,
    occupancy: f64,
    temp_factor: f64,
    element: []const u8,
    charge: []const u8,
    model_num: u32,

    /// 计算与另一原子的 3D 欧氏距离（单位：埃 Å）
    fn distanceTo(other: PdbAtom): f64 {
        const dx = _.x - other.x;
        const dy = _.y - other.y;
        const dz = _.z - other.z;
        return @sqrt(dx * dx + dy * dy + dz * dz);
    }
}

/// PDB CONECT 共价键连接记录（定长栈数组，零堆分配）
pub struct PdbConect {
    serial: i32,
    bonds: [4]i32,
    bond_count: u8,
}

/// PDB 单遍结构统计汇总（原子/异源原子/模型/链数、3D 包围盒与质心）
pub struct PdbStats {
    atom_count: u32,
    hetatm_count: u32,
    model_count: u32,
    chain_count: u32,
    min_x: f64,
    max_x: f64,
    min_y: f64,
    max_y: f64,
    min_z: f64,
    max_z: f64,
    centroid_x: f64,
    centroid_y: f64,
    centroid_z: f64,
}

// ---------------------------------------------------------------------------
// 内部零分配切片与数值解析辅助函数
// ---------------------------------------------------------------------------

fn isSpaceOrTab(c: u8): bool {
    return c == ' ' || c == '\t';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn trimAscii(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpaceOrTab(s[start])) {
        start += 1;
    }
    let end: usize = s.len;
    while (end > start && isSpaceOrTab(s[end - 1])) {
        end -= 1;
    }
    return s[start..end];
}

fn sliceCols(line: []const u8, start: usize, end: usize): []const u8 {
    if (start >= line.len) {
        return line[line.len..line.len];
    }
    const clamped_end = if (end < line.len) end else line.len;
    if (clamped_end <= start) {
        return line[line.len..line.len];
    }
    return line[start..clamped_end];
}

fn startsWith(line: []const u8, prefix: []const u8): bool {
    if (line.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (line[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

/// 严格判断 PDB 固定 6 列记录关键字（支持行尾提前截断的短关键字如 "END" / "MODEL" / "TITLE"）
fn isRecordTag(line: []const u8, tag: []const u8): bool {
    if (!startsWith(line, tag)) {
        return false;
    }
    if (line.len == tag.len) {
        return true;
    }
    // 若 tag 长度为 6（如 "HEADER"、"ATOM  "、"HETATM"、"EXPDTA"、"CONECT"），精确匹配前 6 字节即可
    if (tag.len >= 6) {
        return true;
    }
    // 短关键字（如 "END"、"MODEL"、"TITLE"）后必须紧跟空格或行尾，防止 "END" 误匹配 "ENDMDL"
    return line[tag.len] == ' ' || line[tag.len] == '\t';
}

/// 剥离行尾 \r 与 \n
fn stripLineEnd(line: []const u8): []const u8 {
    let end: usize = line.len;
    while (end > 0 && (line[end - 1] == '\r' || line[end - 1] == '\n')) {
        end -= 1;
    }
    return line[0..end];
}

/// 从 data[*pos..] 零拷贝切出下一行（不含 \r\n），并推进 pos
fn nextLine(data: []const u8, pos: *usize): ([]const u8)? {
    const start = pos.*;
    if (start >= data.len) {
        return null;
    }
    let i = start;
    while (i < data.len && data[i] != '\n') {
        i += 1;
    }
    let line_end = i;
    if (line_end > start && data[line_end - 1] == '\r') {
        line_end -= 1;
    }
    pos.* = if (i < data.len) (i + 1) else data.len;
    return data[start..line_end];
}

/// 严格零分配解析有符号 32 位整数
fn parseI32Strict(raw: []const u8): i32? {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) {
        return null;
    }
    let acc: i64 = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isDigit(c)) {
            return null;
        }
        acc = acc * 10 + i64(c - '0');
        if (neg) {
            if (acc > 2147483648) {
                return null;
            }
        } else {
            if (acc > 2147483647) {
                return null;
            }
        }
        i += 1;
    }
    if (neg) {
        return (-acc) as i32;
    }
    return acc as i32;
}

/// 严格零分配解析无符号 32 位整数
fn parseU32Strict(raw: []const u8): u32? {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) {
        return null;
    }
    let acc: u64 = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isDigit(c)) {
            return null;
        }
        acc = acc * 10 + u64(c - '0');
        if (acc > 4294967295) {
            return null;
        }
        i += 1;
    }
    return acc as u32;
}

/// 严格零分配解析 64 位浮点数（支持正负号、小数点与科学计数法 e/E）
fn parseF64Strict(raw: []const u8): f64? {
    const s = trimAscii(raw);
    if (s.len == 0) {
        return null;
    }
    let i: usize = 0;
    let neg = false;
    if (s[0] == '-') {
        neg = true;
        i = 1;
    } else if (s[0] == '+') {
        i = 1;
    }
    if (i >= s.len) {
        return null;
    }

    let val: f64 = 0.0;
    let digits: u32 = 0;

    while (i < s.len && isDigit(s[i])) {
        val = val * 10.0 + f64(s[i] - '0');
        digits += 1;
        i += 1;
    }

    if (i < s.len && s[i] == '.') {
        i += 1;
        let frac_scale: f64 = 0.1;
        while (i < s.len && isDigit(s[i])) {
            val += f64(s[i] - '0') * frac_scale;
            frac_scale *= 0.1;
            digits += 1;
            i += 1;
        }
    }

    if (digits == 0) {
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
        if (i >= s.len || !isDigit(s[i])) {
            return null;
        }
        let exp_val: i32 = 0;
        while (i < s.len && isDigit(s[i])) {
            if (exp_val < 1000) {
                exp_val = exp_val * 10 + i32(s[i] - '0');
            }
            i += 1;
        }
        if (exp_val > 308) {
            return null;
        }
        let scale: f64 = 1.0;
        let p: i32 = 0;
        while (p < exp_val) {
            scale *= 10.0;
            p += 1;
        }
        if (exp_neg) {
            val /= scale;
        } else {
            val *= scale;
        }
    }

    // 不允许尾随非法字符
    if (i != s.len) {
        return null;
    }

    return if (neg) -val else val;
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 快速检测缓冲区是否以标准 RCSB PDB 记录开头
pub fn isPdb(data: []const u8): bool {
    let pos: usize = 0;
    let checked: u32 = 0;
    while (checked < 32) {
        const line = nextLine(data, *pos) ?? break;
        const trimmed = trimAscii(line);
        if (trimmed.len == 0) {
            continue;
        }
        checked += 1;
        if (isRecordTag(line, "HEADER") ||
            isRecordTag(line, "TITLE") ||
            isRecordTag(line, "EXPDTA") ||
            isRecordTag(line, "COMPND") ||
            isRecordTag(line, "REMARK") ||
            isRecordTag(line, "CRYST1") ||
            isRecordTag(line, "MODEL") ||
            isRecordTag(line, "ATOM  ") ||
            isRecordTag(line, "HETATM"))
        {
            return true;
        }
        return false;
    }
    return false;
}

/// 解析 PDB 头部信息：HEADER（classification/dep_date/pdb_id）、首个 TITLE 与 EXPDTA
pub fn parsePdbHeader(data: []const u8): PdbHeader? {
    const empty = data[0..0];
    let hdr = PdbHeader{
        classification: empty,
        dep_date: empty,
        pdb_id: empty,
        title: empty,
        expdta: empty,
    };
    let found_any = false;
    let has_header = false;
    let has_title = false;
    let has_expdta = false;

    let pos: usize = 0;
    while (line := nextLine(data, *pos)) {
        if (isRecordTag(line, "END")) {
            break;
        }
        if (isRecordTag(line, "ATOM  ") || isRecordTag(line, "HETATM") || isRecordTag(line, "MODEL")) {
            break;
        }
        if (!has_header && isRecordTag(line, "HEADER")) {
            has_header = true;
            found_any = true;
            // PDB v3.3 HEADER:
            // Cols 11-50 (0-based 10..50): classification
            // Cols 51-59 (0-based 50..59): depDate
            // Cols 63-66 (0-based 62..66): idCode (4 chars)
            hdr.classification = trimAscii(sliceCols(line, 10, 50));
            hdr.dep_date = trimAscii(sliceCols(line, 50, 59));
            hdr.pdb_id = trimAscii(sliceCols(line, 62, 66));
        } else if (!has_title && isRecordTag(line, "TITLE")) {
            has_title = true;
            found_any = true;
            // PDB v3.3 TITLE: Cols 11-80 (0-based 10..80)
            hdr.title = trimAscii(sliceCols(line, 10, 80));
        } else if (!has_expdta && isRecordTag(line, "EXPDTA")) {
            has_expdta = true;
            found_any = true;
            // PDB v3.3 EXPDTA: Cols 11-79 (0-based 10..80)
            hdr.expdta = trimAscii(sliceCols(line, 10, 80));
        }
        if (has_header && has_title && has_expdta) {
            break;
        }
    }

    if (!found_any) {
        return null;
    }
    return hdr;
}

/// 按 PDB v3.3 严格定宽列解析单条 ATOM / HETATM 记录（至少 54 列，兼容 80 列）
pub fn parsePdbAtomLine(raw_line: []const u8): PdbAtom? {
    const line = stripLineEnd(raw_line);
    if (line.len < 54) {
        return null;
    }

    let is_hetatm = false;
    if (startsWith(line, "ATOM  ")) {
        is_hetatm = false;
    } else if (startsWith(line, "HETATM")) {
        is_hetatm = true;
    } else {
        return null;
    }

    // Cols 7-11 (0-based 6..11): serial
    const serial = parseI32Strict(line[6..11]) ?? return null;
    // Cols 13-16 (0-based 12..16): atom name
    const name = trimAscii(line[12..16]);
    // Col 17 (0-based 16): altLoc
    const alt_loc = line[16];
    // Cols 18-20 (0-based 17..20): resName
    const res_name = trimAscii(line[17..20]);
    // Col 22 (0-based 21): chainID
    const chain_id = line[21];
    // Cols 23-26 (0-based 22..26): resSeq
    const res_seq = parseI32Strict(line[22..26]) ?? return null;

    // Cols 31-38 (0-based 30..38): x
    const x = parseF64Strict(line[30..38]) ?? return null;
    // Cols 39-46 (0-based 38..46): y
    const y = parseF64Strict(line[38..46]) ?? return null;
    // Cols 47-54 (0-based 46..54): z
    const z = parseF64Strict(line[46..54]) ?? return null;

    // Cols 55-60 (0-based 54..60): occupancy (缺省 1.0)
    let occupancy: f64 = 1.0;
    const occ_slice = trimAscii(sliceCols(line, 54, 60));
    if (occ_slice.len > 0) {
        occupancy = parseF64Strict(occ_slice) ?? return null;
    }

    // Cols 61-66 (0-based 60..66): tempFactor (缺省 0.0)
    let temp_factor: f64 = 0.0;
    const tf_slice = trimAscii(sliceCols(line, 60, 66));
    if (tf_slice.len > 0) {
        temp_factor = parseF64Strict(tf_slice) ?? return null;
    }

    // Cols 77-78 (0-based 76..78): element
    const element = trimAscii(sliceCols(line, 76, 78));
    // Cols 79-80 (0-based 78..80): charge
    const charge = trimAscii(sliceCols(line, 78, 80));

    return PdbAtom{
        is_hetatm: is_hetatm,
        serial: serial,
        name: name,
        alt_loc: alt_loc,
        res_name: res_name,
        chain_id: chain_id,
        res_seq: res_seq,
        x: x,
        y: y,
        z: z,
        occupancy: occupancy,
        temp_factor: temp_factor,
        element: element,
        charge: charge,
        model_num: 0,
    };
}

/// 解析单条 CONECT 共价键记录（列 7..11 中心原子，列 12..31 最多 4 个键合原子）
pub fn parsePdbConectLine(raw_line: []const u8): PdbConect? {
    const line = stripLineEnd(raw_line);
    if (line.len < 11 || !startsWith(line, "CONECT")) {
        return null;
    }
    const serial = parseI32Strict(line[6..11]) ?? return null;
    let bonds: [4]i32 = [0, 0, 0, 0];
    let count: u8 = 0;

    let slot: usize = 0;
    while (slot < 4) {
        const col_start = 11 + slot * 5;
        const col_end = col_start + 5;
        if (col_start >= line.len) {
            break;
        }
        const field = trimAscii(sliceCols(line, col_start, col_end));
        if (field.len > 0) {
            const b = parseI32Strict(field) ?? return null;
            bonds[usize(count)] = b;
            count += 1;
        }
        slot += 1;
    }

    return PdbConect{
        serial: serial,
        bonds: bonds,
        bond_count: count,
    };
}

/// 零分配逐行迭代器：遍历 PDB 中的全部 ATOM 与 HETATM 记录，追踪 MODEL 编号，遇 END 停止
pub struct PdbAtomIter {
    data: []const u8,
    pos: usize,
    model_num: u32,
    stopped: bool,

    fn init(data: []const u8): PdbAtomIter {
        return PdbAtomIter{
            data: data,
            pos: 0,
            model_num: 0,
            stopped: false,
        };
    }

    fn next(*_): PdbAtom? {
        if (_.stopped) {
            return null;
        }
        while (line := nextLine(_.data, *_.pos)) {
            if (isRecordTag(line, "END")) {
                _.stopped = true;
                return null;
            }
            if (isRecordTag(line, "MODEL")) {
                // PDB v3.3 MODEL: Cols 11-14 (0-based 10..14), 兼容 Cols 7..len
                const num_slice = trimAscii(sliceCols(line, 6, 16));
                if (m := parseU32Strict(num_slice)) {
                    _.model_num = m;
                } else {
                    _.model_num += 1;
                }
                continue;
            }
            if (startsWith(line, "ATOM  ") || startsWith(line, "HETATM")) {
                if (atom := parsePdbAtomLine(line)) {
                    let res = atom;
                    res.model_num = _.model_num;
                    return res;
                }
            }
        }
        _.stopped = true;
        return null;
    }
}

/// 零分配逐行迭代器：遍历 PDB 中的全部 CONECT 记录，遇 END 停止
pub struct PdbConectIter {
    data: []const u8,
    pos: usize,
    stopped: bool,

    fn init(data: []const u8): PdbConectIter {
        return PdbConectIter{
            data: data,
            pos: 0,
            stopped: false,
        };
    }

    fn next(*_): PdbConect? {
        if (_.stopped) {
            return null;
        }
        while (line := nextLine(_.data, *_.pos)) {
            if (isRecordTag(line, "END")) {
                _.stopped = true;
                return null;
            }
            if (startsWith(line, "CONECT")) {
                if (c := parsePdbConectLine(line)) {
                    return c;
                }
            }
        }
        _.stopped = true;
        return null;
    }
}

/// 单遍扫描统计 PDB 结构摘要：原子数、异源原子数、模型数、链数、3D 包围盒与质心
pub fn summarizePdb(data: []const u8): PdbStats {
    let stats = PdbStats{
        atom_count: 0,
        hetatm_count: 0,
        model_count: 0,
        chain_count: 0,
        min_x: 0.0,
        max_x: 0.0,
        min_y: 0.0,
        max_y: 0.0,
        min_z: 0.0,
        max_z: 0.0,
        centroid_x: 0.0,
        centroid_y: 0.0,
        centroid_z: 0.0,
    };

    let seen_chains: [256]bool = [false; 256];
    let explicit_models: u32 = 0;
    let total_atoms: u32 = 0;
    let sum_x: f64 = 0.0;
    let sum_y: f64 = 0.0;
    let sum_z: f64 = 0.0;

    let pos: usize = 0;
    while (line := nextLine(data, *pos)) {
        if (isRecordTag(line, "END")) {
            break;
        }
        if (isRecordTag(line, "MODEL")) {
            explicit_models += 1;
            continue;
        }
        if (startsWith(line, "ATOM  ") || startsWith(line, "HETATM")) {
            if (atom := parsePdbAtomLine(line)) {
                if (atom.is_hetatm) {
                    stats.hetatm_count += 1;
                } else {
                    stats.atom_count += 1;
                }

                if (atom.chain_id != ' ' && atom.chain_id != 0) {
                    const idx = usize(atom.chain_id);
                    if (!seen_chains[idx]) {
                        seen_chains[idx] = true;
                        stats.chain_count += 1;
                    }
                }

                if (total_atoms == 0) {
                    stats.min_x = atom.x;
                    stats.max_x = atom.x;
                    stats.min_y = atom.y;
                    stats.max_y = atom.y;
                    stats.min_z = atom.z;
                    stats.max_z = atom.z;
                } else {
                    if (atom.x < stats.min_x) stats.min_x = atom.x;
                    if (atom.x > stats.max_x) stats.max_x = atom.x;
                    if (atom.y < stats.min_y) stats.min_y = atom.y;
                    if (atom.y > stats.max_y) stats.max_y = atom.y;
                    if (atom.z < stats.min_z) stats.min_z = atom.z;
                    if (atom.z > stats.max_z) stats.max_z = atom.z;
                }

                sum_x += atom.x;
                sum_y += atom.y;
                sum_z += atom.z;
                total_atoms += 1;
            }
        }
    }

    if (explicit_models > 0) {
        stats.model_count = explicit_models;
    } else if (total_atoms > 0) {
        stats.model_count = 1;
    }

    if (total_atoms > 0) {
        const inv = 1.0 / f64(total_atoms);
        stats.centroid_x = sum_x * inv;
        stats.centroid_y = sum_y * inv;
        stats.centroid_z = sum_z * inv;
    }

    return stats;
}
