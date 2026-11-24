use crate::ast::*;
use std::collections::HashSet;

pub fn extract_words_from_str(s: &str, vars: &mut HashSet<String>) {
    let mut cur = String::new();
    for ch in s.chars() {
        if ch.is_ascii_alphanumeric() || ch == '_' {
            cur.push(ch);
        } else {
            if !cur.is_empty() {
                if !cur.chars().next().unwrap().is_ascii_digit() {
                    vars.insert(std::mem::take(&mut cur));
                } else {
                    cur.clear();
                }
            }
        }
    }
    if !cur.is_empty() && !cur.chars().next().unwrap().is_ascii_digit() {
        vars.insert(cur);
    }
}

pub fn find_referenced_vars_in_type(ty: &Type, vars: &mut HashSet<String>) {
    match ty {
        Type::Ref(id) | Type::BoxDyn(id) | Type::BareDyn(id) => {
            extract_words_from_str(&id.sym, vars);
        }
        Type::Array(inner)
        | Type::Optional(inner)
        | Type::ErrorUnion(inner)
        | Type::Set(inner)
        | Type::Shared(inner)
        | Type::SentinelPtr(inner, _)
        | Type::SentinelSlice(inner, _) => find_referenced_vars_in_type(inner, vars),
        Type::FixedArray(inner, size) => {
            find_referenced_vars_in_type(inner, vars);
            match size {
                ArraySize::Ident(id) => {
                    extract_words_from_str(&id.sym, vars);
                }
                ArraySize::Expr(s) => {
                    extract_words_from_str(s, vars);
                }
                _ => {}
            }
        }
        Type::BorrowType { inner, .. }
        | Type::RawPtr { inner, .. }
        | Type::ManyRawPtr(inner)
        | Type::CPtr(inner)
        | Type::Aligned { inner, .. } => find_referenced_vars_in_type(inner, vars),
        Type::ResultType { ok, err } => {
            find_referenced_vars_in_type(ok, vars);
            find_referenced_vars_in_error_type(err, vars);
        }
        Type::Map(k, v) | Type::Dict(k, v) => {
            find_referenced_vars_in_type(k, vars);
            find_referenced_vars_in_type(v, vars);
        }
        Type::Tuple(elems) => {
            for e in elems {
                find_referenced_vars_in_type(e, vars);
            }
        }
        Type::Object(fields) => {
            for f in fields {
                find_referenced_vars_in_type(&f.ty, vars);
            }
        }
        Type::Fn(params, ret) | Type::CFn(params, ret) => {
            for p in params {
                find_referenced_vars_in_type(p, vars);
            }
            find_referenced_vars_in_type(ret, vars);
        }
        Type::Conditional(cond, cons, alt) => {
            find_referenced_vars_in_expr(cond, vars);
            find_referenced_vars_in_type(cons, vars);
            find_referenced_vars_in_type(alt, vars);
        }
        _ => {}
    }
}

