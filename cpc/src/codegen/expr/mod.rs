mod builtin;
mod call;
mod control;
mod error_flow;
mod ops;

use super::*;
use crate::ast::*;
use crate::Span;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

impl Codegen {
    pub(crate) fn maybe_wrap_try_call(
        &mut self,
        call_code: String,
        raw_ret_ty: Option<&Type>,
    ) -> String {
        if !self.in_try_region {
            return call_code;
        }
        if let Some(Type::ResultType { err: call_err, .. }) = raw_ret_ty {
            let id = self.lambda_counter;
            self.lambda_counter += 1;
            if let Some((blk_label, err_var, flag_var, target_err)) = self.try_catch_target.clone()
            {
                self.verify_error_type_compatibility(call_err, &target_err);
                let tgt_zig = self.map_error_type(&target_err);
                return format!(
                    "(blk_try_{id}: {{ const __r_{id} = {call_code}; switch (__r_{id}) {{ .err => |__e_{id}| {{ {err_var} = cp_convert_err({tgt_zig}, __e_{id}); {flag_var} = true; break :{blk_label}; }}, .ok => |__v_{id}| break :blk_try_{id} __v_{id} }} }})"
                );
            } else if let Some(Type::ResultType { err: fn_err, .. }) =
                self.current_fn_ret_ty.clone()
            {
                self.verify_error_type_compatibility(call_err, &fn_err);
                let tgt_zig = self.map_error_type(&fn_err);
                return format!(
                    "(blk_try_{id}: {{ const __r_{id} = {call_code}; switch (__r_{id}) {{ .err => |__e_{id}| return .{{ .err = cp_convert_err({tgt_zig}, __e_{id}) }}, .ok => |__v_{id}| break :blk_try_{id} __v_{id} }} }})"
                );
            } else {
                panic!("CP Compile Error: Cannot auto-bubble error in 'try' block inside a function without '!Error' return type or 'catch' handler");
            }
        }
        call_code
    }

    pub(crate) fn lower_template_stream(&mut self, t: &TemplateLit) -> (String, Vec<String>) {
        if t.expressions.is_empty() {
            let quasi_str = if t.quasis.is_empty() {
                String::new()
            } else {
                Self::escape_zig_fmt_string(&t.quasis[0])
            };
            return (quasi_str, Vec::new());
        }

        // Constant folding if all are literals
        let all_lits = t.expressions.iter().all(|e| matches!(e, Expr::Lit(_)));
        if all_lits {
            let mut folded = String::new();
            for (idx, quasi) in t.quasis.iter().enumerate() {
                folded.push_str(quasi);
                if idx < t.expressions.len() {
                    if let Expr::Lit(lit) = &t.expressions[idx] {
                        match lit {
                            Lit::Int(n) => folded.push_str(&n.to_string()),
                            Lit::Float(f) => folded.push_str(&f.to_string()),
                            Lit::Str(s) => folded.push_str(s),
                            Lit::Bool(b) => folded.push_str(if *b { "true" } else { "false" }),
                            Lit::Null => folded.push_str("null"),
                            Lit::Undefined => folded.push_str("undefined"),
                        }
                    }
                }
            }
            return (Self::escape_zig_fmt_string(&folded), Vec::new());
        }

        let mut fmt_pattern = String::new();
        let mut args_code = Vec::new();

        for (idx, quasi) in t.quasis.iter().enumerate() {
            fmt_pattern.push_str(&Self::escape_zig_fmt_string(quasi));
            if idx < t.expressions.len() {
                fmt_pattern.push_str("{s}");
                let expr_code = self.generate_expr(&t.expressions[idx]);
                args_code.push(format!("cp_fmt({})", expr_code));
            }
        }

        (fmt_pattern, args_code)
    }

