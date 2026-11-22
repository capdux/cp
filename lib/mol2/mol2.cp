// Origin: Pure-CP Stdlib (Zero-Alloc Tripos SYBYL .mol2 3D Molecule Parser) | Created: 2026-11-22

pub const MOL2_RTI_MOLECULE: []const u8 = "@<TRIPOS>MOLECULE";
pub const MOL2_RTI_ATOM: []const u8 = "@<TRIPOS>ATOM";
pub const MOL2_RTI_BOND: []const u8 = "@<TRIPOS>BOND";
pub const MOL2_RTI_SUBSTRUCTURE: []const u8 = "@<TRIPOS>SUBSTRUCTURE";

const MOL2_RTI_PREFIX: []const u8 = "@<TRIPOS>";

/// Tripos MOL2 @<TRIPOS>MOLECULE 头部元数据及区段定位（零拷贝切片）
pub struct Mol2Header {
    name: []const u8,
    atom_count: u32,
    bond_count: u32,
    subst_count: u32,
    feat_count: u32,
    set_count: u32,
    mol_type: []const u8,
    charge_type: []const u8,
    atom_section: []const u8,
    bond_section: []const u8,
}

/// Tripos MOL2 @<TRIPOS>ATOM 单条原子记录（零拷贝切片）
pub struct Mol2Atom {
    atom_id: u32,
    atom_name: []const u8,
    x: f64,
    y: f64,
    z: f64,
    sybyl_type: []const u8,
    subst_id: u32,
    subst_name: []const u8,
    charge: f64,
    has_charge: bool,
}

/// Tripos MOL2 @<TRIPOS>BOND 单条化学键记录（零拷贝切片）
pub struct Mol2Bond {
    bond_id: u32,
    origin_atom: u32,
    target_atom: u32,
    bond_type: []const u8,

    /// 判断是否为芳香键 ("ar" / "AR")
    fn isAromatic(): bool {
        if (_.bond_type.len != 2) {
            return false;
        }
        const c0 = _.bond_type[0];
        const c1 = _.bond_type[1];
        return (c0 == 'a' || c0 == 'A') && (c1 == 'r' || c1 == 'R');
    }
}

/// Tripos MOL2 多分子单遍统计汇总
pub struct Mol2Stats {
    molecule_count: u32,
    total_atoms: u32,
    total_bonds: u32,
    aromatic_bonds: u32,
    total_charge: f64,
}

// ---------------------------------------------------------------------------
// 内部零分配切片、空白分词与数值解析辅助函数
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

/// 从 data[*pos..] 零拷贝读取下一行（剥离 \r\n）
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

/// 从一行中按空白分隔切出下一个 token
fn nextToken(line: []const u8, pos: *usize): ([]const u8)? {
    let i = pos.*;
    while (i < line.len && isSpaceOrTab(line[i])) {
        i += 1;
    }
    if (i >= line.len) {
        pos.* = i;
        return null;
    }
    const start = i;
    while (i < line.len && !isSpaceOrTab(line[i])) {
        i += 1;
    }
    pos.* = i;
    return line[start..i];
}

fn isRtiLine(line: []const u8, rti: []const u8): bool {
    const trimmed = trimAscii(line);
    if (!startsWith(trimmed, rti)) {
        return false;
    }
    if (trimmed.len == rti.len) {
        return true;
    }
    return isSpaceOrTab(trimmed[rti.len]);
}

fn isAnyRtiLine(line: []const u8): bool {
    return startsWith(trimAscii(line), MOL2_RTI_PREFIX);
}