pub fn find_referenced_vars_in_expr(expr: &Expr, vars: &mut HashSet<String>) {
    match expr {
        Expr::Ident(i) => {
            vars.insert(i.sym.clone());
            if !i.sym.chars().all(|c| c.is_ascii_alphanumeric() || c == '_') {
                extract_words_from_str(&i.sym, vars);
            }
        }
        Expr::Bin(b) => {
            if b.op == BinaryOp::Assign {
                if let Expr::Ident(id) = &*b.left {
                    if id.sym == "_" {
                        find_referenced_vars_in_expr(&b.right, vars);
                        return;
                    }
                }
            }
            find_referenced_vars_in_expr(&b.left, vars);
            find_referenced_vars_in_expr(&b.right, vars);
        }
        Expr::Call(c) => {
            find_referenced_vars_in_expr(&c.callee, vars);
            for a in &c.args {
                find_referenced_vars_in_expr(a, vars);
            }
        }
        Expr::New(c) => {
            find_referenced_vars_in_expr(&c.callee, vars);
            for a in &c.args {
                find_referenced_vars_in_expr(a, vars);
            }
        }
        Expr::Member(m) | Expr::OptMember(m) => {
            find_referenced_vars_in_expr(&m.obj, vars);
        } // prop is just name
        Expr::OptCall(c) => {
            find_referenced_vars_in_expr(&c.callee, vars);
            for a in &c.args {
                find_referenced_vars_in_expr(a, vars);
            }
        }
        Expr::Slice(s) => {
            find_referenced_vars_in_expr(&s.obj, vars);
            if let Some(st) = &s.start {
                find_referenced_vars_in_expr(st, vars);
            }
            if let Some(e) = &s.end {
                find_referenced_vars_in_expr(e, vars);
            }
        }
        Expr::Match(m) => {
            find_referenced_vars_in_expr(&m.disc, vars);
            for arm in &m.arms {
                find_referenced_vars_in_pattern(&arm.pat, vars);
                if let Some(g) = &arm.guard {
                    find_referenced_vars_in_expr(g, vars);
                }
                find_referenced_vars_in_stmt(&arm.body, vars);
            }
        }
        Expr::Receive(Some(ty)) => {
            find_referenced_vars_in_type(ty, vars);
        }
        Expr::Spawn(s) => {
            find_referenced_vars_in_expr(&s.callee, vars);
            for a in &s.args {
                find_referenced_vars_in_expr(a, vars);
            }
        }
        Expr::Question(q) | Expr::ErrWrap(q) | Expr::BoxNew(q) => {
            find_referenced_vars_in_expr(q, vars);
        }
        Expr::QuestionFallback { expr, fallback, .. } => {
            find_referenced_vars_in_expr(expr, vars);
            find_referenced_vars_in_expr(fallback, vars);
        }
        Expr::NullCoalesce { left, right } => {
            find_referenced_vars_in_expr(left, vars);
            find_referenced_vars_in_expr(right, vars);
        }
        Expr::Catch { expr, body, .. } => {
            find_referenced_vars_in_expr(expr, vars);
            find_referenced_vars_in_stmt(body, vars);
        }
        Expr::Downcast { obj, target_ty, .. } => {
            find_referenced_vars_in_expr(obj, vars);
            extract_words_from_str(&target_ty.sym, vars);
        }
        Expr::Array(arr) => {
            for a in arr {
                find_referenced_vars_in_expr(a, vars);
            }
        }
        Expr::ArrayRepeat(val, count) => {
            find_referenced_vars_in_expr(val, vars);
            find_referenced_vars_in_expr(count, vars);
        }
        Expr::Object(obj) | Expr::Dict(obj) => {
            for f in obj {
                find_referenced_vars_in_expr(&f.val, vars);
            }
        }
        Expr::Index { obj, index } => {
            find_referenced_vars_in_expr(obj, vars);
            find_referenced_vars_in_expr(index, vars);
        }
        Expr::EnumInit(e) => {
            extract_words_from_str(&e.enum_name.sym, vars);
            if let Some(f) = &e.fields {
                for a in f {
                    find_referenced_vars_in_expr(&a.val, vars);
                }
            }
            if let Some(args) = &e.tuple_args {
                for a in args {
                    find_referenced_vars_in_expr(a, vars);
                }
            }
        }
        Expr::Template(t) => {
            for e in &t.expressions {
                find_referenced_vars_in_expr(e, vars);
            }
        }
        Expr::Await(a) => {
            find_referenced_vars_in_expr(a, vars);
        }
        Expr::Move(m) => {
            find_referenced_vars_in_expr(m, vars);
        }
        Expr::Borrow(b, _) => {
            find_referenced_vars_in_expr(b, vars);
        }
        Expr::Builtin(b) => {
            for a in &b.args {
                find_referenced_vars_in_expr(a, vars);
            }
        }
        Expr::Arrow(a) => match &*a.body {
            ArrowBody::Expr(e) => find_referenced_vars_in_expr(e, vars),
            ArrowBody::Block(b) => find_referenced_vars_in_block(b, vars),
        },
        Expr::ReflectType { target_ty, .. } | Expr::TypeVal { ty: target_ty, .. } => {
            find_referenced_vars_in_type(target_ty, vars);
        }
        Expr::ReflectOf { expr, .. } => {
            find_referenced_vars_in_expr(expr, vars);
        }
        Expr::Range { start, end, .. } => {
            find_referenced_vars_in_expr(start, vars);
            find_referenced_vars_in_expr(end, vars);
        }
        Expr::Cast {
            expr, target_ty, ..
        } => {
            find_referenced_vars_in_expr(expr, vars);
            find_referenced_vars_in_type(target_ty, vars);
        }
        Expr::Zig(z) => {
            extract_words_from_str(&z.code, vars);
        }
        Expr::Deref(d)
        | Expr::Unwrap(d)
        | Expr::BitNot(d)
        | Expr::Neg(d)
        | Expr::ComptimeExpr(d) => find_referenced_vars_in_expr(d, vars),
        Expr::Unsafe(b) | Expr::Comptime(b) | Expr::Arena(b) => {
            find_referenced_vars_in_block(b, vars)
        }
        Expr::If {
            test, cons, alt, ..
        } => {
            find_referenced_vars_in_expr(test, vars);
            find_referenced_vars_in_expr(cons, vars);
            if let Some(a) = alt {
                find_referenced_vars_in_expr(a, vars);
            }
        }
        Expr::Continue(_) => {}
        Expr::Break(_, val) | Expr::Return(_, val) | Expr::LabeledBreak(_, _, val) => {
            if let Some(v) = val {
                find_referenced_vars_in_expr(v, vars);
            }
        }
        Expr::LabeledBlock { block, .. } => find_referenced_vars_in_block(block, vars),
        Expr::While { stmt, else_expr } => {
            find_referenced_vars_in_expr(&stmt.test, vars);
            if let Some(c) = &stmt.continue_expr {
                find_referenced_vars_in_expr(c, vars);
            }
            find_referenced_vars_in_stmt(&stmt.body, vars);
            if let Some(e) = else_expr {
                find_referenced_vars_in_expr(e, vars);
            }
        }
        Expr::InlineAsm { .. } => {}
        Expr::ComptimeMap {
            target_ty, entries, ..
        } => {
            if let Some(ty) = target_ty {
                find_referenced_vars_in_type(ty, vars);
            }
            for f in entries {
                find_referenced_vars_in_expr(&f.val, vars);
            }
        }
        _ => {}
    }
}

