use super::super::*;
use crate::ast::*;
use crate::Span;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

impl Codegen {
    pub(crate) fn generate_expr_ops(&mut self, expr: &Expr) -> String {
        match expr {
            Expr::Ident(i) => {
                if self.moved_vars.contains(&i.sym)
                    || ((i.sym == "this" || i.sym == "_")
                        && (self.moved_vars.contains("this") || self.moved_vars.contains("_")))
                {
                    panic!("CP Compile Error: Use of moved value '{}'", i.sym);
                }
                if !self.is_lib_mode && self.uninitialized_vars.contains(&i.sym) {
                    panic!(
                        "CP Compile Error: Variable '{}' read before initialization",
                        i.sym
                    );
                }
                if i.sym == "@is_beam" || i.sym == "@is_nest" {
                    return if self.is_beam_mode {
                        "true".to_string()
                    } else {
                        "false".to_string()
                    };
                }
                if i.sym == "@self" {
                    return if self.is_state_machine {
                        "ctx._self".to_string()
                    } else {
                        format!(
                            "PID{{ .node_id = {}, .local_id = 1 }}",
                            self.rt_sym("current_node_id")
                        )
                    };
                }
                if i.sym == "@sender" {
                    return if self.is_state_machine {
                        "ctx.received_sender.?".to_string()
                    } else {
                        "PID{ .node_id = 0, .local_id = 0 }".to_string()
                    };
                }
                if i.sym == "c" && !self.c_includes.is_empty() && !self.locals.contains_key("c") {
                    "__c_stdlib".to_string()
                } else if i.sym == "this"
                    && self.locals.contains_key("this")
                    && !self.locals.contains_key("self")
                {
                    "this".to_string()
                } else if i.sym == "this" || i.sym == "_" {
                    if self.is_state_machine {
                        "ctx.self".to_string()
                    } else {
                        "self".to_string()
                    }
                } else if i.sym == "true"
                    || i.sym == "false"
                    || i.sym == "null"
                    || i.sym.starts_with('@')
                    || i.sym.starts_with('[')
                    || i.sym.starts_with("__pipe_")
                    || i.sym.ends_with("_Context")
                    || self.is_comptime
                    || self.globals.contains(&i.sym)
                {
                    i.sym.clone()
                } else if self.locals.contains_key(&i.sym) {
                    let base = if self.is_state_machine
                        && !self.inline_closure_params.contains(&i.sym)
                        && !self.block_locals.contains(&i.sym)
                    {
                        format!("ctx.{}", i.sym)
                    } else {
                        i.sym.clone()
                    };
                    if self.mut_borrow_loop_vars.contains(&i.sym) {
                        format!("({}.*)", base)
                    } else if self.narrowed_null_vars.contains(&i.sym) {
                        format!("({}.?)", base)
                    } else {
                        base
                    }
                } else if self.mut_borrow_loop_vars.contains(&i.sym) {
                    format!("({}.*)", i.sym)
                } else if self.is_class_method && self.is_class_field(&i.sym) {
                    if self.moved_vars.contains("this") || self.moved_vars.contains("_") {
                        panic!(
                            "CP Compile Error: Use of moved value '_' when accessing field '{}'",
                            i.sym
                        );
                    }
                    let base = if self.is_state_machine {
                        format!("ctx.self.{}", i.sym)
                    } else {
                        format!("self.{}", i.sym)
                    };
                    if self.narrowed_null_vars.contains(&i.sym) {
                        format!("({}.?)", base)
                    } else {
                        base
                    }
                } else if self.local_trampolines.contains(&i.sym) {
                    format!("__cp_trampoline_{}.{}", i.sym, i.sym)
                } else if self.narrowed_null_vars.contains(&i.sym) {
                    format!("({}.?)", i.sym)
                } else {
                    i.sym.clone()
                }
            }
            Expr::Borrow(inner, is_mut) => {
                if let Expr::Ident(i) = &**inner {
                    if *is_mut
                        && (self.class_defs.contains_key(&i.sym)
                            || self.enum_defs.contains_key(&i.sym)
                            || self.trait_defs.contains_key(&i.sym)
                            || self.type_aliases.contains_key(&i.sym)
                            || self.type_namespace_vars.contains(&i.sym))
                    {
                        return format!("*{}", i.sym);
                    }
                    if self.local_trampolines.contains(&i.sym) {
                        return format!("&__cp_trampoline_{}.{}", i.sym, i.sym);
                    }
                    self.uninitialized_vars.remove(&i.sym);
                    if i.sym == "this" || i.sym == "_" || self.is_class_field(&i.sym) {
                        if *is_mut && self.current_receiver_mode == ReceiverMode::ReadOnly {
                            panic!(
                                "CP Compile Error: Cannot take mutable borrow '*' of '{}' in a read-only method (add '*_' as first parameter)",
                                i.sym
                            );
                        }
                        self.check_borrow_conflict("this", *is_mut);
                        self.check_borrow_conflict("_", *is_mut);
                    } else {
                        if *is_mut {
                            if let Some((_, is_m)) = self.var_borrow_sources.get(&i.sym) {
                                if !*is_m {
                                    panic!(
                                        "CP Compile Error: Cannot take mutable borrow '*{}' from read-only borrow",
                                        i.sym
                                    );
                                }
                            } else if self.current_fn_params.contains(&i.sym)
                                && !self.mutable_fn_params.contains(&i.sym)
                            {
                                panic!(
                                    "CP Compile Error: Cannot take mutable borrow '*{}' from read-only parameter (declare as '{}: *T')",
                                    i.sym, i.sym
                                );
                            }
                        }
                        self.check_borrow_conflict(&i.sym, *is_mut);
                    }
                }
                let inner_code = self.generate_expr(inner);
                let is_type_name = match &**inner {
                    Expr::Ident(id) => {
                        !self.locals.contains_key(&id.sym)
                            && !self.var_cp_types.contains_key(&id.sym)
                            && (self.class_defs.contains_key(&id.sym)
                                || self.value_classes.contains(&id.sym)
                                || self.type_aliases.contains_key(&id.sym)
                                || self.trait_defs.contains_key(&id.sym)
                                || id
                                    .sym
                                    .chars()
                                    .next()
                                    .map_or(false, |c| c.is_ascii_uppercase()))
                    }
                    Expr::Member(mem) => mem
                        .prop
                        .sym
                        .chars()
                        .next()
                        .map_or(false, |c| c.is_ascii_uppercase()),
                    _ => false,
                };
                if is_type_name {
                    return if *is_mut {
                        format!("*{}", inner_code)
                    } else {
                        format!("*const {}", inner_code)
                    };
                }
                let already_ptr = if let Some(ty) = self.infer_cp_type(inner) {
                    self.is_zig_heap_ptr_type(&ty)
                } else if matches!(&**inner, Expr::Member(_) | Expr::Index { .. }) {
                    false
                } else if let Expr::Ident(id) = &**inner {
                    if let Some(zig_ty) = self.locals.get(&id.sym) {
                        let clean_zig_ty = if self.narrowed_null_vars.contains(&id.sym) {
                            zig_ty.trim_start_matches('?')
                        } else {
                            zig_ty.as_str()
                        };
                        clean_zig_ty.starts_with('*') && clean_zig_ty != "*anyopaque"
                    } else if let Some(ty) = self.var_cp_types.get(&id.sym) {
                        let clean_ty = if self.narrowed_null_vars.contains(&id.sym) {
                            Self::unwrap_optional_type(ty)
                        } else {
                            ty.clone()
                        };
                        self.is_zig_heap_ptr_type(&clean_ty)
                    } else {
                        false
                    }
                } else {
                    false
                };
                if already_ptr {
                    inner_code
                } else {
                    format!("(&{})", inner_code)
                }
            }
            Expr::Move(expr) => match &**expr {
                Expr::Ident(i) => {
                    if i.sym == "this" || i.sym == "_" {
                        if self.current_receiver_mode != ReceiverMode::Move {
                            panic!(
                                "CP Compile Error: Cannot move '{}' in a non-move method (add 'move _' as first parameter)",
                                i.sym
                            );
                        }
                        self.check_active_borrows_before_move("this");
                        self.check_active_borrows_before_move("_");
                        let gen = self.generate_expr(expr);
                        self.moved_vars.insert("this".to_string());
                        self.moved_vars.insert("_".to_string());
                        if !self.is_state_machine {
                            format!("(blk: {{ __self_moved = true; break :blk {}; }})", gen)
                        } else {
                            gen
                        }
                    } else {
                        if self.var_borrow_sources.contains_key(&i.sym)
                            || (self.current_fn_params.contains(&i.sym)
                                && !self.move_fn_params.contains(&i.sym))
                        {
                            panic!(
                                "CP Compile Error: Cannot move borrowed reference or read-only parameter '{}'",
                                i.sym
                            );
                        }
                        self.check_active_borrows_before_move(&i.sym);
                        let gen = self.generate_expr(expr);
                        self.moved_vars.insert(i.sym.clone());
                        if !self.is_state_machine && self.drop_flag_vars.contains(&i.sym) {
                            format!("(blk: {{ __{}_moved = true; break :blk {}; }})", i.sym, gen)
                        } else {
                            gen
                        }
                    }
                }
                Expr::Member(_) => {
                    let (root_opt, field_opt) = self.extract_member_path(expr);
                    if let Some(root) = root_opt {
                        if root == "this" || root == "_" {
                            if self.current_receiver_mode == ReceiverMode::ReadOnly {
                                panic!(
                                    "CP Compile Error: Cannot move field '{}.{}' from read-only receiver (use '*_' or 'move _')",
                                    root,
                                    field_opt.as_deref().unwrap_or("")
                                );
                            }
                            self.check_borrow_conflict_field("this", field_opt.as_deref(), true);
                            self.check_borrow_conflict_field("_", field_opt.as_deref(), true);
                        } else {
                            if let Some((_, is_m)) = self.var_borrow_sources.get(&root) {
                                if !*is_m {
                                    panic!(
                                        "CP Compile Error: Cannot move field '{}.{}' out of read-only borrow '{}'",
                                        root,
                                        field_opt.as_deref().unwrap_or(""),
                                        root
                                    );
                                }
                            } else if self.current_fn_params.contains(&root)
                                && !self.mutable_fn_params.contains(&root)
                                && !self.move_fn_params.contains(&root)
                            {
                                panic!(
                                    "CP Compile Error: Cannot move field '{}.{}' out of read-only parameter '{}'",
                                    root,
                                    field_opt.as_deref().unwrap_or(""),
                                    root
                                );
                            }
                            self.check_borrow_conflict_field(&root, field_opt.as_deref(), true);
                        }
                    }
                    self.generate_expr(expr)
                }
                _ => self.generate_expr(expr),
            },
            Expr::Bin(bin) => {
                if bin.op == BinaryOp::Pipe {
                    return self.generate_pipeline(&bin.left, &bin.right);
                }
                if bin.op == BinaryOp::Assign {
                    if let Expr::Ident(id) = &*bin.left {
                        self.uninitialized_vars.remove(&id.sym);
                        if matches!(&*bin.right, Expr::Lit(Lit::Null))
                            || matches!(&*bin.right, Expr::Ident(rid) if rid.sym == "null")
                        {
                            self.narrowed_null_vars.remove(&id.sym);
                        }
                    } else if let Expr::Index { obj, .. } = &*bin.left {
                        if let Expr::Ident(id) = &**obj {
                            self.uninitialized_vars.remove(&id.sym);
                        }
                    }
                }
                let is_null_cmp = matches!(&*bin.left, Expr::Lit(Lit::Null))
                    || matches!(&*bin.right, Expr::Lit(Lit::Null))
                    || matches!(&*bin.left, Expr::Ident(id) if id.sym == "null")
                    || matches!(&*bin.right, Expr::Ident(id) if id.sym == "null");
                let l = if bin.op == BinaryOp::Assign {
                    if let Expr::Ident(id) = &*bin.left {
                        let was = self.narrowed_null_vars.remove(&id.sym);
                        let code = self.generate_expr(&bin.left);
                        if was {
                            self.narrowed_null_vars.insert(id.sym.clone());
                        }
                        code
                    } else {
                        self.generate_expr(&bin.left)
                    }
                } else if is_null_cmp {
                    let l_id = if let Expr::Ident(id) = &*bin.left {
                        Some(id.sym.clone())
                    } else {
                        None
                    };
                    let was_l = l_id
                        .as_ref()
                        .map_or(false, |s| self.narrowed_null_vars.remove(s));
                    let code = self.generate_expr(&bin.left);
                    if was_l {
                        self.narrowed_null_vars.insert(l_id.unwrap());
                    }
                    code
                } else {
                    self.generate_expr(&bin.left)
                };
                let r = if is_null_cmp {
                    let r_id = if let Expr::Ident(id) = &*bin.right {
                        Some(id.sym.clone())
                    } else {
                        None
                    };
                    let was_r = r_id
                        .as_ref()
                        .map_or(false, |s| self.narrowed_null_vars.remove(s));
                    let code = self.generate_expr(&bin.right);
                    if was_r {
                        self.narrowed_null_vars.insert(r_id.unwrap());
                    }
                    code
                } else if bin.op == BinaryOp::LogicalAnd {
                    let not_null_vars = Self::extract_not_null_vars(&bin.left);
                    let old_narrowed = self.narrowed_null_vars.clone();
                    self.narrowed_null_vars.extend(not_null_vars);
                    let code = self.generate_expr(&bin.right);
                    self.narrowed_null_vars = old_narrowed;
                    code
                } else {
                    self.generate_expr(&bin.right)
                };
                let l_ty = self.infer_cp_type(&bin.left);
                let r_ty = self.infer_cp_type(&bin.right);
                let are_both_numeric = match (&l_ty, &r_ty) {
                    (Some(t1), Some(t2)) => Self::is_numeric_type(t1) && Self::is_numeric_type(t2),
                    (Some(t1), None) => {
                        Self::is_numeric_type(t1)
                            && matches!(
                                &*bin.right,
                                Expr::Lit(Lit::Int(_))
                                    | Expr::Lit(Lit::Float(_))
                                    | Expr::Index { .. }
                            )
                    }
                    (None, Some(t2)) => {
                        Self::is_numeric_type(t2)
                            && matches!(
                                &*bin.left,
                                Expr::Lit(Lit::Int(_))
                                    | Expr::Lit(Lit::Float(_))
                                    | Expr::Index { .. }
                            )
                    }
                    _ => {
                        matches!(
                            &*bin.left,
                            Expr::Lit(Lit::Int(_)) | Expr::Lit(Lit::Float(_))
                        ) || matches!(
                            &*bin.right,
                            Expr::Lit(Lit::Int(_)) | Expr::Lit(Lit::Float(_))
                        ) || (matches!(&*bin.left, Expr::Index { .. })
                            && matches!(&*bin.right, Expr::Index { .. }))
                    }
                };
                let is_null_cmp = matches!(&*bin.left, Expr::Lit(Lit::Null))
                    || matches!(&*bin.right, Expr::Lit(Lit::Null))
                    || matches!(&*bin.left, Expr::Ident(id) if id.sym == "null")
                    || matches!(&*bin.right, Expr::Ident(id) if id.sym == "null");
                let is_enum_cmp = matches!(&*bin.left, Expr::EnumInit(_))
                    || matches!(&*bin.right, Expr::EnumInit(_))
                    || match (&l_ty, &r_ty) {
                        (Some(Type::Ref(id)), _) if self.enum_defs.contains_key(&id.sym) => true,
                        (_, Some(Type::Ref(id))) if self.enum_defs.contains_key(&id.sym) => true,
                        _ => false,
                    };
                let is_bool_cmp = match (&l_ty, &r_ty) {
                    (Some(Type::Bool), _) | (_, Some(Type::Bool)) => true,
                    _ => {
                        matches!(&*bin.left, Expr::Lit(Lit::Bool(_)))
                            || matches!(&*bin.right, Expr::Lit(Lit::Bool(_)))
                    }
                };
                let is_vec_ty = |t_opt: &Option<Type>| -> bool {
                    if let Some(Type::Ref(id)) = t_opt {
                        if id.sym.starts_with("@Vector(") {
                            return true;
                        }
                        if let Some(ta) = self.type_aliases.get(&id.sym) {
                            if let Type::Ref(tid) = &ta.target_ty {
                                return tid.sym.starts_with("@Vector(");
                            }
                        }
                    }
                    false
                };
                let is_vec_cmp = is_vec_ty(&l_ty) || is_vec_ty(&r_ty);
                match bin.op {
                    BinaryOp::Eq => {
                        let has_num = are_both_numeric
                            || matches!(
                                &*bin.left,
                                Expr::Lit(Lit::Int(_)) | Expr::Lit(Lit::Float(_))
                            )
                            || matches!(
                                &*bin.right,
                                Expr::Lit(Lit::Int(_)) | Expr::Lit(Lit::Float(_))
                            );
                        if self.is_lib_mode
                            || has_num
                            || is_null_cmp
                            || is_enum_cmp
                            || is_bool_cmp
                            || is_vec_cmp
                        {
                            format!("({} == {})", l, r)
                        } else {
                            format!("cp_eq({}, {})", l, r)
                        }
                    }
                    BinaryOp::NotEq => {
                        let has_num = are_both_numeric
                            || matches!(
                                &*bin.left,
                                Expr::Lit(Lit::Int(_)) | Expr::Lit(Lit::Float(_))
                            )
                            || matches!(
                                &*bin.right,
                                Expr::Lit(Lit::Int(_)) | Expr::Lit(Lit::Float(_))
                            );
                        if self.is_lib_mode
                            || has_num
                            || is_null_cmp
                            || is_enum_cmp
                            || is_bool_cmp
                            || is_vec_cmp
                        {
                            format!("({} != {})", l, r)
                        } else {
                            format!("cp_neq({}, {})", l, r)
                        }
                    }
                    BinaryOp::Add => {
                        let is_str_concat = matches!(l_ty, Some(Type::String))
                            || matches!(r_ty, Some(Type::String))
                            || matches!(&*bin.left, Expr::Lit(Lit::Str(_)))
                            || matches!(&*bin.right, Expr::Lit(Lit::Str(_)));
                        if self.is_lib_mode || !is_str_concat || are_both_numeric {
                            format!("({} + {})", l, r)
                        } else {
                            format!("cp_add({}, {})", l, r)
                        }
                    }
                    BinaryOp::Lt => {
                        let is_str_cmp = matches!(l_ty, Some(Type::String))
                            && matches!(r_ty, Some(Type::String));
                        if self.is_lib_mode || !is_str_cmp || are_both_numeric {
                            format!("({} < {})", l, r)
                        } else {
                            format!("cp_lt({}, {})", l, r)
                        }
                    }
                    BinaryOp::Gt => {
                        let is_str_cmp = matches!(l_ty, Some(Type::String))
                            && matches!(r_ty, Some(Type::String));
                        if self.is_lib_mode || !is_str_cmp || are_both_numeric {
                            format!("({} > {})", l, r)
                        } else {
                            format!("cp_gt({}, {})", l, r)
                        }
                    }
                    BinaryOp::LtEq => {
                        let is_str_cmp = matches!(l_ty, Some(Type::String))
                            && matches!(r_ty, Some(Type::String));
                        if self.is_lib_mode || !is_str_cmp || are_both_numeric {
                            format!("({} <= {})", l, r)
                        } else {
                            format!("cp_lte({}, {})", l, r)
                        }
                    }
                    BinaryOp::GtEq => {
                        let is_str_cmp = matches!(l_ty, Some(Type::String))
                            && matches!(r_ty, Some(Type::String));
                        if self.is_lib_mode || !is_str_cmp || are_both_numeric {
                            format!("({} >= {})", l, r)
                        } else {
                            format!("cp_gte({}, {})", l, r)
                        }
                    }
                    BinaryOp::Mod => format!("({} % {})", l, r),
                    BinaryOp::BitAnd => format!("({} & {})", l, r),
                    BinaryOp::BitOr => format!("({} | {})", l, r),
                    BinaryOp::BitXor => format!("({} ^ {})", l, r),
                    BinaryOp::Shl => format!("cp_shl({}, {})", l, r),
                    BinaryOp::Shr => format!("cp_shr({}, {})", l, r),
                    BinaryOp::LogicalAnd => format!("({} and {})", l, r),
                    BinaryOp::LogicalOr => match &*bin.right {
                        Expr::Continue(_) => {
                            let id = self.lambda_counter;
                            self.lambda_counter += 1;
                            format!(
                                "(blk_{id}: {{ if (!({})) continue; break :blk_{id} true; }})",
                                l
                            )
                        }
                        Expr::Break(_, val) => {
                            let id = self.lambda_counter;
                            self.lambda_counter += 1;
                            if let Some(v) = val {
                                let v_code = self.generate_expr(v);
                                format!(
                                    "(blk_{id}: {{ if (!({})) break {}; break :blk_{id} true; }})",
                                    l, v_code
                                )
                            } else {
                                format!(
                                    "(blk_{id}: {{ if (!({})) break; break :blk_{id} true; }})",
                                    l
                                )
                            }
                        }
                        Expr::Return(_, ret_expr) => {
                            let id = self.lambda_counter;
                            self.lambda_counter += 1;
                            let ret_stmt = self.build_ret_stmt_for_fallback(ret_expr.as_deref());
                            format!(
                                "(blk_{id}: {{ if (!({})) {{ {} }}; break :blk_{id} true; }})",
                                l, ret_stmt
                            )
                        }
                        _ => {
                            let is_errset = |s: &str| -> bool {
                                let t = s.trim().trim_start_matches('(').trim_end_matches(')');
                                if t.starts_with("error{")
                                    || t.starts_with("error {")
                                    || t.contains(" || ")
                                {
                                    return true;
                                }
                                if t.contains(' ')
                                    || t.contains("==")
                                    || t.contains("!=")
                                    || t.contains('<')
                                    || t.contains('>')
                                {
                                    return false;
                                }
                                if t == "OOM"
                                    || t == "anyerror"
                                    || t == "Error"
                                    || self.error_sets.contains_key(t)
                                {
                                    return true;
                                }
                                if let Some(last) = t.rsplit('.').next() {
                                    if last == "OOM"
                                        || last == "Error"
                                        || (last
                                            .chars()
                                            .next()
                                            .map_or(false, |c| c.is_ascii_uppercase())
                                            && last.ends_with("Error"))
                                        || self.error_sets.contains_key(last)
                                    {
                                        return true;
                                    }
                                }
                                false
                            };
                            if is_errset(&l) || is_errset(&r) {
                                format!("({} || {})", l, r)
                            } else {
                                format!("({} or {})", l, r)
                            }
                        }
                    },
                    BinaryOp::Assign => {
                        if let Expr::Ident(id) = &*bin.left {
                            if id.sym == "_" {
                                format!("_ = {}", r)
                            } else {
                                format!("{} = {}", l, r)
                            }
                        } else {
                            format!("{} = {}", l, r)
                        }
                    }
                    _ => {
                        let op = match bin.op {
                            BinaryOp::Sub => "-",
                            BinaryOp::Mul => "*",
                            BinaryOp::Div => "/",
                            BinaryOp::WrappingAdd => "+%",
                            BinaryOp::WrappingSub => "-%",
                            BinaryOp::WrappingMul => "*%",
                            BinaryOp::SaturatingAdd => "+|",
                            BinaryOp::SaturatingSub => "-|",
                            BinaryOp::SaturatingMul => "*|",
                            BinaryOp::SaturatingShl => "<<|",
                            BinaryOp::Concat => "++",
                            _ => "",
                        };
                        format!("({} {} {})", l, op, r)
                    }
                }
            }
            Expr::Cast {
                expr,
                target_ty,
                span,
            } => {
                if let Expr::Ident(id) = &**expr {
                    self.uninitialized_vars.remove(&id.sym);
                }
                let resolved_ty = if let Type::RawPtr { inner, .. } = target_ty {
                    if **inner == Type::Any {
                        self.infer_cp_type(&Expr::Cast {
                            span: *span,
                            expr: expr.clone(),
                            target_ty: target_ty.clone(),
                        })
                        .unwrap_or_else(|| target_ty.clone())
                    } else {
                        target_ty.clone()
                    }
                } else {
                    target_ty.clone()
                };
                let target_zig_ty = self.map_type(&resolved_ty);
                match &**expr {
                    Expr::Borrow(inner, _) => {
                        let inner_code = self.generate_expr(inner);
                        let is_class_ref = if let Expr::Ident(id) = &**inner {
                            if let Some(ty) = self.var_cp_types.get(&id.sym) {
                                self.is_zig_heap_ptr_type(ty)
                            } else {
                                false
                            }
                        } else {
                            false
                        };
                        if is_class_ref && matches!(resolved_ty, Type::RawPtr { depth: 1, .. }) {
                            format!(
                                "@as({}, @ptrCast(@alignCast({})))",
                                target_zig_ty, inner_code
                            )
                        } else {
                            format!(
                                "@as({}, @ptrCast(@alignCast(&({}))))",
                                target_zig_ty, inner_code
                            )
                        }
                    }
                    _ => {
                        let inner_code = self.generate_expr(expr);
                        let src_ty = self.infer_cp_type(expr);
                        let is_src_float = matches!(src_ty, Some(Type::F32 | Type::F64));
                        let is_src_int = matches!(
                            src_ty,
                            Some(
                                Type::U8
                                    | Type::I8
                                    | Type::U16
                                    | Type::I16
                                    | Type::I32
                                    | Type::U32
                                    | Type::I64
                                    | Type::U64
                                    | Type::U128
                                    | Type::I128
                                    | Type::ISize
                                    | Type::USize
                            )
                        );
                        let is_src_ptr = matches!(
                            src_ty,
                            Some(
                                Type::RawPtr { .. }
                                    | Type::ManyRawPtr(_)
                                    | Type::CStr
                                    | Type::BorrowType { .. }
                            )
                        );
                        let is_target_int = matches!(
                            resolved_ty,
                            Type::U8
                                | Type::I8
                                | Type::U16
                                | Type::I16
                                | Type::I32
                                | Type::U32
                                | Type::I64
                                | Type::U64
                                | Type::U128
                                | Type::I128
                                | Type::ISize
                                | Type::USize
                        );
                        let is_target_float = matches!(resolved_ty, Type::F32 | Type::F64);
                        let is_target_ptr = matches!(
                            resolved_ty,
                            Type::RawPtr { .. }
                                | Type::ManyRawPtr(_)
                                | Type::CStr
                                | Type::BorrowType { .. }
                        );
                        let is_src_enum = if let Some(Type::Ref(id)) = &src_ty {
                            self.enum_defs.contains_key(&id.sym)
                        } else {
                            false
                        };
                        let is_target_enum = if let Type::Ref(id) = &resolved_ty {
                            self.enum_defs.contains_key(&id.sym)
                        } else {
                            false
                        };
                        let is_target_fn = matches!(resolved_ty, Type::Fn(_, _) | Type::CFn(_, _));

                        if is_target_int {
                            if is_src_float {
                                format!("@as({}, @intFromFloat({}))", target_zig_ty, inner_code)
                            } else if is_src_ptr {
                                if target_zig_ty == "usize" {
                                    format!("@intFromPtr({})", inner_code)
                                } else {
                                    format!(
                                        "cp_cast_int({}, @intFromPtr({}))",
                                        target_zig_ty, inner_code
                                    )
                                }
                            } else if is_src_enum {
                                format!(
                                    "cp_cast_int({}, @intFromEnum({}))",
                                    target_zig_ty, inner_code
                                )
                            } else if inner_code.starts_with("@intCast(")
                                || inner_code.starts_with("@truncate(")
                                || inner_code.starts_with("@bitCast(")
                                || inner_code.starts_with("@enumFromInt(")
                                || inner_code.starts_with("@ptrFromInt(")
                                || inner_code.starts_with("@ptrCast(")
                                || inner_code.starts_with("@floatCast(")
                                || inner_code.starts_with("@intFromFloat(")
                            {
                                format!("@as({}, {})", target_zig_ty, inner_code)
                            } else if self.is_comptime {
                                format!(
                                    "(if (@typeInfo(@TypeOf({0})) == .@\"enum\") cp_cast_int({1}, @intFromEnum({0})) else cp_cast_int({1}, {0}))",
                                    inner_code, target_zig_ty
                                )
                            } else {
                                format!(
                                    "(if (comptime @typeInfo(@TypeOf({0})) == .@\"enum\") cp_cast_int({1}, @intFromEnum({0})) else cp_cast_int({1}, {0}))",
                                    inner_code, target_zig_ty
                                )
                            }
                        } else if is_target_float {
                            if inner_code.starts_with("@bitCast(")
                                || inner_code.starts_with("@floatFromInt(")
                            {
                                format!("@as({}, {})", target_zig_ty, inner_code)
                            } else if is_src_int {
                                format!("@as({}, @floatFromInt({}))", target_zig_ty, inner_code)
                            } else {
                                format!("@as({}, @floatCast({}))", target_zig_ty, inner_code)
                            }
                        } else if is_target_ptr {
                            if is_src_int {
                                format!(
                                    "@as({}, @ptrFromInt(@as(usize, @truncate({}))))",
                                    target_zig_ty, inner_code
                                )
                            } else if matches!(
                                src_ty,
                                Some(Type::Array(_)) | Some(Type::FixedArray(_, _))
                            ) || matches!(src_ty, Some(Type::Ref(ref id)) if id.sym.starts_with('['))
                                || self.infer_zig_type(None, Some(expr)).starts_with('[')
                                || self
                                    .locals
                                    .get(&inner_code)
                                    .map_or(false, |l| l.starts_with('['))
                            {
                                format!("@as({}, @ptrCast(&({})))", target_zig_ty, inner_code)
                            } else if matches!(src_ty, Some(Type::String)) {
                                format!("@as({}, @ptrCast({}.ptr))", target_zig_ty, inner_code)
                            } else if inner_code.starts_with("@ptrCast(") {
                                format!("@as({}, @alignCast({}))", target_zig_ty, inner_code)
                            } else {
                                format!(
                                    "@as({}, @ptrCast(@alignCast({})))",
                                    target_zig_ty, inner_code
                                )
                            }
                        } else if is_target_enum {
                            if inner_code.starts_with("@enumFromInt(") {
                                format!("@as({}, {})", target_zig_ty, inner_code)
                            } else {
                                format!("@as({}, @enumFromInt({}))", target_zig_ty, inner_code)
                            }
                        } else if is_target_fn {
                            format!("@as({}, @ptrCast({}))", target_zig_ty, inner_code)
                        } else if matches!(resolved_ty, Type::Any) {
                            format!(
                                "@as({}, @ptrCast(@alignCast({})))",
                                target_zig_ty, inner_code
                            )
                        } else if matches!(resolved_ty, Type::Ref(_))
                            && !matches!(resolved_ty, Type::Ref(ref id) if self.class_defs.contains_key(&id.sym) || self.trait_defs.contains_key(&id.sym))
                        {
                            if inner_code.starts_with("@enumFromInt(")
                                || inner_code.starts_with("@truncate(")
                                || inner_code.starts_with("@intCast(")
                                || inner_code.starts_with("@bitCast(")
                                || inner_code.starts_with("@ptrCast(")
                                || inner_code.starts_with("@ptrFromInt(")
                            {
                                format!("@as({}, {})", target_zig_ty, inner_code)
                            } else if self.is_comptime {
                                format!(
                                    "(if (@typeInfo({0}) == .@\"enum\") @as({0}, @enumFromInt({1})) else @as({0}, {1}))",
                                    target_zig_ty, inner_code
                                )
                            } else {
                                format!(
                                    "(if (comptime @typeInfo({0}) == .@\"enum\") @as({0}, @enumFromInt({1})) else @as({0}, {1}))",
                                    target_zig_ty, inner_code
                                )
                            }
                        } else if resolved_ty == Type::Bool {
                            if self.is_comptime {
                                format!(
                                    "(if (@typeInfo(@TypeOf({0})) == .bool) {0} else ({0} != 0))",
                                    inner_code
                                )
                            } else {
                                format!("(if (comptime @typeInfo(@TypeOf({0})) == .bool) {0} else ({0} != 0))", inner_code)
                            }
                        } else {
                            format!("@as({}, {})", target_zig_ty, inner_code)
                        }
                    }
                }
            }
            Expr::BitNot(inner) => {
                let inner_code = self.generate_expr(inner);
                format!("(~{})", inner_code)
            }
            Expr::Deref(inner) => {
                let inner_code = self.generate_expr(inner);
                if matches!(&**inner, Expr::Ident(_) | Expr::Member(_) | Expr::Deref(_)) {
                    format!("{}.*", inner_code)
                } else {
                    format!("({}).*", inner_code)
                }
            }
            Expr::Unwrap(inner) => {
                if let Expr::Ident(id) = &**inner {
                    if self.narrowed_null_vars.contains(&id.sym) {
                        let was = self.narrowed_null_vars.remove(&id.sym);
                        let inner_code = self.generate_expr(inner);
                        if was {
                            self.narrowed_null_vars.insert(id.sym.clone());
                        }
                        return if matches!(
                            &**inner,
                            Expr::Ident(_) | Expr::Member(_) | Expr::Deref(_)
                        ) {
                            format!("{}.?", inner_code)
                        } else {
                            format!("({}).?", inner_code)
                        };
                    }
                }
                let inner_code = self.generate_expr(inner);
                if matches!(&**inner, Expr::Ident(_) | Expr::Member(_) | Expr::Deref(_)) {
                    format!("{}.?", inner_code)
                } else {
                    format!("({}).?", inner_code)
                }
            }
            _ => unreachable!(),
        }
    }

