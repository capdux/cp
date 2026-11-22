// Source: Absorbed from /data/dev/bun/src/css/rules/rules.cp | Synced: 2026-11-22T12:49:17Z
// lib/css/stylesheet.cp
// CSS Rules, Declarations, @media Queries, @keyframes, Nesting Flattener, Stylesheet AST, Printer & Minifier
// Absorbed from Bun src/css/rules/{rules,style,media,keyframes,supports,layer,container,font_face,import,nesting}.cp & printer.cp

const std = @import("std");
import { CssLength } from "./length.cp";
import { CssSelector } from "./selector.cp";
import { CssVariableMap } from "./variables.cp";

fn appendSlice(out_buf: []u8, pos: usize, s: []const u8): usize {
    let p = pos;
    let i: usize = 0;
    while (i < s.len && p < out_buf.len) {
        out_buf[p] = s[i];
        p += 1;
        i += 1;
    }
    return p;
}

fn appendIndent(out_buf: []u8, pos: usize, depth: usize): usize {
    let p = pos;
    let d: usize = 0;
    while (d < depth * 2 && p < out_buf.len) {
        out_buf[p] = ' ';
        p += 1;
        d += 1;
    }
    return p;
}

fn trimRightBytes(s: []const u8, strip_chars: []const u8): []const u8 {
    let end = s.len;
    while (end > 0) {
        const ch = s[end - 1];
        if (std.mem.indexOfScalar(u8, strip_chars, ch) == null) {
            break;
        }
        end -= 1;
    }
    return s[0..end];
}

pub struct CssProperty {
    name: []const u8 = "";
    value: []const u8 = "";
    important: bool = false;

    /// Parses a single CSS declaration such as `color: #ff0000 !important` or `--primary-gap: 1.5rem`.
    pub static fn parse(decl: []const u8): CssProperty? {
        const s = std.mem.trim(u8, decl, " \t\r\n;");
        if (s.len == 0) {
            return null;
        }
        const colon_idx = std.mem.indexOfScalar(u8, s, ':') ?? return null;
        const prop_name = std.mem.trim(u8, s[0..colon_idx], " \t\r\n");
        if (prop_name.len == 0) {
            return null;
        }
        let val_part = std.mem.trim(u8, s[colon_idx + 1 ..], " \t\r\n");
        let is_important = false;
        if (val_part.len >= 10 && std.ascii.eqlIgnoreCase(val_part[val_part.len - 10 ..], "!important")) {
            is_important = true;
            val_part = std.mem.trim(u8, val_part[0 .. val_part.len - 10], " \t\r\n");
        }
        if (val_part.len == 0) {
            return null;
        }
        return CssProperty {
            name: prop_name,
            value: val_part,
            important: is_important,
        };
    }

    pub fn isCustomProperty(): bool {
        return std.mem.startsWith(u8, _.name, "--");
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        if (_.important) {
            if (minify) {
                return std.fmt.bufPrint(out_buf, "{s}:{s}!important", .{ _.name, _.value }) catch "";
            }
            return std.fmt.bufPrint(out_buf, "{s}: {s} !important;", .{ _.name, _.value }) catch "";
        }
        if (minify) {
            return std.fmt.bufPrint(out_buf, "{s}:{s}", .{ _.name, _.value }) catch "";
        }
        return std.fmt.bufPrint(out_buf, "{s}: {s};", .{ _.name, _.value }) catch "";
    }
}

pub enum CssRuleKind {
    Style,
    Media,
    Keyframes,
    Import,
    FontFace,
    Supports,
    Layer,
    Container,
    Namespace,
    Page,
    Property,
    StartingStyle,
    Scope,
    Nest,
    Unknown,
}

pub struct CssMediaQuery {
    raw: []const u8 = "";
    media_type: []const u8 = "all";
    min_width_px: f64 = 0.0;
    max_width_px: f64 = 0.0;
    min_height_px: f64 = 0.0;
    max_height_px: f64 = 0.0;
    prefers_dark: bool = false;
    prefers_reduced_motion: bool = false;
    orientation: []const u8 = "";
    negated: bool = false;

