// Source: Absorbed from /data/dev/bun/src/css/printer.cp | Synced: 2026-11-22T01:25:24Z
// lib/css/engine/compiler.cp
// LightningCSS Full Compilation Pipeline:
// CssCompileOptions, CssCompileStats, CssModulesHasher & CssCompiler (Nesting + Media Ranges + Prefixing + Logical + Minify/Pretty + CSS Modules)

const std = @import("std");
import { CssExpandedDecl, CssLogical } from "../prefixes.cp";
import { CssNesting } from "../stylesheet.cp";
import { Targets, Feature, PrefixFeature } from "./targets.cp";
import { Time, Gradient } from "./values.cp";
import { RectShorthand, BorderRadius, FlexShorthand, DisplayValue } from "./properties.cp";
import { MediaRangeQuery } from "./rules.cp";
import { engAppendBytes, engTrimRight } from "./_utils.cp";

// ============================================================================
// 1. Options, Stats & CSS Modules Hasher
// ============================================================================

pub struct CssCompileOptions {
    targets: Targets = Targets.browserDefault();
    minify: bool = false;
    lower_nesting: bool = true;
    lower_media_ranges: bool = true;
    add_vendor_prefixes: bool = true;
    expand_logical: bool = true;
    minify_properties: bool = true;
    css_modules: bool = false;
    module_hash_seed: []const u8 = "style.module.css";
    unused_symbols: [32][]const u8 = [""; 32];
    unused_symbol_count: usize = 0;

    pub fn addUnusedSymbol(*_, sym: []const u8) {
        if (_.unused_symbol_count < 32) {
            _.unused_symbols[_.unused_symbol_count] = sym;
            _.unused_symbol_count += 1;
        }
    }

    pub fn isUnusedSymbol(sym: []const u8): bool {
        let i: usize = 0;
        while (i < _.unused_symbol_count) {
            if (std.mem.eql(u8, _.unused_symbols[i], sym)) return true;
            i += 1;
        }
        return false;
    }
}

pub struct CssCompileStats {
    rules_in: usize = 0;
    rules_out: usize = 0;
    declarations_processed: usize = 0;
    prefixes_injected: usize = 0;
    logical_expanded: usize = 0;
    nesting_flattened: usize = 0;
    media_ranges_lowered: usize = 0;
    shorthands_minified: usize = 0;
    unused_rules_dropped: usize = 0;
    scoped_selectors: usize = 0;
}

pub struct CssModulesHasher {
    /// Generates a deterministic LightningCSS-style URL-safe short hash from `seed` (6 chars, prefixed with `_` if leading digit).
    pub static fn hashSeed(seed: []const u8, out_buf: []u8): []const u8 {
        const alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_-";
        let h: u64 = (@as(u64, 0xcbf29ce4) << 32) | @as(u64, 0x84222325);
        for (b of seed) {
            h ^= @as(u64, b);
            h = h *% 1099511628211;
        }
        h ^= h >> 33;
        h = h *% ((@as(u64, 0xff51afd7) << 32) | @as(u64, 0xed558ccd));
        h ^= h >> 33;

        let raw: [6]u8 = [0; 6];
        let i: usize = 0;
        while (i < 6) {
            const shift: u6 = @as(u6, @intCast(i * 6));
            const idx: usize = @as(usize, @intCast((h >> shift) & 0x3F));
            raw[i] = alphabet[idx];
            i += 1;
        }
        if (raw[0] >= '0' && raw[0] <= '9') {
            return std.fmt.bufPrint(out_buf, "_{s}", .{raw[0..5]}) catch "_css00";
        }
        return std.fmt.bufPrint(out_buf, "{s}", .{raw[0..6]}) catch "css000";
    }

    /// Formats a CSS Module scoped identifier `[local]_[hash]`.
    pub static fn scopeIdent(local: []const u8, file_seed: []const u8, out_buf: []u8): []const u8 {
        let hb: [16]u8 = [0; 16];
        const h = CssModulesHasher.hashSeed(file_seed, hb[0..16]);
        return std.fmt.bufPrint(out_buf, "{s}_{s}", .{ local, h }) catch local;
    }

