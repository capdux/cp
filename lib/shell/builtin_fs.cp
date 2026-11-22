// Source: Absorbed from /data/dev/bun/src/shell/builtin/ls.cp | Synced: 2026-11-22T07:05:10Z
// ============================================================================
// lib/shell/builtin_fs.cp
// Filesystem Built-in Commands for CP Shell (cap:shell)
// Implements: ls (-a/-A/-l/-1/-d/-r), cat (-n), mkdir (-p), rmdir (-p),
// rm (-r/-R/-f/-v), cp (-r/-R/-f/-v), mv (-f/-n/-v), touch (-c),
// basename (-a/-s), dirname, mktemp (-d/-p).
// Built on cap:sys (DirIterator, mkdirAll, rmdirRecursive, copyFile, statMaybe,
// lstatMaybe, rename, File, makeTempFile, makeTempDir) & cap:path.

const std = @import("std");
import { StringJoiner, PathString } from "cap:string";
import {
    FD,
    DirEntry,
    DirIterator,
    DirEntryKind,
    Stat,
    statMaybe,
    lstatMaybe,
    mkdir,
    mkdirAll,
    rmdir,
    rmdirRecursive,
    unlink,
    rename,
    copyFile,
    File,
    O_CREAT,
    O_WRONLY,
    O_CLOEXEC,
    makeTempFile,
    makeTempDir,
    getTempDir,
} from "cap:sys";
import { ShellEnv, DirStack, pushShellUsize } from "./env.cp";

