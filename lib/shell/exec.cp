// Source: /data/dev/bun/src/shell/interpreter.cp
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/interpreter.cp & subproc.cp
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/exec.cp
// Pipeline execution, variable expansion, redirections & session engine for CP Shell (cap:shell)

const std = @import("std");
import { StringJoiner } from "../string/joiner.cp";
import { Command, Pipeline, Script, Statement, ControlOp, Parser, MAX_COMMAND_ARGS } from "./parser.cp";
import { isBuiltin, runBuiltinWithState } from "./builtin.cp";
import { ShellEnv, DirStack, expandWord } from "./env.cp";

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
    if (path.len == 0 || path.len >= 1024) {
        return false;
    }
    let path_buf: [1024]u8 = [0; 1024];
    @memcpy(path_buf[0..path.len], path);
    path_buf[path.len] = 0;

    let ok = false;
    zig {
        const c_path: [*:0]const u8 = path_buf[0..path.len :0].ptr;
        const z_flags = std.c.O{
            .ACCMODE = .WRONLY,
            .CREAT = true,
            .TRUNC = !append,
            .APPEND = append,
            .CLOEXEC = true,
        };
        const fd = std.c.open(c_path, z_flags, @as(std.c.mode_t, 0o644));
        if (fd >= 0) {
            defer _ = std.c.close(fd);
            var written: usize = 0;
            ok = true;
            while (written < content.len) {
                const n = std.c.write(fd, content[written..].ptr, content.len - written);
                if (n <= 0) {
                    ok = false;
                    break;
                }
                written += @intCast(n);
            }
        }
    }
    return ok;
}

fn readRedirectFile(path: []const u8, out: *StringJoiner): bool {
    if (path.len == 0 || path.len >= 1024) {
        return false;
    }
    let path_buf: [1024]u8 = [0; 1024];
    @memcpy(path_buf[0..path.len], path);
    path_buf[path.len] = 0;

    let ok = false;
    zig {
        const c_path: [*:0]const u8 = path_buf[0..path.len :0].ptr;
        const z_flags = std.c.O{ .CLOEXEC = true };
        const fd = std.c.open(c_path, z_flags, @as(std.c.mode_t, 0));
        if (fd >= 0) {
            defer _ = std.c.close(fd);
            var buf: [4096]u8 = undefined;
            while (true) {
                const n = std.c.read(fd, &buf, buf.len);
                if (n <= 0) break;
                const cnt: usize = @intCast(n);
                out.push(buf[0..cnt]);
            }
            ok = true;
        }
    }
    return ok;
}

fn shellQuoteArg(arg: []const u8, out: *StringJoiner) {
    out.push("'");
    let i: usize = 0;
    while (i < arg.len) {
        if (arg[i] == '\'') {
            out.push("'\\''");
        } else {
            out.push(arg[i .. i + 1]);
        }
        i += 1;
    }
    out.push("'");
}

pub struct ShellSession {
    pub env: ShellEnv,
    pub dirs: DirStack,
    pub last_exit_code: i32 = 0,
    pub should_exit: bool = false,

    pub static fn init(): ShellSession {
        return ShellSession {
            env: ShellEnv.init(),
            dirs: DirStack.init(),
            last_exit_code: 0,
            should_exit: false,
        };
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
        const script = parser.parseScript();
        return Executor.executeScriptWithSession(script, _);
    }

    pub fn runWithMemoryTarget(*_, command_str: []const u8, out_buf: *StringJoiner, err_buf: *StringJoiner): i32 {
        const res = _.run(command_str);
        out_buf.push(res.stdout_str);
        err_buf.push(res.stderr_str);
        return res.exit_code;
    }

    pub fn release(*_) {
        _.env.release();
    }
}