    /// Rewrites `.class` tokens in a selector prelude into `.class_<hash>`, preserving `:global(...)`.
    pub static fn scopeSelector(selector: []const u8, file_seed: []const u8, out_buf: []u8): []const u8 {
        let hb: [16]u8 = [0; 16];
        const h = CssModulesHasher.hashSeed(file_seed, hb[0..16]);
        let pos: usize = 0;
        let i: usize = 0;
        let global_depth: usize = 0;

        while (i < selector.len) {
            if (i + 8 <= selector.len && std.ascii.startsWithIgnoreCase(selector[i..], ":global(")) {
                global_depth += 1;
                i += 8;
                continue;
            }
            if (global_depth > 0 && selector[i] == ')') {
                global_depth -= 1;
                i += 1;
                continue;
            }
            if (selector[i] == '.' && global_depth == 0) {
                if (pos < out_buf.len) { out_buf[pos] = '.'; pos += 1; }
                i += 1;
                const start = i;
                while (i < selector.len && (std.ascii.isAlphanumeric(selector[i]) || selector[i] == '-' || selector[i] == '_')) {
                    if (pos < out_buf.len) { out_buf[pos] = selector[i]; pos += 1; }
                    i += 1;
                }
                if (i > start) {
                    pos = engAppendBytes(out_buf, pos, "_");
                    pos = engAppendBytes(out_buf, pos, h);
                }
                continue;
            }
            if (pos < out_buf.len) { out_buf[pos] = selector[i]; pos += 1; }
            i += 1;
        }
        return out_buf[0..pos];
    }
}

// ============================================================================
// 2. Helpers for Declaration & Prelude Lowering
// ============================================================================

fn minifyHexColorInValue(val: []const u8, out_buf: []u8): usize {
    // Shortens `#rrggbb` -> `#rgb` when pairs match; returns written length or 0 if unchanged
    if (val.len == 7 && val[0] == '#' && out_buf.len >= 4) {
        if (std.ascii.toLower(val[1]) == std.ascii.toLower(val[2]) &&
            std.ascii.toLower(val[3]) == std.ascii.toLower(val[4]) &&
            std.ascii.toLower(val[5]) == std.ascii.toLower(val[6]))
        {
            out_buf[0] = '#';
            out_buf[1] = std.ascii.toLower(val[1]);
            out_buf[2] = std.ascii.toLower(val[3]);
            out_buf[3] = std.ascii.toLower(val[5]);
            return 4;
        }
    }
    return 0;
}

fn minifyZeroUnit(val: []const u8): []const u8 {
    if (std.ascii.eqlIgnoreCase(val, "0px") ||
        std.ascii.eqlIgnoreCase(val, "0em") ||
        std.ascii.eqlIgnoreCase(val, "0rem") ||
        std.ascii.eqlIgnoreCase(val, "0pt") ||
        std.ascii.eqlIgnoreCase(val, "0vw") ||
        std.ascii.eqlIgnoreCase(val, "0vh"))
    {
        return "0";
    }
    return val;
}

fn lowerMediaPrelude(prelude: []const u8, opts: CssCompileOptions, out_buf: []u8): usize {
    const need_range = opts.lower_media_ranges &&
        (opts.targets.shouldCompile(Feature.MediaRangeSyntax) || opts.targets.shouldCompile(Feature.MediaIntervalSyntax));
    if (!need_range) return 0;

    let pos: usize = 0;
    let i: usize = 0;
    let lowered_here = false;

    while (i < prelude.len) {
        if (prelude[i] == '(') {
            let depth: usize = 0;
            let j = i;
            while (j < prelude.len) {
                if (prelude[j] == '(') depth += 1;
                else if (prelude[j] == ')') {
                    depth -= 1;
                    if (depth == 0) break;
                }
                j += 1;
            }
            if (j < prelude.len) {
                const paren_chunk = prelude[i .. j + 1];
                if (std.mem.indexOfAny(u8, paren_chunk, "<>=") != null) {
                    if (mrq := MediaRangeQuery.parse(paren_chunk)) {
                        let lb: [128]u8 = [0; 128];
                        pos = engAppendBytes(out_buf, pos, mrq.lowerToLegacy(lb[0..128]));
                        lowered_here = true;
                        i = j + 1;
                        continue;
                    }
                }
                pos = engAppendBytes(out_buf, pos, paren_chunk);
                i = j + 1;
                continue;
            }
        }
        if (pos < out_buf.len) { out_buf[pos] = prelude[i]; pos += 1; }
        i += 1;
    }
    if (lowered_here) return pos;
    return 0;
}

