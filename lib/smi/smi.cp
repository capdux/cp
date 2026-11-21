// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Daylight SMILES / OpenSMILES Molecular Parser)

/// SMILES 词法单元类型
pub enum SmiTokenKind {
    atom,
    bracket_atom,
    bond,
    branch_open,
    branch_close,
    ring_closure,
    dot,
}

/// 括号原子 `[isotope? symbol chiral? hcount? charge? (:class)?]` 零拷贝解析结果
pub struct SmiBracketAtom {
    isotope: u16,
    symbol: []const u8,
    is_aromatic: bool,
    chiral_count: u8,
    h_count: u8,
    charge: i8,
    atom_class: u16,
}

/// SMILES 单个词法单元（零拷贝切片视图）
pub struct SmiToken {
    kind: SmiTokenKind,
    text: []const u8,
    ring_num: u8,
    is_aromatic: bool,
}

/// SMILES 单遍分子图校验与统计汇总
pub struct SmiStats {
    atom_count: u32,
    aromatic_atom_count: u32,
    bond_count: u32,
    ring_count: u32,
    branch_depth_max: u32,
    fragment_count: u32,
    chiral_centers: u32,
    net_charge: i32,
    valid: bool,
}

/// 零分配 SMILES 词法扫描器
pub struct SmiLexer {
    src: []const u8,
    pos: usize,
    had_error: bool,

    static fn init(smiles: []const u8): SmiLexer {
        return SmiLexer {
            src: smiles,
            pos: 0,
            had_error: false,
        };
    }

    fn next(*_): SmiToken? {
        if (_.pos >= _.src.len) {
            return null;
        }
        const start = _.pos;
        const c = _.src[start];

        if (c == '(') {
            _.pos = start + 1;
            return SmiToken { kind: .branch_open, text: _.src[start..start + 1], ring_num: 0, is_aromatic: false };
        }
        if (c == ')') {
            _.pos = start + 1;
            return SmiToken { kind: .branch_close, text: _.src[start..start + 1], ring_num: 0, is_aromatic: false };
        }
        if (c == '.') {
            _.pos = start + 1;
            return SmiToken { kind: .dot, text: _.src[start..start + 1], ring_num: 0, is_aromatic: false };
        }
        if (isBondChar(c)) {
            _.pos = start + 1;
            return SmiToken { kind: .bond, text: _.src[start..start + 1], ring_num: 0, is_aromatic: false };
        }
        if (isDigit(c)) {
            _.pos = start + 1;
            return SmiToken {
                kind: .ring_closure,
                text: _.src[start..start + 1],
                ring_num: c - '0',
                is_aromatic: false,
            };
        }
        if (c == '%') {
            if (start + 2 >= _.src.len || !isDigit(_.src[start + 1]) || !isDigit(_.src[start + 2])) {
                _.had_error = true;
                _.pos = _.src.len;
                return null;
            }
            const rnum: u8 = (_.src[start + 1] - '0') * 10 + (_.src[start + 2] - '0');
            _.pos = start + 3;
            return SmiToken {
                kind: .ring_closure,
                text: _.src[start..start + 3],
                ring_num: rnum,
                is_aromatic: false,
            };
        }
        if (c == '[') {
            let i: usize = start + 1;
            while (i < _.src.len && _.src[i] != ']') {
                if (_.src[i] == '[') {
                    _.had_error = true;
                    _.pos = _.src.len;
                    return null;
                }
                i += 1;
            }
            if (i >= _.src.len || i == start + 1) {
                _.had_error = true;
                _.pos = _.src.len;
                return null;
            }
            const inner = _.src[start + 1..i];
            const parsed_opt = parseBracketAtom(inner);
            if (parsed_opt == null) {
                _.had_error = true;
                _.pos = _.src.len;
                return null;
            }
            const parsed = parsed_opt ?? return null;
            _.pos = i + 1;
            return SmiToken {
                kind: .bracket_atom,
                text: _.src[start..i + 1],
                ring_num: 0,
                is_aromatic: parsed.is_aromatic,
            };
        }

        // 有机子集双字母卤素：Cl / Br
        if (c == 'C' && start + 1 < _.src.len && _.src[start + 1] == 'l') {
            _.pos = start + 2;
            return SmiToken { kind: .atom, text: _.src[start..start + 2], ring_num: 0, is_aromatic: false };
        }
        if (c == 'B' && start + 1 < _.src.len && _.src[start + 1] == 'r') {
            _.pos = start + 2;
            return SmiToken { kind: .atom, text: _.src[start..start + 2], ring_num: 0, is_aromatic: false };
        }

        // 有机子集单字母非芳香原子
        if (isOrganicAliphatic(c) || c == '*') {
            _.pos = start + 1;
            return SmiToken { kind: .atom, text: _.src[start..start + 1], ring_num: 0, is_aromatic: false };
        }

        // 有机子集芳香原子：b, c, n, o, p, s
        if (isOrganicAromatic(c)) {
            _.pos = start + 1;
            return SmiToken { kind: .atom, text: _.src[start..start + 1], ring_num: 0, is_aromatic: true };
        }

        _.had_error = true;
        _.pos = _.src.len;
        return null;
    }
}

