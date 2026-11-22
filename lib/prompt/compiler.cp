// Origin: Pure-CP Stdlib (Compiled Zero-Alloc Prompt Template Bytecode Engine: Vars, Defaults, #if/#else, #each) | Created: 2026-11-22
// 100% Pure CP zero-heap-allocation compiled prompt template engine:
//   - Single-pass lexer/compiler (`compilePromptTemplate`) emitting compact fixed-capacity bytecode instructions:
//     `literal`, `var_lookup` (`{{name}}`, `{{name|fallback}}`, `{{.}}`, `{{@index}}`),
//     `if_jump_false`, `else_jump_end`, `each_begin`, `each_end`
//   - Linear zero-heap VM executor (`renderCompiledTemplate`) with strict/lenient missing-variable modes
//   - Static metadata introspection (`collectRequiredVars`, `collectAllReferencedVars`) without re-parsing source
//   - Full boundary guards: unclosed `{{`, mismatched close tags, nesting overflow, empty template & buffer guard

// ============================================================================
// Constants
// ============================================================================

/// Maximum bytecode instructions emitted per compiled template.
pub const TPL_MAX_INSTRUCTIONS: usize = 128;

/// Maximum block nesting depth for `{{#if}}` and `{{#each}}`.
pub const TPL_MAX_NESTING_DEPTH: usize = 16;

/// Maximum scalar variables stored in `TplRenderContext`.
pub const TPL_MAX_CONTEXT_VARS: usize = 32;

/// Maximum list variables stored in `TplRenderContext`.
pub const TPL_MAX_CONTEXT_LISTS: usize = 16;

/// Maximum items per list variable in `TplRenderContext`.
pub const TPL_MAX_LIST_ITEMS: usize = 32;

/// Maximum byte length captured for missing-variable error diagnostics.
pub const TPL_MAX_MISSING_NAME: usize = 64;

// ============================================================================
// Internal Byte & Slice Helpers
// ============================================================================

fn isWs(b: u8): bool {
    return b == ' ' || b == '\t' || b == '\r' || b == '\n';
}

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn bytesStartsWith(s: []const u8, prefix: []const u8): bool {
    if (prefix.len > s.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (s[i] != prefix[i]) return false;
    }
    return true;
}

fn trimSpanBounds(src: []const u8, start_in: usize, end_in: usize): [2]usize {
    let s: usize = start_in;
    let e: usize = end_in;
    while (s < e && isWs(src[s])) : (s += 1) {}
    while (e > s && isWs(src[e - 1])) : (e -= 1) {}
    return [s, e];
}

fn writeSliceChecked(out_buf: *[]u8, cursor: *usize, bytes: []const u8): bool {
    if (cursor.* + bytes.len > out_buf.len) return false;
    let i: usize = 0;
    while (i < bytes.len) : (i += 1) {
        out_buf[cursor.* + i] = bytes[i];
    }
    cursor.* = cursor.* + bytes.len;
    return true;
}

fn writeUsizeDecimalChecked(out_buf: *[]u8, cursor: *usize, val_in: usize): bool {
    let tmp: [24]u8 = [0; 24];
    let len: usize = 0;
    let v: usize = val_in;
    if (v == 0) {
        tmp[0] = '0';
        len = 1;
    } else {
        while (v > 0) {
            tmp[len] = ('0' as u8) + ((v % 10) as u8);
            v = v / 10;
            len += 1;
        }
    }
    if (cursor.* + len > out_buf.len) return false;
    let i: usize = 0;
    while (i < len) : (i += 1) {
        out_buf[cursor.* + i] = tmp[len - 1 - i];
    }
    cursor.* = cursor.* + len;
    return true;
}

fn inferTruthy(val: []const u8): bool {
    if (val.len == 0) return false;
    if (bytesEq(val, "false") || bytesEq(val, "0") || bytesEq(val, "FALSE")) return false;
    return true;
}

// ============================================================================
// 1. Enums & Bytecode Structs
// ============================================================================

/// Opcode of a compiled prompt template instruction.
pub enum TplOpKind: u8 {
    literal = 0,
    var_lookup = 1,
    if_jump_false = 2,
    else_jump_end = 3,
    each_begin = 4,
    each_end = 5,
}

