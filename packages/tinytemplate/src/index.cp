// @cap/template — Zero-allocation Mustache / Handlebars / MiniJinja-inspired Template Engine
// Clean-room pure-CP implementation (MIT License).
// Features:
// - Default HTML XSS auto-escaping (`& < > " '`) for `{{ key }}`, raw output via `{{{ key }}}`
// - Chained pipeline filters (`upper`, `lower`, `trim`, `len`), e.g. `{{ name | trim | upper }}`
// - Silent comment stripping (`{{! comment }}` and `{{!-- comment --}}`)
// - Nested conditionals (`{{#if key}}...{{else}}...{{/if}}`, `{{#unless key}}...{{else}}...{{/unless}}`)
// - List iteration (`{{#each list}}...{{else}}...{{/each}}`) with `this`, `@index`, `@first`, `@last`
// - Zero heap allocations, `undefined` stack buffers, single-pass scanner, `MAX_BLOCK_DEPTH = 16` guard

const std = @import("std");

pub const MAX_BLOCK_DEPTH: usize = 16;
pub const MAX_VARS: usize = 64;
pub const MAX_LISTS: usize = 16;
pub const MAX_LIST_ITEMS: usize = 128;
pub const INT_POOL_CAPACITY: usize = 512;
const FILTER_BUF_CAPACITY: usize = 2048;

pub err TemplateError {
    BufferTooSmall,
    UnclosedTag,
    UnclosedBlock,
    MismatchedBlock,
    DepthExceeded,
    ContextFull,
    UnknownFilter,
}

struct VarEntry {
    key: []const u8 = "",
    val: []const u8 = "",
    in_pool: bool = false,
    pool_off: u16 = 0,
    pool_len: u16 = 0,
}

struct ListMeta {
    key: []const u8 = "",
    count: usize = 0,
}

struct ListItemEntry {
    list_idx: u8 = 0,
    val: []const u8 = "",
}

enum BlockKind {
    If,
    Unless,
    Each,
}

struct BlockFrame {
    kind: BlockKind = BlockKind.If,
    parent_active: bool = true,
    active: bool = true,
    has_else: bool = false,
    each_body_start: usize = 0,
    each_list_key: []const u8 = "",
    each_index: usize = 0,
    each_len: usize = 0,
}

fn isSpace(c: u8): bool {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

fn trimSlice(s: []const u8): []const u8 {
    let start: usize = 0;
    while (start < s.len && isSpace(s[start])) : (start += 1) {}
    let end: usize = s.len;
    while (end > start && isSpace(s[end - 1])) : (end -= 1) {}
    return s[start..end];
}

fn isValueTruthy(val: []const u8): bool {
    if (val.len == 0) return false;
    if (std.mem.eql(u8, val, "false")) return false;
    if (std.mem.eql(u8, val, "0")) return false;
    return true;
}

fn formatUsize(n: usize, buf: []u8): []const u8 {
    if (n == 0) {
        buf[0] = '0';
        return buf[0..1];
    }
    let tmp: [32]u8 = undefined;
    let len: usize = 0;
    let v = n;
    while (v > 0) {
        tmp[len] = ('0' as u8) + ((v % 10) as u8);
        len += 1;
        v /= 10;
    }
    let i: usize = 0;
    while (i < len) : (i += 1) {
        buf[i] = tmp[len - 1 - i];
    }
    return buf[0..len];
}

fn formatI64(n: i64, buf: []u8): []const u8 {
    if (n == 0) {
        buf[0] = '0';
        return buf[0..1];
    }
    let tmp: [32]u8 = undefined;
    let len: usize = 0;
    let negative = n < 0;
    let mag: u64 = if (negative) @as(u64, 0) -% @as(u64, @bitCast(n)) else @as(u64, @intCast(n));
    while (mag > 0) {
        tmp[len] = ('0' as u8) + ((mag % 10) as u8);
        len += 1;
        mag /= 10;
    }
    let pos: usize = 0;
    if (negative) {
        buf[0] = '-';
        pos = 1;
    }
    let i: usize = 0;
    while (i < len) : (i += 1) {
        buf[pos + i] = tmp[len - 1 - i];
    }
    return buf[0..(pos + len)];
}

fn appendBytes(out: []u8, pos: *usize, data: []const u8): void ! TemplateError {
    if (data.len > out.len - pos.*) return TemplateError.BufferTooSmall;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        out[pos.* + i] = data[i];
    }
    pos.* += data.len;
}