fn shouldDropSelectorForUnused(prelude: []const u8, opts: CssCompileOptions): bool {
    if (opts.unused_symbol_count == 0) return false;
    const s = std.mem.trim(u8, prelude, " \t\r\n");
    if (s.len < 2 || s[0] != '.') return false;
    // Only drop simple single class/id selectors that exactly match an unused symbol
    let end: usize = 1;
    while (end < s.len && (std.ascii.isAlphanumeric(s[end]) || s[end] == '-' || s[end] == '_')) {
        end += 1;
    }
    if (end != s.len) return false;
    return opts.isUnusedSymbol(s[1..]);
}

// ============================================================================
// 3. CssCompiler — Full LightningCSS Compilation Pipeline
// ============================================================================

pub struct CssCompiler {
    /// Compiles a CSS stylesheet with LightningCSS passes:
    /// Nesting lowering, unused symbol pruning, CSS Modules scoping, Media Range L4 lowering,
    /// logical property expansion, shorthand minification, target vendor prefixing, and formatting.
    pub static fn compile(
        css_text: []const u8,
        opts: CssCompileOptions,
        out_buf: []u8,
        stats_out: *CssCompileStats?,
    ): []const u8 {
        let stats = CssCompileStats {};

        // Stage 1: Nesting lowering (if enabled & target lacks native nesting)
        let working_buf: [16384]u8 = [0; 16384];
        let src = std.mem.trim(u8, css_text, " \t\r\n");
        if (opts.lower_nesting && opts.targets.shouldCompile(Feature.Nesting)) {
            const flat = CssNesting.flatten(src, working_buf[0..16384]);
            if (flat.len > 0 && !std.mem.eql(u8, flat, src)) {
                stats.nesting_flattened += 1;
            }
            if (flat.len > 0) src = flat;
        }

        // Stage 2: Walk top-level rules & blocks
        let pos: usize = 0;
        pos = CssCompiler.compileBlock(src, opts, out_buf, pos, 0, &stats);

        // Trim trailing whitespace/newlines
        const final_slice = engTrimRight(out_buf[0..pos], " \t\r\n");
        if (sp := stats_out) {
            sp.* = stats;
        }
        return final_slice;
    }

    fn compileBlock(
        src: []const u8,
        opts: CssCompileOptions,
        out_buf: []u8,
        pos_in: usize,
        depth: usize,
        stats: *CssCompileStats,
    ): usize {
        if (depth > 6) return pos_in;
        let pos = pos_in;
        let i: usize = 0;
        let seg_start: usize = 0;
        let brace_depth: usize = 0;
        let in_q: u8 = 0;

        while (i < src.len) {
            const c = src[i];
            if (in_q != 0) {
                if (c == '\\' && i + 1 < src.len) { i += 2; continue; }
                if (c == in_q) in_q = 0;
                i += 1;
                continue;
            }
            if (i + 1 < src.len && c == '/' && src[i + 1] == '*') {
                const leading_ws = std.mem.trim(u8, src[seg_start..i], " \t\r\n").len == 0;
                i += 2;
                while (i + 1 < src.len && !(src[i] == '*' && src[i + 1] == '/')) i += 1;
                if (i + 1 < src.len) i += 2 else i = src.len;
                if (leading_ws && brace_depth == 0) seg_start = i;
                continue;
            }
            if (c == '"' || c == '\'') { in_q = c; i += 1; continue; }

            if (c == '{') {
                brace_depth += 1;
            } else if (c == '}') {
                if (brace_depth > 0) {
                    brace_depth -= 1;
                    if (brace_depth == 0) {
                        const rule_chunk = std.mem.trim(u8, src[seg_start .. i + 1], " \t\r\n");
                        stats.rules_in += 1;
                        pos = CssCompiler.emitRule(rule_chunk, opts, out_buf, pos, depth, stats);
                        seg_start = i + 1;
                    }
                }
            } else if (c == ';' && brace_depth == 0) {
                const stmt = std.mem.trim(u8, src[seg_start .. i + 1], " \t\r\n");
                if (stmt.len > 0) {
                    stats.rules_in += 1;
                    stats.rules_out += 1;
                    pos = CssCompiler.writeIndent(out_buf, pos, depth, opts.minify);
                    pos = engAppendBytes(out_buf, pos, stmt);
                    if (!opts.minify) pos = engAppendBytes(out_buf, pos, "\n");
                }
                seg_start = i + 1;
            }
            i += 1;
        }
        return pos;
    }