/// Result status of `compilePromptTemplate(src)`.
pub enum TplCompileStatus: u8 {
    ok = 0,
    unclosed_tag = 1,
    empty_tag = 2,
    mismatched_close_tag = 3,
    unclosed_block = 4,
    nesting_too_deep = 5,
    instruction_overflow = 6,
}

/// Result status of `renderCompiledTemplate(...)`.
pub enum TplExecStatus: u8 {
    ok = 0,
    invalid_template = 1,
    missing_variable = 2,
    buffer_too_small = 3,
    runtime_stack_error = 4,
}

/// Single compact bytecode instruction referencing slices inside `CompiledPromptTemplate.src`.
pub struct TplInstruction {
    op: TplOpKind = TplOpKind.literal;
    span_start: u32 = 0;
    span_end: u32 = 0;
    fallback_start: u32 = 0;
    fallback_end: u32 = 0;
    has_fallback: bool = false;
    jump_target: u16 = 0;
}

/// Compiled zero-allocation prompt template bytecode container.
pub struct CompiledPromptTemplate {
    status: TplCompileStatus = TplCompileStatus.ok;
    error_offset: usize = 0;
    src: []const u8 = "";
    instructions: [TPL_MAX_INSTRUCTIONS]TplInstruction = [TplInstruction{}; TPL_MAX_INSTRUCTIONS];
    inst_count: usize = 0;
    max_depth_seen: usize = 0;

    /// Returns `true` iff compilation succeeded with zero syntax errors.
    fn isValid(): bool {
        return _.status == TplCompileStatus.ok;
    }
}

// ============================================================================
// 2. Render Context & Execution Result
// ============================================================================

struct TplScalarEntry {
    active: bool = false;
    key: []const u8 = "";
    value: []const u8 = "";
    truthy: bool = false;
}

struct TplListEntry {
    active: bool = false;
    key: []const u8 = "";
    items: [TPL_MAX_LIST_ITEMS][]const u8 = [""; TPL_MAX_LIST_ITEMS];
    item_count: usize = 0;
}

/// Zero-heap-allocation render context storing scalar variables, booleans, and string lists.
pub struct TplRenderContext {
    vars: [TPL_MAX_CONTEXT_VARS]TplScalarEntry = [TplScalarEntry{}; TPL_MAX_CONTEXT_VARS];
    var_count: usize = 0;
    lists: [TPL_MAX_CONTEXT_LISTS]TplListEntry = [TplListEntry{}; TPL_MAX_CONTEXT_LISTS];
    list_count: usize = 0;

    /// Creates an empty render context.
    static fn init(): TplRenderContext {
        return TplRenderContext{};
    }

    /// Binds or updates scalar string variable `key = value` (infers truthiness automatically).
    fn setVar(*_, key: []const u8, value: []const u8): bool {
        if (key.len == 0) return false;
        let i: usize = 0;
        while (i < _.var_count) : (i += 1) {
            if (_.vars[i].active && bytesEq(_.vars[i].key, key)) {
                _.vars[i].value = value;
                _.vars[i].truthy = inferTruthy(value);
                return true;
            }
        }
        if (_.var_count >= TPL_MAX_CONTEXT_VARS) return false;
        _.vars[_.var_count] = TplScalarEntry{
            active: true,
            key: key,
            value: value,
            truthy: inferTruthy(value),
        };
        _.var_count += 1;
        return true;
    }

    /// Binds or updates explicit boolean variable `key = flag`.
    fn setBool(*_, key: []const u8, flag: bool): bool {
        if (key.len == 0) return false;
        const text: []const u8 = if (flag) "true" else "false";
        let i: usize = 0;
        while (i < _.var_count) : (i += 1) {
            if (_.vars[i].active && bytesEq(_.vars[i].key, key)) {
                _.vars[i].value = text;
                _.vars[i].truthy = flag;
                return true;
            }
        }
        if (_.var_count >= TPL_MAX_CONTEXT_VARS) return false;
        _.vars[_.var_count] = TplScalarEntry{
            active: true,
            key: key,
            value: text,
            truthy: flag,
        };
        _.var_count += 1;
        return true;
    }