pub fn find_referenced_vars_in_stmt(stmt: &Stmt, vars: &mut HashSet<String>) {
    match stmt {
        Stmt::Expr(e) => find_referenced_vars_in_expr(e, vars),
        Stmt::Var(v) => {
            if let Some(ty) = &v.ty {
                find_referenced_vars_in_type(ty, vars);
            }
            if let Some(init) = &v.init {
                find_referenced_vars_in_expr(init, vars);
            }
        }
        Stmt::If(i) => {
            find_referenced_vars_in_expr(&i.test, vars);
            find_referenced_vars_in_stmt(&i.cons, vars);
            if let Some(alt) = &i.alt {
                find_referenced_vars_in_stmt(alt, vars);
            }
        }
        Stmt::While(w) => {
            find_referenced_vars_in_expr(&w.test, vars);
            if let Some(c) = &w.continue_expr {
                find_referenced_vars_in_expr(c, vars);
            }
            find_referenced_vars_in_stmt(&w.body, vars);
        }
        Stmt::For(f) => {
            find_referenced_vars_in_pattern(&f.pat, vars);
            find_referenced_vars_in_expr(&f.expr, vars);
            if let Some(raw) = &f.raw_header {
                extract_words_from_str(raw, vars);
            }
            find_referenced_vars_in_stmt(&f.body, vars);
        }
        Stmt::Return(r) => {
            if let Some(arg) = &r.arg {
                find_referenced_vars_in_expr(arg, vars);
            }
        }
        Stmt::Break | Stmt::Continue => {}
        Stmt::Match(m) => {
            find_referenced_vars_in_expr(&m.disc, vars);
            for arm in &m.arms {
                find_referenced_vars_in_pattern(&arm.pat, vars);
                if let Some(g) = &arm.guard {
                    find_referenced_vars_in_expr(g, vars);
                }
                find_referenced_vars_in_stmt(&arm.body, vars);
            }
        }
        Stmt::Spawn(s) => {
            find_referenced_vars_in_expr(&s.callee, vars);
            for a in &s.args {
                find_referenced_vars_in_expr(a, vars);
            }
        }
        Stmt::Defer(d) => find_referenced_vars_in_stmt(d, vars),
        Stmt::ErrDefer(d) => find_referenced_vars_in_stmt(d, vars),
        Stmt::ComptimeBlock(b) => find_referenced_vars_in_block(b, vars),
        Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
            find_referenced_vars_in_block(b, vars)
        }
        Stmt::TryCatch {
            try_block,
            catch_body,
            ..
        } => {
            find_referenced_vars_in_block(try_block, vars);
            if let Some(cb) = catch_body {
                find_referenced_vars_in_stmt(cb, vars);
            }
        }
        Stmt::Func(f) => {
            find_referenced_vars_in_func(f, vars);
        }
        Stmt::Fuzz(f) => {
            if let Some(ty) = &f.param_type {
                find_referenced_vars_in_type(ty, vars);
            }
            find_referenced_vars_in_block(&f.body, vars);
        }
    }
}