fn isCommentLine(line: []const u8): bool {
    const trimmed = trimAscii(line);
    return trimmed.len > 0 && trimmed[0] == '#';
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

/// 严格零分配解析 f64（支持正负号、小数与科学计数法）
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

/// 在 mol_slice 中定位指定 @<TRIPOS>xxx 区段的正文切片（不含头部行，截至下一 @<TRIPOS>）
fn findSection(mol_slice: []const u8, rti: []const u8): []const u8 {
    let pos: usize = 0;
    let found = false;
    let sec_start: usize = 0;

    while (line := nextLine(mol_slice, *pos)) {
        if (isRtiLine(line, rti)) {
            found = true;
            sec_start = pos;
            break;
        }
    }
    if (!found) {
        return mol_slice[0..0];
    }

    let sec_end = mol_slice.len;
    while (pos < mol_slice.len) {
        const line_start = pos;
        const line = nextLine(mol_slice, *pos) ?? break;
        if (isAnyRtiLine(line)) {
            sec_end = line_start;
            break;
        }
    }
    return mol_slice[sec_start..sec_end];
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 快速检测数据是否包含 @<TRIPOS>MOLECULE 区段头
pub fn isMol2(data: []const u8): bool {
    let pos: usize = 0;
    while (line := nextLine(data, *pos)) {
        if (isRtiLine(line, MOL2_RTI_MOLECULE)) {
            return true;
        }
    }
    return false;
}

/// 读取 @<TRIPOS>MOLECULE 内下一条有效内容行（跳过 # 注释；遇下一 @<TRIPOS> 返回 null）
fn nextMolHeaderLine(mol_slice: []const u8, pos: *usize): ([]const u8)? {
    while (line := nextLine(mol_slice, pos)) {
        if (isAnyRtiLine(line)) {
            return null;
        }
        if (isCommentLine(line)) {
            continue;
        }
        return line;
    }
    return null;
}

/// 解析 @<TRIPOS>MOLECULE 块元数据，并零拷贝定位 @<TRIPOS>ATOM 与 @<TRIPOS>BOND 区段切片
pub fn parseMol2Header(mol_slice: []const u8): Mol2Header? {
    let pos: usize = 0;
    let found_mol = false;
    while (line := nextLine(mol_slice, *pos)) {
        if (isRtiLine(line, MOL2_RTI_MOLECULE)) {
            found_mol = true;
            break;
        }
    }
    if (!found_mol) {
        return null;
    }

    // 行 1: 分子名称
    const name_raw = nextMolHeaderLine(mol_slice, *pos) ?? return null;
    const name = trimAscii(name_raw);

    // 行 2: num_atoms [num_bonds [num_subst [num_feat [num_sets]]]]
    const counts_line = nextMolHeaderLine(mol_slice, *pos) ?? return null;
    let tpos: usize = 0;
    const tok_atoms = nextToken(counts_line, *tpos) ?? return null;
    const atom_count = parseU32Strict(tok_atoms) ?? return null;

    let bond_count: u32 = 0;
    if (tok_bonds := nextToken(counts_line, *tpos)) {
        bond_count = parseU32Strict(tok_bonds) ?? return null;
    }
    let subst_count: u32 = 0;
    if (tok_subst := nextToken(counts_line, *tpos)) {
        subst_count = parseU32Strict(tok_subst) ?? return null;
    }
    let feat_count: u32 = 0;
    if (tok_feat := nextToken(counts_line, *tpos)) {
        feat_count = parseU32Strict(tok_feat) ?? return null;
    }
    let set_count: u32 = 0;
    if (tok_set := nextToken(counts_line, *tpos)) {
        set_count = parseU32Strict(tok_set) ?? return null;
    }

    // 行 3: mol_type (如 SMALL / PROTEIN)
    const mol_type_raw = nextMolHeaderLine(mol_slice, *pos) ?? return null;
    const mol_type = trimAscii(mol_type_raw);

    // 行 4: charge_type (如 GASTEIGER / NO_CHARGES)
    const charge_type_raw = nextMolHeaderLine(mol_slice, *pos) ?? return null;
    const charge_type = trimAscii(charge_type_raw);

    const atom_section = findSection(mol_slice, MOL2_RTI_ATOM);
    const bond_section = findSection(mol_slice, MOL2_RTI_BOND);

    return Mol2Header{
        name: name,
        atom_count: atom_count,
        bond_count: bond_count,
        subst_count: subst_count,
        feat_count: feat_count,
        set_count: set_count,
        mol_type: mol_type,
        charge_type: charge_type,
        atom_section: atom_section,
        bond_section: bond_section,
    };
}

/// 解析单条 @<TRIPOS>ATOM 记录行
pub fn parseMol2AtomLine(line: []const u8): Mol2Atom? {
    let tpos: usize = 0;
    const t_id = nextToken(line, *tpos) ?? return null;
    const atom_name = nextToken(line, *tpos) ?? return null;
    const t_x = nextToken(line, *tpos) ?? return null;
    const t_y = nextToken(line, *tpos) ?? return null;
    const t_z = nextToken(line, *tpos) ?? return null;
    const sybyl_type = nextToken(line, *tpos) ?? return null;

    const atom_id = parseU32Strict(t_id) ?? return null;
    const x = parseF64Strict(t_x) ?? return null;
    const y = parseF64Strict(t_y) ?? return null;
    const z = parseF64Strict(t_z) ?? return null;

    // 可选列：subst_id, subst_name, charge
    let subst_id: u32 = 0;
    let subst_name = line[0..0];
    let charge: f64 = 0.0;
    let has_charge = false;

    if (t_sid := nextToken(line, *tpos)) {
        subst_id = parseU32Strict(t_sid) ?? return null;
        if (t_sname := nextToken(line, *tpos)) {
            subst_name = t_sname;
            if (t_chg := nextToken(line, *tpos)) {
                charge = parseF64Strict(t_chg) ?? return null;
                has_charge = true;
            }
        }
    }

    return Mol2Atom{
        atom_id: atom_id,
        atom_name: atom_name,
        x: x,
        y: y,
        z: z,
        sybyl_type: sybyl_type,
        subst_id: subst_id,
        subst_name: subst_name,
        charge: charge,
        has_charge: has_charge,
    };
}

/// 解析单条 @<TRIPOS>BOND 记录行
pub fn parseMol2BondLine(line: []const u8): Mol2Bond? {
    let tpos: usize = 0;
    const t_id = nextToken(line, *tpos) ?? return null;
    const t_orig = nextToken(line, *tpos) ?? return null;
    const t_targ = nextToken(line, *tpos) ?? return null;
    const bond_type = nextToken(line, *tpos) ?? return null;

    const bond_id = parseU32Strict(t_id) ?? return null;
    const origin_atom = parseU32Strict(t_orig) ?? return null;
    const target_atom = parseU32Strict(t_targ) ?? return null;

    if (origin_atom == 0 || target_atom == 0) {
        return null;
    }

    return Mol2Bond{
        bond_id: bond_id,
        origin_atom: origin_atom,
        target_atom: target_atom,
        bond_type: bond_type,
    };
}

/// 零分配原子迭代器（兼容传入 atom_section 或完整 mol_slice，跳过空行与 # 注释，遇 @<TRIPOS> 停止）
pub struct Mol2AtomIter {
    data: []const u8,
    pos: usize,
    done: bool,
    malformed: bool,

    fn init(slice: []const u8): Mol2AtomIter {
        // 若传入的是完整 mol_slice，自动定位至 @<TRIPOS>ATOM 区段
        let scan: usize = 0;
        let has_any_rti = false;
        while (line := nextLine(slice, *scan)) {
            if (isRtiLine(line, MOL2_RTI_ATOM)) {
                return Mol2AtomIter{
                    data: slice,
                    pos: scan,
                    done: false,
                    malformed: false,
                };
            }
            if (isAnyRtiLine(line)) {
                has_any_rti = true;
            }
        }
        if (has_any_rti) {
            // 有其他 @<TRIPOS> 区段但缺少 @<TRIPOS>ATOM
            return Mol2AtomIter{
                data: slice,
                pos: slice.len,
                done: true,
                malformed: false,
            };
        }
        return Mol2AtomIter{
            data: slice,
            pos: 0,
            done: false,
            malformed: false,
        };
    }

    fn next(*_): Mol2Atom? {
        if (_.done) {
            return null;
        }
        while (line := nextLine(_.data, *_.pos)) {
            if (isAnyRtiLine(line)) {
                _.done = true;
                return null;
            }
            const trimmed = trimAscii(line);
            if (trimmed.len == 0 || trimmed[0] == '#') {
                continue;
            }
            if (atom := parseMol2AtomLine(trimmed)) {
                return atom;
            }
            _.malformed = true;
        }
        _.done = true;
        return null;
    }
}

/// 零分配化学键迭代器（兼容传入 bond_section 或完整 mol_slice，跳过空行与 # 注释，遇 @<TRIPOS> 停止）
pub struct Mol2BondIter {
    data: []const u8,
    pos: usize,
    done: bool,
    malformed: bool,

    fn init(slice: []const u8): Mol2BondIter {
        let scan: usize = 0;
        let has_any_rti = false;
        while (line := nextLine(slice, *scan)) {
            if (isRtiLine(line, MOL2_RTI_BOND)) {
                return Mol2BondIter{
                    data: slice,
                    pos: scan,
                    done: false,
                    malformed: false,
                };
            }
            if (isAnyRtiLine(line)) {
                has_any_rti = true;
            }
        }
        if (has_any_rti) {
            return Mol2BondIter{
                data: slice,
                pos: slice.len,
                done: true,
                malformed: false,
            };
        }
        return Mol2BondIter{
            data: slice,
            pos: 0,
            done: false,
            malformed: false,
        };
    }

    fn next(*_): Mol2Bond? {
        if (_.done) {
            return null;
        }
        while (line := nextLine(_.data, *_.pos)) {
            if (isAnyRtiLine(line)) {
                _.done = true;
                return null;
            }
            const trimmed = trimAscii(line);
            if (trimmed.len == 0 || trimmed[0] == '#') {
                continue;
            }
            if (bond := parseMol2BondLine(trimmed)) {
                return bond;
            }
            _.malformed = true;
        }
        _.done = true;
        return null;
    }
}

/// 零拷贝多分子拼接 .mol2 记录流迭代器（按每次出现 @<TRIPOS>MOLECULE 切分）
pub struct Mol2RecordIter {
    data: []const u8,
    pos: usize,

    fn init(data: []const u8): Mol2RecordIter {
        return Mol2RecordIter{
            data: data,
            pos: 0,
        };
    }

    fn next(*_): ([]const u8)? {
        // 定位下一个 @<TRIPOS>MOLECULE 起点
        let rec_start: usize = 0;
        let found = false;
        while (_.pos < _.data.len) {
            const line_start = _.pos;
            const line = nextLine(_.data, *_.pos) ?? break;
            if (isRtiLine(line, MOL2_RTI_MOLECULE)) {
                rec_start = line_start;
                found = true;
                break;
            }
        }
        if (!found) {
            return null;
        }

        // 向前扫描至下一条 @<TRIPOS>MOLECULE 或 EOF
        let rec_end = _.data.len;
        while (_.pos < _.data.len) {
            const line_start = _.pos;
            const line = nextLine(_.data, *_.pos) ?? break;
            if (isRtiLine(line, MOL2_RTI_MOLECULE)) {
                rec_end = line_start;
                _.pos = line_start;
                break;
            }
        }

        return _.data[rec_start..rec_end];
    }
}

/// 单遍统计多分子 MOL2 流中的分子数、原子数、键数、芳香键数与净电荷
pub fn summarizeMol2(data: []const u8): Mol2Stats {
    let stats = Mol2Stats{
        molecule_count: 0,
        total_atoms: 0,
        total_bonds: 0,
        aromatic_bonds: 0,
        total_charge: 0.0,
    };

    let rit = Mol2RecordIter.init(data);
    while (rec := rit.next()) {
        if (hdr := parseMol2Header(rec)) {
            stats.molecule_count += 1;

            let ait = Mol2AtomIter.init(hdr.atom_section);
            while (atom := ait.next()) {
                stats.total_atoms += 1;
                if (atom.has_charge) {
                    stats.total_charge += atom.charge;
                }
            }

            let bit = Mol2BondIter.init(hdr.bond_section);
            while (bond := bit.next()) {
                stats.total_bonds += 1;
                if (bond.isAromatic()) {
                    stats.aromatic_bonds += 1;
                }
            }
        }
    }

    return stats;
}
