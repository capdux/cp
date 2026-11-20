use super::*;
use crate::ast::*;
use crate::diagnostic::{find_closest_match, Diagnostic, STANDARD_BUILTIN_IMPORTS};
use crate::Span;
use std::collections::HashMap;

impl Analyzer {
    pub(super) fn register_meta_def(&mut self, m: &MetaDef) -> SemaResult<()> {
        let mut allowed_targets = std::collections::HashSet::new();
        let mut is_once = false;

        self.validate_meta_items(&m.meta, "meta")?;

        for item in &m.meta {
            if item.name.sym.to_lowercase() == "target" {
                for arg in &item.args {
                    let target_name = match arg {
                        MetaArg::Positional(Expr::Ident(id)) => id.sym.to_lowercase(),
                        MetaArg::Named(_, Expr::Ident(id)) => id.sym.to_lowercase(),
                        _ => continue,
                    };
                    allowed_targets.insert(target_name);
                }
            } else if item.name.sym.to_lowercase() == "once" {
                is_once = true;
            }
        }

        if allowed_targets.is_empty() {
            allowed_targets.insert("class".to_string());
            allowed_targets.insert("struct".to_string());
            allowed_targets.insert("function".to_string());
            allowed_targets.insert("method".to_string());
            allowed_targets.insert("field".to_string());
            allowed_targets.insert("param".to_string());
        }

        self.meta_defs.insert(
            m.ident.sym.clone(),
            MetaDefInfo {
                allowed_targets,
                is_once,
            },
        );
        Ok(())
    }

    pub(super) fn extract_zig_code_symbols(&mut self, stmt: &Stmt) {
        match stmt {
            Stmt::Expr(Expr::Zig(z)) => self.register_zig_block_symbols(&z.code),
            Stmt::Block(b) | Stmt::UnsafeBlock(b) => {
                for s in &b.body {
                    self.extract_zig_code_symbols(s);
                }
            }
            _ => {}
            _ => {}
        }
    }

    pub(super) fn register_zig_block_symbols(&mut self, code: &str) {
        for line in code.lines() {
            let trimmed = line.trim();
            let stripped = if let Some(s) = trimmed.strip_prefix("pub ") {
                s.trim_start()
            } else {
                trimmed
            };
            let (ident_opt, is_var) = if let Some(rest) = stripped.strip_prefix("const ") {
                (rest.split(&[' ', ':', '='][..]).next(), false)
            } else if let Some(rest) = stripped.strip_prefix("var ") {
                (rest.split(&[' ', ':', '='][..]).next(), true)
            } else if let Some(rest) = stripped.strip_prefix("fn ") {
                (rest.split(&[' ', '(', '<'][..]).next(), false)
            } else if let Some(rest) = stripped.strip_prefix("extern fn ") {
                (rest.split(&[' ', '(', '<'][..]).next(), false)
            } else if let Some(rest) = stripped.strip_prefix("extern \"c\" fn ") {
                (rest.split(&[' ', '(', '<'][..]).next(), false)
            } else {
                (None, false)
            };
            if let Some(ident) = ident_opt {
                let id = ident.trim();
                if !id.is_empty() && id.chars().all(|c| c.is_alphanumeric() || c == '_') {
                    self.scopes[0].insert(
                        id.to_string(),
                        SymbolKind::Var {
                            ty: Type::Any,
                            kind: if is_var {
                                VarDeclKind::Let
                            } else {
                                VarDeclKind::Const
                            },
                        },
                    );
                }
            }
        }
    }

