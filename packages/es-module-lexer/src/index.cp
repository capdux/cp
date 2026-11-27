// packages/@cap/es-module-lexer/src/index.cp
// Clean-room zero-allocation ES Module static/dynamic import & export lexer for CP.
// Parity with npm:es-module-lexer.
// Accelerated with 16-lane @Vector(16, u8) SIMD scanning and comptime 256-entry LUT.

type EsmlVec16 = @Vector(16, u8);
type EsmlBool16 = @Vector(16, bool);

const ESML_FLAG_WS: u8 = 1;
const ESML_FLAG_IDENT_START: u8 = 2;
const ESML_FLAG_IDENT_PART: u8 = 4;

fn esmlBuildCharLut(): [256]u8 {
    @setEvalBranchQuota(20000);
    let lut: [256]u8 = [0; 256];
    let c: usize = 0;
    while (c < 256) {
        let f: u8 = 0;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            f |= ESML_FLAG_WS;
        }
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c >= 0x80) {
            f |= ESML_FLAG_IDENT_START | ESML_FLAG_IDENT_PART;
        } else if (c >= '0' && c <= '9') {
            f |= ESML_FLAG_IDENT_PART;
        }
        lut[c] = f;
        c += 1;
    }
    return lut;
}

const ESML_CHAR_LUT: [256]u8 = esmlBuildCharLut();

fn esmlFindByteSimd(s: string, target: u8, start: usize): usize {
    let i = start;
    const len = s.len;
    const vt: EsmlVec16 = @splat(target);
    while (i + 16 <= len) {
        const v: EsmlVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == vt)) {
            break;
        }
        i += 16;
    }
    while (i < len && s[i] != target) {
        i += 1;
    }
    return i;
}

fn esmlFindTwoBytesSimd(s: string, a: u8, b: u8, start: usize): usize {
    let i = start;
    const len = s.len;
    const va: EsmlVec16 = @splat(a);
    const vb: EsmlVec16 = @splat(b);
    while (i + 16 <= len) {
        const v: EsmlVec16 = s[i .. i + 16][0..16].*;
        if (@reduce(.Or, v == va) || @reduce(.Or, v == vb)) {
            break;
        }
        i += 16;
    }
    while (i < len && s[i] != a && s[i] != b) {
        i += 1;
    }
    return i;
}

pub enum ImportKind {
    Static,
    Dynamic,
    ImportMeta,
}

pub struct ImportSpecifier {
    kind: ImportKind,
    spec_start: usize,
    spec_end: usize,
    stmt_start: usize,
    stmt_end: usize,

    fn specifier(src: string): string {
        if (_.spec_start <= _.spec_end && _.spec_end <= src.len) {
            return src[_.spec_start.._.spec_end];
        }
        return "";
    }
}

pub struct ExportSpecifier {
    name_start: usize,
    name_end: usize,
    local_start: usize,
    local_end: usize,

    fn name(src: string): string {
        if (_.name_start <= _.name_end && _.name_end <= src.len) {
            return src[_.name_start.._.name_end];
        }
        return "";
    }

    fn localName(src: string): string {
        if (_.local_start <= _.local_end && _.local_end <= src.len) {
            return src[_.local_start.._.local_end];
        }
        return "";
    }
}

pub struct EsmParseResult {
    import_count: usize,
    export_count: usize,
    facade: bool,
    has_module_syntax: bool,
}

struct EsmlSpan {
    start: usize,
    end: usize,
    ok: bool,
}

fn esmlIsWs(c: u8): bool {
    return (ESML_CHAR_LUT[c as usize] & ESML_FLAG_WS) != 0;
}

fn esmlIsIdentStart(c: u8): bool {
    return (ESML_CHAR_LUT[c as usize] & ESML_FLAG_IDENT_START) != 0;
}

fn esmlIsIdentPart(c: u8): bool {
    return (ESML_CHAR_LUT[c as usize] & ESML_FLAG_IDENT_PART) != 0;
}

