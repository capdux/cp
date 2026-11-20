use super::super::*;
use crate::ast::*;
use crate::Span;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

impl Codegen {
    pub(crate) fn generate_expr_builtin(&mut self, expr: &Expr) -> String {
        match expr {
            Expr::Builtin(call) => match call.name.as_str() {
                "@hash" => {
                    if call.args.is_empty() {
                        "0".to_string()
                    } else if call.args.len() == 1 {
                        format!("cp_hash_val({})", self.generate_expr(&call.args[0]))
                    } else {
                        let mut block = String::from("(blk: { var _h: usize = 2166136261; ");
                        for arg in &call.args {
                            let gen = self.generate_expr(arg);
                            block.push_str(&format!("_h = (_h *% 16777619) ^ cp_hash_val({}); ", gen));
                        }
                        block.push_str("break :blk _h; })");
                        block
                    }
                }
                "@not" => {
                    let inner = self.generate_expr(&call.args[0]);
                    if self.is_lib_mode {
                        format!("(!{})", inner)
                    } else {
                        format!("cp_not({})", inner)
                    }
                }
                "@print" => {
                    if call.args.len() == 1 {
                        if let Expr::Template(t) = &call.args[0] {
                            let (pattern, args) = self.lower_template_stream(t);
                            let args_joined = if args.is_empty() {
                                String::new()
                            } else {
                                format!(" {}", args.join(", "))
                            };
                            return format!("std.debug.print(\"{}\\n\", .{{{}}})", pattern, args_joined);
                        }
                    }
                    let fmt = self.generate_expr(&call.args[0]);
                    if call.args.len() == 1 {
                        format!("std.debug.print(\"{{s}}\\n\", .{{ {} }})", fmt)
                    } else {
                        let mut args = String::from(".{ ");
                        for idx in 1..call.args.len() {
                            args.push_str(&self.generate_expr(&call.args[idx]));
                            if idx < call.args.len() - 1 {
                                args.push_str(", ");
                            }
                        }
                        args.push_str(" }");
                        format!("std.debug.print({} ++ \"\\n\", {})", fmt, args)
                    }
                }
                "@spawn" => {
                    let callee = match &call.args[0] {
                        Expr::Ident(id) => id.sym.clone(),
                        Expr::Arrow(_) => self.generate_expr(&call.args[0]),
                        _ => "Unknown".to_string(),
                    };
                    let mut captures = String::new();
                    if let Some(cv) = self.captured_vars.get(&callee) {
                        for (r, _) in cv {
                            if self.var_is_ref_struct.contains(r) {
                                panic!("CP Compile Error: Stack-bound ref struct '{}' cannot be captured across Actor @spawn boundary", r);
                            }
                            if self.var_borrow_sources.contains_key(r) {
                                panic!("CP Compile Error: Temporary borrow '{}' cannot be captured across Actor @spawn boundary (use move or Shared<T>)", r);
                            }
                            let src = if self.is_state_machine {
                                format!("ctx.{}", r)
                            } else {
                                r.clone()
                            };
                            captures.push_str(&format!("sub_ctx.{} = {}; ", r, src));
                        }
                    }
                    format!("(blk: {{ const arena_ptr = std.heap.page_allocator.create(std.heap.ArenaAllocator) catch unreachable; arena_ptr.* = std.heap.ArenaAllocator.init(std.heap.page_allocator); const sub_ctx = arena_ptr.allocator().create({0}_Context) catch unreachable; sub_ctx.* = undefined; sub_ctx.pc = 0; sub_ctx.arena = arena_ptr; sub_ctx._parent = null; sub_ctx._parent_fn = null; {1} const p = PID{{ .node_id = current_node_id, .local_id = next_local_id }}; next_local_id += 1; sub_ctx._self = p; schedule(@ptrCast(&{0}), @ptrCast(sub_ctx)); break :blk p; }})", callee, captures)
                }
                "@net_accept_async" => {
                    if !self.is_state_machine {
                        return "0".to_string();
                    }
                    let handle = self.generate_expr(&call.args[0]);
                    let next_pc = self.pc_counter + 1;
                    let current_fn = if let Some(c) = &self.current_class_name {
                        format!("{}.{}", c, self.current_fn_zig_name)
                    } else {
                        self.current_zig_fn_name.clone()
                    };
                    let logic = if self.is_beam_mode {
                        "unreachable;".to_string()
                    } else {
                        format!("const h: std.posix.fd_t = @intCast({}); const rc = std.os.linux.accept4(h, null, null, std.posix.SOCK.NONBLOCK | std.posix.SOCK.CLOEXEC); const err = std.posix.errno(rc); if (err == .SUCCESS) {{ ctx.received_msg = .{{ .int = @intCast(rc) }}; ctx.pc = {}; continue; }} else if (err == .AGAIN or err == .WOULDBLOCK) {{ register_fd(h, @ptrCast(&{}), @ptrCast(ctx), std.posix.POLL.IN); return; }} else {{ schedule(@ptrCast(&{}), @ptrCast(ctx)); return; }}", handle, next_pc, current_fn, current_fn)
                    };
                    self.write_to_case(&logic);
                    self.next_state();
                    "ctx.received_msg.?.int".to_string()
                }
                "@net_read_async" => {
                    if !self.is_state_machine {
                        return "0".to_string();
                    }
                    let handle = self.generate_expr(&call.args[0]);
                    let next_pc = self.pc_counter + 1;
                    let current_fn = if let Some(c) = &self.current_class_name {
                        format!("{}.{}", c, self.current_fn_zig_name)
                    } else {
                        self.current_zig_fn_name.clone()
                    };
                    let logic = if self.is_beam_mode {
                        "unreachable;".to_string()
                    } else {
                        format!("const h: std.posix.fd_t = @intCast({}); var buf = ctx.arena.?.allocator().alloc(u8, 4096) catch unreachable; if (std.posix.read(h, buf)) |bytes_read| {{ if (bytes_read > 0) {{ ctx.received_msg = .{{ .string = buf[0..bytes_read] }}; ctx.pc = {}; continue; }} else {{ schedule(@ptrCast(&{}), @ptrCast(ctx)); return; }} }} else |err| switch (err) {{ error.WouldBlock => {{ register_fd(h, @ptrCast(&{}), @ptrCast(ctx), std.posix.POLL.IN); return; }}, else => {{ schedule(@ptrCast(&{}), @ptrCast(ctx)); return; }}, }}", handle, next_pc, current_fn, current_fn, current_fn)
                    };
                    self.write_to_case(&logic);
                    self.next_state();
                    "ctx.received_msg.?.string".to_string()
                }
                "@net_write_async" => {
                    if !self.is_state_machine {
                        return "0".to_string();
                    }
                    let handle = self.generate_expr(&call.args[0]);
                    let data = self.generate_expr(&call.args[1]);
                    let next_pc = self.pc_counter + 1;
                    let current_fn = if let Some(c) = &self.current_class_name {
                        format!("{}.{}", c, self.current_fn_zig_name)
                    } else {
                        self.current_zig_fn_name.clone()
                    };
                    let logic = if self.is_beam_mode {
                        "unreachable;".to_string()
                    } else {
                        format!("const h: std.posix.fd_t = @intCast({}); const data_slice: []const u8 = {}; const rc = std.os.linux.write(h, data_slice.ptr, data_slice.len); const err = std.posix.errno(rc); if (err == .SUCCESS) {{ ctx.received_msg = .{{ .int = @intCast(rc) }}; ctx.pc = {}; continue; }} else if (err == .AGAIN or err == .WOULDBLOCK) {{ register_fd(h, @ptrCast(&{}), @ptrCast(ctx), std.posix.POLL.OUT); return; }} else {{ schedule(@ptrCast(&{}), @ptrCast(ctx)); return; }}", handle, data, next_pc, current_fn, current_fn)
                    };
                    self.write_to_case(&logic);
                    self.next_state();
                    "ctx.received_msg.?.int".to_string()
                }
                "@fs_read_async" => {
                    if !self.is_state_machine {
                        return "0".to_string();
                    }
                    let path = self.generate_expr(&call.args[0]);
                    let next_pc = self.pc_counter + 1;
                    let logic = if self.is_beam_mode {
                        format!("const io_msg = try beam.Beam.makeFastMessage(ctx.self.env.?, 99990001, @intFromEnum(beam.SystemAction.FS_READ), {}); ctx.pc = {}; const ctx_bin = try beam.Beam.makeBinary(ctx.self.env.?, std.mem.asBytes(ctx)); return beam.Beam.makeTuple(ctx.self.env.?, &[_]beam.Term{{ beam.Beam.makeAtom(ctx.self.env.?, \"yield\"), io_msg, ctx_bin }});", path, next_pc)
                    } else {
                        format!("const f = std.fs.cwd().openFile({}, .{{}}) catch unreachable; defer f.close(); const data = f.readToEndAlloc(ctx.arena.?.allocator(), 10 * 1024 * 1024) catch unreachable; ctx.received_msg = .{{ .string = data }}; ctx.pc = {}; continue;", path, next_pc)
                    };
                    self.write_to_case(&logic);
                    self.next_state();
                    "ctx.received_msg.?.string".to_string()
                }
                "@net_listen" => {
                    let port = self.generate_expr(&call.args[0]);
                    let rt_io = self.rt_sym("global_io");
                    format!("(blk: {{ const addr = @import(\"std\").Io.net.IpAddress.parse(\"127.0.0.1\", @intCast({})) catch unreachable; const _server = addr.listen({}, .{{}}) catch unreachable; break :blk @as(i32, @intCast(_server.socket.handle)); }})", port, rt_io)
                }
                "@net_close" => {
                    let handle = self.generate_expr(&call.args[0]);
                    format!("(blk: {{ const h: std.posix.fd_t = @intCast({}); _ = @import(\"std\").os.linux.close(h); break :blk {{}}; }})", handle)
                }
                "@yield_to_beam" => {
                    if !self.is_state_machine || !self.is_beam_mode {
                        return "0".to_string();
                    }
                    let action_id = self.generate_expr(&call.args[0]);
                    let payload = self.generate_expr(&call.args[1]);
                    let next_pc = self.pc_counter + 1;
                    let logic = format!(
                        "const io_msg = try beam.Beam.makeFastMessage(ctx.self.env.?, 99990001, @intCast({}), {}); \
                        ctx.pc = {}; \
                        const ctx_bin = try beam.Beam.makeBinary(ctx.self.env.?, std.mem.asBytes(ctx)); \
                        return beam.Beam.makeTuple(ctx.self.env.?, &[_]beam.Term{{ beam.Beam.makeAtom(ctx.self.env.?, \"yield\"), io_msg, ctx_bin }});",
                        action_id, payload, next_pc
                    );
                    self.write_to_case(&logic);
                    self.next_state();
                    "ctx.received_msg.?.string".to_string()
                }
                "@print_err" => {
                    if call.args.len() == 1 {
                        if let Expr::Template(t) = &call.args[0] {
                            let (pattern, args) = self.lower_template_stream(t);
                            let args_joined = if args.is_empty() {
                                String::new()
                            } else {
                                format!(" {}", args.join(", "))
                            };
                            return format!("std.debug.print(\"{}\\n\", .{{{}}})", pattern, args_joined);
                        }
                    }
                    let fmt = self.generate_expr(&call.args[0]);
                    let mut args = String::from(".{ ");
                    for idx in 1..call.args.len() {
                        args.push_str(&self.generate_expr(&call.args[idx]));
                        if idx < call.args.len() - 1 {
                            args.push_str(", ");
                        }
                    }
                    args.push_str(" }");
                    format!("std.debug.print({} ++ \"\\n\", {})", fmt, args)
                }
                "@io_write_stdout" => {
                    if call.args.len() == 1 {
                        if let Expr::Template(t) = &call.args[0] {
                            let (pattern, args) = self.lower_template_stream(t);
                            let args_joined = if args.is_empty() {
                                String::new()
                            } else {
                                format!(" {}", args.join(", "))
                            };
                            return format!("(blk: {{ const out = std.io.getStdOut().writer(); try out.print(\"{}\\n\", .{{{}}}); break :blk {{}}; }})", pattern, args_joined);
                        }
                    }
                    let fmt = self.generate_expr(&call.args[0]);
                    let mut args = String::from(".{ ");
                    for idx in 1..call.args.len() {
                        args.push_str(&self.generate_expr(&call.args[idx]));
                        if idx < call.args.len() - 1 {
                            args.push_str(", ");
                        }
                    }
                    args.push_str(" }");
                    format!("(blk: {{ const out = std.io.getStdOut().writer(); try out.print({} ++ \"\\n\", {}); break :blk {{}}; }})", fmt, args)
                }
                "@io_write_stderr" => {
                    if call.args.len() == 1 {
                        if let Expr::Template(t) = &call.args[0] {
                            let (pattern, args) = self.lower_template_stream(t);
                            let args_joined = if args.is_empty() {
                                String::new()
                            } else {
                                format!(" {}", args.join(", "))
                            };
                            return format!("(blk: {{ const err_w = std.io.getStdErr().writer(); try err_w.print(\"{}\\n\", .{{{}}}); break :blk {{}}; }})", pattern, args_joined);
                        }
                    }
                    let fmt = self.generate_expr(&call.args[0]);
                    let mut args = String::from(".{ ");
                    for idx in 1..call.args.len() {
                        args.push_str(&self.generate_expr(&call.args[idx]));
                        if idx < call.args.len() - 1 {
                            args.push_str(", ");
                        }
                    }
                    args.push_str(" }");
                    format!("(blk: {{ const err_w = std.io.getStdErr().writer(); try err_w.print({} ++ \"\\n\", {}); break :blk {{}}; }})", fmt, args)
                }
                "@io_read_stdin_line" => r#"(blk: { var buf: [1024]u8 = undefined; const line = (try std.io.getStdIn().reader().readUntilDelimiterOrEof(&buf, '\n')) orelse ""; break :blk try std.heap.page_allocator.dupe(u8, line); })"#.to_string(),
                "@http_fetch" => {
                    if !self.is_state_machine {
                        return "0".to_string();
                    }
                    let url = self.generate_expr(&call.args[0]);
                    let next_pc = self.pc_counter + 1;
                    let logic = if self.is_beam_mode {
                        format!("const payload_bin = try beam.Beam.makeBinary(ctx.self.env.?, {}); const io_msg = try beam.Beam.makeFastMessage(ctx.self.env.?, 99990001, @intFromEnum(beam.SystemAction.HTTP_FETCH), payload_bin); ctx.pc = {}; const ctx_bin = try beam.Beam.makeBinary(ctx.self.env.?, std.mem.asBytes(ctx)); return beam.Beam.makeTuple(ctx.self.env.?, &[_]beam.Term{{ beam.Beam.makeAtom(ctx.self.env.?, \"yield\"), io_msg, ctx_bin }});", url, next_pc)
                    } else {
                        format!("ctx.received_msg = .{{ .string = \"Mock HTTP Res\" }}; ctx.pc = {}; continue;", next_pc)
                    };
                    self.write_to_case(&logic);
                    self.next_state();
                    "ctx.received_msg.?.string".to_string()
                }
                "@fs_write_file" => {
                    let p = self.generate_expr(&call.args[0]);
                    let d = self.generate_expr(&call.args[1]);
                    let rt_io = self.rt_sym("global_io");
                    format!("(blk: {{ const file_out = @import(\"std\").Io.Dir.createFile(.cwd(), {0}, {1}, .{{}}) catch unreachable; defer @import(\"std\").Io.File.close(file_out, {0}); @import(\"std\").Io.File.writeStreamingAll(file_out, {0}, {2}) catch unreachable; break :blk {{}}; }})", rt_io, p, d)
                }
                "@fs_mkdir" => {
                    let p = self.generate_expr(&call.args[0]);
                    let rt_io = self.rt_sym("global_io");
                    format!("(blk: {{ @import(\"std\").Io.Dir.createDirPath(.cwd(), {}, {}) catch unreachable; break :blk {{}}; }})", rt_io, p)
                }
                "@fs_stat_size" => {
                    let p = self.generate_expr(&call.args[0]);
                    let rt_io = self.rt_sym("global_io");
                    format!("(blk: {{ const stat = @import(\"std\").Io.Dir.statFile(.cwd(), {}, {}, .{{}}) catch unreachable; break :blk @as(i32, @intCast(stat.size)); }})", rt_io, p)
                }
                "@fs_is_dir" => {
                    let p = self.generate_expr(&call.args[0]);
                    let rt_io = self.rt_sym("global_io");
                    format!("(blk: {{ const stat = @import(\"std\").Io.Dir.statFile(.cwd(), {}, {}, .{{}}) catch unreachable; break :blk stat.kind == .directory; }})", rt_io, p)
                }
                "@fs_remove" => {
                    let p = self.generate_expr(&call.args[0]);
                    let rt_io = self.rt_sym("global_io");
                    format!("(blk: {{ @import(\"std\").Io.Dir.deleteFile(.cwd(), {}, {}) catch unreachable; break :blk {{}}; }})", rt_io, p)
                }
                "@fs_rmdir" => {
                    let p = self.generate_expr(&call.args[0]);
                    let rt_io = self.rt_sym("global_io");
                    format!("(blk: {{ @import(\"std\").Io.Dir.deleteDir(.cwd(), {}, {}) catch unreachable; break :blk {{}}; }})", rt_io, p)
                }
                "@fs_rename" => {
                    let old_p = self.generate_expr(&call.args[0]);
                    let new_p = self.generate_expr(&call.args[1]);
                    let rt_io = self.rt_sym("global_io");
                    format!("(blk: {{ @import(\"std\").Io.Dir.rename(.cwd(), {}, .cwd(), {}, {}) catch unreachable; break :blk {{}}; }})", old_p, new_p, rt_io)
                }
                "@fs_copy_file" => {
                    let src = self.generate_expr(&call.args[0]);
                    let dest = self.generate_expr(&call.args[1]);
                    let rt_io = self.rt_sym("global_io");
                    format!("(blk: {{ @import(\"std\").Io.Dir.copyFile(.cwd(), {}, .cwd(), {}, {}, .{{}}) catch unreachable; break :blk {{}}; }})", src, dest, rt_io)
                }
                "@fs_exists" => {
                    let p = self.generate_expr(&call.args[0]);
                    let rt_io = self.rt_sym("global_io");
                    format!("(blk: {{ const is_exists = if (@import(\"std\").Io.Dir.statFile(.cwd(), {}, {}, .{{}})) |_| true else |err| if (err == error.FileNotFound) false else unreachable; break :blk is_exists; }})", rt_io, p)
                }
                "@crypto_uuid" => {
                    let allocator = if self.is_state_machine {
                        "ctx.arena.?.allocator()"
                    } else {
                        "std.heap.page_allocator"
                    };
                    format!("(blk: {{ var uuid: [16]u8 = undefined; _ = @import(\"std\").os.linux.getrandom(&uuid, 16, 0); uuid[6] = (uuid[6] & 0x0f) | 0x40; uuid[8] = (uuid[8] & 0x3f) | 0x80; const s = @import(\"std\").fmt.allocPrint({}, \"{{x:0>2}}{{x:0>2}}{{x:0>2}}{{x:0>2}}-{{x:0>2}}{{x:0>2}}-{{x:0>2}}{{x:0>2}}-{{x:0>2}}{{x:0>2}}-{{x:0>2}}{{x:0>2}}{{x:0>2}}{{x:0>2}}{{x:0>2}}{{x:0>2}}\", .{{ uuid[0], uuid[1], uuid[2], uuid[3], uuid[4], uuid[5], uuid[6], uuid[7], uuid[8], uuid[9], uuid[10], uuid[11], uuid[12], uuid[13], uuid[14], uuid[15] }}) catch unreachable; break :blk s; }})", allocator)
                }
                "@crypto_hash" => {
                    let allocator = if self.is_state_machine {
                        "ctx.arena.?.allocator()"
                    } else {
                        "std.heap.page_allocator"
                    };
                    let data = self.generate_expr(&call.args[0]);
                    format!("(blk: {{ var out: [32]u8 = undefined; @import(\"std\").crypto.hash.sha2.Sha256.hash({}, &out, .{{}}); const hex = @import(\"std\").fmt.bytesToHex(out, .lower); const hex_str = {}.dupe(u8, &hex) catch unreachable; break :blk hex_str; }})", data, allocator)
                }
                "@fs_read_file" => {
                    let p = self.generate_expr(&call.args[0]);
                    let rt_io = self.rt_sym("global_io");
                    format!("(blk: {{ const file_in = @import(\"std\").Io.Dir.openFile(.cwd(), {0}, {1}, .{{}}) catch unreachable; defer @import(\"std\").Io.File.close(file_in, {0}); const stat = @import(\"std\").Io.File.stat(file_in, {0}) catch unreachable; const data = std.heap.page_allocator.alloc(u8, @intCast(stat.size)) catch unreachable; _ = @import(\"std\").Io.File.readPositionalAll(file_in, {0}, data, 0) catch unreachable; break :blk data; }})", rt_io, p)
                }
                "@net_connect" => "(blk: { break :blk 1; })".to_string(),
                "@net_send" => "(blk: { break :blk {}; })".to_string(),
                "@worker_spawn" => {
                    let entry = self.generate_expr(&call.args[0]);
                    let rt_next_wid = self.rt_sym("global_next_worker_id");
                    let rt_cur_nid = self.rt_sym("current_node_id");
                    let rt_next_lid = self.rt_sym("next_local_id");
                    let rt_q_len = self.rt_sym("queue_len");
                    let rt_tq = self.rt_sym("task_queue");
                    let rt_active_fds = self.rt_sym("active_fds");
                    let rt_wait_cnt = self.rt_sym("waiting_count");
                    let rt_poll_fds = self.rt_sym("poll_fds");
                    let rt_fd_run = self.rt_sym("fd_to_run");
                    let rt_fd_ctx = self.rt_sym("fd_to_ctx");
                    format!("(blk: {{ const wid = {1}.fetchAdd(1, .monotonic); const _Runner = struct {{ pub fn run(w_id: u32) void {{ {2} = w_id; {3} = 2; const worker_ctx = std.heap.page_allocator.create({0}_Context) catch unreachable; worker_ctx.* = undefined; worker_ctx.pc = 0; worker_ctx._self = PID{{ .node_id = w_id, .local_id = 1 }}; worker_ctx.arena = null; worker_ctx._parent = null; worker_ctx._parent_fn = null; worker_ctx.received_msg = null; worker_ctx.received_sender = null; schedule(@ptrCast(&{0}), @ptrCast(worker_ctx)); while (true) {{ drain_cross_thread_mailbox(); while ({4} > 0) {{ const task = {5}[0]; for ({5}[1..{4}], 0..) |t, j| {{ {5}[j] = t; }} {4} -= 1; task.run(task.ctx) catch unreachable; }} drain_cross_thread_mailbox(); if ({6} == 0 and {4} == 0 and {7} == 0) break; if ({4} == 0 and {7} > 0 and {6} == 0) {{ const ts = std.os.linux.timespec{{ .sec = 0, .nsec = 500_000 }}; _ = std.os.linux.nanosleep(&ts, null); }} if ({6} > 0) {{ const ready_count = std.posix.poll({8}[0..{6}], 10) catch 0; if (ready_count > 0) {{ var i: usize = 0; while (i < {6}) {{ if ({8}[i].revents != 0) {{ schedule({9}[i], {10}[i]); {8}[i] = {8}[{6} - 1]; {10}[i] = {10}[{6} - 1]; {9}[i] = {9}[{6} - 1]; {6} -= 1; }} else {{ i += 1; }} }} }} }} }} }} }}; const _t = @import(\"std\").Thread.spawn(.{{}}, _Runner.run, .{{wid}}) catch unreachable; _t.detach(); break :blk PID{{ .node_id = wid, .local_id = 1 }}; }})", entry, rt_next_wid, rt_cur_nid, rt_next_lid, rt_q_len, rt_tq, rt_active_fds, rt_wait_cnt, rt_poll_fds, rt_fd_run, rt_fd_ctx)
                }
                "@create_pending_task" => {
                    format!("(blk: {{ const task_ptr = std.heap.page_allocator.create(CpTask) catch unreachable; task_ptr.* = .{{}}; break :blk task_ptr; }})")
                }
                "@complete_task" => {
                    let task = self.generate_expr(&call.args[0]);
                    let val = self.generate_expr(&call.args[1]);
                    format!("(blk: {{ const __cp_target_task: *CpTask = @ptrCast(@alignCast({0})); __cp_target_task.lock.acquire(); __cp_target_task.state = 2; __cp_target_task.base.await_result = cp_message_from({1}); const __jb = __cp_target_task.join_barrier; const __j_idx = __cp_target_task.join_idx; const __w_ctx_opt = __cp_target_task.waiter_ctx; const __w_fn_opt = __cp_target_task.waiter_fn; __cp_target_task.waiter_ctx = null; __cp_target_task.waiter_fn = null; const __res = __cp_target_task.base.await_result; __cp_target_task.lock.release(); if (__jb) |b| {{ b.on_task_done(__j_idx, __res orelse Message{{ .string = \"\" }}); }} if (__w_ctx_opt) |w_ctx| {{ const w_fn = __w_fn_opt.?; const base_waiter: *BaseContext = @ptrCast(@alignCast(w_ctx)); base_waiter.await_result = __res; schedule(w_fn, w_ctx); }} break :blk {{}}; }})", task, val)
                }
                "@send" | "@reply" => {
                    let target = self.generate_expr(&call.args[0]);
                    let msg = self.generate_expr(&call.args[1]);
                    let sender = if self.is_state_machine {
                        "ctx._self".to_string()
                    } else {
                        format!("PID{{ .node_id = {}, .local_id = 1 }}", self.rt_sym("current_node_id"))
                    };
                    format!("cp_send({}, {}, cp_message_from({}))", sender, target, msg)
                }
                "@self" => {
                    if self.is_state_machine {
                        "ctx._self".to_string()
                    } else {
                        format!("PID{{ .node_id = {}, .local_id = 1 }}", self.rt_sym("current_node_id"))
                    }
                }
                "@sender" => {
                    if self.is_state_machine {
                        "ctx.received_sender.?".to_string()
                    } else {
                        "PID{ .node_id = 0, .local_id = 0 }".to_string()
                    }
                }
                "@http_serve" => self.generate_http_serve(&call.args[0]),
                "@receive" => {
                    if !self.is_state_machine {
                        return "null".to_string();
                    }
                    let check_pc = self.pc_counter + 1;
                    self.write_to_case(&format!("ctx.pc = {}; continue;\n", check_pc));
                    self.next_state();
                    let resume_pc = check_pc + 1;
                    let logic = if self.is_beam_mode {
                        format!("ctx.pc = {}; const ctx_bin = try beam.Beam.makeBinary(ctx.self.env.?, std.mem.asBytes(ctx)); return beam.Beam.makeTuple(ctx.self.env.?, &[_]beam.Term{{ beam.Beam.makeAtom(ctx.self.env.?, \"yield\"), beam.Beam.makeAtom(ctx.self.env.?, \"receive\"), ctx_bin }});", resume_pc)
                    } else {
                        format!(
                            "if ({3}[ctx._self.local_id % 1024].pop()) |item| {{ ctx.received_msg = item.msg; ctx.received_sender = item.sender; ctx.pc = {0}; continue; }} else {{ ctx.received_msg = null; ctx.received_sender = null; {4}[ctx._self.local_id % 1024] = ctx; {5}[ctx._self.local_id % 1024] = @ptrCast(&{1}); {6} += 1; ctx.pc = {2}; return; }}",
                            resume_pc, self.current_zig_fn_name, check_pc, self.rt_sym("local_mailboxes"), self.rt_sym("waiting_actors"), self.rt_sym("waiting_actor_fns"), self.rt_sym("waiting_count")
                        )
                    };
                    self.write_to_case(&logic);
                    self.next_state();
                    "ctx.received_msg.?".to_string()
                }
                "@alloc" => {
                    let s = self.generate_expr(&call.args[0]);
                    format!("(blk: {{ const __alloc_res = {}.alloc(u8, {}) catch unreachable; break :blk __alloc_res.ptr; }})", self.current_allocator(), s)
                }
                "@free" => "(blk: { break :blk {}; })".to_string(),
                "@expect_eq" if call.args.len() == 2 => {
                    let a = self.generate_expr(&call.args[0]);
                    let b = self.generate_expr(&call.args[1]);
                    let rt_fail = self.rt_sym("__cap_test_failed");
                    let rt_cp_eq = self.rt_sym("cp_eq");
                    format!("(blk: {{ const _a = {}; const _b = {}; const is_eq = {}(_a, _b); if (!is_eq) {{ @import(\"std\").debug.print(\"\\n    ❌ Assertion Failed!\\n    Expected: {{any}}\\n    Got:      {{any}}\\n\", .{{ _b, _a }}); {} += 1; return error.TestFailed; }} break :blk {{}}; }})", a, b, rt_cp_eq, rt_fail)
                }
                "@test" if call.args.len() == 2 => {
                    let name = self.generate_expr(&call.args[0]);
                    let lambda = self.generate_expr(&call.args[1]);
                    format!("(blk: {{ _ = {}; _ = {}; break :blk {{}}; }})", name, lambda)
                }
                "@get_arena" | "@arena" => {
                    format!("(blk: {{ break :blk {}; }})", self.current_allocator())
                }
                "@shared_tensor_init" => "(blk: { break :blk 0; })".to_string(),
                "@json_parse" => {
                    let json_str = self.generate_expr(&call.args[0]);
                    let type_name = if call.args.len() > 1 {
                        if let Expr::Ident(id) = &call.args[1] {
                            id.sym.clone()
                        } else {
                            "std.json.Value".to_string()
                        }
                    } else {
                        "std.json.Value".to_string()
                    };

                    if type_name == "std.json.Value" {
                        format!("(blk: {{ const parsed = try @import(\"std\").json.parseFromSlice(std.json.Value, std.heap.page_allocator, {}, .{{ .ignore_unknown_fields = true }}); break :blk parsed.value; }})", json_str)
                    } else {
                        format!("(blk: {{ const parsed = try @import(\"std\").json.parseFromSlice({}, std.heap.page_allocator, {}, .{{ .ignore_unknown_fields = true }}); const json_ptr = try std.heap.page_allocator.create({}); json_ptr.* = parsed.value; break :blk json_ptr; }})", type_name, json_str, type_name)
                    }
                }
                "@json_stringify" => {
                    let obj = self.generate_expr(&call.args[0]);
                    format!("(blk: {{ break :blk try @import(\"std\").fmt.allocPrint(std.heap.page_allocator, \"{{f}}\", .{{@import(\"std\").json.fmt({}.*, .{{}})}}); }})", obj)
                }
                "@compileError" => {
                    let msg = self.generate_expr(&call.args[0]);
                    format!("@compileError({})", msg)
                }
                "@comptime" if call.args.len() == 1 => {
                    let expr = self.generate_expr(&call.args[0]);
                    format!("comptime ({})", expr)
                }
                "@compileLog" => {
                    let mut args_code = Vec::new();
                    for a in &call.args {
                        args_code.push(self.generate_expr(a));
                    }
                    format!("@compileLog({})", args_code.join(", "))
                }
                "@typeOf" => {
                    let val = self.generate_expr(&call.args[0]);
                    format!("@TypeOf({})", val)
                }
                "@typeName" => {
                    let t = self.generate_expr(&call.args[0]);
                    format!("@typeName({})", t)
                }
                "@fieldsOf" => {
                    let t = self.generate_expr(&call.args[0]);
                    format!("@typeInfo({}).@\"struct\".fields", t)
                }
                "@field" => {
                    let obj = self.generate_expr(&call.args[0]);
                    let f = self.generate_expr(&call.args[1]);
                    let f = strip_redundant_comptime(&f);
                    format!("@field({}, {})", obj, f)
                }
                "@hasField" => {
                    let t = self.generate_expr(&call.args[0]);
                    let f = self.generate_expr(&call.args[1]);
                    let f = strip_redundant_comptime(&f);
                    format!("@hasField({}, {})", t, f)
                }
                "@sizeOf" => {
                    let t = self.generate_expr(&call.args[0]);
                    format!("@sizeOf({})", t)
                }
                "@alignOf" => {
                    let t = self.generate_expr(&call.args[0]);
                    format!("@alignOf({})", t)
                }
                "@import" if call.args.len() == 1 => {
                    if let Expr::Lit(Lit::Str(src)) = &call.args[0] {
                        if src == "cap" || src == "cap:core" {
                            if self.modular_mode {
                                if self.modular_runtime_import.as_deref() == Some("__cap_hub_rt") {
                                    return "__cap_hub".to_string();
                                }
                                if self.single_file_cap_submod {
                                    return "@import(\"./index.zig\")".to_string();
                                }
                                if let Some(mapped) = self.modular_import_map.get(src) {
                                    return format!("@import(\"{}\")", mapped);
                                }
                            } else {
                                return "__cap_hub".to_string();
                            }
                        } else if self.modular_mode {
                            if let Some(mapped) = self.modular_import_map.get(src) {
                                return format!("@import(\"{}\")", mapped);
                            }
                        }
                    }
                    let mut args_code = Vec::new();
                    for a in &call.args {
                        args_code.push(self.generate_expr(a));
                    }
                    format!("{}({})", call.name, args_code.join(", "))
                }
                _ => {
                    let mut args_code = Vec::new();
                    for a in &call.args {
                        args_code.push(self.generate_expr(a));
                    }
                    format!("{}({})", call.name, args_code.join(", "))
                }
            },
            _ => unreachable!(),
        }
    }

