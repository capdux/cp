// Origin: Pure-CP Stdlib (Zero-Alloc IUPAC InChI v1.06 & InChIKey Parser) | Created: 2026-11-22

pub const INCHI_PREFIX: []const u8 = "InChI=";
pub const INCHI_KEY_LEN: usize = 27;

/// IUPAC InChI 分层类型
pub enum InchiLayerKind {
    formula,
    connections,
    hydrogens,
    charge,
    protons,
    double_bond_stereo,
    tetrahedral_stereo,
    stereo_parity,
    stereo_type,
    isotopic,
    fixed_h,
    reconnected,
    unknown,
}

/// 单个 InChI 层（零拷贝切片）
pub struct InchiLayer {
    prefix: u8,
    kind: InchiLayerKind,
    content: []const u8,
}

/// 完整解析后的 InChI 结构（零拷贝切片）
pub struct InchiParsed {
    version: []const u8,
    is_standard: bool,
    formula: []const u8,
    connections: []const u8,
    hydrogens: []const u8,
    charge: []const u8,
    protons: []const u8,
    double_bond_stereo: []const u8,
    tetrahedral_stereo: []const u8,
    stereo_parity: []const u8,
    stereo_type: []const u8,
    isotopic: []const u8,
    fixed_h: []const u8,
    reconnected: []const u8,
    layer_count: u32,
}

/// Hill 分子式元素与化学计量数（零拷贝切片）
pub struct FormulaElement {
    symbol: []const u8,
    count: u32,
}

/// 27 字符 InChIKey 拆解结构（AAAAAAAAAAAAAA-BBBBBBBBFV-P）
pub struct InchiKey {
    connectivity_block: []const u8,
    stereo_block: []const u8,
    flag_char: u8,
    version_char: u8,
    protonation_char: u8,
    is_standard: bool,

    /// 计算质子化差值：'N' -> 0, 'O' -> +1, 'M' -> -1（即 protonation_char - 'N'）
    fn protonationDelta(): i8 {
        return (i32(_.protonation_char) - i32('N')) as i8;
    }
}

// ---------------------------------------------------------------------------
// 内部零分配字符校验与辅助函数
// ---------------------------------------------------------------------------

fn isUpper(c: u8): bool {
    return c >= 'A' && c <= 'Z';
}

