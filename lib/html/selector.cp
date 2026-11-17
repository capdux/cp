// Source: Bun src/lolhtml_sys/lol_html.cp
// lib/html/selector.cp
// CSS Selector Parser & Streaming HTML Element/Text Rewriter
// Absorbed from Bun src/lolhtml_sys/lol_html.cp & src/css/selectors/

const std = @import("std");

pub struct CssSelector {
    tag_name: []const u8 = "";
    id_name: []const u8 = "";
    class_name: []const u8 = "";
    attr_name: []const u8 = "";
    attr_val: []const u8 = "";

    /// Parses a compound CSS selector such as `div`, `a#main`, `span.highlight`, or `input[type=text]`.
    pub static fn parse(sel: []const u8): CssSelector {
        const trimmed = std.mem.trim(u8, sel, " \t");
        let res = CssSelector {};
        if (trimmed.len == 0) {
            return res;
        }

        // Check attribute selector [attr=val]
        let base = trimmed;
        if (br_open := std.mem.indexOfScalar(u8, trimmed, '[')) {
            if (br_close := std.mem.lastIndexOfScalar(u8, trimmed, ']')) {
                if (br_close > br_open) {
                    const attr_expr = trimmed[br_open + 1 .. br_close];
                    if (eq_idx := std.mem.indexOfScalar(u8, attr_expr, '=')) {
                        res.attr_name = std.mem.trim(u8, attr_expr[0..eq_idx], " \t");
                        res.attr_val = std.mem.trim(u8, attr_expr[eq_idx + 1 ..], " \t\"'");
                    } else {
                        res.attr_name = std.mem.trim(u8, attr_expr, " \t");
                    }
                    base = trimmed[0..br_open];
                }
            }
        }

        if (hash_idx := std.mem.indexOfScalar(u8, base, '#')) {
            res.tag_name = base[0..hash_idx];
            res.id_name = base[hash_idx + 1 ..];
            return res;
        }

        if (dot_idx := std.mem.indexOfScalar(u8, base, '.')) {
            res.tag_name = base[0..dot_idx];
            res.class_name = base[dot_idx + 1 ..];
            return res;
        }

        res.tag_name = base;
        return res;
    }

    pub fn matchesTag(tag: []const u8): bool {
        if (_.tag_name.len == 0 || std.mem.eql(u8, _.tag_name, "*")) {
            return true;
        }
        return std.mem.eql(u8, _.tag_name, tag);
    }
}

pub struct HtmlRewriter {
    /// Strips all `<...>` HTML tags from `html` and writes the plain text content into `out_buf`.
    pub static fn stripTags(html: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let out_pos: usize = 0;
        let in_tag: bool = false;

        while (i < html.len && out_pos < out_buf.len) {
            if (html[i] == '<') {
                in_tag = true;
                i += 1;
                continue;
            }
            if (html[i] == '>') {
                in_tag = false;
                i += 1;
                continue;
            }
            if (!in_tag) {
                out_buf[out_pos] = html[i];
                out_pos += 1;
            }
            i += 1;
        }
        return out_buf[0..out_pos];
    }

    /// Rewrites all occurrences of `<old_tag>` and `</old_tag>` to `<new_tag>` and `</new_tag>`.
    pub static fn renameTag(html: []const u8, old_tag: []const u8, new_tag: []const u8, out_buf: []u8): []const u8 {
        let i: usize = 0;
        let out_pos: usize = 0;

        while (i < html.len && out_pos < out_buf.len) {
            if (html[i] == '<') {
                const is_close = (i + 1 < html.len && html[i + 1] == '/');
                const tag_start = if (is_close) (i + 2) else (i + 1);
                if (tag_start + old_tag.len < html.len &&
                    std.mem.eql(u8, html[tag_start .. tag_start + old_tag.len], old_tag) &&
                    (html[tag_start + old_tag.len] == '>' || html[tag_start + old_tag.len] == ' '))
                {
                    out_buf[out_pos] = '<';
                    out_pos += 1;
                    if (is_close) {
                        out_buf[out_pos] = '/';
                        out_pos += 1;
                    }
                    let k: usize = 0;
                    while (k < new_tag.len && out_pos < out_buf.len) {
                        out_buf[out_pos] = new_tag[k];
                        out_pos += 1;
                        k += 1;
                    }
                    i = tag_start + old_tag.len;
                    continue;
                }
            }
            out_buf[out_pos] = html[i];
            out_pos += 1;
            i += 1;
        }
        return out_buf[0..out_pos];
    }
}
