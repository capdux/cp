// Source: Absorbed from /data/dev/bun/src/shell/interpreter.cp | Synced: 2026-11-22T11:56:36Z
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/interpreter.cp & subproc.cp
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/exec.cp
// Industrial-grade Pipeline Execution, Word/Brace/Glob/Arith/CmdSubst Expansion,
// Control Flow, Subshell Isolation, Redirections & PosixSpawn Engine for CP Shell (cap:shell)

const std = @import("std");
import { StringJoiner } from "cap:string";
import {
    FD,
    File,
    O_RDONLY,
    O_WRONLY,
    O_CREAT,
    O_TRUNC,
    O_APPEND,
    O_CLOEXEC,
    PollFd,
    POLLIN,
    POLLOUT,
    POLLERR,
    POLLHUP,
    PosixSpawn,
    SpawnOptions,
    waitpid,
    poll,
    getcwd,
    chdir,
    getpid,
} from "cap:sys";
import { Pipe, IoStatus } from "cap:io";
import { Matcher, GlobWalker, GlobWalkerOptions } from "cap:glob";
import {
    Command,
    CommandKind,
    Pipeline,
    Script,
    ScriptPtr,
    Statement,
    ControlOp,
    Parser,
    AstPool,
    WordArg,
    MAX_COMMAND_ARGS,
} from "./parser.cp";
import { isBuiltin, runBuiltinWithState } from "./builtin.cp";
import {
    ShellEnv,
    DirStack,
    ShellOptions,
    AliasMap,
    MAX_POS_ARGS,
    expandWord,
    expandBraceBody,
    isShellIdentStart,
    isShellIdentCont,
    pushShellI64,
    pushShellUsize,
} from "./env.cp";
import { BraceExpander } from "./braces.cp";
import { ArithEval } from "./arith.cp";

pub struct ShellResult {
    exit_code: i32 = 0,
    stdout_str: []const u8 = "",
    stderr_str: []const u8 = "",

    pub fn ok(): bool {
        return _.exit_code == 0;
    }

    pub fn text(): []const u8 {
        return _.stdout_str;
    }

    pub fn err(): []const u8 {
        return _.stderr_str;
    }
}

fn writeRedirectFile(path: []const u8, content: []const u8, append: bool): bool {
    if (path.len == 0 || path.len >= 4096) {
        return false;
    }
    const flags = O_WRONLY | O_CREAT | O_CLOEXEC | (if (append) O_APPEND else O_TRUNC);
    let f = File.open(path, flags, 0o644) catch return false;
    defer f.close();
    f.writeAll(content) catch return false;
    return true;
}

fn readRedirectFile(path: []const u8, out: *StringJoiner): bool {
    if (path.len == 0 || path.len >= 4096) {
        return false;
    }
    let f = File.openReadOnly(path) catch return false;
    defer f.close();
    let buf: [4096]u8 = undefined;
    while (true) {
        const n = f.read(&buf) catch return false;
        if (n == 0) break;
        out.push(buf[0..n]);
    }
    return true;
}

fn stripTrailingNewlines(s: []const u8): []const u8 {
    let end = s.len;
    while (end > 0 && s[end - 1] == '\n') {
        end -= 1;
    }
    return s[0..end];
}

fn hasUnquotedGlobMagic(raw: []const u8): bool {
    let in_single = false;
    let in_double = false;
    let i: usize = 0;
    while (i < raw.len) {
        const ch = raw[i];
        if (in_single) {
            if (ch == '\'') in_single = false;
        } else if (in_double) {
            if (ch == '\\' && i + 1 < raw.len) { i += 2; continue; }
            if (ch == '"') in_double = false;
        } else {
            if (ch == '\\' && i + 1 < raw.len) { i += 2; continue; }
            if (ch == '\'') in_single = true;
            else if (ch == '"') in_double = true;
            else if (ch == '*' || ch == '?') return true;
            else if (ch == '[') {
                // Only treat '[' as glob bracket expression if a matching unescaped ']' exists in the same segment
                let k = i + 1;
                let has_inner = false;
                while (k < raw.len && raw[k] != '/') {
                    if (raw[k] == '\\' && k + 1 < raw.len) {
                        k += 2;
                        has_inner = true;
                        continue;
                    }
                    if (raw[k] == ']') {
                        if (has_inner) return true;
                        break;
                    }
                    has_inner = true;
                    k += 1;
                }
            }
        }
        i += 1;
    }
    return false;
}

fn hasAnyQuotes(raw: []const u8): bool {
    let i: usize = 0;
    while (i < raw.len) {
        if (raw[i] == '\\' && i + 1 < raw.len) {
            i += 2;
            continue;
        }
        if (raw[i] == '"' || raw[i] == '\'') return true;
        i += 1;
    }
    return false;
}

fn sortMatches(items: [][]const u8) {
    if (items.len <= 1) return;
    let i: usize = 1;
    while (i < items.len) {
        const key = items[i];
        let j = i;
        while (j > 0 && std.mem.order(u8, items[j - 1], key) == .gt) {
            items[j] = items[j - 1];
            j -= 1;
        }
        items[j] = key;
        i += 1;
    }
}

fn expandGlobPattern(allocator: std.mem.Allocator, pattern: []const u8, out: *std.ArrayListUnmanaged(string)): bool {
    // Find split between fixed directory prefix and glob pattern part
    // e.g., "/tmp/dir/*.cp" -> base="/tmp/dir", rel="*.cp"
    //       "sub/*.cp" -> base="sub", rel="*.cp"
    //       "*.cp" -> base=".", rel="*.cp"
    let first_magic: usize = pattern.len;
    let i: usize = 0;
    while (i < pattern.len) {
        if (pattern[i] == '\\' && i + 1 < pattern.len) {
            i += 2;
            continue;
        }
        if (pattern[i] == '*' || pattern[i] == '?' || pattern[i] == '[') {
            first_magic = i;
            break;
        }
        i += 1;
    }
    if (first_magic == pattern.len) return false;

    let last_slash_before: usize? = null;
    let j: usize = 0;
    while (j < first_magic) {
        if (pattern[j] == '/') last_slash_before = j;
        j += 1;
    }

    let base_dir: []const u8 = ".";
    let rel_pattern: []const u8 = pattern;
    let prefix_to_prepend: []const u8 = "";
    if (ls := last_slash_before) {
        if (ls == 0) {
            base_dir = "/";
            prefix_to_prepend = "/";
            rel_pattern = pattern[1..];
        } else {
            base_dir = pattern[0..ls];
            prefix_to_prepend = pattern[0 .. ls + 1];
            rel_pattern = pattern[ls + 1 ..];
        }
    }

    if (rel_pattern.len == 0) return false;

    let opts = GlobWalkerOptions.default();
    opts.only_files = false;
    opts.onlyFiles = false;
    opts.dot = (rel_pattern[0] == '.');
    if (std.mem.indexOf(u8, rel_pattern, "**") != null) {
        opts.max_depth = 64;
    } else {
        let slash_count: usize = 0;
        for (ch of rel_pattern) {
            if (ch == '/') slash_count += 1;
        }
        opts.max_depth = slash_count;
    }

    const matches = GlobWalker.scanSync(allocator, rel_pattern, base_dir, opts) catch return false;
    defer {
        for (m of matches) allocator.free(m);
        allocator.free(matches);
    }

    if (matches.len == 0) return false;

    sortMatches(matches);
    for (m of matches) {
        if (prefix_to_prepend.len > 0) {
            const full = std.fmt.allocPrint(allocator, "{s}{s}", .{ prefix_to_prepend, m }) catch continue;
            out.append(allocator, full) catch {};
        } else {
            const dup = allocator.dupe(u8, m) catch continue;
            out.append(allocator, dup) catch {};
        }
    }
    return true;
}