fn isLower(c: u8): bool {
    return c >= 'a' && c <= 'z';
}

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn startsWith(s: []const u8, prefix: []const u8): bool {
    if (s.len < prefix.len) {
        return false;
    }
    let i: usize = 0;
    while (i < prefix.len) {
        if (s[i] != prefix[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn prefixToKind(p: u8): InchiLayerKind {
    if (p == 'c') return InchiLayerKind.connections;
    if (p == 'h') return InchiLayerKind.hydrogens;
    if (p == 'q') return InchiLayerKind.charge;
    if (p == 'p') return InchiLayerKind.protons;
    if (p == 'b') return InchiLayerKind.double_bond_stereo;
    if (p == 't') return InchiLayerKind.tetrahedral_stereo;
    if (p == 'm') return InchiLayerKind.stereo_parity;
    if (p == 's') return InchiLayerKind.stereo_type;
    if (p == 'i') return InchiLayerKind.isotopic;
    if (p == 'f') return InchiLayerKind.fixed_h;
    if (p == 'r') return InchiLayerKind.reconnected;
    return InchiLayerKind.unknown;
}

/// 校验 InChI 前缀与版本号，成功时返回首个 '/' 的字节偏移量
fn validateInchiPrefixAndVersion(s: []const u8): usize? {
    if (!startsWith(s, INCHI_PREFIX)) {
        return null;
    }
    // 整串不能含空白字符
    let scan: usize = 0;
    while (scan < s.len) {
        if (isSpace(s[scan])) {
            return null;
        }
        scan += 1;
    }

    const ver_start = INCHI_PREFIX.len;
    let slash_pos = ver_start;
    while (slash_pos < s.len && s[slash_pos] != '/') {
        slash_pos += 1;
    }
    if (slash_pos == ver_start || slash_pos >= s.len) {
        return null;
    }

    // 校验版本号（如 "1S" 或 "1"：至少 1 位数字，可选末尾 'S'）
    const ver = s[ver_start..slash_pos];
    let vi: usize = 0;
    let digits: u32 = 0;
    while (vi < ver.len && isDigit(ver[vi])) {
        digits += 1;
        vi += 1;
    }
    if (digits == 0) {
        return null;
    }
    if (vi < ver.len) {
        if (vi + 1 != ver.len || ver[vi] != 'S') {
            return null;
        }
    }

    // 校验分子式主层非空，且首字符为大写字母或数字（多组分系数如 2H2O）
    const formula_start = slash_pos + 1;
    if (formula_start >= s.len || s[formula_start] == '/') {
        return null;
    }
    const f0 = s[formula_start];
    if (!isUpper(f0) && !isDigit(f0)) {
        return null;
    }

    return slash_pos;
}

// ---------------------------------------------------------------------------
// 公开 API：InChI 解析与逐层迭代
// ---------------------------------------------------------------------------

/// 快速检测字符串是否为合法的 IUPAC InChI 标识符
pub fn isInchi(s: []const u8): bool {
    return validateInchiPrefixAndVersion(s) != null;
}

/// 零分配逐层迭代 InChI 字符串中的全部 /... 层（首层为 formula）
pub struct InchiLayerIter {
    data: []const u8,
    pos: usize,
    is_first: bool,
    done: bool,

    static fn init(s: []const u8): InchiLayerIter {
        if (slash_pos := validateInchiPrefixAndVersion(s)) {
            return InchiLayerIter{
                data: s,
                pos: slash_pos + 1,
                is_first: true,
                done: false,
            };
        }
        return InchiLayerIter{
            data: s,
            pos: s.len,
            is_first: false,
            done: true,
        };
    }

    fn next(*_): InchiLayer? {
        if (_.done || _.pos >= _.data.len) {
            _.done = true;
            return null;
        }

        const seg_start = _.pos;
        let seg_end = seg_start;
        while (seg_end < _.data.len && _.data[seg_end] != '/') {
            seg_end += 1;
        }
        _.pos = if (seg_end < _.data.len) (seg_end + 1) else _.data.len;

        const seg = _.data[seg_start..seg_end];
        if (seg.len == 0) {
            _.done = true;
            return null;
        }

        if (_.is_first) {
            _.is_first = false;
            return InchiLayer{
                prefix: 0,
                kind: InchiLayerKind.formula,
                content: seg,
            };
        }

        const p = seg[0];
        return InchiLayer{
            prefix: p,
            kind: prefixToKind(p),
            content: seg[1..seg.len],
        };
    }
}

/// 解析 IUPAC InChI 字符串并零拷贝提取全部核心分层切片
pub fn parseInchi(s: []const u8): InchiParsed? {
    const slash_pos = validateInchiPrefixAndVersion(s) ?? return null;
    const version = s[INCHI_PREFIX.len..slash_pos];
    const is_standard = version.len > 0 && version[version.len - 1] == 'S';
    const empty = s[0..0];

    let parsed = InchiParsed{
        version: version,
        is_standard: is_standard,
        formula: empty,
        connections: empty,
        hydrogens: empty,
        charge: empty,
        protons: empty,
        double_bond_stereo: empty,
        tetrahedral_stereo: empty,
        stereo_parity: empty,
        stereo_type: empty,
        isotopic: empty,
        fixed_h: empty,
        reconnected: empty,
        layer_count: 0,
    };

    let it = InchiLayerIter.init(s);
    while (layer := it.next()) {
        parsed.layer_count += 1;
        if (layer.kind == InchiLayerKind.formula) {
            parsed.formula = layer.content;
        } else if (layer.kind == InchiLayerKind.connections) {
            if (parsed.connections.len == 0) parsed.connections = layer.content;
        } else if (layer.kind == InchiLayerKind.hydrogens) {
            if (parsed.hydrogens.len == 0) parsed.hydrogens = layer.content;
        } else if (layer.kind == InchiLayerKind.charge) {
            if (parsed.charge.len == 0) parsed.charge = layer.content;
        } else if (layer.kind == InchiLayerKind.protons) {
            if (parsed.protons.len == 0) parsed.protons = layer.content;
        } else if (layer.kind == InchiLayerKind.double_bond_stereo) {
            if (parsed.double_bond_stereo.len == 0) parsed.double_bond_stereo = layer.content;
        } else if (layer.kind == InchiLayerKind.tetrahedral_stereo) {
            if (parsed.tetrahedral_stereo.len == 0) parsed.tetrahedral_stereo = layer.content;
        } else if (layer.kind == InchiLayerKind.stereo_parity) {
            if (parsed.stereo_parity.len == 0) parsed.stereo_parity = layer.content;
        } else if (layer.kind == InchiLayerKind.stereo_type) {
            if (parsed.stereo_type.len == 0) parsed.stereo_type = layer.content;
        } else if (layer.kind == InchiLayerKind.isotopic) {
            if (parsed.isotopic.len == 0) parsed.isotopic = layer.content;
        } else if (layer.kind == InchiLayerKind.fixed_h) {
            if (parsed.fixed_h.len == 0) parsed.fixed_h = layer.content;
        } else if (layer.kind == InchiLayerKind.reconnected) {
            if (parsed.reconnected.len == 0) parsed.reconnected = layer.content;
        }
    }

    if (parsed.formula.len == 0) {
        return null;
    }
    return parsed;
}

// ---------------------------------------------------------------------------
// 公开 API：Hill 记法分子式零分配迭代器
// ---------------------------------------------------------------------------

/// Hill 记法分子式零分配迭代器（支持 "C9H8O4"、"NaCl" 及多组分 "2H2O.NaCl"）
pub struct FormulaIter {
    data: []const u8,
    pos: usize,
    comp_mult: u32,
    at_comp_start: bool,
    malformed: bool,

    static fn init(formula: []const u8): FormulaIter {
        return FormulaIter{
            data: formula,
            pos: 0,
            comp_mult: 1,
            at_comp_start: true,
            malformed: false,
        };
    }

    fn next(*_): FormulaElement? {
        if (_.malformed || _.pos >= _.data.len) {
            return null;
        }

        // 处理多组分分隔符 '.'
        if (_.data[_.pos] == '.') {
            _.pos += 1;
            _.comp_mult = 1;
            _.at_comp_start = true;
            if (_.pos >= _.data.len) {
                _.malformed = true;
                return null;
            }
        }

        // 处理组分前导化学计量系数（如 "2H2O" 中的 2）
        if (_.at_comp_start) {
            _.at_comp_start = false;
            if (_.pos < _.data.len && isDigit(_.data[_.pos])) {
                let mult: u64 = 0;
                while (_.pos < _.data.len && isDigit(_.data[_.pos])) {
                    mult = mult * 10 + u64(_.data[_.pos] - '0');
                    if (mult > 4294967295) {
                        _.malformed = true;
                        return null;
                    }
                    _.pos += 1;
                }
                if (mult == 0 || _.pos >= _.data.len) {
                    _.malformed = true;
                    return null;
                }
                _.comp_mult = mult as u32;
            } else {
                _.comp_mult = 1;
            }
        }

        if (_.pos >= _.data.len || !isUpper(_.data[_.pos])) {
            _.malformed = true;
            return null;
        }

        // 元素符号：1 个大写字母 + 0..2 个小写字母
        const sym_start = _.pos;
        _.pos += 1;
        while (_.pos < _.data.len && isLower(_.data[_.pos])) {
            _.pos += 1;
        }
        const symbol = _.data[sym_start.._.pos];

        // 可选元素计量数（缺省 1）
        let elem_count: u64 = 1;
        if (_.pos < _.data.len && isDigit(_.data[_.pos])) {
            elem_count = 0;
            while (_.pos < _.data.len && isDigit(_.data[_.pos])) {
                elem_count = elem_count * 10 + u64(_.data[_.pos] - '0');
                if (elem_count > 4294967295) {
                    _.malformed = true;
                    return null;
                }
                _.pos += 1;
            }
            if (elem_count == 0) {
                _.malformed = true;
                return null;
            }
        }

        const total = elem_count * u64(_.comp_mult);
        if (total > 4294967295) {
            _.malformed = true;
            return null;
        }

        return FormulaElement{
            symbol: symbol,
            count: total as u32,
        };
    }
}

// ---------------------------------------------------------------------------
// 公开 API：27 字符 InChIKey 严格校验与拆解
// ---------------------------------------------------------------------------

/// 严格校验并拆解 27 字符 InChIKey（格式：AAAAAAAAAAAAAA-BBBBBBBBFV-P）
pub fn parseInchiKey(key: []const u8): InchiKey? {
    if (key.len != INCHI_KEY_LEN) {
        return null;
    }
    if (key[14] != '-' || key[25] != '-') {
        return null;
    }

    // 校验前 14 位连接性块全为大写 ASCII 字母
    let i: usize = 0;
    while (i < 14) {
        if (!isUpper(key[i])) {
            return null;
        }
        i += 1;
    }

    // 校验 15..23 立体/同位素块全为大写 ASCII 字母
    let j: usize = 15;
    while (j < 23) {
        if (!isUpper(key[j])) {
            return null;
        }
        j += 1;
    }

    // 校验第 24 位 (idx 23) 标准/非标准标志：必须为 'S' 或 'N'
    const flag_char = key[23];
    if (flag_char != 'S' && flag_char != 'N') {
        return null;
    }

    // 校验第 25 位 (idx 24) 版本号：大写字母（v1 为 'A'）
    const version_char = key[24];
    if (!isUpper(version_char)) {
        return null;
    }

    // 校验第 27 位 (idx 26) 质子化字符：大写字母 'A'..'Z'
    const protonation_char = key[26];
    if (!isUpper(protonation_char)) {
        return null;
    }

    return InchiKey{
        connectivity_block: key[0..14],
        stereo_block: key[15..23],
        flag_char: flag_char,
        version_char: version_char,
        protonation_char: protonation_char,
        is_standard: flag_char == 'S',
    };
}

/// 严格校验字符串是否为合法的 27 字符 InChIKey
pub fn isValidInchiKey(key: []const u8): bool {
    return parseInchiKey(key) != null;
}