    fn writeIndent(out_buf: []u8, pos_in: usize, depth: usize, minify: bool): usize {
        if (minify) return pos_in;
        let pos = pos_in;
        let d: usize = 0;
        while (d < depth * 2 && pos < out_buf.len) {
            out_buf[pos] = ' ';
            pos += 1;
            d += 1;
        }
        return pos;
    }

    fn emitRule(
        rule_chunk: []const u8,
        opts: CssCompileOptions,
        out_buf: []u8,
        pos_in: usize,
        depth: usize,
        stats: *CssCompileStats,
    ): usize {
        let pos = pos_in;
        const open = std.mem.indexOfScalar(u8, rule_chunk, '{') ?? return pos;
        const close = std.mem.lastIndexOfScalar(u8, rule_chunk, '}') ?? return pos;
        if (close <= open) return pos;

        let prelude = std.mem.trim(u8, rule_chunk[0..open], " \t\r\n");
        const body = std.mem.trim(u8, rule_chunk[open + 1 .. close], " \t\r\n");

        // Dead rule pruning
        if (shouldDropSelectorForUnused(prelude, opts)) {
            stats.unused_rules_dropped += 1;
            return pos;
        }

        // At-rule handling (@media, @container, @supports, @layer, @keyframes, @scope)
        if (prelude.len > 0 && prelude[0] == '@') {
            let mq_buf: [512]u8 = [0; 512];
            if (std.ascii.startsWithIgnoreCase(prelude, "@media") || std.ascii.startsWithIgnoreCase(prelude, "@container")) {
                const lowered_len = lowerMediaPrelude(prelude, opts, mq_buf[0..512]);
                if (lowered_len > 0) {
                    prelude = mq_buf[0..lowered_len];
                    stats.media_ranges_lowered += 1;
                }
            }

            // Emit @-webkit-keyframes when required by targets
            if (opts.add_vendor_prefixes && std.ascii.startsWithIgnoreCase(prelude, "@keyframes ")) {
                const kf_name = std.mem.trim(u8, prelude[11..], " \t");
                const kf_pfx = opts.targets.prefixes(PrefixFeature.AtKeyframes);
                if (kf_pfx.hasWebkit()) {
                    pos = CssCompiler.writeIndent(out_buf, pos, depth, opts.minify);
                    pos = engAppendBytes(out_buf, pos, "@-webkit-keyframes ");
                    pos = engAppendBytes(out_buf, pos, kf_name);
                    pos = engAppendBytes(out_buf, pos, if (opts.minify) "{" else " {\n");
                    pos = CssCompiler.compileBlock(body, opts, out_buf, pos, depth + 1, stats);
                    pos = CssCompiler.writeIndent(out_buf, pos, depth, opts.minify);
                    pos = engAppendBytes(out_buf, pos, if (opts.minify) "}" else "}\n");
                    stats.prefixes_injected += 1;
                    stats.rules_out += 1;
                }
            }

            pos = CssCompiler.writeIndent(out_buf, pos, depth, opts.minify);
            pos = engAppendBytes(out_buf, pos, prelude);
            pos = engAppendBytes(out_buf, pos, if (opts.minify) "{" else " {\n");
            if (std.mem.indexOfScalar(u8, body, '{') != null) {
                pos = CssCompiler.compileBlock(body, opts, out_buf, pos, depth + 1, stats);
            } else {
                pos = CssCompiler.emitDeclarations(body, opts, out_buf, pos, depth + 1, stats);
            }
            pos = CssCompiler.writeIndent(out_buf, pos, depth, opts.minify);
            pos = engAppendBytes(out_buf, pos, if (opts.minify) "}" else "}\n");
            stats.rules_out += 1;
            return pos;
        }

        // Style rule: CSS Modules scoping
        let scoped_buf: [512]u8 = [0; 512];
        if (opts.css_modules) {
            const scoped = CssModulesHasher.scopeSelector(prelude, opts.module_hash_seed, scoped_buf[0..512]);
            if (!std.mem.eql(u8, scoped, prelude)) stats.scoped_selectors += 1;
            prelude = scoped;
        }

        // Minify selector list spacing (`a, b` -> `a,b` when minify=true)
        pos = CssCompiler.writeIndent(out_buf, pos, depth, opts.minify);
        pos = engAppendBytes(out_buf, pos, prelude);
        pos = engAppendBytes(out_buf, pos, if (opts.minify) "{" else " {\n");
        pos = CssCompiler.emitDeclarations(body, opts, out_buf, pos, depth + 1, stats);
        pos = CssCompiler.writeIndent(out_buf, pos, depth, opts.minify);
        pos = engAppendBytes(out_buf, pos, if (opts.minify) "}" else "}\n");
        stats.rules_out += 1;
        return pos;
    }

