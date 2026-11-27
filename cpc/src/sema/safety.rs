use super::*;
use crate::ast::*;
use crate::diagnostic::{find_closest_match, Diagnostic, STANDARD_BUILTIN_IMPORTS};
use crate::Span;
use std::collections::HashMap;

impl Analyzer {
    pub(super) fn enter_scope(&mut self) {
        self.scopes.push(HashMap::new());
        self.known_lengths_stack.push(HashMap::new());
        self.const_int_vars_stack.push(HashMap::new());
    }
    pub(super) fn exit_scope(&mut self) {
        self.scopes.pop();
        self.known_lengths_stack.pop();
        self.const_int_vars_stack.pop();
    }

    pub fn get_known_length(&self, name: &str) -> Option<usize> {
        for scope in self.known_lengths_stack.iter().rev() {
            if let Some(len) = scope.get(name) {
                return Some(*len);
            }
        }
        None
    }

    pub fn set_known_length(&mut self, name: String, len: usize) {
        if let Some(scope) = self.known_lengths_stack.last_mut() {
            scope.insert(name, len);
        }
    }

    pub fn clear_known_length(&mut self, name: &str) {
        for scope in self.known_lengths_stack.iter_mut() {
            scope.remove(name);
        }
    }

    pub fn get_const_int(&self, name: &str) -> Option<i64> {
        for scope in self.const_int_vars_stack.iter().rev() {
            if let Some(val) = scope.get(name) {
                return Some(*val);
            }
        }
        None
    }

    pub fn set_const_int(&mut self, name: String, val: i64) {
        if let Some(scope) = self.const_int_vars_stack.last_mut() {
            scope.insert(name, val);
        }
    }

    pub fn eval_int_expr(&self, expr: &Expr) -> Option<i64> {
        match expr {
            Expr::Lit(Lit::Int(n)) => Some(*n),
            Expr::Ident(id) => self.get_const_int(&id.sym),
            Expr::Bin(bin) => {
                let l = self.eval_int_expr(&bin.left)?;
                let r = self.eval_int_expr(&bin.right)?;
                match bin.op {
                    BinaryOp::Add => Some(l.wrapping_add(r)),
                    BinaryOp::Sub => Some(l.wrapping_sub(r)),
                    BinaryOp::Mul => Some(l.wrapping_mul(r)),
                    BinaryOp::Div => {
                        if r != 0 {
                            Some(l.wrapping_div(r))
                        } else {
                            None
                        }
                    }
                    BinaryOp::Mod => {
                        if r != 0 {
                            Some(l.wrapping_rem(r))
                        } else {
                            None
                        }
                    }
                    BinaryOp::Shl => {
                        if (0..64).contains(&r) {
                            Some(l << r)
                        } else {
                            None
                        }
                    }
                    BinaryOp::Shr => {
                        if (0..64).contains(&r) {
                            Some(l >> r)
                        } else {
                            None
                        }
                    }
                    BinaryOp::BitAnd => Some(l & r),
                    BinaryOp::BitOr => Some(l | r),
                    BinaryOp::BitXor => Some(l ^ r),
                    _ => None,
                }
            }
            _ => None,
        }
    }

    pub fn infer_known_length(&self, ty: &Type, init_opt: Option<&Expr>) -> Option<usize> {
        let mut curr_ty = ty;
        while let Type::BorrowType { inner, .. } = curr_ty {
            curr_ty = inner;
        }
        if let Type::FixedArray(_, len) = curr_ty {
            return len.as_usize();
        }

        if let Some(init) = init_opt {
            match init {
                Expr::Lit(Lit::Str(s)) => {
                    return Some(s.len());
                }
                Expr::Array(elements) => {
                    return Some(elements.len());
                }
                Expr::ArrayRepeat(_, count) => {
                    if let Some(n) = self.eval_int_expr(count) {
                        if n >= 0 {
                            return Some(n as usize);
                        }
                    }
                }
                Expr::Borrow(inner, _) => match &**inner {
                    Expr::Ident(id) => {
                        if let Some(len) = self.get_known_length(&id.sym) {
                            return Some(len);
                        }
                    }
                    Expr::Index { obj, index } => {
                        if let Expr::Ident(id) = &**obj {
                            if let Some(buf_len) = self.get_known_length(&id.sym) {
                                if let Some(idx_val) = self.eval_int_expr(index) {
                                    if idx_val >= 0 && (idx_val as usize) <= buf_len {
                                        return Some(buf_len - idx_val as usize);
                                    }
                                }
                            }
                        }
                    }
                    _ => {}
                },
                Expr::Slice(slice) => {
                    if let (Some(s_expr), Some(e_expr)) = (&slice.start, &slice.end) {
                        if let (Some(s_val), Some(e_val)) =
                            (self.eval_int_expr(s_expr), self.eval_int_expr(e_expr))
                        {
                            if e_val >= s_val && s_val >= 0 {
                                return Some((e_val - s_val) as usize);
                            }
                        }
                    }
                }
                Expr::Ident(id) => {
                    if let Some(len) = self.get_known_length(&id.sym) {
                        return Some(len);
                    }
                }
                _ => {}
            }
        }
        None
    }

