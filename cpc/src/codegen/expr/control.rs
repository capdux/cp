use super::super::*;
use crate::ast::*;
use crate::Span;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

impl Codegen {
    pub(crate) fn generate_expr_control(&mut self, expr: &Expr) -> String {
        match expr {
            Expr::OptMember(mem) => {
                let obj_code = self.generate_expr(&mem.obj);
                let prop = match mem.prop.sym.as_str() {
                    "length" => "len",
                    other => other,
                };
                let id = self.lambda_counter;
                self.lambda_counter += 1;
                format!(
                    "(blk_om_{id}: {{ const __o_{id} = {obj_code}; const __R_{id} = CpOptType(@TypeOf(cp_opt_unwrap(__o_{id}).{prop})); if (cp_opt_is_null(__o_{id})) break :blk_om_{id} @as(__R_{id}, null); break :blk_om_{id} @as(__R_{id}, cp_opt_unwrap(__o_{id}).{prop}); }})"
                )
            }
            Expr::OptCall(call) => {
                if let Expr::OptMember(mem) | Expr::Member(mem) = &*call.callee {
                    let obj_code = self.generate_expr(&mem.obj);
                    let prop = &mem.prop.sym;
                    let mut args = String::new();
                    for (idx, arg) in call.args.iter().enumerate() {
                        args.push_str(&self.generate_expr(arg));
                        if idx + 1 < call.args.len() {
                            args.push_str(", ");
                        }
                    }
                    let id = self.lambda_counter;
                    self.lambda_counter += 1;
                    format!(
                        "(blk_oc_{id}: {{ const __o_{id} = {obj_code}; const __R_{id} = CpOptType(@TypeOf(cp_opt_unwrap(__o_{id}).{prop}({args}))); if (cp_opt_is_null(__o_{id})) break :blk_oc_{id} @as(__R_{id}, null); break :blk_oc_{id} @as(__R_{id}, cp_opt_unwrap(__o_{id}).{prop}({args})); }})"
                    )
                } else {
                    "null".to_string()
                }
            }
            Expr::Match(m) => self.generate_switch(m, true),
            Expr::If {
                test,
                cons,
                alt,
                is_comptime,
                capture,
                is_ptr_capture,
                ..
            } => {
                let not_null_vars = Self::extract_not_null_vars(test);
                let is_null_vars = Self::extract_is_null_vars(test);
                let test_s = self.generate_expr(test);
                let old_narrowed = self.narrowed_null_vars.clone();
                self.narrowed_null_vars.extend(not_null_vars);
                let cons_s = self.generate_expr(cons);
                self.narrowed_null_vars = old_narrowed.clone();
                let alt_s = if let Some(a) = alt {
                    self.narrowed_null_vars.extend(is_null_vars);
                    let res = self.generate_expr(a);
                    self.narrowed_null_vars = old_narrowed;
                    res
                } else {
                    "null".to_string()
                };
                let cap_s = if let Some(cap) = capture {
                    if *is_ptr_capture {
                        format!(" |*{}|", cap.sym)
                    } else {
                        format!(" |{}|", cap.sym)
                    }
                } else {
                    String::new()
                };
                if *is_comptime {
                    format!(
                        "(if (comptime {}){} {} else {})",
                        test_s, cap_s, cons_s, alt_s
                    )
                } else {
                    format!("(if ({}){} {} else {})", test_s, cap_s, cons_s, alt_s)
                }
            }
            Expr::New(call) => {
                let raw_class_name = match &*call.callee {
                    Expr::Ident(id) => id.sym.clone(),
                    _ => "Unknown".to_string(),
                };
                let class_name = self.map_type_name(&raw_class_name);
                let base_name = if let Some(pos) = raw_class_name.find('<') {
                    &raw_class_name[..pos]
                } else {
                    &raw_class_name
                };
                if let Some(cls) = self.class_defs.get(base_name) {
                    if cls.is_static {
                        panic!("CP Compile Error: Cannot instantiate static class or struct '{}' with 'new'", raw_class_name);
                    }
                }
                if class_name == "Error" {
                    let code = if !call.args.is_empty() {
                        self.generate_expr(&call.args[0])
                    } else {
                        "0".to_string()
                    };
                    let msg = if call.args.len() > 1 {
                        self.generate_expr(&call.args[1])
                    } else {
                        "\"\"".to_string()
                    };
                    return format!("Error.init({}, {})", code, msg);
                }
                let init_params: Vec<Param> = self
                    .lookup_method(base_name, "init")
                    .map(|(p, _, _)| {
                        p.into_iter()
                            .filter(|param| {
                                param.ident.sym != "this"
                                    && param.ident.sym != "_"
                                    && param.ident.sym != "self"
                            })
                            .collect()
                    })
                    .unwrap_or_default();
                let mut args = String::new();
                for (idx, arg) in call.args.iter().enumerate() {
                    let mut arg_is_move_param = false;
                    if idx < init_params.len() {
                        let param = &init_params[idx];
                        if let Expr::Ident(arg_id) = arg {
                            if param.is_mut {
                                self.check_borrow_conflict(&arg_id.sym, true);
                            } else if param.is_move {
                                self.check_active_borrows_before_move(&arg_id.sym);
                                arg_is_move_param = true;
                            } else {
                                self.check_borrow_conflict(&arg_id.sym, false);
                            }
                        }
                    }
                    let gen_arg = self.generate_expr(arg);
                    if arg_is_move_param {
                        if let Expr::Ident(arg_id) = arg {
                            self.moved_vars.insert(arg_id.sym.clone());
                            if !self.is_state_machine && self.drop_flag_vars.contains(&arg_id.sym) {
                                args.push_str(&format!(
                                    "(blk_arg: {{ __{}_moved = true; break :blk_arg {}; }})",
                                    arg_id.sym, gen_arg
                                ));
                            } else {
                                args.push_str(&gen_arg);
                            }
                        } else {
                            args.push_str(&gen_arg);
                        }
                    } else {
                        args.push_str(&gen_arg);
                    }
                    if idx < call.args.len() - 1 {
                        args.push_str(", ");
                    }
                }
                self.tmp_counter += 1;
                let blk_lbl = format!("blk_new_{}", self.tmp_counter);
                if args.is_empty() {
                    format!("({}: {{ const _obj = {}._create({}) catch unreachable; if (@hasDecl({}, \"init\")) {{ if (@typeInfo(@TypeOf({}.init)).@\"fn\".param_types.len == 1) {{ const RType = @typeInfo(@TypeOf({}.init)).@\"fn\".return_type.?; if (@typeInfo(RType) == .error_union) {{ _obj.init() catch unreachable; }} else {{ _obj.init(); }} }} }} break :{} _obj; }})", blk_lbl, class_name, self.current_allocator(), class_name, class_name, class_name, blk_lbl)
                } else {
                    format!("({}: {{ const _obj = {}._create({}) catch unreachable; if (@hasDecl({}, \"init\")) {{ const RType = @typeInfo(@TypeOf({}.init)).@\"fn\".return_type.?; if (@typeInfo(RType) == .error_union) {{ _obj.init({}) catch unreachable; }} else {{ _obj.init({}); }} }} break :{} _obj; }})", blk_lbl, class_name, self.current_allocator(), class_name, class_name, args, args, blk_lbl)
                }
            }
            Expr::Await(inner) => {
                if let Expr::Call(call) = &**inner {
                    if let Expr::Ident(id) = &*call.callee {
                        if id.sym == "join" || id.sym == "@task_join" {
                            if self.is_state_machine {
                                let next_pc = self.pc_counter + 1;
                                let current_fn = if let Some(c) = &self.current_class_name {
                                    format!("{}.{}", c, self.current_fn_zig_name)
                                } else {
                                    self.current_zig_fn_name.clone()
                                };
                                let mut reg_tasks = String::new();
                                for (idx, arg) in call.args.iter().enumerate() {
                                    let arg_code = self.generate_expr(arg);
                                    reg_tasks.push_str(&format!(
                                        "const __t{idx}: *CpTask = @ptrCast(@alignCast({arg_code})); \
                                        __t{idx}.lock.acquire(); \
                                        if (__t{idx}.state == 2) {{ \
                                            const __res{idx} = __t{idx}.base.await_result orelse Message{{ .string = \"\" }}; \
                                            __t{idx}.lock.release(); \
                                            barrier.on_task_done({idx}, __res{idx}); \
                                        }} else {{ \
                                            __t{idx}.join_barrier = barrier; \
                                            __t{idx}.join_idx = {idx}; \
                                            __t{idx}.lock.release(); \
                                        }} "
                                    ));
                                }
                                let logic = format!(
                                    "const barrier = JoinBarrier.init(ctx.arena.?.allocator(), {0}) catch unreachable; \
                                    barrier.waiter_ctx = @ptrCast(ctx); \
                                    barrier.waiter_fn = @ptrCast(&{1}); \
                                    {2} \
                                    ctx.pc = {3}; \
                                    if (barrier.finish_setup_and_should_suspend()) return; \
                                    continue;",
                                    call.args.len(), current_fn, reg_tasks, next_pc
                                );
                                self.write_to_case(&logic);
                                self.next_state();
                                return "ctx.await_result".to_string();
                            }
                        }
                    }
                    if self.is_state_machine {
                        let (callee_init, callee_fn) = match &*call.callee {
                            Expr::Ident(id) => (id.sym.clone(), id.sym.clone()),
                            Expr::Member(mem) => {
                                let obj_type = self.infer_zig_type(None, Some(&mem.obj));
                                let base = obj_type.trim_start_matches('*');
                                let prop = &mem.prop;
                                (
                                    format!("{}.{}_{}", base, base, prop.sym),
                                    format!("{}.{}", base, prop.sym),
                                )
                            }
                            _ => ("Unknown".to_string(), "Unknown".to_string()),
                        };
                        let mut args_str = String::new();
                        if let Expr::Member(mem) = &*call.callee {
                            args_str.push_str(", ");
                            args_str.push_str(&self.generate_expr(&mem.obj));
                        }
                        for arg in &call.args {
                            args_str.push_str(", ");
                            args_str.push_str(&self.generate_expr(arg));
                        }
                        let next_pc = self.pc_counter + 1;
                        let current_fn = if let Some(c) = &self.current_class_name {
                            format!("{}.{}", c, self.current_fn_zig_name)
                        } else {
                            self.current_zig_fn_name.clone()
                        };
                        let logic = format!(
                            "const child_ctx = {}_init(ctx.arena.?, @ptrCast(ctx), @ptrCast(&{}){}); \
                            schedule(@ptrCast(&{}), @ptrCast(child_ctx)); \
                            ctx.pc = {}; \
                            return;",
                            callee_init, current_fn, args_str, callee_fn, next_pc
                        );
                        self.write_to_case(&logic);
                        self.next_state();

                        let inferred = self.infer_zig_type(None, Some(inner));
                        if inferred == "void" {
                            "(blk: { break :blk {}; })".to_string()
                        } else if inferred == "[]const u8" {
                            "ctx.await_result.?.string".to_string()
                        } else if inferred == "i32" || inferred == "usize" || inferred == "u32" {
                            format!("@as({}, @intCast(ctx.await_result.?.int))", inferred)
                        } else if inferred.starts_with('*') {
                            format!(
                                "@as({}, @ptrCast(@alignCast(ctx.await_result.?.any)))",
                                inferred
                            )
                        } else if inferred.starts_with("struct {") {
                            format!(
                                "@as(*{}, @ptrCast(@alignCast(ctx.await_result.?.any))).*",
                                inferred
                            )
                        } else {
                            "ctx.await_result".to_string()
                        }
                    } else {
                        self.generate_expr(inner)
                    }
                } else if matches!(&**inner, Expr::Builtin(_)) {
                    self.generate_expr(inner)
                } else if self.is_state_machine {
                    let task_code = self.generate_expr(inner);
                    let next_pc = self.pc_counter + 1;
                    let current_fn = if let Some(c) = &self.current_class_name {
                        format!("{}.{}", c, self.current_fn_zig_name)
                    } else {
                        self.current_zig_fn_name.clone()
                    };
                    let logic = format!(
                        "const __single_task: *CpTask = @ptrCast(@alignCast({0})); \
                        __single_task.lock.acquire(); \
                        if (__single_task.state == 2) {{ \
                            ctx.await_result = __single_task.base.await_result; \
                            __single_task.lock.release(); \
                            ctx.pc = {2}; \
                            continue; \
                        }} else {{ \
                            __single_task.waiter_ctx = @ptrCast(ctx); \
                            __single_task.waiter_fn = @ptrCast(&{1}); \
                            ctx.pc = {2}; \
                            __single_task.lock.release(); \
                            return; \
                        }}",
                        task_code, current_fn, next_pc
                    );
                    self.write_to_case(&logic);
                    self.next_state();
                    "ctx.await_result".to_string()
                } else {
                    self.generate_expr(inner)
                }
            }
            Expr::Member(member) => {
                if let Expr::Ident(id) = &*member.obj {
                    if id.sym == "error"
                        && member
                            .prop
                            .sym
                            .chars()
                            .next()
                            .map_or(false, |c| c.is_ascii_uppercase())
                    {
                        return format!("error.{}", member.prop.sym);
                    }
                    if id.sym == "Math" {
                        if member.prop.sym == "PI" {
                            return "@import(\"std\").math.pi".to_string();
                        }
                        if member.prop.sym == "E" {
                            return "@import(\"std\").math.e".to_string();
                        }
                    }
                    if let Some(en) = self.enum_defs.get(&id.sym) {
                        let is_pure_enum = en.variants.iter().all(|v| {
                            v.fields.is_none() && v.tuple_type.is_none() && v.from_err.is_none()
                        });
                        if is_pure_enum {
                            return format!("{}.{}", id.sym, member.prop.sym);
                        } else {
                            return format!("{}{{ .{} = {{}} }}", id.sym, member.prop.sym);
                        }
                    }
                    if self.error_sets.contains_key(&id.sym) {
                        return format!("{}.{}", id.sym, member.prop.sym);
                    }
                    if id.sym.contains('<') && id.sym.ends_with('>') {
                        let mapped = self.map_type_name(&id.sym);
                        return format!("{}.{}", mapped, member.prop.sym);
                    }
                }
                if matches!(&*member.obj, Expr::OptMember(_) | Expr::OptCall(_)) {
                    let obj_code = self.generate_expr(&member.obj);
                    let prop = match member.prop.sym.as_str() {
                        "length" => "len",
                        other => other,
                    };
                    let id = self.lambda_counter;
                    self.lambda_counter += 1;
                    return format!(
                        "(blk_om_{id}: {{ const __o_{id} = {obj_code}; const __R_{id} = CpOptType(@TypeOf(cp_opt_unwrap(__o_{id}).{prop})); if (cp_opt_is_null(__o_{id})) break :blk_om_{id} @as(__R_{id}, null); break :blk_om_{id} @as(__R_{id}, cp_opt_unwrap(__o_{id}).{prop}); }})"
                    );
                }
                let obj = self.generate_expr(&member.obj);
                let cp_ty = self.infer_cp_type(&member.obj);
                let obj_ty = self.infer_zig_type(None, Some(&member.obj));

                let is_array_or_string = match &cp_ty {
                    Some(Type::String) | Some(Type::Array(_)) | Some(Type::FixedArray(_, _)) => {
                        true
                    }
                    _ => false,
                };
                let prop_str = if is_array_or_string && member.prop.sym == "length" {
                    "len".to_string()
                } else {
                    member.prop.sym.clone()
                };
                let prop = prop_str.as_str();
                let is_guard = obj_ty.contains("LockGuard(")
                    || obj_ty.contains("ReadGuard(")
                    || obj_ty.contains("WriteGuard(");
                let safe_prop = if matches!(prop, "and" | "or" | "orelse") {
                    format!("@\"{}\"", prop)
                } else {
                    prop.to_string()
                };
                if is_guard && prop != "release" && prop != "unlock" {
                    return if obj_ty.starts_with('?') {
                        format!("({}).?.data.{}", obj, safe_prop)
                    } else {
                        format!("{}.data.{}", obj, safe_prop)
                    };
                }

                let is_struct_or_class = match &cp_ty {
                    Some(Type::Ref(id)) => self.class_defs.contains_key(&id.sym),
                    Some(Type::BorrowType { inner, .. }) => match &**inner {
                        Type::Ref(id) => self.class_defs.contains_key(&id.sym),
                        _ => false,
                    },
                    _ => false,
                };

                let is_enum = match &cp_ty {
                    Some(Type::Ref(id)) => self.enum_defs.contains_key(&id.sym),
                    _ => false,
                };
                if !is_struct_or_class {
                    if is_enum && member.prop.sym == "name" {
                        return format!("@tagName({})", obj);
                    }
                    if is_enum && member.prop.sym == "value" {
                        return format!("@intFromEnum({})", obj);
                    }
                    if member.prop.sym == "name" {
                        return format!("(if (@typeInfo(@TypeOf({0})) == .@\"enum\" or @typeInfo(@TypeOf({0})) == .@\"union\") @tagName({0}) else {0}.name)", obj);
                    }
                    if member.prop.sym == "value" {
                        return format!("(if (@typeInfo(@TypeOf({0})) == .@\"enum\") @intFromEnum({0}) else {0}.value)", obj);
                    }
                }

                let is_cstr = matches!(cp_ty, Some(Type::CStr)) || obj_ty.contains("[*:0]");
                if is_cstr && (member.prop.sym == "len" || member.prop.sym == "length") {
                    return format!("std.mem.span({}).len", obj);
                }

                if obj_ty.starts_with('?') {
                    format!("({}).?.{}", obj, safe_prop)
                } else if obj_ty.starts_with("*[]") {
                    format!("{}.*.{}", obj, safe_prop)
                } else {
                    format!("{}.{}", obj, safe_prop)
                }
            }
            Expr::Arrow(arrow) => {
                let lambda_name = format!("lambda_{}", self.lambda_counter);
                self.lambda_counter += 1;
                let old_fn = self.current_zig_fn_name.clone();
                let mut lambda_output = String::new();
                let mut lambda_gen = Codegen::new();
                lambda_gen.modular_mode = self.modular_mode;
                lambda_gen.is_lib_mode = self.is_lib_mode;
                lambda_gen.is_beam_mode = self.is_beam_mode;
                lambda_gen.globals = self.globals.clone();
                lambda_gen.class_defs = self.class_defs.clone();
                lambda_gen.ref_classes = self.ref_classes.clone();
                lambda_gen.value_classes = self.value_classes.clone();
                lambda_gen.enum_defs = self.enum_defs.clone();
                lambda_gen.error_sets = self.error_sets.clone();
                lambda_gen.fn_sigs = self.fn_sigs.clone();
                lambda_gen.trait_defs = self.trait_defs.clone();
                lambda_gen.var_cp_types = self.var_cp_types.clone();
                lambda_gen.global_var_types = self.global_var_types.clone();
                lambda_gen.anon_obj_structs = self.anon_obj_structs.clone();
                lambda_gen.type_aliases = self.type_aliases.clone();
                lambda_gen.is_state_machine = true;
                lambda_gen.current_fn_is_async = true;
                lambda_gen.current_zig_fn_name = lambda_name.clone();
                let ctx_name = format!("{}_Context", lambda_name);
                writeln!(lambda_output, "const {} = struct {{ pc: u32 = 0, _self: PID = undefined, _parent: ?*anyopaque = null, _parent_fn: ?*const fn(*anyopaque) anyerror!void = null, arena: ?*std.heap.ArenaAllocator = null, received_msg: ?Message = null, received_sender: ?PID = null, await_result: ?Message = null,", ctx_name).unwrap();

                let mut refs = std::collections::HashSet::new();
                match &*arrow.body {
                    ArrowBody::Expr(e) => {
                        crate::ast_visitor::find_referenced_vars_in_expr(e, &mut refs)
                    }
                    ArrowBody::Block(b) => {
                        crate::ast_visitor::find_referenced_vars_in_block(b, &mut refs)
                    }
                }
                let decls = match &*arrow.body {
                    ArrowBody::Expr(_) => std::collections::HashSet::new(),
                    ArrowBody::Block(b) => crate::ast_visitor::get_declared_vars_in_block(b),
                };
                for d in decls {
                    refs.remove(&d);
                }
                let mut captured = Vec::new();
                for r in refs {
                    if r == "this" || r == "_" {
                        if let Some(self_ty) = self.locals.get("self") {
                            captured.push(("self".to_string(), self_ty.clone()));
                        } else if let Some(c) = &self.current_class_name {
                            captured.push(("self".to_string(), format!("*{}", c)));
                        }
                    } else if let Some(ty) = self.locals.get(&r) {
                        if self.moved_vars.contains(&r) {
                            panic!("CP Compile Error: Use of moved variable '{}'", r);
                        }
                        captured.push((r.clone(), ty.clone()));
                    }
                }
                self.captured_vars
                    .insert(lambda_name.clone(), captured.clone());

                for (r, ty) in &captured {
                    lambda_gen.locals.insert(r.clone(), ty.clone());
                    if r != "self" || self.current_receiver_mode != ReceiverMode::Mut {
                        lambda_gen.current_fn_params.insert(r.clone());
                    }
                    if !self.var_borrow_sources.contains_key(r)
                        && (!self.current_fn_params.contains(r) || self.move_fn_params.contains(r))
                    {
                        lambda_gen.move_fn_params.insert(r.clone());
                    }
                    writeln!(lambda_output, "    {}: {},", r, ty).unwrap();
                }

                for param in &arrow.params {
                    if let Pattern::Ident(pid) = param {
                        lambda_gen
                            .locals
                            .insert(pid.sym.clone(), "?*anyopaque".to_string());
                        writeln!(lambda_output, "    {}: ?*anyopaque = null,", pid.sym).unwrap();
                    }
                }

                if let ArrowBody::Block(b) = &*arrow.body {
                    lambda_gen.extract_local_vars_to_str(b, &mut lambda_output);
                }
                writeln!(lambda_output, "}};").unwrap();
                writeln!(
                    lambda_output,
                    "pub fn {}(ptr: *anyopaque) anyerror!void {{",
                    lambda_name
                )
                .unwrap();
                writeln!(
                    lambda_output,
                    "    const ctx: *{} = @ptrCast(@alignCast(ptr)); while (true) {{ switch (ctx.pc) {{",
                    ctx_name
                )
                .unwrap();
                lambda_gen.indent_level = 2;
                match &*arrow.body {
                    ArrowBody::Expr(e) => {
                        let ec = lambda_gen.generate_expr(e);
                        lambda_gen.write_to_case(&format!(
                            "const __ret_val = {}; if (ctx._parent) |p| {{ const base: *BaseContext = @ptrCast(@alignCast(p)); base.await_result = cp_message_from(__ret_val); schedule(ctx._parent_fn.?, p); }} return;",
                            ec
                        ));
                    }
                    ArrowBody::Block(b) => {
                        for s in &b.body {
                            lambda_gen.generate_stmt(s);
                        }
                    }
                }
                lambda_gen.emit_current_case();
                for case in &lambda_gen.cases {
                    lambda_output.push_str(case);
                }
                writeln!(
                    lambda_output,
                    "        else => unreachable,\n    }} }} }}\n"
                )
                .unwrap();
                self.lambda_funcs.push(lambda_output);

                self.lambda_moved_vars
                    .insert(lambda_name.clone(), lambda_gen.moved_vars.clone());
                for moved in &lambda_gen.moved_vars {
                    if self.locals.contains_key(moved) {
                        self.check_active_borrows_before_move(moved);
                        self.moved_vars.insert(moved.clone());
                    }
                }

                self.current_zig_fn_name = old_fn;
                lambda_name
            }
            Expr::Unsafe(b) => {
                let saved = self.current_case_content.clone();
                self.current_case_content.clear();
                let len = b.body.len();
                for (idx, s) in b.body.iter().enumerate() {
                    if idx + 1 == len {
                        if let Stmt::Expr(e) = s {
                            let ec = self.generate_expr(e);
                            self.write_to_case(&format!("break :blk {};", ec));
                        } else {
                            self.generate_stmt(s);
                        }
                    } else {
                        self.generate_stmt(s);
                    }
                }
                let body_str = self.current_case_content.clone();
                self.current_case_content = saved;
                format!("(blk: {{\n{}\n}})", body_str)
            }
            Expr::ComptimeExpr(e) => format!("(comptime {})", self.generate_expr(e)),
            Expr::Comptime(b) => {
                let saved = self.current_case_content.clone();
                let saved_sm = self.is_state_machine;
                let saved_comptime = self.is_comptime;
                self.is_state_machine = false;
                self.is_comptime = true;
                self.current_case_content.clear();
                let len = b.body.len();
                for (idx, s) in b.body.iter().enumerate() {
                    if idx + 1 == len {
                        if let Stmt::Expr(e) = s {
                            let ec = self.generate_expr(e);
                            self.write_to_case(&format!("break :blk {};", ec));
                        } else {
                            self.generate_stmt(s);
                        }
                    } else {
                        self.generate_stmt(s);
                    }
                }
                let body_str = self.current_case_content.clone();
                self.current_case_content = saved;
                self.is_state_machine = saved_sm;
                self.is_comptime = saved_comptime;
                format!("comptime (blk: {{\n{}\n}})", body_str)
            }
            Expr::Arena(b) => {
                let arena_id = self.tmp_counter;
                self.tmp_counter += 1;
                let blk_label = format!("blk_arena_{}", arena_id);
                let arena_var = format!("__arena_{}", arena_id);
                let alloc_var = format!("__arena_alloc_{}", arena_id);
                let saved = self.current_case_content.clone();
                self.current_case_content.clear();
                self.write_to_case(&format!(
                    "var {} = std.heap.ArenaAllocator.init(std.heap.page_allocator);",
                    arena_var
                ));
                self.write_to_case(&format!("defer {}.deinit();", arena_var));
                self.write_to_case(&format!(
                    "const {} = {}.allocator(); _ = &{};",
                    alloc_var, arena_var, alloc_var
                ));
                self.arena_allocator_stack.push(alloc_var);
                let len = b.body.len();
                if len == 0 {
                    self.write_to_case(&format!("break :{} {{}};", blk_label));
                } else {
                    for (idx, s) in b.body.iter().enumerate() {
                        if idx + 1 == len {
                            if let Stmt::Expr(e) = s {
                                let ec = self.generate_expr(e);
                                self.write_to_case(&format!("break :{} {};", blk_label, ec));
                            } else {
                                self.generate_stmt(s);
                                self.write_to_case(&format!("break :{} {{}};", blk_label));
                            }
                        } else {
                            self.generate_stmt(s);
                        }
                    }
                }
                self.arena_allocator_stack.pop();
                let body_str = self.current_case_content.clone();
                self.current_case_content = saved;
                format!("({}: {{\n{}\n}})", blk_label, body_str)
            }
            _ => unreachable!(),
        }
    }
}