    /// Parses a `@media` query prelude such as `screen and (min-width: 768px) and (max-width: 1200px)`
    /// or `(prefers-color-scheme: dark)`.
    pub static fn parse(query: []const u8): CssMediaQuery {
        let s = std.mem.trim(u8, query, " \t\r\n");
        if (std.ascii.startsWithIgnoreCase(s, "@media")) {
            s = std.mem.trim(u8, s[6..], " \t\r\n");
        }
        let res = CssMediaQuery { raw: s, media_type: "all" };
        if (s.len == 0) {
            return res;
        }

        let rest = s;
        if (std.ascii.startsWithIgnoreCase(rest, "not ")) {
            res.negated = true;
            rest = std.mem.trim(u8, rest[4..], " \t\r\n");
        } else if (std.ascii.startsWithIgnoreCase(rest, "only ")) {
            rest = std.mem.trim(u8, rest[5..], " \t\r\n");
        }

        if (std.ascii.startsWithIgnoreCase(rest, "screen")) {
            res.media_type = "screen";
        } else if (std.ascii.startsWithIgnoreCase(rest, "print")) {
            res.media_type = "print";
        }

        // Scan parenthesized media feature conditions `(...)`
        let pos: usize = 0;
        while (pos < s.len) {
            if (s[pos] == '(') {
                const start = pos + 1;
                let end = start;
                while (end < s.len && s[end] != ')') {
                    end += 1;
                }
                const feature = std.mem.trim(u8, s[start..end], " \t\r\n");
                if (colon := std.mem.indexOfScalar(u8, feature, ':')) {
                    const feat_name = std.mem.trim(u8, feature[0..colon], " \t");
                    const feat_val = std.mem.trim(u8, feature[colon + 1 ..], " \t");
                    if (std.ascii.eqlIgnoreCase(feat_name, "min-width")) {
                        if (len_v := CssLength.parse(feat_val)) {
                            res.min_width_px = len_v.toPx(16.0, 1920.0, 1080.0);
                        }
                    } else if (std.ascii.eqlIgnoreCase(feat_name, "max-width")) {
                        if (len_v := CssLength.parse(feat_val)) {
                            res.max_width_px = len_v.toPx(16.0, 1920.0, 1080.0);
                        }
                    } else if (std.ascii.eqlIgnoreCase(feat_name, "min-height")) {
                        if (len_v := CssLength.parse(feat_val)) {
                            res.min_height_px = len_v.toPx(16.0, 1920.0, 1080.0);
                        }
                    } else if (std.ascii.eqlIgnoreCase(feat_name, "max-height")) {
                        if (len_v := CssLength.parse(feat_val)) {
                            res.max_height_px = len_v.toPx(16.0, 1920.0, 1080.0);
                        }
                    } else if (std.ascii.eqlIgnoreCase(feat_name, "prefers-color-scheme")) {
                        if (std.ascii.eqlIgnoreCase(feat_val, "dark")) {
                            res.prefers_dark = true;
                        }
                    } else if (std.ascii.eqlIgnoreCase(feat_name, "prefers-reduced-motion")) {
                        if (std.ascii.eqlIgnoreCase(feat_val, "reduce")) {
                            res.prefers_reduced_motion = true;
                        }
                    } else if (std.ascii.eqlIgnoreCase(feat_name, "orientation")) {
                        res.orientation = feat_val;
                    }
                }
                pos = end + 1;
            } else {
                pos += 1;
            }
        }
        return res;
    }

    /// Evaluates whether the media query matches a given viewport width (in `px`) and color scheme.
    pub fn matches(viewport_w: f64, is_dark: bool): bool {
        return _.matchesFull(viewport_w, 1080.0, is_dark, false);
    }

    /// Full media query evaluation including viewport height, dark scheme, reduced motion, and orientation.
    pub fn matchesFull(viewport_w: f64, viewport_h: f64, is_dark: bool, reduced_motion: bool): bool {
        let ok = true;
        if (_.min_width_px > 0.0 && viewport_w < _.min_width_px) {
            ok = false;
        }
        if (_.max_width_px > 0.0 && viewport_w > _.max_width_px) {
            ok = false;
        }
        if (_.min_height_px > 0.0 && viewport_h < _.min_height_px) {
            ok = false;
        }
        if (_.max_height_px > 0.0 && viewport_h > _.max_height_px) {
            ok = false;
        }
        if (_.prefers_dark && !is_dark) {
            ok = false;
        }
        if (_.prefers_reduced_motion && !reduced_motion) {
            ok = false;
        }
        if (_.orientation.len > 0) {
            if (std.ascii.eqlIgnoreCase(_.orientation, "landscape") && viewport_w < viewport_h) {
                ok = false;
            } else if (std.ascii.eqlIgnoreCase(_.orientation, "portrait") && viewport_h < viewport_w) {
                ok = false;
            }
        }
        return if (_.negated) !ok else ok;
    }
}

pub struct CssKeyframeStep {
    offset_percent: f64 = 0.0;
    declarations: [8]CssProperty = undefined;
    decl_count: usize = 0;
}

pub struct CssKeyframes {
    name: []const u8 = "";
    steps: [8]CssKeyframeStep = undefined;
    step_count: usize = 0;

