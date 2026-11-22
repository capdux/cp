mod infer_cp;
mod infer_zig;
mod map_type;

use super::*;
use crate::ast::*;
use crate::Span;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

impl Codegen {
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
        }
        vars
    }

    pub fn type_from_name_static(name: &str) -> Type {
        match name {
            "i8" => Type::I8,
            "u8" => Type::U8,
            "i16" => Type::I16,
            "u16" => Type::U16,
            "i32" => Type::I32,
            "u32" => Type::U32,
            "i64" => Type::I64,
            "u64" => Type::U64,
            "isize" => Type::ISize,
            "usize" => Type::USize,
            "f32" => Type::F32,
            "f64" => Type::F64,
            "bool" => Type::Bool,
            "string" => Type::String,
            _ => Type::Ref(Ident {
                sym: name.to_string(),
                span: Span { start: 0, end: 0 },
            }),
        }
    }

    pub fn fixed_or_vec_elem_type(ty: &Type) -> Option<Type> {
        match ty {
            Type::FixedArray(elem, _) => Some((**elem).clone()),
            Type::Ref(id) if id.sym.starts_with('[') => {
                let idx = id.sym.find(']')?;
                let elem_name = id.sym[idx + 1..].trim_start_matches("const ").trim();
                Some(Self::type_from_name_static(elem_name))
            }
            Type::Ref(id) if id.sym.starts_with("@Vector(") => {
                let comma_idx = id.sym.find(',')?;
                let elem_name = id.sym[comma_idx + 1..].trim_end_matches(')').trim();
                Some(Self::type_from_name_static(elem_name))
            }
            _ => None,
        }
    }

    pub fn fnv1a_type_id(name: &str) -> u64 {
        let mut hash: u64 = 0xcbf29ce484222325;
        for b in name.as_bytes() {
            hash ^= *b as u64;
            hash = hash.wrapping_mul(0x100000001b3);
        }
        hash
    }

    pub fn is_numeric_type(ty: &Type) -> bool {
        matches!(
            ty,
            Type::I8
                | Type::U8
                | Type::I16
                | Type::U16
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
        )
    }

    pub fn is_scalar_or_primitive_type(ty: &Type) -> bool {
        match ty {
            Type::I8
            | Type::U8
            | Type::I16
            | Type::U16
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
            | Type::String
            | Type::RawPtr { .. } => true,
            Type::BorrowType { .. } => true,
            Type::FixedArray(inner, _) | Type::Array(inner) => {
                Self::is_scalar_or_primitive_type(inner)
            }
            Type::Optional(inner) => Self::is_scalar_or_primitive_type(inner),
            _ => false,
        }
    }

    pub fn method_is_zero_arg_destructor(m: &Function) -> bool {
        !m.is_static && m.ident.sym == "__cp_deinit"
    }

    pub fn method_is_zero_arg_err_destructor(m: &Function) -> bool {
        !m.is_static && m.ident.sym == "__cp_errdeinit"
    }

    pub fn current_fn_returns_error_union(&self) -> bool {
        match &self.current_fn_ret_ty {
            Some(Type::ErrorUnion(_)) => true,
            Some(Type::ResultType { err: fn_err, .. }) => self.is_native_error_union_err(fn_err),
            Some(Type::Ref(id)) => {
                let resolved = self.resolve_type_alias_sym(&id.sym);
                id.sym.contains('!') || resolved.contains('!')
            }
            _ => false,
        }
    }

    pub fn is_builtin_raii_type(&self, ty: Option<&Type>) -> bool {
        let Some(mut t) = ty else {
            return false;
        };
        while let Type::Optional(inner) = t {
            t = inner;
        }
        if let Type::Ref(id) = t {
            if self.class_defs.contains_key(&id.sym) {
                return false;
            }
            return id.sym.starts_with("LockGuard")
                || id.sym.starts_with("ReadGuard")
                || id.sym.starts_with("WriteGuard")
                || matches!(
                    id.sym.as_str(),
                    "Map"
                        | "Dict"
                        | "Set"
                        | "HashMap"
                        | "HashSet"
                        | "VecDeque"
                        | "BinaryHeap"
                        | "BTreeMap"
                        | "BTreeSet"
                        | "RegExp"
                );
        }
        false
    }

    pub fn is_raw_ptr_or_borrow_type(ty: &Type) -> bool {
        match ty {
            Type::BorrowType { .. }
            | Type::RawPtr { .. }
            | Type::ManyRawPtr(_)
            | Type::CPtr(_)
            | Type::SentinelPtr(_, _) => true,
            Type::Optional(inner) => Self::is_raw_ptr_or_borrow_type(inner),
            _ => false,
        }
    }

    pub fn is_refcounted_type(&self, ty: Option<&Type>) -> bool {
        let Some(mut t) = ty else {
            return false;
        };
        while let Type::Optional(inner) = t {
            t = inner;
        }
        match t {
            Type::Ref(id) => {
                if id.sym == "SharedTensor" {
                    return true;
                }
                if let Some(cls) = self.class_defs.get(&id.sym) {
                    if cls
                        .meta
                        .iter()
                        .any(|m| m.name.sym == "refCount" || m.name.sym == "refcount")
                    {
                        return true;
                    }
                    let has_retain = cls
                        .methods
                        .iter()
                        .any(|m| m.ident.sym == "retain" || m.ident.sym == "ref");
                    let has_release = cls
                        .methods
                        .iter()
                        .any(|m| m.ident.sym == "release" || m.ident.sym == "deref");
                    return has_retain && has_release;
                }
                false
            }
            _ => false,
        }
    }

    pub fn is_copy_type(&self, ty: Option<&Type>) -> bool {
        let Some(t) = ty else {
            return true;
        };
        if Self::is_scalar_or_primitive_type(t) || Self::is_raw_ptr_or_borrow_type(t) {
            return true;
        }
        match t {
            Type::Ref(id) => {
                if self.enum_defs.contains_key(&id.sym) {
                    return true;
                }
                if self.type_has_release(Some(t)) || self.type_has_err_release(Some(t)) {
                    return false;
                }
                if let Some(cls) = self.class_defs.get(&id.sym) {
                    if cls.is_union || self.value_classes.contains(&id.sym) {
                        return cls
                            .fields
                            .iter()
                            .all(|f| f.is_static || self.is_copy_type(Some(&f.ty)));
                    }
                    return false;
                }
                true
            }
            Type::BoxDyn(_) => false,
            Type::Optional(inner) => self.is_copy_type(Some(inner)),
            Type::Tuple(elems) => elems.iter().all(|e| self.is_copy_type(Some(e))),
            _ => true,
        }
    }

    pub fn type_has_release(&self, ty: Option<&Type>) -> bool {
        let Some(mut t) = ty else {
            return false;
        };
        // Raw pointers (*T!, *const T!, [*]T, [*c]T) and safe borrows (&T, *T) NEVER auto-drop!
        if Self::is_raw_ptr_or_borrow_type(t) {
            return false;
        }
        let mut depth = 0;
        while depth < 20 {
            if let Type::Optional(inner) = t {
                t = inner;
                depth += 1;
                continue;
            }
            if let Type::Ref(id) = t {
                let base = id
                    .sym
                    .split('<')
                    .next()
                    .unwrap_or(&id.sym)
                    .split('(')
                    .next()
                    .unwrap_or(&id.sym);
                if let Some(ta) = self
                    .type_aliases
                    .get(&id.sym)
                    .or_else(|| self.type_aliases.get(base))
                {
                    t = &ta.target_ty;
                    depth += 1;
                    continue;
                }
            }
            break;
        }
        if Self::is_raw_ptr_or_borrow_type(t) || Self::is_scalar_or_primitive_type(t) {
            return false;
        }
        match t {
            Type::Ref(id) => {
                let base_name = id
                    .sym
                    .split('<')
                    .next()
                    .unwrap_or(&id.sym)
                    .split('(')
                    .next()
                    .unwrap_or(&id.sym);
                if let Some(cls) = self
                    .class_defs
                    .get(&id.sym)
                    .or_else(|| self.class_defs.get(base_name))
                {
                    cls.methods
                        .iter()
                        .any(|m| Self::method_is_zero_arg_destructor(m))
                } else {
                    id.sym == "SharedTensor"
                        || id.sym.starts_with("LockGuard")
                        || id.sym.starts_with("ReadGuard")
                        || id.sym.starts_with("WriteGuard")
                        || matches!(
                            id.sym.as_str(),
                            "Map"
                                | "Dict"
                                | "Set"
                                | "HashMap"
                                | "HashSet"
                                | "VecDeque"
                                | "BinaryHeap"
                                | "BTreeMap"
                                | "BTreeSet"
                                | "RegExp"
                        )
                }
            }
            Type::BoxDyn(_) => true,
            _ => false,
        }
    }

    pub fn type_has_err_release(&self, ty: Option<&Type>) -> bool {
        let Some(mut t) = ty else {
            return false;
        };
        if Self::is_raw_ptr_or_borrow_type(t) {
            return false;
        }
        let mut depth = 0;
        while depth < 20 {
            if let Type::Optional(inner) = t {
                t = inner;
                depth += 1;
                continue;
            }
            if let Type::Ref(id) = t {
                let base = id
                    .sym
                    .split('<')
                    .next()
                    .unwrap_or(&id.sym)
                    .split('(')
                    .next()
                    .unwrap_or(&id.sym);
                if let Some(ta) = self
                    .type_aliases
                    .get(&id.sym)
                    .or_else(|| self.type_aliases.get(base))
                {
                    t = &ta.target_ty;
                    depth += 1;
                    continue;
                }
            }
            break;
        }
        if Self::is_raw_ptr_or_borrow_type(t) || Self::is_scalar_or_primitive_type(t) {
            return false;
        }
        match t {
            Type::Ref(id) => {
                let base_name = id
                    .sym
                    .split('<')
                    .next()
                    .unwrap_or(&id.sym)
                    .split('(')
                    .next()
                    .unwrap_or(&id.sym);
                if let Some(cls) = self
                    .class_defs
                    .get(&id.sym)
                    .or_else(|| self.class_defs.get(base_name))
                {
                    cls.methods
                        .iter()
                        .any(|m| Self::method_is_zero_arg_err_destructor(m))
                } else {
                    false
                }
            }
            _ => false,
        }
    }

    pub fn build_drop_call_snippet_with_method(var_expr: &str, method_name: &str) -> String {
        format!(
            concat!(
                "const __T = @TypeOf({0}); ",
                "if (@typeInfo(__T) == .optional) {{ ",
                "if ({0}) |__cp_opt_v| {{ ",
                "const __OptT = @TypeOf(__cp_opt_v); ",
                "if (@typeInfo(__OptT) == .pointer and @typeInfo(__OptT).pointer.size == .one and (if (@hasField(@TypeOf(@typeInfo(__OptT).pointer), \"is_const\")) !@typeInfo(__OptT).pointer.is_const else !@typeInfo(__OptT).pointer.attrs.@\"const\")) {{ ",
                "const __CpChild = @typeInfo(__OptT).pointer.child; ",
                "if (@typeInfo(__CpChild) == .@\"struct\" and @hasDecl(__CpChild, \"{1}\")) {{ __cp_opt_v.{1}(); }} ",
                "}} ",
                "else if (@typeInfo(__OptT) == .@\"struct\" and @hasDecl(__OptT, \"{1}\")) {{ ",
                "var _drop_val = __cp_opt_v; _drop_val.{1}(); ",
                "}} }} }} ",
                "else if (@typeInfo(__T) == .pointer and @typeInfo(__T).pointer.size == .one and (if (@hasField(@TypeOf(@typeInfo(__T).pointer), \"is_const\")) !@typeInfo(__T).pointer.is_const else !@typeInfo(__T).pointer.attrs.@\"const\")) {{ ",
                "const __CpChild = @typeInfo(__T).pointer.child; ",
                "if (@typeInfo(__CpChild) == .@\"struct\" and @hasDecl(__CpChild, \"{1}\")) {{ {0}.{1}(); }} ",
                "}} ",
                "else if (@typeInfo(__T) == .@\"struct\" and @hasDecl(__T, \"{1}\")) {{ ",
                "var _drop_val = {0}; _drop_val.{1}(); ",
                "}}"
            ),
            var_expr, method_name
        )
    }

    pub fn build_drop_call_snippet(var_expr: &str) -> String {
        Self::build_drop_call_snippet_with_method(var_expr, "__cp_deinit")
    }

    pub fn build_err_drop_call_snippet(var_expr: &str) -> String {
        Self::build_drop_call_snippet_with_method(var_expr, "__cp_errdeinit")
    }

    pub fn map_error_type(&self, err: &ErrorType) -> String {
        match err {
            ErrorType::Named(id) => {
                if id.sym == "Err" || id.sym == "Error" || id.sym == "err" || id.sym == "error" {
                    "Err".to_string()
                } else if id.sym == "AnyErr" || id.sym == "anyErr" || id.sym == "anyerror" {
                    "anyerror".to_string()
                } else if self.ref_classes.contains(&id.sym) || self.value_classes.contains(&id.sym)
                {
                    id.sym.clone()
                } else if self.class_defs.contains_key(&id.sym) {
                    format!("*{}", id.sym)
                } else {
                    id.sym.clone()
                }
            }
            ErrorType::Union(idents) => {
                let has_err = idents
                    .iter()
                    .any(|i| i.sym == "Err" || i.sym == "Error" || i.sym == "err");
                let all_zig_sets = idents.iter().all(|i| {
                    self.error_sets.contains_key(&i.sym)
                        || i.sym.starts_with("error{")
                        || i.sym == "anyerror"
                        || i.sym.contains('.')
                });
                let has_qualified = idents.iter().any(|i| i.sym.contains('.'));
                if has_err
                    && !self.is_lib_mode
                    && !has_qualified
                    && !self.error_sets.contains_key("Error")
                {
                    "Err".to_string()
                } else if all_zig_sets || has_qualified || self.is_lib_mode {
                    let names: Vec<String> = idents
                        .iter()
                        .map(|i| {
                            if i.sym == "anyerror" {
                                "anyerror".to_string()
                            } else {
                                i.sym.clone()
                            }
                        })
                        .collect();
                    format!("({})", names.join(" || "))
                } else {
                    let names: Vec<String> = idents.iter().map(|i| i.sym.clone()).collect();
                    format!("__UnionErr_{}", names.join("_"))
                }
            }
            ErrorType::AnonSet(idents) => {
                let names: Vec<String> = idents.iter().map(|i| i.sym.clone()).collect();
                format!("__AnonErr_{}", names.join("_"))
            }
            ErrorType::BoxDyn(id) => id.sym.clone(),
            ErrorType::DefaultError => "Err".to_string(),
            ErrorType::AnyErr => "anyerror".to_string(),
        }
    }

    pub fn check_type_validity_and_collect_errors(&mut self, ty: &Type) {
        match ty {
            Type::BareDyn(tr) => {
                panic!(
                    "CP Compile Error: Bare 'dyn {}' cannot be placed on the stack; wrap it in 'Box<dyn {}>'",
                    tr.sym, tr.sym
                );
            }
            Type::ResultType { ok, err } => {
                self.check_type_validity_and_collect_errors(ok);
                match err {
                    ErrorType::AnonSet(idents) => {
                        let zig_name = self.map_error_type(err);
                        if !self.anon_err_sets.iter().any(|(n, _)| n == &zig_name) {
                            let variants = idents.iter().map(|i| i.sym.clone()).collect();
                            self.anon_err_sets.push((zig_name, variants));
                        }
                    }
                    ErrorType::Union(idents) => {
                        let has_err = idents
                            .iter()
                            .any(|i| i.sym == "Err" || i.sym == "Error" || i.sym == "err");
                        let all_err_sets = idents.iter().all(|i| {
                            self.error_sets.contains_key(&i.sym)
                                || i.sym.starts_with("error{")
                                || i.sym == "anyerror"
                        });
                        if !has_err && !all_err_sets {
                            let zig_name = self.map_error_type(err);
                            if !self.union_err_sets.iter().any(|(n, _)| n == &zig_name) {
                                let sub_names = idents.iter().map(|i| i.sym.clone()).collect();
                                self.union_err_sets.push((zig_name, sub_names));
                            }
                        }
                    }
                    _ => {}
                }
            }
            Type::Optional(inner)
            | Type::Array(inner)
            | Type::Shared(inner)
            | Type::ErrorUnion(inner)
            | Type::RawPtr { inner, .. }
            | Type::BorrowType { inner, .. } => {
                self.check_type_validity_and_collect_errors(inner);
            }
            Type::Tuple(tys) => {
                for t in tys {
                    self.check_type_validity_and_collect_errors(t);
                }
            }
            Type::Object(fields) => {
                for f in fields {
                    self.check_type_validity_and_collect_errors(&f.ty);
                }
            }
            _ => {}
        }
    }

    pub fn scan_stmt_types(&mut self, stmt: &Stmt) {
        match stmt {
            Stmt::Var(v) => {
                if let Some(t) = &v.ty {
                    self.check_type_validity_and_collect_errors(t);
                }
            }
            Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
                for s in &b.body {
                    self.scan_stmt_types(s);
                }
            }
            Stmt::If(i) => {
                self.scan_stmt_types(&i.cons);
                if let Some(alt) = &i.alt {
                    self.scan_stmt_types(alt);
                }
            }
            Stmt::While(w) => self.scan_stmt_types(&w.body),
            Stmt::For(f) => self.scan_stmt_types(&f.body),
            Stmt::Match(m) => {
                for arm in &m.arms {
                    self.scan_stmt_types(&arm.body);
                }
            }
            Stmt::TryCatch {
                try_block,
                catch_body,
                ..
            } => {
                for s in &try_block.body {
                    self.scan_stmt_types(s);
                }
                if let Some(cb) = catch_body {
                    self.scan_stmt_types(cb);
                }
            }
            _ => {}
        }
    }

    pub fn verify_error_type_compatibility(&self, src: &ErrorType, dst: &ErrorType) {
        if src == dst {
            return;
        }
        let src_name = self.map_error_type(src);
        let dst_name = self.map_error_type(dst);
        if src_name == dst_name
            || dst_name == "Error"
            || dst_name == "Err"
            || dst_name == "anyerror"
            || dst_name == "AnyErr"
        {
            return;
        }
        match (src, dst) {
            (ErrorType::Named(s_id), ErrorType::Union(d_ids)) => {
                if d_ids.iter().any(|d| d.sym == s_id.sym) {
                    return;
                }
            }
            (ErrorType::Union(s_ids), ErrorType::Union(d_ids)) => {
                if s_ids.iter().all(|s| d_ids.iter().any(|d| d.sym == s.sym)) {
                    return;
                }
            }
            (ErrorType::AnonSet(s_ids), ErrorType::AnonSet(d_ids)) => {
                if s_ids.iter().all(|s| d_ids.iter().any(|d| d.sym == s.sym)) {
                    return;
                }
            }
            (ErrorType::Named(s_id), ErrorType::Named(d_id)) => {
                if let Some(en) = self.enum_defs.get(&d_id.sym) {
                    if en
                        .variants
                        .iter()
                        .any(|v| v.from_err.as_ref().map_or(false, |f| f.sym == s_id.sym))
                    {
                        return;
                    }
                }
                if self.trait_defs.contains_key(&d_id.sym) {
                    if let Some(cls) = self.class_defs.get(&s_id.sym) {
                        if cls.implements.iter().any(|i| i.sym == d_id.sym) {
                            return;
                        }
                    }
                }
            }
            (ErrorType::Named(s_id), ErrorType::BoxDyn(tr_id)) => {
                if s_id.sym == tr_id.sym {
                    return;
                }
                if let Some(cls) = self.class_defs.get(&s_id.sym) {
                    if cls.implements.iter().any(|i| i.sym == tr_id.sym) {
                        return;
                    }
                }
            }
            _ => {}
        }
        panic!(
            "CP Compile Error: Error type '{}' cannot be automatically converted to '{}' (missing 'from {}' in target error enum)",
            src_name, dst_name, src_name
        );
    }

    pub fn is_class_field(&self, field_name: &str) -> bool {
        if self.locals.contains_key(field_name)
            || self.var_cp_types.contains_key(field_name)
            || self.block_locals.contains(field_name)
        {
            return false;
        }
        if let Some(c_name) = &self.current_class_name {
            if let Some(cls) = self.class_defs.get(c_name) {
                return cls
                    .fields
                    .iter()
                    .any(|f| !f.is_static && f.ident.sym == field_name);
            }
        }
        false
    }

    pub fn is_static_class_field(&self, field_name: &str) -> bool {
        if self.locals.contains_key(field_name)
            || self.var_cp_types.contains_key(field_name)
            || self.block_locals.contains(field_name)
        {
            return false;
        }
        if let Some(c_name) = &self.current_class_name {
            if let Some(cls) = self.class_defs.get(c_name) {
                return cls
                    .fields
                    .iter()
                    .any(|f| f.is_static && f.ident.sym == field_name);
            }
        }
        false
    }

    fn substitute_generic_type(ty: &Type, subst: &HashMap<String, String>) -> Type {
        match ty {
            Type::Ref(id) => {
                if let Some(rep) = subst.get(&id.sym) {
                    match rep.as_str() {
                        "i8" => Type::I8,
                        "u8" => Type::U8,
                        "i16" => Type::I16,
                        "u16" => Type::U16,
                        "i32" => Type::I32,
                        "u32" => Type::U32,
                        "i64" => Type::I64,
                        "u64" => Type::U64,
                        "isize" => Type::ISize,
                        "usize" => Type::USize,
                        "f32" => Type::F32,
                        "f64" => Type::F64,
                        "bool" => Type::Bool,
                        "string" | "[]const u8" => Type::String,
                        _ => Type::Ref(Ident {
                            span: id.span,
                            sym: rep.clone(),
                        }),
                    }
                } else if let Some(pos) = id.sym.find('<') {
                    if id.sym.ends_with('>') {
                        let base = &id.sym[..pos];
                        let args_part = &id.sym[pos + 1..id.sym.len() - 1];
                        let new_args: Vec<String> = args_part
                            .split(',')
                            .map(|a| {
                                let trimmed = a.trim();
                                subst
                                    .get(trimmed)
                                    .cloned()
                                    .unwrap_or_else(|| trimmed.to_string())
                            })
                            .collect();
                        Type::Ref(Ident {
                            span: id.span,
                            sym: format!("{}<{}>", base, new_args.join(", ")),
                        })
                    } else {
                        ty.clone()
                    }
                } else {
                    ty.clone()
                }
            }
            Type::Optional(inner) => {
                Type::Optional(Box::new(Self::substitute_generic_type(inner, subst)))
            }
            Type::ErrorUnion(inner) => {
                Type::ErrorUnion(Box::new(Self::substitute_generic_type(inner, subst)))
            }
            Type::BorrowType {
                inner,
                is_mut,
                lifetimes,
            } => Type::BorrowType {
                inner: Box::new(Self::substitute_generic_type(inner, subst)),
                is_mut: *is_mut,
                lifetimes: lifetimes.clone(),
            },
            Type::RawPtr { inner, depth } => Type::RawPtr {
                inner: Box::new(Self::substitute_generic_type(inner, subst)),
                depth: *depth,
            },
            Type::Array(inner) => {
                Type::Array(Box::new(Self::substitute_generic_type(inner, subst)))
            }
            _ => ty.clone(),
        }
    }

    pub fn lookup_method(
        &self,
        type_name: &str,
        method_name: &str,
    ) -> Option<(Vec<Param>, Option<Type>, ReceiverMode)> {
        let clean = type_name.trim_start_matches('?').trim_start_matches('*');
        let (base_name, type_args_str) = if let Some(pos) = clean.find('<') {
            (&clean[..pos], clean[pos + 1..].trim_end_matches('>'))
        } else if let Some(pos) = clean.find('(') {
            (&clean[..pos], clean[pos + 1..].trim_end_matches(')'))
        } else {
            (clean, "")
        };
        if let Some(cls) = self.class_defs.get(base_name) {
            let mut subst = HashMap::new();
            if !type_args_str.is_empty() && !cls.generic_params.is_empty() {
                for (gp, arg_s) in cls.generic_params.iter().zip(type_args_str.split(',')) {
                    let gp_name = match gp {
                        GenericParam::Type { ident, .. } | GenericParam::Const { ident, .. } => {
                            ident.sym.clone()
                        }
                    };
                    subst.insert(gp_name, arg_s.trim().to_string());
                }
            }
            if let Some(m) = cls.methods.iter().find(|m| m.ident.sym == method_name) {
                let ret_ty = m.return_type.as_ref().map(|rt| {
                    if subst.is_empty() {
                        rt.clone()
                    } else {
                        Self::substitute_generic_type(rt, &subst)
                    }
                });
                return Some((m.params.clone(), ret_ty, Self::get_func_receiver_mode(m)));
            }
        }
        if let Some(tr) = self.trait_defs.get(base_name) {
            if let Some(tm) = tr.methods.iter().find(|m| m.ident.sym == method_name) {
                return Some((
                    tm.params.clone(),
                    tm.return_type.clone(),
                    Self::get_sig_receiver_mode(tm),
                ));
            }
        }
        None
    }

    pub fn resolve_type_alias_sym<'a>(&'a self, mut sym: &'a str) -> &'a str {
        let mut depth = 0;
        while depth < 20 {
            let base = sym
                .split('<')
                .next()
                .unwrap_or(sym)
                .split('(')
                .next()
                .unwrap_or(sym);
            if let Some(ta) = self
                .type_aliases
                .get(sym)
                .or_else(|| self.type_aliases.get(base))
            {
                if let Type::Ref(id) = &ta.target_ty {
                    sym = &id.sym;
                    depth += 1;
                    continue;
                }
            }
            break;
        }
        sym
    }

    pub fn resolve_var_type_name(&self, var_name: &str) -> Option<String> {
        if var_name == "this" || var_name == "_" || var_name == "self" {
            return self.current_class_name.clone();
        }
        if let Some(ty) = self.var_cp_types.get(var_name) {
            match ty {
                Type::Ref(id) | Type::BoxDyn(id) => return Some(id.sym.clone()),
                Type::RawPtr { inner, .. } => {
                    if let Type::Ref(id) | Type::BoxDyn(id) = &**inner {
                        return Some(id.sym.clone());
                    }
                }
                Type::BorrowType { inner, .. } => {
                    if let Type::Ref(id) | Type::BoxDyn(id) = &**inner {
                        return Some(id.sym.clone());
                    }
                }
                Type::Optional(inner) => match &**inner {
                    Type::Ref(id) | Type::BoxDyn(id) => return Some(id.sym.clone()),
                    Type::RawPtr { inner: r_in, .. } => {
                        if let Type::Ref(id) | Type::BoxDyn(id) = &**r_in {
                            return Some(id.sym.clone());
                        }
                    }
                    Type::BorrowType { inner: b_in, .. } => {
                        if let Type::Ref(id) | Type::BoxDyn(id) = &**b_in {
                            return Some(id.sym.clone());
                        }
                    }
                    _ => {}
                },
                Type::Shared(inner) => {
                    if let Type::Ref(id) = &**inner {
                        return Some(id.sym.clone());
                    }
                }
                _ => {}
            }
        }

        if let Some(zig_ty) = self.locals.get(var_name) {
            let clean = zig_ty.trim_start_matches('?').trim_start_matches('*');
            let base = clean
                .split(|c| c == '<' || c == '(')
                .next()
                .unwrap_or(clean);
            if clean == "Error"
                || self.class_defs.contains_key(base)
                || self.trait_defs.contains_key(base)
            {
                return Some(clean.to_string());
            }
        }
        None
    }
}

