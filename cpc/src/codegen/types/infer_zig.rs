use super::super::*;
use crate::ast::*;
use std::collections::HashMap;

impl Codegen {
    pub(crate) fn infer_zig_type(&self, ty: Option<&Type>, init: Option<&Expr>) -> String {
        if let Some(t) = ty {
            return self.map_type(t);
        }
        if let Some(expr) = init {
            if let Expr::New(call) = expr {
                if let Expr::Ident(id) = &*call.callee {
                    if id.sym == "Error" {
                        return "Error".to_string();
                    }
                    return format!("*{}", self.map_type_name(&id.sym));
                }
            }
            if let Expr::Slice(s) = expr {
                let obj_t = self.infer_zig_type(None, Some(&s.obj));
                let elem_t = if let Some(Type::Array(elem)) = self.infer_cp_type(expr) {
                    self.map_type(&elem)
                } else {
                    "u8".to_string()
                };
                if obj_t.contains("const") || obj_t.contains("[:0]const") {
                    return format!("[]const {}", elem_t);
                } else {
                    return format!("[]{}", elem_t);
                }
            }
            if let Some(cp_ty) = self.infer_cp_type(expr) {
                return self.map_type(&cp_ty);
            }
            match expr {
                Expr::Lit(crate::ast::Lit::Int(_)) => return "i32".to_string(),
                Expr::Lit(crate::ast::Lit::Float(_)) => return "f32".to_string(),
                Expr::Lit(crate::ast::Lit::Str(_)) => return "[]const u8".to_string(),
                Expr::Lit(crate::ast::Lit::Bool(_)) => return "bool".to_string(),
                Expr::Template(_) => return "[]const u8".to_string(),
                Expr::New(call) => {
                    if let Expr::Ident(id) = &*call.callee {
                        if id.sym == "Error" {
                            return "Error".to_string();
                        }
                        return format!("*{}", self.map_type_name(&id.sym));
                    }
                }
                Expr::Call(call) => {
                    if let Expr::Ident(id) = &*call.callee {
                        if (id.sym == "string" || id.sym == "String") && call.args.len() == 1 {
                            return "[]const u8".to_string();
                        }
                        match id.sym.as_str() {
                            "i8" => return "i8".to_string(),
                            "u8" => return "u8".to_string(),
                            "i16" => return "i16".to_string(),
                            "u16" => return "u16".to_string(),
                            "i32" => return "i32".to_string(),
                            "u32" => return "u32".to_string(),
                            "i64" => return "i64".to_string(),
                            "u64" => return "u64".to_string(),
                            "i128" => return "i128".to_string(),
                            "u128" => return "u128".to_string(),
                            "isize" => return "isize".to_string(),
                            "usize" => return "usize".to_string(),
                            "f32" => return "f32".to_string(),
                            "f64" => return "f64".to_string(),
                            "bool" => return "bool".to_string(),
                            _ => {}
                        }
                        if id.sym == "next" {
                            if let Some(ret_ty) = &self.current_fn_ret_ty {
                                return self.map_type(ret_ty);
                            } else {
                                return "void".to_string();
                            }
                        }
                        if let Some(sig) = self.fn_sigs.get(&id.sym) {
                            if let Some(ret_ty) = &sig.return_type {
                                return self.map_type(ret_ty);
                            } else {
                                return "void".to_string();
                            }
                        }
                        if let Some(c_name) = &self.current_class_name {
                            if let Some((_, ret_ty_opt, _)) = self.lookup_method(c_name, &id.sym) {
                                if let Some(ret_ty) = ret_ty_opt {
                                    return self.map_type(&ret_ty);
                                } else {
                                    return "void".to_string();
                                }
                            }
                        }
                        if id.sym == "listen" {
                            return "*Server".to_string();
                        }
                        if id.sym == "serve" {
                            return "*Server".to_string();
                        }
                        if id.sym == "connect" {
                            return "*Socket".to_string();
                        }
                        if id.sym == "fetch" {
                            return "*Response".to_string();
                        }
                        if id.sym == "json" {
                            return "*Response".to_string();
                        }
                        if id.sym == "readFileSync" {
                            return "[]const u8".to_string();
                        }
                        if id.sym == "readFile" {
                            return "[]const u8".to_string();
                        }
                        if id.sym == "randomUUID" {
                            return "[]const u8".to_string();
                        }
                        if id.sym == "hash" {
                            return "[]const u8".to_string();
                        }
                        if id.sym == "@alloc" {
                            return "[*]u8".to_string();
                        }
                        if id.sym == "self_pid" || id.sym == "sender" || id.sym == "spawn" {
                            return "PID".to_string();
                        }
                        if id.sym == "receive" {
                            return "Message".to_string();
                        }
                        if id.sym == "send" || id.sym == "reply" {
                            return "bool".to_string();
                        }
                        if id.sym == "sleep" {
                            return "void".to_string();
                        }
                    } else if let Expr::Member(mem) = &*call.callee {
                        if mem.prop.sym == "getField" {
                            return "?FieldInfo".to_string();
                        }
                        if mem.prop.sym == "getMethod" {
                            return "?MethodInfo".to_string();
                        }
                        if let Expr::Ident(obj_id) = &*mem.obj {
                            if obj_id.sym == "super" {
                                if let Some(c_name) = &self.current_class_name {
                                    if let Some((_, Some(ret_ty), _)) = self
                                        .lookup_method(c_name, &format!("__super_{}", mem.prop.sym))
                                    {
                                        return self.map_type(&ret_ty);
                                    }
                                }
                            } else if let Some(tname) = self.resolve_var_type_name(&obj_id.sym) {
                                if let Some((_, Some(ret_ty), _)) =
                                    self.lookup_method(&tname, &mem.prop.sym)
                                {
                                    return self.map_type(&ret_ty);
                                }
                            } else if self.class_defs.contains_key(&obj_id.sym) {
                                if let Some((_, Some(ret_ty), _)) =
                                    self.lookup_method(&obj_id.sym, &mem.prop.sym)
                                {
                                    return self.map_type(&ret_ty);
                                }
                            }
                        }
                        let prop = mem.prop.sym.as_str();
                        if (prop == "indexOf" || prop == "lastIndexOf" || prop == "charCodeAt")
                            && call.args.len() == 1
                        {
                            return "i32".to_string();
                        } else if prop == "includes"
                            || prop == "startsWith"
                            || prop == "endsWith"
                            || prop == "test"
                        {
                            return "bool".to_string();
                        } else if prop == "slice"
                            || prop == "substring"
                            || prop == "toLowerCase"
                            || prop == "toUpperCase"
                            || prop == "replace"
                            || prop == "match"
                            || prop == "exec"
                            || prop == "toString"
                        {
                            return "[]const u8".to_string();
                        }
                        if mem.prop.sym == "accept" {
                            return "*Socket".to_string();
                        }
                        if mem.prop.sym == "get" {
                            if let Some(mut oty) = self.infer_cp_type(&mem.obj) {
                                while let Type::BorrowType { inner, .. } = oty {
                                    oty = *inner;
                                }
                                if let Type::Map(_, val_ty) | Type::Dict(_, val_ty) = oty {
                                    return format!("?{}", self.map_type(&val_ty));
                                }
                            }
                        }
                        if mem.prop.sym == "peek"
                            || mem.prop.sym == "peek_front"
                            || mem.prop.sym == "peek_back"
                            || mem.prop.sym == "pop"
                            || mem.prop.sym == "pop_front"
                            || mem.prop.sym == "pop_back"
                            || mem.prop.sym == "min_key"
                            || mem.prop.sym == "max_key"
                            || mem.prop.sym == "min"
                            || mem.prop.sym == "max"
                            || mem.prop.sym == "insert"
                            || mem.prop.sym == "remove"
                        {
                            return "?Message".to_string();
                        }
                        if mem.prop.sym == "has"
                            || mem.prop.sym == "delete"
                            || mem.prop.sym == "is_empty"
                        {
                            return "bool".to_string();
                        }
                        if mem.prop.sym == "size" {
                            return "i32".to_string();
                        }
                        if mem.prop.sym == "len" {
                            return "usize".to_string();
                        }
                        if mem.prop.sym == "keys" || mem.prop.sym == "values" {
                            return "[]Message".to_string();
                        }
                        if mem.prop.sym == "entries" {
                            return "[]MessageEntry".to_string();
                        }
                        if mem.prop.sym == "read" {
                            return "[]const u8".to_string();
                        }
                    }
                    if let Expr::Member(mem) = &*call.callee {
                        if let Expr::Ident(id) = &*mem.obj {
                            if id.sym == "Math" {
                                return "f32".to_string();
                            }
                        }
                    }
                }

                Expr::Builtin(call) => {
                    if call.name == "@self"
                        || call.name == "@sender"
                        || call.name == "@worker_spawn"
                        || call.name == "@spawn"
                    {
                        return "PID".to_string();
                    }
                    if call.name == "@alloc" {
                        return "[*]u8".to_string();
                    }
                    if call.name == "@receive" {
                        return "Message".to_string();
                    }
                    if call.name == "@send" || call.name == "@reply" || call.name == "@not" {
                        return "bool".to_string();
                    }
                    if call.name == "@http_serve" {
                        return "*Server".to_string();
                    }
                    if call.name == "@json_stringify"
                        || call.name == "@crypto_uuid"
                        || call.name == "@crypto_hash"
                    {
                        return "[]const u8".to_string();
                    }
                    if call.name == "@json_parse" {
                        if call.args.len() > 1 {
                            if let Expr::Ident(id) = &call.args[1] {
                                return format!("*{}", id.sym);
                            }
                        }
                        return "std.json.Value".to_string();
                    }
                }
                Expr::Await(inner) => {
                    let inner_type = self.infer_zig_type(None, Some(inner));
                    if inner_type == "Message" {
                        return "Message".to_string();
                    }
                    if inner_type == "Unknown" || inner_type.is_empty() {
                        return "?Message".to_string();
                    }
                    return inner_type;
                }
                Expr::Move(inner) => {
                    return self.infer_zig_type(None, Some(inner));
                }
                Expr::Borrow(inner, _) => {
                    if let Some(cp_ty) = self.infer_cp_type(expr) {
                        return self.map_type(&cp_ty);
                    }
                    let inner_z = self.infer_zig_type(None, Some(inner));
                    if inner_z.starts_with('*') {
                        return inner_z;
                    }
                    return format!("*{}", inner_z);
                }
                Expr::Ident(id) => {
                    if id.sym == "true" || id.sym == "false" {
                        return "bool".to_string();
                    }
                    if id.sym == "@self" || id.sym == "@sender" {
                        return "PID".to_string();
                    }
                    if id.sym == "this" || id.sym == "_" {
                        if let Some(t) = self.locals.get("self") {
                            return t.clone();
                        }
                    }
                    if let Some(t) = self.locals.get(&id.sym) {
                        if self.narrowed_null_vars.contains(&id.sym) && t.starts_with('?') {
                            return t[1..].to_string();
                        }
                        return t.clone();
                    }
                    if let Some(c_name) = &self.current_class_name {
                        if let Some(cls) = self.class_defs.get(c_name) {
                            if let Some(f) = cls.fields.iter().find(|f| f.ident.sym == id.sym) {
                                return self.map_type(&f.ty);
                            }
                        }
                    }
                }
                Expr::Member(mem) => {
                    if let Some(cp_ty) = self.infer_cp_type(expr) {
                        return self.map_type(&cp_ty);
                    }
                    if let Expr::Ident(id) = &*mem.obj {
                        if id.sym == "Math" {
                            return "f32".to_string();
                        }
                        if let Some(tname) = self.resolve_var_type_name(&id.sym) {
                            if let Some(cls) = self.class_defs.get(&tname) {
                                if let Some(f) =
                                    cls.fields.iter().find(|f| f.ident.sym == mem.prop.sym)
                                {
                                    return self.map_type(&f.ty);
                                }
                            }
                        }
                    }
                    if mem.prop.sym == "node_id" || mem.prop.sym == "local_id" {
                        return "u32".to_string();
                    }
                    if mem.prop.sym == "string" {
                        return "[]const u8".to_string();
                    }
                    if mem.prop.sym == "int" {
                        return "i32".to_string();
                    }
                }
                Expr::ReflectType { .. } | Expr::ReflectOf { .. } => {
                    return "*const TypeInfo".to_string();
                }
                Expr::TypeVal { .. } => {
                    return "type".to_string();
                }
                Expr::Index { .. } => {
                    if let Some(cp_ty) = self.infer_cp_type(expr) {
                        return self.map_type(&cp_ty);
                    }
                }
                Expr::Comptime(_) | Expr::Unsafe(_) | Expr::Arena(_) => {
                    if let Some(cp_ty) = self.infer_cp_type(expr) {
                        return self.map_type(&cp_ty);
                    }
                }
                _ => {}
            }
        }

        "*anyopaque".to_string() // Fallback
    }

}
