// Source: Absorbed from /data/dev/bun/src/md/parser.cp | Synced: 2026-11-20T07:49:10Z
// lib/md/parser.cp
// Complete CommonMark + GFM Block Parser, Inline Parser, AST Document & HTML Renderer

const std = @import("std");
import { MdTableAlign, MdTable } from "./table";
import { MarkdownOptions, MdBlockKind, MdBlock, MdLinkDef, MdRenderer } from "./ast";

pub struct MdDocument {
    allocator: std.mem.Allocator = undefined,
    root: MdBlock = MdBlock{ kind: .Document },
    link_defs: []MdLinkDef = [],
    options: MarkdownOptions = MarkdownOptions{},

    pub fn deinit(*_) {
        _.root.deinit(_.allocator);
        if (_.link_defs.len > 0) {
            _.allocator.free(_.link_defs);
            _.link_defs = [];
        }
    }

    pub fn blocks(): []MdBlock {
        return _.root.children;
    }

    pub fn walk(renderer: MdRenderer) {
        renderer.walkBlock(_.root);
    }

    pub fn toHtml(): ([]u8)! {
        return MdParser.renderDocumentToHtml(_.allocator, _.root, _.link_defs, _.options);
    }

    pub fn toHtmlWithOptions(opts: MarkdownOptions): ([]u8)! {
        return MdParser.renderDocumentToHtml(_.allocator, _.root, _.link_defs, opts);
    }
}

struct AtxHeadingMatch {
    level: u8 = 1,
    text: []const u8 = "",
}

struct FenceOpenMatch {
    ch: u8 = '`',
    len: usize = 3,
    indent: usize = 0,
    info: []const u8 = "",
    lang: []const u8 = "",
}

struct ListMarkerMatch {
    is_ordered: bool = false,
    bullet_char: u8 = '-',
    start_num: u32 = 1,
    content_col: usize = 2,
    rest: []const u8 = "",
}

struct LinkParseResult {
    text: []const u8 = "",
    url: []const u8 = "",
    title: []const u8 = "",
    next_pos: usize = 0,
}

pub struct MdParser {
    static fn trimCr(line: []const u8): []const u8 {
        if (line.len > 0 && line[line.len - 1] == '\r') {
            return line[0 .. line.len - 1];
        }
        return line;
    }

    static fn countLeadingSpaces(line: []const u8): usize {
        let i: usize = 0;
        let col: usize = 0;
        while (i < line.len) : (i += 1) {
            if (line[i] == ' ') {
                col += 1;
            } else if (line[i] == '\t') {
                col += 4 - (col % 4);
            } else {
                break;
            }
        }
        return col;
    }

    static fn skipSpacesCols(line: []const u8, cols: usize): []const u8 {
        let i: usize = 0;
        let col: usize = 0;
        while (i < line.len && col < cols) : (i += 1) {
            if (line[i] == ' ') {
                col += 1;
            } else if (line[i] == '\t') {
                col += 4 - (col % 4);
            } else {
                break;
            }
        }
        return line[i..];
    }

    static fn isBlankLine(line: []const u8): bool {
        for (c of line) {
            if (c != ' ' && c != '\t' && c != '\r') return false;
        }
        return true;
    }

    static fn byteOffsetAfterSpaces(line: []const u8, max_spaces: usize): usize {
        let i: usize = 0;
        let col: usize = 0;
        while (i < line.len && col < max_spaces) : (i += 1) {
            if (line[i] == ' ') {
                col += 1;
            } else if (line[i] == '\t') {
                col += 4;
            } else {
                break;
            }
        }
        return i;
    }

    static fn isThematicBreak(line: []const u8): bool {
        const lead = MdParser.countLeadingSpaces(line);
        if (lead > 3) return false;
        const s = std.mem.trim(u8, line, " \t\r");
        if (s.len < 3) return false;
        const ch = s[0];
        if (ch != '-' && ch != '*' && ch != '_') return false;
        let count: usize = 0;
        for (c of s) {
            if (c == ch) {
                count += 1;
            } else if (c != ' ' && c != '\t') {
                return false;
            }
        }
        return count >= 3;
    }

    static fn parseSetextUnderline(line: []const u8): u8 {
        const lead = MdParser.countLeadingSpaces(line);
        if (lead > 3) return 0;
        const s = std.mem.trim(u8, line, " \t\r");
        if (s.len == 0) return 0;
        const ch = s[0];
        if (ch != '=' && ch != '-') return 0;
        for (c of s) {
            if (c != ch) return 0;
        }
        return if (ch == '=') (1 as u8) else (2 as u8);
    }

    static fn parseAtxHeading(line: []const u8): AtxHeadingMatch? {
        const lead = MdParser.countLeadingSpaces(line);
        if (lead > 3) return null;
        const s = MdParser.skipSpacesCols(line, lead);
        if (s.len == 0 || s[0] != '#') return null;
        let level: usize = 0;
        while (level < s.len && level < 7 && s[level] == '#') {
            level += 1;
        }
        if (level == 0 || level > 6) return null;
        if (level < s.len && s[level] != ' ' && s[level] != '\t') {
            return null;
        }
        let content = if (level < s.len) std.mem.trim(u8, s[level..], " \t\r") else "";
        // Strip optional closing sequence of `#` preceded by space/tab
        if (content.len > 0 && content[content.len - 1] == '#') {
            let end_idx = content.len;
            while (end_idx > 0 && content[end_idx - 1] == '#') {
                end_idx -= 1;
            }
            if (end_idx == 0) {
                content = "";
            } else if (content[end_idx - 1] == ' ' || content[end_idx - 1] == '\t') {
                content = std.mem.trimEnd(u8, content[0..end_idx], " \t");
            }
        }
        return AtxHeadingMatch{
            level: level as u8,
            text: content,
        };
    }

    static fn parseFenceOpen(line: []const u8): FenceOpenMatch? {
        const indent = MdParser.countLeadingSpaces(line);
        if (indent > 3) return null;
        const s = MdParser.skipSpacesCols(line, indent);
        if (s.len < 3) return null;
        const ch = s[0];
        if (ch != '`' && ch != '~') return null;
        let fence_len: usize = 0;
        while (fence_len < s.len && s[fence_len] == ch) {
            fence_len += 1;
        }
        if (fence_len < 3) return null;
        const info = std.mem.trim(u8, s[fence_len..], " \t\r");
        // CommonMark: backtick fence info string cannot contain backticks
        if (ch == '`' && std.mem.indexOfScalar(u8, info, '`') != null) {
            return null;
        }
        let lang_end: usize = 0;
        while (lang_end < info.len && info[lang_end] != ' ' && info[lang_end] != '\t') {
            lang_end += 1;
        }
        return FenceOpenMatch{
            ch: ch,
            len: fence_len,
            indent: indent,
            info: info,
            lang: info[0..lang_end],
        };
    }

    static fn isFenceClose(line: []const u8, fence_ch: u8, min_len: usize): bool {
        const indent = MdParser.countLeadingSpaces(line);
        if (indent > 3) return false;
        const s = MdParser.skipSpacesCols(line, indent);
        let run: usize = 0;
        while (run < s.len && s[run] == fence_ch) {
            run += 1;
        }
        if (run < min_len) return false;
        return MdParser.isBlankLine(s[run..]);
    }