fn appendEscapedHtml(out: []u8, pos: *usize, input: []const u8): void ! TemplateError {
    let i: usize = 0;
    while (i < input.len) : (i += 1) {
        const c = input[i];
        if (c == '&') {
            appendBytes(out, pos, "&amp;")?;
        } else if (c == '<') {
            appendBytes(out, pos, "&lt;")?;
        } else if (c == '>') {
            appendBytes(out, pos, "&gt;")?;
        } else if (c == '"') {
            appendBytes(out, pos, "&quot;")?;
        } else if (c == '\'') {
            appendBytes(out, pos, "&#39;")?;
        } else {
            if (pos.* >= out.len) return TemplateError.BufferTooSmall;
            out[pos.*] = c;
            pos.* += 1;
        }
    }
}

/// Escapes HTML special characters (`&`, `<`, `>`, `"`, `'`) into `out` to prevent XSS.
/// Returns a slice of `out` containing the escaped result, or `TemplateError.BufferTooSmall`.
pub fn escapeHtml(input: []const u8, out: []u8): ([]const u8) ! TemplateError {
    let pos: usize = 0;
    appendEscapedHtml(out, &pos, input)?;
    return out[0..pos];
}

/// Zero-allocation template rendering context supporting scalar strings, booleans,
/// 64-bit integers, dot-separated flat keys (e.g. `"user.name"`), and named lists.
pub struct TemplateContext {
    vars: [64]VarEntry = undefined,
    var_count: usize = 0,
    lists: [16]ListMeta = undefined,
    list_count: usize = 0,
    items: [128]ListItemEntry = undefined,
    item_count: usize = 0,
    int_pool: [512]u8 = undefined,
    int_pool_len: usize = 0,

    /// Creates an empty stack-resident `TemplateContext` with uninitialized backing arrays.
    pub static fn init(): TemplateContext {
        return TemplateContext{
            vars: undefined,
            var_count: 0,
            lists: undefined,
            list_count: 0,
            items: undefined,
            item_count: 0,
            int_pool: undefined,
            int_pool_len: 0,
        };
    }

    /// Resets all stored variables and lists for zero-allocation reuse.
    pub fn reset(*_) {
        _.var_count = 0;
        _.list_count = 0;
        _.item_count = 0;
        _.int_pool_len = 0;
    }

    /// Sets or updates a string variable (supports dotted keys such as `"user.name"`).
    pub fn set(*_, key: []const u8, val: []const u8): void ! TemplateError {
        let i: usize = 0;
        while (i < _.var_count) : (i += 1) {
            if (std.mem.eql(u8, _.vars[i].key, key)) {
                _.vars[i].val = val;
                _.vars[i].in_pool = false;
                _.vars[i].pool_off = 0;
                _.vars[i].pool_len = 0;
                return;
            }
        }
        if (_.var_count >= MAX_VARS) return TemplateError.ContextFull;
        _.vars[_.var_count] = VarEntry{
            key: key,
            val: val,
            in_pool: false,
            pool_off: 0,
            pool_len: 0,
        };
        _.var_count += 1;
    }

    /// Sets or updates a boolean variable (`"true"` or `"false"`).
    pub fn setBool(*_, key: []const u8, b: bool): void ! TemplateError {
        const s: []const u8 = if (b) "true" else "false";
        _.set(key, s)?;
    }

