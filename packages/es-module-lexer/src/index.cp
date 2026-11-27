// packages/@cap/es-module-lexer/src/index.cp
// Clean-room zero-allocation ES Module static/dynamic import & export lexer for CP.
// Parity with npm:es-module-lexer.

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
    return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

fn esmlIsIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || c == '$' || c >= 0x80;
}

fn esmlIsIdentPart(c: u8): bool {
    return esmlIsIdentStart(c) || (c >= '0' && c <= '9');
}

fn esmlStrEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn esmlSkipWsAndComments(src: string, start: usize): usize {
    let i = start;
    while (i < src.len) {
        const c = src[i];
        if (esmlIsWs(c)) {
            i += 1;
            continue;
        }
        if (c == '/' && i + 1 < src.len) {
            if (src[i + 1] == '/') {
                i += 2;
                while (i < src.len && src[i] != '\n') {
                    i += 1;
                }
                continue;
            }
            if (src[i + 1] == '*') {
                i += 2;
                while (i + 1 < src.len && !(src[i] == '*' && src[i + 1] == '/')) {
                    i += 1;
                }
                if (i + 1 < src.len) {
                    i += 2;
                } else {
                    i = src.len;
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
        const c = src[i];
        if (c == '\\') {
            i += 2;
            continue;
        }
        if (c == quote) {
            return EsmlSpan{ start: start + 1, end: i, ok: true };
        }
        i += 1;
    }
    return EsmlSpan{ start: start + 1, end: src.len, ok: false };
}

fn esmlSkipTemplate(src: string, start: usize): usize {
    let i = start + 1;
    while (i < src.len) {
        const c = src[i];
        if (c == '\\') {
            i += 2;
            continue;
        }
        if (c == '`') {
            return i + 1;
        }
        i += 1;
    }
    return src.len;
}

fn esmlScanIdent(src: string, start: usize): EsmlSpan {
    if (start >= src.len || !esmlIsIdentStart(src[start])) {
        return EsmlSpan{ start: start, end: start, ok: false };
    }
    let i = start + 1;
    while (i < src.len && esmlIsIdentPart(src[i])) {
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