    /// Binds or updates list variable `key = items` (up to `TPL_MAX_LIST_ITEMS = 32` items).
    fn setList(*_, key: []const u8, items: []const []const u8): bool {
        if (key.len == 0 || items.len > TPL_MAX_LIST_ITEMS) return false;
        let target: usize = _.list_count;
        let i: usize = 0;
        while (i < _.list_count) : (i += 1) {
            if (_.lists[i].active && bytesEq(_.lists[i].key, key)) {
                target = i;
                break;
            }
        }
        if (target == _.list_count) {
            if (_.list_count >= TPL_MAX_CONTEXT_LISTS) return false;
            _.list_count += 1;
        }
        let entry = TplListEntry{
            active: true,
            key: key,
            items: [""; TPL_MAX_LIST_ITEMS],
            item_count: items.len,
        };
        let k: usize = 0;
        while (k < items.len) : (k += 1) {
            entry.items[k] = items[k];
        }
        _.lists[target] = entry;
        return true;
    }

    /// Looks up a scalar variable by `key`. Returns `null` if absent.
    fn getVar(key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.var_count) : (i += 1) {
            if (_.vars[i].active && bytesEq(_.vars[i].key, key)) {
                return _.vars[i].value;
            }
        }
        return null;
    }

    /// Evaluates truthiness of `key` for `{{#if key}}` (checks scalar first, then non-empty list).
    fn isTruthy(key: []const u8): bool {
        let i: usize = 0;
        while (i < _.var_count) : (i += 1) {
            if (_.vars[i].active && bytesEq(_.vars[i].key, key)) {
                return _.vars[i].truthy;
            }
        }
        let j: usize = 0;
        while (j < _.list_count) : (j += 1) {
            if (_.lists[j].active && bytesEq(_.lists[j].key, key)) {
                return _.lists[j].item_count > 0;
            }
        }
        return false;
    }

    fn findListIndex(key: []const u8): usize? {
        let j: usize = 0;
        while (j < _.list_count) : (j += 1) {
            if (_.lists[j].active && bytesEq(_.lists[j].key, key)) {
                return j;
            }
        }
        return null;
    }
}

/// Execution result returned by `renderCompiledTemplate(...)`.
pub struct TplExecResult {
    status: TplExecStatus = TplExecStatus.ok;
    bytes_written: usize = 0;
    substitutions: usize = 0;
    missing_var: [TPL_MAX_MISSING_NAME]u8 = [0; TPL_MAX_MISSING_NAME];
    missing_var_len: usize = 0;

    /// Returns the name of the missing variable when `status == missing_variable`.
    fn missingVarSlice(): []const u8 {
        return _.missing_var[0.._.missing_var_len];
    }
}

// ============================================================================
// 3. Single-Pass Template Compiler
// ============================================================================

enum CompileBlockKind: u8 {
    if_block = 0,
    each_block = 1,
}

struct CompileFrame {
    kind: CompileBlockKind = CompileBlockKind.if_block;
    open_inst_idx: usize = 0;
    has_else: bool = false;
    else_inst_idx: usize = 0;
}