    /// Formats a 64-bit signed integer into the internal pool and binds it to `key`.
    pub fn setInt(*_, key: []const u8, n: i64): void ! TemplateError {
        let num_buf: [32]u8 = undefined;
        const formatted = formatI64(n, num_buf[0..]);
        if (formatted.len > INT_POOL_CAPACITY - _.int_pool_len) {
            return TemplateError.ContextFull;
        }
        const off = _.int_pool_len;
        let k: usize = 0;
        while (k < formatted.len) : (k += 1) {
            _.int_pool[off + k] = formatted[k];
        }

        let i: usize = 0;
        while (i < _.var_count) : (i += 1) {
            if (std.mem.eql(u8, _.vars[i].key, key)) {
                _.int_pool_len += formatted.len;
                _.vars[i].val = "";
                _.vars[i].in_pool = true;
                _.vars[i].pool_off = off as u16;
                _.vars[i].pool_len = formatted.len as u16;
                return;
            }
        }
        if (_.var_count >= MAX_VARS) return TemplateError.ContextFull;
        _.int_pool_len += formatted.len;
        _.vars[_.var_count] = VarEntry{
            key: key,
            val: "",
            in_pool: true,
            pool_off: off as u16,
            pool_len: formatted.len as u16,
        };
        _.var_count += 1;
    }

    /// Looks up a scalar variable by key (returns `null` if undefined).
    pub fn get(key: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.var_count) : (i += 1) {
            if (std.mem.eql(u8, _.vars[i].key, key)) {
                if (_.vars[i].in_pool) {
                    const start = _.vars[i].pool_off as usize;
                    const end = start + (_.vars[i].pool_len as usize);
                    return _.int_pool[start..end];
                }
                return _.vars[i].val;
            }
        }
        return null;
    }

    /// Returns true if the scalar key exists in the context.
    pub fn has(key: []const u8): bool {
        return _.get(key) != null;
    }

    /// Returns the number of scalar variables currently stored.
    pub fn varCount(): usize {
        return _.var_count;
    }

    /// Appends an item to the named list (creating the list if it does not yet exist).
    pub fn pushListItem(*_, list_key: []const u8, item: []const u8): void ! TemplateError {
        if (_.item_count >= MAX_LIST_ITEMS) return TemplateError.ContextFull;
        let found_idx: usize? = null;
        let i: usize = 0;
        while (i < _.list_count) : (i += 1) {
            if (std.mem.eql(u8, _.lists[i].key, list_key)) {
                found_idx = i;
                break;
            }
        }
        let target_idx: usize = 0;
        if (found_idx != null) {
            target_idx = found_idx.?;
        } else {
            if (_.list_count >= MAX_LISTS) return TemplateError.ContextFull;
            target_idx = _.list_count;
            _.lists[target_idx] = ListMeta{
                key: list_key,
                count: 0,
            };
            _.list_count += 1;
        }
        _.items[_.item_count] = ListItemEntry{
            list_idx: target_idx as u8,
            val: item,
        };
        _.item_count += 1;
        _.lists[target_idx].count += 1;
    }

    /// Returns the number of items in the named list (0 if undefined).
    pub fn listLen(list_key: []const u8): usize {
        let i: usize = 0;
        while (i < _.list_count) : (i += 1) {
            if (std.mem.eql(u8, _.lists[i].key, list_key)) {
                return _.lists[i].count;
            }
        }
        return 0;
    }

    /// Returns the `idx`-th item of the named list, or `null` if out of range.
    pub fn listItem(list_key: []const u8, idx: usize): ([]const u8)? {
        let list_idx: usize? = null;
        let i: usize = 0;
        while (i < _.list_count) : (i += 1) {
            if (std.mem.eql(u8, _.lists[i].key, list_key)) {
                list_idx = i;
                break;
            }
        }
        if (list_idx == null) return null;
        const target = list_idx.? as u8;
        let seen: usize = 0;
        let j: usize = 0;
        while (j < _.item_count) : (j += 1) {
            if (_.items[j].list_idx == target) {
                if (seen == idx) return _.items[j].val;
                seen += 1;
            }
        }
        return null;
    }

    /// Evaluates truthiness of `key`:
    /// empty string, `"false"`, `"0"`, empty list, or undefined are falsy; all others are truthy.
    pub fn isTruthy(key: []const u8): bool {
        const val = _.get(key);
        if (val != null) {
            return isValueTruthy(val.?);
        }
        return _.listLen(key) > 0;
    }
}

fn findActiveEach(stack: *const [16]BlockFrame, depth: usize): usize? {
    let i = depth;
    while (i > 0) {
        i -= 1;
        if (stack.*[i].kind == BlockKind.Each && stack.*[i].active) {
            return i;
        }
    }
    return null;
}

