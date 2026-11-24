use crate::ast::*;
use crate::Span;
use std::collections::{HashMap, HashSet};

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ReceiverMode {
    ReadOnly,
    Mut,
    Move,
    Static,
}

#[derive(Default, Debug)]
pub struct ModuleFeatures {
    pub needs_regex: bool,
    pub needs_collections: bool,
    pub needs_actors: bool,
    pub needs_reflection: bool,
    pub needs_unpack: bool,
    pub needs_result_error: bool,
    pub needs_opt_chaining: bool,
    pub needs_null_coalesce: bool,
    pub needs_fmt: bool,
    pub needs_math_random: bool,
    pub needs_equality: bool,
    pub needs_comparison: bool,
    pub needs_hash: bool,
    pub needs_array_helpers: bool,
    pub needs_iter_helpers: bool,
    pub needs_string_helpers: bool,
    pub needs_add: bool,
    pub needs_not: bool,
    pub needs_index_helpers: bool,
    pub needs_shift: bool,
    pub needs_cast_int: bool,
    pub needs_sync: bool,
    pub needs_json: bool,
    pub needs_simd_math: bool,
    pub needs_split_at_mut: bool,
    pub has_imported_json: bool,
    pub has_imported_ptr: bool,
    pub is_lib_mode: bool,
}

impl ModuleFeatures {
    pub fn scan(module: &Module, is_lib_mode: bool) -> Self {
        let mut f = Self {
            is_lib_mode,
            ..Self::default()
        };
        for item in &module.body {
            match item {
                ModuleItem::Decl(d) => f.scan_decl(d),
                ModuleItem::Stmt(s) => f.scan_stmt(s),
                ModuleItem::Import(imp) => {
                    if imp.source == "cap:sync"
                        || imp
                            .specifiers
                            .iter()
                            .any(|s| s.sym == "Mutex" || s.sym == "RwLock" || s.sym == "Atomic")
                    {
                        f.needs_sync = true;
                    }
                    if imp.source == "cap:json"
                        || imp.source.contains("json")
                        || imp.specifiers.iter().any(|s| {
                            s.sym == "JSON"
                                || s.sym == "JsonValue"
                                || s.sym == "JsonParser"
                                || s.sym == "JsonTag"
                        })
                    {
                        f.has_imported_json = true;
                    }
                    if imp.source == "cap:ptr"
                        || imp.source.contains("ptr")
                        || imp.specifiers.iter().any(|s| {
                            s.sym == "Shared"
                                || s.sym == "AtomicShared"
                                || s.sym == "Cow"
                                || s.sym == "CowSlice"
                                || s.sym == "Owned"
                                || s.sym == "TaggedPointer"
                        })
                    {
                        f.has_imported_ptr = true;
                    }
                }
                ModuleItem::Export(exp) => {
                    if let Some(source) = &exp.source {
                        if source == "cap:sync"
                            || exp.specifiers.iter().any(|s| {
                                s.local.sym == "Mutex"
                                    || s.local.sym == "RwLock"
                                    || s.local.sym == "Atomic"
                            })
                        {
                            f.needs_sync = true;
                        }
                        if source == "cap:json"
                            || exp
                                .specifiers
                                .iter()
                                .any(|s| s.local.sym == "JSON" || s.local.sym == "JsonValue")
                        {
                            f.has_imported_json = true;
                        }
                        if source == "cap:ptr"
                            || source.contains("ptr")
                            || exp.specifiers.iter().any(|s| {
                                s.local.sym == "Shared"
                                    || s.local.sym == "AtomicShared"
                                    || s.local.sym == "Cow"
                                    || s.local.sym == "CowSlice"
                                    || s.local.sym == "Owned"
                                    || s.local.sym == "TaggedPointer"
                            })
                        {
                            f.has_imported_ptr = true;
                        }
                    }
                }
            }
        }
        if f.needs_regex && Self::has_top_decl(module, "RegExp") {
            f.needs_regex = false;
        }
        if f.needs_collections || f.needs_actors {
            f.needs_string_helpers = true;
            f.needs_equality = true;
            f.needs_comparison = true;
            f.needs_add = true;
            f.needs_index_helpers = true;
            f.needs_iter_helpers = true;
            f.needs_fmt = true;
        }
        if f.needs_index_helpers {
            f.needs_string_helpers = true;
            f.needs_equality = true;
            f.needs_iter_helpers = true;
        }
        if f.needs_array_helpers
            || f.needs_hash
            || f.needs_fmt
            || f.needs_null_coalesce
            || f.needs_equality
            || f.needs_comparison
            || f.needs_add
            || f.needs_not
            || f.needs_result_error
        {
            f.needs_string_helpers = true;
        }
        f
    }