/// Compiles prompt template `src` into compact fixed-capacity bytecode in a single pass.
///
/// Supported syntax:
///   - `{{name}}` and `{{name|fallback}}`
///   - `{{#if flag}}...{{else}}...{{/if}}` (arbitrarily nested up to `TPL_MAX_NESTING_DEPTH`)
///   - `{{#each items}}...{{@index}}...{{.}}...{{/each}}`
pub fn compilePromptTemplate(src: []const u8): CompiledPromptTemplate {
    let tpl = CompiledPromptTemplate{
        status: TplCompileStatus.ok,
        error_offset: 0,
        src: src,
        inst_count: 0,
        max_depth_seen: 0,
    };

    if (src.len == 0) return tpl;

    let stack: [TPL_MAX_NESTING_DEPTH]CompileFrame = [CompileFrame{}; TPL_MAX_NESTING_DEPTH];
    let depth: usize = 0;

    let pos: usize = 0;
    let lit_start: usize = 0;

    while (pos < src.len) {
        if (pos + 1 < src.len && src[pos] == '{' && src[pos + 1] == '{') {
            // Flush preceding literal segment
            if (pos > lit_start) {
                if (tpl.inst_count >= TPL_MAX_INSTRUCTIONS) {
                    tpl.status = TplCompileStatus.instruction_overflow;
                    tpl.error_offset = lit_start;
                    return tpl;
                }
                tpl.instructions[tpl.inst_count] = TplInstruction{
                    op: TplOpKind.literal,
                    span_start: lit_start as u32,
                    span_end: pos as u32,
                };
                tpl.inst_count += 1;
            }

            const tag_open_pos: usize = pos;
            const inner_start: usize = pos + 2;
            let close_pos: usize = src.len;
            let scan: usize = inner_start;
            while (scan + 1 < src.len) : (scan += 1) {
                if (src[scan] == '}' && src[scan + 1] == '}') {
                    close_pos = scan;
                    break;
                }
            }
            if (close_pos >= src.len) {
                tpl.status = TplCompileStatus.unclosed_tag;
                tpl.error_offset = tag_open_pos;
                return tpl;
            }

            const trimmed = trimSpanBounds(src, inner_start, close_pos);
            const t_start: usize = trimmed[0];
            const t_end: usize = trimmed[1];
            if (t_start >= t_end) {
                tpl.status = TplCompileStatus.empty_tag;
                tpl.error_offset = tag_open_pos;
                return tpl;
            }

            const tag_text: []const u8 = src[t_start..t_end];

            if (bytesStartsWith(tag_text, "#if") && (tag_text.len == 3 || isWs(tag_text[3]))) {
                const arg_bounds = trimSpanBounds(src, t_start + 3, t_end);
                if (arg_bounds[0] >= arg_bounds[1]) {
                    tpl.status = TplCompileStatus.empty_tag;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                if (depth >= TPL_MAX_NESTING_DEPTH) {
                    tpl.status = TplCompileStatus.nesting_too_deep;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                if (tpl.inst_count >= TPL_MAX_INSTRUCTIONS) {
                    tpl.status = TplCompileStatus.instruction_overflow;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                const inst_idx: usize = tpl.inst_count;
                tpl.instructions[inst_idx] = TplInstruction{
                    op: TplOpKind.if_jump_false,
                    span_start: arg_bounds[0] as u32,
                    span_end: arg_bounds[1] as u32,
                };
                tpl.inst_count += 1;

                stack[depth] = CompileFrame{
                    kind: CompileBlockKind.if_block,
                    open_inst_idx: inst_idx,
                    has_else: false,
                    else_inst_idx: 0,
                };
                depth += 1;
                if (depth > tpl.max_depth_seen) tpl.max_depth_seen = depth;
            } else if (bytesEq(tag_text, "else")) {
                if (depth == 0 || stack[depth - 1].kind != CompileBlockKind.if_block || stack[depth - 1].has_else) {
                    tpl.status = TplCompileStatus.mismatched_close_tag;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                if (tpl.inst_count >= TPL_MAX_INSTRUCTIONS) {
                    tpl.status = TplCompileStatus.instruction_overflow;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                const else_idx: usize = tpl.inst_count;
                tpl.instructions[else_idx] = TplInstruction{
                    op: TplOpKind.else_jump_end,
                };
                tpl.inst_count += 1;

                const if_idx: usize = stack[depth - 1].open_inst_idx;
                tpl.instructions[if_idx].jump_target = (else_idx + 1) as u16;
                stack[depth - 1].has_else = true;
                stack[depth - 1].else_inst_idx = else_idx;
            } else if (bytesEq(tag_text, "/if")) {
                if (depth == 0 || stack[depth - 1].kind != CompileBlockKind.if_block) {
                    tpl.status = TplCompileStatus.mismatched_close_tag;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                depth -= 1;
                const frame = stack[depth];
                if (frame.has_else) {
                    tpl.instructions[frame.else_inst_idx].jump_target = tpl.inst_count as u16;
                } else {
                    tpl.instructions[frame.open_inst_idx].jump_target = tpl.inst_count as u16;
                }
            } else if (bytesStartsWith(tag_text, "#each") && (tag_text.len == 5 || isWs(tag_text[5]))) {
                const arg_bounds = trimSpanBounds(src, t_start + 5, t_end);
                if (arg_bounds[0] >= arg_bounds[1]) {
                    tpl.status = TplCompileStatus.empty_tag;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                if (depth >= TPL_MAX_NESTING_DEPTH) {
                    tpl.status = TplCompileStatus.nesting_too_deep;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                if (tpl.inst_count >= TPL_MAX_INSTRUCTIONS) {
                    tpl.status = TplCompileStatus.instruction_overflow;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                const begin_idx: usize = tpl.inst_count;
                tpl.instructions[begin_idx] = TplInstruction{
                    op: TplOpKind.each_begin,
                    span_start: arg_bounds[0] as u32,
                    span_end: arg_bounds[1] as u32,
                };
                tpl.inst_count += 1;

                stack[depth] = CompileFrame{
                    kind: CompileBlockKind.each_block,
                    open_inst_idx: begin_idx,
                    has_else: false,
                    else_inst_idx: 0,
                };
                depth += 1;
                if (depth > tpl.max_depth_seen) tpl.max_depth_seen = depth;
            } else if (bytesEq(tag_text, "/each")) {
                if (depth == 0 || stack[depth - 1].kind != CompileBlockKind.each_block) {
                    tpl.status = TplCompileStatus.mismatched_close_tag;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                if (tpl.inst_count >= TPL_MAX_INSTRUCTIONS) {
                    tpl.status = TplCompileStatus.instruction_overflow;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                depth -= 1;
                const begin_idx: usize = stack[depth].open_inst_idx;
                const end_idx: usize = tpl.inst_count;
                tpl.instructions[end_idx] = TplInstruction{
                    op: TplOpKind.each_end,
                    jump_target: (begin_idx + 1) as u16,
                };
                tpl.inst_count += 1;
                tpl.instructions[begin_idx].jump_target = tpl.inst_count as u16;
            } else if (tag_text[0] == '/' || tag_text[0] == '#') {
                // Unknown block directive or stray closing tag
                tpl.status = TplCompileStatus.mismatched_close_tag;
                tpl.error_offset = tag_open_pos;
                return tpl;
            } else {
                // Variable lookup: `{{name}}` or `{{name|fallback}}`
                if (tpl.inst_count >= TPL_MAX_INSTRUCTIONS) {
                    tpl.status = TplCompileStatus.instruction_overflow;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                let pipe_pos: usize = t_end;
                let p: usize = t_start;
                while (p < t_end) : (p += 1) {
                    if (src[p] == '|') {
                        pipe_pos = p;
                        break;
                    }
                }
                const name_bounds = trimSpanBounds(src, t_start, pipe_pos);
                if (name_bounds[0] >= name_bounds[1]) {
                    tpl.status = TplCompileStatus.empty_tag;
                    tpl.error_offset = tag_open_pos;
                    return tpl;
                }
                const has_fb: bool = pipe_pos < t_end;
                tpl.instructions[tpl.inst_count] = TplInstruction{
                    op: TplOpKind.var_lookup,
                    span_start: name_bounds[0] as u32,
                    span_end: name_bounds[1] as u32,
                    fallback_start: if (has_fb) ((pipe_pos + 1) as u32) else 0,
                    fallback_end: if (has_fb) (t_end as u32) else 0,
                    has_fallback: has_fb,
                };
                tpl.inst_count += 1;
            }

            pos = close_pos + 2;
            lit_start = pos;
        } else {
            pos += 1;
        }
    }

    // Flush trailing literal segment
    if (lit_start < src.len) {
        if (tpl.inst_count >= TPL_MAX_INSTRUCTIONS) {
            tpl.status = TplCompileStatus.instruction_overflow;
            tpl.error_offset = lit_start;
            return tpl;
        }
        tpl.instructions[tpl.inst_count] = TplInstruction{
            op: TplOpKind.literal,
            span_start: lit_start as u32,
            span_end: src.len as u32,
        };
        tpl.inst_count += 1;
    }

    if (depth > 0) {
        tpl.status = TplCompileStatus.unclosed_block;
        tpl.error_offset = src.len;
        return tpl;
    }

    return tpl;
}

// ============================================================================
// 4. Zero-Heap Bytecode Executor & Static Introspection
// ============================================================================

struct LoopRuntimeFrame {
    list_idx: usize = 0;
    item_idx: usize = 0;
}

/// Renders a compiled prompt template into `out_buf` with zero dynamic heap allocation.
///
/// - When `strict_missing == true`, any unresolved variable without a `|fallback` returns
///   `TplExecStatus.missing_variable` with the variable name populated in `missingVarSlice()`.
/// - When `strict_missing == false`, unresolved variables without fallback emit empty string `""`.
pub fn renderCompiledTemplate(
    tpl: CompiledPromptTemplate,
    ctx: TplRenderContext,
    strict_missing: bool,
    out_buf: *[]u8,
): TplExecResult {
    if (!tpl.isValid()) {
        return TplExecResult{ status: TplExecStatus.invalid_template };
    }

    let loop_stack: [TPL_MAX_NESTING_DEPTH]LoopRuntimeFrame = [LoopRuntimeFrame{}; TPL_MAX_NESTING_DEPTH];
    let loop_depth: usize = 0;

    let cursor: usize = 0;
    let subs: usize = 0;
    let pc: usize = 0;

    while (pc < tpl.inst_count) {
        const inst = tpl.instructions[pc];

        if (inst.op == TplOpKind.literal) {
            const lit: []const u8 = tpl.src[(inst.span_start as usize)..(inst.span_end as usize)];
            if (!writeSliceChecked(out_buf, &cursor, lit)) {
                return TplExecResult{ status: TplExecStatus.buffer_too_small, bytes_written: cursor, substitutions: subs };
            }
            pc += 1;
        } else if (inst.op == TplOpKind.var_lookup) {
            const name: []const u8 = tpl.src[(inst.span_start as usize)..(inst.span_end as usize)];
            if (bytesEq(name, ".")) {
                if (loop_depth > 0) {
                    const top = loop_stack[loop_depth - 1];
                    const item_val: []const u8 = ctx.lists[top.list_idx].items[top.item_idx];
                    if (!writeSliceChecked(out_buf, &cursor, item_val)) {
                        return TplExecResult{ status: TplExecStatus.buffer_too_small, bytes_written: cursor, substitutions: subs };
                    }
                    subs += 1;
                } else if (inst.has_fallback) {
                    const fb: []const u8 = tpl.src[(inst.fallback_start as usize)..(inst.fallback_end as usize)];
                    if (!writeSliceChecked(out_buf, &cursor, fb)) {
                        return TplExecResult{ status: TplExecStatus.buffer_too_small, bytes_written: cursor, substitutions: subs };
                    }
                    subs += 1;
                } else if (strict_missing) {
                    let res = TplExecResult{ status: TplExecStatus.missing_variable, bytes_written: cursor, substitutions: subs };
                    res.missing_var[0] = '.';
                    res.missing_var_len = 1;
                    return res;
                }
            } else if (bytesEq(name, "@index")) {
                if (loop_depth > 0) {
                    const top = loop_stack[loop_depth - 1];
                    if (!writeUsizeDecimalChecked(out_buf, &cursor, top.item_idx)) {
                        return TplExecResult{ status: TplExecStatus.buffer_too_small, bytes_written: cursor, substitutions: subs };
                    }
                    subs += 1;
                } else if (strict_missing) {
                    let res = TplExecResult{ status: TplExecStatus.missing_variable, bytes_written: cursor, substitutions: subs };
                    const lbl: []const u8 = "@index";
                    let k: usize = 0;
                    while (k < lbl.len) : (k += 1) {
                        res.missing_var[k] = lbl[k];
                    }
                    res.missing_var_len = lbl.len;
                    return res;
                }
            } else {
                if (val := ctx.getVar(name)) {
                    if (!writeSliceChecked(out_buf, &cursor, val)) {
                        return TplExecResult{ status: TplExecStatus.buffer_too_small, bytes_written: cursor, substitutions: subs };
                    }
                    subs += 1;
                } else if (inst.has_fallback) {
                    const fb: []const u8 = tpl.src[(inst.fallback_start as usize)..(inst.fallback_end as usize)];
                    if (!writeSliceChecked(out_buf, &cursor, fb)) {
                        return TplExecResult{ status: TplExecStatus.buffer_too_small, bytes_written: cursor, substitutions: subs };
                    }
                    subs += 1;
                } else if (strict_missing) {
                    let res = TplExecResult{ status: TplExecStatus.missing_variable, bytes_written: cursor, substitutions: subs };
                    const copy_len: usize = if (name.len < TPL_MAX_MISSING_NAME) name.len else TPL_MAX_MISSING_NAME;
                    let k: usize = 0;
                    while (k < copy_len) : (k += 1) {
                        res.missing_var[k] = name[k];
                    }
                    res.missing_var_len = copy_len;
                    return res;
                }
            }
            pc += 1;
        } else if (inst.op == TplOpKind.if_jump_false) {
            const cond_name: []const u8 = tpl.src[(inst.span_start as usize)..(inst.span_end as usize)];
            if (ctx.isTruthy(cond_name)) {
                pc += 1;
            } else {
                pc = inst.jump_target as usize;
            }
        } else if (inst.op == TplOpKind.else_jump_end) {
            pc = inst.jump_target as usize;
        } else if (inst.op == TplOpKind.each_begin) {
            const list_name: []const u8 = tpl.src[(inst.span_start as usize)..(inst.span_end as usize)];
            if (l_idx := ctx.findListIndex(list_name)) {
                if (ctx.lists[l_idx].item_count == 0) {
                    pc = inst.jump_target as usize;
                } else {
                    if (loop_depth >= TPL_MAX_NESTING_DEPTH) {
                        return TplExecResult{ status: TplExecStatus.runtime_stack_error, bytes_written: cursor, substitutions: subs };
                    }
                    loop_stack[loop_depth] = LoopRuntimeFrame{
                        list_idx: l_idx,
                        item_idx: 0,
                    };
                    loop_depth += 1;
                    pc += 1;
                }
            } else {
                pc = inst.jump_target as usize;
            }
        } else if (inst.op == TplOpKind.each_end) {
            if (loop_depth == 0) {
                return TplExecResult{ status: TplExecStatus.runtime_stack_error, bytes_written: cursor, substitutions: subs };
            }
            const top_idx: usize = loop_depth - 1;
            const l_idx: usize = loop_stack[top_idx].list_idx;
            const next_item: usize = loop_stack[top_idx].item_idx + 1;
            if (next_item < ctx.lists[l_idx].item_count) {
                loop_stack[top_idx].item_idx = next_item;
                pc = inst.jump_target as usize;
            } else {
                loop_depth -= 1;
                pc += 1;
            }
        } else {
            pc += 1;
        }
    }

    return TplExecResult{
        status: TplExecStatus.ok,
        bytes_written: cursor,
        substitutions: subs,
    };
}

fn collectVarsInternal(
    tpl: CompiledPromptTemplate,
    include_optional: bool,
    out_names: *[][]const u8,
): usize {
    if (!tpl.isValid() || out_names.len == 0) return 0;

    let count: usize = 0;
    let i: usize = 0;
    while (i < tpl.inst_count && count < out_names.len) : (i += 1) {
        const inst = tpl.instructions[i];
        let should_collect: bool = false;
        if (inst.op == TplOpKind.var_lookup) {
            should_collect = include_optional || !inst.has_fallback;
        } else if (inst.op == TplOpKind.if_jump_false || inst.op == TplOpKind.each_begin) {
            should_collect = true;
        }
        if (!should_collect) continue;

        const name: []const u8 = tpl.src[(inst.span_start as usize)..(inst.span_end as usize)];
        if (bytesEq(name, ".") || bytesEq(name, "@index")) continue;

        // Deduplicate against already collected variable names
        let dup: bool = false;
        let j: usize = 0;
        while (j < count) : (j += 1) {
            if (bytesEq(out_names[j], name)) {
                dup = true;
                break;
            }
        }
        if (!dup) {
            out_names[count] = name;
            count += 1;
        }
    }
    return count;
}

/// Statically extracts unique required variable names referenced by `tpl` (excluding `.`, `@index`,
/// and `var_lookup` instructions that specify a `|fallback` default).
pub fn collectRequiredVars(tpl: CompiledPromptTemplate, out_names: *[][]const u8): usize {
    return collectVarsInternal(tpl, false, out_names);
}

/// Statically extracts all unique variable names referenced by `tpl` (including optional variables with fallback).
pub fn collectAllReferencedVars(tpl: CompiledPromptTemplate, out_names: *[][]const u8): usize {
    return collectVarsInternal(tpl, true, out_names);
}