    /// Parses a `@keyframes <name> { from { ... } 50% { ... } to { ... } }` rule.
    pub static fn parse(rule_text: []const u8): CssKeyframes? {
        const s = std.mem.trim(u8, rule_text, " \t\r\n");
        if (!std.ascii.startsWithIgnoreCase(s, "@keyframes")) {
            return null;
        }
        const open_brace = std.mem.indexOfScalar(u8, s, '{') ?? return null;
        const close_brace = std.mem.lastIndexOfScalar(u8, s, '}') ?? return null;
        if (close_brace <= open_brace) {
            return null;
        }

        const anim_name = std.mem.trim(u8, s[10..open_brace], " \t\r\n");
        if (anim_name.len == 0) {
            return null;
        }

        let kf = CssKeyframes {
            name: anim_name,
            steps: std.mem.zeroes([8]CssKeyframeStep),
            step_count: 0,
        };
        const body = s[open_brace + 1 .. close_brace];
        let pos: usize = 0;

        while (pos < body.len && kf.step_count < 8) {
            while (pos < body.len && (body[pos] == ' ' || body[pos] == '\t' || body[pos] == '\r' || body[pos] == '\n')) {
                pos += 1;
            }
            if (pos >= body.len) {
                break;
            }
            const step_open_opt = std.mem.indexOfScalar(u8, body[pos..], '{');
            if (step_open_opt == null) {
                break;
            }
            const step_open = pos + step_open_opt.?;
            const step_close_opt = std.mem.indexOfScalar(u8, body[step_open + 1 ..], '}');
            if (step_close_opt == null) {
                break;
            }
            const step_close = step_open + 1 + step_close_opt.?;

            const stop_label = std.mem.trim(u8, body[pos..step_open], " \t\r\n");
            let pct: f64 = 0.0;
            if (std.ascii.eqlIgnoreCase(stop_label, "from")) {
                pct = 0.0;
            } else if (std.ascii.eqlIgnoreCase(stop_label, "to")) {
                pct = 100.0;
            } else if (std.mem.endsWith(u8, stop_label, "%")) {
                pct = std.fmt.parseFloat(f64, stop_label[0 .. stop_label.len - 1]) catch 0.0;
            }

            let step = CssKeyframeStep {
                offset_percent: pct,
                declarations: std.mem.zeroes([8]CssProperty),
                decl_count: 0,
            };
            const decls_str = body[step_open + 1 .. step_close];
            let decl_it = std.mem.splitScalar(u8, decls_str, ';');
            while (d_raw := decl_it.next()) {
                if (step.decl_count >= 8) {
                    break;
                }
                if (prop := CssProperty.parse(d_raw)) {
                    step.declarations[step.decl_count] = prop;
                    step.decl_count += 1;
                }
            }

            kf.steps[kf.step_count] = step;
            kf.step_count += 1;
            pos = step_close + 1;
        }
        return kf;
    }
}

pub struct CssRule {
    kind: CssRuleKind = CssRuleKind.Style;
    prelude: []const u8 = "";
    selector: CssSelector = CssSelector {};
    media: CssMediaQuery = CssMediaQuery {};
    properties: [16]CssProperty = undefined;
    prop_count: usize = 0;
    nested_body: []const u8 = "";

    pub fn hasProperty(prop_name: []const u8): bool {
        return _.getProperty(prop_name) != null;
    }

    pub fn getProperty(prop_name: []const u8): ([]const u8)? {
        let i: usize = 0;
        while (i < _.prop_count) {
            if (std.ascii.eqlIgnoreCase(_.properties[i].name, prop_name)) {
                return _.properties[i].value;
            }
            i += 1;
        }
        return null;
    }

    pub fn setProperty(*_, prop_name: []const u8, prop_value: []const u8, important: bool): bool {
        let i: usize = 0;
        while (i < _.prop_count) {
            if (std.ascii.eqlIgnoreCase(_.properties[i].name, prop_name)) {
                _.properties[i].value = prop_value;
                _.properties[i].important = important;
                return true;
            }
            i += 1;
        }
        if (_.prop_count >= 16) return false;
        _.properties[_.prop_count] = CssProperty {
            name: prop_name,
            value: prop_value,
            important: important,
        };
        _.prop_count += 1;
        return true;
    }

    pub fn removeProperty(*_, prop_name: []const u8): bool {
        let i: usize = 0;
        while (i < _.prop_count) {
            if (std.ascii.eqlIgnoreCase(_.properties[i].name, prop_name)) {
                let j = i;
                while (j + 1 < _.prop_count) {
                    _.properties[j] = _.properties[j + 1];
                    j += 1;
                }
                _.prop_count -= 1;
                return true;
            }
            i += 1;
        }
        return false;
    }

    pub fn extractVariables(vars: *CssVariableMap): usize {
        let added: usize = 0;
        let i: usize = 0;
        while (i < _.prop_count) {
            if (_.properties[i].isCustomProperty()) {
                if (vars.set(_.properties[i].name, _.properties[i].value)) {
                    added += 1;
                }
            }
            i += 1;
        }
        return added;
    }