pub fn find_referenced_vars_in_error_type(err: &ErrorType, vars: &mut HashSet<String>) {
    match err {
        ErrorType::Named(id) | ErrorType::BoxDyn(id) => {
            extract_words_from_str(&id.sym, vars);
        }
        ErrorType::Union(ids) | ErrorType::AnonSet(ids) => {
            for id in ids {
                extract_words_from_str(&id.sym, vars);
            }
        }
        ErrorType::DefaultError | ErrorType::AnyErr => {}
    }
}

pub fn find_referenced_vars_in_pattern(pat: &Pattern, vars: &mut HashSet<String>) {
    match pat {
        Pattern::Array(pats) | Pattern::Tuple(pats) | Pattern::Or(pats) => {
            for p in pats {
                find_referenced_vars_in_pattern(p, vars);
            }
        }
        Pattern::Object(props) => {
            for p in props {
                if let Some(v) = &p.val {
                    find_referenced_vars_in_pattern(v, vars);
                }
            }
        }
        Pattern::Enum(ep) => {
            extract_words_from_str(&ep.enum_name.sym, vars);
            if let Some(fields) = &ep.fields {
                for f in fields {
                    if let Some(v) = &f.val {
                        find_referenced_vars_in_pattern(v, vars);
                    }
                }
            }
            if let Some(tpats) = &ep.tuple_pats {
                for p in tpats {
                    find_referenced_vars_in_pattern(p, vars);
                }
            }
        }
        Pattern::Rest(inner) | Pattern::Err(inner) => {
            find_referenced_vars_in_pattern(inner, vars);
        }
        Pattern::Range { start, end, .. } => {
            for b in [start, end] {
                match b {
                    RangeBound::Ident(id) | RangeBound::NegIdent(id) => {
                        extract_words_from_str(&id.sym, vars);
                    }
                    RangeBound::Int(_) => {}
                }
            }
        }
        Pattern::Binding { subpat, .. } => {
            find_referenced_vars_in_pattern(subpat, vars);
        }
        Pattern::TypeMatch { ty, .. } | Pattern::Type(ty) => {
            find_referenced_vars_in_type(ty, vars);
        }
        Pattern::Expr(e) => {
            find_referenced_vars_in_expr(e, vars);
        }
        Pattern::Ident(_) | Pattern::Lit(_) | Pattern::Wildcard | Pattern::Null => {}
    }
}

fn find_referenced_vars_in_meta_items(meta: &[MetaItem], vars: &mut HashSet<String>) {
    for m in meta {
        extract_words_from_str(&m.name.sym, vars);
        for arg in &m.args {
            match arg {
                MetaArg::Positional(e) | MetaArg::Named(_, e) => {
                    find_referenced_vars_in_expr(e, vars);
                }
            }
        }
    }
}

fn find_referenced_vars_in_generic_params(params: &[GenericParam], vars: &mut HashSet<String>) {
    for gp in params {
        match gp {
            GenericParam::Type { bounds, .. } => {
                for b in bounds {
                    extract_words_from_str(&b.sym, vars);
                }
            }
            GenericParam::Const { ty, .. } => {
                find_referenced_vars_in_type(ty, vars);
            }
        }
    }
}

fn find_referenced_vars_in_func(f: &Function, vars: &mut HashSet<String>) {
    find_referenced_vars_in_meta_items(&f.meta, vars);
    find_referenced_vars_in_generic_params(&f.generic_params, vars);
    for p in &f.params {
        find_referenced_vars_in_type(&p.ty, vars);
    }
    if let Some(ret) = &f.return_type {
        find_referenced_vars_in_type(ret, vars);
    }
    find_referenced_vars_in_block(&f.body, vars);
}

