// Source: /data/dev/bun/src/shell/builtin/seq.cp, yes.cp & Builtin.cp
// ============================================================================
// lib/shell/builtin_text.cp
// Text, Sequence & Filter Built-in Commands for CP Shell (cap:shell)
// Implements: seq, head, tail, wc, grep (-i/-v/-c/-n/-l/-q/-F/-E), yes, sort, uniq, tr.

const std = @import("std");
import { StringJoiner } from "cap:string";
import { parseShellInt64, pushShellUsize, pushShellI64 } from "./env.cp";
import { readFileInto } from "./builtin_fs.cp";

fn writeHeadContent(content: []const u8, max_lines: usize, out: *StringJoiner) {
    if (max_lines == 0) return;
    let lines_seen: usize = 0;
    let i: usize = 0;
    while (i < content.len) {
        if (content[i] == '\n') {
            lines_seen += 1;
            if (lines_seen >= max_lines) {
                out.push(content[0 .. i + 1]);
                return;
            }
        }
        i += 1;
    }
    out.push(content);
}

fn writeTailContent(content: []const u8, max_lines: usize, out: *StringJoiner) {
    if (max_lines == 0 || content.len == 0) return;
    let pos = content.len;
    if (pos > 0 && content[pos - 1] == '\n') {
        pos -= 1;
    }
    let lines_found: usize = 0;
    while (pos > 0) {
        if (content[pos - 1] == '\n') {
            lines_found += 1;
            if (lines_found >= max_lines) {
                out.push(content[pos..]);
                return;
            }
        }
        pos -= 1;
    }
    out.push(content);
}

fn countWc(content: []const u8): { lines: usize, words: usize, bytes: usize } {
    let lines: usize = 0;
    let words: usize = 0;
    let in_word = false;
    let i: usize = 0;
    while (i < content.len) {
        const c = content[i];
        if (c == '\n') lines += 1;
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == 0x0b || c == 0x0c) {
            in_word = false;
        } else if (!in_word) {
            in_word = true;
            words += 1;
        }
        i += 1;
    }
    return { lines: lines, words: words, bytes: content.len };
}