    pub fn formatRule(out_buf: []u8, minify: bool): []const u8 {
        let pos: usize = 0;

        if (_.kind == CssRuleKind.Import || _.kind == CssRuleKind.Namespace) {
            const kw = if (_.kind == CssRuleKind.Import) "@import " else "@namespace ";
            pos = appendSlice(out_buf, pos, kw);
            pos = appendSlice(out_buf, pos, _.prelude);
            pos = appendSlice(out_buf, pos, ";");
            return out_buf[0..pos];
        }

        pos = appendSlice(out_buf, pos, _.prelude);
        if (minify) {
            pos = appendSlice(out_buf, pos, "{");
        } else {
            pos = appendSlice(out_buf, pos, " { ");
        }

        if (_.prop_count > 0) {
            let i: usize = 0;
            while (i < _.prop_count) {
                const p = _.properties[i];
                pos = appendSlice(out_buf, pos, p.name);
                if (minify) {
                    pos = appendSlice(out_buf, pos, ":");
                    pos = appendSlice(out_buf, pos, p.value);
                    if (p.important) {
                        pos = appendSlice(out_buf, pos, "!important");
                    }
                    if (i + 1 < _.prop_count) {
                        pos = appendSlice(out_buf, pos, ";");
                    }
                } else {
                    pos = appendSlice(out_buf, pos, ": ");
                    pos = appendSlice(out_buf, pos, p.value);
                    if (p.important) {
                        pos = appendSlice(out_buf, pos, " !important; ");
                    } else {
                        pos = appendSlice(out_buf, pos, "; ");
                    }
                }
                i += 1;
            }
        } else if (_.nested_body.len > 0) {
            pos = appendSlice(out_buf, pos, _.nested_body);
            if (!minify) {
                pos = appendSlice(out_buf, pos, " ");
            }
        }

        pos = appendSlice(out_buf, pos, "}");
        return out_buf[0..pos];
    }
}

fn splitTopLevelDecls(body: []const u8, rule: *CssRule) {
    let i: usize = 0;
    let start: usize = 0;
    let p_depth: usize = 0;
    let b_depth: usize = 0;
    let in_q: u8 = 0;

    while (i <= body.len && rule.prop_count < 16) {
        let is_split = false;
        if (i == body.len) {
            is_split = true;
        } else {
            const c = body[i];
            if (in_q != 0) {
                if (c == '\\' && i + 1 < body.len) {
                    i += 2;
                    continue;
                }
                if (c == in_q) in_q = 0;
            } else {
                if (c == '"' || c == '\'') in_q = c;
                else if (c == '(') p_depth += 1;
                else if (c == ')' && p_depth > 0) p_depth -= 1;
                else if (c == '{') b_depth += 1;
                else if (c == '}' && b_depth > 0) b_depth -= 1;
                else if (c == ';' && p_depth == 0 && b_depth == 0) is_split = true;
            }
        }
        if (is_split) {
            if (b_depth == 0) {
                if (p := CssProperty.parse(body[start..i])) {
                    rule.properties[rule.prop_count] = p;
                    rule.prop_count += 1;
                }
            }
            start = i + 1;
        }
        i += 1;
    }
}

pub struct CssStylesheetParser {
    /// Parses a single CSS rule (Style rule, `@media`, `@keyframes`, `@import`, `@supports`, `@layer`, `@container`, `@font-face`, etc.).
    pub static fn parseRule(rule_str: []const u8): CssRule? {
        const s = std.mem.trim(u8, rule_str, " \t\r\n");
        if (s.len == 0) {
            return null;
        }

        if (std.ascii.startsWithIgnoreCase(s, "@import")) {
            const rest = std.mem.trim(u8, s[7..], " \t\r\n;");
            return CssRule {
                kind: CssRuleKind.Import,
                prelude: rest,
                properties: std.mem.zeroes([16]CssProperty),
            };
        }

        if (std.ascii.startsWithIgnoreCase(s, "@namespace")) {
            const rest = std.mem.trim(u8, s[10..], " \t\r\n;");
            return CssRule {
                kind: CssRuleKind.Namespace,
                prelude: rest,
                properties: std.mem.zeroes([16]CssProperty),
            };
        }

        const open_brace = std.mem.indexOfScalar(u8, s, '{') ?? return null;
        const close_brace = std.mem.lastIndexOfScalar(u8, s, '}') ?? return null;
        if (close_brace <= open_brace) {
            return null;
        }

        const prelude = std.mem.trim(u8, s[0..open_brace], " \t\r\n");
        const body = std.mem.trim(u8, s[open_brace + 1 .. close_brace], " \t\r\n");

        if (prelude.len > 0 && prelude[0] == '@') {
            let kind = CssRuleKind.Unknown;
            let media = CssMediaQuery {};
            if (std.ascii.startsWithIgnoreCase(prelude, "@media")) {
                kind = CssRuleKind.Media;
                media = CssMediaQuery.parse(prelude);
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@keyframes") ||
                std.ascii.startsWithIgnoreCase(prelude, "@-webkit-keyframes"))
            {
                kind = CssRuleKind.Keyframes;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@supports")) {
                kind = CssRuleKind.Supports;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@layer")) {
                kind = CssRuleKind.Layer;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@container")) {
                kind = CssRuleKind.Container;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@font-face")) {
                kind = CssRuleKind.FontFace;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@page")) {
                kind = CssRuleKind.Page;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@property")) {
                kind = CssRuleKind.Property;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@starting-style")) {
                kind = CssRuleKind.StartingStyle;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@scope")) {
                kind = CssRuleKind.Scope;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@nest")) {
                kind = CssRuleKind.Nest;
            }

            let rule = CssRule {
                kind: kind,
                prelude: prelude,
                media: media,
                properties: std.mem.zeroes([16]CssProperty),
                nested_body: body,
            };

            if (kind == CssRuleKind.FontFace || kind == CssRuleKind.Property || kind == CssRuleKind.Page) {
                splitTopLevelDecls(body, &rule);
            }
            return rule;
        }

        // Standard Style Rule
        let rule = CssRule {
            kind: CssRuleKind.Style,
            prelude: prelude,
            selector: CssSelector.parse(prelude),
            properties: std.mem.zeroes([16]CssProperty),
            nested_body: body,
        };
        splitTopLevelDecls(body, &rule);
        return rule;
    }

