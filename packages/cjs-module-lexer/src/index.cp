// @cap/cjs-module-lexer — Zero-allocation CommonJS static export & re-export lexer
// Clean-room MIT implementation inspired by Node.js cjs-module-lexer
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning and comptime 256-entry character classification LUT.

type CjslVec16 = @Vector(16, u8);

const CJSL_FLAG_WS: u8 = 1;
const CJSL_FLAG_IDENT_START: u8 = 2;
const CJSL_FLAG_IDENT_CHAR: u8 = 4;

fn cjslBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    lut[9] = CJSL_FLAG_WS;
    lut[10] = CJSL_FLAG_WS;
    lut[13] = CJSL_FLAG_WS;
    lut[32] = CJSL_FLAG_WS;
    let c: usize = 48;
    while (c <= 57) {
        lut[c] = CJSL_FLAG_IDENT_CHAR;
        c += 1;
    }
    c = 65;
    while (c <= 90) {
        lut[c] = CJSL_FLAG_IDENT_START | CJSL_FLAG_IDENT_CHAR;
        c += 1;
    }
    c = 97;
    while (c <= 122) {
        lut[c] = CJSL_FLAG_IDENT_START | CJSL_FLAG_IDENT_CHAR;
        c += 1;
    }
    lut[95] = CJSL_FLAG_IDENT_START | CJSL_FLAG_IDENT_CHAR; // '_'
    lut[36] = CJSL_FLAG_IDENT_START | CJSL_FLAG_IDENT_CHAR; // '$'
    return lut;
}

const CJSL_CHAR_LUT: [256]u8 = cjslBuildCharLut();

pub const CJSL_MAX_EXPORTS: usize = 32;
pub const CJSL_MAX_REEXPORTS: usize = 16;

pub struct CjsParseResult {
    exports: [32]string,
    export_count: usize,
    reexports: [16]string,
    reexport_count: usize,
}

struct CjslLexer {
    src: string,
    pos: usize,
}

struct CjslStrRes {
    ok: bool,
    val: string,
}

fn cjslIsWs(ch: u8): bool {
    return (CJSL_CHAR_LUT[ch as usize] & CJSL_FLAG_WS) != 0;
}

fn cjslIsIdentStart(ch: u8): bool {
    return (CJSL_CHAR_LUT[ch as usize] & CJSL_FLAG_IDENT_START) != 0;
}

fn cjslIsIdentChar(ch: u8): bool {
    return (CJSL_CHAR_LUT[ch as usize] & CJSL_FLAG_IDENT_CHAR) != 0;
}

fn cjslStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: CjslVec16 = a[i .. i + 16][0..16].*;
        const vb: CjslVec16 = b[i .. i + 16][0..16].*;
        if (@reduce(.Or, va != vb)) return false;
        i += 16;
    }
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn cjslAddExport(res: *CjsParseResult, name: string) {
    if (name.len == 0) return;
    let i: usize = 0;
    while (i < res.export_count) : (i += 1) {
        if (cjslStrEq(res.exports[i], name)) return;
    }
    if (res.export_count < CJSL_MAX_EXPORTS) {
        res.exports[res.export_count] = name;
        res.export_count += 1;
    }
}

fn cjslAddReexport(res: *CjsParseResult, spec: string) {
    if (spec.len == 0) return;
    let i: usize = 0;
    while (i < res.reexport_count) : (i += 1) {
        if (cjslStrEq(res.reexports[i], spec)) return;
    }
    if (res.reexport_count < CJSL_MAX_REEXPORTS) {
        res.reexports[res.reexport_count] = spec;
        res.reexport_count += 1;
    }
}

fn cjslSkipString(lex: *CjslLexer) {
    if (lex.pos >= lex.src.len) return;
    const q = lex.src[lex.pos];
    lex.pos += 1;
    const v_q: CjslVec16 = @splat(q);
    const v_bs: CjslVec16 = @splat(92 as u8);
    while (lex.pos < lex.src.len) {
        while (lex.pos + 16 <= lex.src.len) {
            const v: CjslVec16 = lex.src[lex.pos .. lex.pos + 16][0..16].*;
            if (!@reduce(.Or, v == v_q) && !@reduce(.Or, v == v_bs)) {
                lex.pos += 16;
                continue;
            }
            break;
        }
        if (lex.pos >= lex.src.len) return;
        const ch = lex.src[lex.pos];
        if (ch == '\\' && lex.pos + 1 < lex.src.len) {
            lex.pos += 2;
            continue;
        }
        if (ch == q) {
            lex.pos += 1;
            return;
        }
        lex.pos += 1;
    }
}

