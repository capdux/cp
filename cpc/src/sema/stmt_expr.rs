use super::*;
use crate::ast::*;
use crate::diagnostic::{find_closest_match, Diagnostic, STANDARD_BUILTIN_IMPORTS};
use crate::Span;
use std::collections::HashMap;

impl Analyzer {
    /// Returns `(type_name, dtor_kind)` if `ty` represents an owned value (not a borrow/raw pointer)
    /// whose struct/class defines `~init()` (`__cp_deinit`) or `~errinit()` (`__cp_errdeinit`).
    pub(super) fn find_owned_raii_destructor(&self, ty: &Type) -> Option<(String, String)> {
        match ty {
            Type::BorrowType { .. }
            | Type::RawPtr { .. }
            | Type::ManyRawPtr(_)
            | Type::CPtr(_) => None,
            Type::Optional(inner) | Type::ErrorUnion(inner) => {
                self.find_owned_raii_destructor(inner)
            }
            Type::ResultType { ok, .. } => self.find_owned_raii_destructor(ok),
            Type::Ref(id) => self.lookup_class_destructor(&id.sym),
            _ => None,
        }
    }

    fn lookup_class_destructor(&self, raw_name: &str) -> Option<(String, String)> {
        let base_name = raw_name.split('<').next().unwrap_or(raw_name).trim();
        let short_name = base_name.rsplit('.').next().unwrap_or(base_name);
        let cls = self
            .classes
            .get(base_name)
            .or_else(|| self.classes.get(short_name))?;
        let has_deinit = cls
            .methods
            .iter()
            .any(|m| !m.is_static && m.ident.sym == "__cp_deinit");
        let has_errdeinit = cls
            .methods
            .iter()
            .any(|m| !m.is_static && m.ident.sym == "__cp_errdeinit");
        if has_deinit && has_errdeinit {
            Some((cls.ident.sym.clone(), "~init() / ~errinit()".to_string()))
        } else if has_deinit {
            Some((cls.ident.sym.clone(), "~init()".to_string()))
        } else if has_errdeinit {
            Some((cls.ident.sym.clone(), "~errinit()".to_string()))
        } else {
            None
        }
    }