pub(crate) fn type_to_display_string(ty: &Type) -> String {
    match ty {
        Type::U8 => "u8".to_string(),
        Type::I8 => "i8".to_string(),
        Type::U16 => "u16".to_string(),
        Type::I16 => "i16".to_string(),
        Type::I32 => "i32".to_string(),
        Type::U32 => "u32".to_string(),
        Type::I64 => "i64".to_string(),
        Type::U64 => "u64".to_string(),
        Type::I128 => "i128".to_string(),
        Type::U128 => "u128".to_string(),
        Type::ISize => "isize".to_string(),
        Type::USize => "usize".to_string(),
        Type::F32 => "f32".to_string(),
        Type::F64 => "f64".to_string(),
        Type::Bool => "bool".to_string(),
        Type::String => "string".to_string(),
        Type::Void => "void".to_string(),
        Type::Any => "any".to_string(),
        Type::PID => "PID".to_string(),
        Type::Array(inner) => format!("Array<{}>", type_to_display_string(inner)),
        Type::FixedArray(inner, size) => format!("{}[{}]", type_to_display_string(inner), size),
        Type::Ref(id) => id.sym.clone(),
        Type::Optional(inner) => format!("?{}", type_to_display_string(inner)),
        Type::ResultType { ok, err } => {
            let ok_str = type_to_display_string(ok);
            match err {
                ErrorType::AnyErr => format!("anyerror!{}", ok_str),
                ErrorType::Named(id) => format!("{}!{}", id.sym, ok_str),
                _ => ok_str,
            }
        }
        Type::ErrorUnion(inner) => format!("{}!", type_to_display_string(inner)),
        Type::BorrowType { inner, .. } => type_to_display_string(inner),
        Type::Shared(inner) => type_to_display_string(inner),
        Type::RawPtr { inner, depth } => {
            format!(
                "{}{}{}",
                "*".repeat(*depth),
                type_to_display_string(inner),
                "!"
            )
        }
        Type::ManyRawPtr(inner) => format!("[*]{}!", type_to_display_string(inner)),
        Type::CStr => "cstr".to_string(),
        Type::CFn(params, ret) => {
            let p_strs: Vec<String> = params.iter().map(|p| type_to_display_string(p)).collect();
            format!(
                "@c fn({}) -> {}",
                p_strs.join(", "),
                type_to_display_string(ret)
            )
        }
        Type::Fn(params, ret) => {
            let p_strs: Vec<String> = params.iter().map(|p| type_to_display_string(p)).collect();
            format!(
                "fn({}) -> {}",
                p_strs.join(", "),
                type_to_display_string(ret)
            )
        }
        Type::Aligned { qual, inner } => {
            format!("{}{}", qual, type_to_display_string(inner))
        }
        _ => "any".to_string(),
    }
}