// ---------------------------------------------------------------------------
// 内部字符分类辅助函数
// ---------------------------------------------------------------------------

fn isDigit(c: u8): bool {
    return c >= '0' && c <= '9';
}

fn isUpper(c: u8): bool {
    return c >= 'A' && c <= 'Z';
}

fn isLower(c: u8): bool {
    return c >= 'a' && c <= 'z';
}

fn isBondChar(c: u8): bool {
    return c == '-' || c == '=' || c == '#' || c == '$' || c == ':' || c == '/' || c == '\\';
}

fn isOrganicAliphatic(c: u8): bool {
    return c == 'B' || c == 'C' || c == 'N' || c == 'O' || c == 'P' || c == 'S' || c == 'F' || c == 'I';
}

fn isOrganicAromatic(c: u8): bool {
    return c == 'b' || c == 'c' || c == 'n' || c == 'o' || c == 'p' || c == 's';
}

// ---------------------------------------------------------------------------
// 公开 API
// ---------------------------------------------------------------------------

/// 解析括号原子内容（支持直接传入内部切片 `"13C@@H"` 或带方括号切片 `"[13C@@H]"`）
pub fn parseBracketAtom(raw: []const u8): SmiBracketAtom? {
    if (raw.len == 0) {
        return null;
    }
    let s = raw;
    if (s[0] == '[') {
        if (s.len < 3 || s[s.len - 1] != ']') {
            return null;
        }
        s = s[1..s.len - 1];
    }
    if (s.len == 0) {
        return null;
    }

    let i: usize = 0;

    // 1. 可选同位素质量数
    let isotope: u32 = 0;
    while (i < s.len && isDigit(s[i])) {
        isotope = isotope * 10 + ((s[i] - '0') as u32);
        if (isotope > 65535) {
            return null;
        }
        i += 1;
    }
    if (i >= s.len) {
        return null;
    }

    // 2. 必选元素符号（大写+可选小写，或芳香 b/c/n/o/p/s/se/as，或通配 '*'）
    const sym_start = i;
    let is_aromatic: bool = false;
    if (s[i] == '*') {
        i += 1;
    } else if (isUpper(s[i])) {
        i += 1;
        if (i < s.len && isLower(s[i])) {
            i += 1;
        }
    } else if (isLower(s[i])) {
        is_aromatic = true;
        if (s[i] == 's' && i + 1 < s.len && s[i + 1] == 'e') {
            i += 2;
        } else if (s[i] == 'a' && i + 1 < s.len && s[i + 1] == 's') {
            i += 2;
        } else if (isOrganicAromatic(s[i])) {
            i += 1;
        } else {
            return null;
        }
    } else {
        return null;
    }
    const symbol = s[sym_start..i];

    // 3. 可选手性标记（@ 或 @@）
    let chiral_count: u8 = 0;
    if (i < s.len && s[i] == '@') {
        chiral_count = 1;
        i += 1;
        if (i < s.len && s[i] == '@') {
            chiral_count = 2;
            i += 1;
        }
        if (i < s.len && s[i] == '@') {
            return null;
        }
    }

    // 4. 可选显式氢计数（H 或 H0..H9）
    let h_count: u8 = 0;
    if (i < s.len && s[i] == 'H') {
        i += 1;
        if (i < s.len && isDigit(s[i])) {
            h_count = s[i] - '0';
            i += 1;
        } else {
            h_count = 1;
        }
    }

    // 5. 可选形式电荷（+/++/++2/-/-/--/-3 等）
    let charge: i8 = 0;
    if (i < s.len && (s[i] == '+' || s[i] == '-')) {
        const sign_ch = s[i];
        i += 1;
        let mag: i32 = 1;
        if (i < s.len && isDigit(s[i])) {
            mag = 0;
            while (i < s.len && isDigit(s[i])) {
                mag = mag * 10 + ((s[i] - '0') as i32);
                if (mag > 127) {
                    return null;
                }
                i += 1;
            }
        } else {
            while (i < s.len && s[i] == sign_ch) {
                mag += 1;
                if (mag > 127) {
                    return null;
                }
                i += 1;
            }
        }
        const signed_chg: i32 = if (sign_ch == '-') -mag else mag;
        if (signed_chg < -128 || signed_chg > 127) {
            return null;
        }
        charge = signed_chg as i8;
    }

    // 6. 可选原子映射类（:N）
    let atom_class: u32 = 0;
    if (i < s.len && s[i] == ':') {
        i += 1;
        if (i >= s.len || !isDigit(s[i])) {
            return null;
        }
        while (i < s.len && isDigit(s[i])) {
            atom_class = atom_class * 10 + ((s[i] - '0') as u32);
            if (atom_class > 65535) {
                return null;
            }
            i += 1;
        }
    }

    if (i != s.len) {
        return null;
    }

    return SmiBracketAtom {
        isotope: isotope as u16,
        symbol: symbol,
        is_aromatic: is_aromatic,
        chiral_count: chiral_count,
        h_count: h_count,
        charge: charge,
        atom_class: atom_class as u16,
    };
}

