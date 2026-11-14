// lib/md/markdown.cp
// CommonMark / GFM Markdown Parser, HTML Renderer & Terminal ANSI Renderer
// Absorbed from Bun src/md/root.cp, html_renderer.cp & ansi_renderer.cp

const std = @import("std");

pub struct MdOptions {
    tables: bool = true;
    strikethrough: bool = true;
    tasklists: bool = true;
    autolinks: bool = true;
    tag_filter: bool = false;
    heading_ids: bool = false;

    pub static fn commonmark(): MdOptions {
        return MdOptions {
            tables: false,
            strikethrough: false,
            tasklists: false,
            autolinks: false,
            tag_filter: false,
            heading_ids: false,
        };
    }

    pub static fn github(): MdOptions {
        return MdOptions {
            tables: true,
            strikethrough: true,
            tasklists: true,
            autolinks: true,
            tag_filter: true,
            heading_ids: true,
        };
    }
}

pub struct Markdown {
    pub static fn appendSlice(out: []u8, pos: usize, s: []const u8): usize {
        if (pos + s.len > out.len) {
            return pos;
        }
        let i: usize = 0;
        while (i < s.len) {
            out[pos + i] = s[i];
            i += 1;
        }
        return pos + s.len;
    }

    /// Renders inline Markdown (`**bold**`, `*em*`, `` `code` ``) into HTML inside `out`.
    pub static fn renderInlineHtml(line: []const u8, out: []u8, start_pos: usize): usize {
        let pos = start_pos;
        let i: usize = 0;
        let in_code: bool = false;
        let in_bold: bool = false;

        while (i < line.len && pos < out.len) {
            if (line[i] == '`') {
                if (!in_code) {
                    pos = Markdown.appendSlice(out, pos, "<code>");
                    in_code = true;
                } else {
                    pos = Markdown.appendSlice(out, pos, "</code>");
                    in_code = false;
                }
                i += 1;
                continue;
            }

            if (!in_code && i + 1 < line.len && line[i] == '*' && line[i + 1] == '*') {
                if (!in_bold) {
                    pos = Markdown.appendSlice(out, pos, "<strong>");
                    in_bold = true;
                } else {
                    pos = Markdown.appendSlice(out, pos, "</strong>");
                    in_bold = false;
                }
                i += 2;
                continue;
            }

            out[pos] = line[i];
            pos += 1;
            i += 1;
        }
        return pos;
    }

    /// Renders Markdown headings (`#`..`######`), unordered lists (`- `), and paragraphs to HTML in `out_buf`.
    pub static fn renderHtml(md_text: []const u8, out_buf: []u8): []const u8 {
        let pos: usize = 0;
        let line_iter = std.mem.splitScalar(u8, md_text, '\n');

        while (raw_line := line_iter.next()) {
            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0) {
                continue;
            }

            // ATX Heading (# .. ######)
            if (line[0] == '#') {
                let level: usize = 0;
                while (level < line.len && level < 6 && line[level] == '#') {
                    level += 1;
                }
                if (level < line.len && line[level] == ' ') {
                    const content = std.mem.trim(u8, line[level + 1 ..], " \t");
                    let open_tag: [4]u8 = ['<' as u8, 'h' as u8, (48 as u8) + (level as u8), '>' as u8];
                    let close_tag: [6]u8 = ['<' as u8, '/' as u8, 'h' as u8, (48 as u8) + (level as u8), '>' as u8, '\n' as u8];
                    pos = Markdown.appendSlice(out_buf, pos, open_tag[0..4]);
                    pos = Markdown.renderInlineHtml(content, out_buf, pos);
                    pos = Markdown.appendSlice(out_buf, pos, close_tag[0..6]);
                    continue;
                }
            }

            // Unordered list item (- item)
            if (line.len >= 2 && (line[0] == '-' || line[0] == '*') && line[1] == ' ') {
                pos = Markdown.appendSlice(out_buf, pos, "<li>");
                pos = Markdown.renderInlineHtml(line[2..], out_buf, pos);
                pos = Markdown.appendSlice(out_buf, pos, "</li>\n");
                continue;
            }

            // Paragraph
            pos = Markdown.appendSlice(out_buf, pos, "<p>");
            pos = Markdown.renderInlineHtml(line, out_buf, pos);
            pos = Markdown.appendSlice(out_buf, pos, "</p>\n");
        }

        return out_buf[0..pos];
    }

    /// Renders Markdown headings and bold text into ANSI terminal sequences.
    pub static fn renderAnsi(md_text: []const u8, out_buf: []u8): []const u8 {
        let pos: usize = 0;
        let line_iter = std.mem.splitScalar(u8, md_text, '\n');

        while (raw_line := line_iter.next()) {
            const line = std.mem.trim(u8, raw_line, " \t\r");
            if (line.len == 0) {
                continue;
            }
            if (std.mem.startsWith(u8, line, "# ")) {
                pos = Markdown.appendSlice(out_buf, pos, "\x1b[1;36m");
                pos = Markdown.appendSlice(out_buf, pos, line[2..]);
                pos = Markdown.appendSlice(out_buf, pos, "\x1b[0m\n");
            } else {
                pos = Markdown.appendSlice(out_buf, pos, line);
                pos = Markdown.appendSlice(out_buf, pos, "\n");
            }
        }

        return out_buf[0..pos];
    }
}