    /// Parses a full CSS stylesheet into `out_rules` (ignoring `/* ... */` comments and respecting quotes) and returns the number of rules parsed.
    pub static fn parseStylesheet(css_text: []const u8, out_rules: []CssRule): usize {
        let count: usize = 0;
        let i: usize = 0;
        let rule_start: usize = 0;
        let brace_depth: usize = 0;
        let in_quote: u8 = 0;

        while (i < css_text.len && count < out_rules.len) {
            const c = css_text[i];

            if (in_quote != 0) {
                if (c == '\\' && i + 1 < css_text.len) {
                    i += 2;
                    continue;
                }
                if (c == in_quote) {
                    in_quote = 0;
                }
                i += 1;
                continue;
            }

            // Skip /* ... */ comments
            if (i + 1 < css_text.len && c == '/' && css_text[i + 1] == '*') {
                const was_leading = (std.mem.trim(u8, css_text[rule_start..i], " \t\r\n").len == 0);
                i += 2;
                while (i + 1 < css_text.len && !(css_text[i] == '*' && css_text[i + 1] == '/')) {
                    i += 1;
                }
                if (i + 1 < css_text.len) {
                    i += 2;
                } else {
                    i = css_text.len;
                }
                if (was_leading && brace_depth == 0) {
                    rule_start = i;
                }
                continue;
            }

            if (c == '"' || c == '\'') {
                in_quote = c;
            } else if (c == '{') {
                brace_depth += 1;
            } else if (c == '}') {
                if (brace_depth > 0) {
                    brace_depth -= 1;
                    if (brace_depth == 0) {
                        const chunk = css_text[rule_start .. i + 1];
                        if (r := CssStylesheetParser.parseRule(chunk)) {
                            out_rules[count] = r;
                            count += 1;
                        }
                        rule_start = i + 1;
                    }
                }
            } else if (c == ';' && brace_depth == 0) {
                const chunk = css_text[rule_start .. i + 1];
                if (r := CssStylesheetParser.parseRule(chunk)) {
                    out_rules[count] = r;
                    count += 1;
                }
                rule_start = i + 1;
            }
            i += 1;
        }
        return count;
    }