    pub(super) fn analyze_stmt(&mut self, stmt: &Stmt) -> SemaResult<()> {
        match stmt {
            Stmt::Var(var) => {
                if var.is_extern {
                    let ty = var.ty.clone().unwrap_or(Type::Unknown);
                    self.analyze_pattern(&var.pat, ty.clone(), &var.kind)?;
                    return Ok(());
                }
                let mut inferred_ty = Type::Unknown;
                if let Some(init) = &var.init {
                    inferred_ty = self.infer_type(init)?;
                    self.analyze_expr(init)?;
                }
                if let Some(ref target_ty) = var.ty {
                    if let Some(init) = &var.init {
                        if !self.can_coerce(target_ty, &inferred_ty, Some(init)) {
                            return Err(SemaError::TypeMismatch(
                                target_ty.clone(),
                                inferred_ty,
                                var.span,
                            ));
                        }
                    }
                }
                let ty = var.ty.clone().unwrap_or(inferred_ty);
                // Reject discarding an owned RAII value (type with ~init() or ~errinit()) via `* = expr;`
                // unless `expr` is merely a named variable reference (e.g. `* = existing_guard;` used to silence unused warnings).
                if matches!(var.pat, Pattern::Wildcard) {
                    if let Some(init) = &var.init {
                        let is_plain_ident = matches!(init, Expr::Ident(_));
                        if !is_plain_ident {
                            if let Some((raii_name, dtor_kind)) = self.find_owned_raii_destructor(&ty) {
                                return Err(SemaError::DiscardedRaiiValue {
                                    type_name: raii_name,
                                    dtor_kind,
                                    span: var.span,
                                });
                            }
                        }
                    }
                }
                self.analyze_pattern(&var.pat, ty.clone(), &var.kind)?;
                if let Pattern::Ident(ident) = &var.pat {
                    let var_name = ident.sym.clone();
                    if matches!(
                        ty,
                        Type::BorrowType { .. }
                            | Type::RawPtr { .. }
                            | Type::ManyRawPtr(_)
                            | Type::CPtr(_)
                    ) || var.init.as_ref().map_or(false, |init| {
                        matches!(init, Expr::Borrow(..) | Expr::Slice(..))
                    }) {
                        self.local_borrow_vars.insert(var_name.clone());
                    }
                    if let Some(known_len) = self.infer_known_length(&ty, var.init.as_ref()) {
                        self.set_known_length(var_name.clone(), known_len);
                    }
                    if var.kind == VarDeclKind::Const {
                        if let Some(init) = &var.init {
                            if let Some(val) = self.eval_int_expr(init) {
                                self.set_const_int(var_name.clone(), val);
                            }
                            if let Expr::Builtin(call) = init {
                                if call.name == "@Vector" {
                                    self.type_aliases.insert(
                                        var_name,
                                        Type::Ref(Ident {
                                            span: var.span,
                                            sym: "@Vector(alias)".to_string(),
                                        }),
                                    );
                                }
                            } else if let Expr::TypeVal { ty: alias_ty, .. } = init {
                                self.type_aliases.insert(var_name, alias_ty.clone());
                            }
                        }
                    }
                }
            }
            Stmt::Expr(expr) => {
                self.infer_type(expr)?;
                self.analyze_expr(expr)?;
            }
            Stmt::Block(block) => self.analyze_block(block)?,
            Stmt::ComptimeBlock(block) => self.analyze_block(block)?,
            Stmt::UnsafeBlock(block) => {
                self.unsafe_depth += 1;
                let res = self.analyze_block(block);
                self.unsafe_depth -= 1;
                res?;
            }
            Stmt::ArenaBlock(block) => self.analyze_block(block)?,
            Stmt::Fuzz(f) => {
                self.enter_scope();
                let p_ty = f
                    .param_type
                    .clone()
                    .unwrap_or(Type::Array(Box::new(Type::U8)));
                let p_kind = SymbolKind::Var {
                    ty: p_ty,
                    kind: VarDeclKind::Const,
                };
                let _ = self.define(f.param_name.clone(), p_kind, f.span);
                let mut b = f.body.clone();
                self.analyze_block(&mut b)?;
                self.exit_scope();
            }
            Stmt::If(i) => {
                self.infer_type(&i.test)?;
                self.analyze_expr(&i.test)?;
                let not_null_vars = Self::extract_not_null_vars(&i.test);
                let is_null_vars = Self::extract_is_null_vars(&i.test);
                if let Some(cap) = &i.capture {
                    self.enter_scope();
                    self.define(
                        cap.sym.clone(),
                        SymbolKind::Var {
                            ty: Type::Any,
                            kind: if i.is_ptr_capture {
                                VarDeclKind::Let
                            } else {
                                VarDeclKind::Const
                            },
                        },
                        cap.span,
                    )?;
                    self.analyze_stmt(&i.cons)?;
                    self.exit_scope();
                } else if !not_null_vars.is_empty() {
                    self.enter_scope();
                    for v in &not_null_vars {
                        if let Some(SymbolKind::Var { ty, kind }) = self.resolve(v).cloned() {
                            let unwrapped = Self::unwrap_optional_type(&ty);
                            self.define(
                                v.clone(),
                                SymbolKind::Var {
                                    ty: unwrapped,
                                    kind,
                                },
                                i.span,
                            )?;
                        }
                    }
                    self.analyze_stmt(&i.cons)?;
                    self.exit_scope();
                } else {
                    self.analyze_stmt(&i.cons)?;
                }
                if let Some(alt) = &i.alt {
                    if !is_null_vars.is_empty() {
                        self.enter_scope();
                        for v in &is_null_vars {
                            if let Some(SymbolKind::Var { ty, kind }) = self.resolve(v).cloned() {
                                let unwrapped = Self::unwrap_optional_type(&ty);
                                self.define(
                                    v.clone(),
                                    SymbolKind::Var {
                                        ty: unwrapped,
                                        kind,
                                    },
                                    i.span,
                                )?;
                            }
                        }
                        self.analyze_stmt(alt)?;
                        self.exit_scope();
                    } else {
                        self.analyze_stmt(alt)?;
                    }
                }
            }
            Stmt::While(w) => {
                self.infer_type(&w.test)?;
                self.analyze_expr(&w.test)?;
                let not_null_vars = Self::extract_not_null_vars(&w.test);
                if let Some(cap) = &w.capture {
                    self.enter_scope();
                    self.define(
                        cap.sym.clone(),
                        SymbolKind::Var {
                            ty: Type::Any,
                            kind: if w.is_ptr_capture {
                                VarDeclKind::Let
                            } else {
                                VarDeclKind::Const
                            },
                        },
                        cap.span,
                    )?;
                    if let Some(c) = &w.continue_expr {
                        self.infer_type(c)?;
                        self.analyze_expr(c)?;
                    }
                    self.analyze_stmt(&w.body)?;
                    self.exit_scope();
                } else if !not_null_vars.is_empty() {
                    self.enter_scope();
                    for v in &not_null_vars {
                        if let Some(SymbolKind::Var { ty, kind }) = self.resolve(v).cloned() {
                            let unwrapped = Self::unwrap_optional_type(&ty);
                            self.define(
                                v.clone(),
                                SymbolKind::Var {
                                    ty: unwrapped,
                                    kind,
                                },
                                w.span,
                            )?;
                        }
                    }
                    if let Some(c) = &w.continue_expr {
                        self.infer_type(c)?;
                        self.analyze_expr(c)?;
                    }
                    self.analyze_stmt(&w.body)?;
                    self.exit_scope();
                } else {
                    if let Some(c) = &w.continue_expr {
                        self.infer_type(c)?;
                        self.analyze_expr(c)?;
                    }
                    self.analyze_stmt(&w.body)?;
                }
            }
            Stmt::For(f) => {
                if f.raw_header.is_some() {
                    self.enter_scope();
                    if let Pattern::Ident(id) = &f.pat {
                        for part in id.sym.split(',') {
                            let clean_name = part.trim().trim_start_matches('*').trim();
                            if !clean_name.is_empty() {
                                let _ = self.define(
                                    clean_name.to_string(),
                                    SymbolKind::Var {
                                        ty: Type::Any,
                                        kind: VarDeclKind::Let,
                                    },
                                    id.span,
                                );
                            }
                        }
                    }
                    let res = match &*f.body {
                        Stmt::Block(b) => {
                            let mut r = Ok(());
                            for s in &b.body {
                                if let Err(e) = self.analyze_stmt(s) {
                                    r = Err(e);
                                    break;
                                }
                            }
                            r
                        }
                        other => self.analyze_stmt(other),
                    };
                    self.exit_scope();
                    return res;
                }
                let coll_ty = self.infer_type(&f.expr)?;
                self.analyze_expr(&f.expr)?;

                let coll_var_opt = if let Expr::Ident(id) = &f.expr {
                    if self.moved_vars.contains(&id.sym) {
                        return Err(SemaError::UseOfMovedValue(id.sym.clone(), id.span));
                    }
                    if f.mode == IterMode::Move {
                        self.moved_vars.insert(id.sym.clone());
                    }
                    Some(id.sym.clone())
                } else {
                    None
                };

                let elem_ty = match &coll_ty {
                    Type::Array(inner) => (**inner).clone(),
                    _ => Type::Any,
                };

                if let Some(ref coll_name) = coll_var_opt {
                    if f.mode != IterMode::Move {
                        self.loop_borrowed_vars.insert(coll_name.clone());
                    }
                }

                self.enter_scope();
                let bind_res = self.bind_for_pattern(&f.pat, &elem_ty);
                let res = match bind_res {
                    Ok(()) => match &*f.body {
                        Stmt::Block(b) => {
                            let mut r = Ok(());
                            for s in &b.body {
                                if let Err(e) = self.analyze_stmt(s) {
                                    r = Err(e);
                                    break;
                                }
                            }
                            r
                        }
                        other => self.analyze_stmt(other),
                    },
                    Err(e) => Err(e),
                };
                self.exit_scope();

                if let Some(ref coll_name) = coll_var_opt {
                    if f.mode != IterMode::Move {
                        self.loop_borrowed_vars.remove(coll_name);
                    }
                }
                res?;
            }
            Stmt::Return(r) => {
                if let Some(arg) = &r.arg {
                    self.infer_type(arg)?;
                    self.analyze_expr(arg)?;
                }
            }
            Stmt::Break | Stmt::Continue => {}
            Stmt::Match(m) => {
                self.infer_type(&m.disc)?;
                self.analyze_expr(&m.disc)?;
                for arm in &m.arms {
                    self.enter_scope();
                    self.bind_match_pattern(&arm.pat)?;
                    if let Some(cap) = &arm.capture {
                        let _ = self.define(
                            cap.sym.clone(),
                            SymbolKind::Var {
                                ty: Type::Any,
                                kind: VarDeclKind::Let,
                            },
                            cap.span,
                        );
                    }
                    if let Some(g) = &arm.guard {
                        self.infer_type(g)?;
                        self.analyze_expr(g)?;
                    }
                    match &*arm.body {
                        Stmt::Block(b) => {
                            for s in &b.body {
                                self.analyze_stmt(s)?;
                            }
                        }
                        other => self.analyze_stmt(other)?,
                    }
                    self.exit_scope();
                }
            }
            Stmt::TryCatch {
                try_block,
                err_ident,
                catch_body,
                ..
            } => {
                self.analyze_block(try_block)?;
                if let Some(cb) = catch_body {
                    self.enter_scope();
                    if let Some(cv) = err_ident {
                        let _ = self.define(
                            cv.sym.clone(),
                            SymbolKind::Var {
                                ty: Type::Any,
                                kind: VarDeclKind::Let,
                            },
                            cv.span,
                        );
                    }
                    self.analyze_stmt(cb)?;
                    self.exit_scope();
                }
            }
            Stmt::Func(f) => {
                let params = f.params.iter().map(|p| p.ty.clone()).collect();
                let kind = SymbolKind::Func {
                    params,
                    ret: f.return_type.clone(),
                    is_async: f.is_async,
                    is_unsafe: f.is_unsafe,
                };
                let _ = self.define(f.ident.sym.clone(), kind.clone(), f.ident.span);
                self.symbol_map.insert(f.ident.span, kind);

                let old_async = self.current_fn_is_async;
                let old_unsafe = self.current_fn_is_unsafe;
                self.current_fn_is_async = f.is_async;
                self.current_fn_is_unsafe = f.is_unsafe;
                self.enter_scope();
                for p in &f.params {
                    let p_kind = SymbolKind::Var {
                        ty: p.ty.clone(),
                        kind: VarDeclKind::Let,
                    };
                    let _ = self.define(p.ident.sym.clone(), p_kind.clone(), p.ident.span);
                    self.symbol_map.insert(p.ident.span, p_kind);
                }
                for stmt in &f.body.body {
                    if let Err(e) = self.analyze_stmt(stmt) {
                        self.errors.push(e);
                    }
                }
                self.exit_scope();
                self.current_fn_is_async = old_async;
                self.current_fn_is_unsafe = old_unsafe;
            }
            _ => {}
        }
        Ok(())
    }