pub fn runBuiltinSeq(argv: []const []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    if (argv.len < 2) {
        stderr.push("seq: missing operand\n");
        return 1;
    }
    let sep: []const u8 = "\n";
    let term: []const u8 = "";
    let fixed_width = false;
    let idx: usize = 1;
    while (idx < argv.len) {
        const a = argv[idx];
        if (std.mem.eql(u8, a, "-s") || std.mem.eql(u8, a, "--separator")) {
            if (idx + 1 >= argv.len) {
                stderr.push("seq: option requires an argument -- s\n");
                return 1;
            }
            sep = argv[idx + 1];
            idx += 2;
        } else if (std.mem.startsWith(u8, a, "-s") && a.len > 2) {
            sep = a[2..];
            idx += 1;
        } else if (std.mem.eql(u8, a, "-t") || std.mem.eql(u8, a, "--terminator")) {
            if (idx + 1 >= argv.len) {
                stderr.push("seq: option requires an argument -- t\n");
                return 1;
            }
            term = argv[idx + 1];
            idx += 2;
        } else if (std.mem.eql(u8, a, "-w") || std.mem.eql(u8, a, "--fixed-width")) {
            fixed_width = true;
            idx += 1;
        } else {
            break;
        }
    }

    const rem = argv.len - idx;
    if (rem == 0) {
        stderr.push("seq: missing operand\n");
        return 1;
    }

    // Fast exact integer path when all args are integers
    let start_i: i64 = 1;
    let inc_i: i64 = 1;
    let end_i: i64 = 1;
    let all_ints = true;
    if (rem == 1) {
        if (v1 := parseShellInt64(argv[idx])) {
            end_i = v1;
            if (start_i > end_i) inc_i = -1;
        } else {
            all_ints = false;
        }
    } else if (rem == 2) {
        if (v1 := parseShellInt64(argv[idx])) {
            if (v2 := parseShellInt64(argv[idx + 1])) {
                start_i = v1;
                end_i = v2;
                inc_i = if (start_i <= end_i) 1 else -1;
            } else { all_ints = false; }
        } else { all_ints = false; }
    } else {
        if (v1 := parseShellInt64(argv[idx])) {
            if (v2 := parseShellInt64(argv[idx + 1])) {
                if (v3 := parseShellInt64(argv[idx + 2])) {
                    start_i = v1;
                    inc_i = v2;
                    end_i = v3;
                } else { all_ints = false; }
            } else { all_ints = false; }
        } else { all_ints = false; }
    }

    if (all_ints) {
        if (inc_i == 0) {
            stderr.push("seq: zero increment\n");
            return 1;
        }
        let max_width: usize = 0;
        if (fixed_width) {
            let tmp1: [32]u8 = [0; 32];
            let tmp2: [32]u8 = [0; 32];
            const s1 = std.fmt.bufPrint(tmp1[0..32], "{d}", .{start_i}) catch "";
            const s2 = std.fmt.bufPrint(tmp2[0..32], "{d}", .{end_i}) catch "";
            max_width = if (s1.len > s2.len) s1.len else s2.len;
        }
        let cur = start_i;
        let first = true;
        let count: usize = 0;
        while (count < 100000) {
            if (inc_i > 0 && cur > end_i) break;
            if (inc_i < 0 && cur < end_i) break;
            if (!first) stdout.push(sep);
            first = false;
            if (fixed_width && cur >= 0) {
                let tmp: [32]u8 = [0; 32];
                const num_s = std.fmt.bufPrint(tmp[0..32], "{d}", .{cur}) catch "0";
                if (num_s.len < max_width) {
                    let p: usize = 0;
                    while (p < max_width - num_s.len) { stdout.push("0"); p += 1; }
                }
                stdout.push(num_s);
            } else {
                pushShellI64(stdout, cur);
            }
            cur += inc_i;
            count += 1;
        }
        if (!first) {
            if (term.len > 0) stdout.push(term);
            else stdout.push("\n");
        }
        return 0;
    }

    // Float fallback path (from Bun seq.cp)
    let start_f: f64 = 1.0;
    let inc_f: f64 = 1.0;
    let end_f: f64 = 1.0;
    if (rem == 1) {
        end_f = std.fmt.parseFloat(f64, argv[idx]) catch { stderr.push("seq: invalid argument\n"); return 1; };
        if (start_f > end_f) inc_f = -1.0;
    } else if (rem == 2) {
        start_f = std.fmt.parseFloat(f64, argv[idx]) catch { stderr.push("seq: invalid argument\n"); return 1; };
        end_f = std.fmt.parseFloat(f64, argv[idx + 1]) catch { stderr.push("seq: invalid argument\n"); return 1; };
        inc_f = if (start_f <= end_f) 1.0 else -1.0;
    } else {
        start_f = std.fmt.parseFloat(f64, argv[idx]) catch { stderr.push("seq: invalid argument\n"); return 1; };
        inc_f = std.fmt.parseFloat(f64, argv[idx + 1]) catch { stderr.push("seq: invalid argument\n"); return 1; };
        end_f = std.fmt.parseFloat(f64, argv[idx + 2]) catch { stderr.push("seq: invalid argument\n"); return 1; };
    }
    if (inc_f == 0.0) { stderr.push("seq: zero increment\n"); return 1; }
    let cur_f = start_f;
    let first = true;
    let guard: usize = 0;
    while (guard < 100000) {
        if (inc_f > 0.0 && cur_f > end_f + 0.000000001) break;
        if (inc_f < 0.0 && cur_f < end_f - 0.000000001) break;
        if (!first) stdout.push(sep);
        first = false;
        let tmp: [64]u8 = [0; 64];
        const s = std.fmt.bufPrint(tmp[0..64], "{d}", .{cur_f}) catch "0";
        stdout.push(s);
        cur_f += inc_f;
        guard += 1;
    }
    if (!first) {
        if (term.len > 0) stdout.push(term);
        else stdout.push("\n");
    }
    return 0;
}

