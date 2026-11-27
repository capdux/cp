// packages/@cap/spdx-expression-parse/src/index.cp
// Zero-allocation SPDX license expression AST parser and evaluator for CP.
// Clean-room MIT pure CP implementation reusing `@cap/spdx`.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning and comptime 256-entry character classification LUT.

import { lookupLicense } from "../../spdx/src/index.cp";
import { strEq, startsWith } from "cap:ascii";

type SpdxpVec16 = @Vector(16, u8);

const SPDXP_FLAG_WS: u8 = 1;
const SPDXP_FLAG_ID: u8 = 2;

fn spdxpBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[9] = SPDXP_FLAG_WS;
    lut[10] = SPDXP_FLAG_WS;
    lut[11] = SPDXP_FLAG_WS;
    lut[12] = SPDXP_FLAG_WS;
    lut[13] = SPDXP_FLAG_WS;
    lut[32] = SPDXP_FLAG_WS;

    let c: usize = 48;
    while (c <= 57) {
        lut[c] = SPDXP_FLAG_ID;
        c += 1;
    }
    c = 65;
    while (c <= 90) {
        lut[c] = SPDXP_FLAG_ID;
        c += 1;
    }
    c = 97;
    while (c <= 122) {
        lut[c] = SPDXP_FLAG_ID;
        c += 1;
    }
    lut[45] = SPDXP_FLAG_ID; // '-'
    lut[46] = SPDXP_FLAG_ID; // '.'
    return lut;
}

const SPDXP_CHAR_LUT: [256]u8 = spdxpBuildCharLut();

pub const SPDX_NODE_LICENSE: u8 = 1;
pub const SPDX_NODE_AND: u8 = 2;
pub const SPDX_NODE_OR: u8 = 3;

const SPDXP_MAX_NODES: usize = 32;
const SPDXP_MAX_DEPTH: usize = 32;

/// Single AST node inside a fixed-capacity `SpdxAst` pool.
pub struct SpdxAstNode {
    pub kind: u8,
    pub license: string,
    pub exception: string,
    pub plus: bool,
    pub left: i32,
    pub right: i32,
}

/// Zero-allocation SPDX expression AST backed by a 32-node fixed pool.
pub struct SpdxAst {
    pub nodes: [32]SpdxAstNode,
    pub root: i32,
    pub node_count: usize,
    pub valid: bool,
}

struct SpdxpState {
    src: string,
    pos: usize,
    depth: usize,
    failed: bool,
    ast: SpdxAst,
}

fn spdxpEmptyNode(): SpdxAstNode {
    return SpdxAstNode {
        kind: 0,
        license: "",
        exception: "",
        plus: false,
        left: -1,
        right: -1,
    };
}

fn spdxpEmptyAst(): SpdxAst {
    let pool: [32]SpdxAstNode = undefined;
    let i: usize = 0;
    while (i < SPDXP_MAX_NODES) {
        pool[i] = spdxpEmptyNode();
        i += 1;
    }
    return SpdxAst {
        nodes: pool,
        root: -1,
        node_count: 0,
        valid: false,
    };
}

fn spdxpIsSpace(ch: u8): bool {
    return (SPDXP_CHAR_LUT[ch as usize] & SPDXP_FLAG_WS) != 0;
}

fn spdxpIsIdChar(ch: u8): bool {
    return (SPDXP_CHAR_LUT[ch as usize] & SPDXP_FLAG_ID) != 0;
}

fn spdxpIsKeyword(tok: string): bool {
    return strEq(tok, "AND") || strEq(tok, "OR") || strEq(tok, "WITH");
}