pub struct ShellSession {
    pub env: ShellEnv,
    pub dirs: DirStack,
    pub options: ShellOptions = ShellOptions{},
    pub aliases: AliasMap = AliasMap.init(),
    pub pos_args: [MAX_POS_ARGS][]const u8 = undefined,
    pub pos_args_len: usize = 0,
    pub script_name: []const u8 = "cp-sh",
    pub last_exit_code: i32 = 0,
    pub should_exit: bool = false,
    pub loop_break: usize = 0,
    pub loop_continue: usize = 0,

    pub static fn init(): ShellSession {
        let s = ShellSession {
            env: ShellEnv.init(),
            dirs: DirStack.init(),
            options: ShellOptions{},
            aliases: AliasMap.init(),
            pos_args: undefined,
            pos_args_len: 0,
            script_name: "cp-sh",
            last_exit_code: 0,
            should_exit: false,
            loop_break: 0,
            loop_continue: 0,
        };
        let i: usize = 0;
        while (i < MAX_POS_ARGS) {
            s.pos_args[i] = "";
            i += 1;
        }
        return s;
    }

    pub fn cloneForSubshell(*_): ShellSession {
        let copy = ShellSession {
            env: _.env.clone(),
            dirs: _.dirs.clone(),
            options: _.options,
            aliases: _.aliases.clone(),
            pos_args: undefined,
            pos_args_len: _.pos_args_len,
            script_name: _.script_name,
            last_exit_code: _.last_exit_code,
            should_exit: false,
            loop_break: 0,
            loop_continue: 0,
        };
        const alloc = std.heap.page_allocator;
        let i: usize = 0;
        while (i < MAX_POS_ARGS) {
            if (i < _.pos_args_len && _.pos_args[i].len > 0) {
                copy.pos_args[i] = alloc.dupe(u8, _.pos_args[i]) catch "";
            } else {
                copy.pos_args[i] = "";
            }
            i += 1;
        }
        return copy;
    }

    pub fn setPositionalArgs(*_, args: []const []const u8) {
        const alloc = std.heap.page_allocator;
        const count = if (args.len > MAX_POS_ARGS) MAX_POS_ARGS else args.len;
        let next_args: [MAX_POS_ARGS][]const u8 = undefined;
        let i: usize = 0;
        while (i < count) {
            if (args[i].len > 0) {
                next_args[i] = alloc.dupe(u8, args[i]) catch "";
            } else {
                next_args[i] = "";
            }
            i += 1;
        }
        let old_i: usize = 0;
        while (old_i < _.pos_args_len) {
            if (_.pos_args[old_i].len > 0) {
                alloc.free(_.pos_args[old_i]);
            }
            _.pos_args[old_i] = "";
            old_i += 1;
        }
        i = 0;
        while (i < count) {
            _.pos_args[i] = next_args[i];
            i += 1;
        }
        _.pos_args_len = count;
    }

    pub fn setEnv(*_, key: []const u8, val: []const u8): bool {
        return _.env.set(key, val, true);
    }

    pub fn getEnv(key: []const u8): ([]const u8)? {
        return _.env.get(key);
    }

    pub fn unsetEnv(*_, key: []const u8): bool {
        return _.env.unset(key);
    }

    pub fn cwd(): []const u8 {
        return _.dirs.cwd();
    }

    pub fn run(*_, command_str: []const u8): ShellResult {
        let parser = Parser.init(command_str);
        let script_ptr = parser.parseScriptHeap();
        defer {
            script_ptr.destroy();
            std.heap.page_allocator.destroy(script_ptr);
        }
        return Executor.executeScriptPtrWithSession(script_ptr, _);
    }

    pub fn runWithMemoryTarget(*_, command_str: []const u8, out_buf: *StringJoiner, err_buf: *StringJoiner): i32 {
        const res = _.run(command_str);
        out_buf.push(res.stdout_str);
        err_buf.push(res.stderr_str);
        return res.exit_code;
    }

    ~init() {
        const alloc = std.heap.page_allocator;
        let i: usize = 0;
        while (i < _.pos_args_len) {
            if (_.pos_args[i].len > 0) {
                alloc.free(_.pos_args[i]);
            }
            _.pos_args[i] = "";
            i += 1;
        }
        _.pos_args_len = 0;
        _.env.~init();
        _.aliases.~init();
    }
}

fn runCommandSubstitution(cmd_src: []const u8, session: *ShellSession, out: *StringJoiner) {
    let saved_cwd_buf: [1024]u8 = [0; 1024];
    const saved_cwd = getcwd(&saved_cwd_buf) catch "/";
    let sub_sess = session.cloneForSubshell();
    defer {
        sub_sess.~init();
        chdir(saved_cwd) catch {};
    }

    const res = sub_sess.run(cmd_src);
    out.push(stripTrailingNewlines(res.stdout_str));
}