fn resolveValue(
    key: []const u8,
    ctx: *const TemplateContext,
    stack: *const [16]BlockFrame,
    depth: usize,
    index_buf: []u8,
): []const u8 {
    const each_idx = findActiveEach(stack, depth);
    if (each_idx != null) {
        const frame = stack.*[each_idx.?];
        if (std.mem.eql(u8, key, "this") || std.mem.eql(u8, key, ".")) {
            return ctx.listItem(frame.each_list_key, frame.each_index) ?? "";
        }
        if (std.mem.eql(u8, key, "@index")) {
            return formatUsize(frame.each_index, index_buf);
        }
        if (std.mem.eql(u8, key, "@first")) {
            return if (frame.each_index == 0) "true" else "false";
        }
        if (std.mem.eql(u8, key, "@last")) {
            return if (frame.each_index + 1 == frame.each_len) "true" else "false";
        }
    }
    return ctx.get(key) ?? "";
}

fn resolveCondition(
    key: []const u8,
    ctx: *const TemplateContext,
    stack: *const [16]BlockFrame,
    depth: usize,
): bool {
    const each_idx = findActiveEach(stack, depth);
    if (each_idx != null) {
        const frame = stack.*[each_idx.?];
        if (std.mem.eql(u8, key, "this") || std.mem.eql(u8, key, ".")) {
            const v = ctx.listItem(frame.each_list_key, frame.each_index) ?? "";
            return isValueTruthy(v);
        }
        if (std.mem.eql(u8, key, "@index")) {
            return frame.each_index != 0;
        }
        if (std.mem.eql(u8, key, "@first")) {
            return frame.each_index == 0;
        }
        if (std.mem.eql(u8, key, "@last")) {
            return frame.each_index + 1 == frame.each_len;
        }
    }
    return ctx.isTruthy(key);
}

fn applyFilter(
    filter_name: []const u8,
    input: []const u8,
    dst: []u8,
): ([]const u8) ! TemplateError {
    if (std.mem.eql(u8, filter_name, "trim")) {
        return trimSlice(input);
    }
    if (std.mem.eql(u8, filter_name, "upper")) {
        if (input.len > dst.len) return TemplateError.BufferTooSmall;
        let i: usize = 0;
        while (i < input.len) : (i += 1) {
            const c = input[i];
            dst[i] = if (c >= 'a' && c <= 'z') c - 32 else c;
        }
        return dst[0..input.len];
    }
    if (std.mem.eql(u8, filter_name, "lower")) {
        if (input.len > dst.len) return TemplateError.BufferTooSmall;
        let i: usize = 0;
        while (i < input.len) : (i += 1) {
            const c = input[i];
            dst[i] = if (c >= 'A' && c <= 'Z') c + 32 else c;
        }
        return dst[0..input.len];
    }
    if (std.mem.eql(u8, filter_name, "len")) {
        return formatUsize(input.len, dst);
    }
    return TemplateError.UnknownFilter;
}

fn evalExpressionAndEmit(
    expr: []const u8,
    raw_output: bool,
    ctx: *const TemplateContext,
    stack: *const [16]BlockFrame,
    depth: usize,
    out: []u8,
    out_pos: *usize,
): void ! TemplateError {
    // Split expression by '|' into key and optional chained filters
    let pipe_pos: usize? = null;
    let i: usize = 0;
    while (i < expr.len) : (i += 1) {
        if (expr[i] == '|') {
            pipe_pos = i;
            break;
        }
    }

    const key = if (pipe_pos != null) trimSlice(expr[0..pipe_pos.?]) else expr;
    let index_buf: [32]u8 = undefined;
    let current = resolveValue(key, ctx, stack, depth, index_buf[0..]);

    if (pipe_pos != null) {
        let buf_a: [2048]u8 = undefined;
        let buf_b: [2048]u8 = undefined;
        let use_a = true;
        let cursor = pipe_pos.? + 1;
        while (cursor <= expr.len) {
            let next_pipe = expr.len;
            let k = cursor;
            while (k < expr.len) : (k += 1) {
                if (expr[k] == '|') {
                    next_pipe = k;
                    break;
                }
            }
            const fname = trimSlice(expr[cursor..next_pipe]);
            if (fname.len == 0) return TemplateError.UnknownFilter;
            const dst: []u8 = if (use_a) buf_a[0..] else buf_b[0..];
            current = applyFilter(fname, current, dst)?;
            use_a = !use_a;
            if (next_pipe >= expr.len) break;
            cursor = next_pipe + 1;
        }
    }

    if (raw_output) {
        appendBytes(out, out_pos, current)?;
    } else {
        appendEscapedHtml(out, out_pos, current)?;
    }
}