pub fn readFileInto(path: []const u8, out: *StringJoiner): bool {
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

pub fn statPathInfo(path: []const u8, follow_links: bool): { exists: bool, is_file: bool, is_dir: bool, is_link: bool, size: u64 } {
    const res = if (follow_links) statMaybe(path) else lstatMaybe(path);
    if (res.isOk()) {
        const st = res.result;
        return {
            exists: true,
            is_file: st.isFile(),
            is_dir: st.isDir(),
            is_link: st.isSymLink(),
            size: st.size,
        };
    }
    return { exists: false, is_file: false, is_dir: false, is_link: false, size: 0 };
}

fn pushU64Decimal(out: *StringJoiner, v: u64) {
    let tmp: [32]u8 = [0; 32];
    const s = std.fmt.bufPrint(tmp[0..32], "{d}", .{v}) catch "0";
    out.push(s);
}

fn formatModeBits(mode: u32, out: *[10]u8) {
    const kind = mode & 0o170000;
    out.*[0] = if (kind == 0o040000) 'd' else if (kind == 0o120000) 'l' else '-';
    out.*[1] = if ((mode & 0o400) != 0) 'r' else '-';
    out.*[2] = if ((mode & 0o200) != 0) 'w' else '-';
    out.*[3] = if ((mode & 0o100) != 0) 'x' else '-';
    out.*[4] = if ((mode & 0o040) != 0) 'r' else '-';
    out.*[5] = if ((mode & 0o020) != 0) 'w' else '-';
    out.*[6] = if ((mode & 0o010) != 0) 'x' else '-';
    out.*[7] = if ((mode & 0o004) != 0) 'r' else '-';
    out.*[8] = if ((mode & 0o002) != 0) 'w' else '-';
    out.*[9] = if ((mode & 0o001) != 0) 'x' else '-';
}

fn joinChildPath(dir: []const u8, name: []const u8, buf: []u8): ([]const u8)? {
    if (dir.len == 0 || std.mem.eql(u8, dir, ".")) {
        if (name.len >= buf.len) return null;
        @memcpy(buf[0..name.len], name);
        return buf[0..name.len];
    }
    const need_slash = dir[dir.len - 1] != '/';
    const extra: usize = if (need_slash) 1 else 0;
    if (dir.len + extra + name.len >= buf.len) return null;
    @memcpy(buf[0..dir.len], dir);
    if (need_slash) buf[dir.len] = '/';
    @memcpy(buf[dir.len + extra .. dir.len + extra + name.len], name);
    return buf[0 .. dir.len + extra + name.len];
}

fn copyDirRecursive(src_dir: []const u8, dst_dir: []const u8): bool {
    mkdirAll(dst_dir, 0o755) catch return false;
    let iter = DirIterator.open(src_dir) catch return false;
    defer iter.close();

    while (entry := (iter.next() catch return false)) {
        let src_buf: [2048]u8 = undefined;
        let dst_buf: [2048]u8 = undefined;
        const src_child = joinChildPath(src_dir, entry.name, &src_buf) ?? return false;
        const dst_child = joinChildPath(dst_dir, entry.name, &dst_buf) ?? return false;
        if (entry.kind == .directory) {
            if (!copyDirRecursive(src_child, dst_child)) return false;
        } else {
            copyFile(src_child, dst_child) catch return false;
        }
    }
    return true;
}

fn removePathSafe(path: []const u8, recursive: bool, force: bool): bool {
    const info = statPathInfo(path, false);
    if (!info.exists) {
        return force;
    }
    if (!info.is_dir) {
        unlink(path) catch return false;
        return true;
    }
    if (!recursive) {
        return false;
    }
    rmdirRecursive(path) catch return false;
    return true;
}

fn sortStringSlice(items: [][]const u8, reverse: bool) {
    if (items.len <= 1) return;
    let i: usize = 1;
    while (i < items.len) {
        const key = items[i];
        let j = i;
        while (j > 0) {
            const ord = std.mem.order(u8, items[j - 1], key);
            const swap = if (reverse) (ord == .lt) else (ord == .gt);
            if (!swap) break;
            items[j] = items[j - 1];
            j -= 1;
        }
        items[j] = key;
        i += 1;
    }
}

pub fn runBuiltinLs(argv: []const []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let show_all = false;
    let show_almost = false;
    let long_fmt = false;
    let one_per_line = false;
    let dir_only = false;
    let reverse = false;

    let idx: usize = 1;
    while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-') {
        const f = argv[idx];
        if (std.mem.eql(u8, f, "--")) {
            idx += 1;
            break;
        }
        let k: usize = 1;
        while (k < f.len) {
            const c = f[k];
            if (c == 'a') show_all = true;
            else if (c == 'A') show_almost = true;
            else if (c == 'l') { long_fmt = true; one_per_line = true; }
            else if (c == '1') one_per_line = true;
            else if (c == 'd') dir_only = true;
            else if (c == 'r') reverse = true;
            else {
                stderr.push("ls: invalid option -- '");
                stderr.push(f[k .. k + 1]);
                stderr.push("'\n");
                return 2;
            }
            k += 1;
        }
        idx += 1;
    }

    let targets_buf: [64][]const u8 = undefined;
    let targets_len: usize = 0;
    if (idx >= argv.len) {
        targets_buf[0] = ".";
        targets_len = 1;
    } else {
        while (idx < argv.len && targets_len < 64) {
            targets_buf[targets_len] = argv[idx];
            targets_len += 1;
            idx += 1;
        }
    }

    const alloc = std.heap.page_allocator;
    let exit_code: i32 = 0;
    let ti: usize = 0;
    while (ti < targets_len) {
        const target = targets_buf[ti];
        const st_res = lstatMaybe(target);
        if (st_res.isErr()) {
            stderr.push("ls: cannot access '");
            stderr.push(target);
            stderr.push("': No such file or directory\n");
            exit_code = 2;
            ti += 1;
            continue;
        }
        if (st_res.isOk()) {
            const st = st_res.result;
            if (!st.isDir() || dir_only) {
                if (long_fmt) {
                    let mode_str: [10]u8 = undefined;
                    formatModeBits(st.mode, &mode_str);
                    stdout.push(mode_str[0..10]);
                    stdout.push(" ");
                    pushU64Decimal(stdout, st.size);
                    stdout.push(" ");
                }
                stdout.push(target);
                stdout.push("\n");
                ti += 1;
                continue;
            }
        } else {
            stderr.push("ls: cannot access '");
            stderr.push(target);
            stderr.push("': No such file or directory\n");
            exit_code = 2;
            ti += 1;
            continue;
        }

        if (targets_len > 1) {
            if (ti > 0) stdout.push("\n");
            stdout.push(target);
            stdout.push(":\n");
        }

        let iter = DirIterator.open(target) catch {
            stderr.push("ls: cannot open directory '");
            stderr.push(target);
            stderr.push("'\n");
            exit_code = 2;
            ti += 1;
            continue;
        };
        defer iter.close();

        let names: std.ArrayListUnmanaged(string) = .empty;
        defer {
            for (nm of names.items) alloc.free(nm);
            names.deinit(alloc);
        }

        if (show_all) {
            names.append(alloc, alloc.dupe(u8, ".") catch "") catch {};
            names.append(alloc, alloc.dupe(u8, "..") catch "") catch {};
        }

        while (entry := (iter.next() catch null)) {
            if (!show_all && !show_almost && entry.name.len > 0 && entry.name[0] == '.') {
                continue;
            }
            const dup = alloc.dupe(u8, entry.name) catch continue;
            names.append(alloc, dup) catch {};
        }

        sortStringSlice(names.items, reverse);

        let ni: usize = 0;
        while (ni < names.items.len) {
            const name = names.items[ni];
            if (long_fmt) {
                let full_buf: [2048]u8 = undefined;
                const full = joinChildPath(target, name, &full_buf) ?? name;
                const est = lstatMaybe(full).unwrapOr(Stat{});
                let mode_str: [10]u8 = undefined;
                formatModeBits(est.mode, &mode_str);
                stdout.push(mode_str[0..10]);
                stdout.push(" ");
                pushU64Decimal(stdout, est.size);
                stdout.push(" ");
                stdout.push(name);
                stdout.push("\n");
            } else {
                stdout.push(name);
                if (one_per_line || ni + 1 == names.items.len) {
                    stdout.push("\n");
                } else {
                    stdout.push("  ");
                }
            }
            ni += 1;
        }
        ti += 1;
    }

    return exit_code;
}