fn expandDollarWithSession(
    raw: []const u8,
    start_idx: usize,
    session: *ShellSession,
    out: *StringJoiner,
    split_hint: *bool,
): usize {
    let i = start_idx + 1;
    if (i >= raw.len) {
        out.push("$");
        return i;
    }

    const next = raw[i];
    if (next == '?') {
        pushShellI64(out, session.last_exit_code as i64);
        return i + 1;
    }
    if (next == '$') {
        pushShellI64(out, getpid() as i64);
        return i + 1;
    }
    if (next == '#') {
        pushShellUsize(out, session.pos_args_len);
        return i + 1;
    }
    if (next == '0') {
        out.push(session.script_name);
        return i + 1;
    }
    if (next >= '1' && next <= '9') {
        const idx = ((next - '1') as usize);
        if (idx < session.pos_args_len) {
            out.push(session.pos_args[idx]);
            split_hint.* = true;
        }
        return i + 1;
    }
    if (next == '@' || next == '*') {
        let pi: usize = 0;
        while (pi < session.pos_args_len) {
            if (pi > 0) out.push(" ");
            out.push(session.pos_args[pi]);
            pi += 1;
        }
        split_hint.* = true;
        return i + 1;
    }

    // $(( arithmetic ))
    if (next == '(' && i + 1 < raw.len && raw[i + 1] == '(') {
        i += 2;
        const expr_start = i;
        let depth: usize = 2;
        while (i < raw.len && depth > 0) {
            if (raw[i] == '\\' && i + 1 < raw.len) {
                i += 2;
            } else if (raw[i] == '(') {
                depth += 1;
                i += 1;
            } else if (raw[i] == ')') {
                depth -= 1;
                if (depth == 0) break;
                i += 1;
            } else {
                i += 1;
            }
        }
        // Check if closing "))" matched properly
        let expr_end = i;
        if (expr_end > expr_start && raw[expr_end - 1] == ')') {
            expr_end -= 1;
        }
        const expr_raw = raw[expr_start..expr_end];
        if (i < raw.len && raw[i] == ')') i += 1;

        // Pre-expand any $VAR or $(cmd) inside arithmetic expression
        let exp_expr = StringJoiner.init();
        defer exp_expr.~init();
        let dummy_split = false;
        expandWordWithSession(expr_raw, session, &exp_expr, &dummy_split, false);
        const val = ArithEval.eval(exp_expr.slice(), &session.env);
        pushShellI64(out, val);
        return i;
    }

    // $( command substitution )
    if (next == '(') {
        i += 1;
        const cmd_start = i;
        let depth: usize = 1;
        let in_sq = false;
        let in_dq = false;
        while (i < raw.len && depth > 0) {
            const ch = raw[i];
            if (in_sq) {
                if (ch == '\'') in_sq = false;
            } else if (in_dq) {
                if (ch == '\\' && i + 1 < raw.len) { i += 2; continue; }
                if (ch == '"') in_dq = false;
            } else {
                if (ch == '\\' && i + 1 < raw.len) { i += 2; continue; }
                if (ch == '\'') in_sq = true;
                else if (ch == '"') in_dq = true;
                else if (ch == '(') depth += 1;
                else if (ch == ')') {
                    depth -= 1;
                    if (depth == 0) break;
                }
            }
            i += 1;
        }
        const cmd_src = raw[cmd_start..i];
        if (i < raw.len && raw[i] == ')') i += 1;
        runCommandSubstitution(cmd_src, session, out);
        split_hint.* = true;
        return i;
    }

    // ${ ... }
    if (next == '{') {
        i += 1;
        const brace_start = i;
        let depth: usize = 1;
        while (i < raw.len && depth > 0) {
            if (raw[i] == '\\' && i + 1 < raw.len) {
                i += 2;
            } else if (raw[i] == '{') {
                depth += 1;
                i += 1;
            } else if (raw[i] == '}') {
                depth -= 1;
                if (depth == 0) break;
                i += 1;
            } else {
                i += 1;
            }
        }
        const inner = raw[brace_start..i];
        if (i < raw.len && raw[i] == '}') i += 1;
        expandBraceBodyWithSession(inner, session, out);
        split_hint.* = true;
        return i;
    }

    // $VAR
    if (isShellIdentStart(next)) {
        const name_start = i;
        i += 1;
        while (i < raw.len && isShellIdentCont(raw[i])) {
            i += 1;
        }
        const var_name = raw[name_start..i];
        if (val := session.env.get(var_name)) {
            out.push(val);
            split_hint.* = true;
        }
        return i;
    }

    out.push("$");
    return start_idx + 1;
}

fn expandBraceBodyWithSession(inner: []const u8, session: *ShellSession, out: *StringJoiner) {
    if (inner.len == 0) return;

    if (inner[0] == '#' && inner.len > 1) {
        const vname = inner[1..];
        if (val := session.env.get(vname)) {
            pushShellUsize(out, val.len);
        } else {
            out.push("0");
        }
        return;
    }

    let name_end: usize = 0;
    while (name_end < inner.len && isShellIdentCont(inner[name_end])) {
        name_end += 1;
    }
    const var_name = inner[0..name_end];
    const rest = inner[name_end..];

    if (rest.len == 0) {
        if (val := session.env.get(var_name)) {
            out.push(val);
        }
        return;
    }

    let colon = false;
    let op_pos: usize = 0;
    if (rest[0] == ':' && rest.len >= 2) {
        colon = true;
        op_pos = 1;
    }
    const op_ch = rest[op_pos];
    const arg = rest[op_pos + 1 ..];

    const existing_opt = session.env.get(var_name);
    let is_set = false;
    if (v := existing_opt) {
        is_set = !colon || v.len > 0;
    }

    if (op_ch == '-') {
        if (is_set) {
            out.push(existing_opt.?);
        } else {
            let sub = StringJoiner.init();
            defer sub.~init();
            let dummy = false;
            expandWordWithSession(arg, session, &sub, &dummy, false);
            out.push(sub.slice());
        }
        return;
    }

    if (op_ch == '=') {
        if (is_set) {
            out.push(existing_opt.?);
        } else {
            let sub = StringJoiner.init();
            defer sub.~init();
            let dummy = false;
            expandWordWithSession(arg, session, &sub, &dummy, false);
            const expanded = sub.slice();
            * = session.env.set(var_name, expanded, false);
            out.push(expanded);
        }
        return;
    }

    if (op_ch == '+') {
        if (is_set) {
            let sub = StringJoiner.init();
            defer sub.~init();
            let dummy = false;
            expandWordWithSession(arg, session, &sub, &dummy, false);
            out.push(sub.slice());
        }
        return;
    }

    if (val := existing_opt) {
        out.push(val);
    }
}

pub fn expandWordWithSession(
    raw: []const u8,
    session: *ShellSession,
    out: *StringJoiner,
    unquoted_expansion_split: *bool,
    strip_quotes: bool,
) {
    if (raw.len == 0) return;

    let i: usize = 0;
    if (strip_quotes && raw[0] == '~' && (raw.len == 1 || raw[1] == '/')) {
        if (home := session.env.get("HOME")) {
            out.push(home);
            i = 1;
        }
    }

    let in_single = false;
    let in_double = false;

    while (i < raw.len) {
        const ch = raw[i];
        if (in_single) {
            if (ch == '\'') {
                in_single = false;
                if (!strip_quotes) out.push("'");
            } else {
                out.push(raw[i .. i + 1]);
            }
            i += 1;
        } else if (in_double) {
            if (ch == '"') {
                in_double = false;
                if (!strip_quotes) out.push("\"");
                i += 1;
            } else if (ch == '\\' && i + 1 < raw.len) {
                const esc = raw[i + 1];
                if (esc == '"' || esc == '\\' || esc == '$' || esc == '`') {
                    if (strip_quotes) out.push(raw[i + 1 .. i + 2]);
                    else out.push(raw[i .. i + 2]);
                    i += 2;
                } else if (esc == '\n') {
                    i += 2;
                } else {
                    out.push(raw[i .. i + 2]);
                    i += 2;
                }
            } else if (ch == '$') {
                let dummy = false;
                i = expandDollarWithSession(raw, i, session, out, &dummy);
            } else if (ch == '`') {
                i += 1;
                const bt_start = i;
                while (i < raw.len && raw[i] != '`') {
                    if (raw[i] == '\\' && i + 1 < raw.len) i += 2;
                    else i += 1;
                }
                const bt_cmd = raw[bt_start..i];
                if (i < raw.len && raw[i] == '`') i += 1;
                runCommandSubstitution(bt_cmd, session, out);
            } else {
                out.push(raw[i .. i + 1]);
                i += 1;
            }
        } else {
            if (ch == '\'') {
                in_single = true;
                if (!strip_quotes) out.push("'");
                i += 1;
            } else if (ch == '"') {
                in_double = true;
                if (!strip_quotes) out.push("\"");
                i += 1;
            } else if (ch == '$' && i + 1 < raw.len && raw[i + 1] == '\'') {
                i += 2;
                while (i < raw.len && raw[i] != '\'') {
                    if (raw[i] == '\\' && i + 1 < raw.len) {
                        const esc = raw[i + 1];
                        if (esc == 'n') { out.push("\n"); i += 2; }
                        else if (esc == 't') { out.push("\t"); i += 2; }
                        else if (esc == 'r') { out.push("\r"); i += 2; }
                        else if (esc == '\\') { out.push("\\"); i += 2; }
                        else if (esc == '\'') { out.push("'"); i += 2; }
                        else { out.push(raw[i + 1 .. i + 2]); i += 2; }
                    } else {
                        out.push(raw[i .. i + 1]);
                        i += 1;
                    }
                }
                if (i < raw.len && raw[i] == '\'') i += 1;
            } else if (ch == '\\' && i + 1 < raw.len) {
                if (raw[i + 1] != '\n') {
                    if (strip_quotes) out.push(raw[i + 1 .. i + 2]);
                    else out.push(raw[i .. i + 2]);
                }
                i += 2;
            } else if (ch == '$') {
                i = expandDollarWithSession(raw, i, session, out, unquoted_expansion_split);
            } else if (ch == '`') {
                i += 1;
                const bt_start = i;
                while (i < raw.len && raw[i] != '`') {
                    if (raw[i] == '\\' && i + 1 < raw.len) i += 2;
                    else i += 1;
                }
                const bt_cmd = raw[bt_start..i];
                if (i < raw.len && raw[i] == '`') i += 1;
                runCommandSubstitution(bt_cmd, session, out);
                unquoted_expansion_split.* = true;
            } else {
                out.push(raw[i .. i + 1]);
                i += 1;
            }
        }
    }
}