fn find_referenced_vars_in_class(c: &Class, vars: &mut HashSet<String>) {
    find_referenced_vars_in_meta_items(&c.meta, vars);
    find_referenced_vars_in_generic_params(&c.generic_params, vars);
    for imp in &c.implements {
        extract_words_from_str(&imp.sym, vars);
    }
    if let Some(ext) = &c.extends {
        extract_words_from_str(&ext.sym, vars);
    }
    for f in &c.fields {
        find_referenced_vars_in_type(&f.ty, vars);
        if let Some(def) = &f.default {
            find_referenced_vars_in_expr(def, vars);
        }
    }
    for m in &c.methods {
        find_referenced_vars_in_func(m, vars);
    }
    for z in &c.extra_zig {
        extract_words_from_str(z, vars);
    }
    for nested in &c.nested_classes {
        find_referenced_vars_in_class(nested, vars);
    }
    for tb in &c.test_blocks {
        find_referenced_vars_in_block(&tb.body, vars);
    }
}

fn find_referenced_vars_in_decl(decl: &Decl, vars: &mut HashSet<String>) {
    match decl {
        Decl::Func(f) => find_referenced_vars_in_func(f, vars),
        Decl::Class(c) => find_referenced_vars_in_class(c, vars),
        Decl::Trait(t) => {
            find_referenced_vars_in_meta_items(&t.meta, vars);
            for m in &t.methods {
                for p in &m.params {
                    find_referenced_vars_in_type(&p.ty, vars);
                }
                if let Some(ret) = &m.return_type {
                    find_referenced_vars_in_type(ret, vars);
                }
            }
        }
        Decl::Impl(imp) => {
            extract_words_from_str(&imp.trait_name.sym, vars);
            extract_words_from_str(&imp.target_name.sym, vars);
            for m in &imp.methods {
                find_referenced_vars_in_func(m, vars);
            }
        }
        Decl::Enum(e) => {
            find_referenced_vars_in_meta_items(&e.meta, vars);
            if let Some(tag_ty) = &e.tag_type {
                find_referenced_vars_in_type(tag_ty, vars);
            }
            for v in &e.variants {
                if let Some(fields) = &v.fields {
                    for f in fields {
                        find_referenced_vars_in_type(&f.ty, vars);
                        if let Some(def) = &f.default {
                            find_referenced_vars_in_expr(def, vars);
                        }
                    }
                }
                if let Some(val) = &v.value {
                    find_referenced_vars_in_expr(val, vars);
                }
                if let Some(tt) = &v.tuple_type {
                    find_referenced_vars_in_type(tt, vars);
                }
                if let Some(fe) = &v.from_err {
                    extract_words_from_str(&fe.sym, vars);
                }
            }
            for m in &e.methods {
                find_referenced_vars_in_func(m, vars);
            }
            for z in &e.extra_zig {
                extract_words_from_str(z, vars);
            }
            for tb in &e.test_blocks {
                find_referenced_vars_in_block(&tb.body, vars);
            }
        }
        Decl::Interface(iface) => {
            for m in &iface.methods {
                for p in &m.params {
                    find_referenced_vars_in_type(&p.ty, vars);
                }
                if let Some(ret) = &m.return_type {
                    find_referenced_vars_in_type(ret, vars);
                }
            }
        }
        Decl::ErrorSet(es) => {
            for u in &es.unions {
                extract_words_from_str(u, vars);
            }
        }
        Decl::Meta(md) => {
            find_referenced_vars_in_meta_items(&md.meta, vars);
            for p in &md.params {
                find_referenced_vars_in_type(&p.ty, vars);
                if let Some(def) = &p.default {
                    find_referenced_vars_in_expr(def, vars);
                }
            }
        }
        Decl::Dec(dd) => {
            find_referenced_vars_in_meta_items(&dd.meta, vars);
            for p in &dd.params {
                find_referenced_vars_in_type(&p.ty, vars);
            }
            if let Some(ret) = &dd.return_type {
                find_referenced_vars_in_type(ret, vars);
            }
            find_referenced_vars_in_block(&dd.body, vars);
        }
        Decl::Macro(mac) => {
            find_referenced_vars_in_meta_items(&mac.meta, vars);
            for p in &mac.params {
                find_referenced_vars_in_type(&p.ty, vars);
            }
            if let Some(ret) = &mac.return_type {
                find_referenced_vars_in_type(ret, vars);
            }
            find_referenced_vars_in_block(&mac.body, vars);
            for m in &mac.methods {
                find_referenced_vars_in_func(m, vars);
            }
        }
        Decl::TypeAlias(ta) => {
            find_referenced_vars_in_type(&ta.target_ty, vars);
        }
        Decl::CInclude(_) => {}
        Decl::Fuzz(f) => {
            if let Some(ty) = &f.param_type {
                find_referenced_vars_in_type(ty, vars);
            }
            find_referenced_vars_in_block(&f.body, vars);
        }
    }
}