pub fn runBuiltinHead(argv: []const []const u8, stdin: []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let num_lines: usize = 10;
    let num_bytes: usize? = null;
    let idx: usize = 1;
    while (idx < argv.len) {
        const a = argv[idx];
        if (std.mem.eql(u8, a, "-n") && idx + 1 < argv.len) {
            if (n := parseShellInt64(argv[idx + 1])) {
                num_lines = if (n < 0) 0 else (n as usize);
            }
            idx += 2;
        } else if (std.mem.eql(u8, a, "-c") && idx + 1 < argv.len) {
            if (n := parseShellInt64(argv[idx + 1])) {
                num_bytes = if (n < 0) 0 else (n as usize);
            }
            idx += 2;
        } else if (a.len >= 2 && a[0] == '-' && a[1] >= '0' && a[1] <= '9') {
            if (n := parseShellInt64(a[1..])) {
                num_lines = if (n < 0) 0 else (n as usize);
            }
            idx += 1;
        } else {
            break;
        }
    }
    if (idx >= argv.len) {
        if (nb := num_bytes) {
            const take_n = if (nb < stdin.len) nb else stdin.len;
            stdout.push(stdin[0..take_n]);
        } else {
            writeHeadContent(stdin, num_lines, stdout);
        }
        return 0;
    }
    while (idx < argv.len) {
        let fbuf = StringJoiner.init();
        defer fbuf.release();
        if (!readFileInto(argv[idx], &fbuf)) {
            stderr.push("head: cannot open '");
            stderr.push(argv[idx]);
            stderr.push("'\n");
            return 1;
        }
        const data = fbuf.slice();
        if (nb := num_bytes) {
            const take_n = if (nb < data.len) nb else data.len;
            stdout.push(data[0..take_n]);
        } else {
            writeHeadContent(data, num_lines, stdout);
        }
        idx += 1;
    }
    return 0;
}

pub fn runBuiltinTail(argv: []const []const u8, stdin: []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let num_lines: usize = 10;
    let num_bytes: usize? = null;
    let idx: usize = 1;
    while (idx < argv.len) {
        const a = argv[idx];
        if (std.mem.eql(u8, a, "-n") && idx + 1 < argv.len) {
            if (n := parseShellInt64(argv[idx + 1])) {
                num_lines = if (n < 0) 0 else (n as usize);
            }
            idx += 2;
        } else if (std.mem.eql(u8, a, "-c") && idx + 1 < argv.len) {
            if (n := parseShellInt64(argv[idx + 1])) {
                num_bytes = if (n < 0) 0 else (n as usize);
            }
            idx += 2;
        } else if (a.len >= 2 && a[0] == '-' && a[1] >= '0' && a[1] <= '9') {
            if (n := parseShellInt64(a[1..])) {
                num_lines = if (n < 0) 0 else (n as usize);
            }
            idx += 1;
        } else {
            break;
        }
    }
    if (idx >= argv.len) {
        if (nb := num_bytes) {
            const start = if (stdin.len > nb) stdin.len - nb else 0;
            stdout.push(stdin[start..]);
        } else {
            writeTailContent(stdin, num_lines, stdout);
        }
        return 0;
    }
    while (idx < argv.len) {
        let fbuf = StringJoiner.init();
        defer fbuf.release();
        if (!readFileInto(argv[idx], &fbuf)) {
            stderr.push("tail: cannot open '");
            stderr.push(argv[idx]);
            stderr.push("'\n");
            return 1;
        }
        const data = fbuf.slice();
        if (nb := num_bytes) {
            const start = if (data.len > nb) data.len - nb else 0;
            stdout.push(data[start..]);
        } else {
            writeTailContent(data, num_lines, stdout);
        }
        idx += 1;
    }
    return 0;
}