fn cjslSkipWsAndComments(lex: *CjslLexer) {
    const v_zero: CjslVec16 = @splat(0 as u8);
    const v_one: CjslVec16 = @splat(1 as u8);
    const v_sp: CjslVec16 = @splat(32 as u8);
    const v_tab: CjslVec16 = @splat(9 as u8);
    const v_nl: CjslVec16 = @splat(10 as u8);
    const v_cr: CjslVec16 = @splat(13 as u8);
    const v_star: CjslVec16 = @splat(42 as u8);

    while (lex.pos < lex.src.len) {
        const ch = lex.src[lex.pos];
        if (cjslIsWs(ch)) {
            while (lex.pos + 16 <= lex.src.len) {
                const v: CjslVec16 = lex.src[lex.pos .. lex.pos + 16][0..16].*;
                const ws = @select(u8, v == v_sp, v_one, v_zero)
                    | @select(u8, v == v_tab, v_one, v_zero)
                    | @select(u8, v == v_nl, v_one, v_zero)
                    | @select(u8, v == v_cr, v_one, v_zero);
                if (@reduce(.And, ws != v_zero)) {
                    lex.pos += 16;
                    continue;
                }
                break;
            }
            while (lex.pos < lex.src.len && cjslIsWs(lex.src[lex.pos])) {
                lex.pos += 1;
            }
            continue;
        }
        if (ch == '/' && lex.pos + 1 < lex.src.len) {
            const nxt = lex.src[lex.pos + 1];
            if (nxt == '/') {
                lex.pos += 2;
                while (lex.pos + 16 <= lex.src.len) {
                    const v: CjslVec16 = lex.src[lex.pos .. lex.pos + 16][0..16].*;
                    if (!@reduce(.Or, v == v_nl)) {
                        lex.pos += 16;
                        continue;
                    }
                    break;
                }
                while (lex.pos < lex.src.len && lex.src[lex.pos] != '\n') {
                    lex.pos += 1;
                }
                continue;
            }
            if (nxt == '*') {
                lex.pos += 2;
                while (lex.pos + 1 < lex.src.len) {
                    while (lex.pos + 16 < lex.src.len) {
                        const v: CjslVec16 = lex.src[lex.pos .. lex.pos + 16][0..16].*;
                        if (!@reduce(.Or, v == v_star)) {
                            lex.pos += 16;
                            continue;
                        }
                        break;
                    }
                    if (lex.pos + 1 < lex.src.len && lex.src[lex.pos] == '*' && lex.src[lex.pos + 1] == '/') {
                        lex.pos += 2;
                        break;
                    }
                    lex.pos += 1;
                }
                continue;
            }
        }
        break;
    }
}

fn cjslReadIdent(lex: *CjslLexer): string {
    if (lex.pos >= lex.src.len || !cjslIsIdentStart(lex.src[lex.pos])) return "";
    const start = lex.pos;
    lex.pos += 1;
    const v_zero: CjslVec16 = @splat(0 as u8);
    const v_one: CjslVec16 = @splat(1 as u8);
    const v_lo_a: CjslVec16 = @splat(97 as u8);
    const v_lo_z: CjslVec16 = @splat(122 as u8);
    const v_up_a: CjslVec16 = @splat(65 as u8);
    const v_up_z: CjslVec16 = @splat(90 as u8);
    const v_d0: CjslVec16 = @splat(48 as u8);
    const v_d9: CjslVec16 = @splat(57 as u8);
    const v_us: CjslVec16 = @splat(95 as u8);
    const v_dl: CjslVec16 = @splat(36 as u8);
    while (lex.pos + 16 <= lex.src.len) {
        const v: CjslVec16 = lex.src[lex.pos .. lex.pos + 16][0..16].*;
        const ok = (@select(u8, v >= v_lo_a, v_one, v_zero) & @select(u8, v <= v_lo_z, v_one, v_zero))
            | (@select(u8, v >= v_up_a, v_one, v_zero) & @select(u8, v <= v_up_z, v_one, v_zero))
            | (@select(u8, v >= v_d0, v_one, v_zero) & @select(u8, v <= v_d9, v_one, v_zero))
            | @select(u8, v == v_us, v_one, v_zero)
            | @select(u8, v == v_dl, v_one, v_zero);
        if (@reduce(.And, ok != v_zero)) {
            lex.pos += 16;
            continue;
        }
        break;
    }
    while (lex.pos < lex.src.len && cjslIsIdentChar(lex.src[lex.pos])) {
        lex.pos += 1;
    }
    return lex.src[start..lex.pos];
}

