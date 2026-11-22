// Source: cp/lib/template/engine.cp (Mustache-Compatible Pure CP Template Engine)
// Supports {{var}} (HTML-escaped), {{{raw}}} / {{&raw}}, {{#section}}, {{^inverted}},
// {{! comment }}, {{> partial}} with recursion depth guard, and renderRaw() for AI prompts.

const std = @import("std");
import { TemplateValue, TemplateValueTag, TemplateEntry, TemplateContext } from "./templateValue.cp";

pub const DEFAULT_MAX_DEPTH: usize = 32;

pub err TemplateError {
    UnclosedTag,
    UnclosedSection,
    UnexpectedClosingSection,
    MismatchedSection,
    EmptyTag,
    PartialNotFound,
    MaxDepthExceeded,
    OutOfMemory,
}

pub enum TemplateOpKind: u8 {
    Text = 0,
    VarEscaped = 1,
    VarRaw = 2,
    SectionStart = 3,
    InvertedStart = 4,
    SectionEnd = 5,
    Partial = 6,
}

pub struct TemplateOp {
    pub kind: TemplateOpKind = TemplateOpKind.Text,
    pub text: []const u8 = "",
    pub jump_end: usize = 0,
}

struct StandaloneSpan {
    line_start: usize = 0,
    next_pos: usize = 0,
}

pub struct CompiledTemplate {
    pub allocator: std.mem.Allocator,
    pub ops: std.ArrayList(TemplateOp) = .empty,

    pub fn deinit(*_) {
        _.ops.deinit(_.allocator);
    }

    pub fn opCount(): usize {
        return _.ops.items.len;
    }
}

pub struct TemplateEngine {
    pub allocator: std.mem.Allocator,
    pub partials: std.StringHashMapUnmanaged(string) = .empty,
    pub max_depth: usize = DEFAULT_MAX_DEPTH,
    pub escape_html: bool = true,
    pub allow_missing_partials: bool = false,

    pub static fn init(allocator: std.mem.Allocator): TemplateEngine {
        return TemplateEngine{
            allocator: allocator,
            partials: .empty,
            max_depth: DEFAULT_MAX_DEPTH,
            escape_html: true,
            allow_missing_partials: false,
        };
    }

    pub static fn initDefault(): TemplateEngine {
        return TemplateEngine.init(std.heap.page_allocator);
    }

    pub fn deinit(*_) {
        let iter = _.partials.iterator();
        while (entry := iter.next()) {
            _.allocator.free(entry.key_ptr.*);
            _.allocator.free(entry.value_ptr.*);
        }
        _.partials.deinit(_.allocator);
    }

    pub fn setMaxDepth(*_, depth: usize) {
        _.max_depth = depth;
    }

    pub fn setEscapeHtml(*_, enabled: bool) {
        _.escape_html = enabled;
    }

    pub fn setAllowMissingPartials(*_, allow: bool) {
        _.allow_missing_partials = allow;
    }

    pub fn registerPartial(*_, name: []const u8, src: []const u8): ([]const u8) ! TemplateError {
        const key_trim = std.mem.trim(u8, name, " \t\r\n");
        if (key_trim.len == 0) return TemplateError.EmptyTag;

        const owned_val = _.allocator.dupe(u8, src) catch return TemplateError.OutOfMemory;
        if (entry := _.partials.getEntry(key_trim)) {
            _.allocator.free(entry.value_ptr.*);
            entry.value_ptr.* = owned_val;
            return entry.key_ptr.*;
        }

        const owned_key = _.allocator.dupe(u8, key_trim) catch {
            _.allocator.free(owned_val);
            return TemplateError.OutOfMemory;
        };
        _.partials.put(_.allocator, owned_key, owned_val) catch {
            _.allocator.free(owned_key);
            _.allocator.free(owned_val);
            return TemplateError.OutOfMemory;
        };
        return owned_key;
    }

    pub fn hasPartial(name: []const u8): bool {
        const key_trim = std.mem.trim(u8, name, " \t\r\n");
        return _.partials.contains(key_trim);
    }

    pub fn partialCount(): usize {
        return _.partials.count();
    }

    static fn isHorizWs(c: u8): bool {
        return c == ' ' || c == '\t';
    }

