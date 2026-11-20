use super::*;
use crate::ast::*;
use crate::diagnostic::{find_closest_match, Diagnostic, STANDARD_BUILTIN_IMPORTS};
use crate::Span;
use std::collections::HashMap;

impl Analyzer {
    pub(super) fn is_int_type(t: &Type) -> bool {
        match t {
            Type::Conditional(_, cons, alt) => Self::is_int_type(cons) && Self::is_int_type(alt),
            Type::U8
            | Type::I8
            | Type::U16
            | Type::I16
            | Type::I32
            | Type::U32
            | Type::I64
            | Type::U64
            | Type::U128
            | Type::I128
            | Type::ISize
            | Type::USize => true,
            Type::Ref(id) => {
                matches!(
                    id.sym.as_str(),
                    "c_int"
                        | "c_uint"
                        | "c_short"
                        | "c_ushort"
                        | "c_long"
                        | "c_ulong"
                        | "c_char"
                        | "c_uchar"
                        | "ares_socket_t"
                        | "ares_socklen_t"
                        | "ares_ssize_t"
                ) || ((id.sym.starts_with('u') || id.sym.starts_with('i'))
                    && id.sym.len() > 1
                    && id.sym[1..].chars().all(|c| c.is_ascii_digit()))
            }
            _ => false,
        }
    }

    pub(super) fn is_float_type(t: &Type) -> bool {
        matches!(t, Type::F32 | Type::F64)
    }

    pub(super) fn is_numeric_type(t: &Type) -> bool {
        Self::is_int_type(t) || Self::is_float_type(t)
    }

    pub(super) fn type_repr(t: &Type) -> String {
        match t {
            Type::U8 => "u8".to_string(),
            Type::I8 => "i8".to_string(),
            Type::U16 => "u16".to_string(),
            Type::I16 => "i16".to_string(),
            Type::U32 => "u32".to_string(),
            Type::I32 => "i32".to_string(),
            Type::U64 => "u64".to_string(),
            Type::I64 => "i64".to_string(),
            Type::U128 => "u128".to_string(),
            Type::I128 => "i128".to_string(),
            Type::USize => "usize".to_string(),
            Type::ISize => "isize".to_string(),
            Type::F32 => "f32".to_string(),
            Type::F64 => "f64".to_string(),
            Type::Bool => "bool".to_string(),
            Type::String => "string".to_string(),
            Type::Void => "void".to_string(),
            Type::Any => "any".to_string(),
            Type::Ref(id) => id.sym.clone(),
            Type::Conditional(_cond, cons, alt) => {
                format!(
                    "@if (...) {} else {}",
                    Self::type_repr(cons),
                    Self::type_repr(alt)
                )
            }
            _ => format!("{:?}", t),
        }
    }

    pub(super) fn int_fits_in_type(val: i64, target: &Type) -> bool {
        match target {
            Type::Conditional(_, cons, alt) => {
                Self::int_fits_in_type(val, cons) || Self::int_fits_in_type(val, alt)
            }
            Type::U8 => (0..=255).contains(&val),
            Type::I8 => (-128..=127).contains(&val),
            Type::U16 => (0..=65535).contains(&val),
            Type::I16 => (-32768..=32767).contains(&val),
            Type::U32 => val >= 0 && val <= u32::MAX as i64,
            Type::I32 => val >= i32::MIN as i64 && val <= i32::MAX as i64,
            Type::U64 | Type::U128 | Type::USize => true,
            Type::I64 | Type::I128 | Type::ISize => true,
            Type::F32 | Type::F64 => true,
            Type::Ref(id)
                if matches!(
                    id.sym.as_str(),
                    "c_int"
                        | "c_uint"
                        | "c_short"
                        | "c_ushort"
                        | "c_long"
                        | "c_ulong"
                        | "c_char"
                        | "c_uchar"
                        | "ares_socket_t"
                        | "ares_socklen_t"
                        | "ares_ssize_t"
                ) =>
            {
                true
            }
            Type::Ref(id)
                if (id.sym.starts_with('u') || id.sym.starts_with('i'))
                    && id.sym[1..].chars().all(|c| c.is_ascii_digit()) =>
            {
                if let Ok(bits) = id.sym[1..].parse::<u32>() {
                    if id.sym.starts_with('u') {
                        val >= 0 && (bits >= 64 || val < (1i64 << bits.min(62)))
                    } else {
                        bits >= 64
                            || (val >= -(1i64 << (bits - 1).min(62))
                                && val < (1i64 << (bits - 1).min(62)))
                    }
                } else {
                    true
                }
            }
            _ => false,
        }
    }