    /// Minifies a CSS stylesheet by stripping comments, collapsing whitespace, preserving string literals,
    /// and removing redundant punctuation spaces and trailing semicolons before `}`.
    pub static fn minify(css_text: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let out_pos: usize = 0;
        let pending_space: bool = false;
        let in_quote: u8 = 0;

        while (i < css_text.len && out_pos < out_buf.len) {
            const c = css_text[i];

            if (in_quote != 0) {
                out_buf[out_pos] = c;
                out_pos += 1;
                if (c == '\\' && i + 1 < css_text.len && out_pos < out_buf.len) {
                    i += 1;
                    out_buf[out_pos] = css_text[i];
                    out_pos += 1;
                } else if (c == in_quote) {
                    in_quote = 0;
                }
                i += 1;
                continue;
            }

            // Strip /* ... */ comments
            if (i + 1 < css_text.len && c == '/' && css_text[i + 1] == '*') {
                i += 2;
                while (i + 1 < css_text.len && !(css_text[i] == '*' && css_text[i + 1] == '/')) {
                    i += 1;
                }
                if (i + 1 < css_text.len) {
                    i += 2;
                } else {
                    i = css_text.len;
                }
                continue;
            }

            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                pending_space = true;
                i += 1;
                continue;
            }

            if (c == '"' || c == '\'') {
                if (pending_space && out_pos > 0) {
                    const prev = out_buf[out_pos - 1];
                    const prev_punct = (prev == '{' || prev == '}' || prev == ':' || prev == ';' || prev == ',');
                    if (!prev_punct) {
                        out_buf[out_pos] = ' ';
                        out_pos += 1;
                    }
                }
                pending_space = false;
                in_quote = c;
                if (out_pos < out_buf.len) {
                    out_buf[out_pos] = c;
                    out_pos += 1;
                }
                i += 1;
                continue;
            }

            const is_punct = (c == '{' || c == '}' || c == ':' || c == ';' || c == ',');
            if (c == '}' && out_pos > 0 && out_buf[out_pos - 1] == ';') {
                out_pos -= 1;
            }

            if (pending_space && !is_punct && out_pos > 0) {
                const prev = out_buf[out_pos - 1];
                const prev_punct = (prev == '{' || prev == '}' || prev == ':' || prev == ';' || prev == ',');
                if (!prev_punct) {
                    out_buf[out_pos] = ' ';
                    out_pos += 1;
                }
            }
            pending_space = false;

            if (out_pos < out_buf.len) {
                out_buf[out_pos] = c;
                out_pos += 1;
            }
            i += 1;
        }
        return out_buf[0..out_pos];
    }

    /// Pretty-prints a CSS stylesheet with 2-space indentation and clean line breaks.
    pub static fn formatPretty(css_text: []const u8, out_buf: []u8): []const u8 {
        let min_tmp: [4096]u8 = [0; 4096];
        const compact = CssStylesheetParser.minify(css_text, min_tmp[0..4096]);

        let pos: usize = 0;
        let depth: usize = 0;
        let i: usize = 0;
        let in_quote: u8 = 0;
        let paren_depth: usize = 0;

        while (i < compact.len && pos < out_buf.len) {
            const c = compact[i];
            if (in_quote != 0) {
                out_buf[pos] = c;
                pos += 1;
                if (c == '\\' && i + 1 < compact.len && pos < out_buf.len) {
                    i += 1;
                    out_buf[pos] = compact[i];
                    pos += 1;
                } else if (c == in_quote) {
                    in_quote = 0;
                }
                i += 1;
                continue;
            }
            if (c == '"' || c == '\'') {
                in_quote = c;
                out_buf[pos] = c;
                pos += 1;
                i += 1;
                continue;
            }
            if (c == '(') paren_depth += 1;
            else if (c == ')' && paren_depth > 0) paren_depth -= 1;

            if (c == '{') {
                pos = appendSlice(out_buf, pos, " {\n");
                depth += 1;
                pos = appendIndent(out_buf, pos, depth);
            } else if (c == '}') {
                if (pos > 0 && out_buf[pos - 1] != '\n') {
                    // Check if previous line had content
                    let k = pos;
                    let only_spaces = true;
                    while (k > 0 && out_buf[k - 1] != '\n') {
                        k -= 1;
                        if (out_buf[k] != ' ') {
                            only_spaces = false;
                            break;
                        }
                    }
                    if (!only_spaces) {
                        pos = appendSlice(out_buf, pos, ";\n");
                    } else {
                        pos = k;
                    }
                }
                if (depth > 0) depth -= 1;
                pos = appendIndent(out_buf, pos, depth);
                pos = appendSlice(out_buf, pos, "}\n");
                if (i + 1 < compact.len && compact[i + 1] != '}') {
                    pos = appendIndent(out_buf, pos, depth);
                }
            } else if (c == ';' && paren_depth == 0) {
                pos = appendSlice(out_buf, pos, ";\n");
                if (i + 1 < compact.len && compact[i + 1] != '}') {
                    pos = appendIndent(out_buf, pos, depth);
                }
            } else if (c == ':' && depth > 0 && paren_depth == 0) {
                pos = appendSlice(out_buf, pos, ": ");
            } else if (c == ',' && depth == 0) {
                pos = appendSlice(out_buf, pos, ", ");
            } else {
                out_buf[pos] = c;
                pos += 1;
            }
            i += 1;
        }
        return trimRightBytes(out_buf[0..pos], " \t\r\n");
    }
}