    fn has_top_decl(module: &Module, name: &str) -> bool {
        let escaped = format!("@\"{}\"", name);
        let zig_eq = format!("const {} =", name);
        let zig_col = format!("const {}:", name);
        module.body.iter().any(|item| match item {
            ModuleItem::Stmt(Stmt::Var(v)) => match &v.pat {
                Pattern::Ident(id) => id.sym == name || id.sym == escaped,
                _ => false,
            },
            ModuleItem::Decl(Decl::TypeAlias(ta)) => {
                ta.ident.sym == name || ta.ident.sym == escaped
            }
            ModuleItem::Decl(Decl::Class(c)) => c.ident.sym == name || c.ident.sym == escaped,
            ModuleItem::Decl(Decl::Enum(e)) => e.ident.sym == name || e.ident.sym == escaped,
            ModuleItem::Decl(Decl::Trait(t)) => t.ident.sym == name || t.ident.sym == escaped,
            ModuleItem::Decl(Decl::ErrorSet(e)) => e.ident.sym == name || e.ident.sym == escaped,
            ModuleItem::Decl(Decl::Func(f)) => f.ident.sym == name || f.ident.sym == escaped,
            ModuleItem::Import(imp) => {
                imp.default_specifier
                    .as_ref()
                    .map_or(false, |id| id.sym == name || id.sym == escaped)
                    || imp
                        .specifiers
                        .iter()
                        .any(|id| id.sym == name || id.sym == escaped)
            }
            ModuleItem::Export(exp) => exp.specifiers.iter().any(|sp| {
                (exp.source.is_some() || sp.exported.sym != sp.local.sym)
                    && (sp.exported.sym == name || sp.exported.sym == escaped)
            }),
            ModuleItem::Stmt(Stmt::Expr(Expr::Zig(z))) => {
                z.code.contains(&zig_eq) || z.code.contains(&zig_col)
            }
            _ => false,
        })
    }