fn expandWordToArgList(
    allocator: std.mem.Allocator,
    raw: []const u8,
    has_raw: bool,
    literal: []const u8,
    session: *ShellSession,
    out_args: *std.ArrayListUnmanaged(string),
) {
    if (!has_raw) {
        const dup = allocator.dupe(u8, literal) catch return;
        out_args.append(allocator, dup) catch {};
        return;
    }

    // 1. Brace expansion {a,b}, {1..5}
    const brace_variants = BraceExpander.expand(allocator, raw);
    defer BraceExpander.freeResults(allocator, brace_variants);

    for (variant of brace_variants) {
        const quoted = hasAnyQuotes(variant);
        const glob_magic = hasUnquotedGlobMagic(variant);

        let exp = StringJoiner.init();
        defer exp.~init();
        let did_unquoted_split = false;
        expandWordWithSession(variant, session, &exp, &did_unquoted_split, true);
        const expanded = exp.slice();

        if (expanded.len == 0) {
            if (quoted) {
                const empty_dup = allocator.dupe(u8, "") catch continue;
                out_args.append(allocator, empty_dup) catch {};
            }
            continue;
        }

        // 2. Word splitting on unquoted expansions (when not quoted)
        if (!quoted && did_unquoted_split) {
            let iter = std.mem.tokenizeAny(u8, expanded, " \t\n");
            while (tok := iter.next()) {
                if (hasUnquotedGlobMagic(tok)) {
                    if (!expandGlobPattern(allocator, tok, out_args)) {
                        const dup = allocator.dupe(u8, tok) catch continue;
                        out_args.append(allocator, dup) catch {};
                    }
                } else {
                    const dup = allocator.dupe(u8, tok) catch continue;
                    out_args.append(allocator, dup) catch {};
                }
            }
            continue;
        }

        // 3. Glob expansion (only when unquoted glob magic present)
        if (glob_magic) {
            if (expandGlobPattern(allocator, expanded, out_args)) {
                continue;
            }
        }

        const dup = allocator.dupe(u8, expanded) catch continue;
        out_args.append(allocator, dup) catch {};
    }
}

fn executeExternalPosixSpawn(
    argv: []const []const u8,
    stdin_data: []const u8,
    stdout_joiner: *StringJoiner,
    stderr_joiner: *StringJoiner,
    session: *ShellSession,
): i32 {
    const alloc = std.heap.page_allocator;

    let in_pipe = Pipe.create() catch {
        stderr_joiner.push("shell: pipe creation failed\n");
        return 126;
    };
    let out_pipe = Pipe.create() catch {
        in_pipe.close();
        stderr_joiner.push("shell: pipe creation failed\n");
        return 126;
    };
    let err_pipe = Pipe.create() catch {
        in_pipe.close();
        out_pipe.close();
        stderr_joiner.push("shell: pipe creation failed\n");
        return 126;
    };

    const envp = session.env.buildEnvp(alloc);
    defer ShellEnv.freeEnvp(alloc, envp);

    const opts = SpawnOptions {
        cwd: session.dirs.cwd(),
        envp: envp,
        stdin_fd: FD.fromNative(in_pipe.reader.getFd()),
        stdout_fd: FD.fromNative(out_pipe.writer.getFd()),
        stderr_fd: FD.fromNative(err_pipe.writer.getFd()),
        detached: false,
        new_process_group: false,
        pdeathsig: 0,
        search_path: true,
    };

    const spawn_res = PosixSpawn.spawnMaybe(alloc, argv, opts);

    // Close child-side descriptors in parent immediately
    in_pipe.reader.close();
    out_pipe.writer.close();
    err_pipe.writer.close();

    if (spawn_res.isErr()) {
        in_pipe.writer.close();
        out_pipe.reader.close();
        err_pipe.reader.close();
        stderr_joiner.push("shell: ");
        stderr_joiner.push(argv[0]);
        stderr_joiner.push(": command not found\n");
        return 127;
    }

    const pid = spawn_res.result;

    if (stdin_data.len == 0) {
        in_pipe.writer.close();
    } else {
        * = in_pipe.writer.setNonblocking(true) catch {};
    }
    * = out_pipe.reader.setNonblocking(true) catch {};
    * = err_pipe.reader.setNonblocking(true) catch {};

    let stdin_off: usize = 0;
    let stdin_open = (stdin_data.len > 0);
    let out_open = true;
    let err_open = true;
    let rbuf: [4096]u8 = undefined;

    while (out_open || err_open || stdin_open) {
        let pfds: [3]PollFd = undefined;
        let nfds: usize = 0;
        let in_slot: usize = 99;
        let out_slot: usize = 99;
        let err_slot: usize = 99;

        if (stdin_open) {
            in_slot = nfds;
            pfds[nfds] = PollFd{ .fd = in_pipe.writer.getFd(), .events = POLLOUT, .revents = 0 };
            nfds += 1;
        }
        if (out_open) {
            out_slot = nfds;
            pfds[nfds] = PollFd{ .fd = out_pipe.reader.getFd(), .events = POLLIN, .revents = 0 };
            nfds += 1;
        }
        if (err_open) {
            err_slot = nfds;
            pfds[nfds] = PollFd{ .fd = err_pipe.reader.getFd(), .events = POLLIN, .revents = 0 };
            nfds += 1;
        }
        if (nfds == 0) break;

        * = poll(pfds[0..nfds], 5000) catch break;

        if (stdin_open && in_slot < nfds) {
            const rev = pfds[in_slot].revents;
            if ((rev & (POLLOUT | POLLERR | POLLHUP)) != 0) {
                const wr = in_pipe.writer.attemptWrite(stdin_data[stdin_off..]);
                stdin_off += wr.bytes;
                if (IoStatus.BrokenPipe == wr.status || IoStatus.Error == wr.status || stdin_off >= stdin_data.len) {
                    in_pipe.writer.close();
                    stdin_open = false;
                }
            }
        }
        if (out_open && out_slot < nfds) {
            const rev = pfds[out_slot].revents;
            if ((rev & (POLLIN | POLLHUP | POLLERR)) != 0) {
                while (true) {
                    const rr = out_pipe.reader.attemptRead(rbuf[0..rbuf.len]);
                    if (rr.bytes > 0) stdout_joiner.push(rbuf[0..rr.bytes]);
                    if (IoStatus.EndOfFile == rr.status || IoStatus.Error == rr.status || IoStatus.Closed == rr.status) {
                        out_pipe.reader.close();
                        out_open = false;
                        break;
                    }
                    if (rr.status == IoStatus.WouldBlock || rr.bytes == 0) break;
                }
            }
        }
        if (err_open && err_slot < nfds) {
            const rev = pfds[err_slot].revents;
            if ((rev & (POLLIN | POLLHUP | POLLERR)) != 0) {
                while (true) {
                    const rr = err_pipe.reader.attemptRead(rbuf[0..rbuf.len]);
                    if (rr.bytes > 0) stderr_joiner.push(rbuf[0..rr.bytes]);
                    if (IoStatus.EndOfFile == rr.status || IoStatus.Error == rr.status || IoStatus.Closed == rr.status) {
                        err_pipe.reader.close();
                        err_open = false;
                        break;
                    }
                    if (rr.status == IoStatus.WouldBlock || rr.bytes == 0) break;
                }
            }
        }
    }

    if (stdin_open) in_pipe.writer.close();
    if (out_open) out_pipe.reader.close();
    if (err_open) err_pipe.reader.close();

    const wres = waitpid(pid, 0) catch return 1;
    if (wres.exited()) {
        return wres.exitCode() as i32;
    }
    if (wres.signaled()) {
        return 128 + (wres.termSig() as i32);
    }
    return 1;
}