    pub fn check_index_expr(
        &mut self,
        obj: &Expr,
        index: &Expr,
        is_write: bool,
        span: Span,
    ) -> SemaResult<()> {
        let idx_opt = self.eval_int_expr(index);

        if let Some(idx) = idx_opt {
            if idx < 0 {
                return Err(SemaError::NegativeIndex { index: idx, span });
            }
        }

        let mut known_len = None;
        if let Expr::Ident(id) = obj {
            known_len = self.get_known_length(&id.sym);
        }
        if known_len.is_none() {
            if let Ok(obj_ty) = self.infer_type(obj) {
                let mut curr_ty = &obj_ty;
                while let Type::BorrowType { inner, .. } = curr_ty {
                    curr_ty = inner;
                }
                if let Type::FixedArray(_, len) = curr_ty {
                    known_len = len.as_usize();
                }
            }
        }
        if known_len.is_none() {
            if let Expr::Lit(Lit::Str(s)) = obj {
                known_len = Some(s.len());
            }
        }

        if let (Some(idx), Some(len)) = (idx_opt, known_len) {
            if (idx as usize) >= len {
                return Err(SemaError::CompileTimeIndexOutOfBounds {
                    index: idx,
                    len,
                    span,
                });
            }
        }

        let obj_ty_res = self.infer_type(obj);
        if let Ok(ty) = obj_ty_res {
            let mut base_ty = &ty;
            while let Type::BorrowType { inner, .. } = base_ty {
                base_ty = inner;
            }
            let is_raw_ptr = matches!(
                base_ty,
                Type::ManyRawPtr(_) | Type::CPtr(_) | Type::SentinelPtr(_, _) | Type::RawPtr { .. }
            );
            let is_unbounded_ptr_or_cstr = is_raw_ptr
                || matches!(base_ty, Type::CStr)
                || matches!(base_ty, Type::Ref(id) if id.sym == "cstring" || id.sym == "cstr");

            if is_write {
                if is_unbounded_ptr_or_cstr && !self.is_in_unsafe() {
                    let is_statically_proven_safe =
                        if let (Some(idx), Some(len)) = (idx_opt, known_len) {
                            idx >= 0 && (idx as usize) < len
                        } else {
                            false
                        };

                    if !is_statically_proven_safe {
                        let var_name = if let Expr::Ident(id) = obj {
                            id.sym.clone()
                        } else {
                            "pointer".to_string()
                        };
                        return Err(SemaError::UnboundedRawPointerMutation(var_name, span));
                    }
                }
            } else if is_raw_ptr && !self.is_in_unsafe() {
                let var_name = if let Expr::Ident(id) = obj {
                    id.sym.clone()
                } else {
                    "pointer".to_string()
                };
                self.errors.push(SemaError::UnsafeRequired(
                    format!(
                        "Indexing raw pointer '{}' requires an unsafe block or unsafe function",
                        var_name
                    ),
                    span,
                ));
            }
        }

        Ok(())
    }

    pub fn is_safe_concurrency_type(&self, ty: &Type) -> bool {
        match ty {
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
            | Type::F32
            | Type::F64
            | Type::Bool
            | Type::Void
            | Type::PID
            | Type::String => true,

            Type::Shared(_) => true,
            Type::Ref(id) => {
                let name = &id.sym;
                if name == "Shared"
                    || name.starts_with("Shared<")
                    || name == "Atomic"
                    || name.starts_with("Atomic<")
                    || name == "SharedTensor"
                    || name == "Mutex"
                    || name.starts_with("Mutex<")
                    || name == "RwLock"
                    || name.starts_with("RwLock<")
                    || name == "PID"
                {
                    return true;
                }
                if let Some(cls) = self.classes.get(name) {
                    if cls.is_ref {
                        return false;
                    }
                    if cls
                        .fields
                        .iter()
                        .all(|f| self.is_safe_concurrency_type(&f.ty))
                    {
                        return true;
                    }
                }
                false
            }
            Type::Optional(inner) | Type::ErrorUnion(inner) => self.is_safe_concurrency_type(inner),
            Type::ResultType { ok, .. } => self.is_safe_concurrency_type(ok),
            Type::Tuple(elems) => elems.iter().all(|t| self.is_safe_concurrency_type(t)),
            Type::FixedArray(elem, _) => self.is_safe_concurrency_type(elem),
            _ => false,
        }
    }