fn expandNestedSelector(parent: []const u8, child_raw: []const u8, out_buf: []u8): []const u8 {
    const child = std.mem.trim(u8, child_raw, " \t\r\n");
    if (parent.len == 0) return child;
    let pos: usize = 0;

    // Handle comma-separated selectors in child
    let it = std.mem.splitScalar(u8, child, ',');
    let first = true;
    while (part_raw := it.next()) {
        const part = std.mem.trim(u8, part_raw, " \t\r\n");
        if (part.len == 0) continue;
        if (!first) {
            pos = appendSlice(out_buf, pos, ", ");
        }
        first = false;
        if (std.mem.indexOfScalar(u8, part, '&') != null) {
            let j: usize = 0;
            while (j < part.len) {
                if (part[j] == '&') {
                    pos = appendSlice(out_buf, pos, parent);
                } else if (pos < out_buf.len) {
                    out_buf[pos] = part[j];
                    pos += 1;
                }
                j += 1;
            }
        } else {
            pos = appendSlice(out_buf, pos, parent);
            pos = appendSlice(out_buf, pos, " ");
            pos = appendSlice(out_buf, pos, part);
        }
    }
    return out_buf[0..pos];
}

fn flattenBlock(parent_sel: []const u8, body: []const u8, out_buf: []u8, pos_in: usize, depth: usize): usize {
    if (depth >= 8) return pos_in;
    let pos = pos_in;
    let decl_buf: [2048]u8 = [0; 2048];
    let decl_pos: usize = 0;
    let nested_buf: [4096]u8 = [0; 4096];
    let nested_pos: usize = 0;

    let i: usize = 0;
    let seg_start: usize = 0;
    let brace_depth: usize = 0;
    let p_depth: usize = 0;
    let in_q: u8 = 0;
    let prelude_start: usize = 0;
    let prelude_end: usize = 0;

    while (i < body.len) {
        const c = body[i];
        if (in_q != 0) {
            if (c == '\\' && i + 1 < body.len) {
                i += 2;
                continue;
            }
            if (c == in_q) in_q = 0;
            i += 1;
            continue;
        }
        if (i + 1 < body.len && c == '/' && body[i + 1] == '*') {
            i += 2;
            while (i + 1 < body.len && !(body[i] == '*' && body[i + 1] == '/')) i += 1;
            if (i + 1 < body.len) i += 2 else i = body.len;
            if (brace_depth == 0) seg_start = i;
            continue;
        }
        if (c == '"' || c == '\'') {
            in_q = c;
            i += 1;
            continue;
        }
        if (c == '(') p_depth += 1;
        else if (c == ')' && p_depth > 0) p_depth -= 1;

        if (c == '{') {
            if (brace_depth == 0) {
                prelude_start = seg_start;
                prelude_end = i;
                seg_start = i + 1;
            }
            brace_depth += 1;
        } else if (c == '}') {
            if (brace_depth > 0) {
                brace_depth -= 1;
                if (brace_depth == 0) {
                    const cur_prelude = std.mem.trim(u8, body[prelude_start..prelude_end], " \t\r\n");
                    const inner_body = std.mem.trim(u8, body[seg_start..i], " \t\r\n");
                    if (cur_prelude.len > 0 && cur_prelude[0] == '@') {
                        // Nested at-rule like @media { ... } inside a style rule
                        let inner_flat: [2048]u8 = [0; 2048];
                        const flat_len = flattenBlock(parent_sel, inner_body, inner_flat[0..2048], 0, depth + 1);
                        nested_pos = appendSlice(nested_buf[0..4096], nested_pos, cur_prelude);
                        nested_pos = appendSlice(nested_buf[0..4096], nested_pos, " { ");
                        nested_pos = appendSlice(nested_buf[0..4096], nested_pos, inner_flat[0..flat_len]);
                        nested_pos = appendSlice(nested_buf[0..4096], nested_pos, " }\n");
                    } else {
                        let sel_buf: [512]u8 = [0; 512];
                        const full_sel = expandNestedSelector(parent_sel, cur_prelude, sel_buf[0..512]);
                        let sub_flat: [2048]u8 = [0; 2048];
                        const sub_len = flattenBlock(full_sel, inner_body, sub_flat[0..2048], 0, depth + 1);
                        nested_pos = appendSlice(nested_buf[0..4096], nested_pos, sub_flat[0..sub_len]);
                    }
                    seg_start = i + 1;
                }
            }
        } else if (c == ';' && brace_depth == 0 && p_depth == 0) {
            const decl = std.mem.trim(u8, body[seg_start..i], " \t\r\n");
            if (decl.len > 0) {
                decl_pos = appendSlice(decl_buf[0..2048], decl_pos, decl);
                decl_pos = appendSlice(decl_buf[0..2048], decl_pos, "; ");
            }
            seg_start = i + 1;
        }
        i += 1;
    }

    if (brace_depth == 0 && seg_start < body.len) {
        const tail = std.mem.trim(u8, body[seg_start..], " \t\r\n");
        if (tail.len > 0) {
            decl_pos = appendSlice(decl_buf[0..2048], decl_pos, tail);
            decl_pos = appendSlice(decl_buf[0..2048], decl_pos, "; ");
        }
    }

    if (decl_pos > 0 && parent_sel.len > 0) {
        pos = appendSlice(out_buf, pos, parent_sel);
        pos = appendSlice(out_buf, pos, " { ");
        pos = appendSlice(out_buf, pos, trimRightBytes(decl_buf[0..decl_pos], " "));
        pos = appendSlice(out_buf, pos, " }\n");
    }
    if (nested_pos > 0) {
        pos = appendSlice(out_buf, pos, nested_buf[0..nested_pos]);
    }
    return pos;
}