    static fn parseListMarker(line: []const u8): ListMarkerMatch? {
        const indent = MdParser.countLeadingSpaces(line);
        if (indent > 3) return null;
        const s = MdParser.skipSpacesCols(line, indent);
        if (s.len == 0) return null;

        // Unordered list: -, *, +
        if (s[0] == '-' || s[0] == '*' || s[0] == '+') {
            if (s.len == 1) {
                return ListMarkerMatch{
                    is_ordered: false,
                    bullet_char: s[0],
                    start_num: 1,
                    content_col: indent + 2,
                    rest: "",
                };
            }
            if (s[1] == ' ' || s[1] == '\t') {
                let sp: usize = 0;
                let j: usize = 1;
                while (j < s.len && (s[j] == ' ' || s[j] == '\t')) : (j += 1) {
                    sp += if (s[j] == '\t') (4 as usize) else (1 as usize);
                }
                const eff_sp = if (j >= s.len || sp >= 5) (1 as usize) else sp;
                const rest = if (sp >= 5) MdParser.skipSpacesCols(s[1..], 1) else s[j..];
                return ListMarkerMatch{
                    is_ordered: false,
                    bullet_char: s[0],
                    start_num: 1,
                    content_col: indent + 1 + eff_sp,
                    rest: rest,
                };
            }
            return null;
        }

        // Ordered list: 1..9 digits followed by '.' or ')'
        let dlen: usize = 0;
        let num: u32 = 0;
        while (dlen < s.len && dlen < 9 && s[dlen] >= '0' && s[dlen] <= '9') : (dlen += 1) {
            num = num * 10 + ((s[dlen] - '0') as u32);
        }
        if (dlen == 0 || dlen >= s.len) return null;
        const delim = s[dlen];
        if (delim != '.' && delim != ')') return null;

        const after_delim = dlen + 1;
        if (after_delim == s.len) {
            return ListMarkerMatch{
                is_ordered: true,
                bullet_char: delim,
                start_num: num,
                content_col: indent + after_delim + 1,
                rest: "",
            };
        }
        if (s[after_delim] != ' ' && s[after_delim] != '\t') return null;

        let sp: usize = 0;
        let j = after_delim;
        while (j < s.len && (s[j] == ' ' || s[j] == '\t')) : (j += 1) {
            sp += if (s[j] == '\t') (4 as usize) else (1 as usize);
        }
        const eff_sp = if (j >= s.len || sp >= 5) (1 as usize) else sp;
        const rest = if (sp >= 5) MdParser.skipSpacesCols(s[after_delim..], 1) else s[j..];
        return ListMarkerMatch{
            is_ordered: true,
            bullet_char: delim,
            start_num: num,
            content_col: indent + after_delim + eff_sp,
            rest: rest,
        };
    }

    static fn isBlockQuoteLine(line: []const u8): bool {
        const lead = MdParser.countLeadingSpaces(line);
        if (lead > 3) return false;
        const s = MdParser.skipSpacesCols(line, lead);
        return s.len > 0 && s[0] == '>';
    }

    static fn stripBlockQuotePrefix(line: []const u8): []const u8 {
        const lead = MdParser.countLeadingSpaces(line);
        const s = MdParser.skipSpacesCols(line, lead);
        if (s.len == 0 || s[0] != '>') return line;
        if (s.len > 1 && (s[1] == ' ' || s[1] == '\t')) {
            return s[2..];
        }
        return s[1..];
    }

