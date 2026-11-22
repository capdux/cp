// Source: Absorbed from /data/dev/bun/src/md/autolinks.cp | Synced: 2026-11-22T13:35:10Z
// lib/md/autolinks.cp
// GFM Permissive URL, Email & WWW Autolink Scanner + Boundary/Balanced-Paren Validator

const std = @import("std");
import { isAlphaNum, asciiCaseEql } from "./helpers.cp";
import { EmphDelim } from "./types.cp";

pub fn isListBullet(c: u8): bool {
    return c == '-' || c == '+' || c == '*';
}

pub fn isListItemMark(c: u8): bool {
    return c == '-' || c == '+' || c == '*' || c == '.' || c == ')';
}

pub struct AutolinkSpan {
    beg: usize = 0,
    end: usize = 0,
}

pub struct ScanUrlResult {
    end: usize = 0,
    ok: bool = false,
}

struct Scheme {
    name: []const u8 = "",
    suffix: []const u8 = "",
}

pub fn isEmphBoundaryResolved(content: []const u8, al: AutolinkSpan, resolved: []const EmphDelim): bool {
    if (al.beg > 0) {
        const prev = content[al.beg - 1];
        if (prev == '*' || prev == '_' || prev == '~') {
            if (!checkLeftBoundary(content, al.beg, false)) {
                let found_resolved = false;
                for (d of resolved) {
                    if (d.pos <= al.beg - 1 && al.beg - 1 < d.pos + d.count && (d.open_count + d.close_count > 0)) {
                        found_resolved = true;
                        break;
                    }
                }
                if (!found_resolved) return false;
            }
        }
    }
    if (al.end < content.len) {
        const next = content[al.end];
        if (next == '*' || next == '_' || next == '~') {
            if (!checkRightBoundary(content, al.end, false)) {
                let found_resolved = false;
                for (d of resolved) {
                    if (d.pos <= al.end && al.end < d.pos + d.count && (d.open_count + d.close_count > 0)) {
                        found_resolved = true;
                        break;
                    }
                }
                if (!found_resolved) return false;
            }
        }
    }
    return true;
}

pub fn isInSet(c: u8, set: []const u8): bool {
    for (s of set) {
        if (c == s) return true;
    }
    return false;
}

pub fn scanUrlComponent(
    content: []const u8,
    start: usize,
    start_char: u8,
    delim_char: u8,
    allowed_nonalnum: []const u8,
    min_components: u32,
    optional_end_char: u8,
): ScanUrlResult {
    let pos = start;
    let n_components: u32 = 0;
    if (start_char != 0) {
        if (pos >= content.len || content[pos] != start_char) {
            return ScanUrlResult{ end: pos, ok: min_components == 0 };
        }
        if (min_components > 0 && (pos + 1 >= content.len || !isAlphaNum(content[pos + 1]))) {
            return ScanUrlResult{ end: pos, ok: min_components == 0 };
        }
        pos += 1;
    }

    while (pos < content.len) {
        if (isAlphaNum(content[pos])) {
            if (n_components == 0) n_components = 1;
            pos += 1;
        } else if (isInSet(content[pos], allowed_nonalnum) &&
            ((pos > 0 && (isAlphaNum(content[pos - 1]) || content[pos - 1] == ')' || isInSet(content[pos - 1], allowed_nonalnum))) || content[pos] == '(') &&
            ((pos + 1 < content.len && (isAlphaNum(content[pos + 1]) || content[pos + 1] == '(' || isInSet(content[pos + 1], allowed_nonalnum))) || content[pos] == ')'))
        {
            if (content[pos] == delim_char) n_components += 1;
            pos += 1;
        } else {
            break;
        }
    }

    if (pos < content.len && optional_end_char != 0 && content[pos] == optional_end_char) {
        pos += 1;
    }

    if (n_components < min_components) {
        return ScanUrlResult{ end: pos, ok: false };
    }
    return ScanUrlResult{ end: pos, ok: true };
}

pub fn checkLeftBoundary(content: []const u8, pos: usize, allow_emph: bool): bool {
    if (pos == 0) return true;
    return switch (content[pos - 1]) {
        ' ', '\t', '\n', '\r', 0x0B, 0x0C => true,
        '(', '{', '[' => true,
        '*', '_', '~' => allow_emph,
        else => false,
    };
}

pub fn checkRightBoundary(content: []const u8, pos: usize, allow_emph: bool): bool {
    if (pos >= content.len) return true;
    return switch (content[pos]) {
        ' ', '\t', '\n', '\r', 0x0B, 0x0C => true,
        ')', '}', ']', '<' => true,
        '.', '!', '?', ',', ';', '&' => true,
        '*', '_', '~' => allow_emph,
        else => false,
    };
}