    /// Checks whether a tag span `[tag_start .. tag_end]` sits alone on its line
    /// (surrounded only by horizontal whitespace on the same line, with no prior tags on the line).
    /// Returns the line start offset and the offset after the trailing newline if standalone.
    static fn checkStandalone(
        src: []const u8,
        min_pos: usize,
        tag_start: usize,
        tag_end: usize,
    ): StandaloneSpan? {
        // Scan backwards to start of current line
        let line_start = tag_start;
        while (line_start > 0 && src[line_start - 1] != '\n') {
            line_start -= 1;
        }
        // Another tag already occurred on this line before tag_start
        if (line_start < min_pos) return null;

        let i = line_start;
        while (i < tag_start) : (i += 1) {
            if (!TemplateEngine.isHorizWs(src[i])) return null;
        }

        // Scan forwards from tag_end to end of line
        let j = tag_end;
        while (j < src.len && TemplateEngine.isHorizWs(src[j])) {
            j += 1;
        }
        let after_nl = j;
        if (j < src.len) {
            if (src[j] == '\r' && j + 1 < src.len && src[j + 1] == '\n') {
                after_nl = j + 2;
            } else if (src[j] == '\n') {
                after_nl = j + 1;
            } else {
                return null;
            }
        }
        return StandaloneSpan{ line_start: line_start, next_pos: after_nl };
    }

    pub fn compile(src: []const u8): CompiledTemplate ! TemplateError {
        let ops: std.ArrayList(TemplateOp) = .empty;
        errdefer ops.deinit(_.allocator);

        let open_stack: std.ArrayList(usize) = .empty;
        defer open_stack.deinit(_.allocator);

        let pos: usize = 0;
        while (pos < src.len) {
            const open_opt = std.mem.indexOfPos(u8, src, pos, "{{");
            if (open_opt == null) {
                if (pos < src.len) {
                    ops.append(_.allocator, TemplateOp{
                        kind: TemplateOpKind.Text,
                        text: src[pos..],
                    }) catch return TemplateError.OutOfMemory;
                }
                break;
            }

            const tag_start = open_opt.?;
            const is_triple = (tag_start + 2 < src.len && src[tag_start + 2] == '{');

            if (is_triple) {
                const close_opt = std.mem.indexOfPos(u8, src, tag_start + 3, "}}}");
                if (close_opt == null) return TemplateError.UnclosedTag;
                const close_pos = close_opt.?;
                const inner_raw = src[tag_start + 3 .. close_pos];
                if (std.mem.indexOf(u8, inner_raw, "{{") != null) {
                    return TemplateError.UnclosedTag;
                }
                const key = std.mem.trim(u8, inner_raw, " \t\r\n");
                if (key.len == 0) return TemplateError.EmptyTag;

                if (tag_start > pos) {
                    ops.append(_.allocator, TemplateOp{
                        kind: TemplateOpKind.Text,
                        text: src[pos..tag_start],
                    }) catch return TemplateError.OutOfMemory;
                }
                ops.append(_.allocator, TemplateOp{
                    kind: TemplateOpKind.VarRaw,
                    text: key,
                }) catch return TemplateError.OutOfMemory;
                pos = close_pos + 3;
                continue;
            }

            const close_opt = std.mem.indexOfPos(u8, src, tag_start + 2, "}}");
            if (close_opt == null) return TemplateError.UnclosedTag;
            const close_pos = close_opt.?;
            const tag_end = close_pos + 2;
            const inner_raw = src[tag_start + 2 .. close_pos];
            const inner = std.mem.trim(u8, inner_raw, " \t\r\n");
            if (inner.len == 0) return TemplateError.EmptyTag;

            const sigil = inner[0];
            if (sigil != '!' && std.mem.indexOf(u8, inner_raw, "{{") != null) {
                return TemplateError.UnclosedTag;
            }

            // Determine standalone trimming for section, inverted, closing, and comment tags
            const can_standalone = (sigil == '#' || sigil == '^' || sigil == '/' || sigil == '!');
            let text_end = tag_start;
            let next_pos = tag_end;
            if (can_standalone) {
                if (span := TemplateEngine.checkStandalone(src, pos, tag_start, tag_end)) {
                    text_end = span.line_start;
                    next_pos = span.next_pos;
                }
            }

            if (text_end > pos) {
                ops.append(_.allocator, TemplateOp{
                    kind: TemplateOpKind.Text,
                    text: src[pos..text_end],
                }) catch return TemplateError.OutOfMemory;
            }

            if (sigil == '!') {
                // Comment tag: ignored
                pos = next_pos;
                continue;
            }

            if (sigil == '#' || sigil == '^') {
                const sec_name = std.mem.trim(u8, inner[1..], " \t\r\n");
                if (sec_name.len == 0) return TemplateError.EmptyTag;
                const op_idx = ops.items.len;
                const kind = if (sigil == '#') TemplateOpKind.SectionStart else TemplateOpKind.InvertedStart;
                ops.append(_.allocator, TemplateOp{
                    kind: kind,
                    text: sec_name,
                    jump_end: 0,
                }) catch return TemplateError.OutOfMemory;
                open_stack.append(_.allocator, op_idx) catch return TemplateError.OutOfMemory;
                pos = next_pos;
                continue;
            }

            if (sigil == '/') {
                const sec_name = std.mem.trim(u8, inner[1..], " \t\r\n");
                if (sec_name.len == 0) return TemplateError.EmptyTag;
                if (open_stack.items.len == 0) {
                    return TemplateError.UnexpectedClosingSection;
                }
                const start_idx = open_stack.items[open_stack.items.len - 1];
                open_stack.items.len -= 1;
                if (!std.mem.eql(u8, ops.items[start_idx].text, sec_name)) {
                    return TemplateError.MismatchedSection;
                }
                ops.items[start_idx].jump_end = ops.items.len;
                ops.append(_.allocator, TemplateOp{
                    kind: TemplateOpKind.SectionEnd,
                    text: sec_name,
                }) catch return TemplateError.OutOfMemory;
                pos = next_pos;
                continue;
            }

            if (sigil == '>') {
                const partial_name = std.mem.trim(u8, inner[1..], " \t\r\n");
                if (partial_name.len == 0) return TemplateError.EmptyTag;
                ops.append(_.allocator, TemplateOp{
                    kind: TemplateOpKind.Partial,
                    text: partial_name,
                }) catch return TemplateError.OutOfMemory;
                pos = next_pos;
                continue;
            }

            if (sigil == '&') {
                const var_name = std.mem.trim(u8, inner[1..], " \t\r\n");
                if (var_name.len == 0) return TemplateError.EmptyTag;
                ops.append(_.allocator, TemplateOp{
                    kind: TemplateOpKind.VarRaw,
                    text: var_name,
                }) catch return TemplateError.OutOfMemory;
                pos = next_pos;
                continue;
            }

            ops.append(_.allocator, TemplateOp{
                kind: TemplateOpKind.VarEscaped,
                text: inner,
            }) catch return TemplateError.OutOfMemory;
            pos = next_pos;
        }

        if (open_stack.items.len > 0) {
            return TemplateError.UnclosedSection;
        }

        return CompiledTemplate{
            allocator: _.allocator,
            ops: ops,
        };
    }