fn executeSessionBuiltin(argv: []const []const u8, stdout_joiner: *StringJoiner, stderr_joiner: *StringJoiner, session: *ShellSession): i32? {
    if (argv.len == 0) return null;
    const name = argv[0];

    if (std.mem.eql(u8, name, "set")) {
        let i: usize = 1;
        while (i < argv.len) {
            const a = argv[i];
            if (std.mem.eql(u8, a, "--")) {
                session.setPositionalArgs(argv[i + 1 ..]);
                return 0;
            } else if (std.mem.eql(u8, a, "-e")) {
                session.options.errexit = true;
            } else if (std.mem.eql(u8, a, "+e")) {
                session.options.errexit = false;
            } else if (std.mem.eql(u8, a, "-u")) {
                session.options.nounset = true;
            } else if (std.mem.eql(u8, a, "+u")) {
                session.options.nounset = false;
            } else if (std.mem.eql(u8, a, "-x")) {
                session.options.xtrace = true;
            } else if (std.mem.eql(u8, a, "+x")) {
                session.options.xtrace = false;
            } else if (std.mem.eql(u8, a, "-o") && i + 1 < argv.len) {
                if (std.mem.eql(u8, argv[i + 1], "pipefail")) session.options.pipefail = true;
                else if (std.mem.eql(u8, argv[i + 1], "errexit")) session.options.errexit = true;
                i += 1;
            } else if (std.mem.eql(u8, a, "+o") && i + 1 < argv.len) {
                if (std.mem.eql(u8, argv[i + 1], "pipefail")) session.options.pipefail = false;
                else if (std.mem.eql(u8, argv[i + 1], "errexit")) session.options.errexit = false;
                i += 1;
            } else {
                session.setPositionalArgs(argv[i..]);
                return 0;
            }
            i += 1;
        }
        return 0;
    }

    if (std.mem.eql(u8, name, "shift")) {
        let n: usize = 1;
        if (argv.len > 1) {
            n = std.fmt.parseInt(usize, argv[1], 10) catch 1;
        }
        if (n > session.pos_args_len) return 1;
        const alloc = std.heap.page_allocator;
        let fi: usize = 0;
        while (fi < n) {
            if (session.pos_args[fi].len > 0) {
                alloc.free(session.pos_args[fi]);
            }
            fi += 1;
        }
        let i: usize = 0;
        while (i + n < session.pos_args_len) {
            session.pos_args[i] = session.pos_args[i + n];
            i += 1;
        }
        while (i < session.pos_args_len) {
            session.pos_args[i] = "";
            i += 1;
        }
        session.pos_args_len -= n;
        return 0;
    }

    if (std.mem.eql(u8, name, "alias")) {
        if (argv.len == 1) {
            let i: usize = 0;
            while (i < session.aliases.len) {
                if (session.aliases.slots[i].active) {
                    stdout_joiner.push("alias ");
                    stdout_joiner.push(session.aliases.slots[i].name);
                    stdout_joiner.push("='");
                    stdout_joiner.push(session.aliases.slots[i].value);
                    stdout_joiner.push("'\n");
                }
                i += 1;
            }
            return 0;
        }
        let i: usize = 1;
        while (i < argv.len) {
            const item = argv[i];
            if (eq := std.mem.indexOfScalar(u8, item, '=')) {
                * = session.aliases.set(item[0..eq], item[eq + 1 ..]);
            } else if (val := session.aliases.get(item)) {
                stdout_joiner.push("alias "); stdout_joiner.push(item); stdout_joiner.push("='"); stdout_joiner.push(val); stdout_joiner.push("'\n");
            } else {
                stderr_joiner.push("alias: "); stderr_joiner.push(item); stderr_joiner.push(": not found\n");
                return 1;
            }
            i += 1;
        }
        return 0;
    }

    if (std.mem.eql(u8, name, "unalias")) {
        let i: usize = 1;
        while (i < argv.len) {
            * = session.aliases.unset(argv[i]);
            i += 1;
        }
        return 0;
    }

    if (std.mem.eql(u8, name, "break")) {
        session.loop_break = 1;
        if (argv.len > 1) {
            session.loop_break = std.fmt.parseInt(usize, argv[1], 10) catch 1;
        }
        return 0;
    }

    if (std.mem.eql(u8, name, "continue")) {
        session.loop_continue = 1;
        if (argv.len > 1) {
            session.loop_continue = std.fmt.parseInt(usize, argv[1], 10) catch 1;
        }
        return 0;
    }

    return null;
}