pub struct Executor {
    pub static fn executeCommandWithSession(
        cmd: Command,
        stdin_data: []const u8,
        stdout_joiner: *StringJoiner,
        stderr_joiner: *StringJoiner,
        session: *ShellSession,
    ): i32 {
        // 1. Standalone variable assignments (FOO=bar BAZ=qux)
        if (cmd.args_len == 0 && cmd.assigns_len > 0) {
            let ai: usize = 0;
            while (ai < cmd.assigns_len) {
                let val_buf = StringJoiner.init();
                defer val_buf.release();
                expandWord(cmd.assigns[ai].raw_value, &session.env, session.last_exit_code, &val_buf);
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
            defer new_val.release();
            expandWord(cmd.assigns[ai].raw_value, &session.env, session.last_exit_code, &new_val);
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
                saved_bufs[ri].release();
                ri += 1;
            }
        }

        // 3. Expand command arguments
        let expanded_joiners: [MAX_COMMAND_ARGS]StringJoiner = undefined;
        let expanded_argv: [MAX_COMMAND_ARGS][]const u8 = undefined;
        let i: usize = 0;
        while (i < cmd.args_len) {
            expanded_joiners[i] = StringJoiner.init();
            if (cmd.has_raw_args[i]) {
                expandWord(cmd.raw_args[i], &session.env, session.last_exit_code, &expanded_joiners[i]);
                expanded_argv[i] = expanded_joiners[i].slice();
            } else {
                expanded_argv[i] = cmd.args[i];
            }
            i += 1;
        }
        defer {
            let ci: usize = 0;
            while (ci < cmd.args_len) {
                expanded_joiners[ci].release();
                ci += 1;
            }
        }

        const argv = expanded_argv[0..cmd.args_len];

        // 4. Input redirection (< file)
        let in_buf = StringJoiner.init();
        defer in_buf.release();
        let effective_stdin = stdin_data;
        if (cmd.redirect_in.len > 0) {
            let redir_in_path = StringJoiner.init();
            defer redir_in_path.release();
            expandWord(cmd.redirect_in, &session.env, session.last_exit_code, &redir_in_path);
            if (!readRedirectFile(redir_in_path.slice(), &in_buf)) {
                stderr_joiner.push("shell: ");
                stderr_joiner.push(redir_in_path.slice());
                stderr_joiner.push(": No such file or directory\n");
                return 1;
            }
            effective_stdin = in_buf.slice();
        }

        let cmd_out = StringJoiner.init();
        defer cmd_out.release();
        let cmd_err = StringJoiner.init();
        defer cmd_err.release();

        let exit_code: i32 = 0;
        if (argv.len > 0) {
            const bin_name = argv[0];
            if (isBuiltin(bin_name)) {
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
                // External command via popen
                let cmd_line = StringJoiner.init();
                defer cmd_line.release();
                let k: usize = 0;
                while (k < argv.len) {
                    if (k > 0) cmd_line.push(" ");
                    shellQuoteArg(argv[k], &cmd_line);
                    k += 1;
                }
                const cmd_str = cmd_line.slice();
                zig {
                    const allocator = std.heap.page_allocator;
                    const cmd_buf = allocator.alloc(u8, cmd_str.len + 1) catch return 1;
                    defer allocator.free(cmd_buf);
                    @memcpy(cmd_buf[0..cmd_str.len], cmd_str);
                    cmd_buf[cmd_str.len] = 0;

                    const c_libc = struct {
                        extern "c" fn popen(command: [*c]const u8, modes: [*c]const u8) ?*anyopaque;
                        extern "c" fn pclose(stream: ?*anyopaque) c_int;
                        extern "c" fn fread(ptr: [*]u8, size: usize, nmemb: usize, stream: ?*anyopaque) usize;
                    };

                    const c_cmd: [*:0]const u8 = @ptrCast(cmd_buf[0..cmd_str.len :0]);
                    if (c_libc.popen(c_cmd, "r")) |pipe_f| {
                        defer {
                            const status = c_libc.pclose(pipe_f);
                            exit_code = @intCast((status >> 8) & 0xFF);
                        }
                        var out_buf: [4096]u8 = undefined;
                        while (true) {
                            const n = c_libc.fread(&out_buf, 1, out_buf.len, pipe_f);
                            if (n == 0) break;
                            cmd_out.push(out_buf[0..n]);
                        }
                    } else {
                        cmd_err.push("shell: command execution failed\n");
                        exit_code = 127;
                    }
                }
            }
        }

        // 5. Handle stderr/stdout merging & redirections
        if (cmd.redirect_err_to_out) {
            cmd_out.push(cmd_err.slice());
            cmd_err.length = 0;
        } else if (cmd.redirect_out_to_err) {
            cmd_err.push(cmd_out.slice());
            cmd_out.length = 0;
        }

        if (cmd.redirect_all.len > 0) {
            let all_path = StringJoiner.init();
            defer all_path.release();
            expandWord(cmd.redirect_all, &session.env, session.last_exit_code, &all_path);
            let combined = StringJoiner.init();
            defer combined.release();
            combined.push(cmd_out.slice());
            combined.push(cmd_err.slice());
            if (!writeRedirectFile(all_path.slice(), combined.slice(), cmd.redirect_all_append)) {
                stderr_joiner.push("shell: failed to redirect output\n");
                return 1;
            }
            return exit_code;
        }

        if (cmd.redirect_out.len > 0) {
            let out_path = StringJoiner.init();
            defer out_path.release();
            expandWord(cmd.redirect_out, &session.env, session.last_exit_code, &out_path);
            if (!writeRedirectFile(out_path.slice(), cmd_out.slice(), cmd.redirect_append)) {
                stderr_joiner.push("shell: failed to redirect stdout\n");
                return 1;
            }
        } else {
            stdout_joiner.push(cmd_out.slice());
        }

        if (cmd.redirect_err.len > 0) {
            let err_path = StringJoiner.init();
            defer err_path.release();
            expandWord(cmd.redirect_err, &session.env, session.last_exit_code, &err_path);
            if (!writeRedirectFile(err_path.slice(), cmd_err.slice(), cmd.redirect_err_append)) {
                stderr_joiner.push("shell: failed to redirect stderr\n");
                return 1;
            }
        } else {
            stderr_joiner.push(cmd_err.slice());
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
        defer sess.release();
        return Executor.executeCommandWithSession(cmd, stdin_data, stdout_joiner, stderr_joiner, &sess);
    }

    pub static fn executePipelineWithSession(
        pipe: Pipeline,
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
        defer pipe_holder.release();
        let cur_stdin = initial_stdin;
        let last_code: i32 = 0;
        let final_stdout = StringJoiner.init();
        let final_stderr = StringJoiner.init();

        let i: usize = 0;
        while (i < cmds.len) {
            let step_stdout = StringJoiner.init();
            last_code = Executor.executeCommandWithSession(
                cmds[i],
                cur_stdin,
                &step_stdout,
                &final_stderr,
                session,
            );

            const step_slice = step_stdout.slice();
            if (i + 1 < cmds.len) {
                pipe_holder.length = 0;
                pipe_holder.push(step_slice);
                step_stdout.release();
                cur_stdin = pipe_holder.slice();
            } else {
                final_stdout.push(step_slice);
                step_stdout.release();
            }
            if (session.should_exit) {
                break;
            }
            i += 1;
        }

        session.last_exit_code = last_code;
        const out_str = final_stdout.slice();
        const err_str = final_stderr.slice();
        final_stdout.has_allocated = false;
        final_stderr.has_allocated = false;

        return ShellResult {
            exit_code: last_code,
            stdout_str: out_str,
            stderr_str: err_str,
        };
    }

    pub static fn executePipeline(pipe: Pipeline, initial_stdin: []const u8): ShellResult {
        let sess = ShellSession.init();
        defer sess.release();
        return Executor.executePipelineWithSession(pipe, initial_stdin, &sess);
    }

    pub static fn executeScriptWithSession(script: Script, session: *ShellSession): ShellResult {
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
                const res = Executor.executePipelineWithSession(stmt.pipeline, "", session);
                last_code = res.exit_code;
                accum_stdout.push(res.stdout_str);
                accum_stderr.push(res.stderr_str);
                if (session.should_exit) {
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

    pub static fn executeScript(script: Script): ShellResult {
        let sess = ShellSession.init();
        defer sess.release();
        return Executor.executeScriptWithSession(script, &sess);
    }
}