pub fn runBuiltinCat(argv: []const []const u8, stdin: []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let number_lines = false;
    let idx: usize = 1;
    while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-' && !std.mem.eql(u8, argv[idx], "-")) {
        if (std.mem.eql(u8, argv[idx], "--")) {
            idx += 1;
            break;
        }
        if (std.mem.eql(u8, argv[idx], "-n")) {
            number_lines = true;
        }
        idx += 1;
    }

    let combined = StringJoiner.init();
    defer combined.~init();

    if (idx >= argv.len) {
        combined.push(stdin);
    } else {
        while (idx < argv.len) {
            const fpath = argv[idx];
            if (std.mem.eql(u8, fpath, "-")) {
                combined.push(stdin);
            } else if (!readFileInto(fpath, &combined)) {
                stderr.push("cat: ");
                stderr.push(fpath);
                stderr.push(": No such file or directory\n");
                return 1;
            }
            idx += 1;
        }
    }

    const data = combined.slice();
    if (!number_lines) {
        stdout.push(data);
        return 0;
    }

    let line_no: usize = 1;
    let pos: usize = 0;
    while (pos < data.len) {
        let end = pos;
        while (end < data.len && data[end] != '\n') {
            end += 1;
        }
        if (end < data.len && data[end] == '\n') end += 1;
        pushShellUsize(stdout, line_no);
        stdout.push("\t");
        stdout.push(data[pos..end]);
        line_no += 1;
        pos = end;
    }
    return 0;
}