fn cjslReadStringLit(lex: *CjslLexer): CjslStrRes {
    if (lex.pos >= lex.src.len) return CjslStrRes{ ok: false, val: "" };
    const q = lex.src[lex.pos];
    if (q != '\'' && q != '"') return CjslStrRes{ ok: false, val: "" };
    lex.pos += 1;
    const start = lex.pos;
    const v_q: CjslVec16 = @splat(q);
    const v_bs: CjslVec16 = @splat(92 as u8);
    while (lex.pos < lex.src.len) {
        while (lex.pos + 16 <= lex.src.len) {
            const v: CjslVec16 = lex.src[lex.pos .. lex.pos + 16][0..16].*;
            if (!@reduce(.Or, v == v_q) && !@reduce(.Or, v == v_bs)) {
                lex.pos += 16;
                continue;
            }
            break;
        }
        if (lex.pos >= lex.src.len) break;
        const ch = lex.src[lex.pos];
        if (ch == '\\' && lex.pos + 1 < lex.src.len) {
            lex.pos += 2;
            continue;
        }
        if (ch == q) {
            const s = lex.src[start..lex.pos];
            lex.pos += 1;
            return CjslStrRes{ ok: true, val: s };
        }
        lex.pos += 1;
    }
    return CjslStrRes{ ok: false, val: "" };
}

fn cjslMatchRequireCall(lex: *CjslLexer): CjslStrRes {
    const save = lex.pos;
    cjslSkipWsAndComments(lex);
    const id = cjslReadIdent(lex);
    if (!cjslStrEq(id, "require")) {
        lex.pos = save;
        return CjslStrRes{ ok: false, val: "" };
    }
    cjslSkipWsAndComments(lex);
    if (lex.pos >= lex.src.len || lex.src[lex.pos] != '(') {
        lex.pos = save;
        return CjslStrRes{ ok: false, val: "" };
    }
    lex.pos += 1;
    cjslSkipWsAndComments(lex);
    const s = cjslReadStringLit(lex);
    if (!s.ok) {
        lex.pos = save;
        return CjslStrRes{ ok: false, val: "" };
    }
    cjslSkipWsAndComments(lex);
    if (lex.pos >= lex.src.len || lex.src[lex.pos] != ')') {
        lex.pos = save;
        return CjslStrRes{ ok: false, val: "" };
    }
    lex.pos += 1;
    return s;
}

fn cjslSkipPropValue(lex: *CjslLexer) {
    let brace_d: usize = 0;
    let paren_d: usize = 0;
    let brack_d: usize = 0;

    while (lex.pos < lex.src.len) {
        cjslSkipWsAndComments(lex);
        if (lex.pos >= lex.src.len) break;
        const ch = lex.src[lex.pos];
        if (ch == '\'' || ch == '"' || ch == '`') {
            cjslSkipString(lex);
            continue;
        }
        if (ch == '{') {
            brace_d += 1;
            lex.pos += 1;
            continue;
        }
        if (ch == '}') {
            if (brace_d == 0 && paren_d == 0 && brack_d == 0) return;
            if (brace_d > 0) brace_d -= 1;
            lex.pos += 1;
            continue;
        }
        if (ch == '(') {
            paren_d += 1;
            lex.pos += 1;
            continue;
        }
        if (ch == ')') {
            if (paren_d > 0) paren_d -= 1;
            lex.pos += 1;
            continue;
        }
        if (ch == '[') {
            brack_d += 1;
            lex.pos += 1;
            continue;
        }
        if (ch == ']') {
            if (brack_d > 0) brack_d -= 1;
            lex.pos += 1;
            continue;
        }
        if (ch == ',' && brace_d == 0 && paren_d == 0 && brack_d == 0) {
            return;
        }
        lex.pos += 1;
    }
}