pub fn findPermissiveAutolink(content: []const u8, pos: usize, allow_emph: bool): AutolinkSpan? {
    if (pos >= content.len) return null;
    const c = content[pos];

    if (c == ':') {
        const schemes: [3]Scheme = [
            Scheme{ name: "http", suffix: "//" },
            Scheme{ name: "https", suffix: "//" },
            Scheme{ name: "ftp", suffix: "//" },
        ];

        for (scheme of schemes) {
            const slen = scheme.name.len;
            const suflen = scheme.suffix.len;
            if (pos >= slen && pos + 1 + suflen < content.len) {
                if (asciiCaseEql(content[pos - slen .. pos], scheme.name) &&
                    std.mem.eql(u8, content[pos + 1 .. pos + 1 + suflen], scheme.suffix))
                {
                    const beg = pos - slen;
                    if (!checkLeftBoundary(content, beg, allow_emph)) continue;

                    let end = pos + 1 + suflen;
                    const host = scanUrlComponent(content, end, 0, '.', ".-_", 2, 0);
                    if (!host.ok) continue;
                    end = host.end;

                    const path = scanUrlComponent(content, end, '/', '/', "/.-_~*+%", 0, '/');
                    end = path.end;

                    const query = scanUrlComponent(content, end, '?', '&', "&.-+_=()~*%", 1, 0);
                    end = query.end;

                    const frag = scanUrlComponent(content, end, '#', 0, ".-+_~*%", 1, 0);
                    end = frag.end;

                    end = postProcessAutolinkEnd(content, beg, end);
                    if (!checkRightBoundary(content, end, allow_emph)) continue;

                    return AutolinkSpan{ beg: beg, end: end };
                }
            }
        }
    } else if (c == '@') {
        if (pos == 0 || pos + 3 >= content.len) return null;
        if (!isAlphaNum(content[pos - 1]) || !isAlphaNum(content[pos + 1])) return null;

        let beg = pos;
        while (beg > 0) {
            if (isAlphaNum(content[beg - 1])) {
                beg -= 1;
            } else if (beg >= 2 && isAlphaNum(content[beg - 2]) && isInSet(content[beg - 1], ".-_+") && isAlphaNum(content[beg])) {
                beg -= 1;
            } else {
                break;
            }
        }
        if (beg == pos) return null;
        if (!checkLeftBoundary(content, beg, allow_emph)) return null;

        const host = scanUrlComponent(content, pos + 1, 0, '.', ".-_", 2, 0);
        if (!host.ok) return null;
        const end = host.end;

        if (!checkRightBoundary(content, end, allow_emph)) return null;
        return AutolinkSpan{ beg: beg, end: end };
    } else if (c == '.') {
        if (pos < 3) return null;
        if (!asciiCaseEql(content[pos - 3 .. pos], "www")) return null;

        const beg = pos - 3;
        if (!checkLeftBoundary(content, beg, allow_emph)) return null;

        let end = pos + 1;
        const host = scanUrlComponent(content, end, 0, '.', ".-_", 1, 0);
        if (!host.ok) return null;
        end = host.end;

        const path = scanUrlComponent(content, end, '/', '/', "/.-_~*+%", 0, '/');
        end = path.end;

        const query = scanUrlComponent(content, end, '?', '&', "&.-+_=()~*%", 1, 0);
        end = query.end;

        const frag = scanUrlComponent(content, end, '#', 0, ".-+_~*%", 1, 0);
        end = frag.end;

        end = postProcessAutolinkEnd(content, beg, end);
        if (!checkRightBoundary(content, end, allow_emph)) return null;
        return AutolinkSpan{ beg: beg, end: end };
    }

    return null;
}

fn postProcessAutolinkEnd(content: []const u8, beg: usize, end_in: usize): usize {
    let end = end_in;

    if (end > beg && content[end - 1] == ';') {
        let j = end - 2;
        while (j > beg && isAlphaNum(content[j])) j -= 1;
        if (j >= beg && content[j] == '&') {
            end = j;
        }
    }
    if (end < content.len && content[end] == ';' && end > beg) {
        let j = end - 1;
        while (j > beg && isAlphaNum(content[j])) j -= 1;
        if (j >= beg && content[j] == '&') {
            end = j;
        }
    }

    while (end > beg && content[end - 1] == ')') {
        let open: i32 = 0;
        let close: i32 = 0;
        for (ch of content[beg..end]) {
            if (ch == '(') open += 1;
            if (ch == ')') close += 1;
        }
        if (close > open) {
            end -= 1;
        } else {
            break;
        }
    }

    return end;
}