    pub(crate) fn generate_http_serve(&mut self, call_arg: &Expr) -> String {
        let mut port_expr = "3000".to_string();
        let mut fetch_fn_name = String::new();

        match call_arg {
            Expr::Object(fields) => {
                for f in fields {
                    if let Some(key) = &f.key {
                        if key.sym == "port" {
                            port_expr = self.generate_expr(&f.val);
                        } else if key.sym == "fetch" {
                            match &f.val {
                                Expr::Ident(id) => {
                                    fetch_fn_name = id.sym.clone();
                                }
                                Expr::Arrow(arrow) => {
                                    let handler_name =
                                        format!("http_handler_{}", self.lambda_counter);
                                    self.lambda_counter += 1;
                                    let mut handler_output = String::new();
                                    let mut h_gen = Codegen::new();
                                    h_gen.modular_mode = self.modular_mode;
                                    h_gen.is_lib_mode = self.is_lib_mode;
                                    h_gen.is_beam_mode = self.is_beam_mode;
                                    h_gen.globals = self.globals.clone();
                                    h_gen.class_defs = self.class_defs.clone();
                                    h_gen.ref_classes = self.ref_classes.clone();
                                    h_gen.value_classes = self.value_classes.clone();
                                    h_gen.enum_defs = self.enum_defs.clone();
                                    h_gen.error_sets = self.error_sets.clone();
                                    h_gen.fn_sigs = self.fn_sigs.clone();
                                    h_gen.trait_defs = self.trait_defs.clone();
                                    h_gen.var_cp_types = self.var_cp_types.clone();
                                    h_gen.anon_obj_structs = self.anon_obj_structs.clone();
                                    h_gen.is_state_machine = false;
                                    h_gen.current_zig_fn_name = handler_name.clone();

                                    let param_name = if !arrow.params.is_empty() {
                                        if let crate::ast::Pattern::Ident(p_id) = &arrow.params[0] {
                                            p_id.sym.clone()
                                        } else {
                                            "req".to_string()
                                        }
                                    } else {
                                        "req".to_string()
                                    };
                                    h_gen
                                        .locals
                                        .insert(param_name.clone(), "*Request".to_string());

                                    writeln!(
                                        handler_output,
                                        "pub fn {}({}: *Request) *Response {{",
                                        handler_name, param_name
                                    )
                                    .unwrap();
                                    writeln!(handler_output, "    _ = &{};", param_name).unwrap();
                                    match &*arrow.body {
                                        crate::ast::ArrowBody::Expr(e) => {
                                            let ret = h_gen.generate_expr(e);
                                            writeln!(handler_output, "    return {};", ret)
                                                .unwrap();
                                        }
                                        crate::ast::ArrowBody::Block(b) => {
                                            for s in &b.body {
                                                h_gen.generate_stmt(s);
                                            }
                                            handler_output.push_str(&h_gen.current_case_content);
                                        }
                                    }
                                    writeln!(handler_output, "}}\n").unwrap();
                                    self.lambda_funcs.push(handler_output);
                                    fetch_fn_name = handler_name;
                                }
                                _ => {
                                    fetch_fn_name = self.generate_expr(&f.val);
                                }
                            }
                        }
                    }
                }
            }
            _ => match call_arg {
                Expr::Ident(id) => fetch_fn_name = id.sym.clone(),
                _ => fetch_fn_name = self.generate_expr(call_arg),
            },
        }

        let rt_io = self.rt_sym("global_io");
        format!(
            r#"(blk: {{
    const _port: u16 = @intCast({});
    const _addr = @import("std").Io.net.IpAddress.parse("0.0.0.0", _port) catch unreachable;
    const _listener = _addr.listen({}, .{{ .reuse_address = true }}) catch unreachable;
    const _s_fd: i32 = @intCast(_listener.socket.handle);
    const _HttpState = struct {{
        server_fd: i32,
        pub fn on_accept(s_ptr: *anyopaque) anyerror!void {{
            const self_state: *@This() = @ptrCast(@alignCast(s_ptr));
            var client_addr: std.os.linux.sockaddr = undefined;
            var client_addr_len: std.os.linux.socklen_t = @sizeOf(std.os.linux.sockaddr);
            const flags = std.os.linux.SOCK.CLOEXEC;
            const rc = std.os.linux.accept4(self_state.server_fd, &client_addr, &client_addr_len, flags);
            register_fd(self_state.server_fd, &on_accept, s_ptr, std.posix.POLL.IN);
            if (rc > 65535) return;
            const client_fd: i32 = @intCast(rc);
            defer _ = std.os.linux.close(client_fd);
            var buf: [4096]u8 = undefined;
            const n = std.os.linux.read(client_fd, &buf, buf.len);
            if (n == 0 or n > buf.len) return;
            const raw_req = buf[0..n];
            var method: []const u8 = "GET";
            var path: []const u8 = "/";
            var body: []const u8 = "";
            var iter = std.mem.splitScalar(u8, raw_req, '\r');
            if (iter.next()) |first_line| {{
                var parts = std.mem.splitScalar(u8, first_line, ' ');
                if (parts.next()) |m| {{ method = m; }}
                if (parts.next()) |p| {{ path = p; }}
            }}
            if (std.mem.indexOf(u8, raw_req, "\r\n\r\n")) |body_idx| {{
                body = raw_req[body_idx + 4..];
            }}
            const req_obj = try Request._create(std.heap.page_allocator);
            req_obj.method = method;
            req_obj.url = path;
            req_obj.body = body;
            const res_obj: *Response = {}(req_obj);
            const status_text = switch (res_obj.status) {{
                200 => "OK",
                201 => "Created",
                204 => "No Content",
                400 => "Bad Request",
                401 => "Unauthorized",
                403 => "Forbidden",
                404 => "Not Found",
                500 => "Internal Server Error",
                else => "OK",
            }};
            const response_str = try std.fmt.allocPrint(std.heap.page_allocator,
                "HTTP/1.1 {{d}} {{s}}\r\nContent-Type: {{s}}\r\nContent-Length: {{d}}\r\nConnection: close\r\n\r\n{{s}}",
                .{{ res_obj.status, status_text, res_obj.contentType, res_obj.body.len, res_obj.body }}
            );
            defer std.heap.page_allocator.free(response_str);
            _ = std.os.linux.write(client_fd, response_str.ptr, response_str.len);
        }}
    }};
    const state_ptr = std.heap.page_allocator.create(_HttpState) catch unreachable;
    state_ptr.* = .{{ .server_fd = _s_fd }};
    register_fd(_s_fd, &_HttpState.on_accept, state_ptr, std.posix.POLL.IN);
    const s_obj = HttpServer._create(std.heap.page_allocator) catch unreachable;
    s_obj.port = _port;
    s_obj.fd = _s_fd;
    break :blk s_obj;
}})"#,
            port_expr, rt_io, fetch_fn_name
        )
    }
}