fn esmlStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i + 16 <= a.len) {
        const va: EsmlVec16 = a[i .. i + 16][0..16].*;
        const vb: EsmlVec16 = b[i .. i + 16][0..16].*;
        if (!@reduce(.And, va == vb)) {
            return false;
        }
        i += 16;
    }
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn esmlSkipWsAndComments(src: string, start: usize): usize {
    let i = start;
    const len = src.len;
    while (i < len) {
        const c = src[i];
        if ((ESML_CHAR_LUT[c as usize] & ESML_FLAG_WS) != 0) {
            i += 1;
            while (i < len && (ESML_CHAR_LUT[src[i] as usize] & ESML_FLAG_WS) != 0) {
                i += 1;
            }
            continue;
        }
        if (c == '/' && i + 1 < len) {
            if (src[i + 1] == '/') {
                i = esmlFindByteSimd(src, '\n' as u8, i + 2);
                continue;
            }
            if (src[i + 1] == '*') {
                i += 2;
                while (i + 1 < len) {
                    i = esmlFindByteSimd(src, '*' as u8, i);
                    if (i + 1 < len && src[i + 1] == '/') {
                        i += 2;
                        break;
                    }
                    if (i + 1 < len) {
                        i += 1;
                    } else {
                        i = len;
                        break;
                    }
                }
                if (i + 1 == len) {
                    i = len;
                }
                continue;
            }
        }
        break;
    }
    return i;
}

fn esmlScanString(src: string, start: usize): EsmlSpan {
    if (start >= src.len) return EsmlSpan{ start: start, end: start, ok: false };
    const quote = src[start];
    if (quote != '\'' && quote != '"') {
        return EsmlSpan{ start: start, end: start, ok: false };
    }
    let i = start + 1;
    while (i < src.len) {
        i = esmlFindTwoBytesSimd(src, quote, '\\' as u8, i);
        if (i >= src.len) {
            break;
        }
        const c = src[i];
        if (c == '\\') {
            i += 2;
            continue;
        }
        return EsmlSpan{ start: start + 1, end: i, ok: true };
    }
    return EsmlSpan{ start: start + 1, end: src.len, ok: false };
}

fn esmlSkipTemplate(src: string, start: usize): usize {
    let i = start + 1;
    while (i < src.len) {
        i = esmlFindTwoBytesSimd(src, '`' as u8, '\\' as u8, i);
        if (i >= src.len) {
            break;
        }
        const c = src[i];
        if (c == '\\') {
            i += 2;
            continue;
        }
        return i + 1;
    }
    return src.len;
}

fn esmlScanIdent(src: string, start: usize): EsmlSpan {
    if (start >= src.len || !esmlIsIdentStart(src[start])) {
        return EsmlSpan{ start: start, end: start, ok: false };
    }
    let i = start + 1;
    const len = src.len;
    const v_a: EsmlVec16 = @splat('a' as u8);
    const v_z: EsmlVec16 = @splat('z' as u8);
    const v_A: EsmlVec16 = @splat('A' as u8);
    const v_Z: EsmlVec16 = @splat('Z' as u8);
    const v_0: EsmlVec16 = @splat('0' as u8);
    const v_9: EsmlVec16 = @splat('9' as u8);
    const v_u: EsmlVec16 = @splat('_' as u8);
    const v_false: EsmlBool16 = @splat(false);
    const v_true: EsmlBool16 = @splat(true);
    while (i + 16 <= len) {
        const v: EsmlVec16 = src[i .. i + 16][0..16].*;
        const is_lo = @select(bool, v >= v_a, v <= v_z, v_false);
        const is_hi = @select(bool, v >= v_A, v <= v_Z, v_false);
        const is_dg = @select(bool, v >= v_0, v <= v_9, v_false);
        const ok1 = @select(bool, is_lo, v_true, is_hi);
        const ok2 = @select(bool, is_dg, v_true, v == v_u);
        const ok_all = @select(bool, ok1, v_true, ok2);
        if (!@reduce(.And, ok_all)) {
            break;
        }
        i += 16;
    }
    while (i < len && (ESML_CHAR_LUT[src[i] as usize] & ESML_FLAG_IDENT_PART) != 0) {
        i += 1;
    }
    return EsmlSpan{ start: start, end: i, ok: true };
}

fn esmlPrevNonWsChar(src: string, pos: usize): u8 {
    let i = pos;
    while (i > 0) {
        i -= 1;
        if (!esmlIsWs(src[i])) return src[i];
    }
    return 0;
}