    pub(crate) fn generate_expr(&mut self, expr: &Expr) -> String {
        match expr {
            Expr::Lit(Lit::Int(v)) => v.to_string(),
            Expr::Lit(Lit::Float(f)) => {
                let s = f.to_string();
                if s.contains('.') {
                    s
                } else {
                    format!("{}.0", s)
                }
            }
            Expr::Lit(Lit::Str(s)) => format!("\"{}\"", Self::escape_zig_string(s)),
            Expr::Lit(Lit::Bool(b)) => {
                if *b {
                    "true".to_string()
                } else {
                    "false".to_string()
                }
            }
            Expr::Lit(Lit::Null) => "null".to_string(),
            Expr::Lit(Lit::Undefined) => "undefined".to_string(),

            Expr::Ident(_)
            | Expr::Borrow(..)
            | Expr::Move(_)
            | Expr::Bin(_)
            | Expr::Cast { .. }
            | Expr::BitNot(_)
            | Expr::Deref(_)
            | Expr::Unwrap(_) => self.generate_expr_ops(expr),
            Expr::Call(_) => self.generate_expr_call(expr),
            Expr::Question(_)
            | Expr::QuestionFallback { .. }
            | Expr::NullCoalesce { .. }
            | Expr::Catch { .. }
            | Expr::ErrWrap(_)
            | Expr::BoxNew(_)
            | Expr::Downcast { .. }
            | Expr::EnumInit(_) => self.generate_expr_error_flow(expr),
            Expr::OptMember(_)
            | Expr::OptCall(_)
            | Expr::Match(_)
            | Expr::If { .. }
            | Expr::New(_)
            | Expr::Await(_)
            | Expr::Member(_)
            | Expr::Arrow(_)
            | Expr::Unsafe(_)
            | Expr::ComptimeExpr(_)
            | Expr::Comptime(_)
            | Expr::Arena(_) => self.generate_expr_control(expr),
            Expr::Builtin(_) => self.generate_expr_builtin(expr),
            Expr::Object(fields) => {
                if !fields.is_empty() && fields.iter().all(|f| f.key.is_none() && !f.is_spread) {
                    let mut s = String::from(".{ ");
                    for (idx, f) in fields.iter().enumerate() {
                        s.push_str(&self.generate_expr(&f.val));
                        if idx + 1 < fields.len() {
                            s.push_str(", ");
                        }
                    }
                    s.push_str(" }");
                    return s;
                }
                let mut prop_map: Vec<(String, String)> = Vec::new();
                for field in fields {
                    if field.is_spread {
                        let base_code = self.generate_expr(&field.val);
                        let base_ty = self.infer_cp_type(&field.val);
                        if let Some(Type::Object(sub_fs)) = base_ty {
                            for sf in sub_fs {
                                let key = sf.ident.sym.clone();
                                let val_code = format!("{}.{}", base_code, key);
                                if let Some(pos) = prop_map.iter().position(|(k, _)| k == &key) {
                                    prop_map[pos] = (key, val_code);
                                } else {
                                    prop_map.push((key, val_code));
                                }
                            }
                        } else if let Some(Type::Ref(cid)) = base_ty {
                            if let Some(cls) = self.class_defs.get(&cid.sym).cloned() {
                                for fl in cls.fields {
                                    let key = fl.ident.sym.clone();
                                    let val_code = format!("{}.{}", base_code, key);
                                    if let Some(pos) = prop_map.iter().position(|(k, _)| k == &key)
                                    {
                                        prop_map[pos] = (key, val_code);
                                    } else {
                                        prop_map.push((key, val_code));
                                    }
                                }
                            }
                        }
                    } else if let Some(key) = &field.key {
                        let val_code = self.generate_expr(&field.val);
                        if let Some(pos) = prop_map.iter().position(|(k, _)| k == &key.sym) {
                            prop_map[pos] = (key.sym.clone(), val_code);
                        } else {
                            prop_map.push((key.sym.clone(), val_code));
                        }
                    }
                }
                if prop_map.is_empty() {
                    return ".{}".to_string();
                }
                let mut s = String::from(".{ ");
                for (idx, (k, v)) in prop_map.iter().enumerate() {
                    let val_out = if v == ".{}" && k == "none" {
                        "{}"
                    } else {
                        v.as_str()
                    };
                    s.push_str(&format!(".{} = {}", k, val_out));
                    if idx + 1 < prop_map.len() {
                        s.push_str(", ");
                    }
                }
                s.push_str(" }");
                s
            }
            Expr::Dict(fields) => {
                let mut sets = String::new();
                for field in fields {
                    if field.is_spread {
                        let base_code = self.generate_expr(&field.val);
                        sets.push_str(&format!("_dict.put_all({}); ", base_code));
                    } else if let Some(key) = &field.key {
                        let val_code = self.generate_expr(&field.val);
                        sets.push_str(&format!("_ = _dict.set(\"{}\", {}); ", key.sym, val_code));
                    }
                }
                format!("(blk: {{ const _dict = Dict._create({}) catch unreachable; {}break :blk _dict; }})", self.current_allocator(), sets)
            }
            Expr::Index { obj, index } => {
                self.check_read_allowed(obj);
                let obj_code = self.generate_expr(obj);
                let idx_code = self.generate_expr(index);
                let is_collection = match self.infer_cp_type(obj) {
                    Some(Type::Dict(_, _)) | Some(Type::Map(_, _)) => true,
                    Some(Type::Ref(id))
                        if id.sym == "Dict" || id.sym == "Map" || id.sym == "VecDeque" =>
                    {
                        true
                    }
                    _ => false,
                };
                if !is_collection {
                    if matches!(&**index, Expr::Lit(Lit::Int(_))) {
                        format!("{}[{}]", obj_code, idx_code)
                    } else {
                        format!("{}[@as(usize, @intCast({}))]", obj_code, idx_code)
                    }
                } else {
                    format!("cp_index_get({}, {})", obj_code, idx_code)
                }
            }
            Expr::Array(items) => {
                if items.is_empty() {
                    return "&.{}".to_string();
                }
                let mut s = String::from(".{ ");
                for (idx, item) in items.iter().enumerate() {
                    s.push_str(&self.generate_expr(item));
                    if idx < items.len() - 1 {
                        s.push_str(", ");
                    }
                }
                s.push_str(" }");
                if self.expected_array_elem_type.is_some() || self.is_comptime || self.is_lib_mode {
                    s
                } else if !items.is_empty() {
                    let elem_zig_ty = self
                        .infer_cp_type(&items[0])
                        .map(|t| self.map_type(&t))
                        .unwrap_or_else(|| "i32".to_string());
                    format!("cp_array_lit({}, &{})", elem_zig_ty, s)
                } else {
                    s
                }
            }
            Expr::ArrayRepeat(val, count) => {
                let outer_expected = self.expected_array_elem_type.clone();
                let inner_expected = match &outer_expected {
                    Some(Type::FixedArray(inner, _)) | Some(Type::Array(inner)) => {
                        Some((**inner).clone())
                    }
                    _ => None,
                };
                let old_expected =
                    std::mem::replace(&mut self.expected_array_elem_type, inner_expected);
                let val_code = self.generate_expr(val);
                self.expected_array_elem_type = None;
                let count_code = self.generate_expr(count);
                self.expected_array_elem_type = old_expected;
                let elem_zig_ty = outer_expected
                    .or_else(|| self.infer_cp_type(val))
                    .map(|t| self.map_type(&t))
                    .unwrap_or_else(|| "u8".to_string());
                format!(
                    "@as([{}]{}, @splat(@as({}, {})))",
                    count_code, elem_zig_ty, elem_zig_ty, val_code
                )
            }
            Expr::Slice(s) => {
                if let Expr::Ident(id) = &*s.obj {
                    self.uninitialized_vars.remove(&id.sym);
                }
                let obj_code = self.generate_expr(&s.obj);
                let start_code = s
                    .start
                    .as_ref()
                    .map(|e| self.generate_expr(e))
                    .unwrap_or_else(|| "0".to_string());
                let sentinel_code = s
                    .sentinel
                    .as_ref()
                    .map(|e| format!(" :{}", self.generate_expr(e)))
                    .unwrap_or_default();
                if let Some(end_expr) = &s.end {
                    let end_code = self.generate_expr(end_expr);
                    format!(
                        "{}[{}..{}{}]",
                        obj_code, start_code, end_code, sentinel_code
                    )
                } else {
                    format!("{}[{}..{}]", obj_code, start_code, sentinel_code)
                }
            }
            Expr::ReflectType { target_ty, .. } => {
                if let Type::Ref(id) = target_ty {
                    format!("_get_typeinfo_{}()", id.sym)
                } else {
                    "@panic(\"Cannot reflect non-class type\")".to_string()
                }
            }
            Expr::TypeVal { ty, .. } => self.map_type(ty),
            Expr::ReflectOf { expr, .. } => {
                let class_name = if let Expr::Ident(id) = &**expr {
                    self.resolve_var_type_name(&id.sym)
                } else if let Some(Type::Ref(id)) = self.infer_cp_type(expr) {
                    Some(id.sym.clone())
                } else {
                    None
                };
                if let Some(cname) = class_name {
                    if matches!(&**expr, Expr::Ident(_)) {
                        format!("_get_typeinfo_{}()", cname)
                    } else {
                        let e_code = self.generate_expr(expr);
                        format!(
                            "(blk: {{ _ = {}; break :blk _get_typeinfo_{}(); }})",
                            e_code, cname
                        )
                    }
                } else {
                    "@panic(\"Cannot determine type for reflect.of\")".to_string()
                }
            }
            Expr::Range { .. } => "0".to_string(),
            Expr::Template(t) => {
                if t.expressions.is_empty() {
                    if t.quasis.is_empty() {
                        return "\"\"".to_string();
                    } else {
                        return format!("\"{}\"", Self::escape_zig_string(&t.quasis[0]));
                    }
                }

                // Compile-time constant folding if all expressions are literals
                let all_lits = t.expressions.iter().all(|e| matches!(e, Expr::Lit(_)));
                if all_lits {
                    let mut folded = String::new();
                    for (idx, quasi) in t.quasis.iter().enumerate() {
                        folded.push_str(quasi);
                        if idx < t.expressions.len() {
                            if let Expr::Lit(lit) = &t.expressions[idx] {
                                match lit {
                                    Lit::Int(n) => folded.push_str(&n.to_string()),
                                    Lit::Float(f) => folded.push_str(&f.to_string()),
                                    Lit::Str(s) => folded.push_str(s),
                                    Lit::Bool(b) => {
                                        folded.push_str(if *b { "true" } else { "false" })
                                    }
                                    Lit::Null => folded.push_str("null"),
                                    Lit::Undefined => folded.push_str("undefined"),
                                }
                            }
                        }
                    }
                    return format!("\"{}\"", Self::escape_zig_string(&folded));
                }

                let mut fmt_pattern = String::new();
                let mut args_code = Vec::new();

                for (idx, quasi) in t.quasis.iter().enumerate() {
                    fmt_pattern.push_str(&Self::escape_zig_fmt_string(quasi));
                    if idx < t.expressions.len() {
                        fmt_pattern.push_str("{s}");
                        let expr_code = self.generate_expr(&t.expressions[idx]);
                        args_code.push(format!("cp_fmt({})", expr_code));
                    }
                }

                format!(
                    "(std.fmt.allocPrint({}, \"{}\", .{{ {} }}) catch unreachable)",
                    self.current_allocator(),
                    fmt_pattern,
                    args_code.join(", ")
                )
            }
            Expr::Continue(_) => "continue".to_string(),
            Expr::Break(_, val) => {
                let cur_blk = self.current_block_label.clone();
                if let Some(v) = val {
                    let vc = self.generate_expr(v);
                    if let Some(blk) = cur_blk {
                        format!("break :{} {}", blk, vc)
                    } else {
                        format!("break {}", vc)
                    }
                } else {
                    if let Some(blk) = cur_blk {
                        format!("break :{}", blk)
                    } else {
                        "break".to_string()
                    }
                }
            }
            Expr::LabeledBreak(_, label, val) => {
                if let Some(v) = val {
                    format!("break :{} {}", label.sym, self.generate_expr(v))
                } else {
                    format!("break :{}", label.sym)
                }
            }
            Expr::LabeledContinue(_, label) => {
                format!("continue :{}", label.sym)
            }
            Expr::While { stmt, else_expr } => {
                let saved = self.current_case_content.clone();
                self.current_case_content.clear();
                let old_blk = self.current_block_label.clone();
                self.current_block_label = None;
                self.generate_stmt(&Stmt::While((**stmt).clone()));
                let while_code = self.current_case_content.trim().to_string();
                self.current_case_content = saved;
                self.current_block_label = old_blk;
                if let Some(e) = else_expr {
                    format!("({} else {})", while_code, self.generate_expr(e))
                } else {
                    format!("({})", while_code)
                }
            }
            Expr::LabeledBlock { label, block } => {
                let saved = self.current_case_content.clone();
                self.current_case_content.clear();
                let len = block.body.len();
                let blk_label = label
                    .as_ref()
                    .map(|l| l.sym.clone())
                    .unwrap_or_else(|| "blk".to_string());
                let old_blk = self.current_block_label.clone();
                self.current_block_label = Some(blk_label.clone());
                for (idx, s) in block.body.iter().enumerate() {
                    if idx + 1 == len {
                        if let Stmt::Expr(e) = s {
                            match e {
                                Expr::Break(_, _)
                                | Expr::LabeledBreak(_, _, _)
                                | Expr::Return(_, _) => {
                                    self.generate_stmt(s);
                                }
                                Expr::Bin(b) if b.op == BinaryOp::Assign => {
                                    self.generate_stmt(s);
                                }
                                Expr::Builtin(b)
                                    if matches!(
                                        b.name.as_str(),
                                        "@compileError" | "@panic" | "@trap"
                                    ) =>
                                {
                                    self.generate_stmt(s);
                                }
                                _ => {
                                    let ec = self.generate_expr(e);
                                    self.write_to_case(&format!("break :{} {};\n", blk_label, ec));
                                }
                            }
                        } else {
                            self.generate_stmt(s);
                        }
                    } else {
                        self.generate_stmt(s);
                    }
                }
                self.current_block_label = old_blk;
                let body_str = self.current_case_content.clone();
                self.current_case_content = saved;
                let uses_label = body_str.contains(&format!(":{}", blk_label));
                if label.is_some() || uses_label {
                    format!("{}: {{\n{}}}", blk_label, body_str)
                } else {
                    format!("({{\n{}}})", body_str)
                }
            }
            Expr::Return(_, val) => {
                if let Some(v) = val {
                    format!("return {}", self.generate_expr(v))
                } else {
                    "return".to_string()
                }
            }
            Expr::InlineAsm {
                is_volatile,
                content,
                ..
            } => {
                let vol = if *is_volatile { " volatile" } else { "" };
                format!("asm{} ({})", vol, content)
            }
            Expr::ComptimeMap {
                target_ty, entries, ..
            } => {
                let val_zig_ty = if let Some(ty) = target_ty {
                    self.map_type(ty)
                } else if let Some(first) = entries.first() {
                    if let Some(inferred) = self.infer_cp_type(&first.val) {
                        self.map_type(&inferred)
                    } else if let Expr::Member(m) = &first.val {
                        if let Expr::Ident(id) = &*m.obj {
                            id.sym.clone()
                        } else {
                            "anytype".to_string()
                        }
                    } else if let Expr::EnumInit(ei) = &first.val {
                        if !ei.enum_name.sym.is_empty() {
                            ei.enum_name.sym.clone()
                        } else {
                            "anytype".to_string()
                        }
                    } else {
                        "anytype".to_string()
                    }
                } else {
                    "anytype".to_string()
                };
                let mut entries_str = Vec::new();
                for f in entries {
                    let key_str = if let Some(k) = &f.key { &k.sym } else { "" };
                    let val_code = self.generate_expr(&f.val);
                    entries_str.push(format!(".{{ \"{}\", {} }}", key_str, val_code));
                }
                format!(
                    "std.StaticStringMap({}).initComptime(.{{ {} }})",
                    val_zig_ty,
                    entries_str.join(", ")
                )
            }
            Expr::Zig(z) => {
                if z.code.trim() == "*const !" {
                    "*const anyopaque".to_string()
                } else {
                    self.generate_zig_or_struct(&z.code)
                }
            }
            _ => "0".to_string(),
        }
    }
}