    static fn appendHtmlEscaped(
        allocator: std.mem.Allocator,
        out: *std.ArrayList(u8),
        input: []const u8,
    ): bool ! TemplateError {
        let last: usize = 0;
        let i: usize = 0;
        while (i < input.len) : (i += 1) {
            const c = input[i];
            let esc: []const u8 = "";
            if (c == '&') {
                esc = "&amp;";
            } else if (c == '<') {
                esc = "&lt;";
            } else if (c == '>') {
                esc = "&gt;";
            } else if (c == '"') {
                esc = "&quot;";
            } else if (c == '\'') {
                esc = "&#39;";
            } else {
                continue;
            }
            if (i > last) {
                out.appendSlice(allocator, input[last..i]) catch return TemplateError.OutOfMemory;
            }
            out.appendSlice(allocator, esc) catch return TemplateError.OutOfMemory;
            last = i + 1;
        }
        if (last < input.len) {
            out.appendSlice(allocator, input[last..]) catch return TemplateError.OutOfMemory;
        }
        return true;
    }

    static fn appendValueFormatted(
        allocator: std.mem.Allocator,
        out: *std.ArrayList(u8),
        val: TemplateValue,
        escaped: bool,
    ): bool ! TemplateError {
        if (!escaped) {
            val.appendPlain(allocator, out) catch return TemplateError.OutOfMemory;
            return true;
        }
        if (val.tag == TemplateValueTag.Str) {
            * = TemplateEngine.appendHtmlEscaped(allocator, out, val.str_val)?;
            return true;
        }
        val.appendPlain(allocator, out) catch return TemplateError.OutOfMemory;
        return true;
    }