fn cjslParseObjectExports(lex: *CjslLexer, res: *CjsParseResult) {
    // caller verified lex.src[lex.pos] == '{'
    lex.pos += 1;
    while (lex.pos < lex.src.len) {
        cjslSkipWsAndComments(lex);
        if (lex.pos >= lex.src.len) break;
        const ch = lex.src[lex.pos];
        if (ch == '}') {
            lex.pos += 1;
            return;
        }
        if (ch == ',') {
            lex.pos += 1;
            continue;
        }
        // Spread re-export: ...require('./x')
        if (ch == '.' && lex.pos + 2 < lex.src.len && lex.src[lex.pos + 1] == '.' && lex.src[lex.pos + 2] == '.') {
            lex.pos += 3;
            const req = cjslMatchRequireCall(lex);
            if (req.ok) {
                cjslAddReexport(res, req.val);
            } else {
                cjslSkipPropValue(lex);
            }
            continue;
        }

        let key: string = "";
        if (ch == '\'' || ch == '"') {
            const s = cjslReadStringLit(lex);
            if (!s.ok) {
                cjslSkipPropValue(lex);
                continue;
            }
            key = s.val;
        } else if (cjslIsIdentStart(ch)) {
            key = cjslReadIdent(lex);
        } else {
            cjslSkipPropValue(lex);
            continue;
        }

        cjslSkipWsAndComments(lex);
        if (lex.pos >= lex.src.len) break;
        const next_ch = lex.src[lex.pos];
        if (next_ch == ',') {
            // Shorthand property: { a, ... }
            cjslAddExport(res, key);
            lex.pos += 1;
            continue;
        }
        if (next_ch == '}') {
            // Trailing shorthand property: { a }
            cjslAddExport(res, key);
            lex.pos += 1;
            return;
        }
        if (next_ch == ':') {
            cjslAddExport(res, key);
            lex.pos += 1;
            cjslSkipPropValue(lex);
            continue;
        }
        if (next_ch == '(') {
            // Method shorthand: { foo() { ... } }
            cjslAddExport(res, key);
            cjslSkipPropValue(lex);
            continue;
        }
        cjslSkipPropValue(lex);
    }
}

fn cjslParseExportsMemberAssign(lex: *CjslLexer, res: *CjsParseResult) {
    cjslSkipWsAndComments(lex);
    if (lex.pos >= lex.src.len) return;

    let prop: string = "";
    const ch = lex.src[lex.pos];
    if (ch == '.') {
        lex.pos += 1;
        cjslSkipWsAndComments(lex);
        prop = cjslReadIdent(lex);
    } else if (ch == '[') {
        lex.pos += 1;
        cjslSkipWsAndComments(lex);
        const s = cjslReadStringLit(lex);
        if (!s.ok) return;
        cjslSkipWsAndComments(lex);
        if (lex.pos >= lex.src.len || lex.src[lex.pos] != ']') return;
        lex.pos += 1;
        prop = s.val;
    } else {
        return;
    }

    if (prop.len == 0) return;
    cjslSkipWsAndComments(lex);
    if (lex.pos < lex.src.len && lex.src[lex.pos] == '=') {
        if (lex.pos + 1 < lex.src.len && lex.src[lex.pos + 1] == '=') return;
        lex.pos += 1;
        cjslAddExport(res, prop);
    }
}

