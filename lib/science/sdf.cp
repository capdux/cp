// Origin: Pure-CP Stdlib (Zero-Alloc MDL Molfile V2000 & SDF Structure-Data File Parser) | Created: 2026-11-22

/// MDL Molfile V2000 头部与 Counts Line 元数据（零拷贝切片）
pub struct SdfMolHeader {
    title: []const u8,
    program_line: []const u8,
    comment: []const u8,
    atom_count: u32,
    bond_count: u32,
    is_chiral: bool,
    version: []const u8,
    atoms_offset: usize,
    bonds_offset: usize,
}

/// MDL V2000 Atom Block 单条原子记录（零拷贝切片）
pub struct SdfAtom {
    x: f64,
    y: f64,
    z: f64,
    symbol: []const u8,
    mass_diff: i8,
    charge_code: u8,

    /// 将 MDL V2000 电荷码 (0..7) 映射为化学形式电荷：
    /// 1->+3, 2->+2, 3->+1, 4->0 (doublet radical), 5->-1, 6->-2, 7->-3, 0->0
    fn formalCharge(): i8 {
        if (_.charge_code == 1) return 3;
        if (_.charge_code == 2) return 2;
        if (_.charge_code == 3) return 1;
        if (_.charge_code == 4) return 0;
        if (_.charge_code == 5) return -1;
        if (_.charge_code == 6) return -2;
        if (_.charge_code == 7) return -3;
        return 0;
    }
}

/// MDL V2000 Bond Block 单条化学键记录
pub struct SdfBond {
    atom1: u32,
    atom2: u32,
    bond_type: u8,
    stereo: u8,
}

/// SDF 关联数据项（> <PROP_NAME> ...，零拷贝切片）
pub struct SdfProperty {
    name: []const u8,
    value: []const u8,
}

/// SDF 多分子单遍统计汇总
pub struct SdfStats {
    molecule_count: u32,
    total_atoms: u32,
    total_bonds: u32,
    chiral_count: u32,
}

// ---------------------------------------------------------------------------
// 内部零分配切片与定宽数值解析辅助函数
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

/// 判断是否为 SDF 记录分隔行 "$$$$"
fn isRecordSeparator(line: []const u8): bool {
    return eqlBytes(trimAscii(line), "$$$$");
}

/// 从 data[*pos..] 切出下一行（不含 \r\n），并推进 pos
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

/// 严格零分配解析 u32
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

/// 严格零分配解析 i8
fn parseI8Strict(raw: []const u8): i8? {
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
    let acc: i32 = 0;
    while (i < s.len) {
        const c = s[i];
        if (!isDigit(c)) {
            return null;
        }
        acc = acc * 10 + i32(c - '0');
        if (neg) {
            if (acc > 128) return null;
        } else {
            if (acc > 127) return null;
        }
        i += 1;
    }
    if (neg) {
        return (-acc) as i8;
    }
    return acc as i8;
}

/// 严格零分配解析 f64（支持符号、小数、科学计数法）
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

    if (i != s.len) {
        return null;
    }
    return if (neg) -val else val;
}

/// 解析单条 MDL V2000 Atom Block 行
fn parseAtomLine(line: []const u8): SdfAtom? {
    // 至少需要包含 x(0..10), y(10..20), z(20..30), space(30), symbol(31..34 至少 1 字节)
    if (line.len < 32) {
        return null;
    }
    const x = parseF64Strict(line[0..10]) ?? return null;
    const y = parseF64Strict(line[10..20]) ?? return null;
    const z = parseF64Strict(line[20..30]) ?? return null;
    const symbol = trimAscii(sliceCols(line, 31, 34));
    if (symbol.len == 0) {
        return null;
    }

    let mass_diff: i8 = 0;
    const md_slice = trimAscii(sliceCols(line, 34, 36));
    if (md_slice.len > 0) {
        mass_diff = parseI8Strict(md_slice) ?? return null;
    }

    let charge_code: u8 = 0;
    const chg_slice = trimAscii(sliceCols(line, 36, 39));
    if (chg_slice.len > 0) {
        const c_val = parseU32Strict(chg_slice) ?? return null;
        if (c_val > 255) {
            return null;
        }
        charge_code = c_val as u8;
    }

    return SdfAtom{
        x: x,
        y: y,
        z: z,
        symbol: symbol,
        mass_diff: mass_diff,
        charge_code: charge_code,
    };
}