pub struct Executor {
    static fn applyOutputRedirections(
        cmd: Command,
        cmd_out: *StringJoiner,
        cmd_err: *StringJoiner,
        stdout_joiner: *StringJoiner,
        stderr_joiner: *StringJoiner,
        session: *ShellSession,
    ): bool {
        if (cmd.redirect_err_to_out) {
            cmd_out.push(cmd_err.slice());
            cmd_err.length = 0;
        } else if (cmd.redirect_out_to_err) {
            cmd_err.push(cmd_out.slice());
            cmd_out.length = 0;
        }

        if (cmd.redirect_all.len > 0) {
            let all_path = StringJoiner.init();
            defer all_path.~init();
            let dummy = false;
            expandWordWithSession(cmd.redirect_all, session, &all_path, &dummy, true);
            let combined = StringJoiner.init();
            defer combined.~init();
            combined.push(cmd_out.slice());
            combined.push(cmd_err.slice());
            if (!writeRedirectFile(all_path.slice(), combined.slice(), cmd.redirect_all_append)) {
                stderr_joiner.push("shell: failed to redirect output\n");
                return false;
            }
            return true;
        }

        if (cmd.redirect_out.len > 0) {
            let out_path = StringJoiner.init();
            defer out_path.~init();
            let dummy = false;
            expandWordWithSession(cmd.redirect_out, session, &out_path, &dummy, true);
            if (!writeRedirectFile(out_path.slice(), cmd_out.slice(), cmd.redirect_append)) {
                stderr_joiner.push("shell: failed to redirect stdout\n");
                return false;
            }
        } else {
            stdout_joiner.push(cmd_out.slice());
        }

        if (cmd.redirect_err.len > 0) {
            let err_path = StringJoiner.init();
            defer err_path.~init();
            let dummy = false;
            expandWordWithSession(cmd.redirect_err, session, &err_path, &dummy, true);
            if (!writeRedirectFile(err_path.slice(), cmd_err.slice(), cmd.redirect_err_append)) {
                stderr_joiner.push("shell: failed to redirect stderr\n");
                return false;
            }
        } else {
            stderr_joiner.push(cmd_err.slice());
        }
        return true;
    }

    static fn executeScriptBlockWithPool(
        script: *Script!,
        pool: *AstPool!,
        initial_stdin: []const u8,
        stdout_joiner: *StringJoiner,
        stderr_joiner: *StringJoiner,
        session: *ShellSession,
    ): i32 {
        const stmts = script.statements[0..script.statements_len];
        if (stmts.len == 0) return 0;

        let prev_op: ControlOp = .None;
        let last_code: i32 = session.last_exit_code;
        let i: usize = 0;
        while (i < stmts.len) {
            const stmt = stmts[i];
            let should_run = true;
            if (prev_op == .And && last_code != 0) should_run = false;
            else if (prev_op == .Or && last_code == 0) should_run = false;

            if (should_run) {
                const pipe_stdin = if (i == 0) initial_stdin else "";
                const res = Executor.executePipelineWithPool(stmt.pipeline, pool, pipe_stdin, session);
                last_code = res.exit_code;
                stdout_joiner.push(res.stdout_str);
                stderr_joiner.push(res.stderr_str);
                if (session.should_exit || session.loop_break > 0 || session.loop_continue > 0) {
                    break;
                }
                if (session.options.errexit && last_code != 0 && !stmt.pipeline.negated && stmt.op != .And && stmt.op != .Or) {
                    session.should_exit = true;
                    break;
                }
            }
            prev_op = stmt.op;
            i += 1;
        }
        session.last_exit_code = last_code;
        return last_code;
    }

    static fn executeCompoundCommand(
        cmd: Command,
        pool_opt: (*AstPool!)?,
        stdin_data: []const u8,
        stdout_joiner: *StringJoiner,
        stderr_joiner: *StringJoiner,
        session: *ShellSession,
    ): i32 {
        let pool = pool_opt orelse return 0;

        let cmd_out = StringJoiner.init();
        defer cmd_out.~init();
        let cmd_err = StringJoiner.init();
        defer cmd_err.~init();

        let in_buf = StringJoiner.init();
        defer in_buf.~init();
        let effective_stdin = stdin_data;
        if (cmd.redirect_herestring.len > 0) {
            let dummy = false;
            expandWordWithSession(cmd.redirect_herestring, session, &in_buf, &dummy, true);
            in_buf.push("\n");
            effective_stdin = in_buf.slice();
        } else if (cmd.redirect_in.len > 0) {
            let redir_in_path = StringJoiner.init();
            defer redir_in_path.~init();
            let dummy = false;
            expandWordWithSession(cmd.redirect_in, session, &redir_in_path, &dummy, true);
            if (!readRedirectFile(redir_in_path.slice(), &in_buf)) {
                stderr_joiner.push("shell: "); stderr_joiner.push(redir_in_path.slice()); stderr_joiner.push(": No such file or directory\n");
                return 1;
            }
            effective_stdin = in_buf.slice();
        }

        let code: i32 = 0;

        if (cmd.kind == .Subshell || cmd.kind == .Group) {
            if (cmd.compound_id < pool.subshell_nodes.items.len) {
                const node = pool.subshell_nodes.items[cmd.compound_id];
                if (sptr := pool.getScript(node.script_id)) {
                    if (node.isolated) {
                        let saved_cwd_buf: [1024]u8 = [0; 1024];
                        const saved_cwd = getcwd(&saved_cwd_buf) catch "/";
                        let sub_sess = session.cloneForSubshell();
                        defer {
                            sub_sess.~init();
                            chdir(saved_cwd) catch {};
                        }
                        code = Executor.executeScriptBlockWithPool(sptr, pool, effective_stdin, &cmd_out, &cmd_err, &sub_sess);
                    } else {
                        code = Executor.executeScriptBlockWithPool(sptr, pool, effective_stdin, &cmd_out, &cmd_err, session);
                    }
                }
            }
        } else if (cmd.kind == .If) {
            if (cmd.compound_id < pool.if_nodes.items.len) {
                const if_node = pool.if_nodes.items[cmd.compound_id];
                let matched = false;
                for (br of if_node.branches.items) {
                    if (cptr := pool.getScript(br.cond)) {
                        const saved_e = session.options.errexit;
                        session.options.errexit = false;
                        const cond_rc = Executor.executeScriptBlockWithPool(cptr, pool, effective_stdin, &cmd_out, &cmd_err, session);
                        session.options.errexit = saved_e;
                        if (cond_rc == 0) {
                            matched = true;
                            if (bptr := pool.getScript(br.body)) {
                                code = Executor.executeScriptBlockWithPool(bptr, pool, "", &cmd_out, &cmd_err, session);
                            }
                            break;
                        }
                    }
                }
                if (!matched) {
                    if (else_id := if_node.else_body) {
                        if (eptr := pool.getScript(else_id)) {
                            code = Executor.executeScriptBlockWithPool(eptr, pool, "", &cmd_out, &cmd_err, session);
                        }
                    } else {
                        code = 0;
                    }
                }
            }
        } else if (cmd.kind == .For) {
            if (cmd.compound_id < pool.for_nodes.items.len) {
                const alloc = std.heap.page_allocator;
                const for_node = pool.for_nodes.items[cmd.compound_id];
                let items: std.ArrayListUnmanaged(string) = .empty;
                defer {
                    for (it of items.items) alloc.free(it);
                    items.deinit(alloc);
                }
                if (for_node.has_in) {
                    for (w of for_node.words.items) {
                        expandWordToArgList(alloc, w.raw_slice, w.has_raw, w.slice, session, &items);
                    }
                } else {
                    let pi: usize = 0;
                    while (pi < session.pos_args_len) {
                        const dup = alloc.dupe(u8, session.pos_args[pi]) catch continue;
                        items.append(alloc, dup) catch {};
                        pi += 1;
                    }
                }
                if (bptr := pool.getScript(for_node.body)) {
                    for (val of items.items) {
                        if (for_node.var_name.len > 0) {
                            * = session.env.set(for_node.var_name, val, false);
                        }
                        code = Executor.executeScriptBlockWithPool(bptr, pool, "", &cmd_out, &cmd_err, session);
                        if (session.should_exit) break;
                        if (session.loop_break > 0) {
                            session.loop_break -= 1;
                            break;
                        }
                        if (session.loop_continue > 0) {
                            session.loop_continue -= 1;
                            if (session.loop_continue > 0) break;
                        }
                    }
                }
            }
        } else if (cmd.kind == .While || cmd.kind == .Until) {
            if (cmd.compound_id < pool.while_nodes.items.len) {
                const wnode = pool.while_nodes.items[cmd.compound_id];
                let iter_guard: usize = 0;
                while (iter_guard < 10000) {
                    iter_guard += 1;
                    let cond_rc: i32 = 1;
                    if (cptr := pool.getScript(wnode.cond)) {
                        const saved_e = session.options.errexit;
                        session.options.errexit = false;
                        cond_rc = Executor.executeScriptBlockWithPool(cptr, pool, "", &cmd_out, &cmd_err, session);
                        session.options.errexit = saved_e;
                    }
                    const keep_going = if (wnode.is_until) (cond_rc != 0) else (cond_rc == 0);
                    if (!keep_going) break;
                    if (bptr := pool.getScript(wnode.body)) {
                        code = Executor.executeScriptBlockWithPool(bptr, pool, "", &cmd_out, &cmd_err, session);
                    }
                    if (session.should_exit) break;
                    if (session.loop_break > 0) {
                        session.loop_break -= 1;
                        break;
                    }
                    if (session.loop_continue > 0) {
                        session.loop_continue -= 1;
                        if (session.loop_continue > 0) break;
                    }
                }
            }
        }

        if (!Executor.applyOutputRedirections(cmd, &cmd_out, &cmd_err, stdout_joiner, stderr_joiner, session)) {
            return 1;
        }
        return code;
    }

