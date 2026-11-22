use super::super::*;
use crate::ast::*;
use crate::Span;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

impl Codegen {
    pub(crate) fn generate_expr_call(&mut self, expr: &Expr) -> String {
        match expr {
            Expr::Call(call) => {
                if let Expr::Member(mem) = &*call.callee {
                    let prop = mem.prop.sym.as_str();

                    if (prop == "write" || prop == "print" || prop == "writeAll")
                        && call.args.len() == 1
                    {
                        if let Expr::Template(t) = &call.args[0] {
                            let (pattern, args) = self.lower_template_stream(t);
                            let args_joined = if args.is_empty() {
                                String::new()
                            } else {
                                format!(" {}", args.join(", "))
                            };
                            let obj = self.generate_expr(&mem.obj);
                            return format!("{}.print(\"{}\", .{{{}}})", obj, pattern, args_joined);
                        }
                    }

                    if let Expr::Ident(id) = &*mem.obj {
                        if id.sym == "reflect" {
                            if prop == "hasMethod" && call.args.len() == 2 {
                                let target = self.generate_expr(&call.args[0]);
                                let name = self.generate_expr(&call.args[1]);
                                return format!("@hasDecl({}, {})", target, name);
                            }
                            if prop == "hasField" && call.args.len() == 2 {
                                let target = self.generate_expr(&call.args[0]);
                                let name = self.generate_expr(&call.args[1]);
                                return format!("@hasField({}, {})", target, name);
                            }
                            if prop == "has" && call.args.len() == 2 {
                                let target = self.generate_expr(&call.args[0]);
                                let name = self.generate_expr(&call.args[1]);
                                return format!(
                                    "(@hasDecl({0}, {1}) or @hasField({0}, {1}))",
                                    target, name
                                );
                            }
                        }
                        if (id.sym == "string" || id.sym == "String")
                            && prop == "from"
                            && call.args.len() == 1
                        {
                            let arg = self.generate_expr(&call.args[0]);
                            return format!("cp_string_from({})", arg);
                        }
                        if id.sym == "JSON" {
                            if (prop == "stringify" || prop == "encode") && !call.args.is_empty() {
                                let arg = self.generate_expr(&call.args[0]);
                                return format!(
                                    "(blk: {{ const __jv = {}; break :blk std.fmt.allocPrint(std.heap.smp_allocator, \"{{f}}\", .{{std.json.fmt(if (@typeInfo(@TypeOf(__jv)) == .pointer and @typeInfo(@TypeOf(__jv)).pointer.size == .one) __jv.* else __jv, .{{}})}}) catch unreachable; }})",
                                    arg
                                );
                            }
                            if (prop == "parse" || prop == "decode") && !call.args.is_empty() {
                                let (is_type_arg, zig_ty) = match call.args.first() {
                                    Some(Expr::TypeVal { ty, .. }) => (true, self.map_type(ty)),
                                    Some(Expr::Ident(ident)) => {
                                        let is_type = matches!(
                                            ident.sym.as_str(),
                                            "i8" | "u8"
                                                | "i16"
                                                | "u16"
                                                | "i32"
                                                | "u32"
                                                | "i64"
                                                | "u64"
                                                | "i128"
                                                | "u128"
                                                | "isize"
                                                | "usize"
                                                | "f32"
                                                | "f64"
                                                | "bool"
                                                | "string"
                                        ) || self.class_defs.contains_key(&ident.sym)
                                            || self.enum_defs.contains_key(&ident.sym)
                                            || self.trait_defs.contains_key(&ident.sym)
                                            || (!self.locals.contains_key(&ident.sym)
                                                && !self.globals.contains(&ident.sym)
                                                && ident
                                                    .sym
                                                    .chars()
                                                    .next()
                                                    .map_or(false, |c| c.is_ascii_uppercase()));
                                        if is_type {
                                            (true, self.map_type_name(&ident.sym))
                                        } else {
                                            (false, String::new())
                                        }
                                    }
                                    _ => (false, String::new()),
                                };

                                if is_type_arg {
                                    if call.args.len() >= 3 {
                                        let alloc_arg = self.generate_expr(&call.args[1]);
                                        let str_arg = self.generate_expr(&call.args[2]);
                                        return format!(
                                            "std.json.parseFromSliceLeaky({}, {}, {}, .{{ .ignore_unknown_fields = true, .allocate = .alloc_if_needed }})",
                                            zig_ty, alloc_arg, str_arg
                                        );
                                    } else if call.args.len() >= 2 {
                                        let str_arg = self.generate_expr(&call.args[1]);
                                        return format!(
                                            "std.json.parseFromSliceLeaky({}, std.heap.smp_allocator, {}, .{{ .ignore_unknown_fields = true, .allocate = .alloc_if_needed }})",
                                            zig_ty, str_arg
                                        );
                                    } else {
                                        return format!(
                                            "std.json.parseFromSliceLeaky({}, std.heap.smp_allocator, \"\", .{{ .ignore_unknown_fields = true, .allocate = .alloc_if_needed }})",
                                            zig_ty
                                        );
                                    }
                                } else {
                                    if call.args.len() >= 2 {
                                        let alloc_arg = self.generate_expr(&call.args[0]);
                                        let str_arg = self.generate_expr(&call.args[1]);
                                        return format!("JSON.parse({}, {})", alloc_arg, str_arg);
                                    } else {
                                        let str_arg = self.generate_expr(&call.args[0]);
                                        return format!(
                                            "JSON.parse(std.heap.smp_allocator, {})",
                                            str_arg
                                        );
                                    }
                                }
                            }
                        }
                        let is_enum = self.enum_defs.contains_key(&id.sym)
                            || (!self.locals.contains_key(&id.sym)
                                && !self.global_var_types.contains_key(&id.sym)
                                && id
                                    .sym
                                    .chars()
                                    .next()
                                    .map_or(false, |c| c.is_ascii_uppercase())
                                && !self.class_defs.contains_key(&id.sym));
                        let is_custom_method = self
                            .enum_defs
                            .get(&id.sym)
                            .map_or(false, |ed| ed.methods.iter().any(|m| m.ident.sym == prop));
                        if is_enum
                            && !is_custom_method
                            && (prop == "get" || prop == "fromName" || prop == "fromString")
                            && call.args.len() == 1
                        {
                            let arg = self.generate_expr(&call.args[0]);
                            return format!("{}.get({})", id.sym, arg);
                        }
                        if is_enum && prop == "has" && call.args.len() == 1 {
                            let arg = self.generate_expr(&call.args[0]);
                            return format!("{}.has({})", id.sym, arg);
                        }
                        if id.sym == "Managed" {
                            let mut args_code = Vec::new();
                            for a in call.args.iter() {
                                let mut gen_arg = self.generate_expr(a);
                                let arg_ty = self.infer_cp_type(a);
                                let mut ty = arg_ty.clone();
                                if let Some(Type::BorrowType { inner, .. }) = ty {
                                    ty = Some(*inner);
                                }
                                if let Some(Type::Ref(id)) = ty {
                                    if id.sym == "Managed"
                                        && !gen_arg.starts_with('&')
                                        && !gen_arg.starts_with('*')
                                    {
                                        gen_arg = format!("&{}", gen_arg);
                                    }
                                }
                                args_code.push(gen_arg);
                            }
                            return format!("{}.{}({})", id.sym, prop, args_code.join(", "));
                        }
                        if !self.has_imported_ptr
                            && id.sym == "Shared"
                            && prop == "new"
                            && !call.args.is_empty()
                        {
                            let arg = self.generate_expr(&call.args[0]);
                            return format!(
                                "(blk: {{ const _val = {}; const _T = @TypeOf(_val); if (comptime @typeInfo(_T) == .pointer) break :blk _val else {{ const _p = std.heap.page_allocator.create(_T) catch unreachable; _p.* = _val; break :blk _p; }} }})",
                                arg
                            );
                        }
                        if id.sym == "Mutex" && prop == "new" {
                            if call.args.is_empty() {
                                return "CpBareMutex{}".to_string();
                            } else {
                                let arg = self.generate_expr(&call.args[0]);
                                return format!("cp_make_mutex({})", arg);
                            }
                        }
                        if id.sym == "RwLock" && prop == "new" {
                            if call.args.is_empty() {
                                return "CpBareRwLock{}".to_string();
                            } else {
                                let arg = self.generate_expr(&call.args[0]);
                                return format!("cp_make_rwlock({})", arg);
                            }
                        }
                        if id.sym == "Atomic" && prop == "new" {
                            if call.args.is_empty() {
                                return "cp_make_atomic(@as(i32, 0))".to_string();
                            } else {
                                let arg = self.generate_expr(&call.args[0]);
                                return format!("cp_make_atomic({})", arg);
                            }
                        }
                    }

                    let mut raw_obj_ty = self.infer_cp_type(&mem.obj);
                    loop {
                        match raw_obj_ty {
                            Some(Type::Optional(inner))
                            | Some(Type::BorrowType { inner, .. })
                            | Some(Type::Shared(inner))
                            | Some(Type::RawPtr { inner, .. }) => {
                                raw_obj_ty = Some(*inner);
                            }
                            _ => break,
                        }
                    }

                    if (prop == "withLock" || prop == "withRead" || prop == "withWrite")
                        && !call.args.is_empty()
                    {
                        let obj_code = self.generate_expr(&mem.obj);
                        let blk_id = self.lambda_counter;
                        self.lambda_counter += 1;
                        let (lock_fn, unlock_fn) = match prop {
                            "withLock" => ("lock", "unlock"),
                            "withRead" => ("lockShared", "unlockShared"),
                            "withWrite" => ("lockExclusive", "unlockExclusive"),
                            _ => unreachable!(),
                        };
                        if let Expr::Arrow(arrow) = &call.args[0] {
                            let mut param_bind = String::new();
                            let mut saved_local = None;
                            let mut saved_cp_ty = None;
                            let mut param_ident = None;
                            if !arrow.params.is_empty() {
                                if let Pattern::Ident(pid) = &arrow.params[0] {
                                    let pname = pid.sym.clone();
                                    param_ident = Some(pname.clone());
                                    self.inline_closure_params.insert(pname.clone());
                                    param_bind = format!(
                                        "    const {0} = &(_target.data); _ = &{0};\n",
                                        pname
                                    );
                                    saved_local = self.locals.get(&pname).cloned();
                                    saved_cp_ty = self.var_cp_types.get(&pname).cloned();
                                    if let Some(Type::Ref(id)) = &raw_obj_ty {
                                        let inner_sym = if id.sym.starts_with("Mutex<") {
                                            &id.sym[6..id.sym.len() - 1]
                                        } else if id.sym.starts_with("RwLock<") {
                                            &id.sym[7..id.sym.len() - 1]
                                        } else {
                                            ""
                                        };
                                        if !inner_sym.is_empty() {
                                            self.locals
                                                .insert(pname.clone(), format!("*{}", inner_sym));
                                            self.var_cp_types.insert(
                                                pname.clone(),
                                                Type::BorrowType {
                                                    inner: Box::new(Self::type_from_name_static(
                                                        inner_sym,
                                                    )),
                                                    is_mut: prop != "withRead",
                                                    lifetimes: vec![],
                                                },
                                            );
                                        }
                                    }
                                }
                            }

                            let res = match &*arrow.body {
                                ArrowBody::Expr(e) => {
                                    let body_expr = self.generate_expr(e);
                                    format!(
                                        "(blk_sync_{0}: {{\n    const _target = if (comptime @typeInfo(@TypeOf({1})) == .pointer) ({1}) else &({1});\n    _target.raw.{2}();\n    defer _target.raw.{3}();\n{4}    break :blk_sync_{0} {5};\n}})",
                                        blk_id, obj_code, lock_fn, unlock_fn, param_bind, body_expr
                                    )
                                }
                                ArrowBody::Block(b) => {
                                    let old_blk = self.inline_closure_blk.clone();
                                    self.inline_closure_blk = Some(format!("blk_sync_{}", blk_id));
                                    let saved_content = self.current_case_content.clone();
                                    self.current_case_content.clear();

                                    for s in &b.body {
                                        self.generate_stmt(s);
                                    }
                                    if !Self::block_returns_value(&b.body) {
                                        self.write_to_case(&format!(
                                            "break :blk_sync_{} {{}};",
                                            blk_id
                                        ));
                                    }

                                    let body_stmts = self.current_case_content.clone();
                                    self.current_case_content = saved_content;
                                    self.inline_closure_blk = old_blk;

                                    format!(
                                        "(blk_sync_{0}: {{\n    const _target = if (comptime @typeInfo(@TypeOf({1})) == .pointer) ({1}) else &({1});\n    _target.raw.{2}();\n    defer _target.raw.{3}();\n{4}{5}\n}})",
                                        blk_id, obj_code, lock_fn, unlock_fn, param_bind, body_stmts
                                    )
                                }
                            };

                            if let Some(pname) = param_ident {
                                self.inline_closure_params.remove(&pname);
                                if let Some(l) = saved_local {
                                    self.locals.insert(pname.clone(), l);
                                } else {
                                    self.locals.remove(&pname);
                                }
                                if let Some(t) = saved_cp_ty {
                                    self.var_cp_types.insert(pname, t);
                                } else {
                                    self.var_cp_types.remove(&pname);
                                }
                            }
                            return res;
                        }
                    }

                    let is_sync_type = matches!(&raw_obj_ty, Some(Type::Ref(id)) if id.sym == "Mutex" || id.sym.starts_with("Mutex<") || id.sym == "RwLock" || id.sym.starts_with("RwLock<") || id.sym == "Atomic" || id.sym.starts_with("Atomic<") || id.sym.starts_with("LockGuard<") || id.sym.starts_with("ReadGuard<") || id.sym.starts_with("WriteGuard<"));
                    if is_sync_type
                        || (matches!(prop, "cas" | "compareAndSwap") && call.args.len() == 2)
                        || (prop == "testAndSet" && call.args.len() <= 1)
                        || (matches!(
                            prop,
                            "tryLock"
                                | "isLocked"
                                | "tryRead"
                                | "tryWrite"
                                | "isReadLocked"
                                | "isWriteLocked"
                        ) && call.args.is_empty())
                        || (prop == "release"
                            && call.args.is_empty()
                            && self.is_builtin_raii_type(raw_obj_ty.as_ref()))
                    {
                        let obj_raw = self.generate_expr(&mem.obj);
                        let obj_ty = self.infer_zig_type(None, Some(&mem.obj));
                        let obj = if obj_ty.starts_with('?') {
                            format!("({}).?", obj_raw)
                        } else {
                            obj_raw
                        };
                        if prop == "and" || prop == "or" || prop == "xor" {
                            let arg = self.generate_expr(&call.args[0]);
                            return format!("(if (comptime @typeInfo(@TypeOf({0})) == .pointer) ({0}) else &({0})).@\"{1}\"({2})", obj, prop, arg);
                        }
                        if prop == "cas" || prop == "compareAndSwap" {
                            let arg0 = self.generate_expr(&call.args[0]);
                            let arg1 = self.generate_expr(&call.args[1]);
                            return format!("(if (comptime @typeInfo(@TypeOf({0})) == .pointer) ({0}) else &({0})).cas({1}, {2})", obj, arg0, arg1);
                        }
                        if matches!(
                            prop,
                            "load"
                                | "store"
                                | "swap"
                                | "add"
                                | "sub"
                                | "inc"
                                | "dec"
                                | "testAndSet"
                                | "clear"
                        ) {
                            let mut args_code = Vec::new();
                            for a in &call.args {
                                args_code.push(self.generate_expr(a));
                            }
                            return format!("(if (comptime @typeInfo(@TypeOf({0})) == .pointer) ({0}) else &({0})).{1}({2})", obj, prop, args_code.join(", "));
                        }
                        if matches!(
                            prop,
                            "lock"
                                | "unlock"
                                | "tryLock"
                                | "isLocked"
                                | "read"
                                | "write"
                                | "tryRead"
                                | "tryWrite"
                                | "isReadLocked"
                                | "isWriteLocked"
                                | "release"
                        ) && call.args.is_empty()
                        {
                            let raw_call = format!(
                                "(if (comptime @typeInfo(@TypeOf({0})) == .pointer) ({0}) else &({0})).{1}()",
                                obj, prop
                            );
                            if prop == "release" && !self.is_state_machine {
                                if let Expr::Ident(id) = &*mem.obj {
                                    if self.drop_flag_vars.contains(&id.sym)
                                        && !self.is_refcounted_type(self.var_cp_types.get(&id.sym))
                                    {
                                        return format!(
                                            "(blk: {{ if (!__{}_moved) {{ __{}_moved = true; {}; }} break :blk {{}}; }})",
                                            id.sym, id.sym, raw_call
                                        );
                                    }
                                }
                            }
                            return raw_call;
                        }
                    }

                    let _is_builtin_container = matches!(
                        raw_obj_ty,
                        Some(Type::Ref(ref id)) if id.sym == "BinaryHeap" || id.sym == "VecDeque" || id.sym == "BTreeMap" || id.sym == "BTreeSet" || id.sym == "Map" || id.sym == "Set" || id.sym == "HashMap" || id.sym == "HashSet"
                    );
                    let is_array_type = matches!(raw_obj_ty, Some(Type::Array(_)));
                    if prop == "push" && is_array_type {
                        self.check_mutation_allowed(&mem.obj, "container method 'push'");
                        let obj = self.generate_expr(&mem.obj);
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("cp_array_push(&{}, {})", obj, arg);
                    } else if prop == "pop" && is_array_type {
                        self.check_mutation_allowed(&mem.obj, "container method 'pop'");
                        let obj = self.generate_expr(&mem.obj);
                        return format!("cp_array_pop(&{})", obj);
                    } else if prop == "clear" && is_array_type {
                        self.check_mutation_allowed(&mem.obj, "container method 'clear'");
                        let obj = self.generate_expr(&mem.obj);
                        return format!("cp_container_clear(&{})", obj);
                    } else if _is_builtin_container
                        && (prop == "push"
                            || prop == "push_back"
                            || prop == "push_front"
                            || prop == "pop"
                            || prop == "pop_back"
                            || prop == "pop_front"
                            || prop == "clear"
                            || prop == "set"
                            || prop == "put_all"
                            || prop == "add"
                            || prop == "delete"
                            || prop == "insert"
                            || prop == "remove")
                    {
                        self.check_mutation_allowed(
                            &mem.obj,
                            &format!("container method '{}'", prop),
                        );
                        let obj = self.generate_expr(&mem.obj);
                        let mut args_code = Vec::new();
                        for a in &call.args {
                            args_code.push(self.generate_expr(a));
                        }
                        let mut type_to_lookup = None;
                        if let Some(ty) = self.infer_cp_type(&mem.obj) {
                            type_to_lookup = Some(self.map_type(&ty));
                        }
                        if type_to_lookup.is_none() {
                            type_to_lookup = Some(obj.clone());
                        }
                        if let Some(target_type) = type_to_lookup {
                            if let Some((params, _, _)) = self.lookup_method(&target_type, prop) {
                                let non_recv_params: Vec<Param> = params
                                    .into_iter()
                                    .filter(|p| {
                                        p.ident.sym != "this"
                                            && p.ident.sym != "_"
                                            && p.ident.sym != "self"
                                    })
                                    .collect();
                                if call.args.len() < non_recv_params.len() {
                                    for p in &non_recv_params[call.args.len()..] {
                                        if let Some(def) = &p.default {
                                            args_code.push(self.generate_expr(def));
                                        }
                                    }
                                }
                            }
                        }
                        return format!("{}.{}({})", obj, prop, args_code.join(", "));
                    } else if prop == "get"
                        || prop == "has"
                        || prop == "keys"
                        || prop == "values"
                        || prop == "entries"
                        || prop == "peek"
                        || prop == "peek_front"
                        || prop == "peek_back"
                        || prop == "len"
                        || prop == "is_empty"
                        || prop == "min_key"
                        || prop == "max_key"
                        || prop == "min"
                        || prop == "max"
                    {
                        self.check_read_allowed(&mem.obj);
                        let obj = self.generate_expr(&mem.obj);
                        let mut args_code = Vec::new();
                        for a in &call.args {
                            args_code.push(self.generate_expr(a));
                        }
                        let mut type_to_lookup = None;
                        if let Some(ty) = self.infer_cp_type(&mem.obj) {
                            type_to_lookup = Some(self.map_type(&ty));
                        }
                        if type_to_lookup.is_none() {
                            type_to_lookup = Some(obj.clone());
                        }
                        if let Some(target_type) = type_to_lookup {
                            if let Some((params, _, _)) = self.lookup_method(&target_type, prop) {
                                let non_recv_params: Vec<Param> = params
                                    .into_iter()
                                    .filter(|p| {
                                        p.ident.sym != "this"
                                            && p.ident.sym != "_"
                                            && p.ident.sym != "self"
                                    })
                                    .collect();
                                if call.args.len() < non_recv_params.len() {
                                    for p in &non_recv_params[call.args.len()..] {
                                        if let Some(def) = &p.default {
                                            args_code.push(self.generate_expr(def));
                                        }
                                    }
                                }
                            }
                        }
                        return format!("{}.{}({})", obj, prop, args_code.join(", "));
                    } else if prop == "indexOf" && call.args.len() == 1 {
                        self.check_read_allowed(&mem.obj);
                        let obj = self.generate_expr(&mem.obj);
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("cp_index_of({}, {})", obj, arg);
                    } else if prop == "lastIndexOf" && call.args.len() == 1 {
                        self.check_read_allowed(&mem.obj);
                        let obj = self.generate_expr(&mem.obj);
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("cp_last_index_of({}, {})", obj, arg);
                    } else if prop == "includes" && call.args.len() == 1 {
                        self.check_read_allowed(&mem.obj);
                        let obj = self.generate_expr(&mem.obj);
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("cp_includes({}, {})", obj, arg);
                    } else if prop == "startsWith" && call.args.len() == 1 {
                        self.check_read_allowed(&mem.obj);
                        let obj = self.generate_expr(&mem.obj);
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("@import(\"std\").mem.startsWith(u8, {}, {})", obj, arg);
                    } else if prop == "endsWith" && call.args.len() == 1 {
                        self.check_read_allowed(&mem.obj);
                        let obj = self.generate_expr(&mem.obj);
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("@import(\"std\").mem.endsWith(u8, {}, {})", obj, arg);
                    } else if prop == "split"
                        && call.args.len() == 1
                        && matches!(
                            &raw_obj_ty,
                            Some(Type::Array(_)) | Some(Type::FixedArray(_, _))
                        )
                        && (matches!(&call.args[0], Expr::Lit(Lit::Int(_)))
                            || matches!(
                                self.infer_cp_type(&call.args[0]),
                                Some(
                                    Type::U8
                                        | Type::I8
                                        | Type::U16
                                        | Type::I16
                                        | Type::I32
                                        | Type::U32
                                        | Type::I64
                                        | Type::U64
                                        | Type::I128
                                        | Type::U128
                                        | Type::ISize
                                        | Type::USize
                                )
                            ))
                    {
                        self.check_read_allowed(&mem.obj);
                        let orig_ty = self.infer_cp_type(&mem.obj);
                        let obj = self.generate_expr(&mem.obj);
                        let mid = self.generate_expr(&call.args[0]);
                        let s_arg = if matches!(orig_ty, Some(Type::FixedArray(_, _))) {
                            format!("&({})", obj)
                        } else {
                            obj
                        };
                        return format!("__cp_split_at_mut({}, {})", s_arg, mid);
                    } else if (prop == "slice" || prop == "substring")
                        && !call.args.is_empty()
                        && call.args.len() <= 2
                        && {
                            let is_sliceable = match &raw_obj_ty {
                                Some(Type::Array(_)) | Some(Type::SentinelSlice(_, _)) => true,
                                Some(Type::Ref(ref id)) => {
                                    id.sym == "string"
                                        || id.sym == "String"
                                        || id.sym == "Array"
                                        || id.sym == "Vec"
                                        || id.sym.starts_with("[]")
                                        || id.sym.starts_with("[*]")
                                }
                                Some(Type::BorrowType { inner, .. }) => match &**inner {
                                    Type::Array(_) | Type::SentinelSlice(_, _) => true,
                                    Type::Ref(ref id) => {
                                        id.sym == "string"
                                            || id.sym == "String"
                                            || id.sym == "Array"
                                            || id.sym == "Vec"
                                            || id.sym.starts_with("[]")
                                            || id.sym.starts_with("[*]")
                                    }
                                    _ => false,
                                },
                                _ => false,
                            };
                            is_sliceable
                        }
                    {
                        self.check_read_allowed(&mem.obj);
                        let obj = self.generate_expr(&mem.obj);
                        let start = self.generate_expr(&call.args[0]);
                        if call.args.len() > 1 {
                            let end = self.generate_expr(&call.args[1]);
                            return format!(
                                "cp_iter_slice({})[@intCast({})..@intCast({})]",
                                obj, start, end
                            );
                        } else {
                            return format!("cp_iter_slice({})[@intCast({})..]", obj, start);
                        }
                    } else if prop == "toLowerCase" && call.args.is_empty() {
                        let obj = self.generate_expr(&mem.obj);
                        return format!("(blk: {{ var out = {}.alloc(u8, {}.len) catch unreachable; for ({}, 0..) |c, i| out[i] = @import(\"std\").ascii.toLower(c); break :blk out; }})", self.current_allocator(), obj, obj);
                    } else if prop == "toUpperCase" && call.args.is_empty() {
                        let obj = self.generate_expr(&mem.obj);
                        return format!("(blk: {{ var out = {}.alloc(u8, {}.len) catch unreachable; for ({}, 0..) |c, i| out[i] = @import(\"std\").ascii.toUpper(c); break :blk out; }})", self.current_allocator(), obj, obj);
                    } else if prop == "replace" && call.args.len() == 2 {
                        let obj = self.generate_expr(&mem.obj);
                        let arg = self.generate_expr(&call.args[0]);
                        let replacement = self.generate_expr(&call.args[1]);
                        return format!("@import(\"std\").mem.replaceOwned(u8, {}, {}, {}, {}) catch unreachable", self.current_allocator(), obj, arg, replacement);
                    } else if prop == "charCodeAt" && !call.args.is_empty() {
                        let obj = self.generate_expr(&mem.obj);
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("@as(i32, @intCast({}[@intCast({})]))", obj, arg);
                    } else if prop == "match" && call.args.len() == 1 {
                        let is_class_or_enum = match &*mem.obj {
                            Expr::Ident(id) => {
                                self.class_defs.contains_key(&id.sym)
                                    || self.enum_defs.contains_key(&id.sym)
                                    || id.sym.chars().next().map_or(false, |c| c.is_uppercase())
                            }
                            _ => false,
                        };
                        if !is_class_or_enum {
                            let obj = self.generate_expr(&mem.obj);
                            let arg = self.generate_expr(&call.args[0]);
                            return format!("{}.exec({})", arg, obj);
                        }
                    } else if prop == "test" && !call.args.is_empty() {
                        let obj = self.generate_expr(&mem.obj);
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("{}.@\"test\"({})", obj, arg);
                    } else if prop == "exec" && !call.args.is_empty() {
                        let obj = self.generate_expr(&mem.obj);
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("{}.exec({})", obj, arg);
                    } else if prop == "toString" && call.args.is_empty() {
                        let obj = self.generate_expr(&mem.obj);
                        if let Some(Type::Ref(id)) = self.infer_cp_type(&mem.obj) {
                            if self.enum_defs.contains_key(&id.sym) {
                                return format!("@tagName({})", obj);
                            }
                        }
                        return format!("cp_string_from({})", obj);
                    }
                }

                if let Expr::Member(mem) = &*call.callee {
                    if let Expr::Ident(id) = &*mem.obj {
                        if (id.sym == "string" || id.sym == "String")
                            && mem.prop.sym == "from"
                            && call.args.len() == 1
                        {
                            let arg_code = self.generate_expr(&call.args[0]);
                            return format!("cp_string_from({})", arg_code);
                        }
                        if let Some(ed) = self.enum_defs.get(&id.sym) {
                            let is_method = ed.methods.iter().any(|m| m.ident.sym == mem.prop.sym)
                                || ed
                                    .extra_zig
                                    .iter()
                                    .any(|z| z.contains(&format!("fn {}(", mem.prop.sym)));
                            let is_variant =
                                ed.variants.iter().any(|v| v.ident.sym == mem.prop.sym);
                            if is_method || !is_variant {
                                let mut args_code = Vec::new();
                                for a in &call.args {
                                    args_code.push(self.generate_expr(a));
                                }
                                return format!(
                                    "{}.{}({})",
                                    id.sym,
                                    mem.prop.sym,
                                    args_code.join(", ")
                                );
                            }
                            let arg_code = if !call.args.is_empty() {
                                self.generate_expr(&call.args[0])
                            } else {
                                "{}".to_string()
                            };
                            return format!("{}{{ .{} = {} }}", id.sym, mem.prop.sym, arg_code);
                        }
                        if id.sym == "Math" {
                            let prop = mem.prop.sym.as_str();
                            if prop == "abs" {
                                return format!("@abs({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "ceil" {
                                return format!("@ceil({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "floor" {
                                return format!("@floor({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "round" {
                                return format!("@round({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "trunc" {
                                return format!("@trunc({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "max" {
                                return format!(
                                    "@max({}, {})",
                                    self.generate_expr(&call.args[0]),
                                    self.generate_expr(&call.args[1])
                                );
                            }
                            if prop == "min" {
                                return format!(
                                    "@min({}, {})",
                                    self.generate_expr(&call.args[0]),
                                    self.generate_expr(&call.args[1])
                                );
                            }
                            if prop == "sqrt" {
                                return format!("@sqrt({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "sin" {
                                return format!("@sin({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "cos" {
                                return format!("@cos({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "tan" {
                                return format!("@tan({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "exp" {
                                return format!("@exp({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "log" {
                                return format!("@log({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "log10" {
                                return format!("@log10({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "log2" {
                                return format!("@log2({})", self.generate_expr(&call.args[0]));
                            }
                            if prop == "pow" {
                                return format!(
                                    "@import(\"std\").math.pow(f32, {}, {})",
                                    self.generate_expr(&call.args[0]),
                                    self.generate_expr(&call.args[1])
                                );
                            }
                            if prop == "asin" {
                                return format!(
                                    "@import(\"std\").math.asin({})",
                                    self.generate_expr(&call.args[0])
                                );
                            }
                            if prop == "acos" {
                                return format!(
                                    "@import(\"std\").math.acos({})",
                                    self.generate_expr(&call.args[0])
                                );
                            }
                            if prop == "atan" {
                                return format!(
                                    "@import(\"std\").math.atan({})",
                                    self.generate_expr(&call.args[0])
                                );
                            }
                            if prop == "atan2" {
                                return format!(
                                    "@import(\"std\").math.atan2({}, {})",
                                    self.generate_expr(&call.args[0]),
                                    self.generate_expr(&call.args[1])
                                );
                            }
                            if prop == "random" {
                                return "cp_math_random()".to_string();
                            }
                        }

                        // 1. Handle `super.method(...)` in single-inheritance subclass
                        if id.sym == "super" {
                            let c_name = self.current_class_name.clone().expect(
                                "CP Compile Error: 'super' can only be used inside a class method",
                            );
                            let super_method_name = format!("__super_{}", mem.prop.sym);
                            if let Some((_params, ret_ty, target_recv_mode)) =
                                self.lookup_method(&c_name, &super_method_name)
                            {
                                if self.moved_vars.contains("this") || self.moved_vars.contains("_")
                                {
                                    panic!("CP Compile Error: Use of moved value '_'");
                                }
                                match target_recv_mode {
                                    ReceiverMode::Mut => {
                                        if self.current_receiver_mode == ReceiverMode::ReadOnly {
                                            panic!(
                                                "CP Compile Error: Cannot call mutable parent method 'super.{}()' from a read-only method (add '*_' as first parameter)",
                                                mem.prop.sym
                                            );
                                        }
                                        if let Some(mutated_fields) = self
                                            .find_method_mutated_fields(&c_name, &super_method_name)
                                        {
                                            if mutated_fields.is_empty() {
                                                self.check_borrow_conflict("this", false);
                                                self.check_borrow_conflict("_", false);
                                            } else {
                                                for f in &mutated_fields {
                                                    self.check_borrow_conflict_field(
                                                        "this",
                                                        Some(f),
                                                        true,
                                                    );
                                                    self.check_borrow_conflict_field(
                                                        "_",
                                                        Some(f),
                                                        true,
                                                    );
                                                }
                                            }
                                        } else {
                                            self.check_borrow_conflict("this", true);
                                            self.check_borrow_conflict("_", true);
                                        }
                                    }
                                    ReceiverMode::Move => {
                                        if self.current_receiver_mode != ReceiverMode::Move {
                                            panic!(
                                                "CP Compile Error: Cannot call consuming parent method 'super.{}()' from a non-move method (add 'move _' as first parameter)",
                                                mem.prop.sym
                                            );
                                        }
                                        self.check_active_borrows_before_move("this");
                                        self.check_active_borrows_before_move("_");
                                    }
                                    ReceiverMode::ReadOnly => {
                                        self.check_borrow_conflict("this", false);
                                        self.check_borrow_conflict("_", false);
                                    }
                                    ReceiverMode::Static => {}
                                }
                                let mut args = String::new();
                                for (idx, arg) in call.args.iter().enumerate() {
                                    args.push_str(&self.generate_expr(arg));
                                    if idx < call.args.len() - 1 {
                                        args.push_str(", ");
                                    }
                                }
                                let self_str = if self.is_state_machine {
                                    "ctx.self"
                                } else {
                                    "self"
                                };
                                let call_res = if target_recv_mode == ReceiverMode::Move {
                                    self.moved_vars.insert("this".to_string());
                                    self.moved_vars.insert("_".to_string());
                                    if !self.is_state_machine {
                                        format!(
                                            "(blk: {{ __self_moved = true; break :blk {}.{}({}); }})",
                                            self_str, super_method_name, args
                                        )
                                    } else {
                                        format!("{}.{}({})", self_str, super_method_name, args)
                                    }
                                } else {
                                    format!("{}.{}({})", self_str, super_method_name, args)
                                };
                                return self.maybe_wrap_try_call(call_res, ret_ty.as_ref());
                            } else {
                                panic!(
                                    "CP Compile Error: Parent method 'super.{}()' not found in class '{}'",
                                    mem.prop.sym, c_name
                                );
                            }
                        }

                        // 2. Handle `_.method(...)` / `this.method(...)` or `obj.method(...)`
                        if let Some(type_name) = self.resolve_var_type_name(&id.sym) {
                            if let Some((params, ret_ty, target_recv_mode)) =
                                self.lookup_method(&type_name, &mem.prop.sym)
                            {
                                let is_self_recv = id.sym == "this" || id.sym == "_";
                                if is_self_recv {
                                    if self.moved_vars.contains("this")
                                        || self.moved_vars.contains("_")
                                    {
                                        panic!("CP Compile Error: Use of moved value '{}'", id.sym);
                                    }
                                    match target_recv_mode {
                                        ReceiverMode::Mut => {
                                            if self.current_receiver_mode == ReceiverMode::ReadOnly
                                            {
                                                panic!(
                                                    "CP Compile Error: Cannot call mutable method '{}.{}()' inside a read-only method (add '*_' as first parameter)",
                                                    id.sym, mem.prop.sym
                                                );
                                            }
                                            if let Some(mutated_fields) = self
                                                .find_method_mutated_fields(
                                                    &type_name,
                                                    &mem.prop.sym,
                                                )
                                            {
                                                if mutated_fields.is_empty() {
                                                    self.check_borrow_conflict("this", false);
                                                    self.check_borrow_conflict("_", false);
                                                } else {
                                                    for f in &mutated_fields {
                                                        self.check_borrow_conflict_field(
                                                            "this",
                                                            Some(f),
                                                            true,
                                                        );
                                                        self.check_borrow_conflict_field(
                                                            "_",
                                                            Some(f),
                                                            true,
                                                        );
                                                    }
                                                }
                                            } else {
                                                self.check_borrow_conflict("this", true);
                                                self.check_borrow_conflict("_", true);
                                            }
                                        }
                                        ReceiverMode::Move => {
                                            if self.current_receiver_mode != ReceiverMode::Move {
                                                panic!(
                                                    "CP Compile Error: Cannot call consuming 'move _' method '{}.{}()' inside a non-move method (add 'move _' as first parameter)",
                                                    id.sym, mem.prop.sym
                                                );
                                            }
                                            self.check_active_borrows_before_move("this");
                                            self.check_active_borrows_before_move("_");
                                        }
                                        ReceiverMode::ReadOnly => {
                                            self.check_borrow_conflict("this", false);
                                            self.check_borrow_conflict("_", false);
                                        }
                                        ReceiverMode::Static => {}
                                    }
                                } else {
                                    if self.moved_vars.contains(&id.sym) {
                                        panic!("CP Compile Error: Use of moved value '{}'", id.sym);
                                    }
                                    match target_recv_mode {
                                        ReceiverMode::Mut => {
                                            if let Some((_, is_mut)) =
                                                self.var_borrow_sources.get(&id.sym)
                                            {
                                                if !*is_mut {
                                                    panic!(
                                                        "CP Compile Error: Cannot call mutable method '{}()' on read-only borrow '{}'",
                                                        mem.prop.sym, id.sym
                                                    );
                                                }
                                            } else if self.current_fn_params.contains(&id.sym)
                                                && !self.mutable_fn_params.contains(&id.sym)
                                            {
                                                panic!(
                                                    "CP Compile Error: Cannot call mutable method '{}()' on read-only parameter '{}' (mark parameter as '{}: *T')",
                                                    mem.prop.sym, id.sym, id.sym
                                                );
                                            }
                                            if let Some(mutated_fields) = self
                                                .find_method_mutated_fields(
                                                    &type_name,
                                                    &mem.prop.sym,
                                                )
                                            {
                                                if mutated_fields.is_empty() {
                                                    self.check_borrow_conflict(&id.sym, false);
                                                } else {
                                                    for f in &mutated_fields {
                                                        self.check_borrow_conflict_field(
                                                            &id.sym,
                                                            Some(f),
                                                            true,
                                                        );
                                                    }
                                                }
                                            } else {
                                                self.check_borrow_conflict(&id.sym, true);
                                            }
                                        }
                                        ReceiverMode::Move => {
                                            if self.var_borrow_sources.contains_key(&id.sym) {
                                                panic!(
                                                    "CP Compile Error: Cannot call consuming 'move _' method '{}()' on borrowed reference '{}'",
                                                    mem.prop.sym, id.sym
                                                );
                                            }
                                            if self.current_fn_params.contains(&id.sym)
                                                && !self.move_fn_params.contains(&id.sym)
                                            {
                                                panic!(
                                                    "CP Compile Error: Cannot call consuming 'move _' method '{}()' on borrowed parameter '{}' (mark parameter as 'move {}')",
                                                    mem.prop.sym, id.sym, id.sym
                                                );
                                            }
                                            self.check_active_borrows_before_move(&id.sym);
                                        }
                                        ReceiverMode::ReadOnly => {
                                            self.check_borrow_conflict(&id.sym, false);
                                        }
                                        ReceiverMode::Static => {}
                                    }
                                }

                                let non_recv_params: Vec<Param> = params
                                    .into_iter()
                                    .filter(|p| {
                                        p.ident.sym != "this"
                                            && p.ident.sym != "_"
                                            && p.ident.sym != "self"
                                    })
                                    .collect();
                                let obj_code = self.generate_expr(&mem.obj);
                                let mut args = String::new();
                                for (idx, arg) in call.args.iter().enumerate() {
                                    let mut arg_is_move_param = false;
                                    let old_box = self.expected_box_trait.clone();
                                    if idx < non_recv_params.len() {
                                        let param = &non_recv_params[idx];
                                        if let Type::BoxDyn(tr_id) = &param.ty {
                                            self.expected_box_trait = Some(tr_id.sym.clone());
                                        }
                                        if let Type::Ref(t_id) = &param.ty {
                                            if self.trait_defs.contains_key(&t_id.sym) {
                                                if let Expr::Ident(arg_id) = arg {
                                                    if let Some(arg_cls) =
                                                        self.resolve_var_type_name(&arg_id.sym)
                                                    {
                                                        let implements = self
                                                            .class_defs
                                                            .get(&arg_cls)
                                                            .map_or(false, |c| {
                                                                c.implements
                                                                    .iter()
                                                                    .any(|i| i.sym == t_id.sym)
                                                            });
                                                        if !implements {
                                                            panic!(
                                                                "CP Compile Error: Type '{}' does not implement trait '{}'",
                                                                arg_cls, t_id.sym
                                                            );
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if let Expr::Ident(arg_id) = arg {
                                            if param.is_mut {
                                                if let Some((_, is_m)) =
                                                    self.var_borrow_sources.get(&arg_id.sym)
                                                {
                                                    if !*is_m {
                                                        panic!(
                                                            "CP Compile Error: Cannot pass read-only borrow '{}' to mutable parameter '{}'",
                                                            arg_id.sym, param.ident.sym
                                                        );
                                                    }
                                                } else if self
                                                    .current_fn_params
                                                    .contains(&arg_id.sym)
                                                    && !self.mutable_fn_params.contains(&arg_id.sym)
                                                {
                                                    panic!(
                                                        "CP Compile Error: Cannot pass read-only parameter '{}' to mutable parameter '{}'",
                                                        arg_id.sym, param.ident.sym
                                                    );
                                                }
                                                self.check_borrow_conflict(&arg_id.sym, true);
                                            } else if param.is_move {
                                                if self.var_borrow_sources.contains_key(&arg_id.sym)
                                                    || (self
                                                        .current_fn_params
                                                        .contains(&arg_id.sym)
                                                        && !self
                                                            .move_fn_params
                                                            .contains(&arg_id.sym))
                                                {
                                                    panic!(
                                                        "CP Compile Error: Cannot move borrowed reference or read-only parameter '{}' into consuming parameter '{}'",
                                                        arg_id.sym, param.ident.sym
                                                    );
                                                }
                                                self.check_active_borrows_before_move(&arg_id.sym);
                                                arg_is_move_param = true;
                                            } else {
                                                self.check_borrow_conflict(&arg_id.sym, false);
                                            }
                                        }
                                    }
                                    let mut gen_arg = self.generate_expr(arg);
                                    if gen_arg.starts_with("lambda_") {
                                        gen_arg = format!("@ptrCast(@constCast(&{}))", gen_arg);
                                    }
                                    self.expected_box_trait = old_box;
                                    if arg_is_move_param {
                                        if let Expr::Ident(arg_id) = arg {
                                            self.moved_vars.insert(arg_id.sym.clone());
                                            if !self.is_state_machine {
                                                args.push_str(&format!(
                                                    "(blk: {{ __{}_moved = true; break :blk {}; }})",
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

                                let prop_name =
                                    if matches!(mem.prop.sym.as_str(), "and" | "or" | "orelse") {
                                        format!("@\"{}\"", mem.prop.sym)
                                    } else {
                                        mem.prop.sym.clone()
                                    };
                                let is_explicit_dtor = mem.prop.sym == "__cp_deinit"
                                    || mem.prop.sym == "__cp_errdeinit";
                                let call_res = if target_recv_mode == ReceiverMode::Move {
                                    if is_self_recv {
                                        self.moved_vars.insert("this".to_string());
                                        self.moved_vars.insert("_".to_string());
                                        if !self.is_state_machine {
                                            format!(
                                                "(blk: {{ __self_moved = true; break :blk {}.{}({}); }})",
                                                obj_code, prop_name, args
                                            )
                                        } else {
                                            format!("{}.{}({})", obj_code, prop_name, args)
                                        }
                                    } else {
                                        self.moved_vars.insert(id.sym.clone());
                                        if !self.is_state_machine
                                            && self.drop_flag_vars.contains(&id.sym)
                                        {
                                            format!(
                                                "(blk: {{ __{}_moved = true; break :blk {}.{}({}); }})",
                                                id.sym, obj_code, prop_name, args
                                            )
                                        } else {
                                            format!("{}.{}({})", obj_code, prop_name, args)
                                        }
                                    }
                                } else if is_explicit_dtor
                                    && !is_self_recv
                                    && !self.is_state_machine
                                    && self.drop_flag_vars.contains(&id.sym)
                                {
                                    format!(
                                        "(blk: {{ if (!__{}_moved) {{ __{}_moved = true; {}.{}({}); }} break :blk {{}}; }})",
                                        id.sym, id.sym, obj_code, prop_name, args
                                    )
                                } else {
                                    format!("{}.{}({})", obj_code, prop_name, args)
                                };
                                return self.maybe_wrap_try_call(call_res, ret_ty.as_ref());
                            }
                        }
                    }
                }

                // 3. Check omitted-receiver sibling method call inside a class method (`helper()` -> `self.helper()`)
                if let Expr::Ident(ident) = &*call.callee {
                    if self.local_trampolines.contains(&ident.sym) {
                        let mut args = String::new();
                        for (idx, arg) in call.args.iter().enumerate() {
                            args.push_str(&self.generate_expr(arg));
                            if idx + 1 < call.args.len() {
                                args.push_str(", ");
                            }
                        }
                        return format!("__cp_trampoline_{}.{}({})", ident.sym, ident.sym, args);
                    }
                    if self.enum_defs.contains_key(&ident.sym) && call.args.len() == 1 {
                        let arg_code = self.generate_expr(&call.args[0]);
                        return format!("@as({}, @enumFromInt({}))", ident.sym, arg_code);
                    }
                    if (ident.sym == "string" || ident.sym == "String") && call.args.len() == 1 {
                        let arg_code = self.generate_expr(&call.args[0]);
                        if let Some(Type::Ref(id)) = self.infer_cp_type(&call.args[0]) {
                            if self.enum_defs.contains_key(&id.sym) {
                                return format!("@tagName({})", arg_code);
                            }
                        }
                        return format!("cp_string_from({})", arg_code);
                    }
                    let is_primitive = matches!(
                        ident.sym.as_str(),
                        "i8" | "u8"
                            | "i16"
                            | "u16"
                            | "i32"
                            | "u32"
                            | "i64"
                            | "u64"
                            | "i128"
                            | "u128"
                            | "isize"
                            | "usize"
                            | "f32"
                            | "f64"
                            | "bool"
                    );
                    if is_primitive && call.args.len() == 1 {
                        let target_ty = match ident.sym.as_str() {
                            "i8" => Type::I8,
                            "u8" => Type::U8,
                            "i16" => Type::I16,
                            "u16" => Type::U16,
                            "i32" => Type::I32,
                            "u32" => Type::U32,
                            "i64" => Type::I64,
                            "u64" => Type::U64,
                            "i128" => Type::I128,
                            "u128" => Type::U128,
                            "isize" => Type::ISize,
                            "usize" => Type::USize,
                            "f32" => Type::F32,
                            "f64" => Type::F64,
                            "bool" => Type::Bool,
                            _ => unreachable!(),
                        };
                        return self.generate_expr(&Expr::Cast {
                            expr: Box::new(call.args[0].clone()),
                            target_ty,
                            span: call.span,
                        });
                    }
                    if let Some(c_name) = self.current_class_name.clone() {
                        if self.current_receiver_mode != ReceiverMode::Static
                            && !self.locals.contains_key(&ident.sym)
                            && !self.fn_sigs.contains_key(&ident.sym)
                        {
                            if let Some((params, ret_ty, target_recv_mode)) =
                                self.lookup_method(&c_name, &ident.sym)
                            {
                                if self.moved_vars.contains("this") || self.moved_vars.contains("_")
                                {
                                    panic!("CP Compile Error: Use of moved value '_'");
                                }
                                match target_recv_mode {
                                    ReceiverMode::Mut => {
                                        if self.current_receiver_mode == ReceiverMode::ReadOnly {
                                            panic!(
                                                "CP Compile Error: Cannot call mutable method '{}()' inside a read-only method (add '*_' as first parameter)",
                                                ident.sym
                                            );
                                        }
                                        if let Some(mutated_fields) =
                                            self.find_method_mutated_fields(&c_name, &ident.sym)
                                        {
                                            if mutated_fields.is_empty() {
                                                self.check_borrow_conflict("this", false);
                                                self.check_borrow_conflict("_", false);
                                            } else {
                                                for f in &mutated_fields {
                                                    self.check_borrow_conflict_field(
                                                        "this",
                                                        Some(f),
                                                        true,
                                                    );
                                                    self.check_borrow_conflict_field(
                                                        "_",
                                                        Some(f),
                                                        true,
                                                    );
                                                }
                                            }
                                        } else {
                                            self.check_borrow_conflict("this", true);
                                            self.check_borrow_conflict("_", true);
                                        }
                                    }
                                    ReceiverMode::Move => {
                                        if self.current_receiver_mode != ReceiverMode::Move {
                                            panic!(
                                                "CP Compile Error: Cannot call consuming 'move _' method '{}()' inside a non-move method (add 'move _' as first parameter)",
                                                ident.sym
                                            );
                                        }
                                        self.check_active_borrows_before_move("this");
                                        self.check_active_borrows_before_move("_");
                                    }
                                    ReceiverMode::ReadOnly => {
                                        self.check_borrow_conflict("this", false);
                                        self.check_borrow_conflict("_", false);
                                    }
                                    ReceiverMode::Static => {}
                                }
                                let non_recv_params: Vec<Param> = params
                                    .into_iter()
                                    .filter(|p| {
                                        p.ident.sym != "this"
                                            && p.ident.sym != "_"
                                            && p.ident.sym != "self"
                                    })
                                    .collect();
                                let mut args = String::new();
                                for (idx, arg) in call.args.iter().enumerate() {
                                    let mut arg_is_move_param = false;
                                    let old_box = self.expected_box_trait.clone();
                                    if idx < non_recv_params.len() {
                                        let param = &non_recv_params[idx];
                                        if let Type::BoxDyn(tr_id) = &param.ty {
                                            self.expected_box_trait = Some(tr_id.sym.clone());
                                        }
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
                                    let mut gen_arg = self.generate_expr(arg);
                                    if gen_arg.starts_with("lambda_") {
                                        gen_arg = format!("@ptrCast(@constCast(&{}))", gen_arg);
                                    }
                                    self.expected_box_trait = old_box;
                                    if arg_is_move_param {
                                        if let Expr::Ident(arg_id) = arg {
                                            self.moved_vars.insert(arg_id.sym.clone());
                                            if !self.is_state_machine {
                                                args.push_str(&format!(
                                                    "(blk: {{ __{}_moved = true; break :blk {}; }})",
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
                                if target_recv_mode == ReceiverMode::Static {
                                    let call_res = format!("{}.{}({})", c_name, ident.sym, args);
                                    return self.maybe_wrap_try_call(call_res, ret_ty.as_ref());
                                }
                                let self_str = if self.is_state_machine {
                                    "ctx.self"
                                } else {
                                    "self"
                                };
                                let call_res = if target_recv_mode == ReceiverMode::Move {
                                    self.moved_vars.insert("this".to_string());
                                    self.moved_vars.insert("_".to_string());
                                    if !self.is_state_machine {
                                        format!(
                                            "(blk: {{ __self_moved = true; break :blk {}.{}({}); }})",
                                            self_str, ident.sym, args
                                        )
                                    } else {
                                        format!("{}.{}({})", self_str, ident.sym, args)
                                    }
                                } else {
                                    format!("{}.{}({})", self_str, ident.sym, args)
                                };
                                return self.maybe_wrap_try_call(call_res, ret_ty.as_ref());
                            }
                        }
                    }
                }

                let is_spawn_cpu = if let Expr::Member(mem) = &*call.callee {
                    if let Expr::Ident(obj_id) = &*mem.obj {
                        obj_id.sym == "spawn" && mem.prop.sym == "cpu"
                    } else {
                        false
                    }
                } else {
                    false
                };

                if let Expr::Ident(ident) = &*call.callee {
                    if (ident.sym == "print" || ident.sym == "println") && call.args.len() == 1 {
                        if let Expr::Template(t) = &call.args[0] {
                            let (pattern, args) = self.lower_template_stream(t);
                            let args_joined = if args.is_empty() {
                                String::new()
                            } else {
                                format!(" {}", args.join(", "))
                            };
                            return format!(
                                "std.debug.print(\"{}\\n\", .{{{}}})",
                                pattern, args_joined
                            );
                        }
                    }
                    if (ident.sym == "eprint" || ident.sym == "eprintln") && call.args.len() == 1 {
                        if let Expr::Template(t) = &call.args[0] {
                            let (pattern, args) = self.lower_template_stream(t);
                            let args_joined = if args.is_empty() {
                                String::new()
                            } else {
                                format!(" {}", args.join(", "))
                            };
                            let nl = if ident.sym == "eprintln" { "\\n" } else { "" };
                            return format!(
                                "std.debug.print(\"{}{}\", .{{{}}})",
                                pattern, nl, args_joined
                            );
                        }
                    }
                }

                let callee = if is_spawn_cpu {
                    "@task_spawn_cpu".to_string()
                } else {
                    match &*call.callee {
                        Expr::Ident(ident) => {
                            if self.is_state_machine
                                && self.locals.contains_key(&ident.sym)
                                && !self.inline_closure_params.contains(&ident.sym)
                            {
                                format!("ctx.{}", ident.sym)
                            } else {
                                ident.sym.clone()
                            }
                        }
                        Expr::Member(_) => {
                            let prev = self.in_call_callee;
                            self.in_call_callee = true;
                            let res = self.generate_expr(&call.callee);
                            self.in_call_callee = prev;
                            res
                        }
                        _ => self.generate_expr(&call.callee),
                    }
                };
                if (callee == "spawn" || callee == "@task_spawn" || callee == "@task_spawn_cpu")
                    && call.args.len() == 1
                {
                    let task_fn = match &call.args[0] {
                        Expr::Ident(id) => id.sym.clone(),
                        Expr::Arrow(_) => self.generate_expr(&call.args[0]),
                        _ => "Unknown".to_string(),
                    };
                    let mut captures = String::new();
                    let moved_in_closure = self
                        .lambda_moved_vars
                        .get(&task_fn)
                        .cloned()
                        .unwrap_or_default();
                    if let Some(cv) = self.captured_vars.get(&task_fn).cloned() {
                        for (r, _) in &cv {
                            if self.var_is_ref_struct.contains(r) {
                                panic!("CP Compile Error: Stack-bound ref struct '{}' cannot be captured across parallel task spawn boundary", r);
                            }
                            if self.var_borrow_sources.contains_key(r) {
                                panic!("CP Compile Error: Temporary borrow '{}' cannot be captured across parallel task spawn boundary (use move or Shared<T>)", r);
                            }
                            if !moved_in_closure.contains(r) {
                                if self.moved_vars.contains(r) {
                                    panic!("CP Compile Error: Use of moved variable '{}'", r);
                                }
                                self.check_borrow_conflict(r, false);
                            }
                            let src = if self.is_state_machine {
                                format!("ctx.{}", r)
                            } else {
                                r.clone()
                            };
                            captures.push_str(&format!("sub_ctx.{} = {}; ", r, src));
                        }
                    }
                    let rt_act = self.rt_sym("global_active_tasks");
                    let rt_nid = self.rt_sym("current_node_id");
                    let rt_lid = self.rt_sym("next_local_id");
                    if callee == "@task_spawn_cpu" {
                        return format!("(blk: {{ _ = {2}.fetchAdd(1, .acquire); const arena_ptr = std.heap.page_allocator.create(std.heap.ArenaAllocator) catch unreachable; arena_ptr.* = std.heap.ArenaAllocator.init(std.heap.page_allocator); const task_ptr = arena_ptr.allocator().create(CpTask) catch unreachable; task_ptr.* = .{{ .arena = arena_ptr }}; const sub_ctx = arena_ptr.allocator().create({0}_Context) catch unreachable; sub_ctx.* = undefined; sub_ctx.pc = 0; sub_ctx.arena = arena_ptr; sub_ctx._parent = @ptrCast(task_ptr); sub_ctx._parent_fn = @ptrCast(&__cp_task_bridge); {1} const p = PID{{ .node_id = {3}, .local_id = {4} }}; {4} += 1; sub_ctx._self = p; const _CpuRunner = struct {{ pub fn run(c: *anyopaque) void {{ {0}(c) catch {{}}; }} }}; const _th = std.Thread.spawn(.{{}}, _CpuRunner.run, .{{@as(*anyopaque, @ptrCast(sub_ctx))}}) catch unreachable; _th.detach(); break :blk task_ptr; }})", task_fn, captures, rt_act, rt_nid, rt_lid);
                    }
                    return format!("(blk: {{ _ = {2}.fetchAdd(1, .acquire); const arena_ptr = std.heap.page_allocator.create(std.heap.ArenaAllocator) catch unreachable; arena_ptr.* = std.heap.ArenaAllocator.init(std.heap.page_allocator); const task_ptr = arena_ptr.allocator().create(CpTask) catch unreachable; task_ptr.* = .{{ .arena = arena_ptr }}; const sub_ctx = arena_ptr.allocator().create({0}_Context) catch unreachable; sub_ctx.* = undefined; sub_ctx.pc = 0; sub_ctx.arena = arena_ptr; sub_ctx._parent = @ptrCast(task_ptr); sub_ctx._parent_fn = @ptrCast(&__cp_task_bridge); {1} const p = PID{{ .node_id = {3}, .local_id = {4} }}; {4} += 1; sub_ctx._self = p; schedule(@ptrCast(&{0}), @ptrCast(sub_ctx)); break :blk task_ptr; }})", task_fn, captures, rt_act, rt_nid, rt_lid);
                }
                if !self.has_imported_ptr && callee == "Shared" && !call.args.is_empty() {
                    let arg = self.generate_expr(&call.args[0]);
                    return format!(
                        "(blk: {{ const _val = {}; const _T = @TypeOf(_val); if (comptime @typeInfo(_T) == .pointer) break :blk _val else {{ const _p = std.heap.page_allocator.create(_T) catch unreachable; _p.* = _val; break :blk _p; }} }})",
                        arg
                    );
                }
                if callee == "Mutex" {
                    if call.args.is_empty() {
                        return "CpBareMutex{}".to_string();
                    } else {
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("cp_make_mutex({})", arg);
                    }
                }
                if callee == "RwLock" {
                    if call.args.is_empty() {
                        return "CpBareRwLock{}".to_string();
                    } else {
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("cp_make_rwlock({})", arg);
                    }
                }
                if callee == "Atomic" {
                    if call.args.is_empty() {
                        return "cp_make_atomic(@as(i32, 0))".to_string();
                    } else {
                        let arg = self.generate_expr(&call.args[0]);
                        return format!("cp_make_atomic({})", arg);
                    }
                }
                if callee == "drop" && call.args.len() == 1 && !self.fn_sigs.contains_key("drop") {
                    let inner_arg = match &call.args[0] {
                        Expr::Move(inner) => &**inner,
                        other => other,
                    };
                    if let Expr::Ident(id) = inner_arg {
                        if self.var_borrow_sources.contains_key(&id.sym)
                            || (self.current_fn_params.contains(&id.sym)
                                && !self.move_fn_params.contains(&id.sym))
                        {
                            panic!(
                                "CP Compile Error: Cannot drop borrowed reference or read-only parameter '{}'",
                                id.sym
                            );
                        }
                        self.check_active_borrows_before_move(&id.sym);
                        let gen = self.generate_expr(inner_arg);
                        self.moved_vars.insert(id.sym.clone());
                        let drop_body = Self::build_drop_call_snippet(&gen);
                        if !self.is_state_machine && self.drop_flag_vars.contains(&id.sym) {
                            return format!(
                                "(blk: {{ if (!__{}_moved) {{ __{}_moved = true; {} }} break :blk {{}}; }})",
                                id.sym, id.sym, drop_body
                            );
                        } else {
                            return format!("(blk: {{ {} break :blk {{}}; }})", drop_body);
                        }
                    } else {
                        let gen = self.generate_expr(inner_arg);
                        let drop_body = Self::build_drop_call_snippet("__cp_drop_tmp");
                        return format!(
                            "(blk: {{ var __cp_drop_tmp = {}; _ = &__cp_drop_tmp; {} break :blk {{}}; }})",
                            gen, drop_body
                        );
                    }
                }
                if callee == "Error" {
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
                if callee == "test" && call.args.len() == 2 {
                    let name = self.generate_expr(&call.args[0]);
                    let lambda = self.generate_expr(&call.args[1]);
                    let rt_tot = self.rt_sym("__cap_test_total");
                    let rt_pas = self.rt_sym("__cap_test_passed");
                    let rt_fai = self.rt_sym("__cap_test_failed");
                    let rt_nid = self.rt_sym("current_node_id");
                    let rt_lid = self.rt_sym("next_local_id");
                    return format!("(blk: {{ {3} += 1; const __start_fails = {5}; @import(\"std\").debug.print(\"\\n▶ RUN   {{s}}\\n\", .{{{0}}}); var _test_arena = std.heap.ArenaAllocator.init(std.heap.page_allocator); defer _test_arena.deinit(); var test_ctx: {1}_Context = undefined; test_ctx.pc = 0; test_ctx._self = PID{{ .node_id = {6}, .local_id = {7} }}; {7} += 1; test_ctx._parent = null; test_ctx._parent_fn = null; test_ctx.arena = &_test_arena; test_ctx.received_msg = null; test_ctx.received_sender = null; test_ctx.await_result = null; {2}(&test_ctx) catch |e| {{ if (e != error.TestFailed) {{ @import(\"std\").debug.print(\"❌ Panic: {{any}}\\n\", .{{e}}); {5} += 1; }} }}; if ({5} == __start_fails) {{ @import(\"std\").debug.print(\"✔ PASS  {{s}}\\n\", .{{{0}}}); {4} += 1; }} else {{ @import(\"std\").debug.print(\"❌ FAIL  {{s}}\\n\", .{{{0}}}); }} break :blk {{}}; }})", name, lambda, lambda, rt_tot, rt_pas, rt_fai, rt_nid, rt_lid);
                }
                if callee == "expect_eq" && call.args.len() == 2 {
                    let a = self.generate_expr(&call.args[0]);
                    let b = self.generate_expr(&call.args[1]);
                    let rt_fai = self.rt_sym("__cap_test_failed");
                    return format!("(blk: {{ const _a = {}; const _b = {}; const is_eq = cp_eq(_a, _b); if (!is_eq) {{ @import(\"std\").debug.print(\"\\n    ❌ Assertion Failed!\\n    Expected: {{any}}\\n    Got:      {{any}}\\n\", .{{ _b, _a }}); {} += 1; return error.TestFailed; }} break :blk {{}}; }})", a, b, rt_fai);
                }
                if callee == "serve" && !call.args.is_empty() {
                    return self.generate_http_serve(&call.args[0]);
                }
                if callee == "json" && call.args.len() == 1 {
                    let arg = self.generate_expr(&call.args[0]);
                    return format!(
                        r#"(blk: {{
    const _val = {};
    const _fmt = if (@typeInfo(@TypeOf(_val)) == .pointer) _val.* else _val;
    const _raw_json = @import("std").fmt.allocPrint(std.heap.page_allocator, "{{f}}", .{{@import("std").json.fmt(_fmt, .{{}})}}) catch unreachable;
    const _res = Response._create(std.heap.page_allocator) catch unreachable;
    _res.init(_raw_json);
    _res.contentType = "application/json";
    break :blk _res;
}})"#,
                        arg
                    );
                }

                if callee == "next" {
                    if let Some(next_fn) = self.current_next_callee.clone() {
                        let mut args_str = String::new();
                        if call.args.is_empty() {
                            for p in &self.current_forward_params {
                                if !args_str.is_empty() {
                                    args_str.push_str(", ");
                                }
                                args_str.push_str(p);
                            }
                        } else {
                            for arg in &call.args {
                                if !args_str.is_empty() {
                                    args_str.push_str(", ");
                                }
                                args_str.push_str(&self.generate_expr(arg));
                            }
                        }
                        if self.is_class_method {
                            return format!("self.{}({})", next_fn, args_str);
                        } else if let Some(c) = &self.current_class_name {
                            return format!("{}.{}({})", c, next_fn, args_str);
                        } else {
                            return format!("{}({})", next_fn, args_str);
                        }
                    }
                }

                // 4. Free function call: check Trait conformance and `mut` / `move` parameter modifiers
                let fn_sig_opt = self.fn_sigs.get(&callee).cloned();
                let mut method_params_opt = None;
                if let Expr::Member(mem) = &*call.callee {
                    let type_target = self.generate_expr(&mem.obj);
                    if let Some((params, _, _)) = self.lookup_method(&type_target, &mem.prop.sym) {
                        let non_recv: Vec<Param> = params
                            .into_iter()
                            .filter(|p| {
                                p.ident.sym != "this" && p.ident.sym != "_" && p.ident.sym != "self"
                            })
                            .collect();
                        method_params_opt = Some(non_recv);
                    }
                }
                let mut args = String::new();
                for (idx, arg) in call.args.iter().enumerate() {
                    let mut arg_is_move_param = false;
                    let old_box = self.expected_box_trait.clone();
                    let old_array_elem = self.expected_array_elem_type.clone();
                    if let Some(sig) = &fn_sig_opt {
                        if idx < sig.params.len() {
                            let param = &sig.params[idx];
                            if let Some(el) = Self::fixed_or_vec_elem_type(&param.ty) {
                                self.expected_array_elem_type = Some(el);
                            }
                            if let Type::BoxDyn(tr_id) = &param.ty {
                                self.expected_box_trait = Some(tr_id.sym.clone());
                            }
                            if let Type::Ref(t_id) = &param.ty {
                                if self.trait_defs.contains_key(&t_id.sym) {
                                    let inner_arg = match arg {
                                        Expr::Move(inner) | Expr::Borrow(inner, _) => &**inner,
                                        other => other,
                                    };
                                    if let Expr::Ident(arg_id) = inner_arg {
                                        if let Some(arg_cls) =
                                            self.resolve_var_type_name(&arg_id.sym)
                                        {
                                            let implements =
                                                self.class_defs.get(&arg_cls).map_or(false, |c| {
                                                    c.implements.iter().any(|i| i.sym == t_id.sym)
                                                        || if let Some(tr) =
                                                            self.trait_defs.get(&t_id.sym)
                                                        {
                                                            tr.methods.iter().all(|tm| {
                                                                tm.default_body.is_some()
                                                                    || c.methods.iter().any(|m| {
                                                                        m.ident.sym == tm.ident.sym
                                                                    })
                                                            })
                                                        } else {
                                                            false
                                                        }
                                                });
                                            if !implements {
                                                panic!(
                                                    "CP Compile Error: Type '{}' does not implement trait '{}'",
                                                    arg_cls, t_id.sym
                                                );
                                            }
                                        }
                                    }
                                }
                            }
                            if let Expr::Ident(arg_id) = arg {
                                if param.is_mut {
                                    if let Some((_, is_m)) =
                                        self.var_borrow_sources.get(&arg_id.sym)
                                    {
                                        if !*is_m {
                                            panic!(
                                                "CP Compile Error: Cannot pass read-only borrow '{}' to mutable parameter '{}'",
                                                arg_id.sym, param.ident.sym
                                            );
                                        }
                                    } else if self.current_fn_params.contains(&arg_id.sym)
                                        && !self.mutable_fn_params.contains(&arg_id.sym)
                                    {
                                        panic!(
                                            "CP Compile Error: Cannot pass read-only parameter '{}' to mutable parameter '{}'",
                                            arg_id.sym, param.ident.sym
                                        );
                                    }
                                    self.check_borrow_conflict(&arg_id.sym, true);
                                } else if param.is_move {
                                    if self.var_borrow_sources.contains_key(&arg_id.sym)
                                        || (self.current_fn_params.contains(&arg_id.sym)
                                            && !self.move_fn_params.contains(&arg_id.sym))
                                    {
                                        panic!(
                                            "CP Compile Error: Cannot move borrowed reference or read-only parameter '{}' into consuming parameter '{}'",
                                            arg_id.sym, param.ident.sym
                                        );
                                    }
                                    self.check_active_borrows_before_move(&arg_id.sym);
                                    arg_is_move_param = true;
                                } else {
                                    self.check_borrow_conflict(&arg_id.sym, false);
                                }
                            }
                        }
                    }
                    let param = fn_sig_opt.as_ref().and_then(|sig| sig.params.get(idx));
                    let mut gen_arg = self.generate_expr(arg);
                    if let Some(p) = param {
                        if let Type::Ref(id) = &p.ty {
                            if id.sym == "BigInt" {
                                if !gen_arg.starts_with('&') {
                                    gen_arg = format!("&{}", gen_arg);
                                }
                            }
                        }
                    }
                    if gen_arg.starts_with("lambda_") {
                        gen_arg = format!("@ptrCast(@constCast(&{}))", gen_arg);
                    }
                    self.expected_array_elem_type = old_array_elem;
                    self.expected_box_trait = old_box;
                    if arg_is_move_param {
                        if let Expr::Ident(arg_id) = arg {
                            self.moved_vars.insert(arg_id.sym.clone());
                            if !self.is_state_machine && self.drop_flag_vars.contains(&arg_id.sym) {
                                args.push_str(&format!(
                                    "(blk: {{ __{}_moved = true; break :blk {}; }})",
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
                if let Some(params) = method_params_opt {
                    if call.args.len() < params.len() {
                        for p in &params[call.args.len()..] {
                            if let Some(def) = &p.default {
                                if !args.is_empty() {
                                    args.push_str(", ");
                                }
                                args.push_str(&self.generate_expr(def));
                            }
                        }
                    }
                } else if let Some(sig) = &fn_sig_opt {
                    if call.args.len() < sig.params.len() {
                        for p in &sig.params[call.args.len()..] {
                            if let Some(def) = &p.default {
                                if !args.is_empty() {
                                    args.push_str(", ");
                                }
                                args.push_str(&self.generate_expr(def));
                            }
                        }
                    }
                }
                let call_res = if call.is_comptime {
                    format!("comptime {}({})", callee, args)
                } else if !self.is_state_machine && call.args.is_empty() {
                    let wrapped = if let Expr::Member(mem) = &*call.callee {
                        if let Expr::Ident(id) = &*mem.obj {
                            let is_dtor_call = mem.prop.sym == "__cp_deinit"
                                || mem.prop.sym == "__cp_errdeinit"
                                || (mem.prop.sym == "release"
                                    && self.is_builtin_raii_type(self.var_cp_types.get(&id.sym)));
                            if is_dtor_call && self.drop_flag_vars.contains(&id.sym) {
                                Some(format!(
                                    "(blk: {{ if (!__{}_moved) {{ __{}_moved = true; {}({}); }} break :blk {{}}; }})",
                                    id.sym, id.sym, callee, args
                                ))
                            } else {
                                None
                            }
                        } else {
                            None
                        }
                    } else {
                        None
                    };
                    wrapped.unwrap_or_else(|| format!("{}({})", callee, args))
                } else {
                    format!("{}({})", callee, args)
                };
                self.maybe_wrap_try_call(
                    call_res,
                    fn_sig_opt.as_ref().and_then(|s| s.return_type.as_ref()),
                )
            }
            _ => unreachable!(),
        }
    }
}
