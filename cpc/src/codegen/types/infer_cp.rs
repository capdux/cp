use super::super::*;
use crate::ast::*;
use std::collections::HashMap;

impl Codegen {
    pub(crate) fn infer_cp_type(&self, expr: &Expr) -> Option<Type> {
        self.infer_cp_type_with_mock(expr, &HashMap::new())
    }

    pub(crate) fn infer_cp_type_with_mock(
        &self,
        expr: &Expr,
        mock_types: &HashMap<String, Type>,
    ) -> Option<Type> {
        match expr {
            Expr::Lit(Lit::Int(_)) => Some(Type::I32),
            Expr::Lit(Lit::Float(_)) => Some(Type::F32),
            Expr::Lit(Lit::Str(_)) => Some(Type::String),
            Expr::Lit(Lit::Bool(_)) => Some(Type::Bool),
            Expr::Template(_) => Some(Type::String),
            Expr::Ident(id) => {
                if let Some(t) = mock_types.get(&id.sym) {
                    return Some(t.clone());
                }
                if id.sym == "Managed" {
                    return Some(Type::Ref(Ident {
                        sym: "Managed".to_string(),
                        span: id.span,
                    }));
                }
                if id.sym == "true" || id.sym == "false" {
                    return Some(Type::Bool);
                }
                if id.sym == "this" || id.sym == "_" || id.sym == "self" {
                    return self.current_class_name.as_ref().map(|c| {
                        Type::Ref(Ident {
                            span: Span { start: 0, end: 0 },
                            sym: c.clone(),
                        })
                    });
                }
                if let Some(t) = self
                    .var_cp_types
                    .get(&id.sym)
                    .or_else(|| self.global_var_types.get(&id.sym))
                {
                    if self.narrowed_null_vars.contains(&id.sym) {
                        return Some(Self::unwrap_optional_type(t));
                    }
                    return Some(t.clone());
                }
                if let Some(c_name) = &self.current_class_name {
                    if let Some(cls) = self.class_defs.get(c_name) {
                        if let Some(f) = cls.fields.iter().find(|f| f.ident.sym == id.sym) {
                            return Some(f.ty.clone());
                        }
                    }
                }
                None
            }
            Expr::New(nc) => {
                if let Expr::Ident(cid) = &*nc.callee {
                    return Some(Type::Ref(cid.clone()));
                }
                None
            }
            Expr::BoxNew(_) => self.expected_box_trait.as_ref().map(|tr| {
                Type::BoxDyn(Ident {
                    span: Span { start: 0, end: 0 },
                    sym: tr.clone(),
                })
            }),
            Expr::Downcast {
                target_ty, is_mut, ..
            } => Some(Type::Optional(Box::new(Type::BorrowType {
                inner: Box::new(Type::Ref(target_ty.clone())),
                is_mut: *is_mut,
                lifetimes: vec![],
            }))),
            Expr::EnumInit(ei) => Some(Type::Ref(ei.enum_name.clone())),
            Expr::Cast {
                expr, target_ty, ..
            } => {
                if let Type::RawPtr { inner, depth } = target_ty {
                    if **inner == Type::Any {
                        if let Expr::Borrow(b, _) = &**expr {
                            if let Some(mut base_ty) = self.infer_cp_type_with_mock(b, mock_types) {
                                for _ in 1..*depth {
                                    if let Type::RawPtr {
                                        inner: sub,
                                        depth: d,
                                    } = base_ty
                                    {
                                        if d > 1 {
                                            base_ty = Type::RawPtr {
                                                inner: sub,
                                                depth: d - 1,
                                            };
                                        } else {
                                            base_ty = *sub;
                                        }
                                    }
                                }
                                return Some(Type::RawPtr {
                                    inner: Box::new(base_ty),
                                    depth: *depth,
                                });
                            }
                        }
                    }
                }
                Some(target_ty.clone())
            }
            Expr::Deref(inner) => {
                let inner_ty = self.infer_cp_type_with_mock(inner, mock_types)?;
                match inner_ty {
                    Type::RawPtr { inner, depth } => {
                        if depth > 1 {
                            Some(Type::RawPtr {
                                inner,
                                depth: depth - 1,
                            })
                        } else {
                            Some(*inner)
                        }
                    }
                    Type::BorrowType { inner, .. } => Some(*inner),
                    other => Some(other),
                }
            }
            Expr::BitNot(inner) => self.infer_cp_type_with_mock(inner, mock_types),
            Expr::Move(inner) => self.infer_cp_type_with_mock(inner, mock_types),
            Expr::Borrow(inner, is_mut) => {
                let inner_ty = self.infer_cp_type_with_mock(inner, mock_types)?;
                match inner_ty {
                    Type::BorrowType { inner: sub, .. } => Some(Type::BorrowType {
                        inner: sub,
                        is_mut: *is_mut,
                        lifetimes: vec![],
                    }),
                    other => Some(Type::BorrowType {
                        inner: Box::new(other),
                        is_mut: *is_mut,
                        lifetimes: vec![],
                    }),
                }
            }
            Expr::Await(inner) => self.infer_cp_type_with_mock(inner, mock_types),
            Expr::Call(c) => {
                if let Expr::Ident(fid) = &*c.callee {
                    if fid.sym == "join" || fid.sym == "@task_join" {
                        let mut tup_tys = Vec::new();
                        for _ in &c.args {
                            tup_tys.push(Type::String);
                        }
                        return Some(Type::Tuple(tup_tys));
                    }
                    if fid.sym == "spawn"
                        || fid.sym == "@task_spawn"
                        || fid.sym == "@task_spawn_cpu"
                    {
                        return Some(Type::Ref(Ident {
                            span: fid.span,
                            sym: "Task".to_string(),
                        }));
                    }
                    if fid.sym == "next" {
                        return self.current_fn_ret_ty.clone();
                    }
                    if fid.sym == "Error" {
                        return Some(Type::Ref(Ident {
                            span: Span { start: 0, end: 0 },
                            sym: "Error".to_string(),
                        }));
                    }
                    if fid.sym == "Mutex" {
                        if c.args.is_empty() {
                            return Some(Type::Ref(Ident {
                                sym: "Mutex".to_string(),
                                span: fid.span,
                            }));
                        } else {
                            let inner = self
                                .infer_cp_type_with_mock(&c.args[0], mock_types)
                                .unwrap_or(Type::I32);
                            return Some(Type::Ref(Ident {
                                sym: format!("Mutex<{}>", type_to_display_string(&inner)),
                                span: fid.span,
                            }));
                        }
                    }
                    if fid.sym == "RwLock" {
                        if c.args.is_empty() {
                            return Some(Type::Ref(Ident {
                                sym: "RwLock".to_string(),
                                span: fid.span,
                            }));
                        } else {
                            let inner = self
                                .infer_cp_type_with_mock(&c.args[0], mock_types)
                                .unwrap_or(Type::I32);
                            return Some(Type::Ref(Ident {
                                sym: format!("RwLock<{}>", type_to_display_string(&inner)),
                                span: fid.span,
                            }));
                        }
                    }
                    if fid.sym == "Atomic" {
                        let inner = if !c.args.is_empty() {
                            self.infer_cp_type_with_mock(&c.args[0], mock_types)
                                .unwrap_or(Type::I32)
                        } else {
                            Type::I32
                        };
                        return Some(Type::Ref(Ident {
                            sym: format!("Atomic<{}>", type_to_display_string(&inner)),
                            span: fid.span,
                        }));
                    }
                    if fid.sym == "Shared" {
                        if !c.args.is_empty() {
                            let inner = self
                                .infer_cp_type_with_mock(&c.args[0], mock_types)
                                .unwrap_or(Type::I32);
                            return Some(Type::Shared(Box::new(inner)));
                        }
                    }
                    if (fid.sym == "string" || fid.sym == "String") && c.args.len() == 1 {
                        return Some(Type::String);
                    }
                    if fid.sym == "@alloc" {
                        return Some(Type::ManyRawPtr(Box::new(Type::U8)));
                    }
                    match fid.sym.as_str() {
                        "i8" => return Some(Type::I8),
                        "u8" => return Some(Type::U8),
                        "i16" => return Some(Type::I16),
                        "u16" => return Some(Type::U16),
                        "i32" => return Some(Type::I32),
                        "u32" => return Some(Type::U32),
                        "i64" => return Some(Type::I64),
                        "u64" => return Some(Type::U64),
                        "i128" => return Some(Type::I128),
                        "u128" => return Some(Type::U128),
                        "isize" => return Some(Type::ISize),
                        "usize" => return Some(Type::USize),
                        "f32" => return Some(Type::F32),
                        "f64" => return Some(Type::F64),
                        "bool" => return Some(Type::Bool),
                        _ => {}
                    }
                    if self.enum_defs.contains_key(&fid.sym) && c.args.len() == 1 {
                        return Some(Type::Ref(fid.clone()));
                    }
                    if let Some(sig) = self.fn_sigs.get(&fid.sym) {
                        if let Some(rt) = &sig.return_type {
                            if let Type::Ref(rid) = rt {
                                for (idx, p) in sig.params.iter().enumerate() {
                                    let mut p_ty = &p.ty;
                                    while let Type::BorrowType { inner, .. } = p_ty {
                                        p_ty = inner;
                                    }
                                    if let Type::Ref(pid) = p_ty {
                                        if pid.sym == rid.sym && idx < c.args.len() {
                                            if let Some(arg_ty) = self
                                                .infer_cp_type_with_mock(&c.args[idx], mock_types)
                                            {
                                                return Some(arg_ty);
                                            }
                                        }
                                    }
                                }
                            }
                            if self.in_try_region {
                                if let Type::ResultType { ok, .. } = rt {
                                    return Some((**ok).clone());
                                }
                            }
                            return Some(rt.clone());
                        }
                    } else if let Some(c_name) = &self.current_class_name {
                        if let Some((_, Some(rt), _)) = self.lookup_method(c_name, &fid.sym) {
                            if self.in_try_region {
                                if let Type::ResultType { ok, .. } = &rt {
                                    return Some((**ok).clone());
                                }
                            }
                            return Some(rt);
                        }
                    }
                } else if let Expr::Member(mem) = &*c.callee {
                    if mem.prop.sym == "toString" && c.args.is_empty() {
                        return Some(Type::String);
                    }
                    if let Some(mut oty) = self.infer_cp_type_with_mock(&mem.obj, mock_types) {
                        while let Type::BorrowType { inner, .. } = oty {
                            oty = *inner;
                        }
                        while let Type::Shared(inner) = oty {
                            oty = *inner;
                        }
                        while let Type::RawPtr { inner, .. } = oty {
                            oty = *inner;
                        }
                        if let Type::Ref(id) = &oty {
                            let prop = mem.prop.sym.as_str();
                            if id.sym.starts_with("Mutex<") {
                                let inner_sym = &id.sym[6..id.sym.len() - 1];
                                if prop == "lock" {
                                    return Some(Type::Ref(Ident {
                                        sym: format!("LockGuard<{}>", inner_sym),
                                        span: mem.prop.span,
                                    }));
                                }
                                if prop == "tryLock" {
                                    return Some(Type::Optional(Box::new(Type::Ref(Ident {
                                        sym: format!("LockGuard<{}>", inner_sym),
                                        span: mem.prop.span,
                                    }))));
                                }
                                if prop == "isLocked" {
                                    return Some(Type::Bool);
                                }
                                if prop == "withLock" && !c.args.is_empty() {
                                    if let Expr::Arrow(arr) = &c.args[0] {
                                        let mut sub_mock = mock_types.clone();
                                        if !arr.params.is_empty() {
                                            if let Pattern::Ident(pid) = &arr.params[0] {
                                                sub_mock.insert(
                                                    pid.sym.clone(),
                                                    Self::type_from_name_static(inner_sym),
                                                );
                                            }
                                        }
                                        match &*arr.body {
                                            ArrowBody::Expr(e) => {
                                                return self.infer_cp_type_with_mock(e, &sub_mock)
                                            }
                                            ArrowBody::Block(b) => {
                                                if let Some(Stmt::Return(r)) = b.body.last() {
                                                    if let Some(a) = &r.arg {
                                                        return self
                                                            .infer_cp_type_with_mock(a, &sub_mock);
                                                    }
                                                }
                                                return Some(Type::Void);
                                            }
                                        }
                                    }
                                }
                            } else if id.sym == "Mutex" {
                                if prop == "tryLock" || prop == "isLocked" {
                                    return Some(Type::Bool);
                                }
                                if prop == "lock" || prop == "unlock" {
                                    return Some(Type::Void);
                                }
                            } else if id.sym.starts_with("RwLock<") {
                                let inner_sym = &id.sym[7..id.sym.len() - 1];
                                if prop == "read" {
                                    return Some(Type::Ref(Ident {
                                        sym: format!("ReadGuard<{}>", inner_sym),
                                        span: mem.prop.span,
                                    }));
                                }
                                if prop == "tryRead" {
                                    return Some(Type::Optional(Box::new(Type::Ref(Ident {
                                        sym: format!("ReadGuard<{}>", inner_sym),
                                        span: mem.prop.span,
                                    }))));
                                }
                                if prop == "write" {
                                    return Some(Type::Ref(Ident {
                                        sym: format!("WriteGuard<{}>", inner_sym),
                                        span: mem.prop.span,
                                    }));
                                }
                                if prop == "tryWrite" {
                                    return Some(Type::Optional(Box::new(Type::Ref(Ident {
                                        sym: format!("WriteGuard<{}>", inner_sym),
                                        span: mem.prop.span,
                                    }))));
                                }
                                if prop == "isReadLocked" || prop == "isWriteLocked" {
                                    return Some(Type::Bool);
                                }
                                if (prop == "withRead" || prop == "withWrite") && !c.args.is_empty()
                                {
                                    if let Expr::Arrow(arr) = &c.args[0] {
                                        let mut sub_mock = mock_types.clone();
                                        if !arr.params.is_empty() {
                                            if let Pattern::Ident(pid) = &arr.params[0] {
                                                sub_mock.insert(
                                                    pid.sym.clone(),
                                                    Self::type_from_name_static(inner_sym),
                                                );
                                            }
                                        }
                                        match &*arr.body {
                                            ArrowBody::Expr(e) => {
                                                return self.infer_cp_type_with_mock(e, &sub_mock)
                                            }
                                            ArrowBody::Block(b) => {
                                                if let Some(Stmt::Return(r)) = b.body.last() {
                                                    if let Some(a) = &r.arg {
                                                        return self
                                                            .infer_cp_type_with_mock(a, &sub_mock);
                                                    }
                                                }
                                                return Some(Type::Void);
                                            }
                                        }
                                    }
                                }
                            } else if id.sym == "RwLock" {
                                if prop == "tryLock"
                                    || prop == "tryLockShared"
                                    || prop == "isReadLocked"
                                    || prop == "isWriteLocked"
                                {
                                    return Some(Type::Bool);
                                }
                                if prop == "lock"
                                    || prop == "unlock"
                                    || prop == "lockShared"
                                    || prop == "unlockShared"
                                {
                                    return Some(Type::Void);
                                }
                            } else if id.sym.starts_with("Atomic<") {
                                let inner_sym = &id.sym[7..id.sym.len() - 1];
                                let inner_ty = Self::type_from_name_static(inner_sym);
                                if prop == "load"
                                    || prop == "swap"
                                    || prop == "add"
                                    || prop == "sub"
                                    || prop == "inc"
                                    || prop == "dec"
                                    || prop == "and"
                                    || prop == "or"
                                    || prop == "xor"
                                {
                                    return Some(inner_ty);
                                }
                                if prop == "cas" || prop == "compareAndSwap" || prop == "testAndSet"
                                {
                                    return Some(Type::Bool);
                                }
                                if prop == "store" || prop == "clear" {
                                    return Some(Type::Void);
                                }
                            } else if id.sym.starts_with("LockGuard<")
                                || id.sym.starts_with("ReadGuard<")
                                || id.sym.starts_with("WriteGuard<")
                            {
                                if prop == "release" || prop == "unlock" {
                                    return Some(Type::Void);
                                }
                            }
                        }
                    }
                    if mem.prop.sym == "split" && c.args.len() == 1 {
                        let arg_is_int = matches!(&c.args[0], Expr::Lit(Lit::Int(_)))
                            || matches!(
                                self.infer_cp_type_with_mock(&c.args[0], mock_types),
                                Some(
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
                                )
                            );
                        if arg_is_int {
                            if let Some(obj_ty) = self.infer_cp_type_with_mock(&mem.obj, mock_types)
                            {
                                let mut cur = &obj_ty;
                                let mut is_const = false;
                                let mut elem_opt: Option<Box<Type>> = None;
                                loop {
                                    match cur {
                                        Type::BorrowType { inner, is_mut, .. } => {
                                            if !*is_mut {
                                                is_const = true;
                                            }
                                            cur = inner;
                                        }
                                        Type::RawPtr { inner, depth: 1 } => cur = inner,
                                        Type::Shared(inner) => cur = inner,
                                        Type::Array(elem) | Type::FixedArray(elem, _) => {
                                            elem_opt = Some(elem.clone());
                                            break;
                                        }
                                        _ => break,
                                    }
                                }
                                if let Some(elem) = elem_opt {
                                    let slice_ty = if is_const {
                                        Type::BorrowType {
                                            inner: Box::new(Type::Array(elem)),
                                            is_mut: false,
                                            lifetimes: vec![],
                                        }
                                    } else {
                                        Type::Array(elem)
                                    };
                                    return Some(Type::Tuple(vec![slice_ty.clone(), slice_ty]));
                                }
                            }
                        }
                    }
                    if let Expr::Ident(oid) = &*mem.obj {
                        if (oid.sym == "string" || oid.sym == "String") && mem.prop.sym == "from" {
                            return Some(Type::String);
                        }
                        if oid.sym == "reflect" {
                            if mem.prop.sym == "hasMethod"
                                || mem.prop.sym == "hasField"
                                || mem.prop.sym == "has"
                            {
                                return Some(Type::Bool);
                            }
                        }
                        if oid.sym == "spawn" && mem.prop.sym == "cpu" {
                            return Some(Type::Ref(Ident {
                                span: mem.prop.span,
                                sym: "Task".to_string(),
                            }));
                        }
                        if oid.sym == "JSON" {
                            if mem.prop.sym == "stringify" || mem.prop.sym == "encode" {
                                return Some(Type::String);
                            }
                            if mem.prop.sym == "parse" || mem.prop.sym == "decode" {
                                if let Some(Expr::TypeVal { ty, .. }) = c.args.first() {
                                    return Some(Type::ErrorUnion(Box::new(ty.clone())));
                                }
                                if let Some(Expr::Ident(id)) = c.args.first() {
                                    let is_type = matches!(
                                        id.sym.as_str(),
                                        "i8" | "u8"
                                            | "i16"
                                            | "u16"
                                            | "i32"
                                            | "u32"
                                            | "i64"
                                            | "u64"
                                            | "i128"
                                            | "u128"
                                            | "isize"
                                            | "usize"
                                            | "f32"
                                            | "f64"
                                            | "bool"
                                            | "string"
                                    ) || self.class_defs.contains_key(&id.sym)
                                        || self.enum_defs.contains_key(&id.sym)
                                        || self.trait_defs.contains_key(&id.sym)
                                        || (!self.locals.contains_key(&id.sym)
                                            && !self.globals.contains(&id.sym)
                                            && id
                                                .sym
                                                .chars()
                                                .next()
                                                .map_or(false, |ch| ch.is_ascii_uppercase()));
                                    if is_type {
                                        return Some(Type::ErrorUnion(Box::new(Type::Ref(
                                            id.clone(),
                                        ))));
                                    }
                                }
                                return Some(Type::ErrorUnion(Box::new(Type::Ref(Ident {
                                    span: mem.prop.span,
                                    sym: "JsonValue".to_string(),
                                }))));
                            }
                        }
                        let oid_base = oid
                            .sym
                            .split(|ch| ch == '<' || ch == '(')
                            .next()
                            .unwrap_or(&oid.sym);
                        let resolved_base = self.resolve_type_alias_sym(oid_base);
                        let is_enum = self.enum_defs.contains_key(&oid.sym)
                            || self.enum_defs.contains_key(resolved_base);
                        if is_enum {
                            if mem.prop.sym == "fromName"
                                || mem.prop.sym == "fromString"
                                || mem.prop.sym == "get"
                            {
                                return Some(Type::Optional(Box::new(Type::Ref(oid.clone()))));
                            }
                            if mem.prop.sym == "has" {
                                return Some(Type::Bool);
                            }
                            let en_opt = self
                                .enum_defs
                                .get(&oid.sym)
                                .or_else(|| self.enum_defs.get(resolved_base));
                            if let Some(en) = en_opt {
                                if let Some(m) =
                                    en.methods.iter().find(|m| m.ident.sym == mem.prop.sym)
                                {
                                    return m.return_type.clone();
                                }
                                if en.variants.iter().any(|v| v.ident.sym == mem.prop.sym)
                                    || en.variants.is_empty()
                                {
                                    return Some(Type::Ref(oid.clone()));
                                }
                                return None;
                            }
                        }
                        let tname_opt = if let Some(mty) = mock_types.get(&oid.sym) {
                            match mty {
                                Type::Ref(id) | Type::BoxDyn(id) => Some(id.sym.clone()),
                                Type::BorrowType { inner, .. } => {
                                    if let Type::Ref(id) | Type::BoxDyn(id) = &**inner {
                                        Some(id.sym.clone())
                                    } else {
                                        None
                                    }
                                }
                                _ => None,
                            }
                        } else {
                            self.resolve_var_type_name(&oid.sym).or_else(|| {
                                if self.class_defs.contains_key(oid_base) {
                                    Some(oid.sym.clone())
                                } else if self.class_defs.contains_key(resolved_base) {
                                    Some(resolved_base.to_string())
                                } else {
                                    None
                                }
                            })
                        };
                        if let Some(tname) = tname_opt {
                            if let Some((_, Some(rt), _)) =
                                self.lookup_method(&tname, &mem.prop.sym)
                            {
                                if self.in_try_region {
                                    if let Type::ResultType { ok, .. } = &rt {
                                        return Some((**ok).clone());
                                    }
                                }
                                return Some(rt);
                            }
                        }
                        if mem.prop.sym == "get" {
                            if let Some(mut oty) =
                                self.infer_cp_type_with_mock(&mem.obj, mock_types)
                            {
                                while let Type::BorrowType { inner, .. } = oty {
                                    oty = *inner;
                                }
                                if let Type::Map(_, val_ty) | Type::Dict(_, val_ty) = oty {
                                    return Some(Type::Optional(val_ty));
                                }
                            }
                            return None;
                        }
                        if mem.prop.sym == "push"
                            || mem.prop.sym == "clear"
                            || mem.prop.sym == "put_all"
                        {
                            return Some(Type::Void);
                        }
                        if mem.prop.sym == "has"
                            || mem.prop.sym == "delete"
                            || mem.prop.sym == "includes"
                        {
                            return Some(Type::Bool);
                        }
                        if mem.prop.sym == "pop" {
                            if let Some(mut oty) =
                                self.infer_cp_type_with_mock(&mem.obj, mock_types)
                            {
                                while let Type::BorrowType { inner, .. } = oty {
                                    oty = *inner;
                                }
                                if let Type::Array(elem) = oty {
                                    return Some(Type::Optional(elem));
                                }
                            }
                        }
                        if mem.prop.sym == "getField" {
                            return Some(Type::Optional(Box::new(Type::Ref(Ident {
                                span: Span { start: 0, end: 0 },
                                sym: "FieldInfo".to_string(),
                            }))));
                        }
                        if mem.prop.sym == "getMethod" {
                            return Some(Type::Optional(Box::new(Type::Ref(Ident {
                                span: Span { start: 0, end: 0 },
                                sym: "MethodInfo".to_string(),
                            }))));
                        }
                    }
                }
                if let Some(mut callee_ty) = self.infer_cp_type_with_mock(&c.callee, mock_types) {
                    while let Type::BorrowType { inner, .. } = callee_ty {
                        callee_ty = *inner;
                    }
                    if let Type::Ref(rid) = &callee_ty {
                        if let Some(target) = self.type_aliases.get(&rid.sym) {
                            callee_ty = target.target_ty.clone();
                        }
                    }
                    if let Type::Fn(_, ret) | Type::CFn(_, ret) = callee_ty {
                        return Some(*ret);
                    }
                }
                None
            }
            Expr::Member(mem) => {
                if mem.prop.sym == "length" || mem.prop.sym == "len" {
                    return Some(Type::USize);
                }
                if let Some(mut obj_ty) = self.infer_cp_type_with_mock(&mem.obj, mock_types) {
                    while let Type::BorrowType { inner, .. } = obj_ty {
                        obj_ty = *inner;
                    }
                    while let Type::RawPtr { inner, .. } = obj_ty {
                        obj_ty = *inner;
                    }
                    while let Type::Shared(inner) = obj_ty {
                        obj_ty = *inner;
                    }
                    if mem.prop.sym == "size"
                        && matches!(obj_ty, Type::Map(_, _) | Type::Dict(_, _) | Type::Set(_))
                    {
                        return Some(Type::I32);
                    }
                    if let Type::Object(fields) = &obj_ty {
                        if let Some(f) = fields.iter().find(|f| f.ident.sym == mem.prop.sym) {
                            return Some(f.ty.clone());
                        }
                    }
                    if let Type::Ref(id) = &obj_ty {
                        if self.enum_defs.contains_key(&id.sym) {
                            if mem.prop.sym == "name" {
                                return Some(Type::String);
                            }
                            if mem.prop.sym == "value" {
                                return Some(Type::I32);
                            }
                        }
                        let tname = &id.sym;
                        let guard_inner = if tname.starts_with("LockGuard<") {
                            Some(&tname[10..tname.len() - 1])
                        } else if tname.starts_with("ReadGuard<") {
                            Some(&tname[10..tname.len() - 1])
                        } else if tname.starts_with("WriteGuard<") {
                            Some(&tname[11..tname.len() - 1])
                        } else {
                            None
                        };
                        if let Some(inner_cls) = guard_inner {
                            if let Some(cls) = self.class_defs.get(inner_cls) {
                                if let Some(f) =
                                    cls.fields.iter().find(|f| f.ident.sym == mem.prop.sym)
                                {
                                    return Some(f.ty.clone());
                                }
                            }
                        }
                        if let Some(cls) = self.class_defs.get(tname) {
                            if let Some(f) = cls.fields.iter().find(|f| f.ident.sym == mem.prop.sym)
                            {
                                return Some(f.ty.clone());
                            }
                        }
                        if tname == "TypeInfo" {
                            if mem.prop.sym == "fields" {
                                return Some(Type::Array(Box::new(Type::Ref(Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "FieldInfo".to_string(),
                                }))));
                            }
                            if mem.prop.sym == "methods" {
                                return Some(Type::Array(Box::new(Type::Ref(Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "MethodInfo".to_string(),
                                }))));
                            }
                            if mem.prop.sym == "name" {
                                return Some(Type::String);
                            }
                            if mem.prop.sym == "isRef" {
                                return Some(Type::Bool);
                            }
                            if mem.prop.sym == "meta" {
                                return Some(Type::Ref(Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "Map".to_string(),
                                }));
                            }
                        }
                        if tname == "FieldInfo" {
                            if mem.prop.sym == "name" || mem.prop.sym == "typeName" {
                                return Some(Type::String);
                            }
                            if mem.prop.sym == "meta" {
                                return Some(Type::Ref(Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "Map".to_string(),
                                }));
                            }
                        }
                        if tname == "MethodInfo" {
                            if mem.prop.sym == "name" || mem.prop.sym == "returnTypeName" {
                                return Some(Type::String);
                            }
                            if mem.prop.sym == "isAsync" || mem.prop.sym == "isStatic" {
                                return Some(Type::Bool);
                            }
                            if mem.prop.sym == "paramNames" {
                                return Some(Type::Array(Box::new(Type::String)));
                            }
                            if mem.prop.sym == "meta" {
                                return Some(Type::Ref(Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "Map".to_string(),
                                }));
                            }
                        }
                    }
                }
                if let Expr::Ident(oid) = &*mem.obj {
                    if let Some(mty) = mock_types.get(&oid.sym) {
                        let tname_opt = match mty {
                            Type::Ref(id) | Type::BoxDyn(id) => Some(id.sym.clone()),
                            Type::BorrowType { inner, .. } => {
                                if let Type::Ref(id) | Type::BoxDyn(id) = &**inner {
                                    Some(id.sym.clone())
                                } else {
                                    None
                                }
                            }
                            _ => None,
                        };
                        if let Some(tname) = tname_opt {
                            if let Some(cls) = self.class_defs.get(&tname) {
                                if let Some(f) =
                                    cls.fields.iter().find(|f| f.ident.sym == mem.prop.sym)
                                {
                                    return Some(f.ty.clone());
                                }
                            }
                        }
                    }
                    if let Some(mut vty) = self.var_cp_types.get(&oid.sym) {
                        while let Type::BorrowType { inner, .. } = vty {
                            vty = inner;
                        }
                        if let Type::Object(fields) = vty {
                            if let Some(f) = fields.iter().find(|f| f.ident.sym == mem.prop.sym) {
                                return Some(f.ty.clone());
                            }
                        }
                    }
                    if let Some(cls) = self.class_defs.get(&oid.sym) {
                        if let Some(f) = cls.fields.iter().find(|f| f.ident.sym == mem.prop.sym) {
                            return Some(f.ty.clone());
                        }
                    }
                    if self.enum_defs.contains_key(&oid.sym)
                        || self.error_sets.contains_key(&oid.sym)
                    {
                        return Some(Type::Ref(oid.clone()));
                    }
                    if let Some(tname) = self.resolve_var_type_name(&oid.sym) {
                        if tname == "Error" {
                            if mem.prop.sym == "code" {
                                return Some(Type::I32);
                            }
                            if mem.prop.sym == "msg" {
                                return Some(Type::String);
                            }
                        }
                        if tname == "TypeInfo" {
                            if mem.prop.sym == "fields" {
                                return Some(Type::Array(Box::new(Type::Ref(Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "FieldInfo".to_string(),
                                }))));
                            }
                            if mem.prop.sym == "methods" {
                                return Some(Type::Array(Box::new(Type::Ref(Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "MethodInfo".to_string(),
                                }))));
                            }
                            if mem.prop.sym == "name" {
                                return Some(Type::String);
                            }
                            if mem.prop.sym == "isRef" {
                                return Some(Type::Bool);
                            }
                            if mem.prop.sym == "meta" {
                                return Some(Type::Ref(Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "Map".to_string(),
                                }));
                            }
                        }
                        if tname == "FieldInfo" {
                            if mem.prop.sym == "name" || mem.prop.sym == "typeName" {
                                return Some(Type::String);
                            }
                            if mem.prop.sym == "meta" {
                                return Some(Type::Ref(Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "Map".to_string(),
                                }));
                            }
                        }
                        if tname == "MethodInfo" {
                            if mem.prop.sym == "name" || mem.prop.sym == "returnTypeName" {
                                return Some(Type::String);
                            }
                            if mem.prop.sym == "isAsync" || mem.prop.sym == "isStatic" {
                                return Some(Type::Bool);
                            }
                            if mem.prop.sym == "paramNames" {
                                return Some(Type::Array(Box::new(Type::String)));
                            }
                            if mem.prop.sym == "meta" {
                                return Some(Type::Ref(Ident {
                                    span: Span { start: 0, end: 0 },
                                    sym: "Map".to_string(),
                                }));
                            }
                        }
                        if let Some(cls) = self.class_defs.get(&tname) {
                            if let Some(f) = cls.fields.iter().find(|f| f.ident.sym == mem.prop.sym)
                            {
                                return Some(f.ty.clone());
                            }
                        }
                    }
                }
                None
            }
            Expr::OptMember(mem) => {
                if let Some(obj_ty) = self.infer_cp_type_with_mock(&mem.obj, mock_types) {
                    let mut base_ty = &obj_ty;
                    while let Type::Optional(inner) | Type::BorrowType { inner, .. } = base_ty {
                        base_ty = inner;
                    }
                    if let Type::Ref(cid) = base_ty {
                        if let Some(cls) = self.class_defs.get(&cid.sym) {
                            if let Some(f) = cls.fields.iter().find(|f| f.ident.sym == mem.prop.sym)
                            {
                                return match &f.ty {
                                    Type::Optional(_) => Some(f.ty.clone()),
                                    other => Some(Type::Optional(Box::new(other.clone()))),
                                };
                            }
                        }
                    }
                }
                None
            }
            Expr::OptCall(call) => {
                if let Expr::Member(mem) | Expr::OptMember(mem) = &*call.callee {
                    if let Some(obj_ty) = self.infer_cp_type_with_mock(&mem.obj, mock_types) {
                        let mut base_ty = &obj_ty;
                        while let Type::Optional(inner) | Type::BorrowType { inner, .. } = base_ty {
                            base_ty = inner;
                        }
                        if let Type::Ref(cid) | Type::BoxDyn(cid) = base_ty {
                            if let Some((_, Some(rt), _)) =
                                self.lookup_method(&cid.sym, &mem.prop.sym)
                            {
                                return match rt {
                                    Type::Optional(_) => Some(rt),
                                    other => Some(Type::Optional(Box::new(other))),
                                };
                            }
                        }
                    }
                }
                None
            }
            Expr::Question(inner) => {
                if let Some(inner_ty) = self.infer_cp_type_with_mock(inner, mock_types) {
                    let fn_bubbles_null = self.try_catch_target.is_none()
                        && match &self.current_fn_ret_ty {
                            Some(Type::ResultType { ok: fn_ok, .. }) => {
                                matches!(&**fn_ok, Type::Optional(_))
                            }
                            Some(Type::Optional(_)) => true,
                            _ => false,
                        };
                    return match inner_ty {
                        Type::ResultType { ok, .. } => match *ok {
                            Type::Optional(opt_in) if fn_bubbles_null => Some(*opt_in),
                            other => Some(other),
                        },
                        Type::ErrorUnion(err_in) => match *err_in {
                            Type::Optional(opt_in) if fn_bubbles_null => Some(*opt_in),
                            other => Some(other),
                        },
                        Type::Optional(opt_in) => Some(*opt_in),
                        other => Some(other),
                    };
                }
                None
            }
            Expr::QuestionFallback {
                expr: inner,
                fallback,
                is_return_fallback,
            } => {
                if let Some(inner_ty) = self.infer_cp_type_with_mock(inner, mock_types) {
                    return match inner_ty {
                        Type::ResultType { ok, .. } => match *ok {
                            Type::Optional(opt_in) => Some(*opt_in),
                            other => Some(other),
                        },
                        Type::ErrorUnion(err_in) => match *err_in {
                            Type::Optional(opt_in) => Some(*opt_in),
                            other => Some(other),
                        },
                        Type::Optional(opt_in) => Some(*opt_in),
                        other => Some(other),
                    };
                }
                if !*is_return_fallback {
                    return self.infer_cp_type_with_mock(fallback, mock_types);
                }
                None
            }
            Expr::NullCoalesce { left, right } => {
                if let Some(l_ty) = self.infer_cp_type_with_mock(left, mock_types) {
                    if let Type::Optional(opt_in) = l_ty {
                        return Some(*opt_in);
                    }
                    return Some(l_ty);
                }
                self.infer_cp_type_with_mock(right, mock_types)
            }
            Expr::Catch { expr: inner, .. } => {
                if let Some(inner_ty) = self.infer_cp_type_with_mock(inner, mock_types) {
                    if let Type::ResultType { ok, .. } = inner_ty {
                        return Some(*ok);
                    }
                    if let Type::ErrorUnion(t) = inner_ty {
                        return Some(*t);
                    }
                    return Some(inner_ty);
                }
                None
            }
            Expr::Bin(bin) => {
                if bin.op == BinaryOp::Pipe {
                    let l_ty = self.infer_cp_type_with_mock(&bin.left, mock_types);
                    let transformed = transform_pipeline_rhs(&bin.right, "__pipe_mock");
                    let mut next_mocks = mock_types.clone();
                    if let Some(ty) = l_ty {
                        next_mocks.insert("__pipe_mock".to_string(), ty);
                    }
                    return self.infer_cp_type_with_mock(&transformed, &next_mocks);
                }
                if bin.op == BinaryOp::Add {
                    let l_ty = self.infer_cp_type_with_mock(&bin.left, mock_types);
                    let r_ty = self.infer_cp_type_with_mock(&bin.right, mock_types);
                    if matches!(l_ty, Some(Type::String)) || matches!(r_ty, Some(Type::String)) {
                        return Some(Type::String);
                    }
                    return l_ty.or(r_ty);
                }
                if matches!(
                    bin.op,
                    BinaryOp::Eq
                        | BinaryOp::NotEq
                        | BinaryOp::Lt
                        | BinaryOp::Gt
                        | BinaryOp::LtEq
                        | BinaryOp::GtEq
                        | BinaryOp::LogicalAnd
                        | BinaryOp::LogicalOr
                ) {
                    return Some(Type::Bool);
                }
                self.infer_cp_type_with_mock(&bin.left, mock_types)
            }
            Expr::Match(m) => {
                let disc_ty = self.infer_cp_type_with_mock(&m.disc, mock_types);
                for arm in &m.arms {
                    let mut arm_mocks = mock_types.clone();
                    match &arm.pat {
                        Pattern::TypeMatch { ident, ty } => {
                            arm_mocks.insert(ident.sym.clone(), ty.clone());
                        }
                        Pattern::Err(ip) => match &**ip {
                            Pattern::TypeMatch { ident, ty } => {
                                arm_mocks.insert(ident.sym.clone(), ty.clone());
                            }
                            Pattern::Binding { ident, subpat } => {
                                if let Pattern::Ident(tid) = &**subpat {
                                    arm_mocks.insert(ident.sym.clone(), Type::Ref(tid.clone()));
                                }
                            }
                            Pattern::Ident(id) => {
                                if let Some(Type::ResultType { err, .. }) = &disc_ty {
                                    arm_mocks
                                        .insert(id.sym.clone(), self.error_type_to_cp_type(err));
                                }
                            }
                            _ => {}
                        },
                        Pattern::Ident(id) => {
                            if let Some(Type::ResultType { ok, .. }) = &disc_ty {
                                arm_mocks.insert(id.sym.clone(), (**ok).clone());
                            } else if let Some(dt) = &disc_ty {
                                arm_mocks.insert(id.sym.clone(), dt.clone());
                            }
                        }
                        _ => {}
                    }
                    if let Stmt::Expr(e) = &*arm.body {
                        if let Some(t) = self.infer_cp_type_with_mock(e, &arm_mocks) {
                            return Some(t);
                        }
                        if let (Pattern::Ident(pid), Expr::Ident(eid)) = (&arm.pat, e) {
                            if pid.sym == eid.sym {
                                if let Some(Type::ResultType { ok, .. }) = &disc_ty {
                                    return match &**ok {
                                        Type::Optional(inner) => Some((**inner).clone()),
                                        other => Some(other.clone()),
                                    };
                                } else if let Some(Type::Optional(inner)) = &disc_ty {
                                    return Some((**inner).clone());
                                } else if let Some(dt) = &disc_ty {
                                    return Some(dt.clone());
                                }
                            }
                        }
                    } else if let Stmt::Block(b) = &*arm.body {
                        if let Some(Stmt::Expr(e)) = b.body.last() {
                            if let Some(t) = self.infer_cp_type_with_mock(e, mock_types) {
                                return Some(t);
                            }
                        }
                    }
                }
                None
            }
            Expr::Object(fields) => {
                if !fields.is_empty() && fields.iter().all(|f| f.key.is_none() && !f.is_spread) {
                    let mut tys = Vec::new();
                    for f in fields {
                        tys.push(
                            self.infer_cp_type_with_mock(&f.val, mock_types)
                                .unwrap_or(Type::Any),
                        );
                    }
                    return Some(Type::Tuple(tys));
                }
                let mut fs: Vec<Field> = Vec::new();
                for (idx, f) in fields.iter().enumerate() {
                    if f.is_spread {
                        if let Some(Type::Object(sub_fs)) =
                            self.infer_cp_type_with_mock(&f.val, mock_types)
                        {
                            for sf in sub_fs {
                                if let Some(pos) = fs
                                    .iter()
                                    .position(|existing| existing.ident.sym == sf.ident.sym)
                                {
                                    fs[pos] = sf;
                                } else {
                                    fs.push(sf);
                                }
                            }
                        }
                    } else if let Some(k) = &f.key {
                        let ty = self
                            .infer_cp_type_with_mock(&f.val, mock_types)
                            .unwrap_or(Type::Any);
                        let new_f = Field {
                            span: k.span,
                            ident: k.clone(),
                            ty,
                            meta: Vec::new(),
                            default: None,
                            is_pub: true,
                            is_static: false,
                        };
                        if let Some(pos) =
                            fs.iter().position(|existing| existing.ident.sym == k.sym)
                        {
                            fs[pos] = new_f;
                        } else {
                            fs.push(new_f);
                        }
                    } else {
                        let ty = self
                            .infer_cp_type_with_mock(&f.val, mock_types)
                            .unwrap_or(Type::Any);
                        let new_f = Field {
                            span: Span { start: 0, end: 0 },
                            ident: Ident {
                                span: Span { start: 0, end: 0 },
                                sym: idx.to_string(),
                            },
                            ty,
                            meta: Vec::new(),
                            default: None,
                            is_pub: true,
                            is_static: false,
                        };
                        fs.push(new_f);
                    }
                }
                Some(Type::Object(fs))
            }
            Expr::Array(items) => {
                if let Some(first) = items.first() {
                    if let Some(elem_ty) = self.infer_cp_type_with_mock(first, mock_types) {
                        return Some(Type::Array(Box::new(elem_ty)));
                    }
                }
                None
            }
            Expr::ArrayRepeat(val, count) => {
                let val_ty = self
                    .infer_cp_type_with_mock(val, mock_types)
                    .unwrap_or(Type::U8);
                if let Expr::Lit(Lit::Int(n)) = &**count {
                    Some(Type::FixedArray(
                        Box::new(val_ty),
                        ArraySize::Literal(*n as usize),
                    ))
                } else if let Expr::Ident(id) = &**count {
                    Some(Type::FixedArray(
                        Box::new(val_ty),
                        ArraySize::Ident(id.clone()),
                    ))
                } else {
                    Some(Type::Array(Box::new(val_ty)))
                }
            }
            Expr::Slice(s) => {
                let obj_ty = self.infer_cp_type_with_mock(&s.obj, mock_types)?;
                match obj_ty {
                    Type::ManyRawPtr(elem) => Some(Type::Array(elem)),
                    Type::FixedArray(elem, _) => Some(Type::Array(elem)),
                    Type::CStr => Some(Type::String),
                    Type::Ref(id) if id.sym == "cstring" || id.sym == "cstr" => {
                        Some(Type::Array(Box::new(Type::U8)))
                    }
                    other => Some(other),
                }
            }
            Expr::Dict(_) => Some(Type::Ref(Ident {
                span: Span { start: 0, end: 0 },
                sym: "Dict".to_string(),
            })),
            Expr::ComptimeMap {
                target_ty, entries, ..
            } => {
                let val_ty = if let Some(ty) = target_ty {
                    ty.clone()
                } else if let Some(first) = entries.first() {
                    self.infer_cp_type_with_mock(&first.val, mock_types)
                        .unwrap_or(Type::Any)
                } else {
                    Type::Any
                };
                Some(Type::Map(Box::new(Type::String), Box::new(val_ty)))
            }
            Expr::InlineAsm { .. } => Some(Type::Any),
            Expr::Index { obj, .. } => {
                let mut obj_ty = self.infer_cp_type_with_mock(obj, mock_types);
                while let Some(Type::BorrowType { inner, .. }) = obj_ty {
                    obj_ty = Some(*inner);
                }
                match obj_ty {
                    Some(Type::Dict(_, v)) | Some(Type::Map(_, v)) => Some(Type::Optional(v)),
                    Some(Type::Array(inner)) => Some(*inner),
                    Some(Type::FixedArray(inner, _)) => Some(*inner),
                    Some(Type::String) => Some(Type::U8),
                    Some(Type::CStr) => Some(Type::U8),
                    Some(Type::ManyRawPtr(inner)) | Some(Type::CPtr(inner)) => Some(*inner),
                    Some(Type::Ref(id)) if id.sym == "cstring" || id.sym == "cstr" => {
                        Some(Type::U8)
                    }
                    Some(Type::Ref(id)) if id.sym == "Dict" || id.sym == "Map" => {
                        Some(Type::Optional(Box::new(Type::Ref(Ident {
                            span: Span { start: 0, end: 0 },
                            sym: "Message".to_string(),
                        }))))
                    }
                    _ => None,
                }
            }
            Expr::ReflectType { .. } | Expr::ReflectOf { .. } => Some(Type::Ref(Ident {
                span: Span { start: 0, end: 0 },
                sym: "TypeInfo".to_string(),
            })),
            Expr::TypeVal { .. } => Some(Type::Ref(Ident {
                span: Span { start: 0, end: 0 },
                sym: "type".to_string(),
            })),
            Expr::Range { .. } => Some(Type::I32),
            Expr::ComptimeExpr(e) => self.infer_cp_type_with_mock(e, mock_types),
            Expr::Comptime(b) | Expr::Unsafe(b) | Expr::Arena(b) => {
                let mut local_mocks = mock_types.clone();
                for s in &b.body {
                    if let Stmt::Var(v) = s {
                        if let Pattern::Ident(id) = &v.pat {
                            let init_ty = v
                                .init
                                .as_ref()
                                .and_then(|init| self.infer_cp_type_with_mock(init, &local_mocks));
                            let vty = v.ty.clone().or(init_ty);
                            if let Some(t) = vty {
                                local_mocks.insert(id.sym.clone(), t);
                            }
                        }
                    }
                }
                if let Some(Stmt::Expr(e)) = b.body.last() {
                    return self.infer_cp_type_with_mock(e, &local_mocks);
                } else if let Some(Stmt::Return(r)) = b.body.last() {
                    if let Some(arg) = &r.arg {
                        return self.infer_cp_type_with_mock(arg, &local_mocks);
                    }
                }
                Some(Type::Void)
            }
            Expr::Unwrap(inner) => {
                let inner_ty = self.infer_cp_type_with_mock(inner, mock_types);
                match inner_ty {
                    Some(Type::Optional(unwrapped)) => Some(*unwrapped),
                    other => other,
                }
            }
            Expr::If { cons, alt, .. } => {
                if let Some(t) = self.infer_cp_type_with_mock(cons, mock_types) {
                    return Some(t);
                }
                if let Some(a) = alt {
                    return self.infer_cp_type_with_mock(a, mock_types);
                }
                None
            }
            _ => None,
        }
    }
}