    pub fn find_ident_span_in_arrow(arrow: &ArrowExpr, name: &str) -> Option<Span> {
        fn find_in_expr(expr: &Expr, name: &str) -> Option<Span> {
            match expr {
                Expr::Ident(id) if id.sym == name => Some(id.span),
                Expr::Bin(b) => {
                    find_in_expr(&b.left, name).or_else(|| find_in_expr(&b.right, name))
                }
                Expr::Deref(inner) => find_in_expr(inner, name),
                Expr::Borrow(inner, _) => find_in_expr(inner, name),
                Expr::Call(c) => find_in_expr(&c.callee, name).or_else(|| {
                    for a in &c.args {
                        if let Some(s) = find_in_expr(a, name) {
                            return Some(s);
                        }
                    }
                    None
                }),
                Expr::Member(m) => find_in_expr(&m.obj, name),
                _ => None,
            }
        }
        fn find_in_block(block: &BlockStmt, name: &str) -> Option<Span> {
            for s in &block.body {
                match s {
                    Stmt::Expr(e) => {
                        if let Some(sp) = find_in_expr(e, name) {
                            return Some(sp);
                        }
                    }
                    Stmt::Var(v) => {
                        if let Some(init) = &v.init {
                            if let Some(sp) = find_in_expr(init, name) {
                                return Some(sp);
                            }
                        }
                    }
                    Stmt::Block(b) => {
                        if let Some(sp) = find_in_block(b, name) {
                            return Some(sp);
                        }
                    }
                    _ => {}
                }
            }
            None
        }
        match &*arrow.body {
            ArrowBody::Expr(e) => find_in_expr(e, name),
            ArrowBody::Block(b) => find_in_block(b, name),
        }
    }

    pub(super) fn find_moved_vars_in_arrow(
        arrow: &ArrowExpr,
        moved: &mut std::collections::HashSet<String>,
    ) {
        fn find_in_expr(expr: &Expr, moved: &mut std::collections::HashSet<String>) {
            match expr {
                Expr::Move(inner) => {
                    if let Expr::Ident(id) = &**inner {
                        moved.insert(id.sym.clone());
                    }
                }
                Expr::Bin(b) => {
                    find_in_expr(&b.left, moved);
                    find_in_expr(&b.right, moved);
                }
                Expr::Call(c) => {
                    find_in_expr(&c.callee, moved);
                    for a in &c.args {
                        find_in_expr(a, moved);
                    }
                }
                _ => {}
            }
        }
        fn find_in_block(block: &BlockStmt, moved: &mut std::collections::HashSet<String>) {
            for s in &block.body {
                match s {
                    Stmt::Expr(e) => find_in_expr(e, moved),
                    Stmt::Var(v) => {
                        if let Some(init) = &v.init {
                            find_in_expr(init, moved);
                        }
                    }
                    Stmt::Block(b) => find_in_block(b, moved),
                    _ => {}
                }
            }
        }
        match &*arrow.body {
            ArrowBody::Expr(e) => find_in_expr(e, moved),
            ArrowBody::Block(b) => find_in_block(b, moved),
        }
    }