pub struct CssNesting {
    /// Expands CSS Nesting syntax (e.g. `.card { color: #111; &:hover { color: red; } .title { font-weight: bold; } }`)
    /// into standard flat CSS rules.
    pub static fn flatten(css_text: []const u8, out_buf: []u8): []const u8 {
        let pos: usize = 0;
        let i: usize = 0;
        let rule_start: usize = 0;
        let brace_depth: usize = 0;
        let in_q: u8 = 0;

        while (i < css_text.len) {
            const c = css_text[i];
            if (in_q != 0) {
                if (c == '\\' && i + 1 < css_text.len) { i += 2; continue; }
                if (c == in_q) in_q = 0;
                i += 1;
                continue;
            }
            if (i + 1 < css_text.len && c == '/' && css_text[i + 1] == '*') {
                const was_leading = (std.mem.trim(u8, css_text[rule_start..i], " \t\r\n").len == 0);
                i += 2;
                while (i + 1 < css_text.len && !(css_text[i] == '*' && css_text[i + 1] == '/')) i += 1;
                if (i + 1 < css_text.len) i += 2 else i = css_text.len;
                if (was_leading && brace_depth == 0) rule_start = i;
                continue;
            }
            if (c == '"' || c == '\'') { in_q = c; i += 1; continue; }

            if (c == '{') {
                brace_depth += 1;
            } else if (c == '}') {
                if (brace_depth > 0) {
                    brace_depth -= 1;
                    if (brace_depth == 0) {
                        const chunk = std.mem.trim(u8, css_text[rule_start .. i + 1], " \t\r\n");
                        if (open_b := std.mem.indexOfScalar(u8, chunk, '{')) {
                            const close_b = std.mem.lastIndexOfScalar(u8, chunk, '}') ?? chunk.len;
                            const prelude = std.mem.trim(u8, chunk[0..open_b], " \t\r\n");
                            const body = std.mem.trim(u8, chunk[open_b + 1 .. close_b], " \t\r\n");
                            if (prelude.len > 0 && prelude[0] == '@') {
                                pos = appendSlice(out_buf, pos, chunk);
                                pos = appendSlice(out_buf, pos, "\n");
                            } else {
                                pos = flattenBlock(prelude, body, out_buf, pos, 0);
                            }
                        }
                        rule_start = i + 1;
                    }
                }
            } else if (c == ';' && brace_depth == 0) {
                const stmt = std.mem.trim(u8, css_text[rule_start .. i + 1], " \t\r\n");
                if (stmt.len > 0) {
                    pos = appendSlice(out_buf, pos, stmt);
                    pos = appendSlice(out_buf, pos, "\n");
                }
                rule_start = i + 1;
            }
            i += 1;
        }
        return trimRightBytes(out_buf[0..pos], "\n");
    }
}

pub struct CssStylesheet {
    rules: [64]CssRule = undefined;
    rule_count: usize = 0;

    pub static fn parse(css_text: []const u8): CssStylesheet {
        let sheet = CssStylesheet {
            rules: std.mem.zeroes([64]CssRule),
            rule_count: 0,
        };
        sheet.rule_count = CssStylesheetParser.parseStylesheet(css_text, sheet.rules[0..64]);
        return sheet;
    }

    pub fn count(): usize {
        return _.rule_count;
    }

    pub fn findRuleBySelector(sel_substr: []const u8): CssRule? {
        let i: usize = 0;
        while (i < _.rule_count) {
            if (_.rules[i].kind == CssRuleKind.Style &&
                std.mem.indexOf(u8, _.rules[i].prelude, sel_substr) != null)
            {
                return _.rules[i];
            }
            i += 1;
        }
        return null;
    }

    pub fn collectVariables(): CssVariableMap {
        let vars = CssVariableMap.init();
        let i: usize = 0;
        while (i < _.rule_count) {
            * = _.rules[i].extractVariables(&vars);
            i += 1;
        }
        return vars;
    }

    pub fn format(out_buf: []u8, minify: bool): []const u8 {
        let pos: usize = 0;
        let tmp: [512]u8 = [0; 512];
        let i: usize = 0;
        while (i < _.rule_count) {
            const s = _.rules[i].formatRule(tmp[0..512], minify);
            pos = appendSlice(out_buf, pos, s);
            if (!minify && i + 1 < _.rule_count) {
                pos = appendSlice(out_buf, pos, "\n");
            }
            i += 1;
        }
        return out_buf[0..pos];
    }
}