pub fn runBuiltinMkdir(argv: []const []const u8, stderr: *StringJoiner): i32 {
    let parents = false;
    let idx: usize = 1;
    while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-') {
        if (std.mem.eql(u8, argv[idx], "--")) { idx += 1; break; }
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
        let ok = true;
        if (parents) {
            mkdirAll(argv[idx], 0o755) catch { ok = false; };
        } else {
            mkdir(argv[idx], 0o755) catch { ok = false; };
        }
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

pub fn runBuiltinRmdir(argv: []const []const u8, stderr: *StringJoiner): i32 {
    if (argv.len < 2) {
        stderr.push("rmdir: missing operand\n");
        return 1;
    }
    let code: i32 = 0;
    let idx: usize = 1;
    while (idx < argv.len) {
        let ok = true;
        rmdir(argv[idx]) catch { ok = false; };
        if (!ok) {
            stderr.push("rmdir: failed to remove '");
            stderr.push(argv[idx]);
            stderr.push("'\n");
            code = 1;
        }
        idx += 1;
    }
    return code;
}

pub fn runBuiltinRm(argv: []const []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let force = false;
    let recursive = false;
    let verbose = false;
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
            else if (flag[k] == 'v') verbose = true;
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
        if (!removePathSafe(argv[idx], recursive, force)) {
            stderr.push("rm: cannot remove '");
            stderr.push(argv[idx]);
            stderr.push("'\n");
            code = 1;
        } else if (verbose) {
            stdout.push("removed '");
            stdout.push(argv[idx]);
            stdout.push("'\n");
        }
        idx += 1;
    }
    return code;
}

pub fn runBuiltinCp(argv: []const []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let recursive = false;
    let verbose = false;
    let idx: usize = 1;
    while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-') {
        const flag = argv[idx];
        if (std.mem.eql(u8, flag, "--")) { idx += 1; break; }
        let k: usize = 1;
        while (k < flag.len) {
            if (flag[k] == 'r' || flag[k] == 'R') recursive = true;
            else if (flag[k] == 'v') verbose = true;
            k += 1;
        }
        idx += 1;
    }

    if (argv.len - idx < 2) {
        stderr.push("cp: missing file operand\n");
        return 1;
    }

    const dest = argv[argv.len - 1];
    const src_count = (argv.len - 1) - idx;
    const dest_info = statPathInfo(dest, true);

    if (src_count > 1 && !dest_info.is_dir) {
        stderr.push("cp: target '");
        stderr.push(dest);
        stderr.push("' is not a directory\n");
        return 1;
    }

    let code: i32 = 0;
    let si = idx;
    while (si < argv.len - 1) {
        const src = argv[si];
        const src_info = statPathInfo(src, true);
        if (!src_info.exists) {
            stderr.push("cp: cannot stat '");
            stderr.push(src);
            stderr.push("': No such file or directory\n");
            code = 1;
            si += 1;
            continue;
        }

        let target_buf: [2048]u8 = undefined;
        let final_dest = dest;
        if (dest_info.is_dir) {
            const base = PathString.init(src).basename();
            if (jp := joinChildPath(dest, base, &target_buf)) {
                final_dest = jp;
            }
        }

        if (src_info.is_dir) {
            if (!recursive) {
                stderr.push("cp: -r not specified; omitting directory '");
                stderr.push(src);
                stderr.push("'\n");
                code = 1;
            } else if (!copyDirRecursive(src, final_dest)) {
                stderr.push("cp: failed to copy directory '");
                stderr.push(src);
                stderr.push("'\n");
                code = 1;
            } else if (verbose) {
                stdout.push("'"); stdout.push(src); stdout.push("' -> '"); stdout.push(final_dest); stdout.push("'\n");
            }
        } else {
            let ok = true;
            copyFile(src, final_dest) catch { ok = false; };
            if (!ok) {
                stderr.push("cp: cannot copy '");
                stderr.push(src);
                stderr.push("'\n");
                code = 1;
            } else if (verbose) {
                stdout.push("'"); stdout.push(src); stdout.push("' -> '"); stdout.push(final_dest); stdout.push("'\n");
            }
        }
        si += 1;
    }
    return code;
}

