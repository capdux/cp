// Origin: Pure-CP Stdlib (AI Coding Agent Zero-Alloc Source Symbol Outline & Repo-Map Extractor) | Created: 2026-11-22
// Single-pass, zero-heap-allocation source code symbol outline & scope line-range extractor
// supporting CP, TypeScript, Rust, and Zig syntax while ignoring comments and string literals.

const std = @import("std");

const NO_SLOT: usize = 0xFFFFFFFF;
const MAX_SCOPE_STACK: usize = 64;

/// Classification of an extracted source-level symbol.
pub enum SymbolKind {
    Function,
    Class,
    Struct,
    Enum,
    Trait,
    Const,
    Import,
    Test,
}

/// Zero-copy symbol span with 1-based inclusive line range and lexical brace depth.
pub struct CodeSymbolSpan {
    kind: SymbolKind = SymbolKind.Function;
    name_start: usize = 0;
    name_end: usize = 0;
    start_line: u32 = 0;
    end_line: u32 = 0;
    brace_depth: u16 = 0;
    is_public: bool = false;

    /// Returns a zero-copy slice of the symbol name from `src` (bounds-safe).
    fn nameSlice(src: string): string {
        if (_.name_start <= _.name_end && _.name_end <= src.len) {
            return src[_.name_start.._.name_end];
        }
        return "";
    }
}

