// Source: /data/dev/bun/src/shell/
// ============================================================================
// Absorbed from Bun: /data/dev/bun/src/shell/
// Upstream Sync: Refer to /data/dev/bun/src/shell/ for future upstream updates.
// ============================================================================
// lib/shell/exec.cp
// Pipeline execution and process management for CP Shell (cap:shell)
// Absorbed from Bun Shell interpreter and subproc with pure CP semantics

import { StringJoiner } from "../string/joiner.cp";
import { Command, Pipeline, Script, Statement, ControlOp } from "./parser.cp";
import { isBuiltin, runBuiltin } from "./builtin.cp";

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

pub struct Executor {
    pub static fn executeCommand(
        cmd: Command,
        stdin_data: []const u8,
        stdout_joiner: *StringJoiner,
        stderr_joiner: *StringJoiner,
    ): i32 {
        const argv = cmd.args[0..cmd.args_len];
        if (argv.len == 0) {
            return 0;
        }

        const bin_name = argv[0];

        // 1. In-process Builtin dispatch
        if (isBuiltin(bin_name)) {
            let code = runBuiltin(argv, stdin_data, stdout_joiner, stderr_joiner);
            // Handle output file redirection
            if (cmd.redirect_out.len > 0) {
                const out_content = stdout_joiner.slice();
                zig {
                    const allocator = std.heap.page_allocator;
                    const path_z = allocator.alloc(u8, cmd.redirect_out.len + 1) catch unreachable;
                    defer allocator.free(path_z);
                    @memcpy(path_z[0..cmd.redirect_out.len], cmd.redirect_out);
                    path_z[cmd.redirect_out.len] = 0;

                    const c_path = path_z[0..cmd.redirect_out.len :0];
                    const mode: [*c]const u8 = if (cmd.redirect_append) "a" else "w";
                    if (std.c.fopen(c_path.ptr, mode)) |f| {
                        _ = std.c.fwrite(out_content.ptr, 1, out_content.len, f);
                        _ = std.c.fclose(f);
                    } else {
                        return 1;
                    }
                }
            }
            return code;
        }

        // 2. External binary execution via popen
        let exit_code: i32 = 0;
        zig {
            const allocator = std.heap.page_allocator;
            var total_cmd_len: usize = 0;
            for (argv) |arg| {
                total_cmd_len += arg.len + 1;
            }

            const cmd_buf = allocator.alloc(u8, total_cmd_len + 1) catch return 1;
            defer allocator.free(cmd_buf);

            var offset: usize = 0;
            for (argv) |arg| {
                @memcpy(cmd_buf[offset .. offset + arg.len], arg);
                offset += arg.len;
                cmd_buf[offset] = ' ';
                offset += 1;
            }
            if (offset > 0) offset -= 1;
            cmd_buf[offset] = 0;

            const c_libc = struct {
                extern "c" fn popen(command: [*c]const u8, modes: [*c]const u8) ?*anyopaque;
                extern "c" fn pclose(stream: ?*anyopaque) c_int;
                extern "c" fn fread(ptr: [*]u8, size: usize, nmemb: usize, stream: ?*anyopaque) usize;
            };

            const c_cmd: [*:0]const u8 = @ptrCast(cmd_buf[0..offset :0]);
            if (c_libc.popen(c_cmd, "r")) |pipe_f| {
                defer {
                    const status = c_libc.pclose(pipe_f);
                    exit_code = @intCast(status);
                }
                var out_buf: [4096]u8 = undefined;
                while (true) {
                    const n = c_libc.fread(&out_buf, 1, out_buf.len, pipe_f);
                    if (n == 0) break;
                    stdout_joiner.push(out_buf[0..n]);
                }
            } else {
                stderr_joiner.push("shell: command execution failed\n");
                exit_code = 127;
            }
        }

        // Handle output file redirection for external binaries
        if (cmd.redirect_out.len > 0) {
            const out_content = stdout_joiner.slice();
            zig {
                const redir_alloc = std.heap.page_allocator;
                const path_z = redir_alloc.alloc(u8, cmd.redirect_out.len + 1) catch unreachable;
                defer redir_alloc.free(path_z);
                @memcpy(path_z[0..cmd.redirect_out.len], cmd.redirect_out);
                path_z[cmd.redirect_out.len] = 0;

                const c_path = path_z[0..cmd.redirect_out.len :0];
                const mode: [*c]const u8 = if (cmd.redirect_append) "a" else "w";
                if (std.c.fopen(c_path.ptr, mode)) |f| {
                    _ = std.c.fwrite(out_content.ptr, 1, out_content.len, f);
                    _ = std.c.fclose(f);
                } else {
                    return 1;
                }
            }
        }

        return exit_code;
    }

    pub static fn executePipeline(pipe: Pipeline, initial_stdin: []const u8): ShellResult {
        const cmds = pipe.commands[0..pipe.commands_len];
        if (cmds.len == 0) {
            return ShellResult {
                exit_code: 0,
                stdout_str: "",
                stderr_str: "",
            };
        }

        let pipe_buf: [16384]u8 = undefined;
        let pipe_len: usize = 0;
        let cur_stdin = initial_stdin;
        let last_code: i32 = 0;
        let final_stdout = StringJoiner.init();
        let final_stderr = StringJoiner.init();

        let i: usize = 0;
        while (i < cmds.len) {
            let step_stdout = StringJoiner.init();
            zig {
                last_code = Executor.executeCommand(cmds[i], cur_stdin, &step_stdout, &final_stderr);
            }

            const step_slice = step_stdout.slice();
            if (i + 1 < cmds.len) {
                pipe_len = if (step_slice.len <= pipe_buf.len) step_slice.len else pipe_buf.len;
                if (pipe_len > 0) {
                    @memcpy(pipe_buf[0..pipe_len], step_slice[0..pipe_len]);
                }
                cur_stdin = pipe_buf[0..pipe_len];
            } else {
                final_stdout.push(step_slice);
            }
            i += 1;
        }

        const out_str = final_stdout.slice();
        const err_str = final_stderr.slice();
        // Transfer buffer ownership to ShellResult so RAII defer does not unmap returned slices
        final_stdout.has_allocated = false;
        final_stderr.has_allocated = false;

        return ShellResult {
            exit_code: last_code,
            stdout_str: out_str,
            stderr_str: err_str,
        };
    }

    pub static fn executeScript(script: Script): ShellResult {
        const stmts = script.statements[0..script.statements_len];
        if (stmts.len == 0) {
            return ShellResult {
                exit_code: 0,
                stdout_str: "",
                stderr_str: "",
            };
        }

        let last_result = ShellResult {
            exit_code: 0,
            stdout_str: "",
            stderr_str: "",
        };

        let i: usize = 0;
        while (i < stmts.len) {
            const stmt = stmts[i];
            last_result = Executor.executePipeline(stmt.pipeline, "");

            if (stmt.op == .And) {
                if (last_result.exit_code != 0) {
                    break;
                }
            } else if (stmt.op == .Or) {
                if (last_result.exit_code == 0) {
                    break;
                }
            }
            i += 1;
        }

        return last_result;
    }
}