pub fn runBuiltinMv(argv: []const []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let no_clobber = false;
    let verbose = false;
    let idx: usize = 1;
    while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-') {
        const flag = argv[idx];
        if (std.mem.eql(u8, flag, "--")) { idx += 1; break; }
        let k: usize = 1;
        while (k < flag.len) {
            if (flag[k] == 'n') no_clobber = true;
            else if (flag[k] == 'f') no_clobber = false;
            else if (flag[k] == 'v') verbose = true;
            k += 1;
        }
        idx += 1;
    }

    if (argv.len - idx < 2) {
        stderr.push("mv: missing file operand\n");
        return 1;
    }

    const dest = argv[argv.len - 1];
    const src_count = (argv.len - 1) - idx;
    const dest_info = statPathInfo(dest, true);

    if (src_count > 1 && !dest_info.is_dir) {
        stderr.push("mv: target '");
        stderr.push(dest);
        stderr.push("' is not a directory\n");
        return 1;
    }

    let code: i32 = 0;
    let si = idx;
    while (si < argv.len - 1) {
        const src = argv[si];
        let target_buf: [2048]u8 = undefined;
        let final_dest = dest;
        if (dest_info.is_dir) {
            const base = PathString.init(src).basename();
            if (jp := joinChildPath(dest, base, &target_buf)) {
                final_dest = jp;
            }
        }
        if (no_clobber && statPathInfo(final_dest, false).exists) {
            si += 1;
            continue;
        }

        let moved = true;
        rename(src, final_dest) catch { moved = false; };
        if (!moved) {
            // Cross-device fallback: copy + remove
            const sinfo = statPathInfo(src, false);
            if (sinfo.is_dir) {
                moved = copyDirRecursive(src, final_dest) && removePathSafe(src, true, true);
            } else {
                let c_ok = true;
                copyFile(src, final_dest) catch { c_ok = false; };
                if (c_ok) {
                    moved = removePathSafe(src, false, true);
                }
            }
        }
        if (!moved) {
            stderr.push("mv: cannot move '");
            stderr.push(src);
            stderr.push("'\n");
            code = 1;
        } else if (verbose) {
            stdout.push("renamed '"); stdout.push(src); stdout.push("' -> '"); stdout.push(final_dest); stdout.push("'\n");
        }
        si += 1;
    }
    return code;
}

pub fn runBuiltinTouch(argv: []const []const u8, stderr: *StringJoiner): i32 {
    let no_create = false;
    let idx: usize = 1;
    while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-') {
        if (std.mem.eql(u8, argv[idx], "--")) { idx += 1; break; }
        if (std.mem.eql(u8, argv[idx], "-c") || std.mem.eql(u8, argv[idx], "--no-create")) {
            no_create = true;
        }
        idx += 1;
    }
    if (idx >= argv.len) {
        stderr.push("touch: missing file operand\n");
        return 1;
    }
    let code: i32 = 0;
    while (idx < argv.len) {
        const path = argv[idx];
        const info = statPathInfo(path, true);
        if (!info.exists) {
            if (!no_create) {
                let f = File.open(path, O_CREAT | O_WRONLY | O_CLOEXEC, 0o644) catch {
                    stderr.push("touch: cannot touch '");
                    stderr.push(path);
                    stderr.push("'\n");
                    code = 1;
                    idx += 1;
                    continue;
                };
                f.close();
            }
        } else {
            let pbuf: [1024]u8 = [0; 1024];
            if (path.len < pbuf.len) {
                @memcpy(pbuf[0..path.len], path);
                pbuf[path.len] = 0;
                const c_path: [*:0]const u8 = pbuf[0..path.len :0].ptr;
                zig {
                    const UTIME_NOW: isize = (1 << 30) - 1;
                    const times: [2]std.os.linux.timespec = .{
                        .{ .sec = 0, .nsec = UTIME_NOW },
                        .{ .sec = 0, .nsec = UTIME_NOW },
                    };
                    _ = std.os.linux.utimensat(std.os.linux.AT.FDCWD, c_path, &times, 0);
                }
            }
        }
        idx += 1;
    }
    return code;
}