fn isIdentStart(c: u8): bool {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

fn isIdentContinue(c: u8): bool {
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

fn isSpaceOrTab(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r';
}

fn skipHorizontalAndComments(src: string, pos: *usize, line: *u32) {
    while (pos.* < src.len) {
        const c = src[pos.*];
        if (c == ' ' || c == '\t' || c == '\r') {
            pos.* += 1;
            continue;
        }
        if (c == '\n') {
            pos.* += 1;
            line.* += 1;
            continue;
        }
        if (c == '/' && pos.* + 1 < src.len && src[pos.* + 1] == '/') {
            pos.* += 2;
            while (pos.* < src.len && src[pos.*] != '\n') {
                pos.* += 1;
            }
            continue;
        }
        if (c == '/' && pos.* + 1 < src.len && src[pos.* + 1] == '*') {
            pos.* += 2;
            while (pos.* + 1 < src.len) {
                if (src[pos.*] == '\n') {
                    line.* += 1;
                    pos.* += 1;
                    continue;
                }
                if (src[pos.*] == '*' && src[pos.* + 1] == '/') {
                    pos.* += 2;
                    break;
                }
                pos.* += 1;
            }
            continue;
        }
        break;
    }
}

fn skipStringLiteral(src: string, pos: *usize, line: *u32, quote: u8) {
    // Assumes src[pos.*] == quote
    if (quote == '\'') {
        // Distinguish single-quoted string/char literals from Rust lifetimes like `'a`
        let scan: usize = pos.* + 1;
        let found_close: bool = false;
        while (scan < src.len && src[scan] != '\n') {
            if (src[scan] == '\\' && scan + 1 < src.len) {
                scan += 2;
                continue;
            }
            if (src[scan] == '\'') {
                found_close = true;
                scan += 1;
                break;
            }
            scan += 1;
        }
        if (found_close) {
            pos.* = scan;
        } else {
            pos.* += 1;
        }
        return;
    }

    pos.* += 1;
    while (pos.* < src.len) {
        const c = src[pos.*];
        if (c == '\\' && pos.* + 1 < src.len) {
            if (src[pos.* + 1] == '\n') {
                line.* += 1;
            }
            pos.* += 2;
            continue;
        }
        if (c == '\n') {
            line.* += 1;
        }
        pos.* += 1;
        if (c == quote) {
            break;
        }
    }
}

fn readQuotedRange(src: string, pos: *usize, line: *u32): [2]usize {
    if (pos.* >= src.len) return [0, 0];
    const q = src[pos.*];
    if (q != '"' && q != '\'') return [0, 0];
    const start = pos.* + 1;
    pos.* += 1;
    while (pos.* < src.len && src[pos.*] != '\n') {
        if (src[pos.*] == '\\' && pos.* + 1 < src.len) {
            pos.* += 2;
            continue;
        }
        if (src[pos.*] == q) {
            const end = pos.*;
            pos.* += 1;
            return [start, end];
        }
        pos.* += 1;
    }
    * = line.*;
    return [start, pos.*];
}

fn extractImportName(src: string, pos: *usize, line: *u32): [2]usize {
    // Scan until ';' or '\n' looking for `from "module"` or first string/identifier
    let scan: usize = pos.*;
    let first_ident_s: usize = 0;
    let first_ident_e: usize = 0;
    let quoted_s: usize = 0;
    let quoted_e: usize = 0;

    while (scan < src.len && src[scan] != ';' && src[scan] != '\n') {
        const c = src[scan];
        if (c == '"' || c == '\'') {
            const q = c;
            const s = scan + 1;
            scan += 1;
            while (scan < src.len && src[scan] != '\n') {
                if (src[scan] == '\\' && scan + 1 < src.len) {
                    scan += 2;
                    continue;
                }
                if (src[scan] == q) {
                    quoted_s = s;
                    quoted_e = scan;
                    scan += 1;
                    break;
                }
                scan += 1;
            }
            continue;
        }
        if (isIdentStart(c) && first_ident_e == 0) {
            const s = scan;
            while (scan < src.len && isIdentContinue(src[scan])) : (scan += 1) {}
            const word = src[s..scan];
            if (!std.mem.eql(u8, word, "from")) {
                first_ident_s = s;
                first_ident_e = scan;
            }
            continue;
        }
        scan += 1;
    }

    pos.* = scan;
    * = line.*;
    if (quoted_e > quoted_s) {
        return [quoted_s, quoted_e];
    }
    return [first_ident_s, first_ident_e];
}

/// Performs a single-pass, zero-heap-allocation scan over `src` to extract source symbols
/// (`fn`/`function`, `class`, `struct`, `enum`, `trait`, `const`, `import`, `test`) along with
/// their 1-based inclusive `[start_line, end_line]` scope range, `brace_depth`, and `is_public`.
/// Ignores `//` line comments, `/* ... */` block comments, and string literals.
/// Writes up to `out_symbols.len` symbols (truncation-safe) and returns the total number of symbols found.
pub fn extractCodeOutline(src: string, out_symbols: *[]CodeSymbolSpan): usize {
    if (src.len == 0) return 0;

    let total_symbols: usize = 0;
    let pos: usize = 0;
    let line: u32 = 1;
    let brace_depth: u16 = 0;

    let at_stmt_start: bool = true;
    let mod_public: bool = false;
    let mod_active: bool = false;

    let awaiting_brace: bool = false;
    let awaiting_slot: usize = NO_SLOT;
    let awaiting_depth: u16 = 0;

    let pending_slots: [MAX_SCOPE_STACK]usize = [NO_SLOT; MAX_SCOPE_STACK];
    let pending_depths: [MAX_SCOPE_STACK]u16 = [0; MAX_SCOPE_STACK];
    let pending_count: usize = 0;

    while (pos < src.len) {
        const c = src[pos];

        // Whitespace
        if (isSpaceOrTab(c)) {
            pos += 1;
            continue;
        }
        if (c == '\n') {
            pos += 1;
            line += 1;
            if (!awaiting_brace && !mod_active) {
                at_stmt_start = true;
            }
            continue;
        }

        // Line comment: // ...
        if (c == '/' && pos + 1 < src.len && src[pos + 1] == '/') {
            pos += 2;
            while (pos < src.len && src[pos] != '\n') {
                pos += 1;
            }
            continue;
        }

        // Block comment: /* ... */
        if (c == '/' && pos + 1 < src.len && src[pos + 1] == '*') {
            pos += 2;
            while (pos + 1 < src.len) {
                if (src[pos] == '\n') {
                    line += 1;
                    pos += 1;
                    continue;
                }
                if (src[pos] == '*' && src[pos + 1] == '/') {
                    pos += 2;
                    break;
                }
                pos += 1;
            }
            continue;
        }

        // String / template literals
        if (c == '"' || c == '\'' || c == '`') {
            skipStringLiteral(src, *pos, *line, c);
            at_stmt_start = false;
            continue;
        }

        // Opening brace: bind pending block symbol if waiting at this depth
        if (c == '{') {
            if (awaiting_brace && brace_depth == awaiting_depth) {
                if (pending_count < MAX_SCOPE_STACK) {
                    pending_slots[pending_count] = awaiting_slot;
                    pending_depths[pending_count] = brace_depth;
                    pending_count += 1;
                }
                awaiting_brace = false;
                awaiting_slot = NO_SLOT;
            }
            brace_depth += 1;
            pos += 1;
            at_stmt_start = true;
            mod_public = false;
            mod_active = false;
            continue;
        }

        // Closing brace: close matching pending symbol's end_line
        if (c == '}') {
            if (brace_depth > 0) {
                brace_depth -= 1;
            }
            if (pending_count > 0 && pending_depths[pending_count - 1] == brace_depth) {
                pending_count -= 1;
                const slot = pending_slots[pending_count];
                if (slot < out_symbols.len) {
                    out_symbols[slot].end_line = line;
                }
            }
            pos += 1;
            at_stmt_start = true;
            mod_public = false;
            mod_active = false;
            continue;
        }

        // Semicolon ends statements and non-block declarations
        if (c == ';') {
            if (awaiting_brace && brace_depth == awaiting_depth) {
                if (awaiting_slot < out_symbols.len) {
                    out_symbols[awaiting_slot].end_line = line;
                }
                awaiting_brace = false;
                awaiting_slot = NO_SLOT;
            }
            pos += 1;
            at_stmt_start = true;
            mod_public = false;
            mod_active = false;
            continue;
        }

        // Identifier / keyword scanning
        if (isIdentStart(c)) {
            const word_start = pos;
            while (pos < src.len && isIdentContinue(src[pos])) : (pos += 1) {}
            const word = src[word_start..pos];

            if (!at_stmt_start) {
                continue;
            }

            // Visibility / storage modifiers
            if (std.mem.eql(u8, word, "pub") || std.mem.eql(u8, word, "export")) {
                mod_public = true;
                mod_active = true;
                continue;
            }
            if (std.mem.eql(u8, word, "static")
                || std.mem.eql(u8, word, "async")
                || std.mem.eql(u8, word, "default")
                || std.mem.eql(u8, word, "declare"))
            {
                mod_active = true;
                continue;
            }

            // Block-scoped named declarations: fn/function, class, struct, enum, trait
            let is_block_decl: bool = false;
            let decl_kind = SymbolKind.Function;
            if (std.mem.eql(u8, word, "fn") || std.mem.eql(u8, word, "function")) {
                is_block_decl = true;
                decl_kind = SymbolKind.Function;
            } else if (std.mem.eql(u8, word, "class")) {
                is_block_decl = true;
                decl_kind = SymbolKind.Class;
            } else if (std.mem.eql(u8, word, "struct")) {
                is_block_decl = true;
                decl_kind = SymbolKind.Struct;
            } else if (std.mem.eql(u8, word, "enum")) {
                is_block_decl = true;
                decl_kind = SymbolKind.Enum;
            } else if (std.mem.eql(u8, word, "trait")) {
                is_block_decl = true;
                decl_kind = SymbolKind.Trait;
            }

            if (is_block_decl) {
                const decl_line = line;
                skipHorizontalAndComments(src, *pos, *line);
                if (pos < src.len && isIdentStart(src[pos])) {
                    const ns = pos;
                    while (pos < src.len && isIdentContinue(src[pos])) : (pos += 1) {}
                    const ne = pos;

                    const slot = if (total_symbols < out_symbols.len) total_symbols else NO_SLOT;
                    if (slot < out_symbols.len) {
                        out_symbols[slot] = CodeSymbolSpan{
                            kind: decl_kind,
                            name_start: ns,
                            name_end: ne,
                            start_line: decl_line,
                            end_line: decl_line,
                            brace_depth: brace_depth,
                            is_public: mod_public,
                        };
                    }
                    total_symbols += 1;
                    awaiting_brace = true;
                    awaiting_slot = slot;
                    awaiting_depth = brace_depth;
                }
                mod_public = false;
                mod_active = false;
                at_stmt_start = false;
                continue;
            }

            // Const declarations (top-level or explicitly modified e.g. pub/static const)
            if (std.mem.eql(u8, word, "const") && (brace_depth == 0 || mod_active)) {
                const decl_line = line;
                skipHorizontalAndComments(src, *pos, *line);
                if (pos < src.len && isIdentStart(src[pos])) {
                    const ns = pos;
                    while (pos < src.len && isIdentContinue(src[pos])) : (pos += 1) {}
                    const ne = pos;

                    const slot = if (total_symbols < out_symbols.len) total_symbols else NO_SLOT;
                    if (slot < out_symbols.len) {
                        out_symbols[slot] = CodeSymbolSpan{
                            kind: SymbolKind.Const,
                            name_start: ns,
                            name_end: ne,
                            start_line: decl_line,
                            end_line: decl_line,
                            brace_depth: brace_depth,
                            is_public: mod_public,
                        };
                    }
                    total_symbols += 1;
                }
                mod_public = false;
                mod_active = false;
                at_stmt_start = false;
                continue;
            }

            // Top-level import declarations
            if (std.mem.eql(u8, word, "import") && brace_depth == 0) {
                const decl_line = line;
                const rng = extractImportName(src, *pos, *line);
                if (rng[1] > rng[0]) {
                    if (total_symbols < out_symbols.len) {
                        out_symbols[total_symbols] = CodeSymbolSpan{
                            kind: SymbolKind.Import,
                            name_start: rng[0],
                            name_end: rng[1],
                            start_line: decl_line,
                            end_line: decl_line,
                            brace_depth: brace_depth,
                            is_public: false,
                        };
                    }
                    total_symbols += 1;
                }
                mod_public = false;
                mod_active = false;
                at_stmt_start = false;
                continue;
            }

            // Test declarations: CP `test("name", fn);` or Zig `test "name" { ... }`
            if (std.mem.eql(u8, word, "test") && brace_depth == 0) {
                const decl_line = line;
                const saved_pos = pos;
                const saved_line = line;
                skipHorizontalAndComments(src, *pos, *line);

                let is_call_test: bool = false;
                if (pos < src.len && src[pos] == '(') {
                    is_call_test = true;
                    pos += 1;
                    skipHorizontalAndComments(src, *pos, *line);
                }

                if (pos < src.len && (src[pos] == '"' || src[pos] == '\'')) {
                    const rng = readQuotedRange(src, *pos, *line);
                    const slot = if (total_symbols < out_symbols.len) total_symbols else NO_SLOT;
                    if (slot < out_symbols.len) {
                        out_symbols[slot] = CodeSymbolSpan{
                            kind: SymbolKind.Test,
                            name_start: rng[0],
                            name_end: rng[1],
                            start_line: decl_line,
                            end_line: decl_line,
                            brace_depth: brace_depth,
                            is_public: false,
                        };
                    }
                    total_symbols += 1;
                    if (!is_call_test) {
                        awaiting_brace = true;
                        awaiting_slot = slot;
                        awaiting_depth = brace_depth;
                    }
                } else {
                    pos = saved_pos;
                    line = saved_line;
                }
                mod_public = false;
                mod_active = false;
                at_stmt_start = false;
                continue;
            }

            // Unrecognized statement starter
            mod_public = false;
            mod_active = false;
            at_stmt_start = false;
            continue;
        }

        // Any other punctuation
        pos += 1;
        at_stmt_start = false;
        mod_public = false;
        mod_active = false;
    }

    return total_symbols;
}

/// Finds the first symbol in `symbols` whose name slice in `src` exactly equals `target`.
/// Returns `null` if `target` is empty or no matching symbol exists.
pub fn findSymbolByName(
    src: string,
    symbols: []const CodeSymbolSpan,
    target: string,
): CodeSymbolSpan? {
    if (target.len == 0) return null;
    let i: usize = 0;
    while (i < symbols.len) : (i += 1) {
        if (std.mem.eql(u8, symbols[i].nameSlice(src), target)) {
            return symbols[i];
        }
    }
    return null;
}

fn symbolKindLabel(k: SymbolKind): string {
    return switch (k) {
        SymbolKind.Function => "fn",
        SymbolKind.Class => "class",
        SymbolKind.Struct => "struct",
        SymbolKind.Enum => "enum",
        SymbolKind.Trait => "trait",
        SymbolKind.Const => "const",
        SymbolKind.Import => "import",
        SymbolKind.Test => "test",
    };
}

fn appendStr(out_buf: *[]u8, pos: *usize, s: string): bool {
    const cur = pos.*;
    if (s.len > out_buf.len - cur) return false;
    let i: usize = 0;
    while (i < s.len) : (i += 1) {
        out_buf[cur + i] = s[i];
    }
    pos.* = cur + s.len;
    return true;
}

fn appendU32(out_buf: *[]u8, pos: *usize, value: u32): bool {
    let digits: [10]u8 = [0; 10];
    let len: usize = 0;
    let v: u32 = value;
    if (v == 0) {
        digits[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            digits[len] = ('0' + (v % 10)) as u8;
            v = v / 10;
            len += 1;
        }
    }

    const cur = pos.*;
    if (len > out_buf.len - cur) return false;
    let i: usize = 0;
    while (i < len) : (i += 1) {
        out_buf[cur + i] = digits[len - 1 - i];
    }
    pos.* = cur + len;
    return true;
}

/// Formats a compact LLM Repo-Map outline line into `out_buf`, e.g.:
/// `pub fn applyUnifiedPatch [L42-L98]` or `struct CodeSymbolSpan [L15-L28]`.
/// Returns the number of bytes written, or `null` if `out_buf` is too small.
pub fn formatRepoMapLine(
    src: string,
    sym: CodeSymbolSpan,
    out_buf: *[]u8,
): usize? {
    let pos: usize = 0;
    if (sym.is_public) {
        if (!appendStr(out_buf, *pos, "pub ")) return null;
    }
    if (!appendStr(out_buf, *pos, symbolKindLabel(sym.kind))) return null;
    if (!appendStr(out_buf, *pos, " ")) return null;
    if (!appendStr(out_buf, *pos, sym.nameSlice(src))) return null;
    if (!appendStr(out_buf, *pos, " [L")) return null;
    if (!appendU32(out_buf, *pos, sym.start_line)) return null;
    if (!appendStr(out_buf, *pos, "-L")) return null;
    if (!appendU32(out_buf, *pos, sym.end_line)) return null;
    if (!appendStr(out_buf, *pos, "]")) return null;
    return pos;
}
