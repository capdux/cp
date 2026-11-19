// Source: /data/dev/bun/src/css/rules/rules.cp
// lib/css/stylesheet.cp
// CSS Rules, Declarations, @media Queries, @keyframes, Stylesheet Parser & Minifier
// Absorbed and unified from Bun src/css/rules/{rules,style,media,keyframes,supports,layer,container,font_face,import}.cp

const std = @import("std");
import { CssLength } from "./length.cp";
import { CssSelector } from "./selector.cp";

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
        if (std.mem.endsWith(u8, val_part, "!important")) {
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
    Unknown,
}

pub struct CssMediaQuery {
    raw: []const u8 = "";
    media_type: []const u8 = "all";
    min_width_px: f64 = 0.0;
    max_width_px: f64 = 0.0;
    prefers_dark: bool = false;

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

        if (std.ascii.startsWithIgnoreCase(s, "screen")) {
            res.media_type = "screen";
        } else if (std.ascii.startsWithIgnoreCase(s, "print")) {
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
                    } else if (std.ascii.eqlIgnoreCase(feat_name, "prefers-color-scheme")) {
                        if (std.ascii.eqlIgnoreCase(feat_val, "dark")) {
                            res.prefers_dark = true;
                        }
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
        if (_.min_width_px > 0.0 && viewport_w < _.min_width_px) {
            return false;
        }
        if (_.max_width_px > 0.0 && viewport_w > _.max_width_px) {
            return false;
        }
        if (_.prefers_dark && !is_dark) {
            return false;
        }
        return true;
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

        let kf = CssKeyframes { name: anim_name };
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

            let step = CssKeyframeStep { offset_percent: pct };
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

    pub fn formatRule(out_buf: []u8, minify: bool): []const u8 {
        let pos: usize = 0;

        if (_.kind == CssRuleKind.Import) {
            pos = appendSlice(out_buf, pos, "@import ");
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

        pos = appendSlice(out_buf, pos, "}");
        return out_buf[0..pos];
    }
}

pub struct CssStylesheetParser {
    /// Parses a single CSS rule (Style rule, `@media`, `@keyframes`, `@import`, `@supports`, `@layer`, `@container`, `@font-face`).
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
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@keyframes")) {
                kind = CssRuleKind.Keyframes;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@supports")) {
                kind = CssRuleKind.Supports;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@layer")) {
                kind = CssRuleKind.Layer;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@container")) {
                kind = CssRuleKind.Container;
            } else if (std.ascii.startsWithIgnoreCase(prelude, "@font-face")) {
                kind = CssRuleKind.FontFace;
            }

            let rule = CssRule {
                kind: kind,
                prelude: prelude,
                media: media,
                nested_body: body,
            };

            if (kind == CssRuleKind.FontFace) {
                let it = std.mem.splitScalar(u8, body, ';');
                while (decl := it.next()) {
                    if (rule.prop_count >= 16) break;
                    if (p := CssProperty.parse(decl)) {
                        rule.properties[rule.prop_count] = p;
                        rule.prop_count += 1;
                    }
                }
            }
            return rule;
        }

        // Standard Style Rule
        let rule = CssRule {
            kind: CssRuleKind.Style,
            prelude: prelude,
            selector: CssSelector.parse(prelude),
            nested_body: body,
        };
        let it = std.mem.splitScalar(u8, body, ';');
        while (decl := it.next()) {
            if (rule.prop_count >= 16) {
                break;
            }
            if (p := CssProperty.parse(decl)) {
                rule.properties[rule.prop_count] = p;
                rule.prop_count += 1;
            }
        }
        return rule;
    }

    /// Parses a full CSS stylesheet into `out_rules` (ignoring `/* ... */` comments) and returns the number of rules parsed.
    pub static fn parseStylesheet(css_text: []const u8, out_rules: []CssRule): usize {
        let count: usize = 0;
        let i: usize = 0;
        let rule_start: usize = 0;
        let brace_depth: usize = 0;

        while (i < css_text.len && count < out_rules.len) {
            // Skip /* ... */ comments if at top level before rule_start
            if (i + 1 < css_text.len && css_text[i] == '/' && css_text[i + 1] == '*') {
                const was_leading = (std.mem.trim(u8, css_text[rule_start..i], " \t\r\n").len == 0);
                i += 2;
                while (i + 1 < css_text.len && !(css_text[i] == '*' && css_text[i + 1] == '/')) {
                    i += 1;
                }
                i += 2;
                if (was_leading && brace_depth == 0) {
                    rule_start = i;
                }
                continue;
            }

            const c = css_text[i];
            if (c == '{') {
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

    /// Minifies a CSS stylesheet by stripping comments, collapsing whitespace, and removing redundant punctuation spaces.
    pub static fn minify(css_text: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let out_pos: usize = 0;
        let pending_space: bool = false;

        while (i < css_text.len && out_pos < out_buf.len) {
            // Strip /* ... */ comments
            if (i + 1 < css_text.len && css_text[i] == '/' && css_text[i + 1] == '*') {
                i += 2;
                while (i + 1 < css_text.len && !(css_text[i] == '*' && css_text[i + 1] == '/')) {
                    i += 1;
                }
                i += 2;
                continue;
            }

            const c = css_text[i];
            if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
                pending_space = true;
                i += 1;
                continue;
            }

            const is_punct = (c == '{' || c == '}' || c == ':' || c == ';' || c == ',');
            if (c == '}' && out_pos > 0 && out_buf[out_pos - 1] == ';') {
                // Drop trailing ';' right before '}'
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
}