    fn emitDeclarations(
        body: []const u8,
        opts: CssCompileOptions,
        out_buf: []u8,
        pos_in: usize,
        depth: usize,
        stats: *CssCompileStats,
    ): usize {
        let pos = pos_in;
        let decl_items: [64][]const u8 = [""; 64];
        let decl_count: usize = 0;

        // Split declarations by ';' respecting quotes and parens
        let i: usize = 0;
        let start: usize = 0;
        let paren: usize = 0;
        let in_q: u8 = 0;
        while (i < body.len) {
            const c = body[i];
            if (in_q != 0) {
                if (c == '\\' && i + 1 < body.len) { i += 2; continue; }
                if (c == in_q) in_q = 0;
                i += 1;
                continue;
            }
            if (c == '"' || c == '\'') { in_q = c; i += 1; continue; }
            if (c == '(') paren += 1;
            else if (c == ')' && paren > 0) paren -= 1;
            else if (c == ';' && paren == 0) {
                const d = std.mem.trim(u8, body[start..i], " \t\r\n");
                if (d.len > 0 && decl_count < 64) {
                    decl_items[decl_count] = d;
                    decl_count += 1;
                }
                start = i + 1;
            }
            i += 1;
        }
        const tail = std.mem.trim(u8, body[start..], " \t\r\n");
        if (tail.len > 0 && decl_count < 64) {
            decl_items[decl_count] = tail;
            decl_count += 1;
        }

        // Expand & transform into flat output list
        let out_names: [128][]const u8 = [""; 128];
        let out_vals: [128][]const u8 = [""; 128];
        let out_imp: [128]bool = [false; 128];
        let out_count: usize = 0;

        // Scratch storage for transformed slices during this rule
        let val_arena: [4096]u8 = [0; 4096];
        let arena_pos: usize = 0;
        let pfx_arena: [1024]u8 = [0; 1024];
        let pfx_pos: usize = 0;

        let idx: usize = 0;
        while (idx < decl_count) {
            const decl = decl_items[idx];
            const colon_opt = std.mem.indexOfScalar(u8, decl, ':');
            if (colon_opt == null) { idx += 1; continue; }
            const colon = colon_opt.?;
            const prop_name = std.mem.trim(u8, decl[0..colon], " \t\r\n");
            let val_part = std.mem.trim(u8, decl[colon + 1 ..], " \t\r\n");
            let important = false;
            if (val_part.len >= 10 && std.ascii.eqlIgnoreCase(val_part[val_part.len - 10 ..], "!important")) {
                important = true;
                val_part = std.mem.trim(u8, val_part[0 .. val_part.len - 10], " \t\r\n");
            }
            stats.declarations_processed += 1;

            // Step A: Value minification & lowering
            let step_a = val_part;
            if (opts.minify_properties) {
                if (RectShorthand.isRectShorthand(prop_name)) {
                    let mb: [256]u8 = [0; 256];
                    const mv = RectShorthand.minifyValue(val_part, mb[0..256]);
                    if (!std.mem.eql(u8, mv, val_part) && arena_pos + mv.len <= val_arena.len) {
                        @memcpy(val_arena[arena_pos .. arena_pos + mv.len], mv);
                        step_a = val_arena[arena_pos .. arena_pos + mv.len];
                        arena_pos += mv.len;
                        stats.shorthands_minified += 1;
                    }
                } else if (std.ascii.eqlIgnoreCase(prop_name, "border-radius")) {
                    if (br := BorderRadius.parse(val_part)) {
                        let brb: [256]u8 = [0; 256];
                        const bs = br.format(brb[0..256], opts.minify);
                        if (!std.mem.eql(u8, bs, val_part) && arena_pos + bs.len <= val_arena.len) {
                            @memcpy(val_arena[arena_pos .. arena_pos + bs.len], bs);
                            step_a = val_arena[arena_pos .. arena_pos + bs.len];
                            arena_pos += bs.len;
                            stats.shorthands_minified += 1;
                        }
                    }
                } else if (std.ascii.eqlIgnoreCase(prop_name, "flex")) {
                    if (fl := FlexShorthand.parse(val_part)) {
                        let fb: [128]u8 = [0; 128];
                        const fs = fl.format(fb[0..128], opts.minify);
                        if (!std.mem.eql(u8, fs, val_part) && arena_pos + fs.len <= val_arena.len) {
                            @memcpy(val_arena[arena_pos .. arena_pos + fs.len], fs);
                            step_a = val_arena[arena_pos .. arena_pos + fs.len];
                            arena_pos += fs.len;
                            stats.shorthands_minified += 1;
                        }
                    }
                } else if (std.ascii.eqlIgnoreCase(prop_name, "display")) {
                    if (dv := DisplayValue.parse(val_part)) {
                        let db: [64]u8 = [0; 64];
                        const ds = dv.format(db[0..64]);
                        if (!std.mem.eql(u8, ds, val_part) && arena_pos + ds.len <= val_arena.len) {
                            @memcpy(val_arena[arena_pos .. arena_pos + ds.len], ds);
                            step_a = val_arena[arena_pos .. arena_pos + ds.len];
                            arena_pos += ds.len;
                            stats.shorthands_minified += 1;
                        }
                    }
                } else if (std.ascii.eqlIgnoreCase(prop_name, "transition-duration") ||
                    std.ascii.eqlIgnoreCase(prop_name, "transition-delay") ||
                    std.ascii.eqlIgnoreCase(prop_name, "animation-duration") ||
                    std.ascii.eqlIgnoreCase(prop_name, "animation-delay"))
                {
                    if (tm := Time.parse(val_part)) {
                        let tb: [64]u8 = [0; 64];
                        const ts = tm.format(tb[0..64], opts.minify);
                        if (!std.mem.eql(u8, ts, val_part) && arena_pos + ts.len <= val_arena.len) {
                            @memcpy(val_arena[arena_pos .. arena_pos + ts.len], ts);
                            step_a = val_arena[arena_pos .. arena_pos + ts.len];
                            arena_pos += ts.len;
                            stats.shorthands_minified += 1;
                        }
                    }
                } else if (std.ascii.eqlIgnoreCase(prop_name, "background") || std.ascii.eqlIgnoreCase(prop_name, "background-image")) {
                    if (gr := Gradient.parse(val_part)) {
                        const lower_dp = !opts.targets.isCompatible(Feature.DoublePositionGradients) && gr.hasDoublePositionStops();
                        if (lower_dp || opts.minify) {
                            let gb: [512]u8 = [0; 512];
                            const gs = gr.format(gb[0..512], opts.minify, lower_dp);
                            if (arena_pos + gs.len <= val_arena.len) {
                                @memcpy(val_arena[arena_pos .. arena_pos + gs.len], gs);
                                step_a = val_arena[arena_pos .. arena_pos + gs.len];
                                arena_pos += gs.len;
                            }
                        }
                    }
                }
            }

            let step_b = step_a;
            if (opts.minify_properties && opts.minify) {
                let hb: [16]u8 = [0; 16];
                const h_len = minifyHexColorInValue(step_a, hb[0..16]);
                if (h_len > 0 && arena_pos + h_len <= val_arena.len) {
                    @memcpy(val_arena[arena_pos .. arena_pos + h_len], hb[0..h_len]);
                    step_b = val_arena[arena_pos .. arena_pos + h_len];
                    arena_pos += h_len;
                }
            }
            const final_val = if (opts.minify_properties && opts.minify) minifyZeroUnit(step_b) else step_b;

            // Step B: Logical property expansion
            let logical_decls: [4]CssExpandedDecl = std.mem.zeroes([4]CssExpandedDecl);
            let logical_n: usize = 0;
            if (opts.expand_logical && opts.targets.shouldCompileLogical() && CssLogical.isLogicalProperty(prop_name)) {
                logical_n = CssLogical.expand(prop_name, final_val, important, logical_decls[0..4]);
                stats.logical_expanded += 1;
            } else {
                logical_decls[0] = CssExpandedDecl { name: prop_name, value: final_val, important: important };
                logical_n = 1;
            }

            // Step C: Vendor prefix injection per emitted declaration
            let li: usize = 0;
            while (li < logical_n) {
                const ld = logical_decls[li];
                if (opts.add_vendor_prefixes && ld.name.len > 0 && ld.name[0] != '-') {
                    const pfx = opts.targets.prefixesForProperty(ld.name);
                    if (pfx.hasWebkit() && out_count < 128 && pfx_pos + ld.name.len + 10 <= pfx_arena.len) {
                        const s = std.fmt.bufPrint(pfx_arena[pfx_pos..], "-webkit-{s}", .{ld.name}) catch "";
                        if (s.len > 0) {
                            pfx_pos += s.len;
                            out_names[out_count] = s;
                            out_vals[out_count] = ld.value;
                            out_imp[out_count] = ld.important;
                            out_count += 1;
                            stats.prefixes_injected += 1;
                        }
                    }
                    if (pfx.hasMoz() && out_count < 128 && pfx_pos + ld.name.len + 8 <= pfx_arena.len) {
                        const s = std.fmt.bufPrint(pfx_arena[pfx_pos..], "-moz-{s}", .{ld.name}) catch "";
                        if (s.len > 0) {
                            pfx_pos += s.len;
                            out_names[out_count] = s;
                            out_vals[out_count] = ld.value;
                            out_imp[out_count] = ld.important;
                            out_count += 1;
                            stats.prefixes_injected += 1;
                        }
                    }
                    if (pfx.hasMs() && out_count < 128 && pfx_pos + ld.name.len + 8 <= pfx_arena.len) {
                        const s = std.fmt.bufPrint(pfx_arena[pfx_pos..], "-ms-{s}", .{ld.name}) catch "";
                        if (s.len > 0) {
                            pfx_pos += s.len;
                            out_names[out_count] = s;
                            out_vals[out_count] = ld.value;
                            out_imp[out_count] = ld.important;
                            out_count += 1;
                            stats.prefixes_injected += 1;
                        }
                    }
                    if (pfx.hasO() && out_count < 128 && pfx_pos + ld.name.len + 6 <= pfx_arena.len) {
                        const s = std.fmt.bufPrint(pfx_arena[pfx_pos..], "-o-{s}", .{ld.name}) catch "";
                        if (s.len > 0) {
                            pfx_pos += s.len;
                            out_names[out_count] = s;
                            out_vals[out_count] = ld.value;
                            out_imp[out_count] = ld.important;
                            out_count += 1;
                            stats.prefixes_injected += 1;
                        }
                    }
                }

                if (out_count < 128) {
                    out_names[out_count] = ld.name;
                    out_vals[out_count] = ld.value;
                    out_imp[out_count] = ld.important;
                    out_count += 1;
                }
                li += 1;
            }
            idx += 1;
        }

        // Emit formatted declarations
        let oi: usize = 0;
        while (oi < out_count) {
            pos = CssCompiler.writeIndent(out_buf, pos, depth, opts.minify);
            pos = engAppendBytes(out_buf, pos, out_names[oi]);
            pos = engAppendBytes(out_buf, pos, if (opts.minify) ":" else ": ");
            pos = engAppendBytes(out_buf, pos, out_vals[oi]);
            if (out_imp[oi]) {
                pos = engAppendBytes(out_buf, pos, if (opts.minify) "!important" else " !important");
            }
            if (opts.minify) {
                if (oi + 1 < out_count) pos = engAppendBytes(out_buf, pos, ";");
            } else {
                pos = engAppendBytes(out_buf, pos, ";\n");
            }
            oi += 1;
        }
        return pos;
    }
}