    pub static fn executeCommandWithSession(
        cmd: Command,
        stdin_data: []const u8,
        stdout_joiner: *StringJoiner,
        stderr_joiner: *StringJoiner,
        session: *ShellSession,
    ): i32 {
        return Executor.executeCommandWithPool(cmd, null, stdin_data, stdout_joiner, stderr_joiner, session);
    }

    static fn executeCommandWithPool(
        cmd: Command,
        pool_opt: (*AstPool!)?,
        stdin_data: []const u8,
        stdout_joiner: *StringJoiner,
        stderr_joiner: *StringJoiner,
        session: *ShellSession,
    ): i32 {
        if (cmd.kind != .Simple) {
            return Executor.executeCompoundCommand(cmd, pool_opt, stdin_data, stdout_joiner, stderr_joiner, session);
        }

        // 1. Standalone variable assignments (FOO=bar BAZ=qux)
        if (cmd.args_len == 0 && cmd.assigns_len > 0) {
            let ai: usize = 0;
            while (ai < cmd.assigns_len) {
                let val_buf = StringJoiner.init();
                defer val_buf.~init();
                let dummy = false;
                expandWordWithSession(cmd.assigns[ai].raw_value, session, &val_buf, &dummy, true);
                * = session.env.set(cmd.assigns[ai].name, val_buf.slice(), false);
                ai += 1;
            }
            return 0;
        }

        if (cmd.args_len == 0 && !cmd.hasContent()) {
            return 0;
        }

        // 2. Prefix assignments for a command (FOO=bar cmd): save previous values and restore after
        let saved_had: [16]bool = [false; 16];
        let saved_vals: [16][]const u8 = undefined;
        let saved_bufs: [16]StringJoiner = undefined;
        let ai: usize = 0;
        while (ai < cmd.assigns_len) {
            const aname = cmd.assigns[ai].name;
            saved_bufs[ai] = StringJoiner.init();
            if (prev := session.env.get(aname)) {
                saved_had[ai] = true;
                saved_bufs[ai].push(prev);
                saved_vals[ai] = saved_bufs[ai].slice();
            } else {
                saved_had[ai] = false;
                saved_vals[ai] = "";
            }
            let new_val = StringJoiner.init();
            defer new_val.~init();
            let dummy = false;
            expandWordWithSession(cmd.assigns[ai].raw_value, session, &new_val, &dummy, true);
            * = session.env.set(aname, new_val.slice(), true);
            ai += 1;
        }
        defer {
            let ri: usize = 0;
            while (ri < cmd.assigns_len) {
                if (saved_had[ri]) {
                    * = session.env.set(cmd.assigns[ri].name, saved_vals[ri], true);
                } else {
                    * = session.env.unset(cmd.assigns[ri].name);
                }
                saved_bufs[ri].~init();
                ri += 1;
            }
        }

        // 3. Expand command arguments (with brace, tilde, var, arith, cmd_subst, word-split, glob)
        const alloc = std.heap.page_allocator;
        let expanded_list: std.ArrayListUnmanaged(string) = .empty;
        defer {
            for (arg_item of expanded_list.items) alloc.free(arg_item);
            expanded_list.deinit(alloc);
        }

        let i: usize = 0;
        while (i < cmd.args_len) {
            expandWordToArgList(
                alloc,
                cmd.raw_args[i],
                cmd.has_raw_args[i],
                cmd.args[i],
                session,
                &expanded_list,
            );
            i += 1;
        }

        // Alias expansion on first argument if matched
        if (expanded_list.items.len > 0) {
            if (alias_val := session.aliases.get(expanded_list.items[0])) {
                let alias_args: std.ArrayListUnmanaged(string) = .empty;
                let a_iter = std.mem.tokenizeAny(u8, alias_val, " \t");
                while (atok := a_iter.next()) {
                    const dup = alloc.dupe(u8, atok) catch continue;
                    alias_args.append(alloc, dup) catch {};
                }
                if (alias_args.items.len > 0) {
                    alloc.free(expanded_list.items[0]);
                    let k: usize = 1;
                    while (k < expanded_list.items.len) {
                        alias_args.append(alloc, expanded_list.items[k]) catch {};
                        k += 1;
                    }
                    expanded_list.deinit(alloc);
                    expanded_list = alias_args;
                } else {
                    alias_args.deinit(alloc);
                }
            }
        }

        const argv = expanded_list.items;

        if (session.options.xtrace && argv.len > 0) {
            stderr_joiner.push("+");
            for (xa of argv) {
                stderr_joiner.push(" ");
                stderr_joiner.push(xa);
            }
            stderr_joiner.push("\n");
        }

        // 4. Input redirection (< file or <<< herestring)
        let in_buf = StringJoiner.init();
        defer in_buf.~init();
        let effective_stdin = stdin_data;
        if (cmd.redirect_herestring.len > 0) {
            let dummy = false;
            expandWordWithSession(cmd.redirect_herestring, session, &in_buf, &dummy, true);
            in_buf.push("\n");
            effective_stdin = in_buf.slice();
        } else if (cmd.redirect_in.len > 0) {
            let redir_in_path = StringJoiner.init();
            defer redir_in_path.~init();
            let dummy = false;
            expandWordWithSession(cmd.redirect_in, session, &redir_in_path, &dummy, true);
            if (!readRedirectFile(redir_in_path.slice(), &in_buf)) {
                stderr_joiner.push("shell: ");
                stderr_joiner.push(redir_in_path.slice());
                stderr_joiner.push(": No such file or directory\n");
                return 1;
            }
            effective_stdin = in_buf.slice();
        }

        let cmd_out = StringJoiner.init();
        defer cmd_out.~init();
        let cmd_err = StringJoiner.init();
        defer cmd_err.~init();

        let exit_code: i32 = 0;
        if (argv.len > 0) {
            const bin_name = argv[0];
            if (sess_code := executeSessionBuiltin(argv, &cmd_out, &cmd_err, session)) {
                exit_code = sess_code;
            } else if (isBuiltin(bin_name)) {
                exit_code = runBuiltinWithState(
                    argv,
                    effective_stdin,
                    &cmd_out,
                    &cmd_err,
                    &session.env,
                    &session.dirs,
                    &session.should_exit,
                );
            } else {
                exit_code = executeExternalPosixSpawn(
                    argv,
                    effective_stdin,
                    &cmd_out,
                    &cmd_err,
                    session,
                );
            }
        }

        // 5. Handle stderr/stdout merging & redirections
        if (!Executor.applyOutputRedirections(cmd, &cmd_out, &cmd_err, stdout_joiner, stderr_joiner, session)) {
            return 1;
        }
        return exit_code;
    }

