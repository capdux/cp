use super::super::*;
use crate::ast::*;
use std::collections::HashMap;

impl Codegen {
    pub(crate) fn map_type_name(&self, name: &str) -> String {
        match name {
            "i8" => "i8".to_string(),
            "u8" => "u8".to_string(),
            "i16" => "i16".to_string(),
            "u16" => "u16".to_string(),
            "i32" => "i32".to_string(),
            "u32" => "u32".to_string(),
            "i64" => "i64".to_string(),
            "u64" => "u64".to_string(),
            "isize" => "isize".to_string(),
            "usize" => "usize".to_string(),
            "f32" => "f32".to_string(),
            "f64" => "f64".to_string(),
            "bool" => "bool".to_string(),
            "string" => "[]const u8".to_string(),
            "u8x16" => "@Vector(16, u8)".to_string(),
            "u8x32" => "@Vector(32, u8)".to_string(),
            "u8x64" => "@Vector(64, u8)".to_string(),
            "i16x8" => "@Vector(8, i16)".to_string(),
            "i16x16" => "@Vector(16, i16)".to_string(),
            "i32x4" => "@Vector(4, i32)".to_string(),
            "i32x8" => "@Vector(8, i32)".to_string(),
            "u32x4" => "@Vector(4, u32)".to_string(),
            "u32x8" => "@Vector(8, u32)".to_string(),
            "f32x4" => "@Vector(4, f32)".to_string(),
            "f32x8" => "@Vector(8, f32)".to_string(),
            "f32x16" => "@Vector(16, f32)".to_string(),
            "f64x2" => "@Vector(2, f64)".to_string(),
            "f64x4" => "@Vector(4, f64)".to_string(),
            _ => {
                if name.starts_with("Mutex<") {
                    let inner = &name[6..name.len() - 1];
                    format!("CpMutex({})", self.map_type_name(inner))
                } else if name.starts_with("RwLock<") {
                    let inner = &name[7..name.len() - 1];
                    format!("CpRwLock({})", self.map_type_name(inner))
                } else if name.starts_with("Atomic<") {
                    let inner = &name[7..name.len() - 1];
                    format!("CpAtomic({})", self.map_type_name(inner))
                } else if let Some(pos) = name.find('<') {
                    if name.ends_with('>') {
                        let base = &name[..pos];
                        let args = &name[pos + 1..name.len() - 1];
                        let mapped_args = Self::map_generic_args_str(args);
                        format!("{}({})", base, mapped_args)
                    } else {
                        name.to_string()
                    }
                } else {
                    name.to_string()
                }
            }
        }
    }