    pub(crate) fn expr_to_zig_pure(&self, expr: &Expr) -> String {
        match expr {
            Expr::Ident(id) => id.sym.clone(),
            Expr::Lit(lit) => match lit {
                crate::ast::Lit::Int(n) => n.to_string(),
                crate::ast::Lit::Float(f) => f.to_string(),
                crate::ast::Lit::Str(s) => format!("\"{}\"", s),
                crate::ast::Lit::Bool(b) => b.to_string(),
                crate::ast::Lit::Null => "null".to_string(),
                crate::ast::Lit::Undefined => "undefined".to_string(),
            },
            Expr::Member(mem) => {
                format!("{}.{}", self.expr_to_zig_pure(&mem.obj), mem.prop.sym)
            }
            Expr::Bin(bin) => {
                let op_str = match bin.op {
                    BinaryOp::Eq => "==",
                    BinaryOp::NotEq => "!=",
                    BinaryOp::Lt => "<",
                    BinaryOp::LtEq => "<=",
                    BinaryOp::Gt => ">",
                    BinaryOp::GtEq => ">=",
                    BinaryOp::LogicalAnd => "and",
                    BinaryOp::LogicalOr => "or",
                    _ => "==",
                };
                format!(
                    "({} {} {})",
                    self.expr_to_zig_pure(&bin.left),
                    op_str,
                    self.expr_to_zig_pure(&bin.right)
                )
            }
            Expr::Unwrap(inner) => {
                format!("{}.?", self.expr_to_zig_pure(inner))
            }
            _ => "true".to_string(),
        }
    }
}
