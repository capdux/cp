// Source: /data/dev/bun/src/shell/
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/builtin.cp
// Built-in command implementations for CP Shell (cap:shell)
// Absorbed from Bun Shell builtins with pure CP semantics

import { StringJoiner } from "../string/joiner.cp";
import { PathString } from "../string/path.cp";

pub fn isBuiltin(name: []const u8): bool {
    let is_match: bool = false;
    zig {
        if (std.mem.eql(u8, name, "echo") or
            std.mem.eql(u8, name, "pwd") or
            std.mem.eql(u8, name, "cd") or
            std.mem.eql(u8, name, "which") or
            std.mem.eql(u8, name, "export") or
            std.mem.eql(u8, name, "true") or
            std.mem.eql(u8, name, "false") or
            std.mem.eql(u8, name, "exit") or
            std.mem.eql(u8, name, "dirname") or
            std.mem.eql(u8, name, "basename") or
            std.mem.eql(u8, name, "cat"))
        {
            is_match = true;
        }
    }
    return is_match;
}

pub fn runBuiltin(
    argv: []const []const u8,
    stdin: []const u8,
    stdout: *StringJoiner,
    stderr: *StringJoiner,
): i32 {
    if (argv.len == 0) {
        return 0;
    }
    const name = argv[0];

    // echo [-n] [args...]
    let is_echo: bool = false;
    zig { is_echo = std.mem.eql(u8, name, "echo"); }
    if (is_echo) {
        let no_newline: bool = false;
        let start_idx: usize = 1;
        if (argv.len > 1) {
            let is_dash_n: bool = false;
            zig { is_dash_n = std.mem.eql(u8, argv[1], "-n"); }
            if (is_dash_n) {
                no_newline = true;
                start_idx = 2;
            }
        }
        let i: usize = start_idx;
        while (i < argv.len) {
            stdout.push(argv[i]);
            if (i + 1 < argv.len) {
                stdout.push(" ");
            }
            i += 1;
        }
        if (!no_newline) {
            stdout.push("\n");
        }
        return 0;
    }

    // pwd
    let is_pwd: bool = false;
    zig { is_pwd = std.mem.eql(u8, name, "pwd"); }
    if (is_pwd) {
        zig {
            var buf: [1024]u8 = undefined;
            if (std.c.getcwd(&buf, buf.len)) |cwd_ptr| {
                const slice = std.mem.sliceTo(@as([*:0]u8, @ptrCast(cwd_ptr)), 0);
                stdout.push(slice);
                stdout.push("\n");
            } else {
                stdout.push("/\n");
            }
        }
        return 0;
    }

    // cd [dir]
    let is_cd: bool = false;
    zig { is_cd = std.mem.eql(u8, name, "cd"); }
    if (is_cd) {
        const target_dir = if (argv.len > 1) argv[1] else "/";
        let success: bool = true;
        zig {
            const chdir_fn = struct { extern "c" fn chdir(path: [*c]const u8) c_int; }.chdir;
            var dir_buf: [1024]u8 = undefined;
            if (target_dir.len < dir_buf.len) {
                @memcpy(dir_buf[0..target_dir.len], target_dir);
                dir_buf[target_dir.len] = 0;
                if (chdir_fn(dir_buf[0..target_dir.len :0].ptr) != 0) {
                    success = false;
                }
            } else {
                success = false;
            }
        }
        if (!success) {
            stderr.push("cd: failed to change directory to ");
            stderr.push(target_dir);
            stderr.push("\n");
            return 1;
        }
        return 0;
    }

    // which [cmd]
    let is_which: bool = false;
    zig { is_which = std.mem.eql(u8, name, "which"); }
    if (is_which) {
        if (argv.len < 2) {
            stderr.push("which: missing argument\n");
            return 1;
        }
        const cmd_to_find = argv[1];
        if (isBuiltin(cmd_to_find)) {
            stdout.push(cmd_to_find);
            stdout.push(": shell built-in command\n");
            return 0;
        }
        stdout.push(cmd_to_find);
        stdout.push("\n");
        return 0;
    }

    // export KEY=VAL
    let is_export: bool = false;
    zig { is_export = std.mem.eql(u8, name, "export"); }
    if (is_export) {
        return 0;
    }

    // true
    let is_true: bool = false;
    zig { is_true = std.mem.eql(u8, name, "true"); }
    if (is_true) {
        return 0;
    }

    // false
    let is_false: bool = false;
    zig { is_false = std.mem.eql(u8, name, "false"); }
    if (is_false) {
        return 1;
    }

    // exit [code]
    let is_exit: bool = false;
    zig { is_exit = std.mem.eql(u8, name, "exit"); }
    if (is_exit) {
        if (argv.len > 1) {
            return 0; // exit code parseable
        }
        return 0;
    }

    // dirname [path]
    let is_dirname: bool = false;
    zig { is_dirname = std.mem.eql(u8, name, "dirname"); }
    if (is_dirname) {
        if (argv.len < 2) {
            stdout.push(".\n");
            return 0;
        }
        const p = PathString.init(argv[1]);
        stdout.push(p.dirname());
        stdout.push("\n");
        return 0;
    }

    // basename [path]
    let is_basename: bool = false;
    zig { is_basename = std.mem.eql(u8, name, "basename"); }
    if (is_basename) {
        if (argv.len < 2) {
            stdout.push("\n");
            return 0;
        }
        const p = PathString.init(argv[1]);
        stdout.push(p.basename());
        stdout.push("\n");
        return 0;
    }

    // cat [files...] (or pass stdin to stdout when no files specified)
    let is_cat: bool = false;
    zig { is_cat = std.mem.eql(u8, name, "cat"); }
    if (is_cat) {
        if (argv.len < 2) {
            stdout.push(stdin);
            return 0;
        }
        let f_idx: usize = 1;
        while (f_idx < argv.len) {
            const fpath = argv[f_idx];
            let read_ok: bool = true;
            zig {
                const fd = std.posix.openat(std.posix.AT.FDCWD, fpath, .{ .ACCMODE = .RDONLY }, 0) catch null;
                if (fd) |valid_fd| {
                    defer _ = std.c.close(valid_fd);
                    var fbuf: [4096]u8 = undefined;
                    while (true) {
                        const n = std.posix.read(valid_fd, &fbuf) catch 0;
                        if (n == 0) break;
                        stdout.push(fbuf[0..n]);
                    }
                } else {
                    read_ok = false;
                }
            }
            if (!read_ok) {
                stderr.push("cat: ");
                stderr.push(fpath);
                stderr.push(": No such file or directory\n");
                return 1;
            }
            f_idx += 1;
        }
        return 0;
    }

    return 127;
}