pub fn runBuiltinWc(argv: []const []const u8, stdin: []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let show_l = false;
    let show_w = false;
    let show_c = false;
    let idx: usize = 1;
    while (idx < argv.len) {
        const a = argv[idx];
        if (a.len < 2 || a[0] != '-') break;
        let k: usize = 1;
        while (k < a.len) {
            if (a[k] == 'l') show_l = true;
            else if (a[k] == 'w') show_w = true;
            else if (a[k] == 'c') show_c = true;
            k += 1;
        }
        idx += 1;
    }
    if (!show_l && !show_w && !show_c) {
        show_l = true;
        show_w = true;
        show_c = true;
    }
    if (idx >= argv.len) {
        const stats = countWc(stdin);
        let first = true;
        if (show_l) { pushShellUsize(stdout, stats.lines); first = false; }
        if (show_w) { if (!first) stdout.push(" "); pushShellUsize(stdout, stats.words); first = false; }
        if (show_c) { if (!first) stdout.push(" "); pushShellUsize(stdout, stats.bytes); }
        stdout.push("\n");
        return 0;
    }
    let total_l: usize = 0;
    let total_w: usize = 0;
    let total_c: usize = 0;
    let file_count: usize = 0;
    while (idx < argv.len) {
        let fbuf = StringJoiner.init();
        defer fbuf.release();
        if (!readFileInto(argv[idx], &fbuf)) {
            stderr.push("wc: ");
            stderr.push(argv[idx]);
            stderr.push(": No such file or directory\n");
            return 1;
        }
        const stats = countWc(fbuf.slice());
        total_l += stats.lines;
        total_w += stats.words;
        total_c += stats.bytes;
        file_count += 1;
        let first = true;
        if (show_l) { pushShellUsize(stdout, stats.lines); first = false; }
        if (show_w) { if (!first) stdout.push(" "); pushShellUsize(stdout, stats.words); first = false; }
        if (show_c) { if (!first) stdout.push(" "); pushShellUsize(stdout, stats.bytes); }
        stdout.push(" ");
        stdout.push(argv[idx]);
        stdout.push("\n");
        idx += 1;
    }
    if (file_count > 1) {
        let first = true;
        if (show_l) { pushShellUsize(stdout, total_l); first = false; }
        if (show_w) { if (!first) stdout.push(" "); pushShellUsize(stdout, total_w); first = false; }
        if (show_c) { if (!first) stdout.push(" "); pushShellUsize(stdout, total_c); }
        stdout.push(" total\n");
    }
    return 0;
}

