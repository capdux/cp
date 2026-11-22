use super::*;
use crate::ast::*;
use crate::diagnostic::{find_closest_match, Diagnostic, STANDARD_BUILTIN_IMPORTS};
use crate::Span;
use std::collections::HashMap;

impl Analyzer {
    pub(super) fn infer_type(&mut self, expr: &Expr) -> SemaResult<Type> {
        match expr {
            Expr::Lit(lit) => match lit {
                Lit::Int(_) => Ok(Type::I32),
                Lit::Float(_) => Ok(Type::F32),
                Lit::Bool(_) => Ok(Type::Bool),
                Lit::Str(_) => Ok(Type::String),
                Lit::Null => Ok(Type::Optional(Box::new(Type::Unknown))),
                Lit::Undefined => Ok(Type::Any),
            },
            Expr::Ident(ident) => {
                if ident.sym == "true" || ident.sym == "false" {
                    return Ok(Type::Bool);
                }
                if ident.sym == "null" {
                    return Ok(Type::Optional(Box::new(Type::Unknown)));
                }
                if ident.sym.starts_with('@')
                    || ident.sym == "super"
                    || ident.sym == "Box"
                    || ident.sym == "Err"
                    || ident.sym == "err"
                    || ident.sym == "Error"
                    || ident.sym == "reflect"
                    || ident.sym == "unreachable"
                    || ident.sym.starts_with("c_")
                    || ((ident.sym.starts_with('u') || ident.sym.starts_with('i'))
                        && ident.sym.len() > 1
                        && ident.sym[1..].chars().all(|c| c.is_ascii_digit()))
                    || matches!(
                        ident.sym.as_str(),
                        "u8" | "i8"
                            | "u16"
                            | "i16"
                            | "u32"
                            | "i32"
                            | "u64"
                            | "i64"
                            | "u128"
                            | "i128"
                            | "usize"
                            | "isize"
                            | "f32"
                            | "f64"
                            | "bool"
                            | "void"
                            | "anytype"
                            | "anyopaque"
                            | "u8x16"
                            | "u8x32"
                            | "u8x64"
                            | "i16x8"
                            | "i16x16"
                            | "i32x4"
                            | "i32x8"
                            | "u32x4"
                            | "u32x8"
                            | "f32x4"
                            | "f32x8"
                            | "f32x16"
                            | "f64x2"
                            | "f64x4"
                    )
                {
                    return Ok(Type::Any);
                }
                match self.resolve(&ident.sym) {
                    Some(SymbolKind::Var { ty, .. }) => Ok(ty.clone()),
                    Some(SymbolKind::Func { params, ret, .. }) => Ok(Type::Fn(
                        params.clone(),
                        Box::new(ret.clone().unwrap_or(Type::I32)),
                    )),
                    Some(SymbolKind::Class(c)) => Ok(Type::Ref(c.ident.clone())),
                    Some(SymbolKind::Trait(t)) => Ok(Type::Ref(t.ident.clone())),
                    Some(SymbolKind::Enum(e)) => Ok(Type::Ref(e.ident.clone())),
                    _ => {
                        if ident.sym == "this" || ident.sym == "_" {
                            Ok(Type::Any)
                        } else {
                            Err(SemaError::UndefinedSymbol(ident.sym.clone(), ident.span))
                        }
                    }
                }
            }
            Expr::Bin(bin) => {
                if bin.op == BinaryOp::Pipe {
                    let transformed = transform_pipeline_rhs(&bin.right, "__pipe_mock");
                    return self.infer_type(&transformed);
                }
                let left = self.infer_type(&bin.left)?;
                let right = if bin.op == BinaryOp::LogicalAnd {
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
                        let r = self.infer_type(&bin.right)?;
                        self.exit_scope();
                        r
                    } else {
                        self.infer_type(&bin.right)?
                    }
                } else {
                    self.infer_type(&bin.right)?
                };
                if bin.op == BinaryOp::Assign {
                    if let Expr::Ident(ident) = &*bin.left {
                        if ident.sym == "_" {
                            return Ok(right);
                        }
                        if let Some(SymbolKind::Var {
                            kind: VarDeclKind::Const,
                            ..
                        }) = self.resolve(&ident.sym)
                        {
                            return Err(SemaError::ConstAssignment(ident.sym.clone(), bin.span));
                        }
                    } else if let Expr::Index { obj, .. } = &*bin.left {
                        if let Expr::Ident(ident) = &**obj {
                            if let Some(SymbolKind::Var {
                                kind: VarDeclKind::Const,
                                ..
                            }) = self.resolve(&ident.sym)
                            {
                                return Err(SemaError::ConstAssignment(
                                    ident.sym.clone(),
                                    bin.span,
                                ));
                            }
                        }
                    }
                    if !self.can_coerce(&left, &right, Some(&*bin.right)) {
                        let mut can_assign_to_outer = false;
                        if let Expr::Ident(ident) = &*bin.left {
                            for scope in self.scopes.iter().rev().skip(1) {
                                if let Some(SymbolKind::Var { ty: outer_ty, kind }) =
                                    scope.get(&ident.sym)
                                {
                                    if *kind != VarDeclKind::Const
                                        && self.can_coerce(outer_ty, &right, Some(&*bin.right))
                                    {
                                        can_assign_to_outer = true;
                                        break;
                                    }
                                }
                            }
                        }
                        if !can_assign_to_outer {
                            return Err(SemaError::TypeMismatch(left, right, bin.span));
                        }
                    }
                    if let Expr::Ident(ident) = &*bin.left {
                        if let Some(scope) = self.scopes.last_mut() {
                            if let Some(SymbolKind::Var { ty, .. }) = scope.get_mut(&ident.sym) {
                                *ty = right.clone();
                            }
                        }
                    }
                    Ok(left)
                } else if bin.op == BinaryOp::Lt
                    || bin.op == BinaryOp::LtEq
                    || bin.op == BinaryOp::Gt
                    || bin.op == BinaryOp::GtEq
                    || bin.op == BinaryOp::NotEq
                    || bin.op == BinaryOp::Eq
                {
                    Ok(Type::Bool)
                } else if bin.op == BinaryOp::LogicalOr || bin.op == BinaryOp::LogicalAnd {
                    Ok(Type::Bool)
                } else if bin.op == BinaryOp::Add && (left == Type::String || right == Type::String)
                {
                    Ok(Type::String)
                } else if bin.op == BinaryOp::Concat {
                    if left == Type::String || right == Type::String {
                        Ok(Type::String)
                    } else if left != Type::Unknown {
                        Ok(left)
                    } else {
                        Ok(right)
                    }
                } else {
                    if bin.op == BinaryOp::Add || bin.op == BinaryOp::Sub {
                        let is_left_single_ptr = matches!(left, Type::RawPtr { .. });
                        let is_right_single_ptr = matches!(right, Type::RawPtr { .. });
                        if is_left_single_ptr || is_right_single_ptr {
                            return Err(SemaError::InvalidPointerArithmetic(
                                "Cannot perform pointer arithmetic on single-item pointer '*T!'; use '[*]T!' instead".to_string(),
                                bin.span,
                            ));
                        }
                    }
                    if !self.types_eq(&left, &right)
                        && !self.can_coerce(&left, &right, Some(&*bin.right))
                        && !self.can_coerce(&right, &left, Some(&*bin.left))
                    {
                        return Err(SemaError::TypeMismatch(left, right, bin.span));
                    }
                    Ok(left)
                }
            }
            Expr::Call(call) => {
                if let Expr::Ident(fid) = &*call.callee {
                    if fid.sym == "num" || fid.sym == "Number" {
                        return Err(SemaError::InvalidCast(
                            "CP does not support 'num()' or 'Number()'. CP is strongly typed: use specific numeric types like i32(x), u32(x), usize(x), or f64(x).".to_string(),
                            call.span,
                        ));
                    }
                    if fid.sym == "join" || fid.sym == "@task_join" {
                        let elem_tys = vec![Type::Any; call.args.len()];
                        return Ok(Type::Tuple(elem_tys));
                    }
                    if fid.sym == "spawn"
                        || fid.sym == "@task_spawn"
                        || fid.sym == "@task_spawn_cpu"
                    {
                        return Ok(Type::Ref(Ident {
                            span: fid.span,
                            sym: "Task".to_string(),
                        }));
                    }
                    if fid.sym == "@alloc" {
                        return Ok(Type::ManyRawPtr(Box::new(Type::U8)));
                    }
                    if let Some(SymbolKind::Func { ret: Some(ret), .. }) = self.resolve(&fid.sym) {
                        let ret_cloned = ret.clone();
                        if let Some(func_decl) = self.functions.get(&fid.sym).cloned() {
                            let mut subs = HashMap::new();
                            for (idx, p) in func_decl.params.iter().enumerate() {
                                if p.is_comptime
                                    && matches!(&p.ty, Type::Ref(pid) if pid.sym == "type")
                                {
                                    if let Some(arg_expr) = call.args.get(idx) {
                                        if let Some(arg_ty) = Self::expr_as_type_arg(arg_expr) {
                                            subs.insert(p.ident.sym.clone(), arg_ty);
                                        }
                                    }
                                }
                            }
                            if !subs.is_empty() {
                                return Ok(Self::substitute_type_params(&ret_cloned, &subs));
                            }
                        }
                        return Ok(ret_cloned);
                    }
                    if let Some(SymbolKind::Enum(_)) = self.resolve(&fid.sym) {
                        return Ok(Type::Ref(fid.clone()));
                    }
                    match fid.sym.as_str() {
                        "i8" => return Ok(Type::I8),
                        "u8" => return Ok(Type::U8),
                        "i16" => return Ok(Type::I16),
                        "u16" => return Ok(Type::U16),
                        "i32" => return Ok(Type::I32),
                        "u32" => return Ok(Type::U32),
                        "i64" => return Ok(Type::I64),
                        "u64" => return Ok(Type::U64),
                        "i128" => return Ok(Type::I128),
                        "u128" => return Ok(Type::U128),
                        "isize" => return Ok(Type::ISize),
                        "usize" => return Ok(Type::USize),
                        "f32" => return Ok(Type::F32),
                        "f64" => return Ok(Type::F64),
                        "bool" => return Ok(Type::Bool),
                        "string" | "String" => return Ok(Type::String),
                        _ => {}
                    }
                }
                if let Expr::Member(mem) = &*call.callee {
                    if mem.prop.sym == "toString" && call.args.is_empty() {
                        return Ok(Type::String);
                    }
                    if mem.prop.sym == "split" && call.args.len() == 1 {
                        let arg_is_int = matches!(&call.args[0], Expr::Lit(Lit::Int(_)))
                            || matches!(
                                self.infer_type(&call.args[0]),
                                Ok(Type::U8
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
                                    | Type::USize)
                            );
                        if arg_is_int {
                            if let Ok(obj_ty) = self.infer_type(&mem.obj) {
                                let mut cur = &obj_ty;
                                let mut is_const = false;
                                let mut is_fixed_arr = false;
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
                                        Type::Array(elem) => {
                                            elem_opt = Some(elem.clone());
                                            break;
                                        }
                                        Type::FixedArray(elem, _) => {
                                            is_fixed_arr = true;
                                            elem_opt = Some(elem.clone());
                                            break;
                                        }
                                        _ => break,
                                    }
                                }
                                if let Some(elem) = elem_opt {
                                    if is_fixed_arr && !is_const {
                                        if let Expr::Ident(id) = &*mem.obj {
                                            if let Some(SymbolKind::Var {
                                                kind: VarDeclKind::Const,
                                                ..
                                            }) = self.resolve(&id.sym)
                                            {
                                                is_const = true;
                                            }
                                        }
                                    }
                                    let slice_ty = if is_const {
                                        Type::BorrowType {
                                            inner: Box::new(Type::Array(elem)),
                                            is_mut: false,
                                            lifetimes: vec![],
                                        }
                                    } else {
                                        Type::Array(elem)
                                    };
                                    return Ok(Type::Tuple(vec![slice_ty.clone(), slice_ty]));
                                }
                            }
                        }
                    }
                    if let Expr::Ident(oid) = &*mem.obj {
                        if (oid.sym == "string" || oid.sym == "String") && mem.prop.sym == "from" {
                            return Ok(Type::String);
                        }
                        if oid.sym == "reflect" {
                            if mem.prop.sym == "hasMethod"
                                || mem.prop.sym == "hasField"
                                || mem.prop.sym == "has"
                            {
                                return Ok(Type::Bool);
                            }
                        }
                        if let Some(SymbolKind::Enum(e)) = self.resolve(&oid.sym) {
                            if let Some(m) = e.methods.iter().find(|m| m.ident.sym == mem.prop.sym)
                            {
                                return Ok(m.return_type.clone().unwrap_or(Type::Void));
                            }
                            if mem.prop.sym == "fromName"
                                || mem.prop.sym == "fromString"
                                || mem.prop.sym == "get"
                            {
                                return Ok(Type::Optional(Box::new(Type::Ref(oid.clone()))));
                            }
                            if mem.prop.sym == "has" {
                                return Ok(Type::Bool);
                            }
                        }
                        if oid.sym == "spawn" && mem.prop.sym == "cpu" {
                            return Ok(Type::Ref(Ident {
                                span: mem.prop.span,
                                sym: "Task".to_string(),
                            }));
                        }
                        if oid.sym == "JSON" {
                            if mem.prop.sym == "stringify" || mem.prop.sym == "encode" {
                                return Ok(Type::String);
                            }
                            if mem.prop.sym == "parse" || mem.prop.sym == "decode" {
                                if let Some(Expr::TypeVal { ty, .. }) = call.args.first() {
                                    return Ok(Type::ErrorUnion(Box::new(ty.clone())));
                                }
                                if let Some(Expr::Ident(id)) = call.args.first() {
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
                                    ) || self.classes.contains_key(&id.sym)
                                        || matches!(
                                            self.resolve(&id.sym),
                                            Some(SymbolKind::Class(_) | SymbolKind::Enum(_))
                                        )
                                        || id
                                            .sym
                                            .chars()
                                            .next()
                                            .map_or(false, |ch| ch.is_ascii_uppercase());
                                    if is_type {
                                        return Ok(Type::ErrorUnion(Box::new(Type::Ref(
                                            id.clone(),
                                        ))));
                                    }
                                }
                                return Ok(Type::ErrorUnion(Box::new(Type::Any)));
                            }
                        }
                    }
                }
                let mut callee_ty = self.infer_type(&call.callee)?;
                if let Type::Ref(id) = &callee_ty {
                    if let Some(target) = self.type_aliases.get(&id.sym).cloned() {
                        callee_ty = target;
                    }
                }
                while let Type::RawPtr { inner, .. } | Type::BorrowType { inner, .. } = &callee_ty {
                    callee_ty = *inner.clone();
                }
                match callee_ty {
                    Type::Fn(params, ret) => {
                        if let Type::Ref(rid) = &*ret {
                            for (idx, p) in params.iter().enumerate() {
                                let mut p_ty = p;
                                while let Type::BorrowType { inner, .. } = p_ty {
                                    p_ty = inner;
                                }
                                if let Type::Ref(pid) = p_ty {
                                    if pid.sym == rid.sym && idx < call.args.len() {
                                        if let Ok(arg_ty) = self.infer_type(&call.args[idx]) {
                                            return Ok(arg_ty);
                                        }
                                    }
                                }
                            }
                        }
                        Ok(*ret)
                    }
                    Type::CFn(_, ret) => Ok(*ret),
                    Type::Any | Type::Auto => Ok(Type::Any),
                    _ => Err(SemaError::NotAFunction(call.span)),
                }
            }
            Expr::OptCall(_) | Expr::OptMember(_) => Ok(Type::Optional(Box::new(Type::Any))),
            Expr::New(call) => {
                if let Expr::Ident(ident) = &*call.callee {
                    let base_sym = if let Some(pos) = ident.sym.find('<') {
                        &ident.sym[..pos]
                    } else {
                        &ident.sym
                    };
                    if self.classes.contains_key(base_sym)
                        || ident.sym == "Error"
                        || self.resolve(base_sym).is_some()
                    {
                        return Ok(Type::Ref(ident.clone()));
                    }
                    return Err(SemaError::UndefinedSymbol(base_sym.to_string(), ident.span));
                }
                Err(SemaError::UndefinedSymbol("Class".to_string(), call.span))
            }
            Expr::EnumInit(init) => {
                if !init.enum_name.sym.is_empty() {
                    Ok(Type::Ref(init.enum_name.clone()))
                } else {
                    Ok(Type::Any)
                }
            }
            Expr::Spawn(_) => Ok(Type::PID),
            Expr::Receive(ty) => Ok(ty.as_ref().map(|t| (**t).clone()).unwrap_or(Type::I32)),
            Expr::Slice(slice) => {
                let mut o_ty = self.infer_type(&slice.obj)?;
                while let Type::BorrowType { inner, .. } | Type::RawPtr { inner, .. } = o_ty {
                    o_ty = *inner;
                }
                let elem = match o_ty {
                    Type::ManyRawPtr(elem)
                    | Type::CPtr(elem)
                    | Type::SentinelPtr(elem, _)
                    | Type::SentinelSlice(elem, _)
                    | Type::Array(elem)
                    | Type::FixedArray(elem, _) => elem,
                    Type::CStr => Box::new(Type::U8),
                    Type::Ref(id) if id.sym == "cstring" || id.sym == "cstr" => Box::new(Type::U8),
                    Type::String => Box::new(Type::U8),
                    Type::Ref(id) if id.sym.ends_with("WPathBuffer") || id.sym.contains("u16") => {
                        Box::new(Type::U16)
                    }
                    _ => Box::new(Type::Auto),
                };
                if let Some(s_expr) = &slice.sentinel {
                    let sentinel_str = match &**s_expr {
                        Expr::Lit(Lit::Int(n)) => n.to_string(),
                        _ => "0".to_string(),
                    };
                    Ok(Type::SentinelSlice(elem, sentinel_str))
                } else {
                    Ok(Type::Array(elem))
                }
            }
            Expr::Question(inner) => {
                let inner_ty = self.infer_type(inner)?;
                match inner_ty {
                    Type::ResultType { ok, .. } => match *ok {
                        Type::Optional(t) => Ok(*t),
                        other => Ok(other),
                    },
                    Type::ErrorUnion(t) | Type::Optional(t) => Ok(*t),
                    _ => Ok(inner_ty),
                }
            }
            Expr::QuestionFallback { expr, fallback, .. }
            | Expr::NullCoalesce {
                left: expr,
                right: fallback,
            } => {
                let inner_ty = self.infer_type(expr)?;
                let fb_ty = self.infer_type(fallback)?;
                match inner_ty {
                    Type::ResultType { ok, .. } => match *ok {
                        Type::Optional(t) => Ok(*t),
                        other => Ok(other),
                    },
                    Type::ErrorUnion(t) | Type::Optional(t) => {
                        if *t == Type::Unknown || *t == Type::Any {
                            Ok(fb_ty)
                        } else {
                            Ok(*t)
                        }
                    }
                    _ => Ok(fb_ty),
                }
            }
            Expr::Catch { expr, .. } => {
                let inner_ty = self.infer_type(expr)?;
                match inner_ty {
                    Type::ResultType { ok, .. } => Ok(*ok),
                    Type::ErrorUnion(t) => Ok(*t),
                    _ => Ok(inner_ty),
                }
            }
            Expr::ErrWrap(_) | Expr::BoxNew(_) => Ok(Type::Any),
            Expr::Downcast {
                target_ty, is_mut, ..
            } => Ok(Type::Optional(Box::new(Type::BorrowType {
                inner: Box::new(Type::Ref(target_ty.clone())),
                is_mut: *is_mut,
                lifetimes: vec![],
            }))),
            Expr::Object(fields) => {
                if !fields.is_empty() && fields.iter().all(|f| f.key.is_none() && !f.is_spread) {
                    let mut tys = Vec::new();
                    for f in fields {
                        tys.push(self.infer_type(&f.val)?);
                    }
                    return Ok(Type::Tuple(tys));
                }
                let mut fs: Vec<Field> = Vec::new();
                for (idx, f) in fields.iter().enumerate() {
                    if f.is_spread {
                        let sp_ty = self.infer_type(&f.val)?;
                        if let Type::Object(sub_fs) = sp_ty {
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
                        let ty = self.infer_type(&f.val)?;
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
                        let ty = self.infer_type(&f.val)?;
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
                Ok(Type::Object(fs))
            }
            Expr::Dict(_) => Ok(Type::Dict(Box::new(Type::String), Box::new(Type::Any))),
            Expr::Index { obj, .. } => {
                let mut o_ty = self.infer_type(obj)?;
                while let Type::BorrowType { inner, .. } = o_ty {
                    o_ty = *inner;
                }
                match o_ty {
                    Type::Array(elem) => Ok(*elem),
                    Type::FixedArray(elem, _) => Ok(*elem),
                    Type::ManyRawPtr(elem)
                    | Type::CPtr(elem)
                    | Type::SentinelPtr(elem, _)
                    | Type::SentinelSlice(elem, _) => {
                        let mut elem_ty = *elem;
                        while let Type::BorrowType {
                            inner,
                            is_mut: false,
                            ..
                        }
                        | Type::Aligned { inner, .. } = elem_ty
                        {
                            elem_ty = *inner;
                        }
                        Ok(elem_ty)
                    }
                    Type::CStr => Ok(Type::U8),
                    Type::String => Ok(Type::U8),
                    Type::Ref(id) if id.sym == "cstring" || id.sym == "cstr" => Ok(Type::U8),
                    Type::Tuple(tys) => Ok(tys.into_iter().next().unwrap_or(Type::Any)),
                    Type::Map(_, v) | Type::Dict(_, v) => Ok(Type::Optional(v)),
                    _ => Ok(Type::Any),
                }
            }
            Expr::Array(elements) => {
                let mut tys = Vec::new();
                for el in elements {
                    tys.push(self.infer_type(el)?);
                }
                Ok(Type::Tuple(tys))
            }
            Expr::ArrayRepeat(val, count) => {
                let val_ty = self.infer_type(val)?;
                if let Expr::Lit(Lit::Int(n)) = &**count {
                    Ok(Type::FixedArray(
                        Box::new(val_ty),
                        ArraySize::Literal(*n as usize),
                    ))
                } else if let Expr::Ident(id) = &**count {
                    Ok(Type::FixedArray(
                        Box::new(val_ty),
                        ArraySize::Ident(id.clone()),
                    ))
                } else {
                    Ok(Type::Array(Box::new(val_ty)))
                }
            }
            Expr::Template(t) => {
                for e in &t.expressions {
                    let _ = self.infer_type(e)?;
                }
                Ok(Type::String)
            }
            Expr::Await(inner) => {
                if !self.current_fn_is_async {
                    return Err(SemaError::AwaitOutsideAsync(inner_span(inner)));
                }
                self.infer_type(inner)
            }
            Expr::Borrow(inner, is_mut) => {
                let inner_ty = self.infer_type(inner)?;
                Ok(Type::BorrowType {
                    inner: Box::new(inner_ty),
                    is_mut: *is_mut,
                    lifetimes: vec![],
                })
            }
            Expr::Move(inner) => self.infer_type(inner),
            Expr::Builtin(call) => match call.name.as_str() {
                "@print" | "@fs_write_file" | "@fs_mkdir" | "@fs_remove" | "@fs_copy"
                | "@net_send" | "@net_close" => Ok(Type::Void),
                "@fs_read_file" | "@fs_read_async" | "@net_read_async" | "@http_fetch" => {
                    Ok(Type::String)
                }
                "@fs_exists" => Ok(Type::Bool),
                "@net_connect" | "@net_listen" | "@net_accept_async" | "@net_write_async" => {
                    Ok(Type::I32)
                }
                "@self" => Ok(Type::PID),
                "@yield_to_beam" | "@json_parse" | "@alloc" => Ok(Type::Any),
                "@get_arena" | "@arena" => Ok(Type::Ref(Ident {
                    span: Span { start: 0, end: 0 },
                    sym: "Arena".to_string(),
                })),
                "@io_read_stdin_line" => Ok(Type::String),
                "@io_write_stdout" | "@io_write_stderr" | "@print_err" | "@free" => Ok(Type::Void),
                "@shared_tensor_init" => Ok(Type::Ref(Ident {
                    span: Span { start: 0, end: 0 },
                    sym: "SharedTensor".to_string(),
                })),
                "@extern" => {
                    if let Some(Expr::TypeVal { ty, .. }) = call.args.first() {
                        Ok(ty.clone())
                    } else {
                        Ok(Type::Any)
                    }
                }
                "@map" => {
                    if call.args.len() > 0 {
                        let inner = self.infer_type(&call.args[0])?;
                        Ok(inner)
                    } else {
                        Ok(Type::Unknown)
                    }
                }
                "@fma" => {
                    let is_num_lit = |e: &Expr| -> bool {
                        match e {
                            Expr::Lit(Lit::Float(_) | Lit::Int(_)) => true,
                            Expr::Neg(inner) => {
                                matches!(&**inner, Expr::Lit(Lit::Float(_) | Lit::Int(_)))
                            }
                            Expr::Bin(b) => {
                                b.op == BinaryOp::Sub
                                    && matches!(&*b.left, Expr::Lit(Lit::Int(0)))
                                    && matches!(&*b.right, Expr::Lit(Lit::Float(_) | Lit::Int(_)))
                            }
                            _ => false,
                        }
                    };
                    let mut fallback = Type::Unknown;
                    for arg in &call.args {
                        let ty = self.infer_type(arg).unwrap_or(Type::Unknown);
                        if !is_num_lit(arg) && matches!(ty, Type::F32 | Type::F64) {
                            return Ok(ty);
                        }
                        if fallback == Type::Unknown && ty != Type::Unknown {
                            fallback = ty;
                        }
                    }
                    Ok(fallback)
                }
                "@dot" | "@l2sq" => {
                    for arg in &call.args {
                        if let Ok(ty) = self.infer_type(arg) {
                            let mut cur = &ty;
                            loop {
                                match cur {
                                    Type::BorrowType { inner, .. } => cur = inner,
                                    Type::RawPtr { inner, depth: 1 } => cur = inner,
                                    Type::Array(elem) | Type::FixedArray(elem, _) => {
                                        return Ok((**elem).clone());
                                    }
                                    _ => break,
                                }
                            }
                        }
                    }
                    Ok(Type::Unknown)
                }
                "@splitAtMut" => {
                    if let Some(first) = call.args.first() {
                        if let Ok(ty) = self.infer_type(first) {
                            let mut cur = &ty;
                            loop {
                                match cur {
                                    Type::BorrowType { inner, .. } => cur = inner,
                                    Type::RawPtr { inner, depth: 1 } => cur = inner,
                                    Type::Array(elem) | Type::FixedArray(elem, _) => {
                                        let slice_ty = Type::Array(elem.clone());
                                        return Ok(Type::Tuple(vec![slice_ty.clone(), slice_ty]));
                                    }
                                    _ => break,
                                }
                            }
                        }
                    }
                    Ok(Type::Tuple(vec![Type::Unknown, Type::Unknown]))
                }
                _ => Ok(Type::Unknown),
            },
            Expr::ReflectType { span, .. } | Expr::ReflectOf { span, .. } => Ok(Type::Ref(Ident {
                span: *span,
                sym: "TypeInfo".to_string(),
            })),
            Expr::TypeVal { span, .. } => Ok(Type::Ref(Ident {
                span: *span,
                sym: "type".to_string(),
            })),
            Expr::Range { .. } => Ok(Type::I32),
            Expr::Cast {
                expr, target_ty, ..
            } => {
                if let Type::RawPtr { inner, depth } = target_ty {
                    if **inner == Type::Any {
                        if let Expr::Borrow(b, _) = &**expr {
                            let mut base_ty = self.infer_type(b)?;
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
                            return Ok(Type::RawPtr {
                                inner: Box::new(base_ty),
                                depth: *depth,
                            });
                        }
                    }
                }
                Ok(target_ty.clone())
            }
            Expr::Neg(inner) | Expr::BitNot(inner) => self.infer_type(inner),
            Expr::Deref(inner) => {
                let inner_ty = self.infer_type(inner)?;
                match inner_ty {
                    Type::RawPtr { inner, depth } => {
                        if depth > 1 {
                            Ok(Type::RawPtr {
                                inner,
                                depth: depth - 1,
                            })
                        } else {
                            Ok(*inner)
                        }
                    }
                    Type::BorrowType { inner, .. } => Ok(*inner),
                    other => Ok(other),
                }
            }
            Expr::ComptimeExpr(e) => self.infer_type(e),
            Expr::Unsafe(b) | Expr::Comptime(b) | Expr::Arena(b) => {
                if let Some(Stmt::Expr(e)) = b.body.last() {
                    return Ok(self.infer_type(e).unwrap_or(Type::Any));
                } else if let Some(Stmt::Return(r)) = b.body.last() {
                    if let Some(arg) = &r.arg {
                        return Ok(self.infer_type(arg).unwrap_or(Type::Any));
                    }
                }
                Ok(Type::Void)
            }
            Expr::Match(m) => {
                for arm in &m.arms {
                    let arm_ty = match &*arm.body {
                        Stmt::Expr(Expr::Return(..))
                        | Stmt::Expr(Expr::Break(..))
                        | Stmt::Expr(Expr::LabeledBreak(..))
                        | Stmt::Expr(Expr::Continue(..)) => Type::Void,
                        Stmt::Expr(Expr::Lit(Lit::Int(_))) => Type::Any,
                        Stmt::Expr(e) => self.infer_type(e).unwrap_or(Type::Any),
                        Stmt::Block(b) => {
                            if let Some(last) = b.body.last() {
                                match last {
                                    Stmt::Expr(Expr::Return(..))
                                    | Stmt::Expr(Expr::Break(..))
                                    | Stmt::Expr(Expr::LabeledBreak(..))
                                    | Stmt::Expr(Expr::Continue(..))
                                    | Stmt::Return(..)
                                    | Stmt::Break
                                    | Stmt::Continue => Type::Void,
                                    Stmt::Expr(e) => self.infer_type(e).unwrap_or(Type::Any),
                                    _ => Type::Void,
                                }
                            } else {
                                Type::Void
                            }
                        }
                        Stmt::Return(_) | Stmt::Break | Stmt::Continue => Type::Void,
                        _ => Type::Void,
                    };
                    if arm_ty != Type::Any && arm_ty != Type::Unknown && arm_ty != Type::Void {
                        return Ok(arm_ty);
                    }
                }
                Ok(Type::Any)
            }
            Expr::Member(mem) => {
                if mem.prop.sym == "toString" {
                    return Ok(Type::Fn(vec![], Box::new(Type::String)));
                }
                if let Expr::Ident(id) = &*mem.obj {
                    if let Some(SymbolKind::Enum(e)) = self.resolve(&id.sym) {
                        if let Some(m) = e.methods.iter().find(|m| m.ident.sym == mem.prop.sym) {
                            return Ok(Type::Fn(
                                m.params.iter().map(|p| p.ty.clone()).collect(),
                                Box::new(m.return_type.clone().unwrap_or(Type::Void)),
                            ));
                        }
                        return Ok(Type::Ref(id.clone()));
                    }
                }
                if let Ok(obj_ty) = self.infer_type(&mem.obj) {
                    let base_ty = match &obj_ty {
                        Type::BorrowType { inner, .. } | Type::RawPtr { inner, .. } => {
                            inner.as_ref()
                        }
                        other => other,
                    };
                    if let Type::Ref(id) = base_ty {
                        if let Some(SymbolKind::Enum(_)) = self.resolve(&id.sym) {
                            if mem.prop.sym == "name" {
                                return Ok(Type::String);
                            } else if mem.prop.sym == "value" {
                                return Ok(Type::I32);
                            }
                        }
                        let mut cur_cls_name = Some(id.sym.clone());
                        while let Some(cls_name) = cur_cls_name {
                            if let Some(cls) = self.classes.get(&cls_name) {
                                if let Some(field) =
                                    cls.fields.iter().find(|f| f.ident.sym == mem.prop.sym)
                                {
                                    return Ok(field.ty.clone());
                                }
                                cur_cls_name = cls.extends.as_ref().map(|e| e.sym.clone());
                            } else {
                                break;
                            }
                        }
                    } else if let Type::Object(fs) = base_ty {
                        if let Some(field) = fs.iter().find(|f| f.ident.sym == mem.prop.sym) {
                            return Ok(field.ty.clone());
                        }
                    } else if let Type::Tuple(tys) = base_ty {
                        if let Ok(idx) = mem.prop.sym.parse::<usize>() {
                            if let Some(t) = tys.get(idx) {
                                return Ok(t.clone());
                            }
                        } else if mem.prop.sym == "len" || mem.prop.sym == "length" {
                            return Ok(Type::USize);
                        }
                    } else if (*base_ty == Type::String
                        || *base_ty == Type::CStr
                        || matches!(
                            base_ty,
                            Type::Array(_)
                                | Type::FixedArray(_, _)
                                | Type::SentinelSlice(_, _)
                                | Type::SentinelPtr(_, _)
                        ))
                        && (mem.prop.sym == "len" || mem.prop.sym == "length")
                    {
                        return Ok(Type::USize);
                    }
                }
                Ok(Type::Any)
            }
            Expr::If {
                test, cons, alt, ..
            } => {
                let not_null_vars = Self::extract_not_null_vars(test);
                let is_null_vars = Self::extract_is_null_vars(test);
                let cons_ty = if !not_null_vars.is_empty() {
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
                    let res = self.infer_type(cons)?;
                    self.exit_scope();
                    res
                } else {
                    self.infer_type(cons)?
                };
                if let Some(a) = alt {
                    let alt_ty = if !is_null_vars.is_empty() {
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
                        let res = self.infer_type(a)?;
                        self.exit_scope();
                        res
                    } else {
                        self.infer_type(a)?
                    };
                    let is_lit = |e: &Expr| matches!(e, Expr::Lit(Lit::Int(_)));
                    if is_lit(&**cons) && is_lit(&**a) {
                        Ok(Type::Any)
                    } else if cons_ty == alt_ty {
                        Ok(cons_ty)
                    } else {
                        Ok(Type::Any)
                    }
                } else {
                    Ok(cons_ty)
                }
            }
            Expr::LabeledBlock { block, .. } => {
                for s in block.body.iter().rev() {
                    if let Some(val) = Self::find_break_value_in_stmt(s) {
                        return Ok(self.infer_type(val).unwrap_or(Type::Any));
                    }
                    if let Stmt::Expr(e) = s {
                        return Ok(self.infer_type(e).unwrap_or(Type::Any));
                    }
                }
                Ok(Type::Void)
            }
            Expr::While { stmt, else_expr } => {
                if let Some(val) = Self::find_break_value_in_stmt(&stmt.body) {
                    return Ok(self.infer_type(val).unwrap_or(Type::Any));
                }
                if let Some(e) = else_expr {
                    return Ok(self.infer_type(e).unwrap_or(Type::Any));
                }
                Ok(Type::Void)
            }
            _ => Ok(Type::Any),
        }
    }

    pub(super) fn find_break_value_in_stmt(s: &Stmt) -> Option<&Expr> {
        match s {
            Stmt::Expr(Expr::LabeledBreak(_, _, Some(val)))
            | Stmt::Expr(Expr::Break(_, Some(val))) => Some(val),
            Stmt::If(i) => Self::find_break_value_in_stmt(&i.cons).or_else(|| {
                i.alt
                    .as_ref()
                    .and_then(|a| Self::find_break_value_in_stmt(a))
            }),
            Stmt::Block(b) | Stmt::UnsafeBlock(b) => {
                for inner_s in b.body.iter().rev() {
                    if let Some(val) = Self::find_break_value_in_stmt(inner_s) {
                        return Some(val);
                    }
                }
                None
            }
            Stmt::While(w) => Self::find_break_value_in_stmt(&w.body),
            _ => None,
        }
    }

    pub(super) fn expr_as_type_arg(expr: &Expr) -> Option<Type> {
        match expr {
            Expr::Ident(id) => match id.sym.as_str() {
                "u8" => Some(Type::U8),
                "i8" => Some(Type::I8),
                "u16" => Some(Type::U16),
                "i16" => Some(Type::I16),
                "i32" => Some(Type::I32),
                "u32" => Some(Type::U32),
                "i64" => Some(Type::I64),
                "u64" => Some(Type::U64),
                "i128" => Some(Type::I128),
                "u128" => Some(Type::U128),
                "isize" => Some(Type::ISize),
                "usize" => Some(Type::USize),
                "f32" => Some(Type::F32),
                "f64" => Some(Type::F64),
                "bool" => Some(Type::Bool),
                "void" => Some(Type::Void),
                "string" => Some(Type::String),
                "cstr" => Some(Type::CStr),
                _ => Some(Type::Ref(id.clone())),
            },
            Expr::ReflectType { target_ty, .. } => Some(target_ty.clone()),
            _ => None,
        }
    }

    pub(super) fn substitute_type_params(ty: &Type, subs: &HashMap<String, Type>) -> Type {
        match ty {
            Type::Ref(id) => subs.get(&id.sym).cloned().unwrap_or_else(|| ty.clone()),
            Type::Optional(inner) => {
                Type::Optional(Box::new(Self::substitute_type_params(inner, subs)))
            }
            Type::ErrorUnion(inner) => {
                Type::ErrorUnion(Box::new(Self::substitute_type_params(inner, subs)))
            }
            Type::ResultType { ok, err } => Type::ResultType {
                ok: Box::new(Self::substitute_type_params(ok, subs)),
                err: err.clone(),
            },
            Type::Array(inner) => Type::Array(Box::new(Self::substitute_type_params(inner, subs))),
            Type::FixedArray(inner, len) => Type::FixedArray(
                Box::new(Self::substitute_type_params(inner, subs)),
                len.clone(),
            ),
            Type::RawPtr { inner, depth } => Type::RawPtr {
                inner: Box::new(Self::substitute_type_params(inner, subs)),
                depth: *depth,
            },
            Type::ManyRawPtr(inner) => {
                Type::ManyRawPtr(Box::new(Self::substitute_type_params(inner, subs)))
            }
            Type::CPtr(inner) => Type::CPtr(Box::new(Self::substitute_type_params(inner, subs))),
            Type::BorrowType {
                inner,
                is_mut,
                lifetimes,
            } => Type::BorrowType {
                inner: Box::new(Self::substitute_type_params(inner, subs)),
                is_mut: *is_mut,
                lifetimes: lifetimes.clone(),
            },
            Type::Aligned { qual, inner } => Type::Aligned {
                qual: qual.clone(),
                inner: Box::new(Self::substitute_type_params(inner, subs)),
            },
            Type::Fn(params, ret) => Type::Fn(
                params
                    .iter()
                    .map(|p| Self::substitute_type_params(p, subs))
                    .collect(),
                Box::new(Self::substitute_type_params(ret, subs)),
            ),
            Type::CFn(params, ret) => Type::CFn(
                params
                    .iter()
                    .map(|p| Self::substitute_type_params(p, subs))
                    .collect(),
                Box::new(Self::substitute_type_params(ret, subs)),
            ),
            other => other.clone(),
        }
    }
}