    pub(super) fn validate_meta_items(&self, items: &[MetaItem], target: &str) -> SemaResult<()> {
        let mut counts: HashMap<String, usize> = HashMap::new();
        for item in items {
            *counts.entry(item.name.sym.clone()).or_insert(0) += 1;

            if item.name.sym == "pre"
                || item.name.sym == "ret"
                || item.name.sym == "arena"
                || item.name.sym == "json"
                || item.name.sym == "json_ignore"
            {
                continue;
            }

            for arg in &item.args {
                let expr = match arg {
                    MetaArg::Positional(e) => e,
                    MetaArg::Named(_, e) => e,
                };
                if !Self::is_const_expr(expr) {
                    return Err(SemaError::MetaArgNotConstant(
                        item.name.sym.clone(),
                        item.span,
                    ));
                }
            }

            if self.macro_defs.contains_key(&item.name.sym) {
                let matches_target = target == "class" || target == "struct";
                if !matches_target {
                    return Err(SemaError::MacroInvalidTarget(
                        item.name.sym.clone(),
                        "class, struct".to_string(),
                        target.to_string(),
                        item.span,
                    ));
                }
            } else if self.dec_defs.contains_key(&item.name.sym) {
                let matches_target = target == "method" || target == "function";
                if !matches_target {
                    return Err(SemaError::DecInvalidTarget(
                        item.name.sym.clone(),
                        "method, function".to_string(),
                        target.to_string(),
                        item.span,
                    ));
                }
            } else if let Some(def) = self.meta_defs.get(&item.name.sym) {
                let matches_target = def.allowed_targets.contains(target)
                    || (target == "struct" && def.allowed_targets.contains("class"))
                    || (target == "class" && def.allowed_targets.contains("struct"))
                    || (target == "method" && def.allowed_targets.contains("function"))
                    || (target == "function" && def.allowed_targets.contains("method"));
                if !matches_target {
                    let mut allowed: Vec<String> = def.allowed_targets.iter().cloned().collect();
                    allowed.sort();
                    return Err(SemaError::MetaInvalidTarget(
                        item.name.sym.clone(),
                        allowed.join(", "),
                        target.to_string(),
                        item.span,
                    ));
                }
            }
        }

        for (name, count) in counts {
            if count > 1 {
                if let Some(def) = self.meta_defs.get(&name) {
                    if def.is_once {
                        let span = items
                            .iter()
                            .find(|i| i.name.sym == name)
                            .map(|i| i.span)
                            .unwrap_or(Span { start: 0, end: 0 });
                        return Err(SemaError::MetaDuplicateOnce(name, span));
                    }
                }
            }
        }
        Ok(())
    }