fn cjslParseDefineProperty(lex: *CjslLexer, res: *CjsParseResult) {
    cjslSkipWsAndComments(lex);
    if (lex.pos >= lex.src.len || lex.src[lex.pos] != '.') return;
    lex.pos += 1;
    cjslSkipWsAndComments(lex);
    const meth = cjslReadIdent(lex);
    if (!cjslStrEq(meth, "defineProperty")) return;

    cjslSkipWsAndComments(lex);
    if (lex.pos >= lex.src.len || lex.src[lex.pos] != '(') return;
    lex.pos += 1;
    cjslSkipWsAndComments(lex);

    const target = cjslReadIdent(lex);
    let is_exports_target = false;
    if (cjslStrEq(target, "exports")) {
        is_exports_target = true;
    } else if (cjslStrEq(target, "module")) {
        cjslSkipWsAndComments(lex);
        if (lex.pos < lex.src.len && lex.src[lex.pos] == '.') {
            lex.pos += 1;
            cjslSkipWsAndComments(lex);
            const sub = cjslReadIdent(lex);
            if (cjslStrEq(sub, "exports")) {
                is_exports_target = true;
            }
        }
    }
    if (!is_exports_target) return;

    cjslSkipWsAndComments(lex);
    if (lex.pos >= lex.src.len || lex.src[lex.pos] != ',') return;
    lex.pos += 1;
    cjslSkipWsAndComments(lex);

    const key = cjslReadStringLit(lex);
    if (key.ok) {
        cjslAddExport(res, key.val);
    }
}

fn cjslParseExportStar(lex: *CjslLexer, res: *CjsParseResult) {
    cjslSkipWsAndComments(lex);
    if (lex.pos >= lex.src.len || lex.src[lex.pos] != '(') return;
    lex.pos += 1;
    const req = cjslMatchRequireCall(lex);
    if (req.ok) {
        cjslAddReexport(res, req.val);
    }
}

pub fn parseCjs(src: string): CjsParseResult {
    let empty_exp: [32]string = undefined;
    let i: usize = 0;
    while (i < CJSL_MAX_EXPORTS) : (i += 1) {
        empty_exp[i] = "";
    }
    let empty_re: [16]string = undefined;
    let j: usize = 0;
    while (j < CJSL_MAX_REEXPORTS) : (j += 1) {
        empty_re[j] = "";
    }

    let res = CjsParseResult{
        exports: empty_exp,
        export_count: 0,
        reexports: empty_re,
        reexport_count: 0,
    };

    let lex = CjslLexer{ src: src, pos: 0 };
    let prev_was_dot = false;

    while (lex.pos < src.len) {
        cjslSkipWsAndComments(*lex);
        if (lex.pos >= src.len) break;

        const ch = src[lex.pos];
        if (ch == '\'' || ch == '"' || ch == '`') {
            cjslSkipString(*lex);
            prev_was_dot = false;
            continue;
        }

        if (cjslIsIdentStart(ch)) {
            const ident = cjslReadIdent(*lex);
            if (!prev_was_dot) {
                if (cjslStrEq(ident, "exports")) {
                    cjslParseExportsMemberAssign(*lex, *res);
                } else if (cjslStrEq(ident, "module")) {
                    const save = lex.pos;
                    cjslSkipWsAndComments(*lex);
                    if (lex.pos < src.len && src[lex.pos] == '.') {
                        lex.pos += 1;
                        cjslSkipWsAndComments(*lex);
                        const sub = cjslReadIdent(*lex);
                        if (cjslStrEq(sub, "exports")) {
                            cjslSkipWsAndComments(*lex);
                            if (lex.pos < src.len && (src[lex.pos] == '.' || src[lex.pos] == '[')) {
                                cjslParseExportsMemberAssign(*lex, *res);
                            } else if (lex.pos < src.len && src[lex.pos] == '=') {
                                if (lex.pos + 1 >= src.len || src[lex.pos + 1] != '=') {
                                    lex.pos += 1;
                                    cjslSkipWsAndComments(*lex);
                                    if (lex.pos < src.len && src[lex.pos] == '{') {
                                        cjslParseObjectExports(*lex, *res);
                                    } else {
                                        const req = cjslMatchRequireCall(*lex);
                                        if (req.ok) {
                                            cjslAddReexport(*res, req.val);
                                        }
                                    }
                                }
                            }
                        } else {
                            lex.pos = save;
                        }
                    } else {
                        lex.pos = save;
                    }
                } else if (cjslStrEq(ident, "Object")) {
                    cjslParseDefineProperty(*lex, *res);
                } else if (cjslStrEq(ident, "__exportStar")) {
                    cjslParseExportStar(*lex, *res);
                }
            }
            prev_was_dot = false;
            continue;
        }

        prev_was_dot = (ch == '.');
        lex.pos += 1;
    }

    return res;
}