    pub(crate) fn map_type(&self, ty: &Type) -> String {
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
            Type::String => "[]const u8".to_string(),
            Type::PID => "PID".to_string(),
            Type::Void => "void".to_string(),
            Type::Any => "*anyopaque".to_string(),
            Type::Auto => "anytype".to_string(),
            Type::Array(inner) => format!("[]{}", self.map_type(inner)),
            Type::FixedArray(inner, size) => format!("[{}]{}", size, self.map_type(inner)),
            Type::Map(_, _) => "*Map".to_string(),
            Type::Dict(_, _) => "*Dict".to_string(),
            Type::Set(_) => "*Set".to_string(),
            Type::Ref(ident) => {
                match ident.sym.as_str() {
                    "u8x16" => return "@Vector(16, u8)".to_string(),
                    "u8x32" => return "@Vector(32, u8)".to_string(),
                    "u8x64" => return "@Vector(64, u8)".to_string(),
                    "i16x8" => return "@Vector(8, i16)".to_string(),
                    "i16x16" => return "@Vector(16, i16)".to_string(),
                    "i32x4" => return "@Vector(4, i32)".to_string(),
                    "i32x8" => return "@Vector(8, i32)".to_string(),
                    "u32x4" => return "@Vector(4, u32)".to_string(),
                    "u32x8" => return "@Vector(8, u32)".to_string(),
                    "f32x4" => return "@Vector(4, f32)".to_string(),
                    "f32x8" => return "@Vector(8, f32)".to_string(),
                    "f32x16" => return "@Vector(16, f32)".to_string(),
                    "f64x2" => return "@Vector(2, f64)".to_string(),
                    "f64x4" => return "@Vector(4, f64)".to_string(),
                    _ => {}
                }
                if ident.sym == "TypeInfo" {
                    return "*const TypeInfo".to_string();
                }
                if ident.sym.starts_with("Mutex<") {
                    let inner = &ident.sym[6..ident.sym.len() - 1];
                    return format!("CpMutex({})", self.map_type_name(inner));
                }
                if ident.sym == "Mutex"
                    && !self.type_aliases.contains_key("Mutex")
                    && !self.class_defs.contains_key("Mutex")
                {
                    return "CpBareMutex".to_string();
                }
                if ident.sym.starts_with("RwLock<") {
                    let inner = &ident.sym[7..ident.sym.len() - 1];
                    return format!("CpRwLock({})", self.map_type_name(inner));
                }
                if ident.sym == "RwLock"
                    && !self.type_aliases.contains_key("RwLock")
                    && !self.class_defs.contains_key("RwLock")
                {
                    return "CpBareRwLock".to_string();
                }
                if ident.sym.starts_with("Atomic<") {
                    let inner = &ident.sym[7..ident.sym.len() - 1];
                    return format!("CpAtomic({})", self.map_type_name(inner));
                }
                if ident.sym == "Atomic" {
                    return "CpAtomic(i32)".to_string();
                }
                if ident.sym.starts_with("LockGuard<") {
                    let inner = &ident.sym[10..ident.sym.len() - 1];
                    return format!("LockGuard({})", self.map_type_name(inner));
                }
                if ident.sym.starts_with("ReadGuard<") {
                    let inner = &ident.sym[10..ident.sym.len() - 1];
                    return format!("ReadGuard({})", self.map_type_name(inner));
                }
                if ident.sym.starts_with("WriteGuard<") {
                    let inner = &ident.sym[11..ident.sym.len() - 1];
                    return format!("WriteGuard({})", self.map_type_name(inner));
                }
                if ident.sym == "FieldInfo" || ident.sym == "MethodInfo" {
                    return ident.sym.clone();
                }
                if let Some(pos) = ident.sym.find('<') {
                    if ident.sym.ends_with('>') {
                        let base = &ident.sym[..pos];
                        let args = &ident.sym[pos + 1..ident.sym.len() - 1];
                        let mapped_args = Self::map_generic_args_str(args);
                        return format!("{}({})", base, mapped_args);
                    }
                }
                if ident.sym == "Managed" {
                    return ident.sym.clone();
                }
                if ident.sym == "cstring" {
                    return "[:0]const u8".to_string();
                }
                if ident.sym.starts_with('[') {
                    return ident.sym.clone();
                }
                if ident.sym.starts_with("struct")
                    || ident.sym.starts_with("?struct")
                    || ident.sym.starts_with("union")
                {
                    return super::lower_struct_body(&ident.sym);
                }
                let is_custom_int = (ident.sym.starts_with('u') || ident.sym.starts_with('i'))
                    && ident.sym.len() > 1
                    && ident.sym[1..].chars().all(|c| c.is_ascii_digit());
                if is_custom_int {
                    return ident.sym.clone();
                }
                let base_name = ident.sym.split('(').next().unwrap_or(&ident.sym);
                if ident.sym == "PID"
                    || ident.sym == "Message"
                    || ident.sym == "Error"
                    || ident.sym == "anytype"
                    || ident.sym == "anyopaque"
                    || ident.sym == "type"
                    || ident.sym == "anyerror"
                    || ident.sym == "noreturn"
                    || (!ident.sym.is_empty() && ident.sym.chars().all(|c| c.is_ascii_uppercase()))
                    || ident.sym.starts_with("c_")
                    || ident.sym.starts_with("Many")
                    || ident.sym.contains('.')
                    || ident.sym.contains('(')
                    || ident.sym.starts_with("__AnonErr_")
                    || ident.sym.starts_with("__UnionErr_")
                    || self.ref_classes.contains(&ident.sym)
                    || self.value_classes.contains(&ident.sym)
                    || self.value_classes.contains(base_name)
                    || self.enum_defs.contains_key(&ident.sym)
                    || self.error_sets.contains_key(&ident.sym)
                    || self.trait_defs.contains_key(&ident.sym)
                {
                    return ident.sym.clone();
                }
                if let Some(cls) = self.class_defs.get(base_name) {
                    if !cls.generic_params.is_empty() || cls.is_union || cls.is_ref {
                        return ident.sym.clone();
                    }
                } else if !self.ref_classes.contains(base_name) {
                    return ident.sym.clone();
                }
                if let Some(ta) = self.type_aliases.get(&ident.sym) {
                    let target_base = match &ta.target_ty {
                        Type::Ref(tid) => tid.sym.split('<').next().unwrap_or(&tid.sym),
                        _ => "",
                    };
                    if let Some(cls) = self.class_defs.get(target_base) {
                        if !cls.generic_params.is_empty()
                            || cls.is_ref
                            || cls.is_union
                            || self.value_classes.contains(target_base)
                        {
                            return ident.sym.clone();
                        }
                    } else if !self.class_defs.contains_key(target_base) {
                        return ident.sym.clone();
                    }
                }
                format!("*{}", ident.sym)
            }
            Type::BorrowType { inner, is_mut, .. } => {
                if !*is_mut {
                    if let Type::Array(elem) = &**inner {
                        return format!("[]const {}", self.map_type(elem));
                    }
                    if matches!(**inner, Type::CStr) {
                        return "[*:0]const u8".to_string();
                    }
                    if let Type::Optional(opt_inner) = &**inner {
                        let opt_zig = self.map_type(opt_inner);
                        if opt_zig.starts_with("*const ") {
                            return format!("?{}", opt_zig);
                        }
                        let clean_opt = if opt_zig.starts_with('*') {
                            &opt_zig[1..]
                        } else {
                            &opt_zig
                        };
                        return format!("?*const {}", clean_opt);
                    }
                    let inner_zig = self.map_type(inner);
                    if matches!(**inner, Type::Aligned { .. }) {
                        return format!("*{}", inner_zig);
                    }
                    if inner_zig.starts_with("*const ") {
                        return inner_zig;
                    }
                    let clean_inner = if inner_zig.starts_with('*') {
                        &inner_zig[1..]
                    } else {
                        &inner_zig
                    };
                    return format!("*const {}", clean_inner);
                }
                if *is_mut {
                    if let Type::Array(elem) = &**inner {
                        return format!("[]{}", self.map_type(elem));
                    }
                    if let Type::String = &**inner {
                        return "[]u8".to_string();
                    }
                    if let Type::Ref(id) = &**inner {
                        if id.sym == "cstring" {
                            return "[:0]u8".to_string();
                        }
                    }
                    if matches!(**inner, Type::CStr) {
                        return "[*:0]u8".to_string();
                    }
                    if let Type::Optional(opt_inner) = &**inner {
                        if let Type::Array(elem) = &**opt_inner {
                            return format!("?[]{}", self.map_type(elem));
                        }
                        if let Type::String = &**opt_inner {
                            return "?[]u8".to_string();
                        }
                    }
                }
                if let Type::Optional(opt_inner) = &**inner {
                    let opt_inner_zig = self.map_type(opt_inner);
                    if opt_inner_zig.starts_with('*') {
                        if matches!(&**opt_inner, Type::BorrowType { .. } | Type::RawPtr { .. }) {
                            return format!("*?{}", opt_inner_zig);
                        }
                        return format!("?{}", opt_inner_zig);
                    }
                    return format!("?*{}", opt_inner_zig);
                }
                let inner_zig = self.map_type(inner);
                let is_trait = if let Type::Ref(id) = &**inner {
                    self.trait_defs.contains_key(&id.sym)
                } else {
                    false
                };
                if inner_zig.starts_with('*') || is_trait || inner_zig == "anytype" {
                    inner_zig
                } else {
                    format!("*{}", inner_zig)
                }
            }
            Type::Shared(inner) => {
                let inner_zig = self.map_type(inner);
                if inner_zig.starts_with('*') {
                    inner_zig
                } else {
                    format!("*{}", inner_zig)
                }
            }
            Type::Optional(inner) => format!("?{}", self.map_type(inner)),
            Type::ErrorUnion(inner) => {
                if self.is_lib_mode {
                    format!("!{}", self.map_type(inner))
                } else {
                    format!("anyerror!{}", self.map_type(inner))
                }
            }
            Type::ResultType { ok, err } => {
                let err_str = self.map_error_type(err);
                let ok_str = self.map_type(ok);
                if self.is_lib_mode
                    || err_str.contains('.')
                    || err_str == "anyerror"
                    || self.error_sets.contains_key(&err_str)
                    || err_str.starts_with('(')
                {
                    format!("{}!{}", err_str, ok_str)
                } else {
                    format!("CpResult({}, {})", ok_str, err_str)
                }
            }
            Type::BoxDyn(id) => id.sym.clone(),
            Type::BareDyn(id) => {
                panic!(
                    "CP Compile Error: Bare 'dyn {}' cannot be placed on the stack or used without Box; use 'Box<dyn {}>' instead",
                    id.sym, id.sym
                )
            }
            Type::Tuple(types) => {
                let mapped: Vec<String> = types.iter().map(|t| self.map_type(t)).collect();
                format!("struct {{ {} }}", mapped.join(", "))
            }
            Type::Object(fields) => {
                let mapped: Vec<String> = fields
                    .iter()
                    .map(|f| format!("{}: {}", f.ident.sym, self.map_type(&f.ty)))
                    .collect();
                let body = format!("struct {{ {} }}", mapped.join(", "));
                let mut name = String::from("__CpObj");
                for f in fields {
                    let clean_ty: String = self
                        .map_type(&f.ty)
                        .chars()
                        .map(|c| if c.is_ascii_alphanumeric() { c } else { '_' })
                        .collect();
                    name.push_str(&format!("_{}_{}", f.ident.sym, clean_ty));
                }
                self.anon_obj_structs
                    .borrow_mut()
                    .insert(name.clone(), body);
                name
            }
            Type::RawPtr { inner, depth } => {
                if let Type::Optional(opt_inner) = &**inner {
                    let base = self.map_raw_ptr_base(opt_inner);
                    format!("?{}{}", "*".repeat(*depth), base)
                } else if let Type::BorrowType {
                    inner: b_in,
                    is_mut: false,
                    ..
                } = &**inner
                {
                    if let Type::Optional(opt_inner) = &**b_in {
                        let base = self.map_raw_ptr_base(opt_inner);
                        return format!("?{}const {}", "*".repeat(*depth), base);
                    }
                    let base = self.map_raw_ptr_base(inner);
                    format!("{}{}", "*".repeat(*depth), base)
                } else {
                    let base = self.map_raw_ptr_base(inner);
                    format!("{}{}", "*".repeat(*depth), base)
                }
            }
            Type::ManyRawPtr(inner) => {
                let base = self.map_raw_ptr_base(inner);
                format!("[*]{}", base)
            }
            Type::CPtr(inner) => {
                let base = self.map_raw_ptr_base(inner);
                format!("[*c]{}", base)
            }
            Type::SentinelPtr(inner, val) => {
                let base = self.map_raw_ptr_base(inner);
                format!("[*:{val}]{}", base)
            }
            Type::SentinelSlice(inner, val) => {
                if let Some(s) = val.strip_suffix(":const") {
                    format!("[:{s}]const {}", self.map_type(inner))
                } else {
                    format!("[:{val}]{}", self.map_type(inner))
                }
            }
            Type::CStr => "[*:0]const u8".to_string(),
            Type::CFn(params, ret) => {
                let mapped: Vec<String> = params.iter().map(|p| self.map_type(p)).collect();
                format!(
                    "*const fn ({}) callconv(.c) {}",
                    mapped.join(", "),
                    self.map_type(ret)
                )
            }
            Type::Fn(params, ret) => {
                let mapped: Vec<String> = params.iter().map(|p| self.map_type(p)).collect();
                format!("*const fn ({}) {}", mapped.join(", "), self.map_type(ret))
            }
            Type::Conditional(cond, cons, alt) => {
                let cond_str = self.expr_to_zig_pure(cond);
                let cons_str = self.map_type(cons);
                let alt_str = self.map_type(alt);
                format!("if ({}) {} else {}", cond_str, cons_str, alt_str)
            }
            Type::Aligned { qual, inner } => {
                format!("{}{}", qual, self.map_type(inner))
            }
            _ => "i32".to_string(),
        }
    }

    pub(crate) fn split_generic_args_str(args: &str) -> Vec<String> {
        let mut parts = Vec::new();
        let mut cur = String::new();
        let mut depth = 0;
        for c in args.chars() {
            match c {
                '[' | '(' | '<' | '{' => {
                    depth += 1;
                    cur.push(c);
                }
                ']' | ')' | '>' | '}' => {
                    depth -= 1;
                    cur.push(c);
                }
                ',' if depth == 0 => {
                    parts.push(cur.trim().to_string());
                    cur.clear();
                }
                _ => cur.push(c),
            }
        }
        if !cur.trim().is_empty() {
            parts.push(cur.trim().to_string());
        }
        parts
    }

    pub(crate) fn map_generic_args_str(args: &str) -> String {
        let parts = Self::split_generic_args_str(args);
        let types = [
            "u8", "u16", "u32", "u64", "i8", "i16", "i32", "i64", "usize", "isize", "f32", "f64",
            "bool", "string",
        ];
        let mapped_parts: Vec<String> = parts
            .into_iter()
            .map(|p| {
                if p.starts_with('[') && p.ends_with(']') {
                    let inner = p[1..p.len() - 1].trim();
                    if types.contains(&inner) {
                        format!("[]{}", inner)
                    } else {
                        format!(".{{ {} }}", inner)
                    }
                } else {
                    p
                }
            })
            .collect();

        mapped_parts.join(", ")
    }

    pub(crate) fn map_raw_ptr_base(&self, ty: &Type) -> String {
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
            Type::String => "[]const u8".to_string(),
            Type::CStr => "[*:0]u8".to_string(),
            Type::Void | Type::Any => "anyopaque".to_string(),
            Type::BorrowType { inner, is_mut, .. } => {
                if !*is_mut {
                    format!("const {}", self.map_raw_ptr_base(inner))
                } else {
                    self.map_raw_ptr_base(inner)
                }
            }
            Type::Ref(id) => {
                if id.sym == "anyopaque" || id.sym == "any" {
                    "anyopaque".to_string()
                } else {
                    id.sym.clone()
                }
            }
            Type::Aligned { qual, inner } => {
                format!("{}{}", qual, self.map_raw_ptr_base(inner))
            }
            _ => self.map_type(ty),
        }
    }
}