    pub fn check_async_boundary_arg(&mut self, arg: &Expr, _call_span: Span) -> SemaResult<()> {
        if let Expr::Arrow(arrow) = arg {
            let mut refs = std::collections::HashSet::new();
            match &*arrow.body {
                ArrowBody::Expr(e) => {
                    crate::ast_visitor::find_referenced_vars_in_expr(e, &mut refs);
                }
                ArrowBody::Block(b) => {
                    crate::ast_visitor::find_referenced_vars_in_block(b, &mut refs);
                }
            }
            let decls = match &*arrow.body {
                ArrowBody::Expr(_) => std::collections::HashSet::new(),
                ArrowBody::Block(b) => crate::ast_visitor::get_declared_vars_in_block(b),
            };
            for d in decls {
                refs.remove(&d);
            }
            for param in &arrow.params {
                let mut param_vars = std::collections::HashSet::new();
                crate::ast_visitor::extract_idents_from_pat(param, &mut param_vars);
                for pv in param_vars {
                    refs.remove(&pv);
                }
            }

            let mut moved_in_closure = std::collections::HashSet::new();
            Self::find_moved_vars_in_arrow(arrow, &mut moved_in_closure);

            for r in refs {
                if r == "this" || r == "_" || r == "self" {
                    continue;
                }
                if let Some(kind) = self.resolve(&r) {
                    if let SymbolKind::Var { ty, .. } = kind {
                        let is_borrow_ptr = matches!(
                            ty,
                            Type::BorrowType { .. }
                                | Type::RawPtr { .. }
                                | Type::ManyRawPtr(_)
                                | Type::CPtr(_)
                        );
                        let is_local_borrow = self.local_borrow_vars.contains(&r);
                        if is_borrow_ptr || is_local_borrow {
                            let span =
                                Self::find_ident_span_in_arrow(arrow, &r).unwrap_or(arrow.span);
                            return Err(SemaError::AsyncIsolationViolation {
                                name: r.clone(),
                                span,
                            });
                        }
                        if !self.is_safe_concurrency_type(ty) && !moved_in_closure.contains(&r) {
                            let span =
                                Self::find_ident_span_in_arrow(arrow, &r).unwrap_or(arrow.span);
                            return Err(SemaError::AsyncIsolationViolation {
                                name: r.clone(),
                                span,
                            });
                        }
                    }
                }
            }
            return Ok(());
        }

        match arg {
            Expr::Move(_) => Ok(()),
            Expr::Borrow(_, _) => Err(SemaError::AsyncIsolationViolation {
                name: String::new(),
                span: arg.span(),
            }),
            Expr::Slice(_) => Err(SemaError::AsyncIsolationViolation {
                name: String::new(),
                span: arg.span(),
            }),
            Expr::Lit(Lit::Str(_)) => Ok(()),
            Expr::Ident(id) => {
                if self.local_borrow_vars.contains(&id.sym) {
                    return Err(SemaError::AsyncIsolationViolation {
                        name: id.sym.clone(),
                        span: id.span,
                    });
                }
                if let Some(SymbolKind::Var { ty, .. }) = self.resolve(&id.sym) {
                    if matches!(
                        ty,
                        Type::BorrowType { .. }
                            | Type::RawPtr { .. }
                            | Type::ManyRawPtr(_)
                            | Type::CPtr(_)
                    ) {
                        return Err(SemaError::AsyncIsolationViolation {
                            name: id.sym.clone(),
                            span: id.span,
                        });
                    }
                }
                Ok(())
            }
            _ => {
                if let Ok(arg_ty) = self.infer_type(arg) {
                    if matches!(
                        arg_ty,
                        Type::BorrowType { .. }
                            | Type::RawPtr { .. }
                            | Type::ManyRawPtr(_)
                            | Type::CPtr(_)
                    ) {
                        return Err(SemaError::AsyncIsolationViolation {
                            name: String::new(),
                            span: arg.span(),
                        });
                    }
                }
                Ok(())
            }
        }
    }

    pub fn check_async_isolation_for_call(&mut self, call: &CallExpr) -> SemaResult<()> {
        let callee_name = match &*call.callee {
            Expr::Ident(id) => id.sym.clone(),
            Expr::Member(mem) => {
                if let Expr::Ident(obj_id) = &*mem.obj {
                    if obj_id.sym == "spawn" && mem.prop.sym == "cpu" {
                        "spawn.cpu".to_string()
                    } else if mem.prop.sym == "send" {
                        "method_send".to_string()
                    } else {
                        String::new()
                    }
                } else if mem.prop.sym == "send" {
                    "method_send".to_string()
                } else {
                    String::new()
                }
            }
            _ => String::new(),
        };

        if callee_name == "spawn"
            || callee_name == "@task_spawn"
            || callee_name == "@task_spawn_cpu"
            || callee_name == "@worker_spawn"
            || callee_name == "spawn.cpu"
        {
            for arg in &call.args {
                self.check_async_boundary_arg(arg, call.span)?;
            }
        } else if callee_name == "send" {
            if call.args.len() >= 2 {
                self.check_async_boundary_arg(&call.args[1], call.span)?;
            }
        } else if callee_name == "method_send" {
            if !call.args.is_empty() {
                self.check_async_boundary_arg(&call.args[0], call.span)?;
            }
        }
        Ok(())
    }

