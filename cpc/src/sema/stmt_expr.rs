use super::*;
use crate::ast::*;
use crate::diagnostic::{find_closest_match, Diagnostic, STANDARD_BUILTIN_IMPORTS};
use crate::Span;
use std::collections::HashMap;

impl Analyzer {
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
                for arg in &call.args {
                    self.analyze_expr(arg)?;
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
            _ => {}
        }
        Ok(())
    }
}
