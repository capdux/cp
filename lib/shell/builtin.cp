// Source: Absorbed from /data/dev/bun/src/shell/Builtin.cp | Synced: 2026-11-22T03:06:36Z
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/Builtin.cp & builtin/*.cp
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/builtin.cp
// Built-in command implementations & unified dispatcher for CP Shell (cap:shell)
// Dispatches 30+ POSIX/Bun builtins:
//   Control/Env: echo, printf, pwd, cd, pushd, popd, dirs, env, export, unset,
//                which (via cap:which), true, :, false, test/[, exit
//   Filesystem:  ls, cat, mkdir, rmdir, rm, cp, mv, touch, basename, dirname, mktemp
//   Text/Seq:    seq, head, tail, wc, grep, yes, sort, uniq

const std = @import("std");
import _cap_hub from "cap";
import { StringJoiner } from "cap:string";
import { access } from "cap:sys";
import { ShellEnv, DirStack, parseShellInt64 } from "./env.cp";
import {
    statPathInfo,
    runBuiltinLs,
    runBuiltinCat,
    runBuiltinMkdir,
    runBuiltinRmdir,
    runBuiltinRm,
    runBuiltinCp,
    runBuiltinMv,
    runBuiltinTouch,
    runBuiltinBasename,
    runBuiltinDirname,
    runBuiltinMktemp,
} from "./builtin_fs.cp";
import {
    runBuiltinSeq,
    runBuiltinHead,
    runBuiltinTail,
    runBuiltinWc,
    runBuiltinGrep,
    runBuiltinYes,
    runBuiltinSort,
    runBuiltinUniq,
} from "./builtin_text.cp";

fn checkAccess(path: []const u8, mode: i32): bool {
    access(path, mode) catch return false;
    return true;
}

fn hexDigitValue(c: u8): u8? {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    return null;
}

fn appendWithEscapes(out: *StringJoiner, input: []const u8, stop_on_c: bool): bool {
    let i: usize = 0;
    while (i < input.len) {
        if (input[i] == '\\' && i + 1 < input.len) {
            const esc = input[i + 1];
            if (esc == '\\') {
                out.push("\\");
                i += 2;
            } else if (esc == 'a') {
                out.push("\x07");
                i += 2;
            } else if (esc == 'b') {
                out.push("\x08");
                i += 2;
            } else if (esc == 'c' && stop_on_c) {
                return true;
            } else if (esc == 'e' || esc == 'E') {
                out.push("\x1b");
                i += 2;
            } else if (esc == 'f') {
                out.push("\x0c");
                i += 2;
            } else if (esc == 'n') {
                out.push("\n");
                i += 2;
            } else if (esc == 'r') {
                out.push("\r");
                i += 2;
            } else if (esc == 't') {
                out.push("\t");
                i += 2;
            } else if (esc == 'v') {
                out.push("\x0b");
                i += 2;
            } else if (esc == '0') {
                i += 2;
                let val: u8 = 0;
                let digits: usize = 0;
                while (digits < 3 && i < input.len && input[i] >= '0' && input[i] <= '7') {
                    val = val *% 8 +% (input[i] - '0');
                    i += 1;
                    digits += 1;
                }
                let b: [1]u8 = [val];
                out.push(b[0..1]);
            } else if (esc == 'x') {
                i += 2;
                let val: u8 = 0;
                let digits: usize = 0;
                while (digits < 2 && i < input.len) {
                    if (hv := hexDigitValue(input[i])) {
                        val = val *% 16 +% hv;
                        i += 1;
                        digits += 1;
                    } else {
                        break;
                    }
                }
                if (digits > 0) {
                    let b: [1]u8 = [val];
                    out.push(b[0..1]);
                } else {
                    out.push("\\x");
                }
            } else {
                out.push(input[i .. i + 2]);
                i += 2;
            }
        } else {
            out.push(input[i .. i + 1]);
            i += 1;
        }
    }
    return false;
}

fn pushPadded(out: *StringJoiner, text: []const u8, width: usize, left_align: bool, pad_char: u8) {
    if (text.len >= width) {
        out.push(text);
        return;
    }
    const pad_len = width - text.len;
    let pad_buf: [128]u8 = [pad_char; 128];
    if (!left_align) {
        let rem = pad_len;
        while (rem > 0) {
            const chunk = if (rem > 128) 128 else rem;
            out.push(pad_buf[0..chunk]);
            rem -= chunk;
        }
    }
    out.push(text);
    if (left_align) {
        let rem = pad_len;
        while (rem > 0) {
            const chunk = if (rem > 128) 128 else rem;
            out.push(pad_buf[0..chunk]);
            rem -= chunk;
        }
    }
}

pub fn evalTestExpr(args: []const []const u8): i32 {
    if (args.len == 0) {
        return 1;
    }

    // Handle -o (OR) at lowest precedence
    let i: usize = 0;
    while (i < args.len) {
        if (std.mem.eql(u8, args[i], "-o")) {
            const lhs = evalTestExpr(args[0..i]);
            if (lhs == 0) {
                return 0;
            }
            return evalTestExpr(args[i + 1 ..]);
        }
        i += 1;
    }

    // Handle -a (AND)
    i = 0;
    while (i < args.len) {
        if (std.mem.eql(u8, args[i], "-a")) {
            const lhs = evalTestExpr(args[0..i]);
            if (lhs != 0) {
                return 1;
            }
            return evalTestExpr(args[i + 1 ..]);
        }
        i += 1;
    }

    // Handle unary !
    if (std.mem.eql(u8, args[0], "!")) {
        const sub = evalTestExpr(args[1..]);
        return if (sub == 0) 1 else 0;
    }

    if (args.len == 1) {
        return if (args[0].len > 0) 0 else 1;
    }

    if (args.len == 2) {
        const op = args[0];
        const val = args[1];
        if (std.mem.eql(u8, op, "-z")) return if (val.len == 0) 0 else 1;
        if (std.mem.eql(u8, op, "-n")) return if (val.len > 0) 0 else 1;
        if (std.mem.eql(u8, op, "-e")) return if (statPathInfo(val, true).exists) 0 else 1;
        if (std.mem.eql(u8, op, "-f")) return if (statPathInfo(val, true).is_file) 0 else 1;
        if (std.mem.eql(u8, op, "-d")) return if (statPathInfo(val, true).is_dir) 0 else 1;
        if (std.mem.eql(u8, op, "-L") || std.mem.eql(u8, op, "-h")) return if (statPathInfo(val, false).is_link) 0 else 1;
        if (std.mem.eql(u8, op, "-s")) {
            const st = statPathInfo(val, true);
            return if (st.exists && st.size > 0) 0 else 1;
        }
        if (std.mem.eql(u8, op, "-r")) return if (checkAccess(val, 4)) 0 else 1;
        if (std.mem.eql(u8, op, "-w")) return if (checkAccess(val, 2)) 0 else 1;
        if (std.mem.eql(u8, op, "-x")) return if (checkAccess(val, 1)) 0 else 1;
        return 2;
    }

    if (args.len == 3) {
        const lhs = args[0];
        const op = args[1];
        const rhs = args[2];
        if (std.mem.eql(u8, op, "=") || std.mem.eql(u8, op, "==")) {
            return if (std.mem.eql(u8, lhs, rhs)) 0 else 1;
        }
        if (std.mem.eql(u8, op, "!=")) {
            return if (!std.mem.eql(u8, lhs, rhs)) 0 else 1;
        }
        const a = parseShellInt64(lhs) ?? return 2;
        const b = parseShellInt64(rhs) ?? return 2;
        if (std.mem.eql(u8, op, "-eq")) return if (a == b) 0 else 1;
        if (std.mem.eql(u8, op, "-ne")) return if (a != b) 0 else 1;
        if (std.mem.eql(u8, op, "-lt")) return if (a < b) 0 else 1;
        if (std.mem.eql(u8, op, "-le")) return if (a <= b) 0 else 1;
        if (std.mem.eql(u8, op, "-gt")) return if (a > b) 0 else 1;
        if (std.mem.eql(u8, op, "-ge")) return if (a >= b) 0 else 1;
        return 2;
    }

    return 2;
}

pub fn isBuiltin(name: []const u8): bool {
    return std.mem.eql(u8, name, "echo") ||
           std.mem.eql(u8, name, "printf") ||
           std.mem.eql(u8, name, "pwd") ||
           std.mem.eql(u8, name, "cd") ||
           std.mem.eql(u8, name, "pushd") ||
           std.mem.eql(u8, name, "popd") ||
           std.mem.eql(u8, name, "dirs") ||
           std.mem.eql(u8, name, "env") ||
           std.mem.eql(u8, name, "export") ||
           std.mem.eql(u8, name, "unset") ||
           std.mem.eql(u8, name, "which") ||
           std.mem.eql(u8, name, "true") ||
           std.mem.eql(u8, name, ":") ||
           std.mem.eql(u8, name, "false") ||
           std.mem.eql(u8, name, "test") ||
           std.mem.eql(u8, name, "[") ||
           std.mem.eql(u8, name, "exit") ||
           std.mem.eql(u8, name, "ls") ||
           std.mem.eql(u8, name, "cat") ||
           std.mem.eql(u8, name, "mkdir") ||
           std.mem.eql(u8, name, "rmdir") ||
           std.mem.eql(u8, name, "rm") ||
           std.mem.eql(u8, name, "cp") ||
           std.mem.eql(u8, name, "mv") ||
           std.mem.eql(u8, name, "touch") ||
           std.mem.eql(u8, name, "basename") ||
           std.mem.eql(u8, name, "dirname") ||
           std.mem.eql(u8, name, "mktemp") ||
           std.mem.eql(u8, name, "seq") ||
           std.mem.eql(u8, name, "head") ||
           std.mem.eql(u8, name, "tail") ||
           std.mem.eql(u8, name, "wc") ||
           std.mem.eql(u8, name, "grep") ||
           std.mem.eql(u8, name, "yes") ||
           std.mem.eql(u8, name, "sort") ||
           std.mem.eql(u8, name, "uniq");
}

pub fn runBuiltinWithState(
    argv: []const []const u8,
    stdin: []const u8,
    stdout: *StringJoiner,
    stderr: *StringJoiner,
    env: *ShellEnv,
    dirs: *DirStack,
    should_exit: *bool,
): i32 {
    if (argv.len == 0) {
        return 0;
    }
    const name = argv[0];

    // echo [-n] [-e] [-E] [args...]
    if (std.mem.eql(u8, name, "echo")) {
        let no_newline = false;
        let escape_seqs = false;
        let arg_idx: usize = 1;
        while (arg_idx < argv.len) {
            const flag = argv[arg_idx];
            if (flag.len < 2 || flag[0] != '-') break;
            let valid = true;
            let k: usize = 1;
            while (k < flag.len) {
                const fc = flag[k];
                if (fc != 'n' && fc != 'e' && fc != 'E') {
                    valid = false;
                    break;
                }
                k += 1;
            }
            if (!valid) break;
            k = 1;
            while (k < flag.len) {
                const fc = flag[k];
                if (fc == 'n') no_newline = true;
                else if (fc == 'e') escape_seqs = true;
                else if (fc == 'E') escape_seqs = false;
                k += 1;
            }
            arg_idx += 1;
        }

        let stop_output = false;
        let i = arg_idx;
        while (i < argv.len) {
            if (stop_output) break;
            if (escape_seqs) {
                stop_output = appendWithEscapes(stdout, argv[i], true);
            } else {
                stdout.push(argv[i]);
            }
            if (!stop_output && i + 1 < argv.len) {
                stdout.push(" ");
            }
            i += 1;
        }
        if (!stop_output && !no_newline) {
            stdout.push("\n");
        }
        return 0;
    }

    // printf FORMAT [ARGS...]
    if (std.mem.eql(u8, name, "printf")) {
        if (argv.len < 2) {
            stderr.push("printf: missing format operand\n");
            return 1;
        }
        const fmt = argv[1];
        let arg_i: usize = 2;
        let fi: usize = 0;
        while (fi < fmt.len) {
            const ch = fmt[fi];
            if (ch == '\\' && fi + 1 < fmt.len) {
                let single_esc = StringJoiner.init();
                defer single_esc.~init();
                * = appendWithEscapes(&single_esc, fmt[fi .. fi + 2], false);
                if (fmt[fi + 1] == '0' || fmt[fi + 1] == 'x') {
                    let end_esc = fi + 2;
                    let max_digits: usize = if (fmt[fi + 1] == '0') 3 else 2;
                    let d: usize = 0;
                    while (d < max_digits && end_esc < fmt.len) {
                        end_esc += 1;
                        d += 1;
                    }
                    let multi_esc = StringJoiner.init();
                    defer multi_esc.~init();
                    * = appendWithEscapes(&multi_esc, fmt[fi..end_esc], false);
                    stdout.push(multi_esc.slice());
                    fi = end_esc;
                } else {
                    stdout.push(single_esc.slice());
                    fi += 2;
                }
            } else if (ch == '%' && fi + 1 < fmt.len) {
                fi += 1;
                if (fmt[fi] == '%') {
                    stdout.push("%");
                    fi += 1;
                    continue;
                }
                let left_align = false;
                let zero_pad = false;
                while (fi < fmt.len && (fmt[fi] == '-' || fmt[fi] == '0')) {
                    if (fmt[fi] == '-') left_align = true;
                    if (fmt[fi] == '0') zero_pad = true;
                    fi += 1;
                }
                let width: usize = 0;
                while (fi < fmt.len && fmt[fi] >= '0' && fmt[fi] <= '9') {
                    width = width * 10 + ((fmt[fi] - '0') as usize);
                    fi += 1;
                }
                if (fi >= fmt.len) break;
                const spec = fmt[fi];
                fi += 1;
                const arg_str = if (arg_i < argv.len) argv[arg_i] else "";
                if (arg_i < argv.len) arg_i += 1;
                const pad_ch: u8 = if (zero_pad && !left_align) '0' else ' ';
                if (spec == 's') {
                    pushPadded(stdout, arg_str, width, left_align, ' ');
                } else if (spec == 'c') {
                    const c_slice = if (arg_str.len > 0) arg_str[0..1] else "\x00";
                    pushPadded(stdout, c_slice, width, left_align, ' ');
                } else if (spec == 'd' || spec == 'i') {
                    const n = parseShellInt64(arg_str) ?? 0;
                    let tmp: [32]u8 = [0; 32];
                    const num_str = std.fmt.bufPrint(tmp[0..32], "{d}", .{n}) catch "0";
                    pushPadded(stdout, num_str, width, left_align, pad_ch);
                } else if (spec == 'u') {
                    const n = std.fmt.parseInt(u64, arg_str, 10) catch 0;
                    let tmp: [32]u8 = [0; 32];
                    const num_str = std.fmt.bufPrint(tmp[0..32], "{d}", .{n}) catch "0";
                    pushPadded(stdout, num_str, width, left_align, pad_ch);
                } else if (spec == 'x' || spec == 'X') {
                    const n = std.fmt.parseInt(u64, arg_str, 10) catch 0;
                    let tmp: [32]u8 = [0; 32];
                    const num_str = if (spec == 'x')
                        (std.fmt.bufPrint(tmp[0..32], "{x}", .{n}) catch "0")
                    else
                        (std.fmt.bufPrint(tmp[0..32], "{X}", .{n}) catch "0");
                    pushPadded(stdout, num_str, width, left_align, pad_ch);
                } else if (spec == 'o') {
                    const n = std.fmt.parseInt(u64, arg_str, 10) catch 0;
                    let tmp: [32]u8 = [0; 32];
                    const num_str = std.fmt.bufPrint(tmp[0..32], "{o}", .{n}) catch "0";
                    pushPadded(stdout, num_str, width, left_align, pad_ch);
                } else {
                    stdout.push("%");
                    stdout.push(fmt[fi - 1 .. fi]);
                }
            } else {
                stdout.push(fmt[fi .. fi + 1]);
                fi += 1;
            }
        }
        return 0;
    }

    // pwd
    if (std.mem.eql(u8, name, "pwd")) {
        dirs.refreshCwd();
        stdout.push(dirs.cwd());
        stdout.push("\n");
        return 0;
    }

    // cd [dir]
    if (std.mem.eql(u8, name, "cd")) {
        const target = if (argv.len > 1) argv[1] else "";
        if (!dirs.chdirTo(target, env)) {
            stderr.push("cd: failed to change directory to ");
            stderr.push(if (target.len > 0) target else "$HOME");
            stderr.push("\n");
            return 1;
        }
        if (argv.len > 1 && std.mem.eql(u8, argv[1], "-")) {
            stdout.push(dirs.cwd());
            stdout.push("\n");
        }
        return 0;
    }

    // pushd [dir]
    if (std.mem.eql(u8, name, "pushd")) {
        if (argv.len < 2) {
            stderr.push("pushd: missing directory\n");
            return 1;
        }
        if (!dirs.pushd(argv[1], env)) {
            stderr.push("pushd: failed to push directory\n");
            return 1;
        }
        stdout.push(dirs.cwd());
        stdout.push("\n");
        return 0;
    }

    // popd
    if (std.mem.eql(u8, name, "popd")) {
        if (new_cwd := dirs.popd(env)) {
            stdout.push(new_cwd);
            stdout.push("\n");
            return 0;
        }
        stderr.push("popd: directory stack empty\n");
        return 1;
    }

    // dirs
    if (std.mem.eql(u8, name, "dirs")) {
        stdout.push(dirs.cwd());
        let si = dirs.stack_len;
        while (si > 0) {
            si -= 1;
            stdout.push(" ");
            stdout.push(dirs.stack[si][0..dirs.stack_lens[si]]);
        }
        stdout.push("\n");
        return 0;
    }

    // env
    if (std.mem.eql(u8, name, "env")) {
        let i: usize = 0;
        while (i < env.len) {
            if (env.entries[i].state == .Set && env.entries[i].exported) {
                stdout.push(env.entries[i].key);
                stdout.push("=");
                stdout.push(env.entries[i].val);
                stdout.push("\n");
            }
            i += 1;
        }
        return 0;
    }

    // export [KEY=VAL | KEY ...]
    if (std.mem.eql(u8, name, "export")) {
        if (argv.len == 1) {
            let i: usize = 0;
            while (i < env.len) {
                if (env.entries[i].state == .Set && env.entries[i].exported) {
                    stdout.push("export ");
                    stdout.push(env.entries[i].key);
                    stdout.push("=\"");
                    stdout.push(env.entries[i].val);
                    stdout.push("\"\n");
                }
                i += 1;
            }
            return 0;
        }
        let idx: usize = 1;
        while (idx < argv.len) {
            const item = argv[idx];
            if (eq_pos := std.mem.indexOfScalar(u8, item, '=')) {
                const k = item[0..eq_pos];
                const v = item[eq_pos + 1 ..];
                if (!ShellEnv.isValidIdent(k)) {
                    stderr.push("export: invalid identifier: ");
                    stderr.push(k);
                    stderr.push("\n");
                    return 1;
                }
                * = env.set(k, v, true);
            } else {
                if (!ShellEnv.isValidIdent(item)) {
                    stderr.push("export: invalid identifier: ");
                    stderr.push(item);
                    stderr.push("\n");
                    return 1;
                }
                const existing = env.get(item) ?? "";
                * = env.set(item, existing, true);
            }
            idx += 1;
        }
        return 0;
    }

    // unset KEY...
    if (std.mem.eql(u8, name, "unset")) {
        let idx: usize = 1;
        while (idx < argv.len) {
            * = env.unset(argv[idx]);
            idx += 1;
        }
        return 0;
    }

    // which [-a] [cmd...] via cap:which
    if (std.mem.eql(u8, name, "which")) {
        let show_all = false;
        let idx: usize = 1;
        while (idx < argv.len && std.mem.eql(u8, argv[idx], "-a")) {
            show_all = true;
            idx += 1;
        }
        if (idx >= argv.len) {
            stderr.push("which: missing argument\n");
            return 1;
        }
        const opts = _cap_hub.which.WhichOptions {
            path: env.get("PATH"),
            cwd: dirs.cwd(),
            pathext: null,
        };
        let all_found = true;
        while (idx < argv.len) {
            const cmd_to_find = argv[idx];
            if (isBuiltin(cmd_to_find)) {
                stdout.push(cmd_to_find);
                stdout.push(": shell built-in command\n");
                if (show_all) {
                    let path_buf: [4096]u8 = undefined;
                    if (found := _cap_hub.which.Which.which(cmd_to_find, opts, &path_buf)) {
                        stdout.push(found);
                        stdout.push("\n");
                    }
                }
            } else if (show_all) {
                let path_buf: [8192]u8 = undefined;
                let matches: [32][]const u8 = undefined;
                const n = _cap_hub.which.Which.whichAll(cmd_to_find, opts, &path_buf, &matches);
                if (n == 0) {
                    all_found = false;
                } else {
                    let mi: usize = 0;
                    while (mi < n) {
                        stdout.push(matches[mi]);
                        stdout.push("\n");
                        mi += 1;
                    }
                }
            } else {
                let path_buf: [4096]u8 = undefined;
                if (found := _cap_hub.which.Which.which(cmd_to_find, opts, &path_buf)) {
                    stdout.push(found);
                    stdout.push("\n");
                } else {
                    all_found = false;
                }
            }
            idx += 1;
        }
        return if (all_found) 0 else 1;
    }

    // true / : / false
    if (std.mem.eql(u8, name, "true") || std.mem.eql(u8, name, ":")) return 0;
    if (std.mem.eql(u8, name, "false")) return 1;

    // test / [
    if (std.mem.eql(u8, name, "test")) return evalTestExpr(argv[1..]);
    if (std.mem.eql(u8, name, "[")) {
        if (argv.len < 2 || !std.mem.eql(u8, argv[argv.len - 1], "]")) {
            stderr.push("[: missing `]`\n");
            return 2;
        }
        return evalTestExpr(argv[1 .. argv.len - 1]);
    }

    // exit [code]
    if (std.mem.eql(u8, name, "exit")) {
        should_exit.* = true;
        if (argv.len > 1) {
            if (parsed := parseShellInt64(argv[1])) {
                let code = @rem(parsed, 256);
                if (code < 0) code += 256;
                return code as i32;
            }
            stderr.push("exit: numeric argument required\n");
            return 2;
        }
        return 0;
    }

    // Filesystem builtins (lib/shell/builtin_fs.cp)
    if (std.mem.eql(u8, name, "ls")) return runBuiltinLs(argv, stdout, stderr);
    if (std.mem.eql(u8, name, "cat")) return runBuiltinCat(argv, stdin, stdout, stderr);
    if (std.mem.eql(u8, name, "mkdir")) return runBuiltinMkdir(argv, stderr);
    if (std.mem.eql(u8, name, "rmdir")) return runBuiltinRmdir(argv, stderr);
    if (std.mem.eql(u8, name, "rm")) return runBuiltinRm(argv, stdout, stderr);
    if (std.mem.eql(u8, name, "cp")) return runBuiltinCp(argv, stdout, stderr);
    if (std.mem.eql(u8, name, "mv")) return runBuiltinMv(argv, stdout, stderr);
    if (std.mem.eql(u8, name, "touch")) return runBuiltinTouch(argv, stderr);
    if (std.mem.eql(u8, name, "basename")) return runBuiltinBasename(argv, stdout);
    if (std.mem.eql(u8, name, "dirname")) return runBuiltinDirname(argv, stdout);
    if (std.mem.eql(u8, name, "mktemp")) return runBuiltinMktemp(argv, stdout, stderr);

    // Text & sequence builtins (lib/shell/builtin_text.cp)
    if (std.mem.eql(u8, name, "seq")) return runBuiltinSeq(argv, stdout, stderr);
    if (std.mem.eql(u8, name, "head")) return runBuiltinHead(argv, stdin, stdout, stderr);
    if (std.mem.eql(u8, name, "tail")) return runBuiltinTail(argv, stdin, stdout, stderr);
    if (std.mem.eql(u8, name, "wc")) return runBuiltinWc(argv, stdin, stdout, stderr);
    if (std.mem.eql(u8, name, "grep")) return runBuiltinGrep(argv, stdin, stdout, stderr);
    if (std.mem.eql(u8, name, "yes")) return runBuiltinYes(argv, stdout);
    if (std.mem.eql(u8, name, "sort")) return runBuiltinSort(argv, stdin, stdout, stderr);
    if (std.mem.eql(u8, name, "uniq")) return runBuiltinUniq(argv, stdin, stdout, stderr);

    return 127;
}

pub fn runBuiltin(
    argv: []const []const u8,
    stdin: []const u8,
    stdout: *StringJoiner,
    stderr: *StringJoiner,
): i32 {
    let env = ShellEnv.init();
    defer env.~init();
    let dirs = DirStack.init();
    let should_exit = false;
    return runBuiltinWithState(argv, stdin, stdout, stderr, &env, &dirs, &should_exit);
}