    pub(super) fn analyze_decl(&mut self, decl: &Decl) -> SemaResult<()> {
        match decl {
            Decl::Meta(m) => {
                self.validate_meta_items(&m.meta, "meta")?;
            }
            Decl::Dec(d) => {
                self.validate_meta_items(&d.meta, "dec")?;
                self.enter_scope();
                for p in &d.params {
                    let p_kind = SymbolKind::Var {
                        ty: p.ty.clone(),
                        kind: VarDeclKind::Let,
                    };
                    self.define(p.ident.sym.clone(), p_kind.clone(), p.ident.span)?;
                    self.symbol_map.insert(p.ident.span, p_kind);
                }
                self.analyze_block(&d.body)?;
                self.exit_scope();
            }
            Decl::Macro(m) => {
                self.validate_meta_items(&m.meta, "macro")?;
                self.enter_scope();
                for p in &m.params {
                    let p_kind = SymbolKind::Var {
                        ty: p.ty.clone(),
                        kind: VarDeclKind::Let,
                    };
                    self.define(p.ident.sym.clone(), p_kind.clone(), p.ident.span)?;
                    self.symbol_map.insert(p.ident.span, p_kind);
                }
                self.analyze_block(&m.body)?;
                self.exit_scope();
            }
            Decl::Func(f) => {
                self.validate_meta_items(&f.meta, "function")?;
                for p in &f.params {
                    self.validate_meta_items(&p.meta, "param")?;
                }
                let params = f.params.iter().map(|p| p.ty.clone()).collect();
                let kind = SymbolKind::Func {
                    params,
                    ret: f.return_type.clone(),
                    is_async: f.is_async,
                    is_unsafe: f.is_unsafe,
                };
                self.define(f.ident.sym.clone(), kind.clone(), f.ident.span)?;
                self.symbol_map.insert(f.ident.span, kind);

                let mut added_generic_bounds = Vec::new();
                for gp in &f.generic_params {
                    if let GenericParam::Type { ident, bounds } = gp {
                        self.active_generic_bounds.insert(
                            ident.sym.clone(),
                            bounds.iter().map(|b| b.sym.clone()).collect(),
                        );
                        added_generic_bounds.push(ident.sym.clone());
                    }
                }

                let old_async = self.current_fn_is_async;
                let old_unsafe = self.current_fn_is_unsafe;
                self.current_fn_is_async = f.is_async;
                self.current_fn_is_unsafe = f.is_unsafe;
                self.enter_scope();
                for gp in &f.generic_params {
                    match gp {
                        GenericParam::Type { ident: id, .. } => {
                            let _ = self.define(
                                id.sym.clone(),
                                SymbolKind::Var {
                                    ty: Type::Ref(id.clone()),
                                    kind: VarDeclKind::Const,
                                },
                                id.span,
                            );
                        }
                        GenericParam::Const { ident, ty } => {
                            let _ = self.define(
                                ident.sym.clone(),
                                SymbolKind::Var {
                                    ty: ty.clone(),
                                    kind: VarDeclKind::Const,
                                },
                                ident.span,
                            );
                        }
                    }
                }
                let mut ro_params = std::collections::HashSet::new();
                for p in &f.params {
                    if !p.ident.sym.starts_with("@\"")
                        && RESERVED_WORDS.contains(&p.ident.sym.as_str())
                    {
                        self.errors.push(SemaError::ReservedIdentifier(
                            p.ident.sym.clone(),
                            p.ident.span,
                            false,
                        ));
                    }
                    if !p.is_mut
                        && !matches!(p.ty, Type::BorrowType { is_mut: true, .. })
                        && p.ident.sym != "_"
                        && p.ident.sym != "this"
                        && p.ident.sym != "self"
                    {
                        ro_params.insert(p.ident.sym.clone());
                    }
                    let p_kind = SymbolKind::Var {
                        ty: p.ty.clone(),
                        kind: VarDeclKind::Let,
                    };
                    self.define(p.ident.sym.clone(), p_kind.clone(), p.ident.span)?;
                    self.symbol_map.insert(p.ident.span, p_kind);
                }
                self.fn_param_vars.push(ro_params);
                for stmt in &f.body.body {
                    if let Err(e) = self.analyze_stmt(stmt) {
                        self.errors.push(e);
                    }
                }
                self.fn_param_vars.pop();
                self.exit_scope();
                for k in added_generic_bounds {
                    self.active_generic_bounds.remove(&k);
                }
                self.current_fn_is_async = old_async;
                self.current_fn_is_unsafe = old_unsafe;
            }
            Decl::Class(c) => {
                let target = if c.is_ref { "struct" } else { "class" };
                self.validate_meta_items(&c.meta, target)?;
                for f in &c.fields {
                    self.validate_meta_items(&f.meta, "field")?;
                }
                for m in &c.methods {
                    self.validate_meta_items(&m.meta, "method")?;
                    for p in &m.params {
                        self.validate_meta_items(&p.meta, "param")?;
                    }
                }
                // Verify that all declared 'implements' traits have their required methods implemented
                for imp in &c.implements {
                    if let Some(tr) = self.traits.get(&imp.sym) {
                        for tm in &tr.methods {
                            if tm.default_body.is_none() {
                                let has_m = c.methods.iter().any(|m| m.ident.sym == tm.ident.sym)
                                    || self.impls.iter().any(|i| {
                                        i.trait_name.sym == imp.sym
                                            && i.target_name.sym == c.ident.sym
                                            && i.methods.iter().any(|m| m.ident.sym == tm.ident.sym)
                                    });
                                if !has_m {
                                    return Err(SemaError::TraitMissingMethod {
                                        class_name: c.ident.sym.clone(),
                                        trait_name: imp.sym.clone(),
                                        missing_method: tm.ident.sym.clone(),
                                        span: c.ident.span,
                                    });
                                }
                            }
                        }
                    }
                }

                let mut added_class_bounds = Vec::new();
                for gp in &c.generic_params {
                    if let GenericParam::Type { ident, bounds } = gp {
                        self.active_generic_bounds.insert(
                            ident.sym.clone(),
                            bounds.iter().map(|b| b.sym.clone()).collect(),
                        );
                        added_class_bounds.push(ident.sym.clone());
                    }
                }

                let old_class = self.current_class.clone();
                self.current_class = Some(c.ident.sym.clone());
                for code in &c.extra_zig {
                    self.register_zig_block_symbols(code);
                }
                for nc in &c.nested_classes {
                    let _ = self.define(
                        nc.ident.sym.clone(),
                        SymbolKind::Class(nc.clone()),
                        nc.ident.span,
                    );
                }
                for m in &c.methods {
                    let mut added_method_bounds = Vec::new();
                    for gp in &m.generic_params {
                        if let GenericParam::Type { ident, bounds } = gp {
                            self.active_generic_bounds.insert(
                                ident.sym.clone(),
                                bounds.iter().map(|b| b.sym.clone()).collect(),
                            );
                            added_method_bounds.push(ident.sym.clone());
                        }
                    }

                    let old_async = self.current_fn_is_async;
                    let old_unsafe = self.current_fn_is_unsafe;
                    self.current_fn_is_async = m.is_async;
                    self.current_fn_is_unsafe = m.is_unsafe;
                    self.enter_scope();
                    for gp in &c.generic_params {
                        match gp {
                            GenericParam::Type { ident: id, .. } => {
                                let _ = self.define(
                                    id.sym.clone(),
                                    SymbolKind::Var {
                                        ty: Type::Ref(id.clone()),
                                        kind: VarDeclKind::Const,
                                    },
                                    id.span,
                                );
                            }
                            GenericParam::Const { ident, ty } => {
                                let _ = self.define(
                                    ident.sym.clone(),
                                    SymbolKind::Var {
                                        ty: ty.clone(),
                                        kind: VarDeclKind::Const,
                                    },
                                    ident.span,
                                );
                            }
                        }
                    }
                    for gp in &m.generic_params {
                        match gp {
                            GenericParam::Type { ident: id, .. } => {
                                let _ = self.define(
                                    id.sym.clone(),
                                    SymbolKind::Var {
                                        ty: Type::Ref(id.clone()),
                                        kind: VarDeclKind::Const,
                                    },
                                    id.span,
                                );
                            }
                            GenericParam::Const { ident, ty } => {
                                let _ = self.define(
                                    ident.sym.clone(),
                                    SymbolKind::Var {
                                        ty: ty.clone(),
                                        kind: VarDeclKind::Const,
                                    },
                                    ident.span,
                                );
                            }
                        }
                    }
                    // Define receiver aliases `this` and `_`
                    let self_ty = Type::Ref(c.ident.clone());
                    let _ = self.define(
                        "this".to_string(),
                        SymbolKind::Var {
                            ty: self_ty.clone(),
                            kind: VarDeclKind::Let,
                        },
                        c.ident.span,
                    );
                    let _ = self.define(
                        "_".to_string(),
                        SymbolKind::Var {
                            ty: self_ty.clone(),
                            kind: VarDeclKind::Let,
                        },
                        c.ident.span,
                    );
                    // Define fields & methods from parent hierarchy and current class for omitted `_.` calls
                    let mut cur_opt = Some(c.clone());
                    while let Some(cur_c) = cur_opt {
                        for f in &cur_c.fields {
                            let _ = self.define(
                                f.ident.sym.clone(),
                                SymbolKind::Var {
                                    ty: f.ty.clone(),
                                    kind: VarDeclKind::Let,
                                },
                                f.ident.span,
                            );
                        }
                        for cm in &cur_c.methods {
                            let params = cm
                                .params
                                .iter()
                                .filter(|p| {
                                    p.ident.sym != "this"
                                        && p.ident.sym != "_"
                                        && p.ident.sym != "self"
                                })
                                .map(|p| p.ty.clone())
                                .collect();
                            let _ = self.define(
                                cm.ident.sym.clone(),
                                SymbolKind::Func {
                                    params,
                                    ret: cm.return_type.clone(),
                                    is_async: cm.is_async,
                                    is_unsafe: cm.is_unsafe,
                                },
                                cm.ident.span,
                            );
                        }
                        cur_opt = cur_c
                            .extends
                            .as_ref()
                            .and_then(|p| self.classes.get(&p.sym).cloned());
                    }
                    // Also define methods from implemented traits
                    for tr_id in &c.implements {
                        if let Some(SymbolKind::Trait(tr_decl)) = self.resolve(&tr_id.sym).cloned()
                        {
                            for tm in &tr_decl.methods {
                                let params = tm
                                    .params
                                    .iter()
                                    .filter(|p| {
                                        p.ident.sym != "this"
                                            && p.ident.sym != "_"
                                            && p.ident.sym != "self"
                                    })
                                    .map(|p| p.ty.clone())
                                    .collect();
                                let _ = self.define(
                                    tm.ident.sym.clone(),
                                    SymbolKind::Func {
                                        params,
                                        ret: tm.return_type.clone(),
                                        is_async: false,
                                        is_unsafe: tm.is_unsafe,
                                    },
                                    tm.ident.span,
                                );
                            }
                        }
                    }
                    let is_static = m.is_static;
                    let is_mut = !is_static
                        && (m.ident.sym == "init"
                            || m.ident.sym == "release"
                            || m.ident.sym == "__super_init"
                            || m.ident.sym == "__super_release"
                            || m.params.iter().any(|p| {
                                (p.ident.sym == "_" || p.ident.sym == "this")
                                    && (p.is_mut
                                        || matches!(p.ty, Type::BorrowType { is_mut: true, .. }))
                            }));
                    let old_method_mut = self.current_method_is_mut;
                    self.current_method_is_mut = is_mut;

                    let mut ro_params = std::collections::HashSet::new();
                    for p in &m.params {
                        if !p.ident.sym.starts_with("@\"")
                            && RESERVED_WORDS.contains(&p.ident.sym.as_str())
                        {
                            self.errors.push(SemaError::ReservedIdentifier(
                                p.ident.sym.clone(),
                                p.ident.span,
                                false,
                            ));
                        }
                        if !p.is_mut
                            && !matches!(p.ty, Type::BorrowType { is_mut: true, .. })
                            && p.ident.sym != "_"
                            && p.ident.sym != "this"
                            && p.ident.sym != "self"
                        {
                            ro_params.insert(p.ident.sym.clone());
                        }
                        if p.ident.sym == "this" || p.ident.sym == "_" {
                            let p_kind = SymbolKind::Var {
                                ty: p.ty.clone(),
                                kind: VarDeclKind::Let,
                            };
                            self.define(p.ident.sym.clone(), p_kind.clone(), p.ident.span)?;
                            self.symbol_map.insert(p.ident.span, p_kind);
                        }
                    }
                    self.fn_param_vars.push(ro_params);
                    self.enter_scope();
                    for p in &m.params {
                        if p.ident.sym != "this" && p.ident.sym != "_" {
                            let p_kind = SymbolKind::Var {
                                ty: p.ty.clone(),
                                kind: VarDeclKind::Let,
                            };
                            self.define(p.ident.sym.clone(), p_kind.clone(), p.ident.span)?;
                            self.symbol_map.insert(p.ident.span, p_kind);
                        }
                    }
                    for stmt in &m.body.body {
                        if let Err(e) = self.analyze_stmt(stmt) {
                            self.errors.push(e);
                        }
                    }
                    self.fn_param_vars.pop();
                    self.current_method_is_mut = old_method_mut;
                    self.exit_scope();
                    self.exit_scope();
                    for k in added_method_bounds {
                        self.active_generic_bounds.remove(&k);
                    }
                    self.current_fn_is_async = old_async;
                    self.current_fn_is_unsafe = old_unsafe;
                }
                for k in added_class_bounds {
                    self.active_generic_bounds.remove(&k);
                }
                self.current_class = old_class;
            }
            Decl::Trait(t) => {
                self.validate_meta_items(&t.meta, "trait")?;
            }
            Decl::Enum(e) => {
                self.validate_meta_items(&e.meta, "enum")?;
                for m in &e.methods {
                    self.validate_meta_items(&m.meta, "method")?;
                    let old_async = self.current_fn_is_async;
                    let old_unsafe = self.current_fn_is_unsafe;
                    self.current_fn_is_async = m.is_async;
                    self.current_fn_is_unsafe = m.is_unsafe;
                    self.enter_scope();
                    let self_ty = Type::Ref(e.ident.clone());
                    let _ = self.define(
                        "this".to_string(),
                        SymbolKind::Var {
                            ty: self_ty.clone(),
                            kind: VarDeclKind::Let,
                        },
                        e.ident.span,
                    );
                    let _ = self.define(
                        "_".to_string(),
                        SymbolKind::Var {
                            ty: self_ty.clone(),
                            kind: VarDeclKind::Let,
                        },
                        e.ident.span,
                    );
                    let _ = self.define(
                        "self".to_string(),
                        SymbolKind::Var {
                            ty: self_ty.clone(),
                            kind: VarDeclKind::Let,
                        },
                        e.ident.span,
                    );
                    for p in &m.params {
                        if p.ident.sym != "this" && p.ident.sym != "_" && p.ident.sym != "self" {
                            let _ = self.define(
                                p.ident.sym.clone(),
                                SymbolKind::Var {
                                    ty: p.ty.clone(),
                                    kind: VarDeclKind::Let,
                                },
                                p.ident.span,
                            );
                        }
                    }
                    self.analyze_block(&m.body)?;
                    self.exit_scope();
                    self.current_fn_is_async = old_async;
                    self.current_fn_is_unsafe = old_unsafe;
                }
            }
            Decl::TypeAlias(ta) => {
                self.type_aliases
                    .insert(ta.ident.sym.clone(), ta.target_ty.clone());
            }
            Decl::Fuzz(f) => {
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
            _ => {}
        }
        Ok(())
    }

    pub(super) fn bind_match_pattern(&mut self, pat: &Pattern) -> SemaResult<()> {
        match pat {
            Pattern::Ident(id) => {
                if let Some(SymbolKind::Var { .. }) = self.scopes.last().unwrap().get(&id.sym) {
                    return Err(SemaError::DuplicateVarDecl(id.sym.clone(), id.span));
                }
                self.define(
                    id.sym.clone(),
                    SymbolKind::Var {
                        ty: Type::Any,
                        kind: VarDeclKind::Let,
                    },
                    id.span,
                )?;
            }
            Pattern::Err(inner) => self.bind_match_pattern(inner)?,
            Pattern::Enum(ep) => {
                if let Some(tpats) = &ep.tuple_pats {
                    for tp in tpats {
                        self.bind_match_pattern(tp)?;
                    }
                }
                if let Some(props) = &ep.fields {
                    for p in props {
                        if let Some(vp) = &p.val {
                            self.bind_match_pattern(vp)?;
                        } else {
                            if let Some(SymbolKind::Var { .. }) =
                                self.scopes.last().unwrap().get(&p.key.sym)
                            {
                                return Err(SemaError::DuplicateVarDecl(
                                    p.key.sym.clone(),
                                    p.key.span,
                                ));
                            }
                            self.define(
                                p.key.sym.clone(),
                                SymbolKind::Var {
                                    ty: Type::Any,
                                    kind: VarDeclKind::Let,
                                },
                                p.key.span,
                            )?;
                        }
                    }
                }
            }
            Pattern::Tuple(els) | Pattern::Array(els) => {
                if els.iter().filter(|p| matches!(p, Pattern::Rest(_))).count() > 1 {
                    return Err(SemaError::MultipleRestInPattern(Span { start: 0, end: 0 }));
                }
                for el in els {
                    self.bind_match_pattern(el)?;
                }
            }
            Pattern::Rest(inner) => self.bind_match_pattern(inner)?,
            Pattern::Or(els) => {
                let base_scope = self.scopes.last().unwrap().clone();
                let mut merged = base_scope.clone();
                for el in els {
                    *self.scopes.last_mut().unwrap() = base_scope.clone();
                    self.bind_match_pattern(el)?;
                    for (k, v) in self.scopes.last().unwrap() {
                        merged.insert(k.clone(), v.clone());
                    }
                }
                *self.scopes.last_mut().unwrap() = merged;
            }
            Pattern::Object(props) => {
                let rest_count = props.iter().filter(|p| p.is_rest).count();
                if rest_count > 1 {
                    return Err(SemaError::MultipleRestInPattern(Span { start: 0, end: 0 }));
                }
                for p in props {
                    if p.key.sym == "*" {
                        continue;
                    }
                    if let Some(vp) = &p.val {
                        self.bind_match_pattern(vp)?;
                    } else {
                        if let Some(SymbolKind::Var { .. }) =
                            self.scopes.last().unwrap().get(&p.key.sym)
                        {
                            return Err(SemaError::DuplicateVarDecl(p.key.sym.clone(), p.key.span));
                        }
                        self.define(
                            p.key.sym.clone(),
                            SymbolKind::Var {
                                ty: Type::Any,
                                kind: VarDeclKind::Let,
                            },
                            p.key.span,
                        )?;
                    }
                }
            }
            Pattern::Binding { ident, subpat } => {
                if let Some(SymbolKind::Var { .. }) = self.scopes.last().unwrap().get(&ident.sym) {
                    return Err(SemaError::DuplicateVarDecl(ident.sym.clone(), ident.span));
                }
                self.define(
                    ident.sym.clone(),
                    SymbolKind::Var {
                        ty: Type::Any,
                        kind: VarDeclKind::Let,
                    },
                    ident.span,
                )?;
                self.bind_match_pattern(subpat)?;
            }
            Pattern::TypeMatch { ident, ty } => {
                if let Some(SymbolKind::Var { .. }) = self.scopes.last().unwrap().get(&ident.sym) {
                    return Err(SemaError::DuplicateVarDecl(ident.sym.clone(), ident.span));
                }
                self.define(
                    ident.sym.clone(),
                    SymbolKind::Var {
                        ty: ty.clone(),
                        kind: VarDeclKind::Let,
                    },
                    ident.span,
                )?;
            }
            _ => {}
            _ => {}
        }
        Ok(())
    }

    pub(super) fn bind_for_pattern(&mut self, pat: &Pattern, elem_ty: &Type) -> SemaResult<()> {
        match pat {
            Pattern::Ident(id) => {
                if let Some(SymbolKind::Var { .. }) = self.scopes.last().unwrap().get(&id.sym) {
                    return Err(SemaError::DuplicateVarDecl(id.sym.clone(), id.span));
                }
                self.define(
                    id.sym.clone(),
                    SymbolKind::Var {
                        ty: elem_ty.clone(),
                        kind: VarDeclKind::Let,
                    },
                    id.span,
                )?;
            }
            Pattern::Tuple(pats) | Pattern::Array(pats) => {
                if pats
                    .iter()
                    .filter(|p| matches!(p, Pattern::Rest(_)))
                    .count()
                    > 1
                {
                    return Err(SemaError::MultipleRestInPattern(Span { start: 0, end: 0 }));
                }
                for p in pats {
                    self.bind_for_pattern(p, &Type::Any)?;
                }
            }
            Pattern::Rest(inner) => {
                self.bind_for_pattern(inner, &Type::Array(Box::new(Type::Any)))?;
            }
            Pattern::Object(props) => {
                let rest_count = props.iter().filter(|p| p.is_rest).count();
                if rest_count > 1 {
                    return Err(SemaError::MultipleRestInPattern(Span { start: 0, end: 0 }));
                }
                for p in props {
                    if p.key.sym == "*" {
                        continue;
                    }
                    if let Some(vp) = &p.val {
                        self.bind_for_pattern(vp, &Type::Any)?;
                    } else {
                        if let Some(SymbolKind::Var { .. }) =
                            self.scopes.last().unwrap().get(&p.key.sym)
                        {
                            return Err(SemaError::DuplicateVarDecl(p.key.sym.clone(), p.key.span));
                        }
                        self.define(
                            p.key.sym.clone(),
                            SymbolKind::Var {
                                ty: Type::Any,
                                kind: VarDeclKind::Let,
                            },
                            p.key.span,
                        )?;
                    }
                }
            }
            _ => self.bind_match_pattern(pat)?,
        }
        Ok(())
    }
}