    pub(super) fn analyze_expr(&mut self, expr: &Expr) -> SemaResult<()> {
        match expr {
            Expr::Ident(ident) => {
                if self.moved_vars.contains(&ident.sym) {
                    return Err(SemaError::UseOfMovedValue(ident.sym.clone(), ident.span));
                }
                if let Some(kind) = self.resolve(&ident.sym) {
                    self.symbol_map.insert(ident.span, kind.clone());
                }
            }
            Expr::Bin(bin) => {
                if bin.op == BinaryOp::Assign {
                    if let Expr::Ident(id) = &*bin.left {
                        if id.sym == "_" {
                            self.analyze_expr(&bin.right)?;
                            return Ok(());
                        }
                        if let Some(ro_set) = self.fn_param_vars.last() {
                            if ro_set.contains(&id.sym) {
                                return Err(SemaError::ParameterReassignment(
                                    id.sym.clone(),
                                    bin.span,
                                ));
                            }
                        }
                        if let Some(SymbolKind::Var {
                            kind: VarDeclKind::Const,
                            ..
                        }) = self.resolve(&id.sym)
                        {
                            return Err(SemaError::ConstAssignment(id.sym.clone(), bin.span));
                        }
                        if self.loop_borrowed_vars.contains(&id.sym) {
                            return Err(SemaError::MutateWhileBorrowedInFor(
                                id.sym.clone(),
                                id.span,
                            ));
                        }
                    } else if let Expr::Member(mem) = &*bin.left {
                        if let Expr::Ident(obj_id) = &*mem.obj {
                            if (obj_id.sym == "_" || obj_id.sym == "this")
                                && !self.current_method_is_mut
                                && self.current_class.is_some()
                            {
                                return Err(SemaError::CannotMutateReadOnlyBorrow(
                                    "_".to_string(),
                                    bin.span,
                                ));
                            }
                        }
                    } else if let Expr::Index { obj, index } = &*bin.left {
                        if let Expr::Ident(id) = &**obj {
                            if let Some(SymbolKind::Var {
                                kind: VarDeclKind::Const,
                                ..
                            }) = self.resolve(&id.sym)
                            {
                                return Err(SemaError::ConstAssignment(id.sym.clone(), bin.span));
                            }
                        }
                        self.check_index_expr(obj, index, true, bin.span)?;
                    }
                }
                self.analyze_expr(&bin.left)?;
                if bin.op == BinaryOp::Pipe {
                    let transformed = transform_pipeline_rhs(&bin.right, "__pipe_mock");
                    self.analyze_expr(&transformed)?;
                } else if bin.op == BinaryOp::LogicalAnd {
                    let not_null_vars = Self::extract_not_null_vars(&bin.left);
                    if !not_null_vars.is_empty() {
                        self.enter_scope();
                        for v in &not_null_vars {
                            if let Some(SymbolKind::Var { ty, kind }) = self.resolve(v).cloned() {
                                let unwrapped = Self::unwrap_optional_type(&ty);
                                let _ = self.define(
                                    v.clone(),
                                    SymbolKind::Var {
                                        ty: unwrapped,
                                        kind,
                                    },
                                    bin.span,
                                );
                            }
                        }
                        self.analyze_expr(&bin.right)?;
                        self.exit_scope();
                    } else {
                        self.analyze_expr(&bin.right)?;
                    }
                } else {
                    self.analyze_expr(&bin.right)?;
                }
            }
            Expr::PipePlaceholder(span) => {
                return Err(SemaError::PipePlaceholderOutsidePipeline(*span));
            }
            Expr::Call(call) | Expr::OptCall(call) => {
                self.check_async_isolation_for_call(call)?;
                if let Expr::Ident(fid) = &*call.callee {
                    if fid.sym == "num" || fid.sym == "Number" {
                        return Err(SemaError::InvalidCast(
                            "CP does not support 'num()' or 'Number()'. CP is strongly typed: use specific numeric types like i32(x), u32(x), usize(x), or f64(x).".to_string(),
                            call.span,
                        ));
                    }
                    if let Some(SymbolKind::Func {
                        is_unsafe: true, ..
                    }) = self.resolve(&fid.sym)
                    {
                        if !self.is_in_unsafe() {
                            return Err(SemaError::UnsafeRequired(
                                format!(
                                    "Call to unsafe function '{}' requires an unsafe block or unsafe function",
                                    fid.sym
                                ),
                                call.span,
                            ));
                        }
                    }
                }
                if let Expr::Member(mem) = &*call.callee {
                    if let Ok(obj_ty) = self.infer_type(&mem.obj) {
                        let base_ty = match &obj_ty {
                            Type::BorrowType { inner, .. } | Type::RawPtr { inner, .. } => {
                                inner.as_ref()
                            }
                            other => other,
                        };
                        if let Type::Ref(cid) = base_ty {
                            if let Some(cls) = self.classes.get(&cid.sym) {
                                if let Some(m) =
                                    cls.methods.iter().find(|m| m.ident.sym == mem.prop.sym)
                                {
                                    let is_lifecycle = m.ident.sym == "init"
                                        || m.ident.sym == "release"
                                        || m.ident.sym == "_create"
                                        || m.ident.sym.starts_with("__");
                                    if !m.is_pub
                                        && !is_lifecycle
                                        && self.current_class.as_deref() != Some(&cid.sym)
                                    {
                                        return Err(SemaError::PrivateAccess(
                                            format!(
                                                "Cannot call private method '{}' of class '{}'",
                                                mem.prop.sym, cid.sym
                                            ),
                                            mem.prop.span,
                                        ));
                                    }
                                }
                            }
                        }
                    }
                    if let Expr::Ident(id) = &*mem.obj {
                        if self.loop_borrowed_vars.contains(&id.sym) {
                            let prop = mem.prop.sym.as_str();
                            if prop == "push"
                                || prop == "push_back"
                                || prop == "push_front"
                                || prop == "clear"
                                || prop == "delete"
                                || prop == "pop"
                                || prop == "pop_back"
                                || prop == "pop_front"
                                || prop == "set"
                                || prop == "put_all"
                                || prop == "add"
                                || prop == "insert"
                                || prop == "remove"
                            {
                                return Err(SemaError::MutateWhileBorrowedInFor(
                                    id.sym.clone(),
                                    mem.prop.span,
                                ));
                            }
                        }
                    }
                }
                // Generic trait bounds checking
                if let Expr::Ident(id) = &*call.callee {
                    if let Some(func) = self.functions.get(&id.sym).cloned() {
                        if !func.generic_params.is_empty() {
                            for gp in &func.generic_params {
                                if let GenericParam::Type {
                                    ident: gp_id,
                                    bounds,
                                } = gp
                                {
                                    if !bounds.is_empty() {
                                        for (idx, p) in func.params.iter().enumerate() {
                                            let mut p_ty = &p.ty;
                                            while let Type::BorrowType { inner, .. } = p_ty {
                                                p_ty = inner;
                                            }
                                            if let Type::Ref(pid) = p_ty {
                                                if pid.sym == gp_id.sym && idx < call.args.len() {
                                                    if let Ok(arg_ty) =
                                                        self.infer_type(&call.args[idx])
                                                    {
                                                        for bound in bounds {
                                                            self.check_trait_bound(
                                                                &arg_ty,
                                                                &bound.sym,
                                                                call.args[idx].span(),
                                                            )?;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                } else if let Expr::Member(mem) = &*call.callee {
                    if let Expr::Ident(obj_id) = &*mem.obj {
                        let base_sym = if let Some(pos) = obj_id.sym.find('<') {
                            &obj_id.sym[..pos]
                        } else {
                            &obj_id.sym
                        };
                        if !matches!(self.resolve(base_sym), Some(SymbolKind::Var { .. })) {
                            if let Some(cls) = self.classes.get(base_sym).cloned() {
                                if let Some(m) =
                                    cls.methods.iter().find(|m| m.ident.sym == mem.prop.sym)
                                {
                                    if !m.is_static
                                        && !m.ident.sym.starts_with("__")
                                        && m.ident.sym != "_create"
                                    {
                                        let non_recv_count = m
                                            .params
                                            .iter()
                                            .filter(|p| {
                                                p.ident.sym != "this"
                                                    && p.ident.sym != "_"
                                                    && p.ident.sym != "self"
                                            })
                                            .count();
                                        if call.args.len() <= non_recv_count {
                                            let is_ufcs_first_arg = if let Some(first_arg) =
                                                call.args.first()
                                            {
                                                if let Ok(arg_ty) = self.infer_type(first_arg) {
                                                    let mut base_arg = &arg_ty;
                                                    while let Type::BorrowType { inner, .. } =
                                                        base_arg
                                                    {
                                                        base_arg = inner;
                                                    }
                                                    matches!(base_arg, Type::Ref(cid) if cid.sym == base_sym)
                                                } else {
                                                    false
                                                }
                                            } else {
                                                false
                                            };
                                            if !is_ufcs_first_arg {
                                                let uses_instance_state =
                                                    method_references_instance_state(m, &cls);
                                                return Err(
                                                    SemaError::InstanceMethodCalledOnType {
                                                        type_name: base_sym.to_string(),
                                                        method_name: mem.prop.sym.clone(),
                                                        uses_instance_state,
                                                        span: call.span,
                                                    },
                                                );
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    if let Ok(obj_ty) = self.infer_type(&mem.obj) {
                        let mut base_ty = &obj_ty;
                        while let Type::BorrowType { inner, .. } = base_ty {
                            base_ty = inner;
                        }
                        if let Type::Ref(cid) = base_ty {
                            if let Some(cls) = self.classes.get(&cid.sym).cloned() {
                                if let Some(m) =
                                    cls.methods.iter().find(|m| m.ident.sym == mem.prop.sym)
                                {
                                    if !m.generic_params.is_empty() {
                                        for gp in &m.generic_params {
                                            if let GenericParam::Type {
                                                ident: gp_id,
                                                bounds,
                                            } = gp
                                            {
                                                if !bounds.is_empty() {
                                                    let method_params: Vec<&Param> = m
                                                        .params
                                                        .iter()
                                                        .filter(|p| {
                                                            p.ident.sym != "this"
                                                                && p.ident.sym != "_"
                                                                && p.ident.sym != "self"
                                                        })
                                                        .collect();
                                                    for (idx, p) in method_params.iter().enumerate()
                                                    {
                                                        let mut p_ty = &p.ty;
                                                        while let Type::BorrowType {
                                                            inner, ..
                                                        } = p_ty
                                                        {
                                                            p_ty = inner;
                                                        }
                                                        if let Type::Ref(pid) = p_ty {
                                                            if pid.sym == gp_id.sym
                                                                && idx < call.args.len()
                                                            {
                                                                if let Ok(arg_ty) =
                                                                    self.infer_type(&call.args[idx])
                                                                {
                                                                    for bound in bounds {
                                                                        self.check_trait_bound(
                                                                            &arg_ty,
                                                                            &bound.sym,
                                                                            call.args[idx].span(),
                                                                        )?;
                                                                    }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if let Expr::Ident(id) = &*call.callee {
                    let base_sym = if let Some(pos) = id.sym.find('<') {
                        &id.sym[..pos]
                    } else {
                        &id.sym
                    };
                    if !self.classes.contains_key(base_sym) && id.sym != "Error" {
                        self.analyze_expr(&call.callee)?;
                    }
                } else {
                    self.analyze_expr(&call.callee)?;
                }
                for arg in &call.args {
                    self.analyze_expr(arg)?;
                }
            }
            Expr::New(call) => {
                if let Expr::Ident(id) = &*call.callee {
                    let base_sym = if let Some(pos) = id.sym.find('<') {
                        &id.sym[..pos]
                    } else {
                        &id.sym
                    };
                    if let Some(cls) = self.classes.get(base_sym).cloned() {
                        if cls.is_static {
                            return Err(SemaError::CannotInstantiateStatic(
                                id.sym.clone(),
                                call.span,
                            ));
                        }
                        if !cls.generic_params.is_empty() {
                            for gp in &cls.generic_params {
                                if let GenericParam::Type {
                                    ident: gp_id,
                                    bounds,
                                } = gp
                                {
                                    if !bounds.is_empty() {
                                        if let Some(init_m) =
                                            cls.methods.iter().find(|m| m.ident.sym == "init")
                                        {
                                            let init_params: Vec<&Param> = init_m
                                                .params
                                                .iter()
                                                .filter(|p| {
                                                    p.ident.sym != "this"
                                                        && p.ident.sym != "_"
                                                        && p.ident.sym != "self"
                                                })
                                                .collect();
                                            for (idx, p) in init_params.iter().enumerate() {
                                                let mut p_ty = &p.ty;
                                                while let Type::BorrowType { inner, .. } = p_ty {
                                                    p_ty = inner;
                                                }
                                                if let Type::Ref(pid) = p_ty {
                                                    if pid.sym == gp_id.sym && idx < call.args.len()
                                                    {
                                                        if let Ok(arg_ty) =
                                                            self.infer_type(&call.args[idx])
                                                        {
                                                            for bound in bounds {
                                                                self.check_trait_bound(
                                                                    &arg_ty,
                                                                    &bound.sym,
                                                                    call.args[idx].span(),
                                                                )?;
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                if let Expr::Ident(id) = &*call.callee {
                    let base_sym = if let Some(pos) = id.sym.find('<') {
                        &id.sym[..pos]
                    } else {
                        &id.sym
                    };
                    if !self.classes.contains_key(base_sym) && id.sym != "Error" {
                        self.analyze_expr(&call.callee)?;
                    }
                } else {
                    self.analyze_expr(&call.callee)?;
                }
                for arg in &call.args {
                    self.analyze_expr(arg)?;
                }
            }
            Expr::Member(member) | Expr::OptMember(member) => {
                self.analyze_expr(&member.obj)?;
                if let Ok(obj_ty) = self.infer_type(&member.obj) {
                    let base_ty = match &obj_ty {
                        Type::BorrowType { inner, .. } | Type::RawPtr { inner, .. } => {
                            inner.as_ref()
                        }
                        other => other,
                    };
                    if let Type::Ref(cid) = base_ty {
                        if let Some(cls) = self.classes.get(&cid.sym) {
                            if let Some(field) =
                                cls.fields.iter().find(|f| f.ident.sym == member.prop.sym)
                            {
                                if !field.is_pub && self.current_class.as_deref() != Some(&cid.sym)
                                {
                                    return Err(SemaError::PrivateAccess(
                                        format!(
                                            "Cannot access private field '{}' of class '{}'",
                                            member.prop.sym, cid.sym
                                        ),
                                        member.prop.span,
                                    ));
                                }
                            }
                        }
                    }
                }
            }
            Expr::Slice(slice) => {
                self.analyze_expr(&slice.obj)?;
                if let Some(s) = &slice.start {
                    self.analyze_expr(s)?;
                }
                if let Some(e) = &slice.end {
                    self.analyze_expr(e)?;
                }
                if !self.is_in_unsafe() {
                    if let Ok(ty) = self.infer_type(&slice.obj) {
                        let mut base_ty = &ty;
                        while let Type::BorrowType { inner, .. } = base_ty {
                            base_ty = inner;
                        }
                        if matches!(
                            base_ty,
                            Type::ManyRawPtr(_)
                                | Type::CPtr(_)
                                | Type::SentinelPtr(_, _)
                                | Type::RawPtr { .. }
                        ) {
                            self.errors.push(SemaError::UnsafeRequired(
                                "Slicing raw pointer requires an unsafe block or unsafe function"
                                    .to_string(),
                                slice.span,
                            ));
                        }
                    }
                }
            }
            Expr::Match(m) => {
                self.analyze_expr(&m.disc)?;
                for arm in &m.arms {
                    self.enter_scope();
                    self.bind_match_pattern(&arm.pat)?;
                    if let Some(cap) = &arm.capture {
                        let _ = self.define(
                            cap.sym.clone(),
                            SymbolKind::Var {
                                ty: Type::Any,
                                kind: VarDeclKind::Let,
                            },
                            cap.span,
                        );
                    }
                    if let Some(g) = &arm.guard {
                        self.analyze_expr(g)?;
                    }
                    match &*arm.body {
                        Stmt::Block(b) => {
                            for s in &b.body {
                                self.analyze_stmt(s)?;
                            }
                        }
                        other => self.analyze_stmt(other)?,
                    }
                    self.exit_scope();
                }
            }
            Expr::Array(elements) => {
                for el in elements {
                    self.analyze_expr(el)?;
                }
            }
            Expr::ArrayRepeat(val, count) => {
                self.analyze_expr(val)?;
                self.analyze_expr(count)?;
            }
            Expr::Object(fields) | Expr::Dict(fields) => {
                for f in fields {
                    self.analyze_expr(&f.val)?;
                }
            }
            Expr::Index { obj, index } => {
                self.analyze_expr(obj)?;
                self.analyze_expr(index)?;
                let span = crate::Span {
                    start: inner_span(obj).start,
                    end: inner_span(index).end,
                };
                self.check_index_expr(obj, index, false, span)?;
            }
            Expr::Await(inner)
            | Expr::Question(inner)
            | Expr::ErrWrap(inner)
            | Expr::BoxNew(inner) => {
                self.analyze_expr(inner)?;
            }
            Expr::Move(inner) => {
                self.analyze_expr(inner)?;
                if let Expr::Ident(id) = &**inner {
                    self.moved_vars.insert(id.sym.clone());
                }
            }
            Expr::Borrow(inner, _) => {
                self.analyze_expr(inner)?;
            }
            Expr::QuestionFallback { expr, fallback, .. } => {
                self.analyze_expr(expr)?;
                self.analyze_expr(fallback)?;
            }
            Expr::NullCoalesce { left, right } => {
                self.analyze_expr(left)?;
                self.analyze_expr(right)?;
            }
            Expr::Catch {
                expr,
                err_ident,
                body,
                ..
            } => {
                self.analyze_expr(expr)?;
                self.enter_scope();
                if let Some(ev) = err_ident {
                    if ev.sym == "err" {
                        self.errors.push(SemaError::ReservedIdentifier(
                            ev.sym.clone(),
                            ev.span,
                            true,
                        ));
                    }
                    let _ = self.define(
                        ev.sym.clone(),
                        SymbolKind::Var {
                            ty: Type::Any,
                            kind: VarDeclKind::Let,
                        },
                        ev.span,
                    );
                }
                self.analyze_stmt(body)?;
                self.exit_scope();
            }
            Expr::Downcast { obj, .. } => {
                self.analyze_expr(obj)?;
            }
            Expr::Builtin(call) => {
                self.check_async_isolation_for_builtin(call)?;
                if matches!(
                    call.name.as_str(),
                    "@ptrCast" | "@alignCast" | "@intFromPtr" | "@ptrFromInt" | "@constCast"
                ) && !self.is_in_unsafe()
                {
                    self.errors.push(SemaError::UnsafeRequired(
                        format!(
                            "Unsafe builtin '{}' requires an unsafe block or unsafe function",
                            call.name
                        ),
                        call.span,
                    ));
                }
                for arg in &call.args {
                    self.analyze_expr(arg)?;
                }
                match call.name.as_str() {
                    "@fma" => {
                        if call.args.len() != 3 {
                            self.errors.push(SemaError::Diagnostic(
                                Diagnostic::new(
                                    "E0320",
                                    format!(
                                        "builtin '@fma' expects 3 arguments (a, b, c), found {}",
                                        call.args.len()
                                    ),
                                )
                                .with_span(call.span)
                                .with_cause("hardware fused multiply-add computes (a * b) + c and requires exactly 3 operands.")
                                .with_help("pass 3 floating-point scalars (f32/f64) or SIMD vectors (@Vector(N, f32/f64)) of the same type.")
                                .with_suggestion("usage: @fma(a, b, c)"),
                            ));
                        } else {
                            let is_num_lit = |e: &Expr| -> bool {
                                match e {
                                    Expr::Lit(Lit::Float(_) | Lit::Int(_)) => true,
                                    Expr::Neg(inner) => {
                                        matches!(&**inner, Expr::Lit(Lit::Float(_) | Lit::Int(_)))
                                    }
                                    Expr::Bin(b) => {
                                        b.op == BinaryOp::Sub
                                            && matches!(&*b.left, Expr::Lit(Lit::Int(0)))
                                            && matches!(
                                                &*b.right,
                                                Expr::Lit(Lit::Float(_) | Lit::Int(_))
                                            )
                                    }
                                    _ => false,
                                }
                            };
                            let mut seen_float: Option<Type> = None;
                            for (idx, arg) in call.args.iter().enumerate() {
                                if is_num_lit(arg) {
                                    continue;
                                }
                                let ty = self.infer_type(arg).unwrap_or(Type::Unknown);
                                match ty {
                                    Type::F32 | Type::F64 => {
                                        if let Some(ref prev) = seen_float {
                                            if *prev != ty {
                                                self.errors.push(SemaError::Diagnostic(
                                                    Diagnostic::new(
                                                        "E0322",
                                                        format!(
                                                            "builtin '@fma' operand {} has type '{}', conflicting with '{}'",
                                                            idx + 1,
                                                            Self::type_repr(&ty),
                                                            Self::type_repr(prev)
                                                        ),
                                                    )
                                                    .with_span(call.span)
                                                    .with_cause("all '@fma(a, b, c)' operands must share the exact same floating-point type.")
                                                    .with_help("cast mismatched operands to the same precision (f32 or f64).")
                                                    .with_suggestion(format!(
                                                        "convert operand {} with '@floatCast(...)' or use matching '{}' values.",
                                                        idx + 1,
                                                        Self::type_repr(prev)
                                                    )),
                                                ));
                                            }
                                        } else {
                                            seen_float = Some(ty);
                                        }
                                    }
                                    Type::Unknown | Type::Any | Type::Ref(_) => {}
                                    other => {
                                        self.errors.push(SemaError::Diagnostic(
                                            Diagnostic::new(
                                                "E0321",
                                                format!(
                                                    "builtin '@fma' expects floating-point operand, found '{}'",
                                                    Self::type_repr(&other)
                                                ),
                                            )
                                            .with_span(call.span)
                                            .with_cause("'@fma' maps to hardware FMA (@mulAdd) and only supports f32, f64, or @Vector(N, f32/f64).")
                                            .with_help("convert integer operands to f32/f64 before calling '@fma'.")
                                            .with_suggestion("use 'x as f64' (or 'x as f32') for numeric operands."),
                                        ));
                                    }
                                }
                            }
                        }
                    }
                    "@dot" | "@l2sq" => {
                        let name = call.name.as_str();
                        if call.args.len() != 2 {
                            self.errors.push(SemaError::Diagnostic(
                                Diagnostic::new(
                                    "E0320",
                                    format!(
                                        "builtin '{}' expects 2 slice arguments (a, b), found {}",
                                        name,
                                        call.args.len()
                                    ),
                                )
                                .with_span(call.span)
                                .with_cause(format!(
                                    "'{}' computes a SIMD reduction across two equal-element floating-point slices.",
                                    name
                                ))
                                .with_help("pass two f32 or f64 slices (e.g., []const f32 or []const f64).")
                                .with_suggestion(format!("usage: {}(a, b)", name)),
                            ));
                        } else {
                            let extract_elem = |ty: &Type| -> Option<Type> {
                                let mut cur = ty;
                                loop {
                                    match cur {
                                        Type::BorrowType { inner, .. } => cur = inner,
                                        Type::RawPtr { inner, depth: 1 } => cur = inner,
                                        Type::Array(elem) | Type::FixedArray(elem, _) => {
                                            return Some((**elem).clone());
                                        }
                                        _ => return None,
                                    }
                                }
                            };
                            let mut seen_elem: Option<Type> = None;
                            for (idx, arg) in call.args.iter().enumerate() {
                                let ty = self.infer_type(arg).unwrap_or(Type::Unknown);
                                if matches!(ty, Type::Unknown | Type::Any | Type::Ref(_)) {
                                    continue;
                                }
                                if let Some(elem) = extract_elem(&ty) {
                                    match elem {
                                        Type::F32 | Type::F64 => {
                                            if let Some(ref prev) = seen_elem {
                                                if *prev != elem {
                                                    self.errors.push(SemaError::Diagnostic(
                                                        Diagnostic::new(
                                                            "E0322",
                                                            format!(
                                                                "builtin '{}' slice argument {} has element type '{}', conflicting with '{}'",
                                                                name,
                                                                idx + 1,
                                                                Self::type_repr(&elem),
                                                                Self::type_repr(prev)
                                                            ),
                                                        )
                                                        .with_span(call.span)
                                                        .with_cause(format!(
                                                            "both slices passed to '{}' must have the exact same floating-point element type.",
                                                            name
                                                        ))
                                                        .with_help("ensure both inputs are []const f32 (or both []const f64).")
                                                        .with_suggestion(format!(
                                                            "align both arguments of '{}' to '[]const {}'.",
                                                            name,
                                                            Self::type_repr(prev)
                                                        )),
                                                    ));
                                                }
                                            } else {
                                                seen_elem = Some(elem);
                                            }
                                        }
                                        Type::Unknown | Type::Any | Type::Ref(_) => {}
                                        other => {
                                            self.errors.push(SemaError::Diagnostic(
                                                Diagnostic::new(
                                                    "E0321",
                                                    format!(
                                                        "builtin '{}' expects f32 or f64 slice elements, found '{}'",
                                                        name,
                                                        Self::type_repr(&other)
                                                    ),
                                                )
                                                .with_span(call.span)
                                                .with_cause(format!(
                                                    "'{}' accelerates floating-point vector reductions via 8-lane SIMD.",
                                                    name
                                                ))
                                                .with_help("use '[]const f32' or '[]const f64' slices.")
                                                .with_suggestion(format!(
                                                    "convert slice elements to f32 or f64 before calling '{}'.",
                                                    name
                                                )),
                                            ));
                                        }
                                    }
                                } else {
                                    self.errors.push(SemaError::Diagnostic(
                                        Diagnostic::new(
                                            "E0321",
                                            format!(
                                                "builtin '{}' expects a floating-point slice or array, found '{}'",
                                                name,
                                                Self::type_repr(&ty)
                                            ),
                                        )
                                        .with_span(call.span)
                                        .with_cause(format!(
                                            "'{}' operates on contiguous floating-point slices ([]f32 / []const f32 / []f64 / []const f64).",
                                            name
                                        ))
                                        .with_help("pass a slice (e.g., vec[0..]) instead of a scalar or non-slice value.")
                                        .with_suggestion(format!(
                                            "pass two slices of f32 or f64: {}(a[0..], b[0..])",
                                            name
                                        )),
                                    ));
                                }
                            }
                        }
                    }
                    "@splitAtMut" => {
                        if call.args.len() != 2 {
                            self.errors.push(SemaError::Diagnostic(
                                Diagnostic::new(
                                    "E0320",
                                    format!(
                                        "builtin '@splitAtMut' expects 2 arguments (slice, mid), found {}",
                                        call.args.len()
                                    ),
                                )
                                .with_span(call.span)
                                .with_cause("'@splitAtMut' splits a mutable slice into two disjoint mutable sub-slices [0..mid) and [mid..len).")
                                .with_help("pass a mutable slice (e.g., []f32 or *[]f32) and an integer split index.")
                                .with_suggestion("usage: let [left, right] = @splitAtMut(buf, mid);"),
                            ));
                        } else {
                            let t0 = self.infer_type(&call.args[0]).unwrap_or(Type::Unknown);
                            if !matches!(t0, Type::Unknown | Type::Any | Type::Ref(_)) {
                                let is_const_borrow =
                                    matches!(t0, Type::BorrowType { is_mut: false, .. });
                                if is_const_borrow {
                                    self.errors.push(SemaError::Diagnostic(
                                        Diagnostic::new(
                                            "E0323",
                                            format!(
                                                "builtin '@splitAtMut' cannot mutably split read-only borrow '{}'",
                                                Self::type_repr(&t0)
                                            ),
                                        )
                                        .with_span(call.span)
                                        .with_cause("'@splitAtMut' returns two mutable slices and requires a mutable slice or mutable borrow.")
                                        .with_help("declare the parameter or variable as mutable slice '[]T' or mutable borrow '*[]T'.")
                                        .with_suggestion("change '[]const T' to '*[]T' (or '[]T') before calling '@splitAtMut'."),
                                    ));
                                } else {
                                    let mut cur = &t0;
                                    let mut is_slice_or_arr = false;
                                    loop {
                                        match cur {
                                            Type::BorrowType { inner, .. } => cur = inner,
                                            Type::RawPtr { inner, depth: 1 } => cur = inner,
                                            Type::Array(_) | Type::FixedArray(_, _) => {
                                                is_slice_or_arr = true;
                                                break;
                                            }
                                            _ => break,
                                        }
                                    }
                                    if !is_slice_or_arr {
                                        self.errors.push(SemaError::Diagnostic(
                                            Diagnostic::new(
                                                "E0321",
                                                format!(
                                                    "builtin '@splitAtMut' expects a mutable slice or array, found '{}'",
                                                    Self::type_repr(&t0)
                                                ),
                                            )
                                            .with_span(call.span)
                                            .with_cause("'@splitAtMut' operates on contiguous mutable slices ([]T or *[]T).")
                                            .with_help("pass a mutable slice (e.g., buf[0..]) as the first argument.")
                                            .with_suggestion("usage: let [left, right] = @splitAtMut(buf[0..], mid);"),
                                        ));
                                    }
                                }
                            }
                            let t1 = self.infer_type(&call.args[1]).unwrap_or(Type::Unknown);
                            if !matches!(
                                t1,
                                Type::Unknown
                                    | Type::Any
                                    | Type::Ref(_)
                                    | Type::U8
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
                            ) {
                                self.errors.push(SemaError::Diagnostic(
                                    Diagnostic::new(
                                        "E0321",
                                        format!(
                                            "builtin '@splitAtMut' split index 'mid' must be an integer, found '{}'",
                                            Self::type_repr(&t1)
                                        ),
                                    )
                                    .with_span(call.span)
                                    .with_cause("the second argument to '@splitAtMut(slice, mid)' is the element index to split at.")
                                    .with_help("pass a usize or integer index.")
                                    .with_suggestion("cast the index to usize, e.g., @splitAtMut(buf, mid as usize)."),
                                ));
                            }
                        }
                    }
                    _ => {}
                }
            }
            Expr::ReflectType { span, target_ty } => {
                let type_name = match target_ty {
                    Type::Ref(id) => id.sym.clone(),
                    _ => {
                        return Err(SemaError::ReflectNotEnabled(
                            format!("{:?}", target_ty),
                            *span,
                        ));
                    }
                };
                if let Some(cls) = self.classes.get(&type_name) {
                    let has_reflect = cls.meta.iter().any(|m| m.name.sym == "reflect");
                    if !has_reflect {
                        return Err(SemaError::ReflectNotEnabled(type_name, *span));
                    }
                } else {
                    return Err(SemaError::UndefinedSymbol(type_name, *span));
                }
            }
            Expr::TypeVal { .. } => {}
            Expr::ReflectOf { span, expr } => {
                self.analyze_expr(expr)?;
                let ty = self.infer_type(expr)?;
                let type_name = match ty {
                    Type::Ref(id) => id.sym,
                    Type::BorrowType { inner, .. } => {
                        if let Type::Ref(id) = &*inner {
                            id.sym.clone()
                        } else {
                            return Err(SemaError::ReflectNotEnabled(
                                format!("{:?}", inner),
                                *span,
                            ));
                        }
                    }
                    _ => {
                        return Err(SemaError::ReflectNotEnabled(format!("{:?}", ty), *span));
                    }
                };
                if let Some(cls) = self.classes.get(&type_name) {
                    let has_reflect = cls.meta.iter().any(|m| m.name.sym == "reflect");
                    if !has_reflect {
                        return Err(SemaError::ReflectNotEnabled(type_name, *span));
                    }
                } else {
                    return Err(SemaError::UndefinedSymbol(type_name, *span));
                }
            }
            Expr::Range { start, end, .. } => {
                self.analyze_expr(start)?;
                self.analyze_expr(end)?;
            }
            Expr::Cast {
                expr,
                target_ty,
                span,
            } => {
                self.analyze_expr(expr)?;
                let src_ty = self.infer_type(expr).unwrap_or(Type::Unknown);

                // Layer 3: Raw pointer casting is only allowed inside unsafe block
                let is_src_ptr = matches!(src_ty, Type::RawPtr { .. });
                let is_target_ptr = matches!(target_ty, Type::RawPtr { .. });
                if (is_src_ptr || is_target_ptr) && !self.is_in_unsafe() {
                    return Err(SemaError::UnsafeRequired(
                        "Raw pointer casting is only allowed inside unsafe block".to_string(),
                        *span,
                    ));
                }

                // Layer 4: Semantic conversions forbidden via `as`
                if src_ty == Type::String && Self::is_numeric_type(target_ty) {
                    return Err(SemaError::InvalidCast(
                        format!(
                            "Cannot cast 'string' to numeric type '{}' via 'as'; use '{}.parse(...) ?' instead",
                            Self::type_repr(target_ty),
                            Self::type_repr(target_ty)
                        ),
                        *span,
                    ));
                }
                if Self::is_numeric_type(&src_ty) && *target_ty == Type::String {
                    return Err(SemaError::InvalidCast(
                        format!(
                            "Cannot cast numeric type '{}' to 'string' via 'as'; use '.to_string()' instead",
                            Self::type_repr(&src_ty)
                        ),
                        *span,
                    ));
                }
            }
            Expr::Neg(inner) | Expr::BitNot(inner) => {
                self.analyze_expr(inner)?;
            }
            Expr::Deref(inner) => {
                self.analyze_expr(inner)?;
                if let Ok(ty) = self.infer_type(inner) {
                    if matches!(ty, Type::RawPtr { .. }) && !self.is_in_unsafe() {
                        return Err(SemaError::UnsafeRequired(
                            "Dereferencing raw pointer requires an unsafe block or unsafe function"
                                .to_string(),
                            inner_span(inner),
                        ));
                    }
                }
            }
            Expr::Unsafe(block) => {
                self.unsafe_depth += 1;
                let res = self.analyze_block(block);
                self.unsafe_depth -= 1;
                res?;
            }
            Expr::Comptime(block) => {
                self.analyze_block(block)?;
            }
            Expr::ComptimeExpr(e) => {
                self.analyze_expr(e)?;
            }
            Expr::Arena(block) => {
                self.analyze_block(block)?;
            }
            Expr::Template(t) => {
                for e in &t.expressions {
                    self.analyze_expr(e)?;
                }
            }
            Expr::If {
                test, cons, alt, ..
            } => {
                self.analyze_expr(test)?;
                let not_null_vars = Self::extract_not_null_vars(test);
                let is_null_vars = Self::extract_is_null_vars(test);
                if !not_null_vars.is_empty() {
                    self.enter_scope();
                    for v in &not_null_vars {
                        if let Some(SymbolKind::Var { ty, kind }) = self.resolve(v).cloned() {
                            let unwrapped = Self::unwrap_optional_type(&ty);
                            let _ = self.define(
                                v.clone(),
                                SymbolKind::Var {
                                    ty: unwrapped,
                                    kind,
                                },
                                Span { start: 0, end: 0 },
                            );
                        }
                    }
                    self.analyze_expr(cons)?;
                    self.exit_scope();
                } else {
                    self.analyze_expr(cons)?;
                }
                if let Some(a) = alt {
                    if !is_null_vars.is_empty() {
                        self.enter_scope();
                        for v in &is_null_vars {
                            if let Some(SymbolKind::Var { ty, kind }) = self.resolve(v).cloned() {
                                let unwrapped = Self::unwrap_optional_type(&ty);
                                let _ = self.define(
                                    v.clone(),
                                    SymbolKind::Var {
                                        ty: unwrapped,
                                        kind,
                                    },
                                    Span { start: 0, end: 0 },
                                );
                            }
                        }
                        self.analyze_expr(a)?;
                        self.exit_scope();
                    } else {
                        self.analyze_expr(a)?;
                    }
                }
            }
            Expr::LabeledBlock { block, .. } => {
                self.analyze_block(block)?;
            }
            Expr::While { stmt, else_expr } => {
                self.analyze_stmt(&Stmt::While((**stmt).clone()))?;
                if let Some(e) = else_expr {
                    self.analyze_expr(e)?;
                }
            }
            Expr::LabeledBreak(_, _, val) => {
                if let Some(v) = val {
                    self.analyze_expr(v)?;
                }
            }
            Expr::Arrow(arrow) => {
                self.enter_scope();
                for p in &arrow.params {
                    self.analyze_pattern(p, Type::Auto, &VarDeclKind::Let)?;
                }
                match &*arrow.body {
                    ArrowBody::Expr(e) => self.analyze_expr(e)?,
                    ArrowBody::Block(b) => self.analyze_block(b)?,
                }
                self.exit_scope();
            }
            _ => {}
        }
        Ok(())
    }
}

fn method_references_instance_state(m: &Function, cls: &Class) -> bool {
    if m.params
        .iter()
        .any(|p| p.ident.sym == "_" || p.ident.sym == "this" || p.ident.sym == "self")
    {
        return true;
    }
    let inst_names: std::collections::HashSet<&str> = cls
        .fields
        .iter()
        .filter(|f| !f.is_static)
        .map(|f| f.ident.sym.as_str())
        .chain(
            cls.methods
                .iter()
                .filter(|other| !other.is_static)
                .map(|other| other.ident.sym.as_str()),
        )
        .collect();
    let shadowed: std::collections::HashSet<String> =
        m.params.iter().map(|p| p.ident.sym.clone()).collect();
    block_uses_instance_state(&m.body, &inst_names, &shadowed)
}

fn block_uses_instance_state(
    block: &BlockStmt,
    inst_names: &std::collections::HashSet<&str>,
    outer_shadowed: &std::collections::HashSet<String>,
) -> bool {
    let mut shadowed = outer_shadowed.clone();
    for s in &block.body {
        if let Stmt::Var(v) = s {
            if v.init.as_ref().map_or(false, |e| {
                expr_uses_instance_state(e, inst_names, &shadowed)
            }) {
                return true;
            }
            collect_pattern_idents(&v.pat, &mut shadowed);
            continue;
        }
        if stmt_uses_instance_state(s, inst_names, &shadowed) {
            return true;
        }
    }
    false
}

fn collect_pattern_idents(pat: &Pattern, out: &mut std::collections::HashSet<String>) {
    match pat {
        Pattern::Ident(id) => {
            out.insert(id.sym.clone());
        }
        Pattern::Tuple(items) | Pattern::Array(items) | Pattern::Or(items) => {
            for p in items {
                collect_pattern_idents(p, out);
            }
        }
        Pattern::Object(fields) => {
            for f in fields {
                if let Some(p) = &f.val {
                    collect_pattern_idents(p, out);
                } else {
                    out.insert(f.key.sym.clone());
                }
            }
        }
        Pattern::Rest(inner) | Pattern::Err(inner) => collect_pattern_idents(inner, out),
        Pattern::Binding { ident, subpat } => {
            out.insert(ident.sym.clone());
            collect_pattern_idents(subpat, out);
        }
        Pattern::TypeMatch { ident, .. } => {
            out.insert(ident.sym.clone());
        }
        _ => {}
    }
}

fn stmt_uses_instance_state(
    stmt: &Stmt,
    inst_names: &std::collections::HashSet<&str>,
    shadowed: &std::collections::HashSet<String>,
) -> bool {
    match stmt {
        Stmt::Expr(e) => expr_uses_instance_state(e, inst_names, shadowed),
        Stmt::Return(r) => r
            .arg
            .as_ref()
            .map_or(false, |e| expr_uses_instance_state(e, inst_names, shadowed)),
        Stmt::Defer(s) | Stmt::ErrDefer(s) => stmt_uses_instance_state(s, inst_names, shadowed),
        Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ComptimeBlock(b) | Stmt::ArenaBlock(b) => {
            block_uses_instance_state(b, inst_names, shadowed)
        }
        Stmt::If(i) => {
            let mut inner = shadowed.clone();
            if let Some(cap) = &i.capture {
                inner.insert(cap.sym.clone());
            }
            expr_uses_instance_state(&i.test, inst_names, shadowed)
                || stmt_uses_instance_state(&i.cons, inst_names, &inner)
                || i.alt
                    .as_ref()
                    .map_or(false, |a| stmt_uses_instance_state(a, inst_names, shadowed))
        }
        Stmt::While(w) => {
            let mut inner = shadowed.clone();
            if let Some(cap) = &w.capture {
                inner.insert(cap.sym.clone());
            }
            expr_uses_instance_state(&w.test, inst_names, shadowed)
                || stmt_uses_instance_state(&w.body, inst_names, &inner)
                || w.continue_expr
                    .as_ref()
                    .map_or(false, |s| expr_uses_instance_state(s, inst_names, &inner))
        }
        Stmt::For(f) => {
            let mut inner = shadowed.clone();
            collect_pattern_idents(&f.pat, &mut inner);
            expr_uses_instance_state(&f.expr, inst_names, shadowed)
                || stmt_uses_instance_state(&f.body, inst_names, &inner)
        }
        Stmt::Match(m) => {
            expr_uses_instance_state(&m.disc, inst_names, shadowed)
                || m.arms.iter().any(|arm| {
                    let mut inner = shadowed.clone();
                    collect_pattern_idents(&arm.pat, &mut inner);
                    arm.guard
                        .as_ref()
                        .map_or(false, |g| expr_uses_instance_state(g, inst_names, &inner))
                        || stmt_uses_instance_state(&arm.body, inst_names, &inner)
                })
        }
        Stmt::Var(v) => v
            .init
            .as_ref()
            .map_or(false, |e| expr_uses_instance_state(e, inst_names, shadowed)),
        Stmt::TryCatch {
            try_block,
            catch_body,
            ..
        } => {
            block_uses_instance_state(try_block, inst_names, shadowed)
                || catch_body
                    .as_ref()
                    .map_or(false, |b| stmt_uses_instance_state(b, inst_names, shadowed))
        }
        _ => false,
    }
}

fn expr_uses_instance_state(
    expr: &Expr,
    inst_names: &std::collections::HashSet<&str>,
    shadowed: &std::collections::HashSet<String>,
) -> bool {
    match expr {
        Expr::Ident(id) => {
            id.sym == "_"
                || id.sym == "this"
                || id.sym == "self"
                || (!shadowed.contains(&id.sym) && inst_names.contains(id.sym.as_str()))
        }
        Expr::Member(m) | Expr::OptMember(m) => {
            expr_uses_instance_state(&m.obj, inst_names, shadowed)
        }
        Expr::Call(c) | Expr::New(c) | Expr::OptCall(c) => {
            expr_uses_instance_state(&c.callee, inst_names, shadowed)
                || c.args
                    .iter()
                    .any(|a| expr_uses_instance_state(a, inst_names, shadowed))
        }
        Expr::Bin(b) => {
            expr_uses_instance_state(&b.left, inst_names, shadowed)
                || expr_uses_instance_state(&b.right, inst_names, shadowed)
        }
        Expr::NullCoalesce { left, right } => {
            expr_uses_instance_state(left, inst_names, shadowed)
                || expr_uses_instance_state(right, inst_names, shadowed)
        }
        Expr::Index { obj, index } => {
            expr_uses_instance_state(obj, inst_names, shadowed)
                || expr_uses_instance_state(index, inst_names, shadowed)
        }
        Expr::Slice(s) => {
            expr_uses_instance_state(&s.obj, inst_names, shadowed)
                || s.start
                    .as_ref()
                    .map_or(false, |e| expr_uses_instance_state(e, inst_names, shadowed))
                || s.end
                    .as_ref()
                    .map_or(false, |e| expr_uses_instance_state(e, inst_names, shadowed))
        }
        Expr::Array(elems) => elems
            .iter()
            .any(|e| expr_uses_instance_state(e, inst_names, shadowed)),
        Expr::ArrayRepeat(a, b) => {
            expr_uses_instance_state(a, inst_names, shadowed)
                || expr_uses_instance_state(b, inst_names, shadowed)
        }
        Expr::Object(fields) | Expr::Dict(fields) => fields
            .iter()
            .any(|f| expr_uses_instance_state(&f.val, inst_names, shadowed)),
        Expr::If {
            test,
            cons,
            alt,
            capture,
            ..
        } => {
            let mut inner = shadowed.clone();
            if let Some(cap) = capture {
                inner.insert(cap.sym.clone());
            }
            expr_uses_instance_state(test, inst_names, shadowed)
                || expr_uses_instance_state(cons, inst_names, &inner)
                || alt
                    .as_ref()
                    .map_or(false, |a| expr_uses_instance_state(a, inst_names, shadowed))
        }
        Expr::Match(m) => {
            expr_uses_instance_state(&m.disc, inst_names, shadowed)
                || m.arms.iter().any(|arm| {
                    let mut inner = shadowed.clone();
                    collect_pattern_idents(&arm.pat, &mut inner);
                    arm.guard
                        .as_ref()
                        .map_or(false, |g| expr_uses_instance_state(g, inst_names, &inner))
                        || stmt_uses_instance_state(&arm.body, inst_names, &inner)
                })
        }
        Expr::Unsafe(b)
        | Expr::Comptime(b)
        | Expr::Arena(b)
        | Expr::LabeledBlock { block: b, .. } => block_uses_instance_state(b, inst_names, shadowed),
        Expr::Arrow(ar) => {
            let mut inner = shadowed.clone();
            for p in &ar.params {
                collect_pattern_idents(p, &mut inner);
            }
            match &*ar.body {
                ArrowBody::Expr(e) => expr_uses_instance_state(e, inst_names, &inner),
                ArrowBody::Block(b) => block_uses_instance_state(b, inst_names, &inner),
            }
        }
        Expr::Range { start, end, .. } => {
            expr_uses_instance_state(start, inst_names, shadowed)
                || expr_uses_instance_state(end, inst_names, shadowed)
        }
        Expr::Cast { expr: e, .. }
        | Expr::Deref(e)
        | Expr::Unwrap(e)
        | Expr::BitNot(e)
        | Expr::Neg(e)
        | Expr::Question(e)
        | Expr::ErrWrap(e)
        | Expr::BoxNew(e)
        | Expr::Downcast { obj: e, .. }
        | Expr::Await(e)
        | Expr::Move(e)
        | Expr::Borrow(e, _)
        | Expr::ComptimeExpr(e)
        | Expr::ReflectOf { expr: e, .. } => expr_uses_instance_state(e, inst_names, shadowed),
        Expr::QuestionFallback { expr, fallback, .. } => {
            expr_uses_instance_state(expr, inst_names, shadowed)
                || expr_uses_instance_state(fallback, inst_names, shadowed)
        }
        Expr::Catch { expr, body, .. } => {
            expr_uses_instance_state(expr, inst_names, shadowed)
                || stmt_uses_instance_state(body, inst_names, shadowed)
        }
        Expr::Builtin(bc) => bc
            .args
            .iter()
            .any(|a| expr_uses_instance_state(a, inst_names, shadowed)),
        Expr::Return(_, Some(e)) | Expr::Break(_, Some(e)) | Expr::LabeledBreak(_, _, Some(e)) => {
            expr_uses_instance_state(e, inst_names, shadowed)
        }
        Expr::While { stmt, else_expr } => {
            stmt_uses_instance_state(&Stmt::While((**stmt).clone()), inst_names, shadowed)
                || else_expr
                    .as_ref()
                    .map_or(false, |e| expr_uses_instance_state(e, inst_names, shadowed))
        }
        _ => false,
    }
}