    fn scan_decl(&mut self, d: &Decl) {
        match d {
            Decl::Func(func) => {
                if func.ident.sym == "Shared"
                    || func.ident.sym == "AtomicShared"
                    || func.ident.sym == "Weak"
                    || func.ident.sym == "AtomicWeak"
                    || func.ident.sym == "Cow"
                    || func.ident.sym == "CowSlice"
                    || func.ident.sym == "Owned"
                    || func.ident.sym == "TaggedPointer"
                    || func.ident.sym == "RawRefCount"
                    || func.ident.sym == "RefCount"
                    || func.ident.sym == "RefPtr"
                {
                    self.has_imported_ptr = true;
                }
                if (func.ident.sym == "main" && !self.is_lib_mode) || func.is_async {
                    self.needs_actors = true;
                }
                if let Some(ret) = &func.return_type {
                    self.scan_type(ret);
                }
                for p in &func.params {
                    self.scan_type(&p.ty);
                }
                self.scan_block(&func.body);
            }
            Decl::Class(c) => {
                if c.ident.sym == "JSON"
                    || c.ident.sym == "JsonValue"
                    || c.ident.sym == "JsonParser"
                    || c.ident.sym == "JsonField"
                {
                    self.has_imported_json = true;
                }
                if c.ident.sym == "TaggedPointer"
                    || c.ident.sym == "LowTagPointer"
                    || c.ident.sym == "Shared"
                    || c.ident.sym == "AtomicShared"
                    || c.ident.sym == "RawRefCount"
                    || c.ident.sym == "RefCount"
                {
                    self.has_imported_ptr = true;
                }
                if c.implements.iter().any(|i| i.sym == "Agent") {
                    self.needs_actors = true;
                }
                for f in &c.fields {
                    self.scan_type(&f.ty);
                    if let Some(def) = &f.default {
                        self.scan_expr(def);
                    }
                }
                for m in &c.methods {
                    if m.is_async {
                        self.needs_actors = true;
                    }
                    if let Some(ret) = &m.return_type {
                        self.scan_type(ret);
                    }
                    for p in &m.params {
                        self.scan_type(&p.ty);
                    }
                    self.scan_block(&m.body);
                }
                for nc in &c.nested_classes {
                    self.scan_decl(&Decl::Class(nc.clone()));
                }
                for tb in &c.test_blocks {
                    self.scan_block(&tb.body);
                }
                for code in &c.extra_zig {
                    self.scan_extra_zig(code);
                }
            }
            Decl::Trait(t) => {
                for m in &t.methods {
                    if let Some(ret) = &m.return_type {
                        self.scan_type(ret);
                    }
                    for p in &m.params {
                        self.scan_type(&p.ty);
                    }
                }
            }
            Decl::Enum(e) => {
                if e.ident.sym == "JsonTag" {
                    self.has_imported_json = true;
                }
                for v in &e.variants {
                    if let Some(val) = &v.value {
                        self.scan_expr(val);
                    }
                    if let Some(tuple_ty) = &v.tuple_type {
                        self.scan_type(tuple_ty);
                    }
                    if let Some(fields) = &v.fields {
                        for f in fields {
                            self.scan_type(&f.ty);
                            if let Some(def) = &f.default {
                                self.scan_expr(def);
                            }
                        }
                    }
                }
                for m in &e.methods {
                    if let Some(ret) = &m.return_type {
                        self.scan_type(ret);
                    }
                    for p in &m.params {
                        self.scan_type(&p.ty);
                    }
                    self.scan_block(&m.body);
                }
                for tb in &e.test_blocks {
                    self.scan_block(&tb.body);
                }
                for code in &e.extra_zig {
                    self.scan_extra_zig(code);
                }
            }
            Decl::Impl(i) => {
                for m in &i.methods {
                    if let Some(ret) = &m.return_type {
                        self.scan_type(ret);
                    }
                    for p in &m.params {
                        self.scan_type(&p.ty);
                    }
                    self.scan_block(&m.body);
                }
            }
            Decl::TypeAlias(ta) => {
                if ta.ident.sym == "JSON" || ta.ident.sym == "JsonValue" {
                    self.has_imported_json = true;
                }
                self.scan_type(&ta.target_ty);
            }
            _ => {}
        }
    }

    fn scan_extra_zig(&mut self, code: &str) {
        if code.contains("<<") || code.contains(">>") {
            self.needs_shift = true;
        }
        let mut parser = crate::parser::Parser::new(code);
        let (module, diags) = parser.parse_module_with_recovery();
        let has_syntax_break = diags
            .iter()
            .any(|d| matches!(d.code.as_str(), "E0100" | "E0111" | "E0199"));
        if !has_syntax_break && module.body.len() == 1 {
            match &module.body[0] {
                ModuleItem::Decl(d) => self.scan_decl(d),
                ModuleItem::Stmt(s) => self.scan_stmt(s),
                _ => {}
            }
        }
    }