/// Collect all identifiers and namespace prefixes actually referenced in non-Import items of a module.
pub fn collect_module_referenced_idents(module: &Module) -> HashSet<String> {
    let mut vars = HashSet::new();
    for item in &module.body {
        match item {
            ModuleItem::Import(_) => {}
            ModuleItem::Decl(d) => find_referenced_vars_in_decl(d, &mut vars),
            ModuleItem::Stmt(s) => find_referenced_vars_in_stmt(s, &mut vars),
            ModuleItem::Export(exp) => {
                if exp.source.is_none() {
                    for sp in &exp.specifiers {
                        vars.insert(sp.local.sym.clone());
                    }
                }
            }
        }
    }
    vars
}

pub fn find_referenced_vars_in_block(block: &BlockStmt, vars: &mut HashSet<String>) {
    for stmt in &block.body {
        find_referenced_vars_in_stmt(stmt, vars);
    }
}

pub fn get_declared_vars_in_block(block: &BlockStmt) -> HashSet<String> {
    let mut vars = HashSet::new();
    for stmt in &block.body {
        if let Stmt::Var(v) = stmt {
            if let Pattern::Ident(i) = &v.pat {
                vars.insert(i.sym.clone());
            }
        } else if let Stmt::Func(f) = stmt {
            vars.insert(f.ident.sym.clone());
        } else if let Stmt::Block(b) | Stmt::UnsafeBlock(b) = stmt {
            let inner = get_declared_vars_in_block(b);
            vars.extend(inner);
        } else if let Stmt::TryCatch {
            try_block,
            catch_body,
            ..
        } = stmt
        {
            vars.extend(get_declared_vars_in_block(try_block));
            if let Some(cb) = catch_body {
                if let Stmt::Block(b) = &**cb {
                    vars.extend(get_declared_vars_in_block(b));
                }
            }
        } else if let Stmt::If(i) = stmt {
            if let Stmt::Block(b) = &*i.cons {
                vars.extend(get_declared_vars_in_block(b));
            }
            if let Some(alt) = &i.alt {
                if let Stmt::Block(b) = &**alt {
                    vars.extend(get_declared_vars_in_block(b));
                }
            }
        } else if let Stmt::For(f) = stmt {
            extract_idents_from_pat(&f.pat, &mut vars);
            if let Stmt::Block(b) = &*f.body {
                vars.extend(get_declared_vars_in_block(b));
            }
        }
    }
    vars
}

pub fn extract_idents_from_pat(pat: &Pattern, vars: &mut HashSet<String>) {
    match pat {
        Pattern::Ident(i) => {
            vars.insert(i.sym.clone());
        }
        Pattern::Array(pats) | Pattern::Tuple(pats) | Pattern::Or(pats) => {
            for p in pats {
                extract_idents_from_pat(p, vars);
            }
        }
        Pattern::Object(props) => {
            for p in props {
                if p.key.sym != "*" {
                    if let Some(val_pat) = &p.val {
                        extract_idents_from_pat(val_pat, vars);
                    } else {
                        vars.insert(p.key.sym.clone());
                    }
                }
            }
        }
        Pattern::Rest(inner) => {
            extract_idents_from_pat(inner, vars);
        }
        Pattern::Binding { ident, subpat } => {
            vars.insert(ident.sym.clone());
            extract_idents_from_pat(subpat, vars);
        }
        Pattern::TypeMatch { ident, .. } => {
            vars.insert(ident.sym.clone());
        }
        _ => {}
    }
}