    pub(super) fn eval_const_int_expr(expr: &Expr) -> Option<i64> {
        match expr {
            Expr::Lit(Lit::Int(n)) => Some(*n),
            Expr::Bin(bin) => {
                let l = Self::eval_const_int_expr(&bin.left)?;
                let r = Self::eval_const_int_expr(&bin.right)?;
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

    pub(super) fn is_safe_widening(target: &Type, src: &Type) -> bool {
        if target == src {
            return true;
        }
        // Small unsigned -> larger unsigned
        match (src, target) {
            (Type::U8, Type::U16 | Type::U32 | Type::U64 | Type::U128 | Type::USize) => true,
            (Type::U16, Type::U32 | Type::U64 | Type::U128 | Type::USize) => true,
            (Type::U32, Type::U64 | Type::U128 | Type::USize) => true,
            (Type::U64, Type::U128) => true,
            (Type::U64, Type::USize) | (Type::USize, Type::U64) => true,

            // Small signed -> larger signed
            (Type::I8, Type::I16 | Type::I32 | Type::I64 | Type::I128 | Type::ISize) => true,
            (Type::I16, Type::I32 | Type::I64 | Type::I128 | Type::ISize) => true,
            (Type::I32, Type::I64 | Type::I128 | Type::ISize) => true,
            (Type::I64, Type::I128) => true,
            (Type::I64, Type::ISize) | (Type::ISize, Type::I64) => true,

            // Int / Float to larger Float
            (Type::I32, Type::F64) => true,
            (Type::F32, Type::F64) => true,
            (Type::I8 | Type::U8 | Type::I16 | Type::U16, Type::F32 | Type::F64) => true,

            _ => false,
        }
    }

    pub(super) fn is_subclass(&self, derived: &str, base: &str) -> bool {
        let mut curr = derived;
        while let Some(cls) = self.classes.get(curr) {
            if let Some(ref ext) = cls.extends {
                if ext.sym == base {
                    return true;
                }
                curr = &ext.sym;
            } else {
                break;
            }
        }
        false
    }

    pub(super) fn is_builtin_trait(name: &str) -> bool {
        matches!(name, "Comparable" | "ToString" | "Clone" | "Hash" | "Debug")
    }

    pub(super) fn is_primitive_scalar(ty: &Type) -> bool {
        matches!(
            ty,
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
                | Type::USize
                | Type::ISize
                | Type::F32
                | Type::F64
                | Type::Bool
                | Type::String
        )
    }

    pub fn check_trait_bound(&self, ty: &Type, bound_name: &str, span: Span) -> SemaResult<()> {
        if bound_name == "Any" {
            return Ok(());
        }

        let mut actual_ty = ty;
        while let Type::BorrowType { inner, .. } = actual_ty {
            actual_ty = inner;
        }

        // 1. If actual_ty is a generic type parameter in the current scope with this bound
        if let Type::Ref(id) = actual_ty {
            if let Some(bounds) = self.active_generic_bounds.get(&id.sym) {
                if bounds.iter().any(|b| b == bound_name) {
                    return Ok(());
                }
            }
        }

        // 2. Built-in traits inherently satisfied by primitive scalar types
        if Self::is_builtin_trait(bound_name) && Self::is_primitive_scalar(actual_ty) {
            return Ok(());
        }

        // 3. For classes/structs (Type::Ref)
        if let Type::Ref(id) = actual_ty {
            if let Some(cls) = self.classes.get(&id.sym) {
                // A. Explicit 'implements'
                if cls.implements.iter().any(|i| i.sym == bound_name) {
                    return Ok(());
                }

                // B. External 'impl Trait for Target'
                if self
                    .impls
                    .iter()
                    .any(|i| i.trait_name.sym == bound_name && i.target_name.sym == id.sym)
                {
                    return Ok(());
                }

                // C. Trait declared in code: check required methods
                if let Some(tr) = self.traits.get(bound_name) {
                    let mut missing = None;
                    for tm in &tr.methods {
                        if tm.default_body.is_none() {
                            let has_m = cls.methods.iter().any(|m| m.ident.sym == tm.ident.sym)
                                || self.impls.iter().any(|i| {
                                    i.trait_name.sym == bound_name
                                        && i.target_name.sym == id.sym
                                        && i.methods.iter().any(|m| m.ident.sym == tm.ident.sym)
                                });
                            if !has_m {
                                missing = Some(tm.ident.sym.clone());
                                break;
                            }
                        }
                    }
                    if missing.is_none() {
                        return Ok(());
                    } else {
                        return Err(SemaError::TraitBoundNotSatisfied {
                            type_name: id.sym.clone(),
                            trait_name: bound_name.to_string(),
                            missing_method: missing,
                            span,
                        });
                    }
                }

                // D. Built-in traits satisfied by method conventions on classes
                match bound_name {
                    "ToString"
                        if cls
                            .methods
                            .iter()
                            .any(|m| m.ident.sym == "toString" || m.ident.sym == "to_string") =>
                    {
                        return Ok(());
                    }
                    "Comparable"
                        if cls
                            .methods
                            .iter()
                            .any(|m| m.ident.sym == "compare" || m.ident.sym == "cmp") =>
                    {
                        return Ok(());
                    }
                    "Clone" if cls.methods.iter().any(|m| m.ident.sym == "clone") => {
                        return Ok(());
                    }
                    "Hash" if cls.methods.iter().any(|m| m.ident.sym == "hash") => {
                        return Ok(());
                    }
                    "Debug"
                        if cls
                            .methods
                            .iter()
                            .any(|m| m.ident.sym == "debug" || m.ident.sym == "format") =>
                    {
                        return Ok(());
                    }
                    _ => {}
                }
            }
        }

        // 4. Any / Auto / Unknown
        if matches!(actual_ty, Type::Any | Type::Auto | Type::Unknown) {
            return Ok(());
        }

        Err(SemaError::TraitBoundNotSatisfied {
            type_name: Self::type_repr(actual_ty),
            trait_name: bound_name.to_string(),
            missing_method: None,
            span,
        })
    }

    pub fn resolve_type(&self, ty: &Type) -> Type {
        let mut curr = ty.clone();
        for _ in 0..16 {
            match &curr {
                Type::Ref(id) => {
                    if let Some(target) = self.type_aliases.get(&id.sym) {
                        curr = target.clone();
                    } else {
                        break;
                    }
                }
                _ => break,
            }
        }
        curr
    }

    pub fn can_coerce(&self, target: &Type, src: &Type, init_expr: Option<&Expr>) -> bool {
        if target == src {
            return true;
        }
        if *target == Type::Any
            || *src == Type::Any
            || *target == Type::Unknown
            || *src == Type::Unknown
            || *target == Type::Auto
            || *src == Type::Auto
        {
            return true;
        }

        if let (Type::BorrowType { inner: t_inner, .. }, Type::BorrowType { inner: s_inner, .. }) =
            (target, src)
        {
            if self.can_coerce(t_inner, s_inner, init_expr) {
                return true;
            }
        }

        let resolved_target = self.resolve_type(target);
        let resolved_src = self.resolve_type(src);
        if &resolved_target != target || &resolved_src != src {
            if self.can_coerce(&resolved_target, &resolved_src, init_expr) {
                return true;
            }
        }

        if let Type::Conditional(_, cons, alt) = target {
            if self.can_coerce(cons, src, init_expr) || self.can_coerce(alt, src, init_expr) {
                return true;
            }
        }
        if let Type::Conditional(_, cons, alt) = src {
            if self.can_coerce(target, cons, init_expr) || self.can_coerce(target, alt, init_expr) {
                return true;
            }
        }

        // Layer 1.1: Literal and Const Expr fitting
        if let Some(expr) = init_expr {
            if let Some(val) = Self::eval_const_int_expr(expr) {
                if Self::int_fits_in_type(val, target) {
                    return true;
                }
            }
            match expr {
                Expr::Lit(lit) => match lit {
                    Lit::Int(val) => {
                        if Self::int_fits_in_type(*val, target) {
                            return true;
                        }
                    }
                    Lit::Float(_) => {
                        if matches!(target, Type::F32 | Type::F64) {
                            return true;
                        }
                    }
                    Lit::Null => {
                        if matches!(target, Type::Optional(_)) {
                            return true;
                        }
                    }
                    Lit::Str(_) => {
                        if matches!(target, Type::CStr)
                            || matches!(target, Type::Ref(id) if id.sym == "cstring" || id.sym == "cstr")
                            || matches!(target, Type::Array(inner) if **inner == Type::U8)
                            || matches!(target, Type::SentinelSlice(inner, _) if **inner == Type::U8)
                        {
                            return true;
                        }
                    }
                    _ => {}
                    _ => {}
                },
                Expr::NullCoalesce { right, .. } => {
                    if self.can_coerce(target, src, Some(right)) {
                        return true;
                    }
                }
                Expr::QuestionFallback {
                    fallback: right,
                    is_return_fallback,
                    ..
                } => {
                    if *is_return_fallback {
                        return true;
                    }
                    if self.can_coerce(target, src, Some(right)) {
                        return true;
                    }
                }
                _ => {}
            }
        }

        // Layer 1.2: Numeric safe widening
        if Self::is_safe_widening(target, src) {
            return true;
        }

        if (*target == Type::String && matches!(src, Type::Array(inner) if **inner == Type::U8))
            || (*src == Type::String && matches!(target, Type::Array(inner) if **inner == Type::U8))
            || (*target == Type::String
                && matches!(src, Type::BorrowType { inner, is_mut: false, .. } if matches!(&**inner, Type::Array(elem) if **elem == Type::U8)))
            || (*src == Type::String
                && matches!(target, Type::BorrowType { inner, is_mut: false, .. } if matches!(&**inner, Type::Array(elem) if **elem == Type::U8)))
            || (*target == Type::String
                && matches!(src, Type::SentinelSlice(inner, _) if **inner == Type::U8))
            || (*src == Type::String
                && matches!(target, Type::SentinelSlice(inner, _) if **inner == Type::U8))
        {
            return true;
        }
        if let Type::BorrowType {
            inner: t_inner,
            is_mut: false,
            ..
        } = target
        {
            if matches!(&**t_inner, Type::Array(_)) && self.can_coerce(t_inner, src, init_expr) {
                return true;
            }
        }

        if let Type::Ref(id) = target {
            if matches!(
                id.sym.as_str(),
                "c_int"
                    | "c_uint"
                    | "c_short"
                    | "c_ushort"
                    | "c_long"
                    | "c_ulong"
                    | "c_char"
                    | "c_uchar"
                    | "ares_socket_t"
                    | "ares_socklen_t"
                    | "ares_ssize_t"
            ) || ((id.sym.starts_with('u') || id.sym.starts_with('i'))
                && id.sym.len() > 1
                && id.sym[1..].chars().all(|c| c.is_ascii_digit()))
            {
                if Self::is_int_type(src) {
                    return true;
                }
            }
        }
        if let Type::Ref(id) = src {
            if matches!(
                id.sym.as_str(),
                "c_int"
                    | "c_uint"
                    | "c_short"
                    | "c_ushort"
                    | "c_long"
                    | "c_ulong"
                    | "c_char"
                    | "c_uchar"
                    | "ares_socket_t"
                    | "ares_socklen_t"
                    | "ares_ssize_t"
            ) {
                if Self::is_int_type(target) {
                    return true;
                }
            }
        }

        if (matches!(target, Type::CStr)
            || matches!(target, Type::Ref(id) if id.sym == "cstring" || id.sym == "cstr"))
            && (matches!(src, Type::String)
                || matches!(src, Type::CStr)
                || matches!(src, Type::Ref(id) if id.sym == "cstring" || id.sym == "cstr"))
        {
            return true;
        }
        if matches!(target, Type::String)
            && (matches!(src, Type::CStr)
                || matches!(src, Type::Ref(id) if id.sym == "cstring" || id.sym == "cstr")
                || matches!(src, Type::Array(inner) if **inner == Type::U8)
                || matches!(src, Type::FixedArray(inner, _) if **inner == Type::U8))
        {
            return true;
        }

        if (matches!(target, Type::CStr)
            || matches!(target, Type::Ref(id) if id.sym == "cstring" || id.sym == "cstr"))
        {
            if let Type::BorrowType { inner, .. } = src {
                if matches!(**inner, Type::U8 | Type::I8) {
                    return true;
                }
            }
            if let Type::ManyRawPtr(inner) = src {
                if matches!(**inner, Type::U8 | Type::I8) {
                    return true;
                }
            }
        }

        // Layer 1.3: Structural wrapping (Optional, ErrorUnion, ResultType)
        match target {
            Type::Optional(t_inner) => {
                if src == &Type::Optional(Box::new(Type::Unknown)) {
                    return true;
                }
                if let Type::Optional(s_inner) = src {
                    return self.can_coerce(t_inner, s_inner, None);
                }
                return self.can_coerce(t_inner, src, init_expr);
            }
            Type::ErrorUnion(t_inner) => {
                if let Type::ErrorUnion(s_inner) = src {
                    return self.can_coerce(t_inner, s_inner, None);
                }
                return self.can_coerce(t_inner, src, init_expr);
            }
            Type::ResultType { ok: t_ok, .. } => {
                if let Type::ResultType { ok: s_ok, .. } = src {
                    return self.can_coerce(t_ok, s_ok, None);
                }
                return self.can_coerce(t_ok, src, init_expr);
            }
            _ => {}
            _ => {}
        }

        // Layer 1.4: Borrow tightening (*T -> &T) or same mutability
        if let (
            Type::BorrowType {
                inner: t_inner,
                is_mut: t_mut,
                ..
            },
            Type::BorrowType {
                inner: s_inner,
                is_mut: s_mut,
                ..
            },
        ) = (target, src)
        {
            if (*t_mut == *s_mut || (!*t_mut && *s_mut)) && self.can_coerce(t_inner, s_inner, None)
            {
                return true;
            }
        }

        // Layer 1.5: Subclass to base class or matching type ref
        if let (Type::Ref(base), Type::Ref(derived)) = (target, src) {
            if base.sym == derived.sym || self.is_subclass(&derived.sym, &base.sym) {
                return true;
            }
        }

        // @Vector(N, T) coercion from fixed array / array literal / tuple
        if let Type::Ref(id) = target {
            if id.sym.starts_with("@Vector(")
                && matches!(
                    src,
                    Type::FixedArray(_, _) | Type::Array(_) | Type::Tuple(_)
                )
            {
                return true;
            }
        }
        if let Type::Ref(id) = src {
            if id.sym.starts_with("@Vector(")
                && matches!(
                    target,
                    Type::FixedArray(_, _) | Type::Array(_) | Type::Tuple(_)
                )
            {
                return true;
            }
        }

        // Object literal initializing a struct/class reference
        if let Type::Ref(_) = target {
            if matches!(src, Type::Object(_)) || matches!(init_expr, Some(Expr::Object(_))) {
                return true;
            }
        }

        // Raw pointers (same depth and coerceable inner, allowing *T! -> *const T!)
        if let (
            Type::RawPtr {
                inner: t_inner,
                depth: t_d,
            },
            Type::RawPtr {
                inner: s_inner,
                depth: s_d,
            },
        ) = (target, src)
        {
            if t_d == s_d {
                if let Type::BorrowType {
                    inner: t_base,
                    is_mut: false,
                    ..
                } = &**t_inner
                {
                    let s_base = match &**s_inner {
                        Type::BorrowType { inner, .. } => &**inner,
                        other => other,
                    };
                    if self.can_coerce(t_base, s_base, None) {
                        return true;
                    }
                } else if !matches!(&**s_inner, Type::BorrowType { is_mut: false, .. })
                    && self.can_coerce(t_inner, s_inner, None)
                {
                    return true;
                }
            }
        }
        // Coerce between Type::Ref("[N]T") and Type::FixedArray(T, Literal(N))
        if let (Type::Ref(id), Type::FixedArray(elem, ArraySize::Literal(n))) = (target, src) {
            if id.sym.starts_with('[') {
                if let Some(idx) = id.sym.find(']') {
                    let len_str = &id.sym[1..idx];
                    let elem_name = id.sym[idx + 1..].trim_start_matches("const ").trim();
                    if let Ok(target_len) = len_str.parse::<usize>() {
                        if target_len == *n {
                            let target_elem =
                                crate::codegen::Codegen::type_from_name_static(elem_name);
                            let elem_expr = match init_expr {
                                Some(Expr::ArrayRepeat(val, _)) => Some(&**val),
                                Some(Expr::Array(elements)) if !elements.is_empty() => {
                                    Some(&elements[0])
                                }
                                _ => None,
                            };
                            if **elem == target_elem
                                || self.can_coerce(&target_elem, elem, elem_expr)
                            {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        if let (Type::FixedArray(elem, ArraySize::Literal(n)), Type::Ref(id)) = (target, src) {
            if id.sym.starts_with('[') {
                if let Some(idx) = id.sym.find(']') {
                    let len_str = &id.sym[1..idx];
                    let elem_name = id.sym[idx + 1..].trim_start_matches("const ").trim();
                    if let Ok(src_len) = len_str.parse::<usize>() {
                        if src_len == *n {
                            let target_elem =
                                crate::codegen::Codegen::type_from_name_static(elem_name);
                            if **elem == target_elem || self.can_coerce(elem, &target_elem, None) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        if let (
            Type::CPtr(t_inner) | Type::ManyRawPtr(t_inner),
            Type::CPtr(s_inner) | Type::ManyRawPtr(s_inner),
        ) = (target, src)
        {
            if self.can_coerce(t_inner, s_inner, None) {
                return true;
            }
        }
        if let Type::CPtr(t_inner) | Type::ManyRawPtr(t_inner) = target {
            if let Type::BorrowType { inner: s_inner, .. } = src {
                if self.can_coerce(t_inner, s_inner, None) {
                    return true;
                }
            }
            if let Type::RawPtr { inner: s_inner, .. } = src {
                if self.can_coerce(t_inner, s_inner, None) {
                    return true;
                }
            }
        }
        if let Type::Array(t_inner) = target {
            if let Type::BorrowType { inner: s_inner, .. } = src {
                if let Type::Tuple(s_elems) = &**s_inner {
                    if s_elems.is_empty() {
                        return true;
                    }
                }
                if self.can_coerce(&Type::Array(t_inner.clone()), s_inner, init_expr) {
                    return true;
                }
            }
        }
        if let (Type::SentinelPtr(t_inner, t_s), Type::SentinelPtr(s_inner, s_s)) = (target, src) {
            if t_s == s_s && self.can_coerce(t_inner, s_inner, None) {
                return true;
            }
        }
        if let (Type::SentinelSlice(t_inner, t_s), Type::SentinelSlice(s_inner, s_s)) =
            (target, src)
        {
            let t_clean = t_s.split(':').next().unwrap_or(t_s);
            let s_clean = s_s.split(':').next().unwrap_or(s_s);
            if t_clean == s_clean && self.can_coerce(t_inner, s_inner, None) {
                return true;
            }
        }
        if let Type::SentinelSlice(t_inner, t_s) = target {
            let t_clean = t_s.split(':').next().unwrap_or(t_s);
            if **t_inner == Type::U8 && t_clean == "0" && src == &Type::String {
                return true;
            }
        }
        if let Type::Array(t_inner) = target {
            if let Type::SentinelSlice(s_inner, _) = src {
                if self.can_coerce(t_inner, s_inner, None) {
                    return true;
                }
            }
        }
        if let Type::SentinelSlice(_, _) = target {
            if let Type::BorrowType { inner, .. } = src {
                if self.can_coerce(target, inner, init_expr) {
                    return true;
                }
            }
        }
        if let Type::BorrowType { inner, .. } = target {
            if let Type::SentinelSlice(_, _) = src {
                if self.can_coerce(inner, src, init_expr) {
                    return true;
                }
            }
        }
        if let (Type::ManyRawPtr(t_inner), Type::CPtr(s_inner))
        | (Type::CPtr(t_inner), Type::ManyRawPtr(s_inner)) = (target, src)
        {
            if self.can_coerce(t_inner, s_inner, None) {
                return true;
            }
        }

        // Layer 1.6: Struct to Trait (&dyn Trait)
        if let Type::BoxDyn(t) | Type::BareDyn(t) = target {
            if let Type::Ref(s) = src {
                if let Some(cls) = self.classes.get(&s.sym) {
                    if cls.implements.iter().any(|i| i.sym == t.sym) {
                        return true;
                    }
                }
            }
            if let Type::BorrowType { inner, .. } = src {
                if let Type::Ref(s) = &**inner {
                    if let Some(cls) = self.classes.get(&s.sym) {
                        if cls.implements.iter().any(|i| i.sym == t.sym) {
                            return true;
                        }
                    }
                }
            }
        }

        // Containers
        match (target, src) {
            (Type::Array(t_inner), Type::Array(s_inner)) => {
                if let Some(Expr::Array(elements)) = init_expr {
                    elements
                        .iter()
                        .all(|el| self.can_coerce(t_inner, s_inner, Some(el)))
                } else if let Some(Expr::ArrayRepeat(val, _)) = init_expr {
                    self.can_coerce(t_inner, s_inner, Some(val))
                } else {
                    self.can_coerce(t_inner, s_inner, None)
                }
            }
            (Type::FixedArray(t_elem, t_len), Type::FixedArray(s_elem, s_len)) => {
                let same_len = match (t_len, s_len) {
                    (ArraySize::Literal(a), ArraySize::Literal(b)) => a == b,
                    (ArraySize::Ident(a), ArraySize::Ident(b)) => a.sym == b.sym,
                    (ArraySize::Expr(a), ArraySize::Expr(b)) => a == b,
                    _ => t_len == s_len,
                };
                if !same_len {
                    return false;
                }
                if let Some(Expr::Array(elements)) = init_expr {
                    elements
                        .iter()
                        .all(|el| self.can_coerce(t_elem, s_elem, Some(el)))
                } else if let Some(Expr::ArrayRepeat(val, _)) = init_expr {
                    self.can_coerce(t_elem, s_elem, Some(val))
                } else {
                    self.can_coerce(t_elem, s_elem, None)
                }
            }
            (Type::FixedArray(elem, _), Type::Array(t2))
            | (Type::Array(t2), Type::FixedArray(elem, _)) => {
                if let Some(Expr::Array(elements)) = init_expr {
                    elements
                        .iter()
                        .all(|el| self.can_coerce(elem, t2, Some(el)))
                } else if let Some(Expr::ArrayRepeat(val, _)) = init_expr {
                    self.can_coerce(elem, t2, Some(val))
                } else {
                    self.can_coerce(elem, t2, None)
                }
            }
            (Type::FixedArray(t_elem, t_len), Type::Tuple(s_elems)) => {
                if t_len.as_usize() != Some(s_elems.len()) {
                    return false;
                }
                if let Some(Expr::Array(elements)) = init_expr {
                    elements
                        .iter()
                        .zip(s_elems)
                        .all(|(el, s)| self.can_coerce(t_elem, s, Some(el)))
                } else {
                    s_elems.iter().all(|s| self.can_coerce(t_elem, s, None))
                }
            }
            (Type::Array(t_elem), Type::Tuple(s_elems)) => {
                if let Some(Expr::Array(elements)) = init_expr {
                    elements
                        .iter()
                        .zip(s_elems)
                        .all(|(el, s)| self.can_coerce(t_elem, s, Some(el)))
                } else {
                    s_elems.iter().all(|s| self.can_coerce(t_elem, s, None))
                }
            }
            (Type::Tuple(t1), Type::Tuple(t2)) => {
                if t1.len() != t2.len() {
                    return false;
                }
                t1.iter().zip(t2).all(|(a, b)| self.can_coerce(a, b, None))
            }
            (Type::Tuple(t1), Type::Object(f2)) => {
                if t1.len() != f2.len() {
                    return false;
                }
                t1.iter()
                    .zip(f2)
                    .all(|(a, b)| self.can_coerce(a, &b.ty, None))
            }
            (Type::Object(f1), Type::Tuple(t2)) => {
                if f1.len() != t2.len() {
                    return false;
                }
                f1.iter()
                    .zip(t2)
                    .all(|(a, b)| self.can_coerce(&a.ty, b, None))
            }
            _ => false,
        }
    }

    pub(super) fn types_eq(&self, a: &Type, b: &Type) -> bool {
        if a == b {
            return true;
        }
        if *a == Type::Any || *b == Type::Any || *a == Type::Unknown || *b == Type::Unknown {
            return true;
        }
        let res_a = self.resolve_type(a);
        let res_b = self.resolve_type(b);
        if &res_a != a || &res_b != b {
            if self.types_eq(&res_a, &res_b) {
                return true;
            }
        }
        if Self::is_int_type(a) && Self::is_int_type(b) {
            return true;
        }
        if matches!(a, Type::F32 | Type::F64) && matches!(b, Type::F32 | Type::F64) {
            return true;
        }
        match (a, b) {
            (Type::Conditional(_, c1, a1), Type::Conditional(_, c2, a2)) => {
                self.types_eq(c1, c2) && self.types_eq(a1, a2)
            }
            (Type::Conditional(_, c1, a1), other) | (other, Type::Conditional(_, c1, a1)) => {
                self.types_eq(c1, other) || self.types_eq(a1, other)
            }
            (Type::Ref(t1), Type::Ref(t2)) => t1.sym == t2.sym,
            (Type::Ref(t1), Type::FixedArray(_, _) | Type::Array(_) | Type::Tuple(_))
            | (Type::FixedArray(_, _) | Type::Array(_) | Type::Tuple(_), Type::Ref(t1))
                if t1.sym.starts_with("@Vector(") =>
            {
                true
            }
            (Type::BoxDyn(t1), Type::BoxDyn(t2)) => t1.sym == t2.sym,
            (Type::BoxDyn(_), Type::Ref(_)) | (Type::Ref(_), Type::BoxDyn(_)) => true,
            (Type::BorrowType { inner: i1, .. }, Type::BorrowType { inner: i2, .. }) => {
                self.types_eq(i1, i2)
            }
            (Type::BorrowType { inner: i1, .. }, other)
            | (other, Type::BorrowType { inner: i1, .. }) => self.types_eq(i1, other),
            (Type::Shared(i1), Type::Shared(i2)) => self.types_eq(i1, i2),
            (Type::Shared(i1), other) | (other, Type::Shared(i1)) => self.types_eq(i1, other),
            (Type::Optional(t1), Type::Optional(t2)) => self.types_eq(t1, t2),
            (Type::Optional(t1), other) | (other, Type::Optional(t1)) => self.types_eq(t1, other),
            (Type::ErrorUnion(t1), Type::ErrorUnion(t2)) => self.types_eq(t1, t2),
            (Type::ResultType { ok: o1, .. }, Type::ResultType { ok: o2, .. }) => {
                self.types_eq(o1, o2)
            }
            (Type::ResultType { ok: o1, .. }, other) | (other, Type::ResultType { ok: o1, .. }) => {
                self.types_eq(o1, other)
            }
            (Type::ManyRawPtr(i1), Type::ManyRawPtr(i2)) => self.types_eq(i1, i2),
            (Type::CPtr(i1), Type::CPtr(i2)) => self.types_eq(i1, i2),
            (Type::CPtr(i1), Type::ManyRawPtr(i2)) | (Type::ManyRawPtr(i1), Type::CPtr(i2)) => {
                self.types_eq(i1, i2)
            }
            (Type::SentinelPtr(i1, s1), Type::SentinelPtr(i2, s2)) => {
                s1 == s2 && self.types_eq(i1, i2)
            }
            (Type::SentinelSlice(i1, s1), Type::SentinelSlice(i2, s2)) => {
                s1.split(':').next() == s2.split(':').next() && self.types_eq(i1, i2)
            }
            (Type::SentinelSlice(i1, _), Type::Array(i2))
            | (Type::Array(i2), Type::SentinelSlice(i1, _)) => self.types_eq(i1, i2),
            (Type::String, Type::Array(inner)) | (Type::Array(inner), Type::String)
                if **inner == Type::U8 =>
            {
                true
            }
            (Type::String, Type::SentinelSlice(inner, _))
            | (Type::SentinelSlice(inner, _), Type::String)
                if **inner == Type::U8 =>
            {
                true
            }
            (Type::Array(t1), Type::Array(t2)) => self.types_eq(t1, t2),
            (Type::FixedArray(t1, s1), Type::FixedArray(t2, s2)) => {
                let same_len = match (s1, s2) {
                    (ArraySize::Literal(a), ArraySize::Literal(b)) => a == b,
                    (ArraySize::Ident(a), ArraySize::Ident(b)) => a.sym == b.sym,
                    (ArraySize::Expr(a), ArraySize::Expr(b)) => a == b,
                    _ => s1 == s2,
                };
                same_len && self.types_eq(t1, t2)
            }
            (Type::FixedArray(elem, _), Type::Array(t2))
            | (Type::Array(t2), Type::FixedArray(elem, _)) => self.types_eq(elem, t2),
            (Type::Array(elem), Type::Tuple(tys)) | (Type::Tuple(tys), Type::Array(elem)) => {
                tys.iter().all(|t| self.types_eq(elem, t))
            }
            (Type::FixedArray(elem, len), Type::Tuple(tys))
            | (Type::Tuple(tys), Type::FixedArray(elem, len)) => {
                len.as_usize() == Some(tys.len()) && tys.iter().all(|t| self.types_eq(elem, t))
            }
            (Type::Tuple(t1), Type::Tuple(t2)) => {
                if t1.len() != t2.len() {
                    return false;
                }
                for (i, p) in t1.iter().enumerate() {
                    if !self.types_eq(p, &t2[i]) {
                        return false;
                    }
                }
                true
            }
            (Type::Tuple(tys), Type::Object(fields)) | (Type::Object(fields), Type::Tuple(tys)) => {
                if tys.len() != fields.len() {
                    return false;
                }
                for (i, p) in tys.iter().enumerate() {
                    if !self.types_eq(p, &fields[i].ty) {
                        return false;
                    }
                }
                true
            }
            (Type::Object(f1), Type::Object(f2)) => {
                if f1.len() != f2.len() {
                    return false;
                }
                for (i, f) in f1.iter().enumerate() {
                    if f.ident.sym != f2[i].ident.sym || !self.types_eq(&f.ty, &f2[i].ty) {
                        return false;
                    }
                }
                true
            }
            (Type::Fn(p1, r1), Type::Fn(p2, r2)) => {
                if p1.len() != p2.len() {
                    return false;
                }
                for (i, p) in p1.iter().enumerate() {
                    if !self.types_eq(p, &p2[i]) {
                        return false;
                    }
                }
                self.types_eq(r1, r2)
            }
            (Type::Void, Type::Void) => true,
            (Type::Any, _) | (_, Type::Any) => true,
            (Type::Auto, _) | (_, Type::Auto) => true,
            (Type::Aligned { inner, .. }, other) | (other, Type::Aligned { inner, .. }) => {
                self.types_eq(inner, other)
            }
            _ => {
                if let Type::Ref(id) = a {
                    if let Some(aliased) = self.type_aliases.get(&id.sym).cloned() {
                        return self.types_eq(&aliased, b);
                    }
                }
                if let Type::Ref(id) = b {
                    if let Some(aliased) = self.type_aliases.get(&id.sym).cloned() {
                        return self.types_eq(a, &aliased);
                    }
                }
                a == b
            }
        }
    }

    pub(super) fn analyze_pattern(&mut self, pat: &Pattern, ty: Type, kind: &VarDeclKind) -> SemaResult<()> {
        match pat {
            Pattern::Ident(ident) => {
                if !ident.sym.starts_with("@\"") && RESERVED_WORDS.contains(&ident.sym.as_str()) {
                    self.errors.push(SemaError::ReservedIdentifier(
                        ident.sym.clone(),
                        ident.span,
                        false,
                    ));
                }
                if self.scopes.len() > 1 {
                    if let Some(SymbolKind::Var { .. }) =
                        self.scopes.last().unwrap().get(&ident.sym)
                    {
                        return Err(SemaError::DuplicateVarDecl(ident.sym.clone(), ident.span));
                    }
                }
                self.define(
                    ident.sym.clone(),
                    SymbolKind::Var {
                        ty,
                        kind: kind.clone(),
                    },
                    ident.span,
                )
            }
            Pattern::Rest(inner) => self.analyze_pattern(inner, ty, kind),
            Pattern::Tuple(elements) => {
                let mut base_ty = ty;
                while let Type::BorrowType { inner, .. } = base_ty {
                    base_ty = *inner;
                }
                let rest_positions: Vec<usize> = elements
                    .iter()
                    .enumerate()
                    .filter_map(|(i, p)| {
                        if matches!(p, Pattern::Rest(_)) {
                            Some(i)
                        } else {
                            None
                        }
                    })
                    .collect();
                if rest_positions.len() > 1 {
                    return Err(SemaError::MultipleRestInPattern(Span { start: 0, end: 0 }));
                }
                if let Some(&rest_idx) = rest_positions.first() {
                    let prefix_len = rest_idx;
                    let suffix_len = elements.len() - 1 - rest_idx;
                    match base_ty {
                        Type::Tuple(element_types) => {
                            if element_types.len() < prefix_len + suffix_len {
                                return Err(SemaError::TypeMismatch(
                                    Type::Tuple(vec![]),
                                    Type::Tuple(element_types),
                                    Span { start: 0, end: 0 },
                                ));
                            }
                            for i in 0..prefix_len {
                                self.analyze_pattern(&elements[i], element_types[i].clone(), kind)?;
                            }
                            if let Pattern::Rest(inner) = &elements[rest_idx] {
                                let slice_elem_ty = element_types
                                    .get(rest_idx)
                                    .cloned()
                                    .or_else(|| element_types.first().cloned())
                                    .unwrap_or(Type::Any);
                                self.analyze_pattern(
                                    inner,
                                    Type::Array(Box::new(slice_elem_ty)),
                                    kind,
                                )?;
                            }
                            for i in (rest_idx + 1)..elements.len() {
                                let offset_from_end = elements.len() - i;
                                let elem_ty =
                                    element_types[element_types.len() - offset_from_end].clone();
                                self.analyze_pattern(&elements[i], elem_ty, kind)?;
                            }
                        }
                        Type::Object(fields) => {
                            let element_types: Vec<Type> =
                                fields.into_iter().map(|f| f.ty).collect();
                            if element_types.len() < prefix_len + suffix_len {
                                return Err(SemaError::TypeMismatch(
                                    Type::Tuple(vec![]),
                                    Type::Tuple(element_types),
                                    Span { start: 0, end: 0 },
                                ));
                            }
                            for i in 0..prefix_len {
                                self.analyze_pattern(&elements[i], element_types[i].clone(), kind)?;
                            }
                            if let Pattern::Rest(inner) = &elements[rest_idx] {
                                let slice_elem_ty = element_types
                                    .get(rest_idx)
                                    .cloned()
                                    .or_else(|| element_types.first().cloned())
                                    .unwrap_or(Type::Any);
                                self.analyze_pattern(
                                    inner,
                                    Type::Array(Box::new(slice_elem_ty)),
                                    kind,
                                )?;
                            }
                            for i in (rest_idx + 1)..elements.len() {
                                let offset_from_end = elements.len() - i;
                                let elem_ty =
                                    element_types[element_types.len() - offset_from_end].clone();
                                self.analyze_pattern(&elements[i], elem_ty, kind)?;
                            }
                        }
                        Type::Array(inner) => {
                            for (i, el) in elements.iter().enumerate() {
                                if i == rest_idx {
                                    self.analyze_pattern(el, Type::Array(inner.clone()), kind)?;
                                } else {
                                    self.analyze_pattern(el, (*inner).clone(), kind)?;
                                }
                            }
                        }
                        Type::Any | Type::Unknown => {
                            for (i, el) in elements.iter().enumerate() {
                                if i == rest_idx {
                                    self.analyze_pattern(
                                        el,
                                        Type::Array(Box::new(Type::Any)),
                                        kind,
                                    )?;
                                } else {
                                    self.analyze_pattern(el, Type::Any, kind)?;
                                }
                            }
                        }
                        _ => {
                            return Err(SemaError::TypeMismatch(
                                Type::Tuple(vec![]),
                                base_ty,
                                Span { start: 0, end: 0 },
                            ))
                        }
                    }
                    Ok(())
                } else {
                    let element_types = match base_ty {
                        Type::Tuple(tys) => tys,
                        Type::Array(inner) => vec![(*inner).clone(); elements.len()],
                        Type::Object(fields) => fields.into_iter().map(|f| f.ty).collect(),
                        Type::Any | Type::Unknown => vec![Type::Any; elements.len()],
                        _ => {
                            return Err(SemaError::TypeMismatch(
                                Type::Tuple(vec![]),
                                base_ty,
                                Span { start: 0, end: 0 },
                            ))
                        }
                    };
                    if elements.len() != element_types.len() {
                        return Err(SemaError::TypeMismatch(
                            Type::Tuple(vec![]),
                            Type::Tuple(element_types),
                            Span { start: 0, end: 0 },
                        ));
                    }
                    for (i, el) in elements.iter().enumerate() {
                        self.analyze_pattern(el, element_types[i].clone(), kind)?;
                    }
                    Ok(())
                }
            }
            Pattern::Object(props) => {
                let rest_count = props.iter().filter(|p| p.is_rest).count();
                if rest_count > 1 {
                    return Err(SemaError::MultipleRestInPattern(Span { start: 0, end: 0 }));
                }
                let mut base_ty = ty;
                while let Type::BorrowType { inner, .. } = base_ty {
                    base_ty = *inner;
                }
                let fields = match base_ty {
                    Type::Object(fs) => fs,
                    Type::Ref(ref id) => {
                        if let Some(cls) = self.classes.get(&id.sym).cloned() {
                            cls.fields
                        } else {
                            vec![]
                        }
                    }
                    Type::Any | Type::Unknown => vec![],
                    _ => {
                        return Err(SemaError::TypeMismatch(
                            Type::Object(vec![]),
                            base_ty,
                            Span { start: 0, end: 0 },
                        ))
                    }
                };
                let non_rest_keys: std::collections::HashSet<String> = props
                    .iter()
                    .filter(|p| !p.is_rest)
                    .map(|p| p.key.sym.clone())
                    .collect();

                for prop in props {
                    if prop.is_rest {
                        if prop.key.sym == "*" || matches!(prop.val, Some(Pattern::Wildcard)) {
                            continue;
                        }
                        let remaining_fields: Vec<Field> = fields
                            .iter()
                            .filter(|f| !non_rest_keys.contains(&f.ident.sym))
                            .cloned()
                            .collect();
                        let rest_ty = Type::Object(remaining_fields);
                        if let Some(inner_pat) = &prop.val {
                            self.analyze_pattern(inner_pat, rest_ty, kind)?;
                        } else {
                            self.analyze_pattern(&Pattern::Ident(prop.key.clone()), rest_ty, kind)?;
                        }
                        continue;
                    }

                    if !fields.is_empty() {
                        let field = fields
                            .iter()
                            .find(|f| f.ident.sym == prop.key.sym)
                            .ok_or_else(|| {
                                SemaError::UndefinedSymbol(prop.key.sym.clone(), prop.key.span)
                            })?;
                        if let Some(inner_pat) = &prop.val {
                            self.analyze_pattern(inner_pat, field.ty.clone(), kind)?;
                        } else {
                            self.analyze_pattern(
                                &Pattern::Ident(prop.key.clone()),
                                field.ty.clone(),
                                kind,
                            )?;
                        }
                    } else {
                        if let Some(inner_pat) = &prop.val {
                            self.analyze_pattern(inner_pat, Type::Any, kind)?;
                        } else {
                            self.analyze_pattern(
                                &Pattern::Ident(prop.key.clone()),
                                Type::Any,
                                kind,
                            )?;
                        }
                    }
                }
                Ok(())
            }
            Pattern::Binding { ident, subpat } => {
                if let Some(SymbolKind::Var { .. }) = self.scopes.last().unwrap().get(&ident.sym) {
                    return Err(SemaError::DuplicateVarDecl(ident.sym.clone(), ident.span));
                }
                self.define(
                    ident.sym.clone(),
                    SymbolKind::Var {
                        ty: ty.clone(),
                        kind: kind.clone(),
                    },
                    ident.span,
                )?;
                self.analyze_pattern(subpat, ty, kind)?;
                Ok(())
            }
            Pattern::TypeMatch { ident, ty: pty } => {
                if let Some(SymbolKind::Var { .. }) = self.scopes.last().unwrap().get(&ident.sym) {
                    return Err(SemaError::DuplicateVarDecl(ident.sym.clone(), ident.span));
                }
                self.define(
                    ident.sym.clone(),
                    SymbolKind::Var {
                        ty: pty.clone(),
                        kind: kind.clone(),
                    },
                    ident.span,
                )?;
                Ok(())
            }
            _ => Ok(()),
        }
    }

    pub(super) fn analyze_block(&mut self, block: &BlockStmt) -> SemaResult<()> {
        self.enter_scope();
        for stmt in &block.body {
            if let Err(e) = self.analyze_stmt(stmt) {
                self.errors.push(e);
            }
        }
        self.exit_scope();
        Ok(())
    }

}