    fn scan_stmt(&mut self, s: &Stmt) {
        match s {
            Stmt::Var(v) => {
                if let Some(t) = &v.ty {
                    self.scan_type(t);
                }
                if let Some(init) = &v.init {
                    self.scan_expr(init);
                }
                self.scan_pat(&v.pat);
            }
            Stmt::Expr(e) => self.scan_expr(e),
            Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => self.scan_block(b),
            Stmt::If(i) => {
                self.scan_expr(&i.test);
                self.scan_stmt(&i.cons);
                if let Some(alt) = &i.alt {
                    self.scan_stmt(alt);
                }
            }
            Stmt::While(w) => {
                self.scan_expr(&w.test);
                self.scan_stmt(&w.body);
            }
            Stmt::For(for_stmt) => {
                if !matches!(for_stmt.expr, Expr::Range { .. }) {
                    let is_native_slice = match &for_stmt.expr {
                        Expr::Member(m) => m.prop.sym == "items",
                        Expr::Call(c) => match &*c.callee {
                            Expr::Member(m) => m.prop.sym == "values",
                            _ => false,
                        },
                        _ => false,
                    };
                    if !is_native_slice {
                        self.needs_iter_helpers = true;
                    }
                }
                self.scan_expr(&for_stmt.expr);
                self.scan_stmt(&for_stmt.body);
            }
            Stmt::Match(m) => {
                self.needs_result_error = true;
                self.needs_string_helpers = true;
                self.needs_equality = true;
                self.scan_expr(&m.disc);
                for arm in &m.arms {
                    self.scan_pat(&arm.pat);
                    if let Some(g) = &arm.guard {
                        self.scan_expr(g);
                    }
                    self.scan_stmt(&arm.body);
                }
            }
            Stmt::TryCatch {
                try_block,
                catch_body,
                ..
            } => {
                self.needs_result_error = true;
                self.scan_block(try_block);
                if let Some(cb) = catch_body {
                    self.scan_stmt(cb);
                }
            }
            Stmt::Return(r) => {
                if let Some(a) = &r.arg {
                    self.scan_expr(a);
                }
            }
            _ => {}
        }
    }

    fn scan_block(&mut self, b: &BlockStmt) {
        for s in &b.body {
            self.scan_stmt(s);
        }
    }

    fn scan_pat(&mut self, p: &Pattern) {
        match p {
            Pattern::Rest(_) => self.needs_unpack = true,
            Pattern::Tuple(items) => {
                self.needs_unpack = true;
                for item in items {
                    self.scan_pat(item);
                }
            }
            Pattern::Enum(ep) => {
                self.needs_result_error = true;
                if ep.tuple_pats.is_some() {
                    self.needs_unpack = true;
                }
                if let Some(tp) = &ep.tuple_pats {
                    for item in tp {
                        self.scan_pat(item);
                    }
                }
            }
            Pattern::Err(inner) => self.scan_pat(inner),
            Pattern::Or(pats) => {
                for item in pats {
                    self.scan_pat(item);
                }
            }
            Pattern::Binding { subpat, .. } => self.scan_pat(subpat),
            Pattern::Expr(expr) => {
                self.needs_equality = true;
                self.scan_expr(expr);
            }
            Pattern::Lit(_) => self.needs_equality = true,
            _ => {}
        }
    }

    fn scan_type(&mut self, t: &Type) {
        match t {
            Type::ResultType { .. } => self.needs_result_error = true,
            Type::ErrorUnion(_) => {}
            Type::Dict(_, _) | Type::Map(_, _) | Type::Set(_) => self.needs_collections = true,
            Type::PID => self.needs_actors = true,
            Type::Conditional(cond, cons, alt) => {
                self.scan_expr(cond);
                self.scan_type(cons);
                self.scan_type(alt);
            }
            Type::Ref(id) => {
                if id.sym == "RegExp" {
                    self.needs_regex = true;
                } else if id.sym == "Mutex"
                    || id.sym == "RwLock"
                    || id.sym == "Atomic"
                    || id.sym.starts_with("Mutex<")
                    || id.sym.starts_with("RwLock<")
                    || id.sym.starts_with("Atomic<")
                    || id.sym.starts_with("LockGuard")
                    || id.sym.starts_with("ReadGuard")
                    || id.sym.starts_with("WriteGuard")
                    || id.sym == "CpBareMutex"
                    || id.sym == "CpBareRwLock"
                {
                    self.needs_sync = true;
                } else if !self.is_lib_mode
                    && (id.sym == "Task"
                        || id.sym == "Message"
                        || id.sym == "PID"
                        || id.sym == "Agent")
                {
                    self.needs_actors = true;
                } else if id.sym == "Error" || id.sym == "CpResult" {
                    self.needs_result_error = true;
                } else if id.sym == "JsonValue" || id.sym == "JsonField" || id.sym == "JsonTag" {
                    self.needs_json = true;
                }
            }
            Type::Optional(inner) => self.scan_type(inner),
            Type::Array(inner)
            | Type::FixedArray(inner, _)
            | Type::ManyRawPtr(inner)
            | Type::CPtr(inner)
            | Type::SentinelPtr(inner, _)
            | Type::SentinelSlice(inner, _) => self.scan_type(inner),
            Type::Tuple(ts) => {
                for item in ts {
                    self.scan_type(item);
                }
            }
            Type::Fn(params, ret) => {
                for p in params {
                    self.scan_type(p);
                }
                self.scan_type(ret);
            }
            _ => {}
        }
    }