    pub fn check_async_isolation_for_builtin(&mut self, call: &BuiltinCall) -> SemaResult<()> {
        if call.name == "@spawn"
            || call.name == "@task_spawn"
            || call.name == "@task_spawn_cpu"
            || call.name == "@worker_spawn"
        {
            for arg in &call.args {
                self.check_async_boundary_arg(arg, call.span)?;
            }
        } else if call.name == "@send" {
            if call.args.len() >= 2 {
                self.check_async_boundary_arg(&call.args[1], call.span)?;
            }
        }
        Ok(())
    }

    pub(super) fn define(&mut self, name: String, kind: SymbolKind, _span: Span) -> SemaResult<()> {
        if self.scopes.last().unwrap().contains_key(&name) {
            // Already defined in current scope
        }
        self.scopes.last_mut().unwrap().insert(name, kind);
        Ok(())
    }
    pub(super) fn resolve(&self, name: &str) -> Option<&SymbolKind> {
        for scope in self.scopes.iter().rev() {
            if let Some(kind) = scope.get(name) {
                return Some(kind);
            }
        }
        None
    }

    pub fn unwrap_optional_type(ty: &Type) -> Type {
        match ty {
            Type::Optional(inner) => *inner.clone(),
            Type::BorrowType {
                inner,
                is_mut,
                lifetimes,
            } => {
                if let Type::Optional(opt_inner) = &**inner {
                    Type::BorrowType {
                        inner: opt_inner.clone(),
                        is_mut: *is_mut,
                        lifetimes: lifetimes.clone(),
                    }
                } else {
                    ty.clone()
                }
            }
            _ => ty.clone(),
        }
    }

    pub fn extract_not_null_vars(expr: &Expr) -> Vec<String> {
        let mut vars = Vec::new();
        match expr {
            Expr::Bin(bin) => {
                if bin.op == BinaryOp::NotEq {
                    if matches!(&*bin.right, Expr::Lit(Lit::Null))
                        || matches!(&*bin.right, Expr::Ident(id) if id.sym == "null")
                    {
                        if let Expr::Ident(id) = &*bin.left {
                            if id.sym != "null" && id.sym != "undefined" {
                                vars.push(id.sym.clone());
                            }
                        }
                    } else if matches!(&*bin.left, Expr::Lit(Lit::Null))
                        || matches!(&*bin.left, Expr::Ident(id) if id.sym == "null")
                    {
                        if let Expr::Ident(id) = &*bin.right {
                            if id.sym != "null" && id.sym != "undefined" {
                                vars.push(id.sym.clone());
                            }
                        }
                    }
                } else if bin.op == BinaryOp::LogicalAnd {
                    vars.extend(Self::extract_not_null_vars(&bin.left));
                    vars.extend(Self::extract_not_null_vars(&bin.right));
                }
            }
            Expr::Builtin(call) if call.name == "@not" && call.args.len() == 1 => {
                vars.extend(Self::extract_is_null_vars(&call.args[0]));
            }
            _ => {}
            _ => {}
        }
        vars
    }

    pub fn extract_is_null_vars(expr: &Expr) -> Vec<String> {
        let mut vars = Vec::new();
        match expr {
            Expr::Bin(bin) => {
                if bin.op == BinaryOp::Eq {
                    if matches!(&*bin.right, Expr::Lit(Lit::Null))
                        || matches!(&*bin.right, Expr::Ident(id) if id.sym == "null")
                    {
                        if let Expr::Ident(id) = &*bin.left {
                            if id.sym != "null" && id.sym != "undefined" {
                                vars.push(id.sym.clone());
                            }
                        }
                    } else if matches!(&*bin.left, Expr::Lit(Lit::Null))
                        || matches!(&*bin.left, Expr::Ident(id) if id.sym == "null")
                    {
                        if let Expr::Ident(id) = &*bin.right {
                            if id.sym != "null" && id.sym != "undefined" {
                                vars.push(id.sym.clone());
                            }
                        }
                    }
                } else if bin.op == BinaryOp::LogicalOr {
                    vars.extend(Self::extract_is_null_vars(&bin.left));
                    vars.extend(Self::extract_is_null_vars(&bin.right));
                }
            }
            Expr::Builtin(call) if call.name == "@not" && call.args.len() == 1 => {
                vars.extend(Self::extract_not_null_vars(&call.args[0]));
            }
            _ => {}
            _ => {}
        }
        vars
    }
}
