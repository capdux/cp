use super::super::*;
use crate::ast::*;
use crate::Span;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

impl Codegen {
    pub(crate) fn generate_expr_error_flow(&mut self, expr: &Expr) -> String {
        match expr {
            Expr::Question(inner) => {
                let old_in_try = self.in_try_region;
                self.in_try_region = false;
                let inner_ty = self.infer_cp_type(inner);
                let exp = self.generate_expr(inner);
                self.in_try_region = old_in_try;

                if self.is_lib_mode && self.try_catch_target.is_none() {
                    return format!("try {}", exp);
                }

                let id = self.lambda_counter;
                self.lambda_counter += 1;

                match inner_ty {
                    Some(Type::ResultType { ok, err: src_err }) => {
                        if self.is_lib_mode && self.try_catch_target.is_none() {
                            return format!("try {}", exp);
                        }
                        let is_fn_native = match &self.current_fn_ret_ty {
                            Some(Type::ResultType { err: fn_err, .. }) => {
                                let s = self.map_error_type(fn_err);
                                self.is_lib_mode
                                    || s.contains('.')
                                    || s == "anyerror"
                                    || self.error_sets.contains_key(&s)
                            }
                            Some(Type::ErrorUnion(_)) => true,
                            _ => false,
                        };

                        let err_stmt = if let Some((blk_label, err_var, flag_var, target_err)) =
                            self.try_catch_target.clone()
                        {
                            self.verify_error_type_compatibility(&src_err, &target_err);
                            let tgt_zig = self.map_error_type(&target_err);
                            format!(
                                "const __e_{id} = cp_unwrap_err_val(__r_{id}); {err_var} = cp_convert_err({tgt_zig}, __e_{id}); {flag_var} = true; break :{blk_label};"
                            )
                        } else if let Some(Type::ResultType { err: fn_err, .. }) =
                            self.current_fn_ret_ty.clone()
                        {
                            self.verify_error_type_compatibility(&src_err, &fn_err);
                            let tgt_zig = self.map_error_type(&fn_err);
                            if is_fn_native {
                                format!("const __e_{id} = cp_unwrap_err_val(__r_{id}); return cp_convert_err({tgt_zig}, __e_{id});")
                            } else {
                                format!("const __e_{id} = cp_unwrap_err_val(__r_{id}); return .{{ .err = cp_convert_err({tgt_zig}, __e_{id}) }};")
                            }
                        } else if is_fn_native {
                            format!(
                                "const __e_{id} = cp_unwrap_err_val(__r_{id}); return __e_{id};"
                            )
                        } else {
                            panic!("CP Compile Error: Operator '?' cannot be used on '!Error' expression inside a function that does not return '!Error'");
                        };

                        let ok_handling = if matches!(*ok, Type::Optional(_))
                            && self.try_catch_target.is_none()
                        {
                            match &self.current_fn_ret_ty {
                                Some(Type::ResultType { ok: fn_ok, .. })
                                    if matches!(&**fn_ok, Type::Optional(_)) =>
                                {
                                    format!(
                                        "const __v_{id} = cp_unwrap_ok_val(__r_{id}); if (__v_{id} == null) return .{{ .ok = null }}; break :blk_q_{id} __v_{id}.?;"
                                    )
                                }
                                Some(Type::Optional(_)) => {
                                    format!(
                                        "const __v_{id} = cp_unwrap_ok_val(__r_{id}); if (__v_{id} == null) return null; break :blk_q_{id} __v_{id}.?;"
                                    )
                                }
                                _ => format!("break :blk_q_{id} cp_unwrap_ok_val(__r_{id});"),
                            }
                        } else {
                            format!("break :blk_q_{id} cp_unwrap_ok_val(__r_{id});")
                        };

                        format!(
                            "(blk_q_{id}: {{ const __r_{id} = {exp}; if (cp_has_ok_val(__r_{id})) {{ {ok_handling} }} {err_stmt} }})"
                        )
                    }
                    Some(Type::Optional(_)) => {
                        let null_ret = match &self.current_fn_ret_ty {
                            Some(Type::ResultType { ok: fn_ok, .. })
                                if matches!(&**fn_ok, Type::Optional(_)) =>
                            {
                                "return .{ .ok = null };"
                            }
                            Some(Type::Optional(_)) => "return null;",
                            _ => panic!(
                                "CP Compile Error: Operator '?' cannot bubble 'null' from '?T' inside a function that does not return an optional '?U' type"
                            ),
                        };
                        format!(
                            "(blk_q_{id}: {{ const __v_{id} = {exp}; if (__v_{id} == null) {null_ret} break :blk_q_{id} __v_{id}.?; }})"
                        )
                    }
                    _ => format!("try {}", exp),
                }
            }
            Expr::QuestionFallback {
                expr: inner,
                fallback,
                is_return_fallback,
            } => {
                let old_in_try = self.in_try_region;
                self.in_try_region = false;
                let inner_ty = self.infer_cp_type(inner);
                let exp = self.generate_expr(inner);
                self.in_try_region = old_in_try;
                let id = self.lambda_counter;
                self.lambda_counter += 1;

                let is_inner_optional = matches!(inner_ty, Some(Type::Optional(_)));
                let is_inner_error_and_optional = match &inner_ty {
                    Some(Type::ResultType { ok, .. }) => matches!(&**ok, Type::Optional(_)),
                    Some(Type::ErrorUnion(ok)) => matches!(&**ok, Type::Optional(_)),
                    _ => false,
                };

                if *is_return_fallback || matches!(&**fallback, Expr::Return(_, _)) {
                    let ret_inner = match &**fallback {
                        Expr::Return(_, val) => val.as_deref(),
                        other => Some(other),
                    };
                    let ret_stmt = self.build_ret_stmt_for_fallback(ret_inner);

                    if self.is_lib_mode {
                        if is_inner_error_and_optional {
                            format!("((({exp}) catch {{ {ret_stmt} }}) orelse {{ {ret_stmt} }})")
                        } else if is_inner_optional {
                            format!("(({exp}) orelse {{ {ret_stmt} }})")
                        } else {
                            format!("(({exp}) catch {{ {ret_stmt} }})")
                        }
                    } else {
                        format!(
                            "(blk_qf_{id}: {{ const __r_{id} = {exp}; if (cp_has_ok_val(__r_{id})) break :blk_qf_{id} cp_unwrap_ok_val(__r_{id}); {ret_stmt} }})"
                        )
                    }
                } else if matches!(&**fallback, Expr::Continue(_)) {
                    if self.is_lib_mode {
                        if is_inner_error_and_optional {
                            format!("((({exp}) catch continue) orelse continue)")
                        } else if is_inner_optional {
                            format!("(({exp}) orelse continue)")
                        } else {
                            format!("(({exp}) catch continue)")
                        }
                    } else {
                        format!(
                            "(blk_qf_{id}: {{ const __r_{id} = {exp}; if (cp_has_ok_val(__r_{id})) break :blk_qf_{id} cp_unwrap_ok_val(__r_{id}); continue; }})"
                        )
                    }
                } else if let Expr::Break(_, val) = &**fallback {
                    let brk_action = if let Some(v) = val {
                        format!("break {}", self.generate_expr(v))
                    } else {
                        "break".to_string()
                    };
                    if self.is_lib_mode {
                        if is_inner_error_and_optional {
                            format!("((({exp}) catch {brk_action}) orelse {brk_action})")
                        } else if is_inner_optional {
                            format!("(({exp}) orelse {brk_action})")
                        } else {
                            format!("(({exp}) catch {brk_action})")
                        }
                    } else {
                        format!(
                            "(blk_qf_{id}: {{ const __r_{id} = {exp}; if (cp_has_ok_val(__r_{id})) break :blk_qf_{id} cp_unwrap_ok_val(__r_{id}); {brk_action}; }})"
                        )
                    }
                } else {
                    let fb_c = self.generate_expr(fallback);
                    if self.is_lib_mode {
                        if is_inner_error_and_optional {
                            format!("((({exp}) catch {fb_c}) orelse {fb_c})")
                        } else if is_inner_optional {
                            format!("(({exp}) orelse {fb_c})")
                        } else {
                            format!("(({exp}) catch {fb_c})")
                        }
                    } else {
                        format!(
                            "(blk_qf_{id}: {{ const __r_{id} = {exp}; if (cp_has_ok_val(__r_{id})) break :blk_qf_{id} cp_unwrap_ok_val(__r_{id}); break :blk_qf_{id} cp_val({fb_c}); }})"
                        )
                    }
                }
            }
            Expr::NullCoalesce { left, right } => {
                let old_in_try = self.in_try_region;
                self.in_try_region = false;
                let l_ty = self.infer_cp_type(left);
                if matches!(
                    l_ty,
                    Some(Type::ResultType { .. }) | Some(Type::ErrorUnion(_))
                ) {
                    panic!(
                        "CP Compile Error: Operator '??' can only be used on optional '?T' for null coalescing; for '!Error' types use '? default' or 'catch'"
                    );
                }
                let l_code = self.generate_expr(left);
                self.in_try_region = old_in_try;

                match &**right {
                    Expr::Continue(_) => {
                        format!("(({}) orelse continue)", l_code)
                    }
                    Expr::LabeledContinue(_, label) => {
                        format!("(({}) orelse continue :{})", l_code, label.sym)
                    }
                    Expr::Break(_, val) => {
                        if let Some(v) = val {
                            let v_code = self.generate_expr(v);
                            format!("(({}) orelse break {})", l_code, v_code)
                        } else {
                            format!("(({}) orelse break)", l_code)
                        }
                    }
                    Expr::LabeledBreak(_, label, val) => {
                        if let Some(v) = val {
                            let v_code = self.generate_expr(v);
                            format!("(({}) orelse break :{} {})", l_code, label.sym, v_code)
                        } else {
                            format!("(({}) orelse break :{})", l_code, label.sym)
                        }
                    }
                    Expr::Return(_, ret_expr) => {
                        let ret_stmt = self.build_ret_stmt_for_fallback(ret_expr.as_deref());
                        format!("(({}) orelse {{ {} }})", l_code, ret_stmt)
                    }
                    Expr::LabeledBlock { label: None, block } => {
                        let old_case = std::mem::take(&mut self.current_case_content);
                        for s in &block.body {
                            self.generate_stmt(s);
                        }
                        let body_code = std::mem::replace(&mut self.current_case_content, old_case);
                        format!("(({}) orelse {{\n{}\n}})", l_code, body_code.trim_end())
                    }
                    _ => {
                        let r_code = self.generate_expr(right);
                        if self.is_lib_mode {
                            format!("(({}) orelse ({}))", l_code, r_code)
                        } else {
                            format!("cp_null_coalesce({}, {})", l_code, r_code)
                        }
                    }
                }
            }
            Expr::Catch {
                expr: inner,
                err_ident,
                body,
                ..
            } => {
                let old_in_try = self.in_try_region;
                self.in_try_region = false;
                let inner_ty = self.infer_cp_type(inner);
                let exp = self.generate_expr(inner);
                self.in_try_region = old_in_try;
                let id = self.lambda_counter;
                self.lambda_counter += 1;
                let blk_label = format!("blk_catch_{}", id);
                let err_bind = err_ident
                    .as_ref()
                    .map(|i| i.sym.clone())
                    .unwrap_or_else(|| format!("__ce_{}", id));

                let old_vars = self.var_cp_types.clone();
                let old_locals = self.locals.clone();
                if let Some(Type::ResultType { err, .. }) = &inner_ty {
                    if let Some(eid) = err_ident {
                        self.var_cp_types
                            .insert(eid.sym.clone(), self.error_type_to_cp_type(err));
                        self.locals
                            .insert(eid.sym.clone(), self.map_error_type(err));
                    }
                }

                let target_payload_ty = match &inner_ty {
                    Some(Type::ErrorUnion(t)) => Some(self.map_type(t)),
                    Some(Type::ResultType { ok, .. }) => Some(self.map_type(ok)),
                    _ => None,
                };

                let catch_arm_body = match &**body {
                    Stmt::Expr(e) => {
                        if matches!(e, Expr::Bin(bin) if bin.op == BinaryOp::Assign) {
                            let saved = self.current_case_content.clone();
                            self.current_case_content.clear();
                            self.generate_stmt(body);
                            let default_val = if let Some(ty_str) = &target_payload_ty {
                                match ty_str.as_str() {
                                    "bool" => "false",
                                    "void" => "{}",
                                    s if s.starts_with('?') => "null",
                                    _ => "undefined",
                                }
                            } else {
                                "{}"
                            };
                            let brk = if let Some(ty_str) = &target_payload_ty {
                                format!("break :{} @as({}, {});", blk_label, ty_str, default_val)
                            } else {
                                format!("break :{} {};", blk_label, default_val)
                            };
                            self.write_to_case(&brk);
                            let code = self.current_case_content.trim().to_string();
                            self.current_case_content = saved;
                            code
                        } else {
                            let ec = self.generate_expr(e);
                            if ec == "unreachable"
                                || ec.starts_with("@panic")
                                || ec.starts_with("@compileError")
                                || ec.starts_with("break")
                                || ec.starts_with("return")
                                || ec.starts_with("continue")
                            {
                                format!("{};", ec)
                            } else if let Some(ty_str) = &target_payload_ty {
                                format!("break :{} @as({}, {});", blk_label, ty_str, ec)
                            } else if self.is_lib_mode || matches!(e, Expr::Lit(_)) {
                                format!("break :{} {};", blk_label, ec)
                            } else {
                                format!("break :{} cp_val({});", blk_label, ec)
                            }
                        }
                    }
                    Stmt::Return(_) => {
                        let saved = self.current_case_content.clone();
                        self.current_case_content.clear();
                        self.generate_stmt(body);
                        let ret_s = self.current_case_content.trim().to_string();
                        self.current_case_content = saved;
                        ret_s
                    }
                    Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
                        let saved = self.current_case_content.clone();
                        self.current_case_content.clear();
                        let len = b.body.len();
                        for (idx, s) in b.body.iter().enumerate() {
                            if idx + 1 == len {
                                if let Stmt::Expr(e) = s {
                                    if matches!(e, Expr::Bin(bin) if bin.op == BinaryOp::Assign) {
                                        self.generate_stmt(s);
                                        if let Some(ty_str) = &target_payload_ty {
                                            let default_val = match ty_str.as_str() {
                                                "bool" => "false",
                                                "void" => "{}",
                                                s if s.starts_with('?') => "null",
                                                _ => "undefined",
                                            };
                                            self.write_to_case(&format!(
                                                "break :{} @as({}, {});",
                                                blk_label, ty_str, default_val
                                            ));
                                        } else if self.is_lib_mode {
                                            self.write_to_case(&format!(
                                                "break :{} {{}};",
                                                blk_label
                                            ));
                                        } else {
                                            self.write_to_case(&format!(
                                                "break :{} cp_val({{}});",
                                                blk_label
                                            ));
                                        }
                                        continue;
                                    }
                                    let ec = self.generate_expr(e);
                                    if ec == "unreachable"
                                        || ec.starts_with("@panic")
                                        || ec.starts_with("@compileError")
                                    {
                                        self.write_to_case(&format!("{};", ec));
                                    } else if let Some(ty_str) = &target_payload_ty {
                                        self.write_to_case(&format!(
                                            "break :{} @as({}, {});",
                                            blk_label, ty_str, ec
                                        ));
                                    } else if self.is_lib_mode {
                                        self.write_to_case(&format!(
                                            "break :{} {};",
                                            blk_label, ec
                                        ));
                                    } else {
                                        self.write_to_case(&format!(
                                            "break :{} cp_val({});",
                                            blk_label, ec
                                        ));
                                    }
                                } else {
                                    self.generate_stmt(s);
                                }
                            } else {
                                self.generate_stmt(s);
                            }
                        }
                        let blk_s = self.current_case_content.trim().to_string();
                        self.current_case_content = saved;
                        blk_s
                    }
                    _ => {
                        let saved = self.current_case_content.clone();
                        self.current_case_content.clear();
                        self.generate_stmt(body);
                        let s_str = self.current_case_content.trim().to_string();
                        self.current_case_content = saved;
                        s_str
                    }
                };

                self.var_cp_types = old_vars;
                self.locals = old_locals;

                let is_native_err = match &inner_ty {
                    Some(Type::ResultType { err, .. }) => {
                        let err_str = self.map_error_type(err);
                        self.is_lib_mode
                            || err_str.contains('.')
                            || err_str == "anyerror"
                            || self.error_sets.contains_key(&err_str)
                            || err_str.starts_with('(')
                    }
                    _ => true,
                };

                if is_native_err {
                    if catch_arm_body == "unreachable;" && err_ident.is_none() {
                        return format!("(({exp}) catch unreachable)");
                    }
                    return format!(
                        "({blk_label}: {{ break :{blk_label} (({exp}) catch |{err_bind}| {{ _ = &{err_bind}; {catch_arm_body} }}); }})"
                    );
                }

                format!(
                    "({blk_label}: {{ const __r_{id} = {exp}; switch (__r_{id}) {{ .ok => |__v_{id}| break :{blk_label} __v_{id}, .err => |{err_bind}| {{ _ = &{err_bind}; {catch_arm_body} }} }} }})"
                )
            }
            Expr::ErrWrap(inner) => self.generate_err_value(inner, None),
            Expr::BoxNew(inner) => {
                let trait_name = self.expected_box_trait.clone().unwrap_or_else(|| {
                    panic!("CP Compile Error: Box.new(...) requires a known Box<dyn Trait> target type")
                });
                let inner_unwrapped = match &**inner {
                    Expr::Move(mv) => &**mv,
                    other => other,
                };
                let cls_name = match inner_unwrapped {
                    Expr::Ident(id) => {
                        if self.moved_vars.contains(&id.sym) {
                            panic!("CP Compile Error: Use of moved value '{}'", id.sym);
                        }
                        if self.var_borrow_sources.contains_key(&id.sym)
                            || (self.current_fn_params.contains(&id.sym)
                                && !self.move_fn_params.contains(&id.sym))
                        {
                            panic!(
                                "CP Compile Error: Box.new() requires ownership of '{}'; cannot box a borrowed reference",
                                id.sym
                            );
                        }
                        self.check_active_borrows_before_move(&id.sym);
                        self.resolve_var_type_name(&id.sym).unwrap_or_else(|| {
                            panic!(
                                "CP Compile Error: Cannot determine concrete type of '{}' for Box.new()",
                                id.sym
                            )
                        })
                    }
                    Expr::New(nc) => match &*nc.callee {
                        Expr::Ident(cid) => cid.sym.clone(),
                        _ => panic!("CP Compile Error: Invalid New expression inside Box.new()"),
                    },
                    other => {
                        if let Some(Type::Ref(cid)) = self.infer_cp_type(other) {
                            cid.sym.clone()
                        } else {
                            panic!("CP Compile Error: Cannot determine concrete type for Box.new()")
                        }
                    }
                };
                let implements = self
                    .class_defs
                    .get(&cls_name)
                    .map_or(false, |c| c.implements.iter().any(|i| i.sym == trait_name));
                if !implements {
                    panic!(
                        "CP Compile Error: Type '{}' does not implement trait '{}' required by Box<dyn {}>",
                        cls_name, trait_name, trait_name
                    );
                }
                let inner_code = self.generate_expr(inner);
                let move_stmt = if let Expr::Ident(id) = &**inner {
                    self.moved_vars.insert(id.sym.clone());
                    if !self.is_state_machine && self.drop_flag_vars.contains(&id.sym) {
                        format!("__{}_moved = true; ", id.sym)
                    } else {
                        String::new()
                    }
                } else {
                    String::new()
                };
                let id = self.lambda_counter;
                self.lambda_counter += 1;
                format!(
                    "(blk_box_{id}: {{ const __b_ptr_{id} = {inner_code}; {move_stmt}break :blk_box_{id} {trait_name}{{ .ptr = @ptrCast(__b_ptr_{id}), .vtable = &vtable_{trait_name}_{cls_name} }}; }})"
                )
            }
            Expr::Downcast {
                obj,
                target_ty,
                is_mut,
            } => {
                if let Expr::Ident(id) = &**obj {
                    if *is_mut {
                        if let Some((_, is_m)) = self.var_borrow_sources.get(&id.sym) {
                            if !*is_m {
                                panic!(
                                    "CP Compile Error: Cannot take mutable downcast borrow from read-only borrow '{}'",
                                    id.sym
                                );
                            }
                        } else if self.current_fn_params.contains(&id.sym)
                            && !self.mutable_fn_params.contains(&id.sym)
                        {
                            panic!(
                                "CP Compile Error: Cannot take mutable downcast borrow from read-only parameter '{}'",
                                id.sym
                            );
                        }
                    }
                    self.check_borrow_conflict(&id.sym, *is_mut);
                }
                let obj_code = self.generate_expr(obj);
                format!("{}.downcast({})", obj_code, target_ty.sym)
            }
            Expr::EnumInit(ei) => {
                let fields_slice = ei.fields.as_deref().unwrap_or(&[]);
                let tuple_slice = ei.tuple_args.as_deref().unwrap_or(&[]);
                if ei.variant_name.sym.is_empty() && !ei.enum_name.sym.is_empty() {
                    let mut target_sym = ei.enum_name.sym.clone();
                    while let Some(ta) = self.type_aliases.get(&target_sym) {
                        if let Type::Ref(ref tid) = ta.target_ty {
                            target_sym = tid.sym.clone();
                        } else {
                            break;
                        }
                    }
                    let base_name = target_sym.split('<').next().unwrap_or(&target_sym);
                    let cls_opt = self.class_defs.get(base_name).cloned();
                    if let Some(ref cls) = cls_opt {
                        if cls.is_static {
                            panic!(
                                "CP Compile Error: Cannot instantiate static class or struct '{}'",
                                ei.enum_name.sym
                            );
                        }
                    }
                    let mut subst_map: std::collections::HashMap<String, String> =
                        std::collections::HashMap::new();
                    if let Some(pos) = target_sym.find('<') {
                        if target_sym.ends_with('>') {
                            let args_str = &target_sym[pos + 1..target_sym.len() - 1];
                            let parts = Self::split_generic_args_str(args_str);
                            if let Some(ref cls) = cls_opt {
                                for (gp, arg) in cls.generic_params.iter().zip(parts.iter()) {
                                    let param_name = match gp {
                                        GenericParam::Type { ident, .. } => &ident.sym,
                                        GenericParam::Const { ident, .. } => &ident.sym,
                                    };
                                    subst_map.insert(param_name.clone(), arg.clone());
                                }
                            }
                        }
                    }
                    let mapped_name = if self.current_class_name.as_deref() == Some(base_name)
                        && cls_opt
                            .as_ref()
                            .map_or(false, |c| !c.generic_params.is_empty())
                    {
                        "@This()".to_string()
                    } else if !ei.enum_name.sym.contains('<')
                        && cls_opt
                            .as_ref()
                            .map_or(false, |c| !c.generic_params.is_empty())
                    {
                        if let Some(Type::Ref(ret_id)) = &self.current_fn_ret_ty {
                            if ret_id.sym.starts_with(&format!("{}<", base_name)) {
                                self.map_type_name(&ret_id.sym)
                            } else {
                                self.map_type_name(&ei.enum_name.sym)
                            }
                        } else {
                            self.map_type_name(&ei.enum_name.sym)
                        }
                    } else {
                        self.map_type_name(&ei.enum_name.sym)
                    };
                    if let Some(tuple_args) = &ei.tuple_args {
                        let args_str: Vec<String> =
                            tuple_args.iter().map(|a| self.generate_expr(a)).collect();
                        return format!("{}{{ {} }}", mapped_name, args_str.join(", "));
                    }
                    let mut s = format!("{}{{ ", mapped_name);
                    for (idx, f) in fields_slice.iter().enumerate() {
                        let old_array_elem = self.expected_array_elem_type.clone();
                        if let Some(ref cls) = cls_opt {
                            if let Some(cf) = cls.fields.iter().find(|fl| fl.ident.sym == f.key.sym)
                            {
                                if let Type::FixedArray(elem, _) | Type::Array(elem) = &cf.ty {
                                    let mut el = (**elem).clone();
                                    if let Type::Ref(ref mut id) = el {
                                        if let Some(sub) = subst_map.get(&id.sym) {
                                            id.sym = sub.clone();
                                        }
                                    }
                                    self.expected_array_elem_type = Some(el);
                                }
                            }
                        }
                        let val_code = self.generate_expr(&f.val);
                        self.expected_array_elem_type = old_array_elem;
                        s.push_str(&format!(".{} = {}", f.key.sym, val_code));
                        if idx + 1 < fields_slice.len() {
                            s.push_str(", ");
                        }
                    }
                    s.push_str(" }");
                    return s;
                }
                if ei.enum_name.sym.is_empty() {
                    if let Some(tuple_args) = &ei.tuple_args {
                        let is_known_payload_variant = self.enum_defs.values().any(|en| {
                            en.variants.iter().any(|v| {
                                v.ident.sym == ei.variant_name.sym && v.tuple_type.is_some()
                            })
                        });
                        let starts_lower = ei
                            .variant_name
                            .sym
                            .chars()
                            .next()
                            .map_or(false, |c| c.is_lowercase());
                        if tuple_args.is_empty() || (!is_known_payload_variant && starts_lower) {
                            let args_str: Vec<String> =
                                tuple_args.iter().map(|a| self.generate_expr(a)).collect();
                            return format!(".{}({})", ei.variant_name.sym, args_str.join(", "));
                        }
                    }
                    if !tuple_slice.is_empty() {
                        let is_expected_tuple = self.enum_defs.values().any(|en| {
                            en.variants.iter().any(|v| {
                                &v.ident.sym == &ei.variant_name.sym
                                    && matches!(v.tuple_type, Some(Type::Tuple(_)))
                            })
                        });
                        if tuple_slice.len() == 1 {
                            if is_expected_tuple {
                                if let Expr::Array(items) = &tuple_slice[0] {
                                    let args_str: Vec<String> =
                                        items.iter().map(|a| self.generate_expr(a)).collect();
                                    format!(
                                        ".{{ .{} = .{{ {} }} }}",
                                        ei.variant_name.sym,
                                        args_str.join(", ")
                                    )
                                } else {
                                    format!(
                                        ".{{ .{} = {} }}",
                                        ei.variant_name.sym,
                                        self.generate_expr(&tuple_slice[0])
                                    )
                                }
                            } else {
                                format!(
                                    ".{{ .{} = {} }}",
                                    ei.variant_name.sym,
                                    self.generate_expr(&tuple_slice[0])
                                )
                            }
                        } else {
                            let args_str: Vec<String> =
                                tuple_slice.iter().map(|a| self.generate_expr(a)).collect();
                            format!(
                                ".{{ .{} = .{{ {} }} }}",
                                ei.variant_name.sym,
                                args_str.join(", ")
                            )
                        }
                    } else if fields_slice.is_empty() {
                        if ei.variant_name.sym.is_empty() {
                            ".{}".to_string()
                        } else {
                            format!(".{}", ei.variant_name.sym)
                        }
                    } else {
                        let anon_cls_opt = if let Some(Type::Ref(ret_id)) = &self.current_fn_ret_ty
                        {
                            let b = ret_id.sym.split('<').next().unwrap_or(&ret_id.sym);
                            self.class_defs.get(b).cloned()
                        } else if let Some(ref cname) = self.current_class_name {
                            self.class_defs.get(cname).cloned()
                        } else {
                            None
                        };
                        let mut f_str = String::new();
                        for (idx, f) in fields_slice.iter().enumerate() {
                            let old_array_elem = self.expected_array_elem_type.clone();
                            if let Some(ref cls) = anon_cls_opt {
                                if let Some(cf) =
                                    cls.fields.iter().find(|fl| fl.ident.sym == f.key.sym)
                                {
                                    if let Type::FixedArray(elem, _) | Type::Array(elem) = &cf.ty {
                                        self.expected_array_elem_type = Some((**elem).clone());
                                    }
                                }
                            }
                            let val_code = self.generate_expr(&f.val);
                            self.expected_array_elem_type = old_array_elem;
                            f_str.push_str(&format!(".{} = {}", f.key.sym, val_code));
                            if idx + 1 < fields_slice.len() {
                                f_str.push_str(", ");
                            }
                        }
                        if ei.variant_name.sym.is_empty() {
                            format!(".{{ {} }}", f_str)
                        } else {
                            format!(".{{ .{} = .{{ {} }} }}", ei.variant_name.sym, f_str)
                        }
                    }
                } else if !tuple_slice.is_empty() {
                    let is_enum = self.enum_defs.contains_key(&ei.enum_name.sym);
                    if !is_enum {
                        let args_str: Vec<String> =
                            tuple_slice.iter().map(|a| self.generate_expr(a)).collect();
                        format!(
                            "{}.{}({})",
                            ei.enum_name.sym,
                            ei.variant_name.sym,
                            args_str.join(", ")
                        )
                    } else {
                        let is_expected_tuple =
                            self.enum_defs.get(&ei.enum_name.sym).map_or(false, |en| {
                                en.variants.iter().any(|v| {
                                    &v.ident.sym == &ei.variant_name.sym
                                        && matches!(v.tuple_type, Some(Type::Tuple(_)))
                                })
                            });
                        if tuple_slice.len() == 1 {
                            if is_expected_tuple {
                                if let Expr::Array(items) = &tuple_slice[0] {
                                    let args_str: Vec<String> =
                                        items.iter().map(|a| self.generate_expr(a)).collect();
                                    format!(
                                        "{}{{ .{} = .{{ {} }} }}",
                                        ei.enum_name.sym,
                                        ei.variant_name.sym,
                                        args_str.join(", ")
                                    )
                                } else {
                                    format!(
                                        "{}{{ .{} = {} }}",
                                        ei.enum_name.sym,
                                        ei.variant_name.sym,
                                        self.generate_expr(&tuple_slice[0])
                                    )
                                }
                            } else {
                                format!(
                                    "{}{{ .{} = {} }}",
                                    ei.enum_name.sym,
                                    ei.variant_name.sym,
                                    self.generate_expr(&tuple_slice[0])
                                )
                            }
                        } else {
                            let args_str: Vec<String> =
                                tuple_slice.iter().map(|a| self.generate_expr(a)).collect();
                            format!(
                                "{}{{ .{} = .{{ {} }} }}",
                                ei.enum_name.sym,
                                ei.variant_name.sym,
                                args_str.join(", ")
                            )
                        }
                    }
                } else if fields_slice.is_empty() {
                    let is_enum = self.enum_defs.contains_key(&ei.enum_name.sym);
                    if !is_enum {
                        format!("{}.{}", ei.enum_name.sym, ei.variant_name.sym)
                    } else {
                        let is_pure_enum =
                            self.enum_defs.get(&ei.enum_name.sym).map_or(false, |en| {
                                en.variants.iter().all(|v| {
                                    v.fields.is_none()
                                        && v.tuple_type.is_none()
                                        && v.from_err.is_none()
                                })
                            });
                        if is_pure_enum {
                            format!("{}.{}", ei.enum_name.sym, ei.variant_name.sym)
                        } else {
                            format!("{}{{ .{} = {{}} }}", ei.enum_name.sym, ei.variant_name.sym)
                        }
                    }
                } else {
                    let mut f_str = String::new();
                    for (idx, f) in fields_slice.iter().enumerate() {
                        f_str.push_str(&format!(".{} = {}", f.key.sym, self.generate_expr(&f.val)));
                        if idx + 1 < fields_slice.len() {
                            f_str.push_str(", ");
                        }
                    }
                    if ei.variant_name.sym.is_empty() {
                        format!("{}{{ {} }}", ei.enum_name.sym, f_str)
                    } else if !self.enum_defs.contains_key(&ei.enum_name.sym)
                        && ei
                            .variant_name
                            .sym
                            .chars()
                            .next()
                            .map_or(false, |c| c.is_uppercase())
                    {
                        format!(
                            "{}.{}{{ {} }}",
                            ei.enum_name.sym, ei.variant_name.sym, f_str
                        )
                    } else {
                        format!(
                            "{}{{ .{} = .{{ {} }} }}",
                            ei.enum_name.sym, ei.variant_name.sym, f_str
                        )
                    }
                }
            }
            _ => unreachable!(),
        }
    }

    pub(crate) fn build_ret_stmt_for_fallback(&mut self, fallback_expr: Option<&Expr>) -> String {
        let is_null_fb = match fallback_expr {
            Some(Expr::Lit(Lit::Null)) => true,
            Some(Expr::Ident(id)) if id.sym == "null" => true,
            None => false,
            _ => false,
        };

        if let Some(Type::ResultType { ok, err: fn_err }) = self.current_fn_ret_ty.clone() {
            if let Some(Expr::ErrWrap(err_inner)) = fallback_expr {
                let ev = self.generate_err_value(err_inner, Some(&fn_err));
                format!("return .{{ .err = {} }};", ev)
            } else if is_null_fb || (fallback_expr.is_none() && matches!(&*ok, Type::Optional(_))) {
                if !matches!(&*ok, Type::Optional(_)) {
                    panic!("CP Compile Error: Cannot return 'null' from function '{}' whose return type is not optional '?T' (actual ret ty: {:?})", self.current_fn_zig_name, self.current_fn_ret_ty);
                }
                "return .{ .ok = null };".to_string()
            } else if let Some(fb) = fallback_expr {
                let fb_c = self.generate_expr(fb);
                format!("return .{{ .ok = {} }};", fb_c)
            } else if matches!(&*ok, Type::Void) {
                "return .{ .ok = {} };".to_string()
            } else {
                "return;".to_string()
            }
        } else {
            let allows_null = match &self.current_fn_ret_ty {
                Some(Type::Optional(_)) => true,
                Some(Type::ErrorUnion(inner)) => matches!(&**inner, Type::Optional(_)),
                _ => false,
            };
            if is_null_fb && !allows_null {
                panic!("CP Compile Error: Cannot return 'null' from function '{}' whose return type is not optional '?T' (actual ret ty: {:?})", self.current_fn_zig_name, self.current_fn_ret_ty);
            }
            if let Some(fb) = fallback_expr {
                let fb_c = self.generate_expr(fb);
                format!("return {};", fb_c)
            } else {
                "return;".to_string()
            }
        }
    }
}