    fn scan_expr(&mut self, e: &Expr) {
        match e {
            Expr::Ident(id) => {
                if id.sym == "RegExp" {
                    self.needs_regex = true;
                } else if id.sym == "JSON" || id.sym == "JsonValue" || id.sym == "JsonParser" {
                    self.needs_json = true;
                } else if id.sym == "Mutex" || id.sym == "RwLock" || id.sym == "Atomic" {
                    self.needs_sync = true;
                } else if !self.is_lib_mode
                    && (id.sym == "Task"
                        || id.sym == "Message"
                        || id.sym == "PID"
                        || id.sym == "Agent")
                {
                    self.needs_actors = true;
                } else if id.sym == "reflect"
                    || id.sym == "TypeInfo"
                    || id.sym == "FieldInfo"
                    || id.sym == "MethodInfo"
                {
                    self.needs_reflection = true;
                } else if id.sym == "Error" || id.sym == "CpResult" {
                    self.needs_result_error = true;
                }
            }
            Expr::Dict(_) => self.needs_collections = true,
            Expr::Spawn(_) | Expr::Receive(_) => self.needs_actors = true,
            Expr::ReflectType { .. } | Expr::ReflectOf { .. } => self.needs_reflection = true,
            Expr::If {
                test, cons, alt, ..
            } => {
                self.scan_expr(test);
                self.scan_expr(cons);
                if let Some(a) = alt {
                    self.scan_expr(a);
                }
            }
            Expr::QuestionFallback { expr, fallback, .. } => {
                if !self.is_lib_mode {
                    self.needs_result_error = true;
                }
                self.scan_expr(expr);
                self.scan_expr(fallback);
            }
            Expr::NullCoalesce { left, right } => {
                if !self.is_lib_mode {
                    self.needs_null_coalesce = true;
                }
                self.scan_expr(left);
                self.scan_expr(right);
            }
            Expr::Question(q) => {
                self.needs_result_error = true;
                self.scan_expr(q);
            }
            Expr::Catch { expr, body, .. } => {
                self.needs_result_error = true;
                self.scan_expr(expr);
                self.scan_stmt(body);
            }
            Expr::ErrWrap(inner) => {
                self.needs_result_error = true;
                self.scan_expr(inner);
            }
            Expr::Template(_) => {
                self.needs_fmt = true;
            }
            Expr::Bin(b) => {
                match b.op {
                    BinaryOp::Eq | BinaryOp::NotEq => {
                        if !self.is_lib_mode {
                            let has_num = matches!(
                                &*b.left,
                                Expr::Lit(Lit::Int(_)) | Expr::Lit(Lit::Float(_))
                            ) || matches!(
                                &*b.right,
                                Expr::Lit(Lit::Int(_)) | Expr::Lit(Lit::Float(_))
                            );
                            let is_enum = matches!(&*b.left, Expr::EnumInit(_))
                                || matches!(&*b.right, Expr::EnumInit(_));
                            let is_null = matches!(&*b.left, Expr::Lit(Lit::Null))
                                || matches!(&*b.right, Expr::Lit(Lit::Null));
                            let is_bool = matches!(&*b.left, Expr::Lit(Lit::Bool(_)))
                                || matches!(&*b.right, Expr::Lit(Lit::Bool(_)));
                            if !has_num && !is_enum && !is_null && !is_bool {
                                self.needs_equality = true;
                            }
                        }
                    }
                    BinaryOp::Add => {
                        if !self.is_lib_mode {
                            self.needs_add = true;
                        }
                    }
                    BinaryOp::Lt | BinaryOp::Gt | BinaryOp::LtEq | BinaryOp::GtEq => {
                        if !self.is_lib_mode {
                            self.needs_comparison = true;
                        }
                    }
                    BinaryOp::Shl | BinaryOp::Shr => {
                        self.needs_shift = true;
                    }
                    BinaryOp::Assign => {
                        if matches!(&*b.left, Expr::Index { .. }) {
                            self.needs_cast_int = true;
                        }
                    }
                    _ => {}
                }
                self.scan_expr(&b.left);
                self.scan_expr(&b.right);
            }
            Expr::New(c) => {
                if let Expr::Ident(id) = &*c.callee {
                    if id.sym == "Map" || id.sym == "Dict" || id.sym == "Set" {
                        self.needs_collections = true;
                    }
                }
                self.scan_expr(&c.callee);
                for a in &c.args {
                    self.scan_expr(a);
                }
            }
            Expr::Call(c) => {
                if let Expr::Ident(id) = &*c.callee {
                    if (id.sym == "expect_eq" || id.sym == "@expect_eq") && !self.is_lib_mode {
                        self.needs_equality = true;
                    }
                    if id.sym == "@not" && !self.is_lib_mode {
                        self.needs_not = true;
                    }
                    if (id.sym == "string" || id.sym == "String") && c.args.len() == 1 {
                        self.needs_fmt = true;
                        self.needs_string_helpers = true;
                    }
                    match id.sym.as_str() {
                        "u8" | "i8" | "u16" | "i16" | "u32" | "i32" | "u64" | "i64" | "u128"
                        | "i128" | "usize" | "isize" => {
                            self.needs_cast_int = true;
                        }
                        _ => {}
                    }
                }
                if let Expr::Member(m) = &*c.callee {
                    if m.prop.sym == "toString" && c.args.is_empty() {
                        self.needs_fmt = true;
                        self.needs_string_helpers = true;
                    }
                    if let Expr::Ident(id) = &*m.obj {
                        if id.sym == "Math" && m.prop.sym == "random" {
                            self.needs_math_random = true;
                        }
                        if (id.sym == "string" || id.sym == "String") && m.prop.sym == "from" {
                            self.needs_fmt = true;
                            self.needs_string_helpers = true;
                        }
                    }
                    match m.prop.sym.as_str() {
                        "push" | "pop" => self.needs_array_helpers = true,
                        "includes" => self.needs_index_helpers = true,
                        "indexOf" | "lastIndexOf" if c.args.len() == 1 => {
                            self.needs_index_helpers = true
                        }
                        "slice" | "substring" if !c.args.is_empty() => {
                            self.needs_iter_helpers = true
                        }
                        "split" if c.args.len() == 1 => {
                            let is_non_int_lit = matches!(
                                &c.args[0],
                                Expr::Lit(Lit::Str(_) | Lit::Float(_) | Lit::Bool(_) | Lit::Null)
                            );
                            let is_static_type_recv = matches!(
                                &*m.obj,
                                Expr::Ident(id)
                                    if id
                                        .sym
                                        .chars()
                                        .next()
                                        .map_or(false, |ch| ch.is_ascii_uppercase())
                            );
                            if !is_non_int_lit && !is_static_type_recv {
                                self.needs_split_at_mut = true;
                            }
                        }
                        "withLock" | "withRead" | "withWrite" => self.needs_sync = true,
                        _ => {}
                    }
                }
                self.scan_expr(&c.callee);
                for a in &c.args {
                    self.scan_expr(a);
                }
            }
            Expr::Builtin(b) => {
                if b.name == "@hash" {
                    self.needs_hash = true;
                } else if b.name == "@dot" || b.name == "@l2sq" {
                    self.needs_simd_math = true;
                } else if b.name == "@splitAtMut" {
                    self.needs_split_at_mut = true;
                } else if b.name == "@not" && !self.is_lib_mode {
                    self.needs_not = true;
                } else if matches!(
                    b.name.as_str(),
                    "@net_listen"
                        | "@net_accept_async"
                        | "@net_read_async"
                        | "@net_write_async"
                        | "@create_pending_task"
                        | "@complete_task"
                        | "@task_spawn"
                        | "@task_join"
                ) {
                    self.needs_actors = true;
                }
                for a in &b.args {
                    self.scan_expr(a);
                }
            }
            Expr::OptCall(c) => {
                self.needs_opt_chaining = true;
                self.scan_expr(&c.callee);
                for a in &c.args {
                    self.scan_expr(a);
                }
            }
            Expr::Member(m) => self.scan_expr(&m.obj),
            Expr::OptMember(m) => {
                self.needs_opt_chaining = true;
                self.scan_expr(&m.obj);
            }
            Expr::Slice(s) => {
                self.scan_expr(&s.obj);
                if let Some(st) = &s.start {
                    self.scan_expr(st);
                }
                if let Some(en) = &s.end {
                    self.scan_expr(en);
                }
                if let Some(sent) = &s.sentinel {
                    self.scan_expr(sent);
                }
            }
            Expr::Index { obj, index } => {
                self.scan_expr(obj);
                self.scan_expr(index);
            }
            Expr::Array(items) => {
                if !self.is_lib_mode {
                    self.needs_array_helpers = true;
                }
                for item in items {
                    self.scan_expr(item);
                }
            }
            Expr::ArrayRepeat(val, count) => {
                self.scan_expr(val);
                self.scan_expr(count);
            }
            Expr::Object(fields) => {
                for f in fields {
                    self.scan_expr(&f.val);
                }
            }
            Expr::EnumInit(ei) => {
                if let Some(fields) = &ei.fields {
                    for f in fields {
                        self.scan_expr(&f.val);
                    }
                }
                if let Some(args) = &ei.tuple_args {
                    for a in args {
                        self.scan_expr(a);
                    }
                }
            }
            Expr::Match(m) => {
                self.needs_result_error = true;
                self.needs_string_helpers = true;
                self.needs_equality = true;
                self.scan_expr(&m.disc);
                for arm in &m.arms {
                    self.scan_pat(&arm.pat);
                    if let Some(g) = &arm.guard {
                        self.scan_expr(g);
                    }
                    self.scan_stmt(&arm.body);
                }
            }
            Expr::Cast {
                expr, target_ty, ..
            } => {
                self.needs_cast_int = true;
                self.scan_expr(expr);
                self.scan_type(target_ty);
            }
            Expr::Borrow(inner, _)
            | Expr::Move(inner)
            | Expr::Await(inner)
            | Expr::Unwrap(inner)
            | Expr::Deref(inner)
            | Expr::BitNot(inner)
            | Expr::Neg(inner)
            | Expr::ComptimeExpr(inner)
            | Expr::BoxNew(inner) => {
                self.scan_expr(inner);
            }
            Expr::Arrow(arr) => match &*arr.body {
                ArrowBody::Expr(e) => self.scan_expr(e),
                ArrowBody::Block(b) => self.scan_block(b),
            },
            Expr::Unsafe(b) | Expr::Comptime(b) | Expr::Arena(b) => self.scan_block(b),
            Expr::LabeledBlock { block, .. } => self.scan_block(block),
            Expr::While { stmt, else_expr } => {
                self.scan_expr(&stmt.test);
                if let Some(c) = &stmt.continue_expr {
                    self.scan_expr(c);
                }
                self.scan_stmt(&stmt.body);
                if let Some(e) = else_expr {
                    self.scan_expr(e);
                }
            }
            Expr::LabeledBreak(_, _, val) => {
                if let Some(v) = val {
                    self.scan_expr(v);
                }
            }
            Expr::Zig(z) => {
                if let Some(cls) = super::Codegen::parse_anon_struct_or_union(&z.code) {
                    self.scan_decl(&Decl::Class(cls));
                }
            }
            _ => {}
        }
    }
}