    static fn isHtmlBlockStart(line: []const u8): bool {
        const lead = MdParser.countLeadingSpaces(line);
        if (lead > 3) return false;
        const s = MdParser.skipSpacesCols(line, lead);
        if (s.len < 3 || s[0] != '<') return false;
        if (std.mem.startsWith(u8, s, "<!--") || std.mem.startsWith(u8, s, "<!") || std.mem.startsWith(u8, s, "<?")) {
            return true;
        }
        let idx: usize = 1;
        if (idx < s.len && s[idx] == '/') idx += 1;
        if (idx >= s.len) return false;
        const c = s[idx];
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) return false;
        const name_start = idx;
        while (idx < s.len && ((s[idx] >= 'a' && s[idx] <= 'z') || (s[idx] >= 'A' && s[idx] <= 'Z') || (s[idx] >= '0' && s[idx] <= '9'))) {
            idx += 1;
        }
        const tag = s[name_start..idx];
        if (idx >= s.len || (s[idx] != '>' && s[idx] != ' ' && s[idx] != '\t' && s[idx] != '/')) return false;
        // CommonMark block tags + common inline-safe check (only treat as HTML block when closing '>' is on line or known block tag)
        if (std.ascii.eqlIgnoreCase(tag, "div") || std.ascii.eqlIgnoreCase(tag, "p") ||
            std.ascii.eqlIgnoreCase(tag, "table") || std.ascii.eqlIgnoreCase(tag, "script") ||
            std.ascii.eqlIgnoreCase(tag, "style") || std.ascii.eqlIgnoreCase(tag, "pre") ||
            std.ascii.eqlIgnoreCase(tag, "section") || std.ascii.eqlIgnoreCase(tag, "article") ||
            std.ascii.eqlIgnoreCase(tag, "header") || std.ascii.eqlIgnoreCase(tag, "footer") ||
            std.ascii.eqlIgnoreCase(tag, "blockquote") || std.ascii.eqlIgnoreCase(tag, "ul") ||
            std.ascii.eqlIgnoreCase(tag, "ol") || std.ascii.eqlIgnoreCase(tag, "details") ||
            std.ascii.eqlIgnoreCase(tag, "summary") || std.ascii.eqlIgnoreCase(tag, "iframe") ||
            std.ascii.eqlIgnoreCase(tag, "hr") || std.ascii.eqlIgnoreCase(tag, "h1") ||
            std.ascii.eqlIgnoreCase(tag, "h2") || std.ascii.eqlIgnoreCase(tag, "h3"))
        {
            return true;
        }
        return false;
    }

    static fn parseLinkRefDef(line: []const u8): MdLinkDef? {
        const lead = MdParser.countLeadingSpaces(line);
        if (lead > 3) return null;
        const s = MdParser.skipSpacesCols(line, lead);
        if (s.len < 5 || s[0] != '[') return null;
        const close_br = std.mem.indexOfScalar(u8, s[1..], ']') ?? return null;
        const label = std.mem.trim(u8, s[1 .. 1 + close_br], " \t");
        if (label.len == 0) return null;
        const colon_pos = 1 + close_br + 1;
        if (colon_pos >= s.len || s[colon_pos] != ':') return null;

        let rest = std.mem.trim(u8, s[colon_pos + 1 ..], " \t\r");
        if (rest.len == 0) return null;

        let url: []const u8 = "";
        if (rest[0] == '<') {
            const gt = std.mem.indexOfScalar(u8, rest[1..], '>') ?? return null;
            url = rest[1 .. 1 + gt];
            rest = std.mem.trim(u8, rest[1 + gt + 1 ..], " \t");
        } else {
            let uend: usize = 0;
            while (uend < rest.len && rest[uend] != ' ' && rest[uend] != '\t') {
                uend += 1;
            }
            url = rest[0..uend];
            rest = std.mem.trim(u8, rest[uend..], " \t");
        }

        let title: []const u8 = "";
        if (rest.len >= 2 && ((rest[0] == '"' && rest[rest.len - 1] == '"') ||
            (rest[0] == '\'' && rest[rest.len - 1] == '\'') ||
            (rest[0] == '(' && rest[rest.len - 1] == ')')))
        {
            title = rest[1 .. rest.len - 1];
        } else if (rest.len > 0) {
            return null;
        }

        return MdLinkDef{
            label: label,
            url: url,
            title: title,
        };
    }

    static fn joinLinesOwned(allocator: std.mem.Allocator, lines: []const []const u8): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        let i: usize = 0;
        while (i < lines.len) : (i += 1) {
            if (i > 0) out.append(allocator, '\n')?;
            out.appendSlice(allocator, lines[i])?;
        }
        return out.toOwnedSlice(allocator);
    }

    static fn canInterruptParagraph(line: []const u8, opts: MarkdownOptions): bool {
        if (MdParser.isBlankLine(line)) return true;
        if (MdParser.parseAtxHeading(line) != null) return true;
        if (MdParser.parseFenceOpen(line) != null) return true;
        if (MdParser.isThematicBreak(line)) return true;
        if (MdParser.isBlockQuoteLine(line)) return true;
        if (MdParser.parseListMarker(line) != null) return true;
        if (MdParser.isHtmlBlockStart(line)) return true;
        * = opts;
        return false;
    }

    static fn makeSlugOwned(allocator: std.mem.Allocator, raw_text: []const u8, seen_slugs: *std.ArrayList([]u8)): ([]u8)! {
        let base: std.ArrayList(u8) = .empty;
        defer base.deinit(allocator);

        let i: usize = 0;
        while (i < raw_text.len) : (i += 1) {
            const c = raw_text[i];
            if (c == '`' || c == '*' || c == '_' || c == '~') continue;
            if (c >= 'A' && c <= 'Z') {
                base.append(allocator, c + 32)?;
            } else if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-' || c == '_') {
                base.append(allocator, c)?;
            } else if (c == ' ' || c == '\t') {
                base.append(allocator, '-')?;
            } else if (c >= 0x80) {
                base.append(allocator, c)?;
            }
        }

        const base_slice = if (base.items.len > 0) base.items else "section";
        let suffix: usize = 0;
        while (true) {
            let candidate: std.ArrayList(u8) = .empty;
            errdefer candidate.deinit(allocator);
            candidate.appendSlice(allocator, base_slice)?;
            if (suffix > 0) {
                let num_buf: [24]u8 = undefined;
                const num_s = std.fmt.bufPrint(&num_buf, "-{d}", .{suffix}) catch unreachable;
                candidate.appendSlice(allocator, num_s)?;
            }
            const cand_slice = candidate.items;
            let dup = false;
            for (existing of seen_slugs.items) {
                if (std.mem.eql(u8, existing, cand_slice)) {
                    dup = true;
                    break;
                }
            }
            if (!dup) {
                const owned = candidate.toOwnedSlice(allocator)?;
                const tracked = allocator.dupe(u8, owned)?;
                seen_slugs.append(allocator, tracked)?;
                return owned;
            }
            candidate.deinit(allocator);
            suffix += 1;
        }
    }

    static fn assignHeadingIds(allocator: std.mem.Allocator, blocks: []MdBlock, seen_slugs: *std.ArrayList([]u8)): ! {
        let bi: usize = 0;
        while (bi < blocks.len) : (bi += 1) {
            if (blocks[bi].kind == .Heading) {
                const slug = MdParser.makeSlugOwned(allocator, blocks[bi].text, seen_slugs)?;
                zig {
                    blocks[bi].heading_id = slug;
                    blocks[bi].owns_heading_id = true;
                }
            }
            if (blocks[bi].children.len > 0) {
                MdParser.assignHeadingIds(allocator, blocks[bi].children, seen_slugs)?;
            }
        }
    }

    static fn parseLines(
        allocator: std.mem.Allocator,
        lines: []const []const u8,
        opts: MarkdownOptions,
        link_defs: *std.ArrayList(MdLinkDef),
    ): ([]MdBlock)! {
        let blocks: std.ArrayList(MdBlock) = .empty;
        errdefer {
            for (*b of blocks.items) b.deinit(allocator);
            blocks.deinit(allocator);
        }

        let i: usize = 0;
        while (i < lines.len) {
            const line = lines[i];
            if (MdParser.isBlankLine(line)) {
                i += 1;
                continue;
            }

            // 1. Fenced code block (``` or ~~~)
            if (fence := MdParser.parseFenceOpen(line)) {
                i += 1;
                let code_lines: std.ArrayList(string) = .empty;
                defer code_lines.deinit(allocator);
                while (i < lines.len) {
                    if (MdParser.isFenceClose(lines[i], fence.ch, fence.len)) {
                        i += 1;
                        break;
                    }
                    code_lines.append(allocator, MdParser.skipSpacesCols(lines[i], fence.indent))?;
                    i += 1;
                }
                const joined = MdParser.joinLinesOwned(allocator, code_lines.items)?;
                blocks.append(allocator, MdBlock{
                    kind: .CodeBlock,
                    fence_lang: fence.lang,
                    fence_info: fence.info,
                    is_fenced: true,
                    text: joined,
                    owns_text: true,
                })?;
                continue;
            }

            // 2. ATX Heading (#..######)
            if (atx := MdParser.parseAtxHeading(line)) {
                blocks.append(allocator, MdBlock{
                    kind: .Heading,
                    level: atx.level,
                    text: atx.text,
                })?;
                i += 1;
                continue;
            }

            // 3. Thematic break (---, ***, ___)
            if (MdParser.isThematicBreak(line)) {
                blocks.append(allocator, MdBlock{
                    kind: .ThematicBreak,
                })?;
                i += 1;
                continue;
            }

            // 4. Blockquote (>)
            if (MdParser.isBlockQuoteLine(line)) {
                let inner_lines: std.ArrayList(string) = .empty;
                defer inner_lines.deinit(allocator);
                while (i < lines.len) {
                    if (MdParser.isBlockQuoteLine(lines[i])) {
                        inner_lines.append(allocator, MdParser.stripBlockQuotePrefix(lines[i]))?;
                        i += 1;
                    } else if (!MdParser.isBlankLine(lines[i]) &&
                        inner_lines.items.len > 0 &&
                        !MdParser.isBlankLine(inner_lines.items[inner_lines.items.len - 1]) &&
                        !MdParser.canInterruptParagraph(lines[i], opts))
                    {
                        // Lazy continuation
                        inner_lines.append(allocator, lines[i])?;
                        i += 1;
                    } else {
                        break;
                    }
                }
                const children = MdParser.parseLines(allocator, inner_lines.items, opts, link_defs)?;
                blocks.append(allocator, MdBlock{
                    kind: .BlockQuote,
                    children: children,
                })?;
                continue;
            }

            // 5. Indented code block (>= 4 spaces or tab)
            if (!opts.no_indented_code_blocks && MdParser.countLeadingSpaces(line) >= 4) {
                let code_lines: std.ArrayList(string) = .empty;
                defer code_lines.deinit(allocator);
                let last_non_blank: usize = 0;
                while (i < lines.len) {
                    if (MdParser.isBlankLine(lines[i])) {
                        code_lines.append(allocator, "")?;
                        i += 1;
                    } else if (MdParser.countLeadingSpaces(lines[i]) >= 4) {
                        code_lines.append(allocator, MdParser.skipSpacesCols(lines[i], 4))?;
                        last_non_blank = code_lines.items.len;
                        i += 1;
                    } else {
                        break;
                    }
                }
                const joined = MdParser.joinLinesOwned(allocator, code_lines.items[0..last_non_blank])?;
                blocks.append(allocator, MdBlock{
                    kind: .CodeBlock,
                    is_fenced: false,
                    text: joined,
                    owns_text: true,
                })?;
                continue;
            }

            // 6. Bullet / Ordered List (with nested lists & GFM tasklists)
            if (first_mk := MdParser.parseListMarker(line)) {
                const list_ordered = first_mk.is_ordered;
                const list_bullet = first_mk.bullet_char;
                const list_start = first_mk.start_num;
                let items: std.ArrayList(MdBlock) = .empty;
                errdefer {
                    for (*it of items.items) it.deinit(allocator);
                    items.deinit(allocator);
                }
                let is_tight = true;
                let prev_had_blank_between = false;

                while (i < lines.len) {
                    const mk = MdParser.parseListMarker(lines[i]) ?? break;
                    if (mk.is_ordered != list_ordered) break;
                    if (!list_ordered && mk.bullet_char != list_bullet) break;
                    if (list_ordered && mk.bullet_char != list_bullet) break;

                    if (prev_had_blank_between) {
                        is_tight = false;
                    }

                    let item_lines: std.ArrayList(string) = .empty;
                    defer item_lines.deinit(allocator);

                    let first_rest = mk.rest;
                    let is_task = false;
                    let task_checked = false;
                    if (opts.tasklists && first_rest.len >= 3 && first_rest[0] == '[' &&
                        (first_rest[1] == ' ' || first_rest[1] == 'x' || first_rest[1] == 'X') &&
                        first_rest[2] == ']' && (first_rest.len == 3 || first_rest[3] == ' ' || first_rest[3] == '\t'))
                    {
                        is_task = true;
                        task_checked = first_rest[1] != ' ';
                        first_rest = if (first_rest.len > 3) std.mem.trimStart(u8, first_rest[3..], " \t") else "";
                    }
                    item_lines.append(allocator, first_rest)?;
                    i += 1;

                    let saw_blank_inside = false;
                    let blank_run: usize = 0;
                    while (i < lines.len) {
                        const cur = lines[i];
                        if (MdParser.isBlankLine(cur)) {
                            blank_run += 1;
                            i += 1;
                            continue;
                        }
                        const cur_lead = MdParser.countLeadingSpaces(cur);
                        if (cur_lead >= mk.content_col) {
                            if (blank_run > 0) {
                                let b: usize = 0;
                                while (b < blank_run) : (b += 1) {
                                    item_lines.append(allocator, "")?;
                                }
                                saw_blank_inside = true;
                                blank_run = 0;
                            }
                            item_lines.append(allocator, MdParser.skipSpacesCols(cur, mk.content_col))?;
                            i += 1;
                            continue;
                        }
                        // Lazy continuation if no blank line before and not a new block/list item
                        if (blank_run == 0 && !MdParser.canInterruptParagraph(cur, opts)) {
                            item_lines.append(allocator, cur)?;
                            i += 1;
                            continue;
                        }
                        break;
                    }
                    if (saw_blank_inside) {
                        is_tight = false;
                    }
                    prev_had_blank_between = blank_run > 0;

                    const item_children = MdParser.parseLines(allocator, item_lines.items, opts, link_defs)?;
                    items.append(allocator, MdBlock{
                        kind: .ListItem,
                        is_task: is_task,
                        task_checked: task_checked,
                        children: item_children,
                    })?;

                    if (blank_run > 0) {
                        // Look ahead: if next non-blank line continues same list, keep looping; else break
                        if (i >= lines.len) break;
                        const next_mk = MdParser.parseListMarker(lines[i]) ?? break;
                        if (next_mk.is_ordered != list_ordered || next_mk.bullet_char != list_bullet) break;
                    }
                }

                blocks.append(allocator, MdBlock{
                    kind: if (list_ordered) MdBlockKind.OrderedList else MdBlockKind.BulletList,
                    start_num: list_start,
                    tight: is_tight,
                    bullet_char: list_bullet,
                    children: items.toOwnedSlice(allocator)?,
                })?;
                continue;
            }

            // 7. GFM Table (header row + delimiter row)
            if (opts.tables && i + 1 < lines.len && std.mem.indexOfScalar(u8, line, '|') != null) {
                let aligns: [16]MdTableAlign = undefined;
                const col_count = MdTable.parseDelimiterRow(lines[i + 1], aligns[0..16]);
                if (col_count > 0) {
                    let hdr_cells: [16][]const u8 = undefined;
                    const hdr_count = MdTable.splitRow(line, hdr_cells[0..16]);
                    if (hdr_count == col_count) {
                        let th_cells = allocator.alloc(MdBlock, col_count)?;
                        let c: usize = 0;
                        while (c < col_count) : (c += 1) {
                            th_cells[c] = MdBlock{
                                kind: .TableHeaderCell,
                                cell_align: aligns[c],
                                text: hdr_cells[c],
                            };
                        }
                        let head_row = allocator.alloc(MdBlock, 1)?;
                        head_row[0] = MdBlock{
                            kind: .TableRow,
                            children: th_cells,
                        };
                        let thead = MdBlock{
                            kind: .TableHead,
                            children: head_row,
                        };

                        i += 2;
                        let body_rows: std.ArrayList(MdBlock) = .empty;
                        errdefer {
                            for (*r of body_rows.items) r.deinit(allocator);
                            body_rows.deinit(allocator);
                        }
                        while (i < lines.len) {
                            const rline = lines[i];
                            if (MdParser.isBlankLine(rline)) break;
                            if (MdParser.parseAtxHeading(rline) != null || MdParser.parseFenceOpen(rline) != null ||
                                MdParser.isThematicBreak(rline) || MdParser.isBlockQuoteLine(rline))
                            {
                                break;
                            }
                            if (std.mem.indexOfScalar(u8, rline, '|') == null) break;

                            let rcells: [16][]const u8 = undefined;
                            const rcount = MdTable.splitRow(rline, rcells[0..16]);
                            let td_cells = allocator.alloc(MdBlock, col_count)?;
                            let k: usize = 0;
                            while (k < col_count) : (k += 1) {
                                td_cells[k] = MdBlock{
                                    kind: .TableCell,
                                    cell_align: aligns[k],
                                    text: if (k < rcount) rcells[k] else "",
                                };
                            }
                            body_rows.append(allocator, MdBlock{
                                kind: .TableRow,
                                children: td_cells,
                            })?;
                            i += 1;
                        }

                        let table_children = allocator.alloc(MdBlock, 2)?;
                        table_children[0] = thead;
                        table_children[1] = MdBlock{
                            kind: .TableBody,
                            children: body_rows.toOwnedSlice(allocator)?,
                        };
                        blocks.append(allocator, MdBlock{
                            kind: .Table,
                            children: table_children,
                        })?;
                        continue;
                    }
                }
            }

            // 8. HTML block
            if (MdParser.isHtmlBlockStart(line)) {
                let html_lines: std.ArrayList(string) = .empty;
                defer html_lines.deinit(allocator);
                while (i < lines.len && !MdParser.isBlankLine(lines[i])) {
                    html_lines.append(allocator, lines[i])?;
                    i += 1;
                }
                const joined = MdParser.joinLinesOwned(allocator, html_lines.items)?;
                blocks.append(allocator, MdBlock{
                    kind: .HtmlBlock,
                    text: joined,
                    owns_text: true,
                })?;
                continue;
            }

            // 9. Link reference definition [label]: url "title"
            if (def := MdParser.parseLinkRefDef(line)) {
                link_defs.append(allocator, def)?;
                i += 1;
                continue;
            }

            // 10. Paragraph or Setext Heading
            let para_lines: std.ArrayList(string) = .empty;
            defer para_lines.deinit(allocator);
            para_lines.append(allocator, std.mem.trimStart(u8, line, " \t"))?;
            i += 1;
            let setext_level: u8 = 0;
            while (i < lines.len) {
                const cur = lines[i];
                if (MdParser.isBlankLine(cur)) break;
                const sl = MdParser.parseSetextUnderline(cur);
                if (sl > 0) {
                    setext_level = sl;
                    i += 1;
                    break;
                }
                if (MdParser.canInterruptParagraph(cur, opts)) break;
                para_lines.append(allocator, std.mem.trimStart(u8, cur, " \t"))?;
                i += 1;
            }

            const joined = MdParser.joinLinesOwned(allocator, para_lines.items)?;
            if (setext_level > 0) {
                blocks.append(allocator, MdBlock{
                    kind: .Heading,
                    level: setext_level,
                    text: joined,
                    owns_text: true,
                })?;
            } else {
                blocks.append(allocator, MdBlock{
                    kind: .Paragraph,
                    text: joined,
                    owns_text: true,
                })?;
            }
        }

        return blocks.toOwnedSlice(allocator);
    }

    pub static fn parse(allocator: std.mem.Allocator, md_text: []const u8, opts: MarkdownOptions): MdDocument! {
        let lines: std.ArrayList(string) = .empty;
        defer lines.deinit(allocator);
        let it = std.mem.splitScalar(u8, md_text, '\n');
        while (raw := it.next()) {
            lines.append(allocator, MdParser.trimCr(raw))?;
        }

        let link_defs: std.ArrayList(MdLinkDef) = .empty;
        errdefer link_defs.deinit(allocator);

        let top_blocks = MdParser.parseLines(allocator, lines.items, opts, &link_defs)?;
        if (opts.headingIds) {
            let seen_slugs: std.ArrayList([]u8) = .empty;
            defer {
                for (s of seen_slugs.items) allocator.free(s);
                seen_slugs.deinit(allocator);
            }
            MdParser.assignHeadingIds(allocator, top_blocks, &seen_slugs)?;
        }

        return MdDocument{
            allocator: allocator,
            root: MdBlock{
                kind: .Document,
                children: top_blocks,
            },
            link_defs: link_defs.toOwnedSlice(allocator)?,
            options: opts,
        };
    }

    // =========================================================================
    // HTML Escaping, XSS Protection & Inline Renderer
    // =========================================================================

    static fn writeHtmlEscaped(out: *std.ArrayList(u8), allocator: std.mem.Allocator, s: []const u8): ! {
        for (c of s) {
            switch (c) {
                '&' => out.appendSlice(allocator, "&amp;")?,
                '<' => out.appendSlice(allocator, "&lt;")?,
                '>' => out.appendSlice(allocator, "&gt;")?,
                '"' => out.appendSlice(allocator, "&quot;")?,
                else => out.append(allocator, c)?,
            }
        }
    }

    static fn isSafeUrl(url: []const u8, sanitize: bool): bool {
        if (!sanitize) return true;
        const trimmed = std.mem.trim(u8, url, " \t\r\n");
        if (trimmed.len >= 11 && std.ascii.eqlIgnoreCase(trimmed[0..11], "javascript:")) return false;
        if (trimmed.len >= 9 && std.ascii.eqlIgnoreCase(trimmed[0..9], "vbscript:")) return false;
        if (trimmed.len >= 14 && std.ascii.eqlIgnoreCase(trimmed[0..14], "data:text/html")) return false;
        return true;
    }

    static fn isDisallowedTag(tag_html: []const u8): bool {
        if (tag_html.len < 3 || tag_html[0] != '<') return false;
        let start: usize = 1;
        if (tag_html[start] == '/') start += 1;
        let end = start;
        while (end < tag_html.len && ((tag_html[end] >= 'a' && tag_html[end] <= 'z') || (tag_html[end] >= 'A' && tag_html[end] <= 'Z'))) {
            end += 1;
        }
        const name = tag_html[start..end];
        if (std.ascii.eqlIgnoreCase(name, "script")) return true;
        if (std.ascii.eqlIgnoreCase(name, "style")) return true;
        if (std.ascii.eqlIgnoreCase(name, "iframe")) return true;
        if (std.ascii.eqlIgnoreCase(name, "textarea")) return true;
        if (std.ascii.eqlIgnoreCase(name, "title")) return true;
        if (std.ascii.eqlIgnoreCase(name, "xmp")) return true;
        if (std.ascii.eqlIgnoreCase(name, "noembed")) return true;
        if (std.ascii.eqlIgnoreCase(name, "noframes")) return true;
        if (std.ascii.eqlIgnoreCase(name, "plaintext")) return true;
        return false;
    }

    static fn isAsciiPunct(c: u8): bool {
        return (c >= '!' && c <= '/') || (c >= ':' && c <= '@') || (c >= '[' && c <= '`') || (c >= '{' && c <= '~');
    }

    static fn isWhitespace(c: u8): bool {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    static fn isAlphaNum(c: u8): bool {
        return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    }

    static fn findLinkDef(defs: []const MdLinkDef, label: []const u8): MdLinkDef? {
        for (d of defs) {
            if (std.ascii.eqlIgnoreCase(d.label, label)) return d;
        }
        return null;
    }

    static fn matchInlineHtmlTagEnd(s: []const u8, start: usize): usize? {
        if (start + 2 >= s.len || s[start] != '<') return null;
        if (std.mem.startsWith(u8, s[start..], "<!--")) {
            const end_c = std.mem.indexOf(u8, s[start + 4 ..], "-->") ?? return null;
            return start + 4 + end_c + 3;
        }
        let i = start + 1;
        if (s[i] == '/') i += 1;
        if (i >= s.len || !((s[i] >= 'a' && s[i] <= 'z') || (s[i] >= 'A' && s[i] <= 'Z'))) return null;
        while (i < s.len && (MdParser.isAlphaNum(s[i]) || s[i] == '-')) i += 1;
        let q: u8 = 0;
        while (i < s.len) {
            const c = s[i];
            if (q != 0) {
                if (c == q) q = 0;
            } else {
                if (c == '"' || c == '\'') {
                    q = c;
                } else if (c == '>') {
                    return i + 1;
                } else if (c == '<' || c == '\n') {
                    return null;
                }
            }
            i += 1;
        }
        return null;
    }

    static fn matchAngleAutolink(s: []const u8, start: usize): usize? {
        if (start + 4 >= s.len || s[start] != '<') return null;
        let i = start + 1;
        while (i < s.len && s[i] != '>' && s[i] != '<' && s[i] != ' ' && s[i] != '\n' && s[i] != '\r') {
            i += 1;
        }
        if (i >= s.len || s[i] != '>') return null;
        const inner = s[start + 1 .. i];
        if (std.mem.startsWith(u8, inner, "http://") || std.mem.startsWith(u8, inner, "https://") ||
            std.mem.startsWith(u8, inner, "mailto:"))
        {
            return i + 1;
        }
        if (std.mem.indexOfScalar(u8, inner, '@') != null && std.mem.indexOfScalar(u8, inner, '.') != null) {
            return i + 1;
        }
        return null;
    }

    static fn tryParseLinkOrImage(s: []const u8, start: usize, defs: []const MdLinkDef): LinkParseResult? {
        // start points to '['
        let i = start + 1;
        let bracket_depth: usize = 1;
        while (i < s.len && bracket_depth > 0) {
            if (s[i] == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (s[i] == '`') {
                let run: usize = 0;
                while (i + run < s.len && s[i + run] == '`') run += 1;
                i += run;
                while (i < s.len) {
                    if (s[i] == '`') {
                        let close_run: usize = 0;
                        while (i + close_run < s.len && s[i + close_run] == '`') close_run += 1;
                        i += close_run;
                        if (close_run == run) break;
                    } else {
                        i += 1;
                    }
                }
                continue;
            }
            if (s[i] == '[') bracket_depth += 1;
            if (s[i] == ']') {
                bracket_depth -= 1;
                if (bracket_depth == 0) break;
            }
            i += 1;
        }
        if (bracket_depth != 0 || i >= s.len) return null;
        const link_text = s[start + 1 .. i];
        let after = i + 1;

        // Inline link: [text](url "title")
        if (after < s.len && s[after] == '(') {
            let p = after + 1;
            while (p < s.len && MdParser.isWhitespace(s[p])) p += 1;
            let url: []const u8 = "";
            if (p < s.len && s[p] == '<') {
                p += 1;
                const ustart = p;
                while (p < s.len && s[p] != '>' && s[p] != '\n') p += 1;
                if (p >= s.len || s[p] != '>') return null;
                url = s[ustart..p];
                p += 1;
            } else {
                const ustart = p;
                let paren_depth: usize = 0;
                while (p < s.len) {
                    const ch = s[p];
                    if (ch == '\\' && p + 1 < s.len) {
                        p += 2;
                        continue;
                    }
                    if (MdParser.isWhitespace(ch)) break;
                    if (ch == '(') {
                        paren_depth += 1;
                    } else if (ch == ')') {
                        if (paren_depth == 0) break;
                        paren_depth -= 1;
                    }
                    p += 1;
                }
                url = s[ustart..p];
            }
            while (p < s.len && MdParser.isWhitespace(s[p])) p += 1;
            let title: []const u8 = "";
            if (p < s.len && (s[p] == '"' || s[p] == '\'' || s[p] == '(')) {
                const close_q: u8 = if (s[p] == '(') ')' else s[p];
                p += 1;
                const tstart = p;
                while (p < s.len && s[p] != close_q) {
                    if (s[p] == '\\' && p + 1 < s.len) {
                        p += 2;
                        continue;
                    }
                    p += 1;
                }
                if (p >= s.len || s[p] != close_q) return null;
                title = s[tstart..p];
                p += 1;
                while (p < s.len && MdParser.isWhitespace(s[p])) p += 1;
            }
            if (p >= s.len || s[p] != ')') return null;
            return LinkParseResult{
                text: link_text,
                url: url,
                title: title,
                next_pos: p + 1,
            };
        }

        // Reference link: [text][ref] or [label][]
        if (after < s.len && s[after] == '[') {
            const close_ref = std.mem.indexOfScalar(u8, s[after + 1 ..], ']') ?? return null;
            const raw_ref = s[after + 1 .. after + 1 + close_ref];
            const lookup_label = if (raw_ref.len == 0) link_text else raw_ref;
            const def = MdParser.findLinkDef(defs, lookup_label) ?? return null;
            return LinkParseResult{
                text: link_text,
                url: def.url,
                title: def.title,
                next_pos: after + 1 + close_ref + 1,
            };
        }

        return null;
    }

    static fn findMatchingDelimiter(s: []const u8, start_after: usize, ch: u8, count: usize): usize? {
        let i = start_after;
        while (i < s.len) {
            if (s[i] == '\\' && i + 1 < s.len) {
                i += 2;
                continue;
            }
            if (s[i] == '`') {
                let run: usize = 0;
                while (i + run < s.len && s[i + run] == '`') run += 1;
                i += run;
                while (i < s.len) {
                    if (s[i] == '`') {
                        let cr: usize = 0;
                        while (i + cr < s.len && s[i + cr] == '`') cr += 1;
                        i += cr;
                        if (cr == run) break;
                    } else {
                        i += 1;
                    }
                }
                continue;
            }
            if (s[i] == ch) {
                let run: usize = 0;
                while (i + run < s.len && s[i + run] == ch) run += 1;
                if (run >= count) {
                    const before_ws = (i == 0) || MdParser.isWhitespace(s[i - 1]);
                    const after_alnum = (i + run < s.len) && MdParser.isAlphaNum(s[i + run]);
                    // Intraword '_' cannot close emphasis inside a word
                    if (!before_ws && !(ch == '_' && after_alnum)) {
                        return i;
                    }
                }
                i += run;
                continue;
            }
            i += 1;
        }
        return null;
    }

    static fn canOpenDelimiter(s: []const u8, pos: usize, run: usize, ch: u8): bool {
        if (pos + run >= s.len) return false;
        if (MdParser.isWhitespace(s[pos + run])) return false;
        // CommonMark intraword '_' rule: cannot open if preceded by alphanumeric
        if (ch == '_' && pos > 0 && MdParser.isAlphaNum(s[pos - 1])) {
            return false;
        }
        return true;
    }

    static fn writePlainAltText(out: *std.ArrayList(u8), allocator: std.mem.Allocator, s: []const u8): ! {
        let i: usize = 0;
        while (i < s.len) {
            if (s[i] == '\\' && i + 1 < s.len && MdParser.isAsciiPunct(s[i + 1])) {
                MdParser.writeHtmlEscaped(out, allocator, s[i + 1 .. i + 2])?;
                i += 2;
                continue;
            }
            if (s[i] == '*' || s[i] == '_' || s[i] == '`') {
                i += 1;
                continue;
            }
            MdParser.writeHtmlEscaped(out, allocator, s[i .. i + 1])?;
            i += 1;
        }
    }

    pub static fn renderInlineToList(
        out: *std.ArrayList(u8),
        allocator: std.mem.Allocator,
        s: []const u8,
        defs: []const MdLinkDef,
        opts: MarkdownOptions,
        in_link: bool,
    ): ! {
        let i: usize = 0;
        while (i < s.len) {
            const c = s[i];

            // 1. Backslash escapes & hard break `\\\n`
            if (c == '\\' && i + 1 < s.len) {
                if (s[i + 1] == '\n') {
                    out.appendSlice(allocator, "<br />\n")?;
                    i += 2;
                    continue;
                }
                if (MdParser.isAsciiPunct(s[i + 1])) {
                    MdParser.writeHtmlEscaped(out, allocator, s[i + 1 .. i + 2])?;
                    i += 2;
                    continue;
                }
            }

            // 2. Hard line break `  \n` and soft break `\n`
            if (c == ' ' && i + 2 < s.len && s[i + 1] == ' ') {
                let sp_end = i;
                while (sp_end < s.len && s[sp_end] == ' ') sp_end += 1;
                if (sp_end < s.len && s[sp_end] == '\n') {
                    out.appendSlice(allocator, "<br />\n")?;
                    i = sp_end + 1;
                    continue;
                }
            }
            if (c == '\n') {
                if (opts.hard_soft_breaks) {
                    out.appendSlice(allocator, "<br />\n")?;
                } else {
                    out.append(allocator, '\n')?;
                }
                i += 1;
                continue;
            }

            // 3. Inline code span (`code` or ``code with ` inside``)
            if (c == '`') {
                let open_run: usize = 0;
                while (i + open_run < s.len && s[i + open_run] == '`') open_run += 1;
                let scan = i + open_run;
                let found_end: usize? = null;
                while (scan < s.len) {
                    if (s[scan] == '`') {
                        let close_run: usize = 0;
                        while (scan + close_run < s.len && s[scan + close_run] == '`') close_run += 1;
                        if (close_run == open_run) {
                            found_end = scan;
                            break;
                        }
                        scan += close_run;
                    } else {
                        scan += 1;
                    }
                }
                if (end_idx := found_end) {
                    let inner = s[i + open_run .. end_idx];
                    // Strip single leading and trailing space if both present and not all spaces
                    if (inner.len >= 2 && (inner[0] == ' ' || inner[0] == '\n') && (inner[inner.len - 1] == ' ' || inner[inner.len - 1] == '\n')) {
                        let all_space = true;
                        for (ic of inner) {
                            if (ic != ' ' && ic != '\n') {
                                all_space = false;
                                break;
                            }
                        }
                        if (!all_space) {
                            inner = inner[1 .. inner.len - 1];
                        }
                    }
                    out.appendSlice(allocator, "<code>")?;
                    let ci: usize = 0;
                    while (ci < inner.len) : (ci += 1) {
                        if (inner[ci] == '\n') {
                            out.append(allocator, ' ')?;
                        } else {
                            MdParser.writeHtmlEscaped(out, allocator, inner[ci .. ci + 1])?;
                        }
                    }
                    out.appendSlice(allocator, "</code>")?;
                    i = end_idx + open_run;
                    continue;
                }
                // Unmatched backticks: emit literally
                out.appendSlice(allocator, s[i .. i + open_run])?;
                i += open_run;
                continue;
            }

            // 4. Image ![alt](src "title")
            if (c == '!' && i + 1 < s.len && s[i + 1] == '[') {
                if (img := MdParser.tryParseLinkOrImage(s, i + 1, defs)) {
                    const safe_url = if (MdParser.isSafeUrl(img.url, opts.sanitizeHtml)) img.url else "";
                    out.appendSlice(allocator, "<img src=\"")?;
                    MdParser.writeHtmlEscaped(out, allocator, safe_url)?;
                    out.appendSlice(allocator, "\" alt=\"")?;
                    MdParser.writePlainAltText(out, allocator, img.text)?;
                    out.append(allocator, '"')?;
                    if (img.title.len > 0) {
                        out.appendSlice(allocator, " title=\"")?;
                        MdParser.writeHtmlEscaped(out, allocator, img.title)?;
                        out.append(allocator, '"')?;
                    }
                    out.appendSlice(allocator, " />")?;
                    i = img.next_pos;
                    continue;
                }
            }

            // 5. Link [text](url "title") or [text][ref]
            if (c == '[' && !in_link) {
                if (lnk := MdParser.tryParseLinkOrImage(s, i, defs)) {
                    const safe_url = if (MdParser.isSafeUrl(lnk.url, opts.sanitizeHtml)) lnk.url else "#";
                    out.appendSlice(allocator, "<a href=\"")?;
                    MdParser.writeHtmlEscaped(out, allocator, safe_url)?;
                    out.append(allocator, '"')?;
                    if (lnk.title.len > 0) {
                        out.appendSlice(allocator, " title=\"")?;
                        MdParser.writeHtmlEscaped(out, allocator, lnk.title)?;
                        out.append(allocator, '"')?;
                    }
                    out.append(allocator, '>')?;
                    MdParser.renderInlineToList(out, allocator, lnk.text, defs, opts, true)?;
                    out.appendSlice(allocator, "</a>")?;
                    i = lnk.next_pos;
                    continue;
                }
            }

            // 6. Angle-bracket autolink <https://...> / <user@domain> or raw inline HTML
            if (c == '<') {
                if (al_end := MdParser.matchAngleAutolink(s, i)) {
                    const inner = s[i + 1 .. al_end - 1];
                    const is_email = std.mem.indexOfScalar(u8, inner, '@') != null &&
                        !std.mem.startsWith(u8, inner, "http://") &&
                        !std.mem.startsWith(u8, inner, "https://") &&
                        !std.mem.startsWith(u8, inner, "mailto:");
                    out.appendSlice(allocator, "<a href=\"")?;
                    if (is_email) out.appendSlice(allocator, "mailto:")?;
                    MdParser.writeHtmlEscaped(out, allocator, inner)?;
                    out.appendSlice(allocator, "\">")?;
                    MdParser.writeHtmlEscaped(out, allocator, inner)?;
                    out.appendSlice(allocator, "</a>")?;
                    i = al_end;
                    continue;
                }
                if (tag_end := MdParser.matchInlineHtmlTagEnd(s, i)) {
                    const raw_tag = s[i..tag_end];
                    if (opts.sanitizeHtml || (opts.tag_filter && MdParser.isDisallowedTag(raw_tag))) {
                        MdParser.writeHtmlEscaped(out, allocator, raw_tag)?;
                    } else {
                        out.appendSlice(allocator, raw_tag)?;
                    }
                    i = tag_end;
                    continue;
                }
            }

            // 7. GFM Permissive Autolinks (https://... / http://... / www....)
            if (opts.autolinks && !in_link && (i == 0 || MdParser.isWhitespace(s[i - 1]) || s[i - 1] == '(' || s[i - 1] == '[')) {
                const rem = s[i..];
                const is_http = std.mem.startsWith(u8, rem, "https://") || std.mem.startsWith(u8, rem, "http://");
                const is_www = std.mem.startsWith(u8, rem, "www.");
                if (is_http || is_www) {
                    let end = i;
                    while (end < s.len && !MdParser.isWhitespace(s[end]) && s[end] != '<' && s[end] != '>' && s[end] != '"') {
                        end += 1;
                    }
                    // Strip trailing punctuation .,;:!?
                    while (end > i && (s[end - 1] == '.' || s[end - 1] == ',' || s[end - 1] == ';' || s[end - 1] == ':' || s[end - 1] == '!' || s[end - 1] == '?')) {
                        end -= 1;
                    }
                    if (end > i && s[end - 1] == ')') {
                        let opens: usize = 0;
                        let closes: usize = 0;
                        for (uc of s[i..end]) {
                            if (uc == '(') opens += 1;
                            if (uc == ')') closes += 1;
                        }
                        if (closes > opens) end -= 1;
                    }
                    const url = s[i..end];
                    if (url.len > 8) {
                        out.appendSlice(allocator, "<a href=\"")?;
                        if (is_www) out.appendSlice(allocator, "http://")?;
                        MdParser.writeHtmlEscaped(out, allocator, url)?;
                        out.appendSlice(allocator, "\">")?;
                        MdParser.writeHtmlEscaped(out, allocator, url)?;
                        out.appendSlice(allocator, "</a>")?;
                        i = end;
                        continue;
                    }
                }
            }

            // 8. Emphasis / Strong / Strikethrough (`*`, `_`, `~~`)
            if (c == '*' || c == '_' || (c == '~' && opts.strikethrough)) {
                let run: usize = 0;
                while (i + run < s.len && s[i + run] == c) run += 1;

                if (c == '~') {
                    if ((run == 1 || run == 2) && MdParser.canOpenDelimiter(s, i, run, '~')) {
                        if (close_pos := MdParser.findMatchingDelimiter(s, i + run, '~', run)) {
                            out.appendSlice(allocator, "<del>")?;
                            MdParser.renderInlineToList(out, allocator, s[i + run .. close_pos], defs, opts, in_link)?;
                            out.appendSlice(allocator, "</del>")?;
                            i = close_pos + run;
                            continue;
                        }
                    }
                    out.appendSlice(allocator, s[i .. i + run])?;
                    i += run;
                    continue;
                }

                if (MdParser.canOpenDelimiter(s, i, run, c)) {
                    if (run >= 3) {
                        if (close_pos := MdParser.findMatchingDelimiter(s, i + 3, c, 3)) {
                            out.appendSlice(allocator, "<em><strong>")?;
                            MdParser.renderInlineToList(out, allocator, s[i + 3 .. close_pos], defs, opts, in_link)?;
                            out.appendSlice(allocator, "</strong></em>")?;
                            i = close_pos + 3;
                            continue;
                        }
                    }
                    if (run >= 2) {
                        if (close_pos := MdParser.findMatchingDelimiter(s, i + 2, c, 2)) {
                            out.appendSlice(allocator, "<strong>")?;
                            MdParser.renderInlineToList(out, allocator, s[i + 2 .. close_pos], defs, opts, in_link)?;
                            out.appendSlice(allocator, "</strong>")?;
                            i = close_pos + 2;
                            continue;
                        }
                    }
                    if (run >= 1) {
                        if (close_pos := MdParser.findMatchingDelimiter(s, i + 1, c, 1)) {
                            out.appendSlice(allocator, "<em>")?;
                            MdParser.renderInlineToList(out, allocator, s[i + 1 .. close_pos], defs, opts, in_link)?;
                            out.appendSlice(allocator, "</em>")?;
                            i = close_pos + 1;
                            continue;
                        }
                    }
                }
                out.appendSlice(allocator, s[i .. i + run])?;
                i += run;
                continue;
            }

            // 9. Plain text (XSS-safe HTML escaping)
            MdParser.writeHtmlEscaped(out, allocator, s[i .. i + 1])?;
            i += 1;
        }
    }

    static fn renderBlockHtml(
        out: *std.ArrayList(u8),
        allocator: std.mem.Allocator,
        blk: MdBlock,
        defs: []const MdLinkDef,
        opts: MarkdownOptions,
    ): ! {
        switch (blk.kind) {
            .Document => {
                for (child of blk.children) {
                    MdParser.renderBlockHtml(out, allocator, child, defs, opts)?;
                }
            },
            .Heading => {
                let tag_buf: [16]u8 = undefined;
                const lvl = if (blk.level >= 1 && blk.level <= 6) blk.level else (1 as u8);
                if (opts.headingIds && blk.heading_id.len > 0) {
                    const open_s = std.fmt.bufPrint(&tag_buf, "<h{d} id=\"", .{lvl}) catch unreachable;
                    out.appendSlice(allocator, open_s)?;
                    MdParser.writeHtmlEscaped(out, allocator, blk.heading_id)?;
                    out.appendSlice(allocator, "\">")?;
                } else {
                    const open_s = std.fmt.bufPrint(&tag_buf, "<h{d}>", .{lvl}) catch unreachable;
                    out.appendSlice(allocator, open_s)?;
                }
                MdParser.renderInlineToList(out, allocator, blk.text, defs, opts, false)?;
                const close_s = std.fmt.bufPrint(&tag_buf, "</h{d}>\n", .{lvl}) catch unreachable;
                out.appendSlice(allocator, close_s)?;
            },
            .Paragraph => {
                out.appendSlice(allocator, "<p>")?;
                MdParser.renderInlineToList(out, allocator, blk.text, defs, opts, false)?;
                out.appendSlice(allocator, "</p>\n")?;
            },
            .CodeBlock => {
                out.appendSlice(allocator, "<pre><code")?;
                if (blk.is_fenced && blk.fence_lang.len > 0) {
                    out.appendSlice(allocator, " class=\"language-")?;
                    MdParser.writeHtmlEscaped(out, allocator, blk.fence_lang)?;
                    out.append(allocator, '"')?;
                }
                out.append(allocator, '>')?;
                MdParser.writeHtmlEscaped(out, allocator, blk.text)?;
                if (blk.text.len > 0) out.append(allocator, '\n')?;
                out.appendSlice(allocator, "</code></pre>\n")?;
            },
            .ThematicBreak => {
                out.appendSlice(allocator, "<hr />\n")?;
            },
            .BlockQuote => {
                out.appendSlice(allocator, "<blockquote>\n")?;
                for (child of blk.children) {
                    MdParser.renderBlockHtml(out, allocator, child, defs, opts)?;
                }
                out.appendSlice(allocator, "</blockquote>\n")?;
            },
            .BulletList => {
                out.appendSlice(allocator, "<ul>\n")?;
                for (item of blk.children) {
                    MdParser.renderListItemHtml(out, allocator, item, blk.tight, defs, opts)?;
                }
                out.appendSlice(allocator, "</ul>\n")?;
            },
            .OrderedList => {
                if (blk.start_num == 1) {
                    out.appendSlice(allocator, "<ol>\n")?;
                } else {
                    let buf: [32]u8 = undefined;
                    const s = std.fmt.bufPrint(&buf, "<ol start=\"{d}\">\n", .{blk.start_num}) catch unreachable;
                    out.appendSlice(allocator, s)?;
                }
                for (item of blk.children) {
                    MdParser.renderListItemHtml(out, allocator, item, blk.tight, defs, opts)?;
                }
                out.appendSlice(allocator, "</ol>\n")?;
            },
            .ListItem => {
                MdParser.renderListItemHtml(out, allocator, blk, true, defs, opts)?;
            },
            .Table => {
                out.appendSlice(allocator, "<table>\n")?;
                for (child of blk.children) {
                    MdParser.renderBlockHtml(out, allocator, child, defs, opts)?;
                }
                out.appendSlice(allocator, "</table>\n")?;
            },
            .TableHead => {
                out.appendSlice(allocator, "<thead>\n")?;
                for (child of blk.children) {
                    MdParser.renderBlockHtml(out, allocator, child, defs, opts)?;
                }
                out.appendSlice(allocator, "</thead>\n")?;
            },
            .TableBody => {
                out.appendSlice(allocator, "<tbody>\n")?;
                for (child of blk.children) {
                    MdParser.renderBlockHtml(out, allocator, child, defs, opts)?;
                }
                out.appendSlice(allocator, "</tbody>\n")?;
            },
            .TableRow => {
                out.appendSlice(allocator, "<tr>")?;
                for (child of blk.children) {
                    MdParser.renderBlockHtml(out, allocator, child, defs, opts)?;
                }
                out.appendSlice(allocator, "</tr>\n")?;
            },
            .TableHeaderCell => {
                out.appendSlice(allocator, "<th")?;
                out.appendSlice(allocator, MdTable.alignAttr(blk.cell_align))?;
                out.append(allocator, '>')?;
                MdParser.renderInlineToList(out, allocator, blk.text, defs, opts, false)?;
                out.appendSlice(allocator, "</th>")?;
            },
            .TableCell => {
                out.appendSlice(allocator, "<td")?;
                out.appendSlice(allocator, MdTable.alignAttr(blk.cell_align))?;
                out.append(allocator, '>')?;
                MdParser.renderInlineToList(out, allocator, blk.text, defs, opts, false)?;
                out.appendSlice(allocator, "</td>")?;
            },
            .HtmlBlock => {
                if (opts.sanitizeHtml) {
                    MdParser.writeHtmlEscaped(out, allocator, blk.text)?;
                    out.append(allocator, '\n')?;
                } else if (opts.tag_filter && MdParser.isDisallowedTag(std.mem.trimStart(u8, blk.text, " \t"))) {
                    MdParser.writeHtmlEscaped(out, allocator, blk.text)?;
                    out.append(allocator, '\n')?;
                } else {
                    out.appendSlice(allocator, blk.text)?;
                    out.append(allocator, '\n')?;
                }
            },
        }
    }

    static fn renderListItemHtml(
        out: *std.ArrayList(u8),
        allocator: std.mem.Allocator,
        item: MdBlock,
        tight: bool,
        defs: []const MdLinkDef,
        opts: MarkdownOptions,
    ): ! {
        if (item.is_task) {
            out.appendSlice(allocator, "<li class=\"task-list-item\"><input type=\"checkbox\" class=\"task-list-item-checkbox\" disabled")?;
            if (item.task_checked) {
                out.appendSlice(allocator, " checked")?;
            }
            out.appendSlice(allocator, " /> ")?;
        } else {
            out.appendSlice(allocator, "<li>")?;
        }

        let idx: usize = 0;
        while (idx < item.children.len) : (idx += 1) {
            const child = item.children[idx];
            if (tight && child.kind == .Paragraph) {
                MdParser.renderInlineToList(out, allocator, child.text, defs, opts, false)?;
                if (idx + 1 < item.children.len) {
                    out.append(allocator, '\n')?;
                }
            } else {
                if (idx == 0 && item.children.len > 1 && !tight) {
                    out.append(allocator, '\n')?;
                }
                MdParser.renderBlockHtml(out, allocator, child, defs, opts)?;
            }
        }
        out.appendSlice(allocator, "</li>\n")?;
    }

    pub static fn renderDocumentToHtml(
        allocator: std.mem.Allocator,
        root: MdBlock,
        defs: []const MdLinkDef,
        opts: MarkdownOptions,
    ): ([]u8)! {
        let out: std.ArrayList(u8) = .empty;
        errdefer out.deinit(allocator);
        MdParser.renderBlockHtml(&out, allocator, root, defs, opts)?;
        return out.toOwnedSlice(allocator);
    }
}
