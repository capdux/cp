// @cap/cjs-module-lexer — Zero-allocation CommonJS static export & re-export lexer
// Clean-room MIT implementation inspired by Node.js cjs-module-lexer

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
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r';
}

fn cjslIsIdentStart(ch: u8): bool {
    if (ch >= 'a' && ch <= 'z') return true;
    if (ch >= 'A' && ch <= 'Z') return true;
    return ch == '_' || ch == '$';
}

fn cjslIsIdentChar(ch: u8): bool {
    if (cjslIsIdentStart(ch)) return true;
    return ch >= '0' && ch <= '9';
}

fn cjslStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
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
    while (lex.pos < lex.src.len) {
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
    while (lex.pos < lex.src.len) {
        const ch = lex.src[lex.pos];
        if (cjslIsWs(ch)) {
            lex.pos += 1;
            continue;
        }
        if (ch == '/' && lex.pos + 1 < lex.src.len) {
            const nxt = lex.src[lex.pos + 1];
            if (nxt == '/') {
                lex.pos += 2;
                while (lex.pos < lex.src.len && lex.src[lex.pos] != '\n') {
                    lex.pos += 1;
                }
                continue;
            }
            if (nxt == '*') {
                lex.pos += 2;
                while (lex.pos + 1 < lex.src.len) {
                    if (lex.src[lex.pos] == '*' && lex.src[lex.pos + 1] == '/') {
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
    while (lex.pos < lex.src.len) {
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