fn findSubstring(haystack: []const u8, start: usize, needle: []const u8): usize? {
    if (needle.len == 0) return start;
    if (start + needle.len > haystack.len) return null;
    let i = start;
    const limit = haystack.len - needle.len;
    while (i <= limit) : (i += 1) {
        if (std.mem.eql(u8, haystack[i..(i + needle.len)], needle)) {
            return i;
        }
    }
    return null;
}

fn isBranchActive(stack: *const [16]BlockFrame, depth: usize): bool {
    if (depth == 0) return true;
    return stack.*[depth - 1].active;
}

/// Renders a template string `tpl` using `ctx` into the caller-provided stack buffer `out`.
/// Performs zero heap allocations and auto-escapes HTML in `{{ key }}` expressions by default.
pub fn render(
    tpl: []const u8,
    ctx: *const TemplateContext,
    out: []u8,
): ([]const u8) ! TemplateError {
    let stack: [16]BlockFrame = undefined;
    let depth: usize = 0;
    let out_pos: usize = 0;
    let pos: usize = 0;

    while (pos < tpl.len) {
        const open_opt = findSubstring(tpl, pos, "{{");
        if (open_opt == null) {
            if (isBranchActive(&stack, depth)) {
                appendBytes(out, &out_pos, tpl[pos..tpl.len])?;
            }
            break;
        }

        const open = open_opt.?;
        if (open > pos && isBranchActive(&stack, depth)) {
            appendBytes(out, &out_pos, tpl[pos..open])?;
        }

        // 1. Triple-brace raw output: {{{ expr }}}
        if (open + 2 < tpl.len && tpl[open + 2] == '{') {
            const close_opt = findSubstring(tpl, open + 3, "}}}");
            if (close_opt == null) return TemplateError.UnclosedTag;
            const close = close_opt.?;
            const expr = trimSlice(tpl[(open + 3)..close]);
            if (isBranchActive(&stack, depth) && expr.len > 0) {
                evalExpressionAndEmit(expr, true, ctx, &stack, depth, out, &out_pos)?;
            }
            pos = close + 3;
            continue;
        }

        // 2. Comment block: {{!-- ... --}} or {{! ... }}
        if (open + 2 < tpl.len && tpl[open + 2] == '!') {
            if (open + 4 < tpl.len && tpl[open + 3] == '-' && tpl[open + 4] == '-') {
                const close_opt = findSubstring(tpl, open + 5, "--}}");
                if (close_opt == null) return TemplateError.UnclosedTag;
                pos = close_opt.? + 4;
            } else {
                const close_opt = findSubstring(tpl, open + 3, "}}");
                if (close_opt == null) return TemplateError.UnclosedTag;
                pos = close_opt.? + 2;
            }
            continue;
        }

        // 3. Standard double-brace tag: {{ ... }}
        const close_opt = findSubstring(tpl, open + 2, "}}");
        if (close_opt == null) return TemplateError.UnclosedTag;
        const close = close_opt.?;
        const tag = trimSlice(tpl[(open + 2)..close]);
        const next_pos = close + 2;

        if (tag.len == 0) {
            pos = next_pos;
            continue;
        }

        // 3a. Block opening: {{#if ...}}, {{#unless ...}}, {{#each ...}}
        if (tag[0] == '#') {
            if (depth >= MAX_BLOCK_DEPTH) return TemplateError.DepthExceeded;
            const parent_active = isBranchActive(&stack, depth);
            const rest = tag[1..tag.len];

            if (rest.len >= 3 && std.mem.eql(u8, rest[0..2], "if") && isSpace(rest[2])) {
                const key = trimSlice(rest[3..rest.len]);
                const cond = if (parent_active) resolveCondition(key, ctx, &stack, depth) else false;
                stack[depth] = BlockFrame{
                    kind: BlockKind.If,
                    parent_active: parent_active,
                    active: parent_active && cond,
                    has_else: false,
                    each_body_start: 0,
                    each_list_key: "",
                    each_index: 0,
                    each_len: 0,
                };
                depth += 1;
                pos = next_pos;
                continue;
            }

            if (rest.len >= 7 && std.mem.eql(u8, rest[0..6], "unless") && isSpace(rest[6])) {
                const key = trimSlice(rest[7..rest.len]);
                const cond = if (parent_active) resolveCondition(key, ctx, &stack, depth) else true;
                stack[depth] = BlockFrame{
                    kind: BlockKind.Unless,
                    parent_active: parent_active,
                    active: parent_active && !cond,
                    has_else: false,
                    each_body_start: 0,
                    each_list_key: "",
                    each_index: 0,
                    each_len: 0,
                };
                depth += 1;
                pos = next_pos;
                continue;
            }

            if (rest.len >= 5 && std.mem.eql(u8, rest[0..4], "each") && isSpace(rest[4])) {
                const list_key = trimSlice(rest[5..rest.len]);
                const list_len = if (parent_active) ctx.listLen(list_key) else 0;
                stack[depth] = BlockFrame{
                    kind: BlockKind.Each,
                    parent_active: parent_active,
                    active: parent_active && (list_len > 0),
                    has_else: false,
                    each_body_start: next_pos,
                    each_list_key: list_key,
                    each_index: 0,
                    each_len: list_len,
                };
                depth += 1;
                pos = next_pos;
                continue;
            }

            return TemplateError.MismatchedBlock;
        }

        // 3b. Block closing: {{/if}}, {{/unless}}, {{/each}}
        if (tag[0] == '/') {
            if (depth == 0) return TemplateError.MismatchedBlock;
            const end_name = trimSlice(tag[1..tag.len]);
            let expected_kind = BlockKind.If;
            if (std.mem.eql(u8, end_name, "if")) {
                expected_kind = BlockKind.If;
            } else if (std.mem.eql(u8, end_name, "unless")) {
                expected_kind = BlockKind.Unless;
            } else if (std.mem.eql(u8, end_name, "each")) {
                expected_kind = BlockKind.Each;
            } else {
                return TemplateError.MismatchedBlock;
            }

            if (stack[depth - 1].kind != expected_kind) return TemplateError.MismatchedBlock;

            if (stack[depth - 1].kind == BlockKind.Each && !stack[depth - 1].has_else && stack[depth - 1].parent_active && stack[depth - 1].each_len > 0 && stack[depth - 1].each_index + 1 < stack[depth - 1].each_len) {
                stack[depth - 1].each_index += 1;
                pos = stack[depth - 1].each_body_start;
                continue;
            }

            depth -= 1;
            pos = next_pos;
            continue;
        }

        // 3c. Else branch: {{else}}
        if (std.mem.eql(u8, tag, "else")) {
            if (depth == 0) return TemplateError.MismatchedBlock;
            if (stack[depth - 1].has_else) return TemplateError.MismatchedBlock;

            if (stack[depth - 1].kind == BlockKind.Each) {
                if (stack[depth - 1].parent_active && stack[depth - 1].each_len > 0 && stack[depth - 1].each_index + 1 < stack[depth - 1].each_len) {
                    stack[depth - 1].each_index += 1;
                    pos = stack[depth - 1].each_body_start;
                    continue;
                }
                stack[depth - 1].has_else = true;
                stack[depth - 1].active = stack[depth - 1].parent_active && (stack[depth - 1].each_len == 0);
            } else {
                stack[depth - 1].has_else = true;
                stack[depth - 1].active = stack[depth - 1].parent_active && !stack[depth - 1].active;
            }
            pos = next_pos;
            continue;
        }

        // 3d. Variable interpolation & chained pipeline filters (auto HTML-escaped)
        if (isBranchActive(&stack, depth)) {
            evalExpressionAndEmit(tag, false, ctx, &stack, depth, out, &out_pos)?;
        }
        pos = next_pos;
    }

    if (depth != 0) return TemplateError.UnclosedBlock;
    return out[0..out_pos];
}