fn spdxpIsAllIdChars(s: string): bool {
    if (s.len == 0) {
        return false;
    }
    let i: usize = 0;
    const v_zero: SpdxpVec16 = @splat(0 as u8);
    const v_one: SpdxpVec16 = @splat(1 as u8);
    const v_lo_a: SpdxpVec16 = @splat(97 as u8);
    const v_lo_z: SpdxpVec16 = @splat(122 as u8);
    const v_up_a: SpdxpVec16 = @splat(65 as u8);
    const v_up_z: SpdxpVec16 = @splat(90 as u8);
    const v_d0: SpdxpVec16 = @splat(48 as u8);
    const v_d9: SpdxpVec16 = @splat(57 as u8);
    const v_hy: SpdxpVec16 = @splat(45 as u8);
    const v_dot: SpdxpVec16 = @splat(46 as u8);

    while (i + 16 <= s.len) {
        const v: SpdxpVec16 = s[i .. i + 16][0..16].*;
        const ok = (@select(u8, v >= v_lo_a, v_one, v_zero) & @select(u8, v <= v_lo_z, v_one, v_zero))
            | (@select(u8, v >= v_up_a, v_one, v_zero) & @select(u8, v <= v_up_z, v_one, v_zero))
            | (@select(u8, v >= v_d0, v_one, v_zero) & @select(u8, v <= v_d9, v_one, v_zero))
            | @select(u8, v == v_hy, v_one, v_zero)
            | @select(u8, v == v_dot, v_one, v_zero);
        if (!@reduce(.And, ok != v_zero)) {
            return false;
        }
        i += 16;
    }
    while (i < s.len) {
        if (!spdxpIsIdChar(s[i])) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn spdxpIsValidLicenseRef(id: string): bool {
    if (startsWith(id, "LicenseRef-")) {
        return spdxpIsAllIdChars(id[11..id.len]);
    }
    if (startsWith(id, "DocumentRef-")) {
        let colon: usize = 12;
        while (colon < id.len && id[colon] != (':' as u8)) {
            colon += 1;
        }
        if (colon == 12 || colon >= id.len) {
            return false;
        }
        let doc_id: string = id[12..colon];
        let rest: string = id[colon + 1 .. id.len];
        if (!spdxpIsAllIdChars(doc_id)) {
            return false;
        }
        if (!startsWith(rest, "LicenseRef-")) {
            return false;
        }
        return spdxpIsAllIdChars(rest[11..rest.len]);
    }
    return false;
}

fn spdxpIsValidLicenseId(id: string): bool {
    if (id.len == 0 || spdxpIsKeyword(id)) {
        return false;
    }
    if (spdxpIsValidLicenseRef(id)) {
        return true;
    }
    return lookupLicense(id) != null;
}

fn spdxpIsValidExceptionId(exc: string): bool {
    if (exc.len == 0 || spdxpIsKeyword(exc)) {
        return false;
    }
    return spdxpIsAllIdChars(exc);
}

fn spdxpSkipSpaces(st: *SpdxpState) {
    let p = st.*.pos;
    const src = st.*.src;
    const v_zero: SpdxpVec16 = @splat(0 as u8);
    const v_one: SpdxpVec16 = @splat(1 as u8);
    const v_sp: SpdxpVec16 = @splat(32 as u8);
    const v_t0: SpdxpVec16 = @splat(9 as u8);
    const v_t1: SpdxpVec16 = @splat(13 as u8);
    while (p + 16 <= src.len) {
        const v: SpdxpVec16 = src[p .. p + 16][0..16].*;
        const ws = @select(u8, v == v_sp, v_one, v_zero)
            | (@select(u8, v >= v_t0, v_one, v_zero) & @select(u8, v <= v_t1, v_one, v_zero));
        if (@reduce(.And, ws != v_zero)) {
            p += 16;
            continue;
        }
        break;
    }
    while (p < src.len && spdxpIsSpace(src[p])) {
        p += 1;
    }
    st.*.pos = p;
}

fn spdxpScanIdRun(src: string, start: usize, allow_colon: bool): usize {
    let p = start;
    const v_zero: SpdxpVec16 = @splat(0 as u8);
    const v_one: SpdxpVec16 = @splat(1 as u8);
    const v_lo_a: SpdxpVec16 = @splat(97 as u8);
    const v_lo_z: SpdxpVec16 = @splat(122 as u8);
    const v_up_a: SpdxpVec16 = @splat(65 as u8);
    const v_up_z: SpdxpVec16 = @splat(90 as u8);
    const v_d0: SpdxpVec16 = @splat(48 as u8);
    const v_d9: SpdxpVec16 = @splat(57 as u8);
    const v_hy: SpdxpVec16 = @splat(45 as u8);
    const v_dot: SpdxpVec16 = @splat(46 as u8);
    const v_col: SpdxpVec16 = @splat(58 as u8);
    while (p + 16 <= src.len) {
        const v: SpdxpVec16 = src[p .. p + 16][0..16].*;
        let ok = (@select(u8, v >= v_lo_a, v_one, v_zero) & @select(u8, v <= v_lo_z, v_one, v_zero))
            | (@select(u8, v >= v_up_a, v_one, v_zero) & @select(u8, v <= v_up_z, v_one, v_zero))
            | (@select(u8, v >= v_d0, v_one, v_zero) & @select(u8, v <= v_d9, v_one, v_zero))
            | @select(u8, v == v_hy, v_one, v_zero)
            | @select(u8, v == v_dot, v_one, v_zero);
        if (allow_colon) {
            ok = ok | @select(u8, v == v_col, v_one, v_zero);
        }
        if (@reduce(.And, ok != v_zero)) {
            p += 16;
            continue;
        }
        break;
    }
    while (p < src.len && (spdxpIsIdChar(src[p]) || (allow_colon && src[p] == (':' as u8)))) {
        p += 1;
    }
    return p;
}

fn spdxpAllocNode(st: *SpdxpState, node: SpdxAstNode): i32 {
    if (st.*.ast.node_count >= SPDXP_MAX_NODES) {
        st.*.failed = true;
        return -1;
    }
    let idx: usize = st.*.ast.node_count;
    st.*.ast.nodes[idx] = node;
    st.*.ast.node_count = idx + 1;
    return idx as i32;
}

fn spdxpConsumeKeyword(st: *SpdxpState, kw: string): bool {
    spdxpSkipSpaces(st);
    let pos: usize = st.*.pos;
    if (!startsWith(st.*.src[pos..st.*.src.len], kw)) {
        return false;
    }
    let after: usize = pos + kw.len;
    if (after < st.*.src.len) {
        let next_ch: u8 = st.*.src[after];
        if (spdxpIsIdChar(next_ch) || next_ch == ('+' as u8) || next_ch == (':' as u8)) {
            return false;
        }
    }
    st.*.pos = after;
    return true;
}

fn spdxpParseSimple(st: *SpdxpState): i32 {
    spdxpSkipSpaces(st);
    let start: usize = st.*.pos;
    if (start >= st.*.src.len) {
        st.*.failed = true;
        return -1;
    }

    let i: usize = spdxpScanIdRun(st.*.src, start, true);
    if (i == start) {
        st.*.failed = true;
        return -1;
    }

    let lic_id: string = st.*.src[start..i];
    st.*.pos = i;

    let has_plus: bool = false;
    if (st.*.pos < st.*.src.len && st.*.src[st.*.pos] == ('+' as u8)) {
        if (spdxpIsValidLicenseRef(lic_id)) {
            st.*.failed = true;
            return -1;
        }
        has_plus = true;
        st.*.pos = st.*.pos + 1;
        // Reject trailing `+` or id chars directly attached after `+`.
        if (st.*.pos < st.*.src.len && (st.*.src[st.*.pos] == ('+' as u8) || spdxpIsIdChar(st.*.src[st.*.pos]))) {
            st.*.failed = true;
            return -1;
        }
    }

    if (!spdxpIsValidLicenseId(lic_id)) {
        st.*.failed = true;
        return -1;
    }

    let exc_id: string = "";
    if (spdxpConsumeKeyword(st, "WITH")) {
        spdxpSkipSpaces(st);
        let exc_start: usize = st.*.pos;
        let j: usize = spdxpScanIdRun(st.*.src, exc_start, false);
        if (j == exc_start) {
            st.*.failed = true;
            return -1;
        }
        exc_id = st.*.src[exc_start..j];
        st.*.pos = j;
        if (!spdxpIsValidExceptionId(exc_id)) {
            st.*.failed = true;
            return -1;
        }
    }

    return spdxpAllocNode(st, SpdxAstNode {
        kind: SPDX_NODE_LICENSE,
        license: lic_id,
        exception: exc_id,
        plus: has_plus,
        left: -1,
        right: -1,
    });
}

fn spdxpParsePrimary(st: *SpdxpState): i32 {
    if (st.*.failed) {
        return -1;
    }
    spdxpSkipSpaces(st);
    if (st.*.pos >= st.*.src.len) {
        st.*.failed = true;
        return -1;
    }

    if (st.*.src[st.*.pos] == ('(' as u8)) {
        if (st.*.depth >= SPDXP_MAX_DEPTH) {
            st.*.failed = true;
            return -1;
        }
        st.*.pos = st.*.pos + 1;
        st.*.depth = st.*.depth + 1;
        let inner: i32 = spdxpParseOr(st);
        if (st.*.failed || inner < 0) {
            st.*.failed = true;
            return -1;
        }
        spdxpSkipSpaces(st);
        if (st.*.pos >= st.*.src.len || st.*.src[st.*.pos] != (')' as u8)) {
            st.*.failed = true;
            return -1;
        }
        st.*.pos = st.*.pos + 1;
        st.*.depth = st.*.depth - 1;
        return inner;
    }

    return spdxpParseSimple(st);
}

fn spdxpParseAnd(st: *SpdxpState): i32 {
    let left: i32 = spdxpParsePrimary(st);
    if (st.*.failed || left < 0) {
        return -1;
    }

    while (!st.*.failed && spdxpConsumeKeyword(st, "AND")) {
        let right: i32 = spdxpParsePrimary(st);
        if (st.*.failed || right < 0) {
            st.*.failed = true;
            return -1;
        }
        left = spdxpAllocNode(st, SpdxAstNode {
            kind: SPDX_NODE_AND,
            license: "",
            exception: "",
            plus: false,
            left: left,
            right: right,
        });
    }
    return left;
}

fn spdxpParseOr(st: *SpdxpState): i32 {
    let left: i32 = spdxpParseAnd(st);
    if (st.*.failed || left < 0) {
        return -1;
    }

    while (!st.*.failed && spdxpConsumeKeyword(st, "OR")) {
        let right: i32 = spdxpParseAnd(st);
        if (st.*.failed || right < 0) {
            st.*.failed = true;
            return -1;
        }
        left = spdxpAllocNode(st, SpdxAstNode {
            kind: SPDX_NODE_OR,
            license: "",
            exception: "",
            plus: false,
            left: left,
            right: right,
        });
    }
    return left;
}

/// Parse an SPDX 2.3 license expression into a fixed-pool zero-allocation `SpdxAst`.
pub fn parseSpdxExpression(expr: string): SpdxAst {
    let st: SpdxpState = SpdxpState {
        src: expr,
        pos: 0,
        depth: 0,
        failed: false,
        ast: spdxpEmptyAst(),
    };

    spdxpSkipSpaces(&st);
    if (st.pos >= expr.len) {
        return st.ast;
    }

    let root_idx: i32 = spdxpParseOr(&st);
    spdxpSkipSpaces(&st);

    if (!st.failed && root_idx >= 0 && st.pos == expr.len) {
        st.ast.root = root_idx;
        st.ast.valid = true;
    } else {
        st.ast.root = -1;
        st.ast.valid = false;
    }
    return st.ast;
}

/// Returns true if `expr` is a syntactically and semantically valid SPDX expression.
pub fn isValidSpdxExpression(expr: string): bool {
    let ast: SpdxAst = parseSpdxExpression(expr);
    return ast.valid;
}

fn spdxpMatchLicenseNode(node: SpdxAstNode, allowed: string): bool {
    if (node.exception.len > 0) {
        // Match `<license>[+] WITH <exception>`.
        let pos: usize = 0;
        if (!startsWith(allowed, node.license)) {
            return false;
        }
        pos = node.license.len;
        if (node.plus && pos < allowed.len && allowed[pos] == ('+' as u8)) {
            pos += 1;
        }
        if (!startsWith(allowed[pos..allowed.len], " WITH ")) {
            return false;
        }
        pos += 6;
        return strEq(allowed[pos..allowed.len], node.exception);
    }

    if (strEq(node.license, allowed)) {
        return true;
    }
    if (node.plus && allowed.len == node.license.len + 1 && allowed[allowed.len - 1] == ('+' as u8)) {
        return startsWith(allowed, node.license);
    }
    return false;
}

fn spdxpEvalAst(ast: SpdxAst, idx: i32, allowed: string, depth: usize): bool {
    if (idx < 0 || depth >= SPDXP_MAX_DEPTH) {
        return false;
    }
    let u_idx: usize = idx as usize;
    if (u_idx >= ast.node_count) {
        return false;
    }
    let node: SpdxAstNode = ast.nodes[u_idx];
    if (node.kind == SPDX_NODE_LICENSE) {
        return spdxpMatchLicenseNode(node, allowed);
    }
    if (node.kind == SPDX_NODE_AND) {
        return spdxpEvalAst(ast, node.left, allowed, depth + 1)
            && spdxpEvalAst(ast, node.right, allowed, depth + 1);
    }
    if (node.kind == SPDX_NODE_OR) {
        return spdxpEvalAst(ast, node.left, allowed, depth + 1)
            || spdxpEvalAst(ast, node.right, allowed, depth + 1);
    }
    return false;
}

/// Evaluate whether a single allowed license string satisfies the parsed `SpdxAst`.
pub fn satisfiesSpdx(ast: SpdxAst, allowed_license: string): bool {
    if (!ast.valid || ast.root < 0) {
        return false;
    }
    return spdxpEvalAst(ast, ast.root, allowed_license, 0);
}
