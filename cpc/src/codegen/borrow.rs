use super::*;
use crate::ast::*;
use crate::Span;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

impl Codegen {
    pub(crate) fn compute_last_uses_block(&mut self, block: &BlockStmt, counter: &mut usize) {
        for s in &block.body {
            self.compute_last_uses_stmt(s, counter);
        }
    }

    pub(crate) fn compute_last_uses_stmt(&mut self, stmt: &Stmt, counter: &mut usize) {
        *counter += 1;
        let idx = *counter;
        let mut refs = HashSet::new();
        match stmt {
            Stmt::Var(v) => {
                if let Some(init) = &v.init {
                    crate::ast_visitor::find_referenced_vars_in_expr(init, &mut refs);
                }
                for r in refs {
                    self.last_use_map.insert(r, idx);
                }
            }
            Stmt::Expr(e) => {
                if let Expr::Bin(bin) = e {
                    if bin.op == BinaryOp::Assign {
                        crate::ast_visitor::find_referenced_vars_in_expr(&bin.right, &mut refs);
                        match &*bin.left {
                            Expr::Member(mem) => {
                                crate::ast_visitor::find_referenced_vars_in_expr(
                                    &mem.obj, &mut refs,
                                );
                            }
                            Expr::Deref(inner) => {
                                crate::ast_visitor::find_referenced_vars_in_expr(inner, &mut refs);
                            }
                            Expr::Index { obj, index } => {
                                crate::ast_visitor::find_referenced_vars_in_expr(obj, &mut refs);
                                crate::ast_visitor::find_referenced_vars_in_expr(index, &mut refs);
                            }
                            _ => {}
                        }
                        for r in refs {
                            self.last_use_map.insert(r, idx);
                        }
                        return;
                    }
                }
                crate::ast_visitor::find_referenced_vars_in_expr(e, &mut refs);
                for r in refs {
                    self.last_use_map.insert(r, idx);
                }
            }
            Stmt::Return(ret) => {
                if let Some(arg) = &ret.arg {
                    crate::ast_visitor::find_referenced_vars_in_expr(arg, &mut refs);
                }
                for r in refs {
                    self.last_use_map.insert(r, idx);
                }
            }
            Stmt::Break | Stmt::Continue => {}
            Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
                self.compute_last_uses_block(b, counter)
            }
            Stmt::If(i) => {
                crate::ast_visitor::find_referenced_vars_in_expr(&i.test, &mut refs);
                for r in refs {
                    self.last_use_map.insert(r, idx);
                }
                if let Stmt::Block(b) = &*i.cons {
                    self.compute_last_uses_block(b, counter);
                } else {
                    self.compute_last_uses_stmt(&i.cons, counter);
                }
                if let Some(alt) = &i.alt {
                    if let Stmt::Block(b) = &**alt {
                        self.compute_last_uses_block(b, counter);
                    } else {
                        self.compute_last_uses_stmt(alt, counter);
                    }
                }
            }
            Stmt::While(w) => {
                crate::ast_visitor::find_referenced_vars_in_expr(&w.test, &mut refs);
                if let Some(c) = &w.continue_expr {
                    crate::ast_visitor::find_referenced_vars_in_expr(c, &mut refs);
                }
                for r in refs {
                    self.last_use_map.insert(r, idx);
                }
                if let Stmt::Block(b) = &*w.body {
                    self.compute_last_uses_block(b, counter);
                } else {
                    self.compute_last_uses_stmt(&w.body, counter);
                }
            }
            Stmt::For(f) => {
                crate::ast_visitor::find_referenced_vars_in_expr(&f.expr, &mut refs);
                for r in refs {
                    self.last_use_map.insert(r, idx);
                }
                if let Stmt::Block(b) = &*f.body {
                    self.compute_last_uses_block(b, counter);
                } else {
                    self.compute_last_uses_stmt(&f.body, counter);
                }
            }
            Stmt::Match(m) => {
                crate::ast_visitor::find_referenced_vars_in_expr(&m.disc, &mut refs);
                for r in refs {
                    self.last_use_map.insert(r, idx);
                }
                for arm in &m.arms {
                    if let Some(g) = &arm.guard {
                        let mut grefs = HashSet::new();
                        crate::ast_visitor::find_referenced_vars_in_expr(g, &mut grefs);
                        for r in grefs {
                            self.last_use_map.insert(r, idx);
                        }
                    }
                    if let Stmt::Block(b) = &*arm.body {
                        self.compute_last_uses_block(b, counter);
                    } else {
                        self.compute_last_uses_stmt(&arm.body, counter);
                    }
                }
            }
            Stmt::TryCatch {
                try_block,
                catch_body,
                ..
            } => {
                self.compute_last_uses_block(try_block, counter);
                if let Some(cb) = catch_body {
                    if let Stmt::Block(b) = &**cb {
                        self.compute_last_uses_block(b, counter);
                    } else {
                        self.compute_last_uses_stmt(cb, counter);
                    }
                }
            }
            Stmt::Defer(d) => self.compute_last_uses_stmt(d, counter),
            Stmt::ErrDefer(d) => self.compute_last_uses_stmt(d, counter),
            _ => {}
        }
    }

    pub(crate) fn check_active_borrows_before_move(&self, var_name: &str) {
        for (borrower, (sources, _)) in &self.var_borrow_sources {
            if sources.iter().any(|(s, _)| {
                s == var_name
                    || ((s == "this" || s == "_") && (var_name == "this" || var_name == "_"))
            }) {
                let last_use = self.last_use_map.get(borrower).copied().unwrap_or(0);
                if last_use > self.current_stmt_idx {
                    panic!(
                        "CP Compile Error: Cannot move '{}' because it is currently borrowed by '{}' (active until statement #{})",
                        var_name, borrower, last_use
                    );
                }
            }
        }
    }

    pub(crate) fn check_borrow_conflict(&self, target_var: &str, req_mut: bool) {
        self.check_borrow_conflict_field_excluding(target_var, None, "", req_mut);
    }

    pub(crate) fn check_borrow_conflict_field(
        &self,
        target_var: &str,
        target_field: Option<&str>,
        req_mut: bool,
    ) {
        self.check_borrow_conflict_field_excluding(target_var, target_field, "", req_mut);
    }

    #[allow(dead_code)]
    pub(crate) fn check_borrow_conflict_excluding(
        &self,
        target_var: &str,
        exclude_borrower: &str,
        req_mut: bool,
    ) {
        self.check_borrow_conflict_field_excluding(target_var, None, exclude_borrower, req_mut);
    }

    pub(crate) fn check_borrow_conflict_field_excluding(
        &self,
        target_var: &str,
        target_field: Option<&str>,
        exclude_borrower: &str,
        req_mut: bool,
    ) {
        for (borrower, (sources, is_mut_borrow)) in &self.var_borrow_sources {
            if borrower == exclude_borrower {
                continue;
            }
            let last_use = self.last_use_map.get(borrower).copied().unwrap_or(0);
            if last_use >= self.current_stmt_idx {
                for (src_var, src_field) in sources {
                    if Self::targets_conflict(
                        target_var,
                        target_field,
                        req_mut,
                        src_var,
                        src_field.as_deref(),
                        *is_mut_borrow,
                    ) {
                        let target_desc = if let Some(f) = target_field {
                            format!("{}.{}", target_var, f)
                        } else {
                            target_var.to_string()
                        };
                        let func_info = format!(
                            "in {}.{}: ",
                            self.current_class_name.as_deref().unwrap_or("<global>"),
                            self.current_func_name.as_deref().unwrap_or("<anon>")
                        );
                        panic!(
                            "CP Compile Error: {}Borrow conflict on '{}': cannot borrow as {} while '{}' is still active (stmt_idx={}, last_use={})",
                            func_info,
                            target_desc,
                            if req_mut { "mutable" } else { "immutable" },
                            borrower,
                            self.current_stmt_idx,
                            last_use
                        );
                    }
                }
            }
        }
    }

    pub(crate) fn targets_conflict(
        target_a: &str,
        field_a: Option<&str>,
        is_mut_a: bool,
        target_b: &str,
        field_b: Option<&str>,
        is_mut_b: bool,
    ) -> bool {
        let is_same_root = target_a == target_b
            || ((target_a == "this" || target_a == "_") && (target_b == "this" || target_b == "_"));
        if !is_same_root {
            return false;
        }
        if !is_mut_a && !is_mut_b {
            return false;
        }
        match (field_a, field_b) {
            (None, _) | (_, None) => true,
            (Some(fa), Some(fb)) => {
                if fa == fb {
                    return true;
                }
                if fa.starts_with(fb) && fa.as_bytes().get(fb.len()) == Some(&b'.') {
                    return true;
                }
                if fb.starts_with(fa) && fb.as_bytes().get(fa.len()) == Some(&b'.') {
                    return true;
                }
                false
            }
        }
    }

    pub(crate) fn extract_member_path(&self, expr: &Expr) -> (Option<String>, Option<String>) {
        match expr {
            Expr::Ident(id) => (Some(id.sym.clone()), None),
            Expr::Member(mem) => {
                let mut parts = vec![mem.prop.sym.clone()];
                let mut curr = &*mem.obj;
                loop {
                    match curr {
                        Expr::Member(inner_mem) => {
                            parts.push(inner_mem.prop.sym.clone());
                            curr = &*inner_mem.obj;
                        }
                        Expr::Ident(id) => {
                            parts.reverse();
                            return (Some(id.sym.clone()), Some(parts.join(".")));
                        }
                        Expr::Deref(inner) | Expr::Unwrap(inner) => {
                            curr = inner;
                        }
                        _ => return (None, None),
                    }
                }
            }
            Expr::Deref(inner) | Expr::Unwrap(inner) => self.extract_member_path(inner),
            _ => (None, None),
        }
    }

    pub(crate) fn resolve_borrow_source_field(
        &self,
        root: &str,
        field: Option<&str>,
    ) -> Vec<(String, Option<String>)> {
        if root == "this" || root == "_" {
            return vec![
                ("this".to_string(), field.map(|f| f.to_string())),
                ("_".to_string(), field.map(|f| f.to_string())),
            ];
        }
        if let Some((orig_sources, _)) = self.var_borrow_sources.get(root) {
            if !orig_sources.is_empty() {
                let mut resolved = Vec::new();
                for (orig_root, orig_field) in orig_sources {
                    let combined_field = match (orig_field, field) {
                        (Some(of), Some(f)) => Some(format!("{}.{}", of, f)),
                        (Some(of), None) => Some(of.clone()),
                        (None, Some(f)) => Some(f.to_string()),
                        (None, None) => None,
                    };
                    resolved.push((orig_root.clone(), combined_field));
                }
                return resolved;
            }
        }
        vec![(root.to_string(), field.map(|f| f.to_string()))]
    }

    pub(crate) fn find_method_borrowed_field(
        &self,
        class_name: &str,
        method_name: &str,
    ) -> Option<String> {
        let mut visited = HashSet::new();
        self.find_method_borrowed_field_depth(class_name, method_name, 0, &mut visited)
    }

    fn find_method_borrowed_field_depth(
        &self,
        class_name: &str,
        method_name: &str,
        depth: usize,
        visited: &mut HashSet<String>,
    ) -> Option<String> {
        if depth > 10 || !visited.insert(format!("{}::{}", class_name, method_name)) {
            return None;
        }
        let clean = class_name.trim_start_matches('?').trim_start_matches('*');
        let base_name = if let Some(pos) = clean.find('<') {
            &clean[..pos]
        } else if let Some(pos) = clean.find('(') {
            &clean[..pos]
        } else {
            clean
        };
        let cls = self.class_defs.get(base_name)?;
        let method = cls.methods.iter().find(|m| m.ident.sym == method_name)?;

        let mut found_fields = HashSet::new();
        self.collect_return_borrow_fields(
            &method.body,
            base_name,
            depth,
            visited,
            &mut found_fields,
        );

        if found_fields.len() == 1 {
            return found_fields.into_iter().next();
        }

        if let Some(ret_ty) = &method.return_type {
            if matches!(ret_ty, Type::BorrowType { is_mut: false, .. }) {
                for f in &cls.fields {
                    if (f.ident.sym == "src"
                        || f.ident.sym == "input"
                        || f.ident.sym == "buffer"
                        || f.ident.sym == "buf"
                        || f.ident.sym == "data")
                        && matches!(f.ty, Type::BorrowType { .. } | Type::Array(_))
                    {
                        return Some(f.ident.sym.clone());
                    }
                }
            }
        }

        None
    }

    fn collect_return_borrow_fields(
        &self,
        block: &BlockStmt,
        class_name: &str,
        depth: usize,
        visited: &mut HashSet<String>,
        fields: &mut HashSet<String>,
    ) {
        for s in &block.body {
            match s {
                Stmt::Return(ret) => {
                    if let Some(arg) = &ret.arg {
                        if let Some(f) =
                            self.extract_expr_borrowed_field(arg, block, class_name, depth, visited)
                        {
                            fields.insert(f);
                        }
                    }
                }
                Stmt::Block(b)
                | Stmt::UnsafeBlock(b)
                | Stmt::ArenaBlock(b)
                | Stmt::ComptimeBlock(b) => {
                    self.collect_return_borrow_fields(b, class_name, depth, visited, fields);
                }
                Stmt::If(i) => {
                    if let Stmt::Block(b) = &*i.cons {
                        self.collect_return_borrow_fields(b, class_name, depth, visited, fields);
                    } else if let Stmt::Return(r) = &*i.cons {
                        if let Some(arg) = &r.arg {
                            if let Some(f) = self
                                .extract_expr_borrowed_field(arg, block, class_name, depth, visited)
                            {
                                fields.insert(f);
                            }
                        }
                    }
                    if let Some(alt) = &i.alt {
                        if let Stmt::Block(b) = &**alt {
                            self.collect_return_borrow_fields(
                                b, class_name, depth, visited, fields,
                            );
                        } else if let Stmt::Return(r) = &**alt {
                            if let Some(arg) = &r.arg {
                                if let Some(f) = self.extract_expr_borrowed_field(
                                    arg, block, class_name, depth, visited,
                                ) {
                                    fields.insert(f);
                                }
                            }
                        }
                    }
                }
                Stmt::While(w) => {
                    if let Stmt::Block(b) = &*w.body {
                        self.collect_return_borrow_fields(b, class_name, depth, visited, fields);
                    }
                }
                Stmt::For(f) => {
                    if let Stmt::Block(b) = &*f.body {
                        self.collect_return_borrow_fields(b, class_name, depth, visited, fields);
                    }
                }
                Stmt::Match(m) => {
                    for arm in &m.arms {
                        if let Stmt::Block(b) = &*arm.body {
                            self.collect_return_borrow_fields(
                                b, class_name, depth, visited, fields,
                            );
                        } else if let Stmt::Return(r) = &*arm.body {
                            if let Some(arg) = &r.arg {
                                if let Some(f) = self.extract_expr_borrowed_field(
                                    arg, block, class_name, depth, visited,
                                ) {
                                    fields.insert(f);
                                }
                            }
                        }
                    }
                }
                _ => {}
            }
        }
    }

    fn extract_expr_borrowed_field(
        &self,
        expr: &Expr,
        block: &BlockStmt,
        class_name: &str,
        depth: usize,
        visited: &mut HashSet<String>,
    ) -> Option<String> {
        match expr {
            Expr::Member(mem) => {
                if matches!(&*mem.obj, Expr::Ident(id) if id.sym == "_" || id.sym == "this" || id.sym == "self")
                {
                    return Some(mem.prop.sym.clone());
                }
                let (root, field) = self.extract_member_path(expr);
                if matches!(root.as_deref(), Some("_") | Some("this") | Some("self")) {
                    return field;
                }
                None
            }
            Expr::Index { obj, .. } => {
                self.extract_expr_borrowed_field(obj, block, class_name, depth, visited)
            }
            Expr::Call(call) => {
                match &*call.callee {
                    Expr::Member(mem) => {
                        if matches!(&*mem.obj, Expr::Ident(id) if id.sym == "_" || id.sym == "this" || id.sym == "self")
                        {
                            return self.find_method_borrowed_field_depth(
                                class_name,
                                &mem.prop.sym,
                                depth + 1,
                                visited,
                            );
                        }
                    }
                    Expr::Ident(id) => {
                        if let Some(cls) = self.class_defs.get(class_name) {
                            if cls.methods.iter().any(|m| m.ident.sym == id.sym) {
                                return self.find_method_borrowed_field_depth(
                                    class_name,
                                    &id.sym,
                                    depth + 1,
                                    visited,
                                );
                            }
                        }
                    }
                    _ => {}
                }
                None
            }
            Expr::Question(inner) | Expr::Unwrap(inner) | Expr::Deref(inner) => {
                self.extract_expr_borrowed_field(inner, block, class_name, depth, visited)
            }
            Expr::Ident(id) => {
                for s in &block.body {
                    if let Stmt::Var(v) = s {
                        if let Pattern::Ident(ident) = &v.pat {
                            if ident.sym == id.sym {
                                if let Some(init) = &v.init {
                                    return self.extract_expr_borrowed_field(
                                        init, block, class_name, depth, visited,
                                    );
                                }
                            }
                        }
                    }
                }
                None
            }
            _ => None,
        }
    }

    pub(crate) fn find_method_mutated_fields(
        &self,
        class_name: &str,
        method_name: &str,
    ) -> Option<HashSet<String>> {
        let mut visited = HashSet::new();
        self.find_method_mutated_fields_depth(class_name, method_name, 0, &mut visited)
    }

    fn find_method_mutated_fields_depth(
        &self,
        class_name: &str,
        method_name: &str,
        depth: usize,
        visited: &mut HashSet<String>,
    ) -> Option<HashSet<String>> {
        if depth > 10 || !visited.insert(format!("{}::{}", class_name, method_name)) {
            return None;
        }
        let clean = class_name.trim_start_matches('?').trim_start_matches('*');
        let base_name = if let Some(pos) = clean.find('<') {
            &clean[..pos]
        } else if let Some(pos) = clean.find('(') {
            &clean[..pos]
        } else {
            clean
        };
        let cls = self.class_defs.get(base_name)?;
        let method = cls.methods.iter().find(|m| m.ident.sym == method_name)?;

        if method.body.body.is_empty() {
            return None;
        }

        let mut mutated = HashSet::new();
        if self.collect_mutated_fields_in_block(
            &method.body,
            base_name,
            depth,
            visited,
            &mut mutated,
        ) {
            Some(mutated)
        } else {
            None
        }
    }

    fn collect_mutated_fields_in_block(
        &self,
        block: &BlockStmt,
        class_name: &str,
        depth: usize,
        visited: &mut HashSet<String>,
        mutated: &mut HashSet<String>,
    ) -> bool {
        for s in &block.body {
            if !self.collect_mutated_fields_in_stmt(s, class_name, depth, visited, mutated) {
                return false;
            }
        }
        true
    }

    fn collect_mutated_fields_in_stmt(
        &self,
        stmt: &Stmt,
        class_name: &str,
        depth: usize,
        visited: &mut HashSet<String>,
        mutated: &mut HashSet<String>,
    ) -> bool {
        match stmt {
            Stmt::Expr(e) => {
                self.collect_mutated_fields_in_expr(e, class_name, depth, visited, mutated)
            }
            Stmt::Var(v) => {
                if let Some(init) = &v.init {
                    self.collect_mutated_fields_in_expr(init, class_name, depth, visited, mutated)
                } else {
                    true
                }
            }
            Stmt::Return(r) => {
                if let Some(arg) = &r.arg {
                    self.collect_mutated_fields_in_expr(arg, class_name, depth, visited, mutated)
                } else {
                    true
                }
            }
            Stmt::Block(b)
            | Stmt::UnsafeBlock(b)
            | Stmt::ArenaBlock(b)
            | Stmt::ComptimeBlock(b) => {
                self.collect_mutated_fields_in_block(b, class_name, depth, visited, mutated)
            }
            Stmt::If(i) => {
                let test_ok = self
                    .collect_mutated_fields_in_expr(&i.test, class_name, depth, visited, mutated);
                let cons_ok = match &*i.cons {
                    Stmt::Block(b) => {
                        self.collect_mutated_fields_in_block(b, class_name, depth, visited, mutated)
                    }
                    other => self
                        .collect_mutated_fields_in_stmt(other, class_name, depth, visited, mutated),
                };
                let alt_ok = if let Some(alt) = &i.alt {
                    match &**alt {
                        Stmt::Block(b) => self.collect_mutated_fields_in_block(
                            b, class_name, depth, visited, mutated,
                        ),
                        other => self.collect_mutated_fields_in_stmt(
                            other, class_name, depth, visited, mutated,
                        ),
                    }
                } else {
                    true
                };
                test_ok && cons_ok && alt_ok
            }
            Stmt::While(w) => {
                let test_ok = self
                    .collect_mutated_fields_in_expr(&w.test, class_name, depth, visited, mutated);
                let body_ok = match &*w.body {
                    Stmt::Block(b) => {
                        self.collect_mutated_fields_in_block(b, class_name, depth, visited, mutated)
                    }
                    other => self
                        .collect_mutated_fields_in_stmt(other, class_name, depth, visited, mutated),
                };
                test_ok && body_ok
            }
            Stmt::For(f) => {
                let iter_ok = self
                    .collect_mutated_fields_in_expr(&f.expr, class_name, depth, visited, mutated);
                let body_ok = match &*f.body {
                    Stmt::Block(b) => {
                        self.collect_mutated_fields_in_block(b, class_name, depth, visited, mutated)
                    }
                    other => self
                        .collect_mutated_fields_in_stmt(other, class_name, depth, visited, mutated),
                };
                iter_ok && body_ok
            }
            Stmt::Match(m) => {
                let disc_ok = self
                    .collect_mutated_fields_in_expr(&m.disc, class_name, depth, visited, mutated);
                if !disc_ok {
                    return false;
                }
                for arm in &m.arms {
                    let ok = match &*arm.body {
                        Stmt::Block(b) => self.collect_mutated_fields_in_block(
                            b, class_name, depth, visited, mutated,
                        ),
                        other => self.collect_mutated_fields_in_stmt(
                            other, class_name, depth, visited, mutated,
                        ),
                    };
                    if !ok {
                        return false;
                    }
                }
                true
            }
            Stmt::TryCatch {
                try_block,
                catch_body,
                ..
            } => {
                let try_ok = self.collect_mutated_fields_in_block(
                    try_block, class_name, depth, visited, mutated,
                );
                let catch_ok = if let Some(cb) = catch_body {
                    self.collect_mutated_fields_in_stmt(cb, class_name, depth, visited, mutated)
                } else {
                    true
                };
                try_ok && catch_ok
            }
            Stmt::Defer(d) | Stmt::ErrDefer(d) => {
                self.collect_mutated_fields_in_stmt(d, class_name, depth, visited, mutated)
            }
            _ => true,
        }
    }

    fn collect_mutated_fields_in_expr(
        &self,
        expr: &Expr,
        class_name: &str,
        depth: usize,
        visited: &mut HashSet<String>,
        mutated: &mut HashSet<String>,
    ) -> bool {
        match expr {
            Expr::Bin(bin) => {
                if bin.op == BinaryOp::Assign {
                    match &*bin.left {
                        Expr::Member(m) => {
                            if matches!(&*m.obj, Expr::Ident(id) if id.sym == "_" || id.sym == "this" || id.sym == "self")
                            {
                                mutated.insert(m.prop.sym.clone());
                            } else {
                                let (root, field) = self.extract_member_path(&bin.left);
                                if matches!(
                                    root.as_deref(),
                                    Some("_") | Some("this") | Some("self")
                                ) {
                                    if let Some(f) = field {
                                        mutated.insert(f);
                                    }
                                }
                            }
                        }
                        Expr::Ident(id) => {
                            if id.sym == "_" || id.sym == "this" || id.sym == "self" {
                                return false;
                            }
                            if let Some(cls) = self.class_defs.get(class_name) {
                                if cls.fields.iter().any(|f| f.ident.sym == id.sym) {
                                    mutated.insert(id.sym.clone());
                                }
                            }
                        }
                        Expr::Index { obj, .. } => {
                            let (root, field) = self.extract_member_path(obj);
                            if matches!(root.as_deref(), Some("_") | Some("this") | Some("self")) {
                                if let Some(f) = field {
                                    mutated.insert(f);
                                }
                            }
                        }
                        _ => {}
                    }
                }
                self.collect_mutated_fields_in_expr(&bin.left, class_name, depth, visited, mutated)
                    && self.collect_mutated_fields_in_expr(
                        &bin.right, class_name, depth, visited, mutated,
                    )
            }
            Expr::Call(call) => {
                match &*call.callee {
                    Expr::Member(mem) => {
                        if matches!(&*mem.obj, Expr::Ident(id) if id.sym == "_" || id.sym == "this" || id.sym == "self")
                        {
                            if let Some(sub_mutated) = self.find_method_mutated_fields_depth(
                                class_name,
                                &mem.prop.sym,
                                depth + 1,
                                visited,
                            ) {
                                mutated.extend(sub_mutated);
                            } else {
                                return false;
                            }
                        }
                    }
                    Expr::Ident(id) => {
                        if let Some(cls) = self.class_defs.get(class_name) {
                            if cls.methods.iter().any(|m| m.ident.sym == id.sym) {
                                if let Some(sub_mutated) = self.find_method_mutated_fields_depth(
                                    class_name,
                                    &id.sym,
                                    depth + 1,
                                    visited,
                                ) {
                                    mutated.extend(sub_mutated);
                                } else {
                                    return false;
                                }
                            }
                        }
                    }
                    _ => {}
                }
                for a in &call.args {
                    if !self.collect_mutated_fields_in_expr(a, class_name, depth, visited, mutated)
                    {
                        return false;
                    }
                }
                true
            }
            Expr::Deref(p)
            | Expr::Unwrap(p)
            | Expr::Question(p)
            | Expr::BitNot(p)
            | Expr::ErrWrap(p)
            | Expr::BoxNew(p)
            | Expr::Await(p)
            | Expr::Move(p)
            | Expr::ComptimeExpr(p) => {
                self.collect_mutated_fields_in_expr(p, class_name, depth, visited, mutated)
            }
            Expr::Member(m) => {
                self.collect_mutated_fields_in_expr(&m.obj, class_name, depth, visited, mutated)
            }
            Expr::Index { obj, index } => {
                self.collect_mutated_fields_in_expr(obj, class_name, depth, visited, mutated)
                    && self
                        .collect_mutated_fields_in_expr(index, class_name, depth, visited, mutated)
            }
            Expr::If {
                test, cons, alt, ..
            } => {
                self.collect_mutated_fields_in_expr(test, class_name, depth, visited, mutated)
                    && self
                        .collect_mutated_fields_in_expr(cons, class_name, depth, visited, mutated)
                    && alt.as_ref().map_or(true, |a| {
                        self.collect_mutated_fields_in_expr(a, class_name, depth, visited, mutated)
                    })
            }
            Expr::LabeledBlock { block: b, .. }
            | Expr::Unsafe(b)
            | Expr::Arena(b)
            | Expr::Comptime(b) => {
                self.collect_mutated_fields_in_block(b, class_name, depth, visited, mutated)
            }
            Expr::Array(items) => items.iter().all(|i| {
                self.collect_mutated_fields_in_expr(i, class_name, depth, visited, mutated)
            }),
            Expr::Object(fields) => fields.iter().all(|f| {
                self.collect_mutated_fields_in_expr(&f.val, class_name, depth, visited, mutated)
            }),
            Expr::QuestionFallback {
                expr: e, fallback, ..
            } => {
                self.collect_mutated_fields_in_expr(e, class_name, depth, visited, mutated)
                    && self.collect_mutated_fields_in_expr(
                        fallback, class_name, depth, visited, mutated,
                    )
            }
            Expr::NullCoalesce { left, right } => {
                self.collect_mutated_fields_in_expr(left, class_name, depth, visited, mutated)
                    && self
                        .collect_mutated_fields_in_expr(right, class_name, depth, visited, mutated)
            }
            Expr::Catch { expr: e, body, .. } => {
                self.collect_mutated_fields_in_expr(e, class_name, depth, visited, mutated)
                    && self
                        .collect_mutated_fields_in_stmt(body, class_name, depth, visited, mutated)
            }
            Expr::Borrow(inner, _) => {
                self.collect_mutated_fields_in_expr(inner, class_name, depth, visited, mutated)
            }
            _ => true,
        }
    }

    pub(crate) fn is_zig_heap_ptr_type(&self, ty: &Type) -> bool {
        match ty {
            Type::Map(_, _) | Type::Dict(_, _) | Type::Set(_) => true,
            Type::BorrowType { .. } | Type::RawPtr { .. } => true,
            Type::Ref(cid) => {
                if cid.sym == "Map"
                    || cid.sym == "Dict"
                    || cid.sym == "Set"
                    || cid.sym == "RegExp"
                    || cid.sym == "Task"
                    || cid.sym == "Server"
                    || cid.sym == "Socket"
                    || cid.sym == "Request"
                    || cid.sym == "Response"
                    || cid.sym == "VecDeque"
                    || cid.sym == "BinaryHeap"
                    || cid.sym == "BTreeMap"
                    || cid.sym == "BTreeSet"
                    || cid.sym == "HashMap"
                    || cid.sym == "HashSet"
                    || self.ref_classes.contains(&cid.sym)
                {
                    return true;
                }
                if self.value_classes.contains(&cid.sym) {
                    return false;
                }
                self.class_defs.get(&cid.sym).map_or(false, |c| !c.is_ref)
            }
            _ => false,
        }
    }

    pub(crate) fn check_mutation_allowed(&self, target_expr: &Expr, op_desc: &str) {
        if self.is_lib_mode {
            return;
        }
        match target_expr {
            Expr::Ident(id) => {
                if id.sym == "this" || id.sym == "_" || self.is_class_field(&id.sym) {
                    if self.moved_vars.contains("this") || self.moved_vars.contains("_") {
                        panic!("CP Compile Error: Use of moved value '{}'", id.sym);
                    }
                    if self.current_zig_fn_name.ends_with("_init") {
                        return;
                    }
                    if self.is_class_field(&id.sym) {
                        if let Some(c_name) = &self.current_class_name {
                            if let Some(cls) = self.class_defs.get(c_name) {
                                if let Some(f) = cls.fields.iter().find(|f| f.ident.sym == id.sym) {
                                    if matches!(
                                        f.ty,
                                        Type::BorrowType { is_mut: true, .. } | Type::RawPtr { .. }
                                    ) {
                                        return;
                                    }
                                    if matches!(f.ty, Type::BorrowType { is_mut: false, .. })
                                        && !op_desc.starts_with("field assignment")
                                    {
                                        panic!(
                                            "CP Compile Error: Cannot perform mutable operation ({}) through read-only borrow field '{}'",
                                            op_desc, id.sym
                                        );
                                    }
                                }
                            }
                        }
                    }
                    if self.current_receiver_mode == ReceiverMode::ReadOnly {
                        panic!(
                            "CP Compile Error: Cannot perform mutable operation ({}) on '{}' in a read-only method (add '*_' as first parameter)",
                            op_desc, id.sym
                        );
                    }
                    self.check_borrow_conflict("this", true);
                    self.check_borrow_conflict("_", true);
                    return;
                }
                if self.const_vars.contains(&id.sym) && !self.type_namespace_vars.contains(&id.sym)
                {
                    let allow_pointee_mut = op_desc != "assignment '='"
                        && self.var_cp_types.get(&id.sym).map_or(false, |t| {
                            matches!(
                                t,
                                Type::RawPtr { .. }
                                    | Type::ManyRawPtr(_)
                                    | Type::CPtr(_)
                                    | Type::SentinelPtr(_, _)
                                    | Type::BorrowType { is_mut: true, .. }
                                    | Type::Auto
                                    | Type::Array(_)
                            )
                        });
                    if !allow_pointee_mut {
                        panic!(
                            "CP Compile Error: Cannot perform mutable operation ({}) on 'const' variable '{}' (declare with 'let' to allow mutation)",
                            op_desc, id.sym
                        );
                    }
                }
                if self.moved_vars.contains(&id.sym) {
                    panic!("CP Compile Error: Use of moved value '{}'", id.sym);
                }
                if self.loop_borrowed_containers.contains(&id.sym) {
                    panic!(
                        "CP Compile Error: Cannot perform mutable operation ({}) on container '{}' while it is borrowed in a 'for' loop",
                        op_desc, id.sym
                    );
                }
                if let Some((sources, is_mut)) = self.var_borrow_sources.get(&id.sym) {
                    if !*is_mut {
                        panic!(
                            "CP Compile Error: Cannot perform mutable operation ({}) through read-only borrow '{}' (use '*...' for mutable borrow)",
                            op_desc, id.sym
                        );
                    }
                    self.check_borrow_conflict(&id.sym, true);
                    for (src, src_f) in sources {
                        if self.loop_borrowed_containers.contains(src) {
                            panic!(
                                "CP Compile Error: Cannot perform mutable operation ({}) on '{}' while '{}' is borrowed in a 'for' loop",
                                op_desc, id.sym, src
                            );
                        }
                        self.check_borrow_conflict_field_excluding(
                            src,
                            src_f.as_deref(),
                            &id.sym,
                            true,
                        );
                    }
                } else {
                    if self.current_fn_params.contains(&id.sym)
                        && !self.mutable_fn_params.contains(&id.sym)
                    {
                        if op_desc == "index assignment '[] ='" {
                            if self.is_lib_mode {
                                return;
                            }
                            if let Some(param_ty) = self.var_cp_types.get(&id.sym) {
                                if matches!(param_ty, Type::Array(_) | Type::FixedArray(_, _)) {
                                    return;
                                }
                            }
                        }
                        let is_ptr_or_mut_borrow =
                            if let Some(param_ty) = self.var_cp_types.get(&id.sym) {
                                matches!(
                                    param_ty,
                                    Type::RawPtr { .. }
                                        | Type::ManyRawPtr(_)
                                        | Type::CPtr(_)
                                        | Type::SentinelPtr(_, _)
                                        | Type::BorrowType { is_mut: true, .. }
                                        | Type::Auto
                                )
                            } else {
                                false
                            };
                        if !is_ptr_or_mut_borrow {
                            panic!(
                                "CP Compile Error: Cannot perform mutable operation ({}) on read-only parameter '{}' (mark parameter with '*{}' or 'mut {}')",
                                op_desc, id.sym, id.sym, id.sym
                            );
                        }
                    }
                    self.check_borrow_conflict(&id.sym, true);
                }
            }
            Expr::Member(mem) => {
                if let Some(m_ty) = self.infer_cp_type(target_expr) {
                    if matches!(
                        m_ty,
                        Type::BorrowType { is_mut: true, .. } | Type::RawPtr { .. }
                    ) {
                        return;
                    }
                    if matches!(m_ty, Type::BorrowType { is_mut: false, .. })
                        && !op_desc.starts_with("field assignment")
                    {
                        panic!(
                            "CP Compile Error: Cannot perform mutable operation ({}) through read-only borrow field '{}'",
                            op_desc, mem.prop.sym
                        );
                    }
                }
                let (root_opt, field_opt) = self.extract_member_path(target_expr);
                if let Some(root) = root_opt {
                    if self.loop_borrowed_containers.contains(&root) {
                        panic!(
                            "CP Compile Error: Cannot perform mutable operation ({}) on container '{}' while it is borrowed in a 'for' loop",
                            op_desc, root
                        );
                    }
                    if let Some((sources, is_mut)) = self.var_borrow_sources.get(&root) {
                        if !*is_mut {
                            panic!(
                                "CP Compile Error: Cannot perform mutable operation ({}) through read-only borrow '{}'",
                                op_desc, root
                            );
                        }
                        self.check_borrow_conflict_field(&root, field_opt.as_deref(), true);
                        for (src, src_f) in sources {
                            let combined_field = match (src_f, &field_opt) {
                                (Some(sf), Some(f)) => Some(format!("{}.{}", sf, f)),
                                (Some(sf), None) => Some(sf.clone()),
                                (None, Some(f)) => Some(f.clone()),
                                (None, None) => None,
                            };
                            self.check_borrow_conflict_field_excluding(
                                src,
                                combined_field.as_deref(),
                                &root,
                                true,
                            );
                        }
                    } else {
                        if self.current_fn_params.contains(&root)
                            && !self.mutable_fn_params.contains(&root)
                        {
                            if op_desc == "index assignment '[] ='" {
                                if self.is_lib_mode {
                                    return;
                                }
                                if let Some(param_ty) = self.var_cp_types.get(&root) {
                                    if matches!(param_ty, Type::Array(_) | Type::FixedArray(_, _)) {
                                        return;
                                    }
                                }
                            }
                            let is_ptr_or_mut_borrow =
                                if let Some(param_ty) = self.var_cp_types.get(&root) {
                                    matches!(
                                        param_ty,
                                        Type::RawPtr { .. } | Type::BorrowType { is_mut: true, .. }
                                    )
                                } else {
                                    false
                                };
                            if !is_ptr_or_mut_borrow {
                                panic!(
                                    "CP Compile Error: Cannot perform mutable operation ({}) on read-only parameter '{}' (mark parameter with '*{}' or 'mut {}')",
                                    op_desc, root, root, root
                                );
                            }
                        }
                        self.check_borrow_conflict_field(&root, field_opt.as_deref(), true);
                    }
                } else {
                    self.check_mutation_allowed(&mem.obj, op_desc);
                }
            }
            Expr::Deref(inner) | Expr::Unwrap(inner) => {
                self.check_mutation_allowed(inner, op_desc);
            }
            Expr::Index { obj, .. } => {
                self.check_mutation_allowed(obj, op_desc);
            }
            _ => {}
        }
    }

    pub(crate) fn check_read_allowed(&self, target_expr: &Expr) {
        if let Expr::Ident(id) = target_expr {
            if self.moved_vars.contains(&id.sym) {
                panic!("CP Compile Error: Use of moved value '{}'", id.sym);
            }
            if !self.var_borrow_sources.contains_key(&id.sym) {
                self.check_borrow_conflict(&id.sym, false);
            }
        } else if let Expr::Member(_) = target_expr {
            let (root_opt, field_opt) = self.extract_member_path(target_expr);
            if let Some(root) = root_opt {
                if self.moved_vars.contains(&root) {
                    panic!("CP Compile Error: Use of moved value '{}'", root);
                }
                if !self.var_borrow_sources.contains_key(&root) {
                    self.check_borrow_conflict_field(&root, field_opt.as_deref(), false);
                }
            }
        }
    }

    pub(crate) fn extract_expr_borrow_info(
        &self,
        expr: &Expr,
    ) -> (Vec<(String, Option<String>)>, bool, bool) {
        // Returns: (source_vars, is_mut_borrow, is_ref_struct)
        match expr {
            Expr::Downcast { obj, is_mut, .. } => {
                if let Expr::Ident(id) = &**obj {
                    self.check_borrow_conflict(&id.sym, *is_mut);
                    let mut sources = vec![(id.sym.clone(), None)];
                    if let Some((orig, _)) = self.var_borrow_sources.get(&id.sym) {
                        if !orig.is_empty() {
                            sources = orig.clone();
                        }
                    }
                    return (sources, *is_mut, false);
                }
                (vec![], *is_mut, false)
            }
            Expr::Question(inner)
            | Expr::Unwrap(inner)
            | Expr::QuestionFallback { expr: inner, .. }
            | Expr::NullCoalesce { left: inner, .. }
            | Expr::Catch { expr: inner, .. } => self.extract_expr_borrow_info(inner),
            Expr::Borrow(inner, is_mut) => {
                match &**inner {
                    Expr::Ident(id) => {
                        if self.local_trampolines.contains(&id.sym) {
                            return (vec![], false, false);
                        }
                        if id.sym == "this" || id.sym == "_" || self.is_class_field(&id.sym) {
                            if *is_mut && self.current_receiver_mode == ReceiverMode::ReadOnly {
                                panic!(
                                    "CP Compile Error: Cannot take mutable borrow '*' of '{}' in a read-only method (add '*_' as first parameter)",
                                    id.sym
                                );
                            }
                            if self.is_class_field(&id.sym) {
                                self.check_borrow_conflict_field("this", Some(&id.sym), *is_mut);
                                return (
                                    vec![
                                        ("this".to_string(), Some(id.sym.clone())),
                                        ("_".to_string(), Some(id.sym.clone())),
                                    ],
                                    *is_mut,
                                    false,
                                );
                            }
                            self.check_borrow_conflict("this", *is_mut);
                            return (
                                vec![("this".to_string(), None), ("_".to_string(), None)],
                                *is_mut,
                                false,
                            );
                        }
                        self.check_borrow_conflict(&id.sym, *is_mut);
                        let mut sources = vec![(id.sym.clone(), None)];
                        if let Some((orig, _)) = self.var_borrow_sources.get(&id.sym) {
                            if !orig.is_empty() {
                                sources = orig.clone();
                            }
                        }
                        return (sources, *is_mut, false);
                    }
                    Expr::Member(_) => {
                        let (root_opt, field_opt) = self.extract_member_path(inner);
                        if let Some(root) = root_opt {
                            if root == "this" || root == "_" {
                                if *is_mut && self.current_receiver_mode == ReceiverMode::ReadOnly {
                                    panic!(
                                        "CP Compile Error: Cannot take mutable borrow '*{}.{}' in a read-only method (add '*_' as first parameter)",
                                        root, field_opt.as_deref().unwrap_or("")
                                    );
                                }
                                self.check_borrow_conflict_field(
                                    "this",
                                    field_opt.as_deref(),
                                    *is_mut,
                                );
                                return (
                                    vec![
                                        ("this".to_string(), field_opt.clone()),
                                        ("_".to_string(), field_opt),
                                    ],
                                    *is_mut,
                                    false,
                                );
                            }
                            let is_obj_ptr =
                                self.locals.get(&root).map_or(false, |t| {
                                    t.starts_with('*')
                                        || t.starts_with("?*")
                                        || t.contains("=> *")
                                        || t.contains("=> ?*")
                                }) || self.var_cp_types.get(&root).map_or(false, |t| {
                                    matches!(
                                        t,
                                        Type::RawPtr { .. }
                                            | Type::BorrowType { .. }
                                            | Type::CPtr(_)
                                            | Type::ManyRawPtr(_)
                                    )
                                });

                            self.check_borrow_conflict_field(&root, field_opt.as_deref(), *is_mut);
                            let resolved =
                                self.resolve_borrow_source_field(&root, field_opt.as_deref());
                            if is_obj_ptr {
                                if let Some((orig, _)) = self.var_borrow_sources.get(&root) {
                                    if !orig.is_empty() {
                                        return (resolved, *is_mut, false);
                                    }
                                }
                                if self.current_fn_params.contains(&root) {
                                    return (resolved, *is_mut, false);
                                }
                                return (vec![], *is_mut, false);
                            }
                            return (resolved, *is_mut, false);
                        }
                    }
                    _ => {}
                }
                (vec![], *is_mut, false)
            }
            Expr::Ident(id) => {
                let is_ref_st = self.var_is_ref_struct.contains(&id.sym);
                if let Some((sources, is_m)) = self.var_borrow_sources.get(&id.sym) {
                    return (sources.clone(), *is_m, is_ref_st);
                }
                if self.current_fn_params.contains(&id.sym) {
                    let is_m = self.mutable_fn_params.contains(&id.sym)
                        || matches!(
                            self.var_cp_types.get(&id.sym),
                            Some(
                                Type::BorrowType { is_mut: true, .. }
                                    | Type::RawPtr { .. }
                                    | Type::ManyRawPtr(_)
                                    | Type::CPtr(_)
                                    | Type::SentinelPtr(_, _)
                            )
                        );
                    return (vec![(id.sym.clone(), None)], is_m, is_ref_st);
                }
                (vec![], false, is_ref_st)
            }
            Expr::New(call) => {
                if let Expr::Ident(cid) = &*call.callee {
                    if self.ref_classes.contains(&cid.sym) {
                        let mut sources = Vec::new();
                        let mut any_mut = false;
                        for arg in &call.args {
                            match arg {
                                Expr::Borrow(inner, is_m) => {
                                    let (root_opt, field_opt) = self.extract_member_path(inner);
                                    if let Some(root) = root_opt {
                                        self.check_borrow_conflict_field(
                                            &root,
                                            field_opt.as_deref(),
                                            *is_m,
                                        );
                                        let resolved = self.resolve_borrow_source_field(
                                            &root,
                                            field_opt.as_deref(),
                                        );
                                        sources.extend(resolved);
                                    }
                                    if *is_m {
                                        any_mut = true;
                                    }
                                }
                                Expr::Ident(aid) => {
                                    if let Some((orig, is_m)) =
                                        self.var_borrow_sources.get(&aid.sym)
                                    {
                                        sources.extend(orig.clone());
                                        if *is_m {
                                            any_mut = true;
                                        }
                                    } else {
                                        sources.push((aid.sym.clone(), None));
                                    }
                                }
                                _ => {}
                            }
                        }
                        return (sources, any_mut, true);
                    }
                }
                (vec![], false, false)
            }
            Expr::Call(call) => {
                if let Expr::Member(mem) = &*call.callee {
                    if let Expr::Ident(obj_id) = &*mem.obj {
                        if let Some(type_name) = self.resolve_var_type_name(&obj_id.sym) {
                            if let Some((params, Some(ret_ty), recv_mode)) =
                                self.lookup_method(&type_name, &mem.prop.sym)
                            {
                                let non_recv_params: Vec<&Param> = params
                                    .iter()
                                    .filter(|p| {
                                        p.ident.sym != "this"
                                            && p.ident.sym != "_"
                                            && p.ident.sym != "self"
                                    })
                                    .collect();
                                match ret_ty {
                                    Type::BorrowType {
                                        is_mut,
                                        lifetimes,
                                        inner,
                                        ..
                                    } => {
                                        let is_slice_of_primitive = !is_mut
                                            && match &*inner {
                                                Type::Array(elem) => matches!(
                                                    &**elem,
                                                    Type::U8
                                                        | Type::I8
                                                        | Type::U16
                                                        | Type::I16
                                                        | Type::U32
                                                        | Type::I32
                                                        | Type::U64
                                                        | Type::I64
                                                        | Type::F32
                                                        | Type::F64
                                                        | Type::Bool
                                                        | Type::String
                                                ),
                                                Type::U8
                                                | Type::I8
                                                | Type::U16
                                                | Type::I16
                                                | Type::U32
                                                | Type::I32
                                                | Type::U64
                                                | Type::I64
                                                | Type::F32
                                                | Type::F64
                                                | Type::Bool
                                                | Type::String => true,
                                                _ => false,
                                            };
                                        let mut sources = Vec::new();
                                        // Tier 1: Method Return Lifetime Elision (default binds to receiver `_`/`this`)
                                        if !is_slice_of_primitive
                                            && (lifetimes.is_empty()
                                                || lifetimes
                                                    .iter()
                                                    .any(|l| l == "this" || l == "_"))
                                        {
                                            let field = self.find_method_borrowed_field(
                                                &type_name,
                                                &mem.prop.sym,
                                            );
                                            if obj_id.sym == "this" || obj_id.sym == "_" {
                                                sources.push(("this".to_string(), field.clone()));
                                                sources.push(("_".to_string(), field));
                                            } else if let Some((orig, _)) =
                                                self.var_borrow_sources.get(&obj_id.sym)
                                            {
                                                for (src_root, src_f) in orig {
                                                    let comb_f = match (src_f, &field) {
                                                        (Some(sf), Some(f)) => {
                                                            Some(format!("{}.{}", sf, f))
                                                        }
                                                        (Some(sf), None) => Some(sf.clone()),
                                                        (None, Some(f)) => Some(f.clone()),
                                                        (None, None) => None,
                                                    };
                                                    sources.push((src_root.clone(), comb_f));
                                                }
                                            } else {
                                                sources.push((obj_id.sym.clone(), field));
                                            }
                                        }
                                        // Tier 3: Explicit parameter binding on method (e.g. `: &other Tensor`)
                                        for lt_name in &lifetimes {
                                            if let Some(pos) = non_recv_params
                                                .iter()
                                                .position(|p| &p.ident.sym == lt_name)
                                            {
                                                if pos < call.args.len() {
                                                    if let Expr::Ident(arg_id) = &call.args[pos] {
                                                        if let Some((orig, _)) =
                                                            self.var_borrow_sources.get(&arg_id.sym)
                                                        {
                                                            sources.extend(orig.clone());
                                                        } else {
                                                            sources
                                                                .push((arg_id.sym.clone(), None));
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        return (sources, is_mut, false);
                                    }
                                    Type::Ref(cid) if self.ref_classes.contains(&cid.sym) => {
                                        let mut sources = Vec::new();
                                        if obj_id.sym == "this" || obj_id.sym == "_" {
                                            sources.push(("this".to_string(), None));
                                            sources.push(("_".to_string(), None));
                                        } else if let Some((orig, _)) =
                                            self.var_borrow_sources.get(&obj_id.sym)
                                        {
                                            sources.extend(orig.clone());
                                        } else {
                                            sources.push((obj_id.sym.clone(), None));
                                        }
                                        return (sources, recv_mode == ReceiverMode::Mut, true);
                                    }
                                    _ => {}
                                }
                            }
                        }
                    }
                } else if let Expr::Ident(fid) = &*call.callee {
                    if fid.sym == "@fieldParentPtr"
                        || fid.sym == "@alignCast"
                        || fid.sym == "@ptrCast"
                    {
                        if let Some(last_arg) = call.args.last() {
                            return self.extract_expr_borrow_info(last_arg);
                        }
                    } else if fid.sym == "@as" {
                        if let Some(val_arg) = call.args.get(1) {
                            return self.extract_expr_borrow_info(val_arg);
                        }
                    }
                    // Check if `fid` is an omitted-receiver method call on current_class_name
                    if let Some(c_name) = &self.current_class_name {
                        if !self.locals.contains_key(&fid.sym) {
                            if let Some((_, Some(ret_ty), recv_mode)) =
                                self.lookup_method(c_name, &fid.sym)
                            {
                                match ret_ty {
                                    Type::BorrowType { is_mut, .. } => {
                                        let field =
                                            self.find_method_borrowed_field(c_name, &fid.sym);
                                        return (
                                            vec![
                                                ("this".to_string(), field.clone()),
                                                ("_".to_string(), field),
                                            ],
                                            is_mut,
                                            false,
                                        );
                                    }
                                    Type::Ref(cid) if self.ref_classes.contains(&cid.sym) => {
                                        return (
                                            vec![
                                                ("this".to_string(), None),
                                                ("_".to_string(), None),
                                            ],
                                            recv_mode == ReceiverMode::Mut,
                                            true,
                                        );
                                    }
                                    _ => {}
                                }
                            }
                        }
                    }
                    if let Some(sig) = self.fn_sigs.get(&fid.sym).cloned() {
                        // Also check param modifiers (`mut` / `move`) at call site
                        for (idx, param) in sig.params.iter().enumerate() {
                            if idx < call.args.len() {
                                if let Expr::Ident(arg_id) = &call.args[idx] {
                                    if param.is_mut {
                                        self.check_borrow_conflict(&arg_id.sym, true);
                                    } else if !param.is_move {
                                        self.check_borrow_conflict(&arg_id.sym, false);
                                    }
                                }
                            }
                        }
                        if let Some(ret_ty) = &sig.return_type {
                            match ret_ty {
                                Type::BorrowType {
                                    is_mut, lifetimes, ..
                                } => {
                                    let mut sources = Vec::new();
                                    if !lifetimes.is_empty() {
                                        // Tier 3: Explicit parameter name binding (&x T or &(x | y) T)
                                        for lt_name in lifetimes {
                                            if let Some(pos) = sig
                                                .params
                                                .iter()
                                                .position(|p| &p.ident.sym == lt_name)
                                            {
                                                if pos < call.args.len() {
                                                    if let Expr::Ident(arg_id) = &call.args[pos] {
                                                        if let Some((orig, _)) =
                                                            self.var_borrow_sources.get(&arg_id.sym)
                                                        {
                                                            sources.extend(orig.clone());
                                                        } else {
                                                            sources
                                                                .push((arg_id.sym.clone(), None));
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    } else {
                                        // Tier 1 & Tier 2: Automatic Elision (single ref param) or Default Union (all object/ref params)
                                        for (pos, param) in sig.params.iter().enumerate() {
                                            let is_obj_param = matches!(
                                                param.ty,
                                                Type::Ref(_)
                                                    | Type::BorrowType { .. }
                                                    | Type::String
                                                    | Type::Array(_)
                                            );
                                            if is_obj_param && pos < call.args.len() {
                                                if let Expr::Ident(arg_id) = &call.args[pos] {
                                                    if let Some((orig, _)) =
                                                        self.var_borrow_sources.get(&arg_id.sym)
                                                    {
                                                        sources.extend(orig.clone());
                                                    } else {
                                                        sources.push((arg_id.sym.clone(), None));
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    return (sources, *is_mut, false);
                                }
                                Type::Ref(cid) if self.ref_classes.contains(&cid.sym) => {
                                    let mut sources = Vec::new();
                                    for (pos, _) in sig.params.iter().enumerate() {
                                        if pos < call.args.len() {
                                            if let Expr::Ident(arg_id) = &call.args[pos] {
                                                if let Some((orig, _)) =
                                                    self.var_borrow_sources.get(&arg_id.sym)
                                                {
                                                    sources.extend(orig.clone());
                                                } else {
                                                    sources.push((arg_id.sym.clone(), None));
                                                }
                                            }
                                        }
                                    }
                                    return (sources, false, true);
                                }
                                _ => {}
                            }
                        }
                    }
                }
                (vec![], false, false)
            }
            _ => (vec![], false, false),
        }
    }
}
