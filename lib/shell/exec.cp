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
        const argv = cmd.argv();
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
        const cmds = pipe.cmds();
        if (cmds.len == 0) {
            return ShellResult {
                exit_code: 0,
                stdout_str: "",
                stderr_str: "",
            };
        }

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

            if (i + 1 < cmds.len) {
                cur_stdin = step_stdout.slice();
            } else {
                final_stdout.push(step_stdout.slice());
            }
            i += 1;
        }

        return ShellResult {
            exit_code: last_code,
            stdout_str: final_stdout.slice(),
            stderr_str: final_stderr.slice(),
        };
    }

    pub static fn executeScript(script: Script): ShellResult {
        const stmts = script.stmts();
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