pub fn runBuiltinBasename(argv: []const []const u8, stdout: *StringJoiner): i32 {
    if (argv.len < 2) {
        stdout.push("\n");
        return 0;
    }
    let multiple = false;
    let suffix_opt: ([]const u8)? = null;
    let idx: usize = 1;
    while (idx < argv.len && argv[idx].len >= 2 && argv[idx][0] == '-') {
        if (std.mem.eql(u8, argv[idx], "-a") || std.mem.eql(u8, argv[idx], "--multiple")) {
            multiple = true;
            idx += 1;
        } else if (std.mem.eql(u8, argv[idx], "-s") && idx + 1 < argv.len) {
            multiple = true;
            suffix_opt = argv[idx + 1];
            idx += 2;
        } else {
            break;
        }
    }
    if (idx >= argv.len) {
        stdout.push("\n");
        return 0;
    }
    if (multiple) {
        while (idx < argv.len) {
            let base = PathString.init(argv[idx]).basename();
            if (sfx := suffix_opt) {
                if (sfx.len > 0 && base.len > sfx.len && std.mem.endsWith(u8, base, sfx)) {
                    base = base[0 .. base.len - sfx.len];
                }
            }
            stdout.push(base);
            stdout.push("\n");
            idx += 1;
        }
        return 0;
    }
    let base = PathString.init(argv[idx]).basename();
    if (idx + 1 < argv.len) {
        const suffix = argv[idx + 1];
        if (suffix.len > 0 && base.len > suffix.len && std.mem.endsWith(u8, base, suffix)) {
            base = base[0 .. base.len - suffix.len];
        }
    }
    stdout.push(base);
    stdout.push("\n");
    return 0;
}

pub fn runBuiltinDirname(argv: []const []const u8, stdout: *StringJoiner): i32 {
    if (argv.len < 2) {
        stdout.push(".\n");
        return 0;
    }
    let idx: usize = 1;
    while (idx < argv.len) {
        const p = PathString.init(argv[idx]);
        stdout.push(p.dirname());
        stdout.push("\n");
        idx += 1;
    }
    return 0;
}

pub fn runBuiltinMktemp(argv: []const []const u8, stdout: *StringJoiner, stderr: *StringJoiner): i32 {
    let make_dir = false;
    let prefix: []const u8 = "tmp";
    let idx: usize = 1;
    while (idx < argv.len) {
        const a = argv[idx];
        if (std.mem.eql(u8, a, "-d") || std.mem.eql(u8, a, "--directory")) {
            make_dir = true;
        } else if (std.mem.eql(u8, a, "-p") && idx + 1 < argv.len) {
            idx += 1;
        } else if (a.len > 0 && a[0] != '-') {
            // Strip trailing X's if template like foo.XXXXXX
            let end = a.len;
            while (end > 0 && a[end - 1] == 'X') end -= 1;
            if (end > 0) {
                prefix = PathString.init(a[0..end]).basename();
            }
        }
        idx += 1;
    }

    let path_buf: [512]u8 = [0; 512];
    if (make_dir) {
        const created = makeTempDir(prefix, &path_buf) catch {
            stderr.push("mktemp: failed to create directory\n");
            return 1;
        };
        stdout.push(created);
        stdout.push("\n");
        return 0;
    }
    let f = makeTempFile(prefix, "", &path_buf) catch {
        stderr.push("mktemp: failed to create file\n");
        return 1;
    };
    let len: usize = 0;
    while (len < path_buf.len && path_buf[len] != 0) len += 1;
    f.close();
    stdout.push(path_buf[0..len]);
    stdout.push("\n");
    return 0;
}
