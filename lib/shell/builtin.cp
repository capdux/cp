// Source: /data/dev/bun/src/shell/Builtin.cp
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/Builtin.cp & builtin/*.cp
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/builtin.cp
// Built-in command implementations for CP Shell (cap:shell)
// Implements echo (-n/-e/-E), cat, true, false, test/[, printf, basename, dirname,
// head, wc, mkdir (-p), rm (-r/-f), pwd, cd, pushd, popd, dirs, env, export, unset,
// which, and exit.

const std = @import("std");
import { StringJoiner } from "../string/joiner.cp";
import { PathString } from "../string/path.cp";
import { ShellEnv, DirStack } from "./env.cp";

fn toCStr(path: []const u8, buf: []u8): ([*:0]const u8)? {
    if (path.len >= buf.len) {
        return null;
    }
    @memcpy(buf[0..path.len], path);
    buf[path.len] = 0;
    return buf[0..path.len :0].ptr;
}

fn readFileInto(path: []const u8, out: *StringJoiner): bool {
    let path_buf: [1024]u8 = [0; 1024];
    const c_path = toCStr(path, &path_buf) ?? return false;
    let ok = false;
    zig {
        const z_flags = std.c.O{ .CLOEXEC = true };
        const fd = std.c.open(c_path, z_flags, @as(std.c.mode_t, 0));
        if (fd >= 0) {
            defer _ = std.c.close(fd);
            var fbuf: [4096]u8 = undefined;
            while (true) {
                const n = std.c.read(fd, &fbuf, fbuf.len);
                if (n <= 0) break;
                const count: usize = @intCast(n);
                out.push(fbuf[0..count]);
            }
            ok = true;
        }
    }
    return ok;
}

fn statPathInfo(path: []const u8, follow_links: bool): { exists: bool, is_file: bool, is_dir: bool, is_link: bool, size: u64 } {
    let path_buf: [1024]u8 = [0; 1024];
    const c_path = toCStr(path, &path_buf) ?? return { exists: false, is_file: false, is_dir: false, is_link: false, size: 0 };
    let exists = false;
    let is_file = false;
    let is_dir = false;
    let is_link = false;
    let sz: u64 = 0;
    zig {
        var stx: std.os.linux.Statx = undefined;
        const mask = std.os.linux.STATX{ .TYPE = true, .MODE = true, .SIZE = true };
        const flags: u32 = if (follow_links) 0 else std.os.linux.AT.SYMLINK_NOFOLLOW;
        const rc = std.c.statx(std.os.linux.AT.FDCWD, c_path, flags, mask, &stx);
        if (rc == 0) {
            exists = true;
            const kind = stx.mode & 0o170000;
            is_file = (kind == 0o100000);
            is_dir = (kind == 0o040000);
            is_link = (kind == 0o120000);
            sz = stx.size;
        }
    }
    return { exists: exists, is_file: is_file, is_dir: is_dir, is_link: is_link, size: sz };
}

fn checkAccess(path: []const u8, mode: i32): bool {
    let path_buf: [1024]u8 = [0; 1024];
    const c_path = toCStr(path, &path_buf) ?? return false;
    let ok = false;
    zig {
        if (std.c.access(c_path, @intCast(mode)) == 0) {
            ok = true;
        }
    }
    return ok;
}

fn parseInt64(s: []const u8): i64? {
    if (s.len == 0) {
        return null;
    }
    return std.fmt.parseInt(i64, s, 10) catch null;
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

fn evalTestExpr(args: []const []const u8): i32 {
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
        if (std.mem.eql(u8, op, "-z")) {
            return if (val.len == 0) 0 else 1;
        }
        if (std.mem.eql(u8, op, "-n")) {
            return if (val.len > 0) 0 else 1;
        }
        if (std.mem.eql(u8, op, "-e")) {
            return if (statPathInfo(val, true).exists) 0 else 1;
        }
        if (std.mem.eql(u8, op, "-f")) {
            return if (statPathInfo(val, true).is_file) 0 else 1;
        }
        if (std.mem.eql(u8, op, "-d")) {
            return if (statPathInfo(val, true).is_dir) 0 else 1;
        }
        if (std.mem.eql(u8, op, "-L") || std.mem.eql(u8, op, "-h")) {
            return if (statPathInfo(val, false).is_link) 0 else 1;
        }
        if (std.mem.eql(u8, op, "-s")) {
            const st = statPathInfo(val, true);
            return if (st.exists && st.size > 0) 0 else 1;
        }
        if (std.mem.eql(u8, op, "-r")) {
            return if (checkAccess(val, 4)) 0 else 1;
        }
        if (std.mem.eql(u8, op, "-w")) {
            return if (checkAccess(val, 2)) 0 else 1;
        }
        if (std.mem.eql(u8, op, "-x")) {
            return if (checkAccess(val, 1)) 0 else 1;
        }
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
        const a = parseInt64(lhs) ?? return 2;
        const b = parseInt64(rhs) ?? return 2;
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

fn mkdirSingle(path: []const u8): bool {
    let path_buf: [1024]u8 = [0; 1024];
    const c_path = toCStr(path, &path_buf) ?? return false;
    let ok = false;
    zig {
        if (std.c.mkdir(c_path, @as(std.c.mode_t, 0o755)) == 0) {
            ok = true;
        }
    }
    return ok;
}

fn mkdirRecursive(path: []const u8): bool {
    if (path.len == 0 || path.len >= 1024) {
        return false;
    }
    if (statPathInfo(path, true).is_dir) {
        return true;
    }
    let tmp: [1024]u8 = [0; 1024];
    @memcpy(tmp[0..path.len], path);
    let i: usize = 1;
    while (i <= path.len) {
        if (i == path.len || tmp[i] == '/') {
            const sub = tmp[0..i];
            if (!statPathInfo(sub, true).is_dir) {
                if (!mkdirSingle(sub)) {
                    if (!statPathInfo(sub, true).is_dir) {
                        return false;
                    }
                }
            }
        }
        i += 1;
    }
    return true;
}

fn removePathRecursive(path: []const u8, recursive: bool, force: bool): bool {
    const info = statPathInfo(path, false);
    if (!info.exists) {
        return force;
    }
    if (!info.is_dir) {
        let path_buf: [1024]u8 = [0; 1024];
        const c_path = toCStr(path, &path_buf) ?? return false;
        let ok = false;
        zig {
            if (std.c.unlink(c_path) == 0) {
                ok = true;
            }
        }
        return ok;
    }
    if (!recursive) {
        return false;
    }

    let dir_buf: [1024]u8 = [0; 1024];
    const c_dir = toCStr(path, &dir_buf) ?? return false;
    let children_ok = true;
    zig {
        const dir = std.c.opendir(c_dir);
        if (dir) |d| {
            defer _ = std.c.closedir(d);
            while (std.c.readdir(d)) |entry| {
                var name_len: usize = 0;
                while (name_len < 256 and entry.name[name_len] != 0) : (name_len += 1) {}
                const name = entry.name[0..name_len];
                if (std.mem.eql(u8, name, ".") or std.mem.eql(u8, name, "..")) continue;
                var child_buf: [1024]u8 = undefined;
                if (path.len + 1 + name.len >= child_buf.len) {
                    children_ok = false;
                    continue;
                }
                @memcpy(child_buf[0..path.len], path);
                var pos: usize = path.len;
                if (pos > 0 and child_buf[pos - 1] != '/') {
                    child_buf[pos] = '/';
                    pos += 1;
                }
                @memcpy(child_buf[pos .. pos + name.len], name);
                pos += name.len;
                if (!removePathRecursive(child_buf[0..pos], true, force)) {
                    children_ok = false;
                }
            }
        } else {
            children_ok = false;
        }
    }
    if (!children_ok) {
        return false;
    }
    let rmdir_ok = false;
    zig {
        if (std.c.rmdir(c_dir) == 0) {
            rmdir_ok = true;
        }
    }
    return rmdir_ok;
}

fn findInPath(cmd: []const u8, env: *ShellEnv, out: *StringJoiner): bool {
    if (std.mem.indexOfScalar(u8, cmd, '/') != null) {
        if (checkAccess(cmd, 1)) {
            out.push(cmd);
            return true;
        }
        return false;
    }
    const path_env = env.get("PATH") ?? "/usr/local/bin:/usr/bin:/bin";
    let start: usize = 0;
    while (start <= path_env.len) {
        let end = start;
        while (end < path_env.len && path_env[end] != ':') {
            end += 1;
        }
        const dir = path_env[start..end];
        if (dir.len > 0 && dir.len + 1 + cmd.len < 1024) {
            let full_buf: [1024]u8 = [0; 1024];
            @memcpy(full_buf[0..dir.len], dir);
            let pos = dir.len;
            if (full_buf[pos - 1] != '/') {
                full_buf[pos] = '/';
                pos += 1;
            }
            @memcpy(full_buf[pos .. pos + cmd.len], cmd);
            pos += cmd.len;
            const full_slice = full_buf[0..pos];
            const st = statPathInfo(full_slice, true);
            if (st.exists && !st.is_dir && checkAccess(full_slice, 1)) {
                out.push(full_slice);
                return true;
            }
        }
        if (end >= path_env.len) {
            break;
        }
        start = end + 1;
    }
    return false;
}

fn writeHeadContent(content: []const u8, max_lines: usize, out: *StringJoiner) {
    if (max_lines == 0) {
        return;
    }
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

fn countWc(content: []const u8): { lines: usize, words: usize, bytes: usize } {
    let lines: usize = 0;
    let words: usize = 0;
    let in_word = false;
    let i: usize = 0;
    while (i < content.len) {
        const c = content[i];
        if (c == '\n') {
            lines += 1;
        }
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

fn pushUsizeDecimal(out: *StringJoiner, v: usize) {
    let tmp: [32]u8 = [0; 32];
    const s = std.fmt.bufPrint(tmp[0..32], "{d}", .{v}) catch "0";
    out.push(s);
}

pub fn isBuiltin(name: []const u8): bool {
    return std.mem.eql(u8, name, "echo") ||
           std.mem.eql(u8, name, "pwd") ||
           std.mem.eql(u8, name, "cd") ||
           std.mem.eql(u8, name, "pushd") ||
           std.mem.eql(u8, name, "popd") ||
           std.mem.eql(u8, name, "dirs") ||
           std.mem.eql(u8, name, "which") ||
           std.mem.eql(u8, name, "env") ||
           std.mem.eql(u8, name, "export") ||
           std.mem.eql(u8, name, "unset") ||
           std.mem.eql(u8, name, "true") ||
           std.mem.eql(u8, name, "false") ||
           std.mem.eql(u8, name, "test") ||
           std.mem.eql(u8, name, "[") ||
           std.mem.eql(u8, name, "printf") ||
           std.mem.eql(u8, name, "exit") ||
           std.mem.eql(u8, name, "dirname") ||
           std.mem.eql(u8, name, "basename") ||
           std.mem.eql(u8, name, "cat") ||
           std.mem.eql(u8, name, "head") ||
           std.mem.eql(u8, name, "wc") ||
           std.mem.eql(u8, name, "mkdir") ||
           std.mem.eql(u8, name, "rm");
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
            if (flag.len < 2 || flag[0] != '-') {
                break;
            }
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
            if (!valid) {
                break;
            }
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
            if (stop_output) {
                break;
            }
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
                defer single_esc.release();
                * = appendWithEscapes(&single_esc, fmt[fi .. fi + 2], false);
                // Handle \xHH or \0nnn by scanning ahead
                if (fmt[fi + 1] == '0' || fmt[fi + 1] == 'x') {
                    let end_esc = fi + 2;
                    let max_digits: usize = if (fmt[fi + 1] == '0') 3 else 2;
                    let d: usize = 0;
                    while (d < max_digits && end_esc < fmt.len) {
                        end_esc += 1;
                        d += 1;
                    }
                    let multi_esc = StringJoiner.init();
                    defer multi_esc.release();
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
                if (fi >= fmt.len) {
                    break;
                }
                const spec = fmt[fi];
                fi += 1;
                const arg_str = if (arg_i < argv.len) argv[arg_i] else "";
                if (arg_i < argv.len) {
                    arg_i += 1;
                }
                const pad_ch: u8 = if (zero_pad && !left_align) '0' else ' ';
                if (spec == 's') {
                    pushPadded(stdout, arg_str, width, left_align, ' ');
                } else if (spec == 'c') {
                    const c_slice = if (arg_str.len > 0) arg_str[0..1] else "\x00";
                    pushPadded(stdout, c_slice, width, left_align, ' ');
                } else if (spec == 'd' || spec == 'i') {
                    const n = parseInt64(arg_str) ?? 0;
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

    // which [cmd...]
    if (std.mem.eql(u8, name, "which")) {
        if (argv.len < 2) {
            stderr.push("which: missing argument\n");
            return 1;
        }
        let all_found = true;
        let idx: usize = 1;
        while (idx < argv.len) {
            const cmd_to_find = argv[idx];
            if (isBuiltin(cmd_to_find)) {
                stdout.push(cmd_to_find);
                stdout.push(": shell built-in command\n");
            } else {
                let found_path = StringJoiner.init();
                defer found_path.release();
                if (findInPath(cmd_to_find, env, &found_path)) {
                    stdout.push(found_path.slice());
                    stdout.push("\n");
                } else {
                    all_found = false;
                }
            }
            idx += 1;
        }
        return if (all_found) 0 else 1;
    }

    // true / false
    if (std.mem.eql(u8, name, "true")) {
        return 0;
    }
    if (std.mem.eql(u8, name, "false")) {
        return 1;
    }

    // test / [
    if (std.mem.eql(u8, name, "test")) {
        return evalTestExpr(argv[1..]);
    }
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
            if (parsed := parseInt64(argv[1])) {
                let code = @rem(parsed, 256);
                if (code < 0) code += 256;
                return code as i32;
            }
            stderr.push("exit: numeric argument required\n");
            return 2;
        }
        return 0;
    }

    // dirname [path]
    if (std.mem.eql(u8, name, "dirname")) {
        if (argv.len < 2) {
            stdout.push(".\n");
            return 0;
        }
        const p = PathString.init(argv[1]);
        stdout.push(p.dirname());
        stdout.push("\n");
        return 0;
    }

    // basename [path] [suffix]
    if (std.mem.eql(u8, name, "basename")) {
        if (argv.len < 2) {
            stdout.push("\n");
            return 0;
        }
        const p = PathString.init(argv[1]);
        let base = p.basename();
        if (argv.len >= 3) {
            const suffix = argv[2];
            if (suffix.len > 0 && base.len > suffix.len && std.mem.endsWith(u8, base, suffix)) {
                base = base[0 .. base.len - suffix.len];
            }
        }
        stdout.push(base);
        stdout.push("\n");
        return 0;
    }

    // cat [-n] [files...]
    if (std.mem.eql(u8, name, "cat")) {
        let idx: usize = 1;
        while (idx < argv.len && std.mem.eql(u8, argv[idx], "-n")) {
            idx += 1;
        }
        if (idx >= argv.len) {
            stdout.push(stdin);
            return 0;
        }
        while (idx < argv.len) {
            const fpath = argv[idx];
            if (std.mem.eql(u8, fpath, "-")) {
                stdout.push(stdin);
            } else if (!readFileInto(fpath, stdout)) {
                stderr.push("cat: ");
                stderr.push(fpath);
                stderr.push(": No such file or directory\n");
                return 1;
            }
            idx += 1;
        }
        return 0;
    }

    // head [-n NUM | -NUM] [files...]
    if (std.mem.eql(u8, name, "head")) {
        let num_lines: usize = 10;
        let idx: usize = 1;
        while (idx < argv.len) {
            const a = argv[idx];
            if (std.mem.eql(u8, a, "-n") && idx + 1 < argv.len) {
                if (n := parseInt64(argv[idx + 1])) {
                    num_lines = if (n < 0) 0 else (n as usize);
                }
                idx += 2;
            } else if (a.len >= 2 && a[0] == '-' && a[1] >= '0' && a[1] <= '9') {
                if (n := parseInt64(a[1..])) {
                    num_lines = if (n < 0) 0 else (n as usize);
                }
                idx += 1;
            } else {
                break;
            }
        }
        if (idx >= argv.len) {
            writeHeadContent(stdin, num_lines, stdout);
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
            writeHeadContent(fbuf.slice(), num_lines, stdout);
            idx += 1;
        }
        return 0;
    }

    // wc [-l] [-w] [-c] [files...]
    if (std.mem.eql(u8, name, "wc")) {
        let show_l = false;
        let show_w = false;
        let show_c = false;
        let idx: usize = 1;
        while (idx < argv.len) {
            const a = argv[idx];
            if (a.len < 2 || a[0] != '-') {
                break;
            }
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
            if (show_l) { pushUsizeDecimal(stdout, stats.lines); first = false; }
            if (show_w) { if (!first) stdout.push(" "); pushUsizeDecimal(stdout, stats.words); first = false; }
            if (show_c) { if (!first) stdout.push(" "); pushUsizeDecimal(stdout, stats.bytes); }
            stdout.push("\n");
            return 0;
        }

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
            let first = true;
            if (show_l) { pushUsizeDecimal(stdout, stats.lines); first = false; }
            if (show_w) { if (!first) stdout.push(" "); pushUsizeDecimal(stdout, stats.words); first = false; }
            if (show_c) { if (!first) stdout.push(" "); pushUsizeDecimal(stdout, stats.bytes); }
            stdout.push(" ");
            stdout.push(argv[idx]);
            stdout.push("\n");
            idx += 1;
        }
        return 0;
    }

    // mkdir [-p] DIR...
    if (std.mem.eql(u8, name, "mkdir")) {
        let parents = false;
        let idx: usize = 1;
        while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-') {
            if (std.mem.indexOfScalar(u8, argv[idx], 'p') != null) {
                parents = true;
            }
            idx += 1;
        }
        if (idx >= argv.len) {
            stderr.push("mkdir: missing operand\n");
            return 1;
        }
        let code: i32 = 0;
        while (idx < argv.len) {
            const ok = if (parents) mkdirRecursive(argv[idx]) else mkdirSingle(argv[idx]);
            if (!ok) {
                stderr.push("mkdir: cannot create directory '");
                stderr.push(argv[idx]);
                stderr.push("'\n");
                code = 1;
            }
            idx += 1;
        }
        return code;
    }

    // rm [-f] [-r|-R] PATH...
    if (std.mem.eql(u8, name, "rm")) {
        let force = false;
        let recursive = false;
        let idx: usize = 1;
        while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-') {
            const flag = argv[idx];
            if (std.mem.eql(u8, flag, "--")) {
                idx += 1;
                break;
            }
            let k: usize = 1;
            while (k < flag.len) {
                if (flag[k] == 'f') force = true;
                else if (flag[k] == 'r' || flag[k] == 'R') recursive = true;
                k += 1;
            }
            idx += 1;
        }
        if (idx >= argv.len) {
            if (force) return 0;
            stderr.push("rm: missing operand\n");
            return 1;
        }
        let code: i32 = 0;
        while (idx < argv.len) {
            if (!removePathRecursive(argv[idx], recursive, force)) {
                stderr.push("rm: cannot remove '");
                stderr.push(argv[idx]);
                stderr.push("'\n");
                code = 1;
            }
            idx += 1;
        }
        return code;
    }

    return 127;
}

pub fn runBuiltin(
    argv: []const []const u8,
    stdin: []const u8,
    stdout: *StringJoiner,
    stderr: *StringJoiner,
): i32 {
    let env = ShellEnv.init();
    defer env.release();
    let dirs = DirStack.init();
    let should_exit = false;
    return runBuiltinWithState(argv, stdin, stdout, stderr, &env, &dirs, &should_exit);
}