fn esmlPushImport(
    out: []ImportSpecifier,
    count: usize,
    kind: ImportKind,
    spec_start: usize,
    spec_end: usize,
    stmt_start: usize,
    stmt_end: usize,
): usize {
    if (count < out.len) {
        out[count] = ImportSpecifier{
            kind: kind,
            spec_start: spec_start,
            spec_end: spec_end,
            stmt_start: stmt_start,
            stmt_end: stmt_end,
        };
    }
    return count + 1;
}

fn esmlPushExport(
    out: []ExportSpecifier,
    count: usize,
    name_start: usize,
    name_end: usize,
    local_start: usize,
    local_end: usize,
): usize {
    if (count < out.len) {
        out[count] = ExportSpecifier{
            name_start: name_start,
            name_end: name_end,
            local_start: local_start,
            local_end: local_end,
        };
    }
    return count + 1;
}

pub fn parseEsm(
    src: string,
    imports_out: []ImportSpecifier,
    exports_out: []ExportSpecifier,
): EsmParseResult {
    let imp_count: usize = 0;
    let exp_count: usize = 0;
    let facade = true;
    let has_mod = false;

    let i: usize = 0;
    while (i < src.len) {
        i = esmlSkipWsAndComments(src, i);
        if (i >= src.len) break;

        const c = src[i];
        if (c == ';') {
            i += 1;
            continue;
        }
        if (c == '\'' || c == '"') {
            facade = false;
            const s = esmlScanString(src, i);
            i = if (s.ok) s.end + 1 else src.len;
            continue;
        }
        if (c == '`') {
            facade = false;
            i = esmlSkipTemplate(src, i);
            continue;
        }

        if (esmlIsIdentStart(c)) {
            const stmt_start = i;
            const prev_ch = esmlPrevNonWsChar(src, stmt_start);
            const id = esmlScanIdent(src, i);
            const word = src[id.start..id.end];
            i = id.end;

            if (prev_ch != '.' && esmlStrEq(word, "import")) {
                const after = esmlSkipWsAndComments(src, i);
                if (after < src.len && src[after] == '.') {
                    // import.meta
                    const m_start = esmlSkipWsAndComments(src, after + 1);
                    const m_id = esmlScanIdent(src, m_start);
                    if (m_id.ok && esmlStrEq(src[m_id.start..m_id.end], "meta")) {
                        has_mod = true;
                        facade = false;
                        imp_count = esmlPushImport(
                            imports_out,
                            imp_count,
                            .ImportMeta,
                            stmt_start,
                            m_id.end,
                            stmt_start,
                            m_id.end,
                        );
                        i = m_id.end;
                        continue;
                    }
                } else if (after < src.len && src[after] == '(') {
                    // Dynamic import(...)
                    has_mod = true;
                    facade = false;
                    const arg_start = esmlSkipWsAndComments(src, after + 1);
                    let spec_start = arg_start;
                    let spec_end = arg_start;
                    let k = arg_start;
                    if (arg_start < src.len && (src[arg_start] == '\'' || src[arg_start] == '"')) {
                        const s = esmlScanString(src, arg_start);
                        spec_start = s.start;
                        spec_end = s.end;
                        k = if (s.ok) s.end + 1 else src.len;
                    }
                    let depth: usize = 1;
                    while (k < src.len && depth > 0) {
                        const kc = src[k];
                        if (kc == '\'' || kc == '"') {
                            const s = esmlScanString(src, k);
                            k = if (s.ok) s.end + 1 else src.len;
                            continue;
                        }
                        if (kc == '(') {
                            depth += 1;
                        } else if (kc == ')') {
                            depth -= 1;
                            if (depth == 0) {
                                if (spec_end == arg_start) {
                                    spec_end = k;
                                }
                                k += 1;
                                break;
                            }
                        }
                        k += 1;
                    }
                    imp_count = esmlPushImport(
                        imports_out,
                        imp_count,
                        .Dynamic,
                        spec_start,
                        spec_end,
                        stmt_start,
                        k,
                    );
                    i = k;
                    continue;
                } else if (after < src.len && (src[after] == '\'' || src[after] == '"')) {
                    // Side-effect import 'mod'
                    has_mod = true;
                    const s = esmlScanString(src, after);
                    const stmt_end = if (s.ok) s.end + 1 else src.len;
                    imp_count = esmlPushImport(
                        imports_out,
                        imp_count,
                        .Static,
                        s.start,
                        s.end,
                        stmt_start,
                        stmt_end,
                    );
                    i = stmt_end;
                    continue;
                } else {
                    // Static import ... from 'mod'
                    let k = after;
                    while (k < src.len) {
                        k = esmlSkipWsAndComments(src, k);
                        if (k >= src.len) break;
                        const kc = src[k];
                        if (kc == '\'' || kc == '"') {
                            const s = esmlScanString(src, k);
                            k = if (s.ok) s.end + 1 else src.len;
                            continue;
                        }
                        if (kc == ';') {
                            k += 1;
                            break;
                        }
                        if (esmlIsIdentStart(kc)) {
                            const kw = esmlScanIdent(src, k);
                            k = kw.end;
                            if (esmlStrEq(src[kw.start..kw.end], "from")) {
                                const sp = esmlSkipWsAndComments(src, k);
                                if (sp < src.len && (src[sp] == '\'' || src[sp] == '"')) {
                                    const s = esmlScanString(src, sp);
                                    const stmt_end = if (s.ok) s.end + 1 else src.len;
                                    has_mod = true;
                                    imp_count = esmlPushImport(
                                        imports_out,
                                        imp_count,
                                        .Static,
                                        s.start,
                                        s.end,
                                        stmt_start,
                                        stmt_end,
                                    );
                                    k = stmt_end;
                                }
                                break;
                            }
                            continue;
                        }
                        k += 1;
                    }
                    i = k;
                    continue;
                }
            }

            if (prev_ch != '.' && esmlStrEq(word, "export")) {
                has_mod = true;
                let k = esmlSkipWsAndComments(src, i);
                if (k >= src.len) break;

                if (src[k] == '*') {
                    // export * [as ns] from 'mod'
                    k = esmlSkipWsAndComments(src, k + 1);
                    const next_id = esmlScanIdent(src, k);
                    if (next_id.ok && esmlStrEq(src[next_id.start..next_id.end], "as")) {
                        const ns_pos = esmlSkipWsAndComments(src, next_id.end);
                        const ns_id = esmlScanIdent(src, ns_pos);
                        if (ns_id.ok) {
                            exp_count = esmlPushExport(
                                exports_out,
                                exp_count,
                                ns_id.start,
                                ns_id.end,
                                ns_id.start,
                                ns_id.end,
                            );
                            k = esmlSkipWsAndComments(src, ns_id.end);
                        }
                    }
                    const from_id = esmlScanIdent(src, k);
                    if (from_id.ok && esmlStrEq(src[from_id.start..from_id.end], "from")) {
                        const sp = esmlSkipWsAndComments(src, from_id.end);
                        if (sp < src.len && (src[sp] == '\'' || src[sp] == '"')) {
                            const s = esmlScanString(src, sp);
                            const stmt_end = if (s.ok) s.end + 1 else src.len;
                            imp_count = esmlPushImport(
                                imports_out,
                                imp_count,
                                .Static,
                                s.start,
                                s.end,
                                stmt_start,
                                stmt_end,
                            );
                            k = stmt_end;
                        }
                    }
                    i = k;
                    continue;
                }

                if (src[k] == '{') {
                    // export { a, b as c } [from 'mod']
                    k += 1;
                    while (k < src.len) {
                        k = esmlSkipWsAndComments(src, k);
                        if (k >= src.len || src[k] == '}') {
                            if (k < src.len) k += 1;
                            break;
                        }
                        if (src[k] == ',') {
                            k += 1;
                            continue;
                        }
                        let loc_start = k;
                        let loc_end = k;
                        if (src[k] == '\'' || src[k] == '"') {
                            const s = esmlScanString(src, k);
                            loc_start = s.start;
                            loc_end = s.end;
                            k = if (s.ok) s.end + 1 else src.len;
                        } else {
                            const lid = esmlScanIdent(src, k);
                            if (!lid.ok) {
                                k += 1;
                                continue;
                            }
                            loc_start = lid.start;
                            loc_end = lid.end;
                            k = lid.end;
                        }

                        let exp_start = loc_start;
                        let exp_end = loc_end;
                        const after_loc = esmlSkipWsAndComments(src, k);
                        const maybe_as = esmlScanIdent(src, after_loc);
                        if (maybe_as.ok && esmlStrEq(src[maybe_as.start..maybe_as.end], "as")) {
                            const alias_pos = esmlSkipWsAndComments(src, maybe_as.end);
                            if (alias_pos < src.len && (src[alias_pos] == '\'' || src[alias_pos] == '"')) {
                                const s = esmlScanString(src, alias_pos);
                                exp_start = s.start;
                                exp_end = s.end;
                                k = if (s.ok) s.end + 1 else src.len;
                            } else {
                                const aid = esmlScanIdent(src, alias_pos);
                                if (aid.ok) {
                                    exp_start = aid.start;
                                    exp_end = aid.end;
                                    k = aid.end;
                                }
                            }
                        }
                        exp_count = esmlPushExport(
                            exports_out,
                            exp_count,
                            exp_start,
                            exp_end,
                            loc_start,
                            loc_end,
                        );
                    }

                    const after_brace = esmlSkipWsAndComments(src, k);
                    const from_id = esmlScanIdent(src, after_brace);
                    if (from_id.ok && esmlStrEq(src[from_id.start..from_id.end], "from")) {
                        const sp = esmlSkipWsAndComments(src, from_id.end);
                        if (sp < src.len && (src[sp] == '\'' || src[sp] == '"')) {
                            const s = esmlScanString(src, sp);
                            const stmt_end = if (s.ok) s.end + 1 else src.len;
                            imp_count = esmlPushImport(
                                imports_out,
                                imp_count,
                                .Static,
                                s.start,
                                s.end,
                                stmt_start,
                                stmt_end,
                            );
                            k = stmt_end;
                        }
                    } else {
                        facade = false;
                    }
                    i = k;
                    continue;
                }

                // export default / const / let / var / function / async function / class
                facade = false;
                const kw = esmlScanIdent(src, k);
                if (kw.ok) {
                    const kw_str = src[kw.start..kw.end];
                    k = kw.end;
                    if (esmlStrEq(kw_str, "default")) {
                        exp_count = esmlPushExport(
                            exports_out,
                            exp_count,
                            kw.start,
                            kw.end,
                            kw.start,
                            kw.end,
                        );
                    } else if (esmlStrEq(kw_str, "async")) {
                        const fn_pos = esmlSkipWsAndComments(src, k);
                        const fn_kw = esmlScanIdent(src, fn_pos);
                        if (fn_kw.ok && esmlStrEq(src[fn_kw.start..fn_kw.end], "function")) {
                            let name_pos = esmlSkipWsAndComments(src, fn_kw.end);
                            if (name_pos < src.len && src[name_pos] == '*') {
                                name_pos = esmlSkipWsAndComments(src, name_pos + 1);
                            }
                            const name_id = esmlScanIdent(src, name_pos);
                            if (name_id.ok) {
                                exp_count = esmlPushExport(
                                    exports_out,
                                    exp_count,
                                    name_id.start,
                                    name_id.end,
                                    name_id.start,
                                    name_id.end,
                                );
                                k = name_id.end;
                            }
                        }
                    } else if (
                        esmlStrEq(kw_str, "function") ||
                        esmlStrEq(kw_str, "class") ||
                        esmlStrEq(kw_str, "const") ||
                        esmlStrEq(kw_str, "let") ||
                        esmlStrEq(kw_str, "var")
                    ) {
                        let name_pos = esmlSkipWsAndComments(src, k);
                        if (name_pos < src.len && src[name_pos] == '*') {
                            name_pos = esmlSkipWsAndComments(src, name_pos + 1);
                        }
                        const name_id = esmlScanIdent(src, name_pos);
                        if (name_id.ok) {
                            exp_count = esmlPushExport(
                                exports_out,
                                exp_count,
                                name_id.start,
                                name_id.end,
                                name_id.start,
                                name_id.end,
                            );
                            k = name_id.end;
                        }
                    }
                }
                i = k;
                continue;
            }

            // Any other top-level identifier breaks facade
            facade = false;
            continue;
        }

        // Non-identifier token
        facade = false;
        i += 1;
    }

    return EsmParseResult{
        import_count: imp_count,
        export_count: exp_count,
        facade: facade,
        has_module_syntax: has_mod,
    };
}

pub fn isFacade(src: string): bool {
    let empty_imp: [0]ImportSpecifier = undefined;
    let empty_exp: [0]ExportSpecifier = undefined;
    const res = parseEsm(src, empty_imp[0..0], empty_exp[0..0]);
    return res.facade;
}