/// 解析单条 MDL V2000 Bond Block 行，并校验 1 <= atom1, atom2 <= atom_count
fn parseBondLine(line: []const u8, atom_count: u32): SdfBond? {
    if (line.len < 9) {
        return null;
    }
    const atom1 = parseU32Strict(line[0..3]) ?? return null;
    const atom2 = parseU32Strict(line[3..6]) ?? return null;
    const btype_u32 = parseU32Strict(line[6..9]) ?? return null;

    if (atom1 < 1 || atom1 > atom_count || atom2 < 1 || atom2 > atom_count) {
        return null;
    }
    if (btype_u32 < 1 || btype_u32 > 8) {
        return null;
    }

    let stereo: u8 = 0;
    const st_slice = trimAscii(sliceCols(line, 9, 12));
    if (st_slice.len > 0) {
        const st_u32 = parseU32Strict(st_slice) ?? return null;
        if (st_u32 > 255) {
            return null;
        }
        stereo = st_u32 as u8;
    }

    return SdfBond{
        atom1: atom1,
        atom2: atom2,
        bond_type: btype_u32 as u8,
        stereo: stereo,
    };
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 解析 MDL Molfile V2000 3 行头块与第 4 行 Counts Line
pub fn parseMolHeader(mol_slice: []const u8): SdfMolHeader? {
    let pos: usize = 0;
    const title = nextLine(mol_slice, *pos) ?? return null;
    const program_line = nextLine(mol_slice, *pos) ?? return null;
    const comment = nextLine(mol_slice, *pos) ?? return null;
    const counts_line = nextLine(mol_slice, *pos) ?? return null;

    // V2000 Counts Line 至少需要 39 列以包含 34..39 的 "V2000" 版本标识
    if (counts_line.len < 39) {
        return null;
    }

    const version = trimAscii(sliceCols(counts_line, 34, 39));
    if (!eqlBytes(version, "V2000")) {
        return null;
    }

    const atom_count = parseU32Strict(counts_line[0..3]) ?? return null;
    const bond_count = parseU32Strict(counts_line[3..6]) ?? return null;

    let is_chiral = false;
    const chiral_slice = trimAscii(sliceCols(counts_line, 12, 15));
    if (chiral_slice.len > 0) {
        const chiral_val = parseU32Strict(chiral_slice) ?? return null;
        is_chiral = chiral_val == 1;
    }

    const atoms_offset = pos;

    // 计算 bonds_offset（跳过 atom_count 行；若提前到达 EOF 则停在 mol_slice.len，由迭代器判截断）
    let scan_pos = atoms_offset;
    let skipped: u32 = 0;
    while (skipped < atom_count) {
        * = nextLine(mol_slice, *scan_pos) ?? break;
        skipped += 1;
    }
    const bonds_offset = scan_pos;

    return SdfMolHeader{
        title: title,
        program_line: program_line,
        comment: comment,
        atom_count: atom_count,
        bond_count: bond_count,
        is_chiral: is_chiral,
        version: version,
        atoms_offset: atoms_offset,
        bonds_offset: bonds_offset,
    };
}

/// 快速检测数据是否以合法的 MDL V2000 Molfile/SDF 分子记录开头
pub fn isSdf(data: []const u8): bool {
    let it = SdfRecordIter.init(data);
    const first = it.next() ?? return false;
    return parseMolHeader(first) != null;
}

/// 精确迭代 Molfile Atom Block 中的 atom_count 条原子记录
pub struct SdfAtomIter {
    data: []const u8,
    pos: usize,
    remaining: u32,
    malformed: bool,

    fn init(mol_slice: []const u8, header: SdfMolHeader): SdfAtomIter {
        return SdfAtomIter{
            data: mol_slice,
            pos: header.atoms_offset,
            remaining: header.atom_count,
            malformed: false,
        };
    }

    fn next(*_): SdfAtom? {
        if (_.remaining == 0 || _.malformed) {
            return null;
        }
        if (line := nextLine(_.data, *_.pos)) {
            if (atom := parseAtomLine(line)) {
                _.remaining -= 1;
                return atom;
            }
        }
        _.malformed = true;
        _.remaining = 0;
        return null;
    }
}

/// 精确迭代 Molfile Bond Block 中的 bond_count 条化学键记录（校验 1 <= atom1, atom2 <= atom_count）
pub struct SdfBondIter {
    data: []const u8,
    pos: usize,
    remaining: u32,
    atom_count: u32,
    malformed: bool,

    fn init(mol_slice: []const u8, header: SdfMolHeader): SdfBondIter {
        return SdfBondIter{
            data: mol_slice,
            pos: header.bonds_offset,
            remaining: header.bond_count,
            atom_count: header.atom_count,
            malformed: false,
        };
    }

    fn next(*_): SdfBond? {
        if (_.remaining == 0 || _.malformed) {
            return null;
        }
        if (line := nextLine(_.data, *_.pos)) {
            if (bond := parseBondLine(line, _.atom_count)) {
                _.remaining -= 1;
                return bond;
            }
        }
        _.malformed = true;
        _.remaining = 0;
        return null;
    }
}

/// 零分配迭代 M  END 之后至 $$$$ 之前的全部 > <PROP_NAME> 关联数据项
pub struct SdfPropIter {
    data: []const u8,
    pos: usize,
    done: bool,

    static fn init(record_slice: []const u8): SdfPropIter {
        let pos: usize = 0;
        let found_end = false;
        while (line := nextLine(record_slice, *pos)) {
            if (isRecordSeparator(line)) {
                break;
            }
            if (startsWith(line, "M  END")) {
                found_end = true;
                break;
            }
        }
        return SdfPropIter{
            data: record_slice,
            pos: if (found_end) pos else record_slice.len,
            done: !found_end,
        };
    }

    fn next(*_): SdfProperty? {
        if (_.done) {
            return null;
        }

        // 查找下一个 "> <PROP_NAME>" 头部行
        let prop_name = _.data[0..0];
        let found_header = false;
        while (line := nextLine(_.data, *_.pos)) {
            if (isRecordSeparator(line)) {
                _.done = true;
                return null;
            }
            if (line.len > 0 && line[0] == '>') {
                // 提取 '<' 与 '>' 之间的属性名
                let lt_idx: usize = 1;
                while (lt_idx < line.len && line[lt_idx] != '<') {
                    lt_idx += 1;
                }
                if (lt_idx < line.len && line[lt_idx] == '<') {
                    const name_start = lt_idx + 1;
                    let gt_idx = name_start;
                    while (gt_idx < line.len && line[gt_idx] != '>') {
                        gt_idx += 1;
                    }
                    if (gt_idx < line.len && line[gt_idx] == '>') {
                        prop_name = line[name_start..gt_idx];
                        found_header = true;
                        break;
                    }
                }
            }
        }

        if (!found_header) {
            _.done = true;
            return null;
        }

        // 收集多行值直至空行、$$$$、下一个 > 头部或 EOF，并自动去首尾空行
        let val_start: usize = 0;
        let val_end: usize = 0;
        let has_content = false;

        while (true) {
            const line_start = _.pos;
            const line = nextLine(_.data, *_.pos) ?? break;
            if (isRecordSeparator(line)) {
                _.pos = line_start;
                _.done = true;
                break;
            }
            if (line.len > 0 && line[0] == '>') {
                // 容错：缺少空行直接紧跟下一属性
                _.pos = line_start;
                break;
            }
            if (trimAscii(line).len == 0) {
                if (has_content) {
                    // 属性值块遇空行结束
                    break;
                }
                // 跳过前置空行
                continue;
            }
            if (!has_content) {
                val_start = line_start;
                has_content = true;
            }
            val_end = line_start + line.len;
        }

        const value = if (has_content) _.data[val_start..val_end] else _.data[0..0];
        return SdfProperty{
            name: prop_name,
            value: value,
        };
    }
}

/// 在单条 SDF 分子记录中按名称零分配查找关联属性值
pub fn findSdfProp(record_slice: []const u8, name: []const u8): ([]const u8)? {
    let it = SdfPropIter.init(record_slice);
    while (prop := it.next()) {
        if (eqlBytes(prop.name, name)) {
            return prop.value;
        }
    }
    return null;
}

/// 按 $$$$ 分隔符零拷贝迭代多分子 .sdf 文件中的每条分子记录切片
pub struct SdfRecordIter {
    data: []const u8,
    pos: usize,

    static fn init(data: []const u8): SdfRecordIter {
        return SdfRecordIter{
            data: data,
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        while (_.pos < _.data.len) {
            const rec_start = _.pos;
            let rec_end = _.pos;
            let has_non_empty = false;

            while (true) {
                const line_start = _.pos;
                if (line := nextLine(_.data, *_.pos)) {
                    if (isRecordSeparator(line)) {
                        rec_end = line_start;
                        break;
                    }
                    if (trimAscii(line).len > 0) {
                        has_non_empty = true;
                    }
                    rec_end = _.pos;
                } else {
                    rec_end = _.data.len;
                    break;
                }
            }

            if (has_non_empty && rec_end > rec_start) {
                return _.data[rec_start..rec_end];
            }
        }
        return null;
    }
}

/// 单遍统计多分子 SDF 流中的分子数、总原子数、总化学键数与手性分子数
pub fn summarizeSdf(data: []const u8): SdfStats {
    let stats = SdfStats{
        molecule_count: 0,
        total_atoms: 0,
        total_bonds: 0,
        chiral_count: 0,
    };

    let it = SdfRecordIter.init(data);
    while (rec := it.next()) {
        if (hdr := parseMolHeader(rec)) {
            stats.molecule_count += 1;
            stats.total_atoms += hdr.atom_count;
            stats.total_bonds += hdr.bond_count;
            if (hdr.is_chiral) {
                stats.chiral_count += 1;
            }
        }
    }
    return stats;
}