/// 单遍零分配分析 SMILES 字符串：校验括号平衡、环闭合配平、悬空键与空分支，并统计分子指标
pub fn analyzeSmiles(smiles: []const u8): SmiStats {
    let stats = SmiStats {
        atom_count: 0,
        aromatic_atom_count: 0,
        bond_count: 0,
        ring_count: 0,
        branch_depth_max: 0,
        fragment_count: 0,
        chiral_centers: 0,
        net_charge: 0,
        valid: false,
    };
    if (smiles.len == 0) {
        return stats;
    }

    let ring_open: [100]bool = [false; 100];
    let branch_has_atom: [64]bool = [false; 64];
    let branch_depth: u32 = 0;
    let has_prev_atom: bool = false;
    let pending_bond: bool = false;
    let expect_fragment_atom: bool = true;
    let ok: bool = true;

    stats.fragment_count = 1;

    let lex = SmiLexer.init(smiles);
    while (true) {
        const tok_opt = lex.next();
        if (tok_opt == null) {
            break;
        }
        const tok = tok_opt ?? break;

        match (tok.kind) {
            .atom => {
                if (has_prev_atom) {
                    stats.bond_count += 1;
                }
                pending_bond = false;
                has_prev_atom = true;
                expect_fragment_atom = false;
                if (branch_depth > 0) {
                    branch_has_atom[(branch_depth - 1) as usize] = true;
                }
                stats.atom_count += 1;
                if (tok.is_aromatic) {
                    stats.aromatic_atom_count += 1;
                }
            },
            .bracket_atom => {
                const ba_opt = parseBracketAtom(tok.text);
                if (ba_opt == null) {
                    ok = false;
                    break;
                }
                const ba = ba_opt ?? break;
                if (has_prev_atom) {
                    stats.bond_count += 1;
                }
                pending_bond = false;
                has_prev_atom = true;
                expect_fragment_atom = false;
                if (branch_depth > 0) {
                    branch_has_atom[(branch_depth - 1) as usize] = true;
                }
                stats.atom_count += 1;
                if (ba.is_aromatic) {
                    stats.aromatic_atom_count += 1;
                }
                if (ba.chiral_count > 0) {
                    stats.chiral_centers += 1;
                }
                stats.net_charge += ba.charge as i32;
            },
            .bond => {
                if (!has_prev_atom || pending_bond) {
                    ok = false;
                    break;
                }
                pending_bond = true;
            },
            .branch_open => {
                if (!has_prev_atom || pending_bond || branch_depth >= 64) {
                    ok = false;
                    break;
                }
                branch_has_atom[branch_depth as usize] = false;
                branch_depth += 1;
                if (branch_depth > stats.branch_depth_max) {
                    stats.branch_depth_max = branch_depth;
                }
            },
            .branch_close => {
                if (branch_depth == 0 || pending_bond || !branch_has_atom[(branch_depth - 1) as usize]) {
                    ok = false;
                    break;
                }
                branch_depth -= 1;
                has_prev_atom = true;
            },
            .ring_closure => {
                if (!has_prev_atom || tok.ring_num >= 100) {
                    ok = false;
                    break;
                }
                const r_idx: usize = tok.ring_num as usize;
                if (ring_open[r_idx]) {
                    ring_open[r_idx] = false;
                    stats.ring_count += 1;
                    stats.bond_count += 1;
                } else {
                    ring_open[r_idx] = true;
                }
                pending_bond = false;
            },
            .dot => {
                if (branch_depth > 0 || !has_prev_atom || pending_bond) {
                    ok = false;
                    break;
                }
                stats.fragment_count += 1;
                has_prev_atom = false;
                expect_fragment_atom = true;
            },
        }
    }

    if (lex.had_error || !ok || stats.atom_count == 0 || branch_depth != 0 || pending_bond || expect_fragment_atom) {
        stats.valid = false;
        return stats;
    }

    let r: usize = 0;
    while (r < 100) {
        if (ring_open[r]) {
            stats.valid = false;
            return stats;
        }
        r += 1;
    }

    stats.valid = true;
    return stats;
}

/// 快速判断字符串是否为合法 SMILES 分子表示
pub fn isValidSmiles(smiles: []const u8): bool {
    return analyzeSmiles(smiles).valid;
}