    pub static fn executeCommand(
        cmd: Command,
        stdin_data: []const u8,
        stdout_joiner: *StringJoiner,
        stderr_joiner: *StringJoiner,
    ): i32 {
        let sess = ShellSession.init();
        defer sess.~init();
        return Executor.executeCommandWithSession(cmd, stdin_data, stdout_joiner, stderr_joiner, &sess);
    }

    static fn executePipelineWithPool(
        pipe: Pipeline,
        pool_opt: (*AstPool!)?,
        initial_stdin: []const u8,
        session: *ShellSession,
    ): ShellResult {
        const cmds = pipe.commands[0..pipe.commands_len];
        if (cmds.len == 0) {
            return ShellResult {
                exit_code: 0,
                stdout_str: "",
                stderr_str: "",
            };
        }

        let pipe_holder = StringJoiner.init();
        defer pipe_holder.~init();
        let cur_stdin = initial_stdin;
        let last_code: i32 = 0;
        let pipefail_code: i32 = 0;
        let final_stdout = StringJoiner.init();
        let final_stderr = StringJoiner.init();

        let i: usize = 0;
        while (i < cmds.len) {
            let step_stdout = StringJoiner.init();
            last_code = Executor.executeCommandWithPool(
                cmds[i],
                pool_opt,
                cur_stdin,
                &step_stdout,
                &final_stderr,
                session,
            );
            if (last_code != 0) {
                pipefail_code = last_code;
            }

            const step_slice = step_stdout.slice();
            if (i + 1 < cmds.len) {
                pipe_holder.length = 0;
                pipe_holder.push(step_slice);
                step_stdout.~init();
                cur_stdin = pipe_holder.slice();
            } else {
                final_stdout.push(step_slice);
                step_stdout.~init();
            }
            if (session.should_exit) {
                break;
            }
            i += 1;
        }

        let result_code = if (session.options.pipefail && pipefail_code != 0) pipefail_code else last_code;
        if (pipe.negated) {
            result_code = if (result_code == 0) 1 else 0;
        }

        session.last_exit_code = result_code;
        const out_str = final_stdout.slice();
        const err_str = final_stderr.slice();
        final_stdout.has_allocated = false;
        final_stderr.has_allocated = false;

        return ShellResult {
            exit_code: result_code,
            stdout_str: out_str,
            stderr_str: err_str,
        };
    }

    pub static fn executePipelineWithSession(
        pipe: Pipeline,
        initial_stdin: []const u8,
        session: *ShellSession,
    ): ShellResult {
        return Executor.executePipelineWithPool(pipe, null, initial_stdin, session);
    }

    pub static fn executePipeline(pipe: Pipeline, initial_stdin: []const u8): ShellResult {
        let sess = ShellSession.init();
        defer sess.~init();
        return Executor.executePipelineWithSession(pipe, initial_stdin, &sess);
    }

    pub static fn executeScriptPtrWithSession(script: ScriptPtr, session: *ShellSession): ShellResult {
        const stmts = script.statements[0..script.statements_len];
        if (stmts.len == 0) {
            return ShellResult {
                exit_code: 0,
                stdout_str: "",
                stderr_str: "",
            };
        }

        let accum_stdout = StringJoiner.init();
        let accum_stderr = StringJoiner.init();
        let prev_op: ControlOp = .None;
        let last_code: i32 = session.last_exit_code;

        let i: usize = 0;
        while (i < stmts.len) {
            const stmt = stmts[i];
            let should_run = true;
            if (prev_op == .And && last_code != 0) {
                should_run = false;
            } else if (prev_op == .Or && last_code == 0) {
                should_run = false;
            }

            if (should_run) {
                const res = Executor.executePipelineWithPool(stmt.pipeline, script.pool, "", session);
                last_code = res.exit_code;
                accum_stdout.push(res.stdout_str);
                accum_stderr.push(res.stderr_str);
                if (session.should_exit) {
                    break;
                }
                if (session.options.errexit && last_code != 0 && !stmt.pipeline.negated && stmt.op != .And && stmt.op != .Or) {
                    session.should_exit = true;
                    break;
                }
            }

            prev_op = stmt.op;
            i += 1;
        }

        session.last_exit_code = last_code;
        const out_str = accum_stdout.slice();
        const err_str = accum_stderr.slice();
        accum_stdout.has_allocated = false;
        accum_stderr.has_allocated = false;

        return ShellResult {
            exit_code: last_code,
            stdout_str: out_str,
            stderr_str: err_str,
        };
    }

    pub static fn executeScriptWithSession(script: Script, session: *ShellSession): ShellResult {
        return Executor.executeScriptPtrWithSession(&script, session);
    }

    pub static fn executeScript(script: Script): ShellResult {
        let sess = ShellSession.init();
        defer sess.~init();
        return Executor.executeScriptWithSession(script, &sess);
    }
}