    fn execRange(
        ops: []const TemplateOp,
        start_idx: usize,
        end_idx: usize,
        ctx: TemplateContext,
        out: *std.ArrayList(u8),
        escape_mode: bool,
        depth: usize,
    ): bool ! TemplateError {
        let i = start_idx;
        while (i < end_idx) {
            const op = ops[i];
            switch (op.kind) {
                .Text => {
                    out.appendSlice(_.allocator, op.text) catch return TemplateError.OutOfMemory;
                    i += 1;
                },
                .VarEscaped => {
                    if (val := ctx.resolve(op.text)) {
                        * = TemplateEngine.appendValueFormatted(_.allocator, out, val, escape_mode)?;
                    }
                    i += 1;
                },
                .VarRaw => {
                    if (val := ctx.resolve(op.text)) {
                        * = TemplateEngine.appendValueFormatted(_.allocator, out, val, false)?;
                    }
                    i += 1;
                },
                .SectionStart => {
                    const body_start = i + 1;
                    const body_end = op.jump_end;
                    if (val := ctx.resolve(op.text)) {
                        if (val.isTruthy()) {
                            if (val.tag == TemplateValueTag.List) {
                                for (item of val.list_val) {
                                    const child_ctx = TemplateContext.withParent(item, &ctx);
                                    * = _.execRange(ops, body_start, body_end, child_ctx, out, escape_mode, depth)?;
                                }
                            } else {
                                const child_ctx = TemplateContext.withParent(val, &ctx);
                                * = _.execRange(ops, body_start, body_end, child_ctx, out, escape_mode, depth)?;
                            }
                        }
                    }
                    i = body_end + 1;
                },
                .InvertedStart => {
                    const body_start = i + 1;
                    const body_end = op.jump_end;
                    const val_opt = ctx.resolve(op.text);
                    const should_render = (val_opt == null) || !val_opt.?.isTruthy();
                    if (should_render) {
                        * = _.execRange(ops, body_start, body_end, ctx, out, escape_mode, depth)?;
                    }
                    i = body_end + 1;
                },
                .SectionEnd => {
                    i += 1;
                },
                .Partial => {
                    const partial_opt = _.partials.get(op.text);
                    if (partial_opt == null) {
                        if (_.allow_missing_partials) {
                            i += 1;
                            continue;
                        }
                        return TemplateError.PartialNotFound;
                    }
                    if (depth >= _.max_depth) {
                        return TemplateError.MaxDepthExceeded;
                    }
                    let compiled = _.compile(partial_opt.?)?;
                    defer compiled.deinit();
                    * = _.execRange(compiled.ops.items, 0, compiled.ops.items.len, ctx, out, escape_mode, depth + 1)?;
                    i += 1;
                },
            }
        }
        return true;
    }

    pub fn renderCompiled(tmpl: &CompiledTemplate, ctx: TemplateContext): ([]u8) ! TemplateError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.allocator);
        * = _.execRange(tmpl.ops.items, 0, tmpl.ops.items.len, ctx, &out, _.escape_html, 0)?;
        return out.toOwnedSlice(_.allocator) catch return TemplateError.OutOfMemory;
    }

    pub fn renderCompiledRaw(tmpl: &CompiledTemplate, ctx: TemplateContext): ([]u8) ! TemplateError {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(_.allocator);
        * = _.execRange(tmpl.ops.items, 0, tmpl.ops.items.len, ctx, &out, false, 0)?;
        return out.toOwnedSlice(_.allocator) catch return TemplateError.OutOfMemory;
    }

    /// Compiles and renders `src` with HTML escaping controlled by `escape_html` (default `true`).
    pub fn render(src: []const u8, ctx: TemplateContext): ([]u8) ! TemplateError {
        let compiled = _.compile(src)?;
        defer compiled.deinit();
        return _.renderCompiled(&compiled, ctx);
    }

    /// Compiles and renders `src` with HTML escaping globally disabled (ideal for AI Prompt, Markdown, and Codegen).
    pub fn renderRaw(src: []const u8, ctx: TemplateContext): ([]u8) ! TemplateError {
        let compiled = _.compile(src)?;
        defer compiled.deinit();
        return _.renderCompiledRaw(&compiled, ctx);
    }
}