fn containsPattern(haystack: []const u8, needle: []const u8, ignore_case: bool): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    if (!ignore_case) {
        return std.mem.indexOf(u8, haystack, needle) != null;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (std.ascii.eqlIgnoreCase(haystack[i .. i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn matchRegexLine(line: []const u8, pattern: []const u8, ignore_case: bool): bool {
    // Supports ^ anchor, $ anchor, . wildcard, * quantifier, and | alternation
    let alt_start: usize = 0;
    let i: usize = 0;
    while (i <= pattern.len) {
        if (i == pattern.len || pattern[i] == '|') {
            const sub = pattern[alt_start..i];
            if (matchRegexAtomSeq(line, sub, ignore_case)) return true;
            alt_start = i + 1;
        }
        i += 1;
    }
    return false;
}

fn charEq(a: u8, b: u8, ignore_case: bool): bool {
    if (!ignore_case) return a == b;
    return std.ascii.toLower(a) == std.ascii.toLower(b);
}

fn matchHere(text: []const u8, pat: []const u8, ignore_case: bool): bool {
    if (pat.len == 0) return true;
    if (pat.len == 1 && pat[0] == '$') return text.len == 0;
    if (pat.len >= 2 && pat[1] == '*') {
        const c = pat[0];
        let k: usize = 0;
        while (k <= text.len) {
            if (matchHere(text[k..], pat[2..], ignore_case)) return true;
            if (k >= text.len) break;
            if (c != '.' && !charEq(text[k], c, ignore_case)) break;
            k += 1;
        }
        return false;
    }
    if (text.len > 0 && (pat[0] == '.' || charEq(text[0], pat[0], ignore_case))) {
        return matchHere(text[1..], pat[1..], ignore_case);
    }
    return false;
}

fn matchRegexAtomSeq(line: []const u8, pat: []const u8, ignore_case: bool): bool {
    if (pat.len > 0 && pat[0] == '^') {
        return matchHere(line, pat[1..], ignore_case);
    }
    let pos: usize = 0;
    while (pos <= line.len) {
        if (matchHere(line[pos..], pat, ignore_case)) return true;
        pos += 1;
    }
    return false;
}

pub fn runBuiltinGrep(argv: []const []const u8, stdin: []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let ignore_case = false;
    let invert = false;
    let count_only = false;
    let line_numbers = false;
    let quiet = false;
    let files_with_matches = false;
    let fixed_strings = false;

    let idx: usize = 1;
    while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-') {
        const flag = argv[idx];
        if (std.mem.eql(u8, flag, "--")) { idx += 1; break; }
        let k: usize = 1;
        while (k < flag.len) {
            const c = flag[k];
            if (c == 'i') ignore_case = true;
            else if (c == 'v') invert = true;
            else if (c == 'c') count_only = true;
            else if (c == 'n') line_numbers = true;
            else if (c == 'q') quiet = true;
            else if (c == 'l') files_with_matches = true;
            else if (c == 'F') fixed_strings = true;
            else if (c == 'E') fixed_strings = false;
            k += 1;
        }
        idx += 1;
    }

    if (idx >= argv.len) {
        stderr.push("grep: missing pattern\n");
        return 2;
    }
    const pattern = argv[idx];
    idx += 1;

    const multi_file = (argv.len - idx) > 1;
    let any_match = false;
    let err_occurred = false;

    if (idx >= argv.len) {
        const matched = grepBuffer(stdin, "", pattern, false, ignore_case, invert, count_only, line_numbers, quiet, files_with_matches, fixed_strings, stdout);
        if (matched) any_match = true;
    } else {
        while (idx < argv.len) {
            const fpath = argv[idx];
            if (std.mem.eql(u8, fpath, "-")) {
                if (grepBuffer(stdin, "(standard input)", pattern, multi_file, ignore_case, invert, count_only, line_numbers, quiet, files_with_matches, fixed_strings, stdout)) {
                    any_match = true;
                }
            } else {
                let fbuf = StringJoiner.init();
                defer fbuf.release();
                if (!readFileInto(fpath, &fbuf)) {
                    stderr.push("grep: ");
                    stderr.push(fpath);
                    stderr.push(": No such file or directory\n");
                    err_occurred = true;
                } else {
                    if (grepBuffer(fbuf.slice(), fpath, pattern, multi_file, ignore_case, invert, count_only, line_numbers, quiet, files_with_matches, fixed_strings, stdout)) {
                        any_match = true;
                    }
                }
            }
            idx += 1;
        }
    }

    if (err_occurred && !any_match) return 2;
    return if (any_match) 0 else 1;
}

fn grepBuffer(
    data: []const u8,
    label: []const u8,
    pattern: []const u8,
    show_label: bool,
    ignore_case: bool,
    invert: bool,
    count_only: bool,
    line_numbers: bool,
    quiet: bool,
    files_with_matches: bool,
    fixed_strings: bool,
    stdout: *StringJoiner,
): bool {
    let match_count: usize = 0;
    let line_no: usize = 0;
    let pos: usize = 0;
    const has_regex_meta = !fixed_strings && (std.mem.indexOfAny(u8, pattern, "^$.*|") != null);

    while (pos < data.len) {
        let end = pos;
        while (end < data.len && data[end] != '\n') end += 1;
        const line = data[pos..end];
        line_no += 1;

        let is_match = if (has_regex_meta)
            matchRegexLine(line, pattern, ignore_case)
        else
            containsPattern(line, pattern, ignore_case);
        if (invert) is_match = !is_match;

        if (is_match) {
            match_count += 1;
            if (quiet) return true;
            if (files_with_matches) {
                stdout.push(label);
                stdout.push("\n");
                return true;
            }
            if (!count_only) {
                if (show_label && label.len > 0) {
                    stdout.push(label);
                    stdout.push(":");
                }
                if (line_numbers) {
                    pushShellUsize(stdout, line_no);
                    stdout.push(":");
                }
                stdout.push(line);
                stdout.push("\n");
            }
        }
        pos = if (end < data.len && data[end] == '\n') end + 1 else end;
    }

    if (count_only && !quiet && !files_with_matches) {
        if (show_label && label.len > 0) {
            stdout.push(label);
            stdout.push(":");
        }
        pushShellUsize(stdout, match_count);
        stdout.push("\n");
    }
    return match_count > 0;
}

pub fn runBuiltinYes(argv: []const []const u8, stdout: *StringJoiner): i32 {
    // Bounded output by default (1024 lines) or -n COUNT to prevent infinite hang in buffered capture
    let max_lines: usize = 1024;
    let idx: usize = 1;
    if (idx + 1 < argv.len && std.mem.eql(u8, argv[idx], "-n")) {
        if (n := parseShellInt64(argv[idx + 1])) {
            max_lines = if (n < 0) 0 else (n as usize);
            if (max_lines > 65536) max_lines = 65536;
        }
        idx += 2;
    }

    let line = StringJoiner.init();
    defer line.release();
    if (idx >= argv.len) {
        line.push("y");
    } else {
        let first = true;
        while (idx < argv.len) {
            if (!first) line.push(" ");
            first = false;
            line.push(argv[idx]);
            idx += 1;
        }
    }
    line.push("\n");
    const line_slice = line.slice();

    let k: usize = 0;
    while (k < max_lines) {
        stdout.push(line_slice);
        k += 1;
    }
    return 0;
}

pub fn runBuiltinSort(argv: []const []const u8, stdin: []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let reverse = false;
    let numeric = false;
    let unique = false;
    let idx: usize = 1;
    while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-') {
        const f = argv[idx];
        if (std.mem.eql(u8, f, "--")) { idx += 1; break; }
        let k: usize = 1;
        while (k < f.len) {
            if (f[k] == 'r') reverse = true;
            else if (f[k] == 'n') numeric = true;
            else if (f[k] == 'u') unique = true;
            k += 1;
        }
        idx += 1;
    }

    let input = StringJoiner.init();
    defer input.release();
    if (idx >= argv.len) {
        input.push(stdin);
    } else {
        while (idx < argv.len) {
            if (!readFileInto(argv[idx], &input)) {
                stderr.push("sort: cannot read '"); stderr.push(argv[idx]); stderr.push("'\n");
                return 2;
            }
            idx += 1;
        }
    }

    const data = input.slice();
    if (data.len == 0) return 0;

    const alloc = std.heap.page_allocator;
    let lines: std.ArrayListUnmanaged(string) = .empty;
    defer lines.deinit(alloc);

    let pos: usize = 0;
    while (pos < data.len) {
        let end = pos;
        while (end < data.len && data[end] != '\n') end += 1;
        lines.append(alloc, data[pos..end]) catch {};
        pos = if (end < data.len && data[end] == '\n') end + 1 else end;
    }

    if (lines.items.len > 1) {
        let i: usize = 1;
        while (i < lines.items.len) {
            const key = lines.items[i];
            let j = i;
            while (j > 0) {
                let cmp_gt = false;
                if (numeric) {
                    const a = parseShellInt64(std.mem.trim(u8, lines.items[j - 1], " \t")) ?? 0;
                    const b = parseShellInt64(std.mem.trim(u8, key, " \t")) ?? 0;
                    cmp_gt = if (reverse) (a < b) else (a > b);
                } else {
                    const ord = std.mem.order(u8, lines.items[j - 1], key);
                    cmp_gt = if (reverse) (ord == .lt) else (ord == .gt);
                }
                if (!cmp_gt) break;
                lines.items[j] = lines.items[j - 1];
                j -= 1;
            }
            lines.items[j] = key;
            i += 1;
        }
    }

    let prev: ([]const u8)? = null;
    for (ln of lines.items) {
        if (unique) {
            if (p := prev) {
                if (std.mem.eql(u8, p, ln)) continue;
            }
        }
        stdout.push(ln);
        stdout.push("\n");
        prev = ln;
    }
    return 0;
}

pub fn runBuiltinUniq(argv: []const []const u8, stdin: []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let count_mode = false;
    let idx: usize = 1;
    while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-') {
        if (std.mem.eql(u8, argv[idx], "-c")) count_mode = true;
        idx += 1;
    }
    let fbuf = StringJoiner.init();
    defer fbuf.release();
    let data = stdin;
    if (idx < argv.len) {
        if (!readFileInto(argv[idx], &fbuf)) {
            stderr.push("uniq: "); stderr.push(argv[idx]); stderr.push(": No such file or directory\n");
            return 1;
        }
        data = fbuf.slice();
    }
    if (data.len == 0) return 0;

    let prev: ([]const u8)? = null;
    let cnt: usize = 0;
    let pos: usize = 0;
    while (pos < data.len) {
        let end = pos;
        while (end < data.len && data[end] != '\n') end += 1;
        const line = data[pos..end];
        if (p := prev) {
            if (std.mem.eql(u8, p, line)) {
                cnt += 1;
            } else {
                if (count_mode) { pushShellUsize(stdout, cnt); stdout.push(" "); }
                stdout.push(p); stdout.push("\n");
                prev = line;
                cnt = 1;
            }
        } else {
            prev = line;
            cnt = 1;
        }
        pos = if (end < data.len && data[end] == '\n') end + 1 else end;
    }
    if (p := prev) {
        if (count_mode) { pushShellUsize(stdout, cnt); stdout.push(" "); }
        stdout.push(p); stdout.push("\n");
    }
    return 0;
}
