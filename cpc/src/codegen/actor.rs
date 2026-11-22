use super::*;
use crate::ast::*;
use crate::Span;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

impl Codegen {
    pub(crate) fn returns_borrowed_view(ty: &Type) -> bool {
        match ty {
            Type::Array(_)
            | Type::String
            | Type::CStr
            | Type::BorrowType { .. }
            | Type::RawPtr { .. }
            | Type::ManyRawPtr(_)
            | Type::CPtr(_)
            | Type::SentinelPtr(_, _)
            | Type::SentinelSlice(_, _) => true,
            Type::Ref(id)
                if id.sym == "string"
                    || id.sym == "cstring"
                    || id.sym.starts_with('[')
                    || id.sym.starts_with('*') =>
            {
                true
            }
            Type::Optional(inner) | Type::ErrorUnion(inner) => Self::returns_borrowed_view(inner),
            Type::ResultType { ok, .. } => Self::returns_borrowed_view(ok),
            _ => false,
        }
    }

    pub(crate) fn is_receiver_param_type(ty: &Type, class_name: Option<&String>) -> bool {
        match ty {
            Type::Auto | Type::Any => true,
            Type::Ref(id) => {
                if id.sym == "@This()"
                    || id.sym == "This"
                    || id.sym == "Self"
                    || id.sym == "__CpAnonStruct__"
                {
                    return true;
                }
                if let Some(c) = class_name {
                    if &id.sym == c {
                        return true;
                    }
                }
                false
            }
            Type::BorrowType { inner, .. } | Type::RawPtr { inner, .. } => {
                Self::is_receiver_param_type(inner, class_name)
            }
            _ => false,
        }
    }

    pub fn get_func_receiver_mode(func: &Function) -> ReceiverMode {
        if func.is_static {
            return ReceiverMode::Static;
        }
        if let Some(first) = func.params.first() {
            let is_receiver_name = first.ident.sym == "this"
                || first.ident.sym == "self"
                || (first.ident.sym == "_"
                    && !matches!(
                        &first.ty,
                        Type::Void
                            | Type::F32
                            | Type::F64
                            | Type::U8
                            | Type::I32
                            | Type::U32
                            | Type::U64
                            | Type::I64
                            | Type::USize
                    ));
            if is_receiver_name
                || matches!(&first.ty, Type::Ref(id) if id.sym == "@This()" || id.sym == "This" || id.sym == "Self")
            {
                if first.is_move {
                    return ReceiverMode::Move;
                } else if first.is_mut {
                    return ReceiverMode::Mut;
                } else if func.ident.sym == "init"
                    || func.ident.sym == "release"
                    || func.ident.sym == "__super_init"
                    || func.ident.sym == "__super_release"
                {
                    return ReceiverMode::Mut;
                } else {
                    return ReceiverMode::ReadOnly;
                }
            }
        }
        if func.ident.sym == "init"
            || func.ident.sym == "release"
            || func.ident.sym == "__super_init"
            || func.ident.sym == "__super_release"
        {
            if let Some(ret) = &func.return_type {
                if *ret != Type::Void {
                    return ReceiverMode::Static;
                }
            }
            return ReceiverMode::Mut;
        }
        if Self::block_contains_ident(&func.body, "this")
            || Self::block_contains_ident(&func.body, "_")
            || Self::block_contains_ident(&func.body, "self")
        {
            return ReceiverMode::ReadOnly;
        }
        ReceiverMode::ReadOnly
    }

    pub fn get_sig_receiver_mode(sig: &MethodSig) -> ReceiverMode {
        if let Some(first) = sig.params.first() {
            if first.ident.sym == "this"
                || first.ident.sym == "_"
                || first.ident.sym == "self"
                || matches!(&first.ty, Type::Ref(id) if id.sym == "@This()" || id.sym == "This" || id.sym == "Self")
            {
                if first.is_move {
                    return ReceiverMode::Move;
                } else if first.is_mut {
                    return ReceiverMode::Mut;
                } else {
                    return ReceiverMode::ReadOnly;
                }
            }
        }
        ReceiverMode::ReadOnly
    }

    pub(crate) fn prepare_decorator_stmts(
        &self,
        dec_def: &DecDef,
        dec_item: &MetaItem,
    ) -> Vec<Stmt> {
        let mut stmts = Vec::new();

        // Check if dec_def is higher-order: return fn(next) { ... } or return (next) => { ... }
        let mut is_higher_order = false;
        let mut inner_stmts = Vec::new();

        if let Some(Stmt::Return(ret_stmt)) = dec_def.body.body.last() {
            if let Some(Expr::Arrow(arrow)) = &ret_stmt.arg {
                is_higher_order = true;
                match &*arrow.body {
                    ArrowBody::Block(b) => {
                        inner_stmts = b.body.clone();
                    }
                    ArrowBody::Expr(e) => {
                        inner_stmts = vec![Stmt::Return(ReturnStmt {
                            span: Span { start: 0, end: 0 },
                            arg: Some(e.clone()),
                        })];
                    }
                }
            }
        }

        let arg_params: &[Param] = if is_higher_order {
            &dec_def.params
        } else {
            if !dec_def.params.is_empty() && dec_def.params[0].ident.sym == "next" {
                &dec_def.params[1..]
            } else {
                &dec_def.params
            }
        };

        for (idx, p) in arg_params.iter().enumerate() {
            let mut arg_val: Option<Expr> = None;
            for arg in &dec_item.args {
                match arg {
                    MetaArg::Named(id, val) if id.sym == p.ident.sym => {
                        arg_val = Some(val.clone());
                        break;
                    }
                    _ => {}
                }
            }
            if arg_val.is_none() {
                if idx < dec_item.args.len() {
                    match &dec_item.args[idx] {
                        MetaArg::Positional(val) => {
                            arg_val = Some(val.clone());
                        }
                        _ => {}
                    }
                }
            }
            if arg_val.is_none() {
                arg_val = p.default.clone();
            }

            if let Some(val) = arg_val {
                stmts.push(Stmt::Var(VarDecl {
                    span: p.span,
                    pat: Pattern::Ident(p.ident.clone()),
                    ty: Some(p.ty.clone()),
                    init: Some(val),
                    kind: VarDeclKind::Let,
                    is_threadlocal: false,
                    is_pub: false,
                    is_export: false,
                    is_comptime: false,
                    is_extern: false,
                    is_c: false,
                }));
            }
        }

        if is_higher_order {
            stmts.extend(inner_stmts);
        } else {
            stmts.extend(dec_def.body.body.clone());
        }

        stmts
    }

    pub(crate) fn generate_function_decorated(
        &mut self,
        func: &Function,
        class_name: Option<&String>,
    ) {
        let active_decs: Vec<&MetaItem> = func
            .meta
            .iter()
            .filter(|m| self.dec_defs.contains_key(&m.name.sym))
            .collect();

        let saved_const = self.const_vars.clone();
        let saved_ns = self.type_namespace_vars.clone();
        let saved_type_params = self.type_param_names.clone();

        if active_decs.is_empty() {
            self.generate_function_logic(func, class_name);
            self.const_vars = saved_const;
            self.type_namespace_vars = saved_ns;
            self.type_param_names = saved_type_params;
            return;
        }

        let raw_name = format!("_raw_{}", func.ident.sym);
        let mut raw_func = func.clone();
        raw_func.ident.sym = raw_name.clone();
        raw_func.meta.clear();

        self.generate_function_logic(&raw_func, class_name);

        let n = active_decs.len();
        let forward_params: Vec<String> = func
            .params
            .iter()
            .filter(|p| p.ident.sym != "this" && p.ident.sym != "_")
            .map(|p| p.ident.sym.clone())
            .collect();

        for i in (0..n).rev() {
            let dec_item = active_decs[i];
            let dec_def = self.dec_defs.get(&dec_item.name.sym).unwrap().clone();

            let wrapper_name = if i == 0 {
                func.ident.sym.clone()
            } else {
                format!("_wrap_{}_{}", i, func.ident.sym)
            };

            let next_target = if i == n - 1 {
                raw_name.clone()
            } else {
                format!("_wrap_{}_{}", i + 1, func.ident.sym)
            };

            let wrapper_stmts = self.prepare_decorator_stmts(&dec_def, dec_item);

            let wrapper_func = Function {
                span: func.span,
                ident: Ident {
                    span: func.ident.span,
                    sym: wrapper_name,
                },
                generic_params: func.generic_params.clone(),
                params: func.params.clone(),
                return_type: func.return_type.clone(),
                body: BlockStmt {
                    span: func.body.span,
                    body: wrapper_stmts,
                },
                is_async: func.is_async,
                is_static: func.is_static,
                is_unsafe: func.is_unsafe,
                meta: vec![],
                is_pub: func.is_pub,
            };

            let old_callee = self.current_next_callee.clone();
            let old_forward = self.current_forward_params.clone();
            self.current_next_callee = Some(next_target);
            self.current_forward_params = forward_params.clone();

            self.generate_function_logic(&wrapper_func, class_name);

            self.current_next_callee = old_callee;
            self.current_forward_params = old_forward;
        }
        self.const_vars = saved_const;
        self.type_namespace_vars = saved_ns;
        self.type_param_names = saved_type_params;
    }

    pub(crate) fn generate_function(&mut self, func: &Function) {
        self.generate_function_decorated(func, None);
    }

    pub(crate) fn code_contains_word(code: &str, word: &str) -> bool {
        if word.is_empty() {
            return false;
        }
        let bytes = code.as_bytes();
        let wbytes = word.as_bytes();
        let n = bytes.len();
        let m = wbytes.len();
        if m > n {
            return false;
        }
        for i in 0..=(n - m) {
            if &bytes[i..i + m] == wbytes {
                let prev_ok = i == 0
                    || (!bytes[i - 1].is_ascii_alphanumeric()
                        && bytes[i - 1] != b'_'
                        && bytes[i - 1] != b'.');
                let next_ok =
                    i + m == n || (!bytes[i + m].is_ascii_alphanumeric() && bytes[i + m] != b'_');
                if prev_ok && next_ok {
                    return true;
                }
            }
        }
        false
    }

    pub(crate) fn block_contains_ident(block: &BlockStmt, id: &str) -> bool {
        let mut vars = std::collections::HashSet::new();
        crate::ast_visitor::find_referenced_vars_in_block(block, &mut vars);
        if vars.contains(id) {
            return true;
        }

        for s in &block.body {
            match s {
                Stmt::Expr(Expr::Zig(z)) => {
                    if Self::code_contains_word(&z.code, id) {
                        return true;
                    }
                }
                Stmt::Block(b)
                | Stmt::UnsafeBlock(b)
                | Stmt::ComptimeBlock(b)
                | Stmt::ArenaBlock(b) => {
                    if Self::block_contains_ident(b, id) {
                        return true;
                    }
                }
                Stmt::If(i) => {
                    if let Stmt::Block(b) = &*i.cons {
                        if Self::block_contains_ident(b, id) {
                            return true;
                        }
                    }
                    if let Some(eb) = &i.alt {
                        if let Stmt::Block(b) = &**eb {
                            if Self::block_contains_ident(b, id) {
                                return true;
                            }
                        }
                    }
                }
                _ => {}
            }
        }

        false
    }

    /// Returns true only for stdlib builtin runtime stubs (`cap:io` `print`,
    /// `cap:actor` `join`, `cap:parallel` `spawn`/`join`) whose bodies merely
    /// delegate to compiler intrinsics, so user/library top-level functions
    /// sharing these names are never silently dropped.
    pub(crate) fn is_builtin_runtime_stub_fn(func: &Function) -> bool {
        let name = func.ident.sym.as_str();
        if name != "print" && name != "spawn" && name != "join" {
            return false;
        }
        if func.body.body.len() != 1 {
            return false;
        }
        let inner_expr = match &func.body.body[0] {
            Stmt::Expr(e) => e,
            Stmt::Return(r) => match &r.arg {
                Some(e) => e,
                None => return false,
            },
            _ => return false,
        };
        let unwrapped = match inner_expr {
            Expr::Await(a) => a.as_ref(),
            other => other,
        };
        let builtin_name = match unwrapped {
            Expr::Builtin(b) => b.name.as_str(),
            Expr::Call(c) => match c.callee.as_ref() {
                Expr::Ident(id) => id.sym.as_str(),
                _ => return false,
            },
            _ => return false,
        };
        match name {
            "print" => builtin_name == "@print",
            "spawn" => builtin_name == "@task_spawn",
            "join" => builtin_name == "@task_join" || builtin_name == "@yield_to_beam",
            _ => false,
        }
    }

    pub(crate) fn generate_function_logic(
        &mut self,
        raw_func: &Function,
        class_name: Option<&String>,
    ) {
        if class_name.is_none() && Self::is_builtin_runtime_stub_fn(raw_func) {
            return;
        }
        let func = raw_func;
        let zig_name = if func.ident.sym == "main" && (!self.is_lib_mode || func.is_async) {
            "cp_main".to_string()
        } else if func.ident.sym == "test" {
            "cp_test".to_string()
        } else if matches!(func.ident.sym.as_str(), "and" | "or" | "orelse") {
            format!("@\"{}\"", func.ident.sym)
        } else {
            func.ident.sym.clone()
        };
        self.current_zig_fn_name = if let Some(c) = class_name {
            format!("{}_{}", c, zig_name)
        } else {
            zig_name.clone()
        };
        self.current_fn_zig_name = zig_name.clone();
        self.drop_flag_vars.clear();
        self.current_fn_contracts_ret = func
            .meta
            .iter()
            .filter(|m| m.name.sym == "ret")
            .cloned()
            .collect();
        self.current_class_name = class_name.cloned();
        self.current_receiver_mode = if let Some(c) = class_name {
            if self.value_classes.contains(c) || self.enum_defs.contains_key(c) {
                Self::get_func_receiver_mode(func)
            } else if func.is_static {
                ReceiverMode::Static
            } else if func.ident.sym == "init"
                || func.ident.sym == "release"
                || func.ident.sym == "__super_init"
                || func.ident.sym == "__super_release"
            {
                if let Some(ret) = &func.return_type {
                    if *ret != Type::Void {
                        ReceiverMode::Static
                    } else {
                        ReceiverMode::Mut
                    }
                } else {
                    ReceiverMode::Mut
                }
            } else if let Some(first) = func.params.first().filter(|p| {
                p.ident.sym == "this"
                    || p.ident.sym == "self"
                    || (p.ident.sym == "_" && Self::is_receiver_param_type(&p.ty, class_name))
            }) {
                if first.is_move {
                    ReceiverMode::Move
                } else if first.is_mut {
                    ReceiverMode::Mut
                } else {
                    ReceiverMode::ReadOnly
                }
            } else {
                ReceiverMode::ReadOnly
            }
        } else {
            ReceiverMode::Static
        };
        self.is_class_method =
            class_name.is_some() && self.current_receiver_mode != ReceiverMode::Static;
        self.is_main = class_name.is_none()
            && func.ident.sym == "main"
            && !self.is_lib_mode
            && (!self.modular_mode || self.modular_is_entry);
        let ret_ty = func
            .return_type
            .as_ref()
            .map(|t| {
                if let Type::Ref(id) = t {
                    if func.generic_params.iter().any(|gp| match gp {
                        GenericParam::Type { ident, .. } => ident.sym == id.sym,
                        _ => false,
                    }) {
                        if let Some(param) = func.params.iter().find(|p| match &p.ty {
                            Type::Ref(pid) => pid.sym == id.sym,
                            _ => false,
                        }) {
                            return format!("@TypeOf({})", param.ident.sym);
                        }
                    }
                }
                self.map_type(t)
            })
            .unwrap_or("void".to_string());
        self.current_fn_is_async = func.is_async;

        // Initialize Lifetime & NLL tracking for this function
        self.current_scope_depth = 1;
        self.var_scope_depth.clear();
        self.var_borrow_sources.clear();
        self.var_is_ref_struct.clear();
        self.loop_borrowed_containers.clear();
        self.current_fn_params.clear();
        self.mutable_fn_params.clear();
        self.move_fn_params.clear();
        self.var_cp_types.clear();
        self.local_trampolines.clear();
        self.current_fn_ret_ty = func.return_type.clone();
        self.current_stmt_idx = 0;
        self.last_use_map.clear();
        let mut stmt_counter = 0;
        self.compute_last_uses_block(&func.body, &mut stmt_counter);

        if let Some(c) = class_name {
            if self.current_receiver_mode != ReceiverMode::Static {
                self.current_fn_params.insert("this".to_string());
                self.current_fn_params.insert("_".to_string());
                self.current_fn_params.insert("self".to_string());
                self.var_scope_depth.insert("this".to_string(), 0);
                self.var_scope_depth.insert("_".to_string(), 0);
                self.var_scope_depth.insert("self".to_string(), 0);
                let cls_ty = Type::Ref(Ident {
                    span: Span { start: 0, end: 0 },
                    sym: c.clone(),
                });
                self.var_cp_types.insert("this".to_string(), cls_ty.clone());
                self.var_cp_types.insert("_".to_string(), cls_ty);
                if self.current_receiver_mode == ReceiverMode::Mut
                    || self.current_receiver_mode == ReceiverMode::Move
                {
                    self.mutable_fn_params.insert("this".to_string());
                    self.mutable_fn_params.insert("_".to_string());
                }
                if self.current_receiver_mode == ReceiverMode::Move {
                    self.move_fn_params.insert("this".to_string());
                    self.move_fn_params.insert("_".to_string());
                }
            }
        }
        self.type_param_names.clear();
        for gp in &func.generic_params {
            if let GenericParam::Type { ident, .. } = gp {
                self.type_param_names.insert(ident.sym.clone());
                self.type_namespace_vars.insert(ident.sym.clone());
            }
        }
        for p in &func.params {
            if p.ident.sym == "this" || p.ident.sym == "_" {
                continue;
            }
            self.current_fn_params.insert(p.ident.sym.clone());
            self.var_scope_depth.insert(p.ident.sym.clone(), 0);
            self.var_cp_types.insert(p.ident.sym.clone(), p.ty.clone());
            if matches!(&p.ty, Type::Ref(id) if id.sym == "type") {
                self.type_param_names.insert(p.ident.sym.clone());
                self.type_namespace_vars.insert(p.ident.sym.clone());
            }
            if p.is_mut || p.is_move {
                self.mutable_fn_params.insert(p.ident.sym.clone());
            }
            if p.is_move {
                self.move_fn_params.insert(p.ident.sym.clone());
            }
            if let Type::Ref(cid) = &p.ty {
                if self.ref_classes.contains(&cid.sym) {
                    self.var_is_ref_struct.insert(p.ident.sym.clone());
                }
            }
        }

        if !func.is_async && !self.is_main {
            self.is_state_machine = false;
            let zig_ret_ty = ret_ty.clone();
            let mut params_str = String::new();
            self.locals.clear();
            self.moved_vars.clear();
            if let Some(c) = class_name {
                if self.current_receiver_mode != ReceiverMode::Static {
                    let is_generic = self
                        .class_defs
                        .get(c)
                        .map(|cls| !cls.generic_params.is_empty())
                        .unwrap_or(false);
                    let is_enum = self.enum_defs.contains_key(c);
                    let is_val =
                        is_enum || self.value_classes.contains(c) || is_generic || c == "@This()";
                    let type_name = if is_generic || c == "@This()" {
                        "@This()".to_string()
                    } else {
                        c.clone()
                    };
                    let needs_const_ptr_self = !is_enum
                        && self.current_receiver_mode == ReceiverMode::ReadOnly
                        && func
                            .return_type
                            .as_ref()
                            .map(Self::returns_borrowed_view)
                            .unwrap_or(false);
                    let ptr_type = if let Some(first) = func.params.first().filter(|p| {
                        p.ident.sym == "this"
                            || p.ident.sym == "self"
                            || (p.ident.sym == "_"
                                && Self::is_receiver_param_type(&p.ty, class_name))
                    }) {
                        if first.is_mut
                            || matches!(
                                &first.ty,
                                Type::BorrowType { is_mut: true, .. } | Type::RawPtr { .. }
                            )
                        {
                            format!("*{}", type_name)
                        } else if matches!(&first.ty, Type::BorrowType { is_mut: false, .. }) {
                            format!("*const {}", type_name)
                        } else if is_val
                            && !needs_const_ptr_self
                            && self.current_receiver_mode == ReceiverMode::ReadOnly
                        {
                            type_name
                        } else if self.current_receiver_mode == ReceiverMode::ReadOnly {
                            format!("*const {}", type_name)
                        } else {
                            format!("*{}", type_name)
                        }
                    } else if is_val
                        && !needs_const_ptr_self
                        && self.current_receiver_mode == ReceiverMode::ReadOnly
                    {
                        type_name
                    } else if self.current_receiver_mode == ReceiverMode::ReadOnly {
                        format!("*const {}", type_name)
                    } else {
                        format!("*{}", type_name)
                    };
                    self.locals.insert("self".to_string(), ptr_type.clone());
                    params_str.push_str(&format!("self: {}", ptr_type));
                }
            }
            let c_meta = func.meta.iter().find(|m| m.name.sym == "c");
            let is_c = c_meta.is_some() || func.meta.iter().any(|m| m.name.sym == "extern");
            let is_extern_decl = is_c && func.body.body.is_empty();
            let has_callconv_meta = func.meta.iter().any(|m| m.name.sym == "callconv");
            let explicit_noalias = func.meta.iter().any(|m| m.name.sym == "noalias");
            let has_exclusive_mut_slice = func.params.iter().any(|p| {
                !p.is_comptime
                    && matches!(
                        &p.ty,
                        Type::BorrowType {
                            is_mut: true,
                            inner,
                            ..
                        } if matches!(&**inner, Type::Array(_))
                    )
            });
            let has_readonly_slice = func.params.iter().any(|p| {
                !p.is_comptime
                    && matches!(
                        &p.ty,
                        Type::BorrowType {
                            is_mut: false,
                            inner,
                            ..
                        } if matches!(&**inner, Type::Array(_))
                    )
            });
            let fn_allows_noalias = class_name.is_none()
                && !is_c
                && !is_extern_decl
                && !has_callconv_meta
                && !func.is_unsafe
                && (explicit_noalias || (has_exclusive_mut_slice && has_readonly_slice));
            let mut mutable_copy_params: Vec<String> = Vec::new();
            let mut receiver_skipped = false;
            for (idx, p) in func.params.iter().enumerate() {
                if self.is_class_method
                    && !receiver_skipped
                    && (p.ident.sym == "this"
                        || p.ident.sym == "self"
                        || (idx == 0
                            && p.ident.sym == "_"
                            && Self::is_receiver_param_type(&p.ty, class_name)))
                {
                    receiver_skipped = true;
                    continue;
                }
                if p.ident.sym == "..." {
                    if !params_str.is_empty() {
                        params_str.push_str(", ");
                    }
                    params_str.push_str("...");
                    continue;
                }
                let mapped_ty = match &p.ty {
                    Type::Ref(id) if self.trait_defs.contains_key(&id.sym) => "anytype".to_string(),
                    Type::BorrowType { inner, .. } | Type::RawPtr { inner, .. } if matches!(&**inner, Type::Ref(id) if self.trait_defs.contains_key(&id.sym)) => {
                        "anytype".to_string()
                    }
                    Type::Ref(id)
                        if func.generic_params.iter().any(|gp| match gp {
                            GenericParam::Type { ident, .. } => ident.sym == id.sym,
                            _ => false,
                        }) =>
                    {
                        "anytype".to_string()
                    }
                    Type::BorrowType { inner, .. } | Type::RawPtr { inner, .. } if matches!(&**inner, Type::Ref(id) if func.generic_params.iter().any(|gp| matches!(gp, GenericParam::Type { ident, .. } if ident.sym == id.sym))) => {
                        "anytype".to_string()
                    }
                    Type::CStr => {
                        if p.is_mut {
                            "[*:0]u8".to_string()
                        } else {
                            "[*:0]const u8".to_string()
                        }
                    }
                    _ => self.map_type(&p.ty),
                };
                self.locals.insert(p.ident.sym.clone(), mapped_ty.clone());
                if !params_str.is_empty() {
                    params_str.push_str(", ");
                }
                let comptime_prefix = if p.is_comptime { "comptime " } else { "" };
                let needs_mut_copy = !is_extern_decl
                    && p.is_mut
                    && !p.is_comptime
                    && p.ident.sym != "this"
                    && p.ident.sym != "_"
                    && p.ident.sym != "self"
                    && !matches!(p.ty, Type::BorrowType { is_mut: true, .. });
                let emit_name = if needs_mut_copy {
                    mutable_copy_params.push(p.ident.sym.clone());
                    format!("_cp_{}", p.ident.sym)
                } else if p.ident.sym == "_"
                    && !func.is_static
                    && !self.locals.contains_key("self")
                    && (Self::block_contains_ident(&func.body, "this")
                        || Self::block_contains_ident(&func.body, "self")
                        || Self::block_contains_ident(&func.body, "_"))
                {
                    self.locals.insert("self".to_string(), mapped_ty.clone());
                    "self".to_string()
                } else if p.ident.sym == "this" {
                    self.locals.insert("self".to_string(), mapped_ty.clone());
                    "self".to_string()
                } else {
                    p.ident.sym.clone()
                };
                let noalias_prefix = if p.is_noalias
                    || (fn_allows_noalias
                        && !p.is_comptime
                        && matches!(&p.ty, Type::BorrowType { .. } | Type::Array(_))
                        && (mapped_ty.starts_with("[]") || mapped_ty.starts_with('*')))
                {
                    "noalias "
                } else {
                    ""
                };
                params_str.push_str(&format!(
                    "{}{}{}: {}",
                    comptime_prefix, noalias_prefix, emit_name, mapped_ty
                ));
            }
            self.extract_locals_simple(&func.body);
            let is_inline = func.meta.iter().any(|m| m.name.sym == "inline");
            let is_noinline = func.meta.iter().any(|m| m.name.sym == "noinline");
            let inline_kw = if is_inline {
                "inline "
            } else if is_noinline {
                "noinline "
            } else {
                ""
            };
            let c_alias = c_meta.and_then(|m| match m.args.first() {
                Some(MetaArg::Positional(Expr::Lit(Lit::Str(s)))) => Some(s.clone()),
                _ => None,
            });
            let callconv_clause =
                if let Some(m) = func.meta.iter().find(|m| m.name.sym == "callconv") {
                    let conv = match m.args.first() {
                        Some(MetaArg::Positional(Expr::Lit(Lit::Str(s)))) => {
                            if s.starts_with('.') || s.contains('.') {
                                s.clone()
                            } else {
                                format!(".{}", s)
                            }
                        }
                        Some(MetaArg::Positional(Expr::Ident(id))) => {
                            if id.sym.starts_with('.') || id.sym.contains('.') {
                                id.sym.clone()
                            } else {
                                format!(".{}", id.sym)
                            }
                        }
                        Some(MetaArg::Positional(Expr::EnumInit(ei))) => {
                            if ei.enum_name.sym.is_empty() {
                                format!(".{}", ei.variant_name.sym)
                            } else {
                                format!("{}.{}", ei.enum_name.sym, ei.variant_name.sym)
                            }
                        }
                        Some(MetaArg::Positional(Expr::Member(mem))) => {
                            format!("{}.{}", self.generate_expr(&mem.obj), mem.prop.sym)
                        }
                        _ => ".c".to_string(),
                    };
                    format!("callconv({}) ", conv)
                } else if is_c {
                    "callconv(.c) ".to_string()
                } else {
                    String::new()
                };
            let pub_prefix = if class_name.is_some() {
                if func.is_pub
                    || func.ident.sym == "init"
                    || func.ident.sym == "release"
                    || func.ident.sym == "_create"
                    || func.ident.sym.starts_with("__")
                {
                    "pub "
                } else {
                    ""
                }
            } else {
                "pub "
            };
            if is_c && func.body.body.is_empty() {
                let c_pub_prefix = if func.is_pub { "pub " } else { "" };
                if let Some(alias) = c_alias {
                    writeln!(
                        self.output,
                        "\nextern \"c\" fn {}({}) {}{};",
                        alias, params_str, callconv_clause, zig_ret_ty
                    )
                    .unwrap();
                    writeln!(
                        self.output,
                        "{}const {} = {};",
                        c_pub_prefix, zig_name, alias
                    )
                    .unwrap();
                } else {
                    writeln!(
                        self.output,
                        "\n{}extern \"c\" fn {}({}) {}{};",
                        c_pub_prefix, zig_name, params_str, callconv_clause, zig_ret_ty
                    )
                    .unwrap();
                }
                return;
            }
            let is_export = func.meta.iter().any(|m| m.name.sym == "export");
            let export_kw = if is_export { "export " } else { "" };
            self.current_func_name = Some(func.ident.sym.clone());
            self.record_source_location(func.span);
            writeln!(
                self.output,
                "\n{}{}{}fn {}({}) {}{} {{",
                pub_prefix, export_kw, inline_kw, zig_name, params_str, callconv_clause, zig_ret_ty
            )
            .unwrap();
            self.indent();
            if self.is_class_method {
                let mut is_self_used = self.current_next_callee.is_some()
                    || self.last_use_map.contains_key("self")
                    || self.last_use_map.contains_key("this")
                    || self.last_use_map.contains_key("_")
                    || Self::block_contains_ident(&func.body, "self")
                    || Self::block_contains_ident(&func.body, "this")
                    || Self::block_contains_ident(&func.body, "_");
                if !is_self_used {
                    if let Some(c_name) = &self.current_class_name {
                        if let Some(cls) = self.class_defs.get(c_name) {
                            for f in &cls.fields {
                                if Self::block_contains_ident(&func.body, &f.ident.sym) {
                                    is_self_used = true;
                                    break;
                                }
                            }
                            if !is_self_used {
                                for m in &cls.methods {
                                    if Self::block_contains_ident(&func.body, &m.ident.sym) {
                                        is_self_used = true;
                                        break;
                                    }
                                }
                            }
                            if !is_self_used {
                                if let Some(base_name) = &cls.extends {
                                    if let Some(base_cls) = self.class_defs.get(&base_name.sym) {
                                        for m in &base_cls.methods {
                                            if Self::block_contains_ident(&func.body, &m.ident.sym)
                                            {
                                                is_self_used = true;
                                                break;
                                            }
                                        }
                                        for f in &base_cls.fields {
                                            if Self::block_contains_ident(&func.body, &f.ident.sym)
                                            {
                                                is_self_used = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            if !is_self_used {
                                for tr_name in &cls.implements {
                                    if let Some(tr) = self.trait_defs.get(&tr_name.sym) {
                                        for tm in &tr.methods {
                                            if Self::block_contains_ident(&func.body, &tm.ident.sym)
                                            {
                                                is_self_used = true;
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                let is_non_c_instance_class = self
                    .current_class_name
                    .as_deref()
                    .and_then(|cn| self.class_defs.get(cn))
                    .map_or(false, |cls| {
                        let is_c_abi = cls.meta.iter().any(|m| {
                            m.name.sym == "extern"
                                || m.name.sym == "c"
                                || m.name.sym == "fix"
                                || m.name.sym == "packed"
                                || m.name.sym == "opaque"
                        });
                        !is_c_abi && !cls.is_union && !cls.is_static
                    });
                let is_cp_deinit_guarded =
                    func.ident.sym == "__cp_deinit" && is_non_c_instance_class;
                let is_cp_errdeinit_guarded =
                    func.ident.sym == "__cp_errdeinit" && is_non_c_instance_class;
                if is_cp_deinit_guarded {
                    self.writeln("if (self.__cp_dropped) return;");
                    self.writeln("self.__cp_dropped = true;");
                } else if is_cp_errdeinit_guarded {
                    self.writeln("if (self.__cp_errdropped) return;");
                    self.writeln("self.__cp_errdropped = true;");
                } else if !is_self_used {
                    self.writeln("_ = self;");
                }
                if self.current_receiver_mode == ReceiverMode::Move
                    && func.ident.sym != "__cp_deinit"
                    && func.ident.sym != "__cp_errdeinit"
                {
                    self.writeln("var __self_moved = false; _ = &__self_moved;");
                    let drop_body = Self::build_drop_call_snippet("self");
                    self.writeln(&format!(
                        "defer if (!__self_moved) {{ __self_moved = true; {} }};",
                        drop_body
                    ));
                }
            }
            for name in &mutable_copy_params {
                self.writeln(&format!("var {0} = _cp_{0}; _ = &{0};", name));
            }
            for p in &func.params {
                let emitted_name = if p.ident.sym == "this" {
                    "self"
                } else if p.ident.sym == "_" {
                    "_"
                } else {
                    &p.ident.sym
                };
                if emitted_name != "_"
                    && !(self.is_class_method && (p.ident.sym == "this" || p.ident.sym == "self"))
                {
                    let is_used = mutable_copy_params.contains(&p.ident.sym)
                        || self.last_use_map.contains_key(&p.ident.sym)
                        || self.last_use_map.contains_key(emitted_name)
                        || self
                            .current_forward_params
                            .iter()
                            .any(|s| s == &p.ident.sym || s == emitted_name)
                        || Self::block_contains_ident(&func.body, &p.ident.sym)
                        || Self::block_contains_ident(&func.body, emitted_name)
                        || Self::code_contains_word(&zig_ret_ty, &p.ident.sym)
                        || Self::code_contains_word(&zig_ret_ty, emitted_name)
                        || func.params.iter().any(|other| {
                            other.ident.sym != p.ident.sym
                                && (Self::code_contains_word(
                                    &self.map_type(&other.ty),
                                    &p.ident.sym,
                                ) || Self::code_contains_word(
                                    &self.map_type(&other.ty),
                                    emitted_name,
                                ))
                        });
                    if !is_used {
                        self.writeln(&format!("_ = {};", emitted_name));
                    }
                    if p.is_move {
                        self.drop_flag_vars.insert(p.ident.sym.clone());
                        self.writeln(&format!(
                            "var __{}_moved = false; _ = &__{}_moved;",
                            p.ident.sym, p.ident.sym
                        ));
                        let drop_body = Self::build_drop_call_snippet(&p.ident.sym);
                        self.writeln(&format!(
                            "defer if (!__{0}_moved) {{ __{0}_moved = true; {1} }};",
                            p.ident.sym, drop_body
                        ));
                    }
                }
            }
            self.dedent();
            self.indent();
            self.current_case_content.clear();
            let is_fn_arena = func.meta.iter().any(|m| m.name.sym == "arena");
            if is_fn_arena {
                let arena_id = self.tmp_counter;
                self.tmp_counter += 1;
                let arena_var = format!("__fn_arena_{}", arena_id);
                let alloc_var = format!("__fn_alloc_{}", arena_id);
                self.write_to_case(&format!(
                    "var {} = std.heap.ArenaAllocator.init(std.heap.page_allocator);\ndefer {}.deinit();\nconst {} = {}.allocator(); _ = &{};\n",
                    arena_var, arena_var, alloc_var, arena_var, alloc_var
                ));
                self.arena_allocator_stack.push(alloc_var);
            }
            for m in &func.meta {
                if m.name.sym == "pre" {
                    if let Some(crate::ast::MetaArg::Positional(expr)) = m.args.first() {
                        let cond_code = self.generate_expr(expr);
                        self.write_to_case(&format!(
                            "@import(\"std\").debug.assert({});\n",
                            cond_code
                        ));
                    }
                }
            }
            for stmt in &func.body.body {
                self.generate_stmt(stmt);
            }
            if is_fn_arena {
                self.arena_allocator_stack.pop();
            }
            let body_str = self.current_case_content.clone();
            self.current_case_content.clear();
            self.output.push_buffer(&body_str);
            if let Some(Type::ResultType { ok, err }) = &func.return_type {
                let err_str = self.map_error_type(err);
                let is_native_err_union = self.is_lib_mode
                    || err_str.contains('.')
                    || err_str == "anyerror"
                    || self.error_sets.contains_key(&err_str)
                    || err_str.starts_with('(');
                if !is_native_err_union
                    && **ok == Type::Void
                    && !body_str.trim_end().ends_with("return .{ .ok = {} };")
                {
                    self.writeln("return .{ .ok = {} };");
                }
            }
            self.dedent();
            self.writeln("}");
            return;
        }

        self.is_state_machine = true;
        let ctx_name = if let Some(c) = class_name {
            format!("{}_{}", c, zig_name)
        } else {
            zig_name.clone()
        };
        writeln!(self.output, "const {}_Context = struct {{ pc: u32 = 0, _self: PID = undefined, _parent: ?*anyopaque = null, _parent_fn: ?*const fn(*anyopaque) anyerror!void = null, arena: ?*std.heap.ArenaAllocator = null, received_msg: ?Message = null, received_sender: ?PID = null, await_result: ?Message = null,", ctx_name).unwrap();
        self.indent();
        if let Some(c) = class_name {
            if self.current_receiver_mode != ReceiverMode::Static {
                self.writeln(&format!("self: *{},", c));
            }
        }
        self.pc_counter = 0;
        self.tmp_counter = 0;
        self.cases.clear();
        self.current_case_content.clear();
        self.locals.clear();
        self.moved_vars.clear();
        if let Some(c) = class_name {
            if self.current_receiver_mode != ReceiverMode::Static {
                self.locals.insert("self".to_string(), format!("*{}", c));
            }
        }
        for p in &func.params {
            if p.ident.sym == "this" || p.ident.sym == "_" {
                continue;
            }
            self.locals
                .insert(p.ident.sym.clone(), self.map_type(&p.ty));
        }
        self.extract_locals_simple(&func.body);

        self.extract_local_vars(&func.body);
        self.dedent();
        self.writeln("};\n");

        if self.is_state_machine {
            let mut init_params = String::from("arena: *std.heap.ArenaAllocator, parent: ?*anyopaque, parent_fn: ?*const fn(*anyopaque) anyerror!void");
            if let Some(c) = class_name {
                if self.current_receiver_mode != ReceiverMode::Static {
                    init_params.push_str(&format!(", self_arg: *{}", c));
                }
            }
            for p in &func.params {
                if p.ident.sym == "this" || p.ident.sym == "_" || p.ident.sym == "self" {
                    continue;
                }
                init_params.push_str(&format!(", arg_{}: {}", p.ident.sym, self.map_type(&p.ty)));
            }
            writeln!(
                self.output,
                "pub fn {}_init({}) *{}_Context {{",
                ctx_name, init_params, ctx_name
            )
            .unwrap();
            writeln!(
                self.output,
                "    const ctx = arena.allocator().create({}_Context) catch unreachable;",
                ctx_name
            )
            .unwrap();
            writeln!(self.output, "    ctx.* = undefined; ctx.pc = 0; ctx._self = PID{{ .node_id = {0}, .local_id = {1} }}; {1} += 1; ctx.received_msg = null; ctx.received_sender = null; ctx.arena = arena; ctx._parent = parent; ctx._parent_fn = parent_fn;", self.rt_sym("current_node_id"), self.rt_sym("next_local_id")).unwrap();
            if self.is_class_method {
                writeln!(self.output, "    ctx.self = self_arg;").unwrap();
            }
            for p in &func.params {
                if p.ident.sym == "this" || p.ident.sym == "_" || p.ident.sym == "self" {
                    continue;
                }
                writeln!(
                    self.output,
                    "    ctx.{} = arg_{};",
                    p.ident.sym, p.ident.sym
                )
                .unwrap();
            }
            writeln!(self.output, "    return ctx;\n}}\n").unwrap();
        }

        for stmt in &func.body.body {
            self.generate_stmt(stmt);
        }
        if self.is_state_machine {
            if !self.current_case_content.contains("return;") {
                self.write_to_case("if (ctx._parent_fn) |pfn| { schedule(pfn, ctx._parent.?); }");
                self.write_to_case("return;");
            }
        }
        self.emit_current_case();

        if self.is_beam_mode && self.is_main {
            self.writeln(&format!("\nexport fn {}(ptr: *anyopaque) void {{ cp_main_internal(ptr) catch unreachable; }}", zig_name));
            self.writeln(&format!(
                "fn {}_internal(ptr: *anyopaque) anyerror!void {{",
                zig_name
            ));
        } else if self.is_beam_mode {
            writeln!(
                self.output,
                "\npub fn {}(ptr: *anyopaque) anyerror!beam.Term {{",
                zig_name
            )
            .unwrap();
        } else {
            writeln!(
                self.output,
                "\npub fn {}(ptr: *anyopaque) anyerror!void {{",
                zig_name
            )
            .unwrap();
        }
        self.indent();
        self.writeln(&format!(
            "const ctx: *{0}_Context = @ptrCast(@alignCast(ptr));",
            ctx_name
        ));
        self.writeln("while (true) { switch (ctx.pc) {");
        self.indent();
        for case in &self.cases {
            self.output.push_str(case);
        }
        if self.is_main {
            self.writeln("else => return,");
        } else if self.is_beam_mode {
            self.writeln(r#"else => return beam.Beam.makeAtom(ctx.self.env.?, "ok"),"#);
        } else {
            self.writeln("else => unreachable,");
        }
        self.dedent();
        self.writeln("} }");
        self.dedent();
        self.writeln("}");
        self.is_state_machine = false;
    }

    pub(crate) fn emit_current_case(&mut self) {
        let mut case_str = String::new();
        let indent = "    ".repeat(self.indent_level);
        writeln!(case_str, "{}{} => {{", indent, self.pc_counter).unwrap();
        case_str.push_str(&self.current_case_content);
        let trimmed = self.current_case_content.trim_end();
        if self.is_state_machine
            && !trimmed.ends_with("return;")
            && !trimmed.ends_with("continue;")
            && !trimmed.ends_with("return")
        {
            if self.is_main {
                writeln!(case_str, "{}    return;", indent).unwrap();
            } else if self.is_beam_mode {
                writeln!(
                    case_str,
                    "{}    return beam.Beam.makeAtom(ctx.self.env.?, \"ok\");",
                    indent
                )
                .unwrap();
            } else {
                writeln!(
                    case_str,
                    "{}    if (ctx._parent) |p| {{ schedule(ctx._parent_fn.?, p); }} return;",
                    indent
                )
                .unwrap();
            }
        }
        writeln!(case_str, "{}}},", indent).unwrap();
        self.cases.push(case_str);
        self.current_case_content.clear();
    }

    pub(crate) fn next_state(&mut self) -> u32 {
        self.emit_current_case();
        self.pc_counter += 1;
        self.pc_counter
    }

    pub(crate) fn collect_stmt_error_types(&self, stmt: &Stmt, out: &mut Vec<ErrorType>) {
        match stmt {
            Stmt::Var(v) => {
                if let Some(init) = &v.init {
                    self.collect_expr_error_types(init, out);
                }
            }
            Stmt::Expr(e) => self.collect_expr_error_types(e, out),
            Stmt::Return(r) => {
                if let Some(a) = &r.arg {
                    self.collect_expr_error_types(a, out);
                }
            }
            Stmt::Break | Stmt::Continue => {}
            Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
                for s in &b.body {
                    self.collect_stmt_error_types(s, out);
                }
            }
            Stmt::If(i) => {
                self.collect_expr_error_types(&i.test, out);
                self.collect_stmt_error_types(&i.cons, out);
                if let Some(alt) = &i.alt {
                    self.collect_stmt_error_types(alt, out);
                }
            }
            Stmt::While(w) => {
                self.collect_expr_error_types(&w.test, out);
                if let Some(c) = &w.continue_expr {
                    self.collect_expr_error_types(c, out);
                }
                self.collect_stmt_error_types(&w.body, out);
            }
            Stmt::For(f) => {
                self.collect_expr_error_types(&f.expr, out);
                self.collect_stmt_error_types(&f.body, out);
            }
            Stmt::Func(f) => {
                for s in &f.body.body {
                    self.collect_stmt_error_types(s, out);
                }
            }
            _ => {}
        }
    }

    pub(crate) fn collect_expr_error_types(&self, expr: &Expr, out: &mut Vec<ErrorType>) {
        match expr {
            Expr::Call(c) => {
                if let Expr::Ident(fid) = &*c.callee {
                    if let Some(sig) = self.fn_sigs.get(&fid.sym) {
                        if let Some(Type::ResultType { err, .. }) = &sig.return_type {
                            if !out.contains(err) {
                                out.push(err.clone());
                            }
                        }
                    } else if let Some(c_name) = &self.current_class_name {
                        if let Some((_, Some(Type::ResultType { err, .. }), _)) =
                            self.lookup_method(c_name, &fid.sym)
                        {
                            if !out.contains(&err) {
                                out.push(err);
                            }
                        }
                    }
                } else if let Expr::Member(mem) = &*c.callee {
                    if let Expr::Ident(oid) = &*mem.obj {
                        if let Some(tname) = self.resolve_var_type_name(&oid.sym) {
                            if let Some((_, Some(Type::ResultType { err, .. }), _)) =
                                self.lookup_method(&tname, &mem.prop.sym)
                            {
                                if !out.contains(&err) {
                                    out.push(err);
                                }
                            }
                        }
                    }
                }
                for a in &c.args {
                    self.collect_expr_error_types(a, out);
                }
            }
            Expr::Question(inner) => self.collect_expr_error_types(inner, out),
            Expr::Bin(b) => {
                self.collect_expr_error_types(&b.left, out);
                self.collect_expr_error_types(&b.right, out);
            }
            _ => {}
        }
    }

    pub(crate) fn extract_locals_from_expr_simple(&mut self, expr: &Expr) {
        match expr {
            Expr::Arena(b) | Expr::Unsafe(b) | Expr::Comptime(b) => {
                self.extract_locals_simple(b);
            }
            Expr::Bin(bin) => {
                self.extract_locals_from_expr_simple(&bin.left);
                self.extract_locals_from_expr_simple(&bin.right);
            }
            Expr::Call(c) | Expr::OptCall(c) | Expr::New(c) => {
                self.extract_locals_from_expr_simple(&c.callee);
                for a in &c.args {
                    self.extract_locals_from_expr_simple(a);
                }
            }
            Expr::Deref(inner)
            | Expr::Unwrap(inner)
            | Expr::Question(inner)
            | Expr::Cast { expr: inner, .. } => {
                self.extract_locals_from_expr_simple(inner);
            }
            Expr::Member(m) => {
                self.extract_locals_from_expr_simple(&m.obj);
            }
            Expr::Array(items) => {
                for item in items {
                    self.extract_locals_from_expr_simple(item);
                }
            }
            _ => {}
        }
    }

    pub(crate) fn extract_locals_simple(&mut self, block: &BlockStmt) {
        for stmt in &block.body {
            match stmt {
                Stmt::Var(v) => {
                    let old_box = self.expected_box_trait.clone();
                    if let Some(Type::BoxDyn(tr_id)) = &v.ty {
                        self.expected_box_trait = Some(tr_id.sym.clone());
                    }
                    let ty_str = self.infer_zig_type(v.ty.as_ref(), v.init.as_ref());
                    match &v.pat {
                        Pattern::Ident(i) => {
                            self.locals.insert(i.sym.clone(), ty_str);
                            if let Some(t) = &v.ty {
                                self.var_cp_types.insert(i.sym.clone(), t.clone());
                            } else if let Some(init_expr) = &v.init {
                                if let Some(inferred_ty) = self.infer_cp_type(init_expr) {
                                    self.var_cp_types.insert(i.sym.clone(), inferred_ty);
                                }
                            }
                        }
                        Pattern::Tuple(el) => {
                            let init_cp_ty = v.init.as_ref().and_then(|e| self.infer_cp_type(e));
                            let mut resolved_ty = v.ty.as_ref().or(init_cp_ty.as_ref()).cloned();
                            while let Some(Type::BorrowType { inner, .. }) = resolved_ty {
                                resolved_ty = Some(*inner);
                            }
                            let rest_pos = el.iter().position(|p| matches!(p, Pattern::Rest(_)));
                            match &resolved_ty {
                                Some(Type::Tuple(tys)) => {
                                    for (idx, p) in el.iter().enumerate() {
                                        match p {
                                            Pattern::Ident(i) => {
                                                let ty_idx = if let Some(r_idx) = rest_pos {
                                                    if idx < r_idx {
                                                        Some(idx)
                                                    } else {
                                                        let offset = el.len() - idx;
                                                        tys.len().checked_sub(offset)
                                                    }
                                                } else {
                                                    Some(idx)
                                                };
                                                let t_str = ty_idx
                                                    .and_then(|ti| tys.get(ti))
                                                    .map(|t| self.map_type(t))
                                                    .unwrap_or_else(|| "[]const u8".to_string());
                                                self.locals.insert(i.sym.clone(), t_str);
                                                if let Some(ti) = ty_idx {
                                                    if let Some(t) = tys.get(ti) {
                                                        self.var_cp_types
                                                            .insert(i.sym.clone(), t.clone());
                                                    }
                                                }
                                            }
                                            Pattern::Rest(inner) => {
                                                if let Pattern::Ident(i) = &**inner {
                                                    let elem_ty = tys
                                                        .get(idx)
                                                        .cloned()
                                                        .or_else(|| tys.first().cloned())
                                                        .unwrap_or(Type::I32);
                                                    let is_const =
                                                        v.kind == crate::ast::VarDeclKind::Const;
                                                    let arr_ty = if is_const {
                                                        Type::BorrowType {
                                                            inner: Box::new(Type::Array(Box::new(
                                                                elem_ty,
                                                            ))),
                                                            is_mut: false,
                                                            lifetimes: vec![],
                                                        }
                                                    } else {
                                                        Type::Array(Box::new(elem_ty))
                                                    };
                                                    self.locals.insert(
                                                        i.sym.clone(),
                                                        self.map_type(&arr_ty),
                                                    );
                                                    self.var_cp_types.insert(i.sym.clone(), arr_ty);
                                                }
                                            }
                                            _ => {}
                                        }
                                    }
                                }
                                Some(Type::Object(fields)) => {
                                    for (idx, p) in el.iter().enumerate() {
                                        match p {
                                            Pattern::Ident(i) => {
                                                let ty_idx = if let Some(r_idx) = rest_pos {
                                                    if idx < r_idx {
                                                        Some(idx)
                                                    } else {
                                                        let offset = el.len() - idx;
                                                        fields.len().checked_sub(offset)
                                                    }
                                                } else {
                                                    Some(idx)
                                                };
                                                let t_str = ty_idx
                                                    .and_then(|ti| fields.get(ti))
                                                    .map(|f| self.map_type(&f.ty))
                                                    .unwrap_or_else(|| "[]const u8".to_string());
                                                self.locals.insert(i.sym.clone(), t_str);
                                                if let Some(ti) = ty_idx {
                                                    if let Some(f) = fields.get(ti) {
                                                        self.var_cp_types
                                                            .insert(i.sym.clone(), f.ty.clone());
                                                    }
                                                }
                                            }
                                            Pattern::Rest(inner) => {
                                                if let Pattern::Ident(i) = &**inner {
                                                    let elem_ty = fields
                                                        .get(idx)
                                                        .map(|f| f.ty.clone())
                                                        .or_else(|| {
                                                            fields.first().map(|f| f.ty.clone())
                                                        })
                                                        .unwrap_or(Type::I32);
                                                    let is_const =
                                                        v.kind == crate::ast::VarDeclKind::Const;
                                                    let arr_ty = if is_const {
                                                        Type::BorrowType {
                                                            inner: Box::new(Type::Array(Box::new(
                                                                elem_ty,
                                                            ))),
                                                            is_mut: false,
                                                            lifetimes: vec![],
                                                        }
                                                    } else {
                                                        Type::Array(Box::new(elem_ty))
                                                    };
                                                    self.locals.insert(
                                                        i.sym.clone(),
                                                        self.map_type(&arr_ty),
                                                    );
                                                    self.var_cp_types.insert(i.sym.clone(), arr_ty);
                                                }
                                            }
                                            _ => {}
                                        }
                                    }
                                }
                                Some(Type::Array(elem_ty)) => {
                                    let t_str = self.map_type(elem_ty);
                                    let arr_ty = Type::Array(elem_ty.clone());
                                    let arr_t_str = self.map_type(&arr_ty);
                                    for p in el {
                                        match p {
                                            Pattern::Ident(i) => {
                                                self.locals.insert(i.sym.clone(), t_str.clone());
                                                self.var_cp_types
                                                    .insert(i.sym.clone(), (**elem_ty).clone());
                                            }
                                            Pattern::Rest(inner) => {
                                                if let Pattern::Ident(i) = &**inner {
                                                    let is_const =
                                                        v.kind == crate::ast::VarDeclKind::Const;
                                                    let arr_ty = if is_const {
                                                        Type::BorrowType {
                                                            inner: Box::new(Type::Array(
                                                                elem_ty.clone(),
                                                            )),
                                                            is_mut: false,
                                                            lifetimes: vec![],
                                                        }
                                                    } else {
                                                        Type::Array(elem_ty.clone())
                                                    };
                                                    let arr_t_str = self.map_type(&arr_ty);
                                                    self.locals.insert(i.sym.clone(), arr_t_str);
                                                    self.var_cp_types.insert(i.sym.clone(), arr_ty);
                                                }
                                            }
                                            _ => {}
                                        }
                                    }
                                }
                                _ => {
                                    for p in el {
                                        match p {
                                            Pattern::Ident(i) => {
                                                self.locals.insert(
                                                    i.sym.clone(),
                                                    "[]const u8".to_string(),
                                                );
                                            }
                                            Pattern::Rest(inner) => {
                                                if let Pattern::Ident(i) = &**inner {
                                                    self.locals.insert(
                                                        i.sym.clone(),
                                                        "[]const Message".to_string(),
                                                    );
                                                }
                                            }
                                            _ => {}
                                        }
                                    }
                                }
                            }
                        }
                        Pattern::Object(fields) => {
                            let whole_is_mut = matches!(&v.init, Some(Expr::Borrow(_, true)));
                            let base_init = match &v.init {
                                Some(Expr::Move(inner)) | Some(Expr::Borrow(inner, _)) => {
                                    Some(&**inner)
                                }
                                other => other.as_ref(),
                            };
                            let init_cp_ty = base_init.and_then(|e| self.infer_cp_type(e));
                            let mut resolved_ty = v.ty.as_ref().or(init_cp_ty.as_ref()).cloned();
                            while let Some(Type::BorrowType { inner, .. }) = resolved_ty {
                                resolved_ty = Some(*inner);
                            }
                            let obj_fields: Option<Vec<Field>> = match &resolved_ty {
                                Some(Type::Object(fs)) => Some(fs.clone()),
                                Some(Type::Ref(cname)) => {
                                    self.class_defs.get(&cname.sym).map(|c| c.fields.clone())
                                }
                                _ => None,
                            };
                            let non_rest_keys: HashSet<String> = fields
                                .iter()
                                .filter(|p| !p.is_rest)
                                .map(|p| p.key.sym.clone())
                                .collect();

                            for p in fields {
                                if p.key.sym == "*" || matches!(p.val, Some(Pattern::Wildcard)) {
                                    continue;
                                }
                                if p.is_rest {
                                    let remaining_fields = if let Some(ref tys) = obj_fields {
                                        tys.iter()
                                            .filter(|t| !non_rest_keys.contains(&t.ident.sym))
                                            .cloned()
                                            .collect::<Vec<_>>()
                                    } else {
                                        vec![]
                                    };
                                    let rest_cp_ty = Type::Object(remaining_fields);
                                    let t_str = self.map_type(&rest_cp_ty);
                                    self.locals.insert(p.key.sym.clone(), t_str);
                                    self.var_cp_types.insert(p.key.sym.clone(), rest_cp_ty);
                                    continue;
                                }

                                let ident = if let Some(Pattern::Ident(i)) = &p.val {
                                    i
                                } else {
                                    &p.key
                                };
                                let f_opt = obj_fields
                                    .as_ref()
                                    .and_then(|tys| tys.iter().find(|t| t.ident.sym == p.key.sym));
                                let is_mut_field = p.mode == DestructureMode::MutBorrow
                                    || (p.mode == DestructureMode::ReadOnly && whole_is_mut);
                                let bound_cp_ty = f_opt.map(|f| {
                                    if is_mut_field {
                                        Type::BorrowType {
                                            inner: Box::new(f.ty.clone()),
                                            is_mut: true,
                                            lifetimes: vec![],
                                        }
                                    } else {
                                        f.ty.clone()
                                    }
                                });
                                let t_str = bound_cp_ty
                                    .as_ref()
                                    .map(|t| self.map_type(t))
                                    .unwrap_or_else(|| {
                                        if is_mut_field {
                                            "*i32".to_string()
                                        } else {
                                            "i32".to_string()
                                        }
                                    });
                                self.locals.insert(ident.sym.clone(), t_str);
                                if let Some(bt) = bound_cp_ty {
                                    self.var_cp_types.insert(ident.sym.clone(), bt);
                                }
                            }
                        }
                        _ => {}
                    }
                    self.expected_box_trait = old_box;
                    if let Some(init) = &v.init {
                        self.extract_locals_from_expr_simple(init);
                    }
                }
                Stmt::Expr(e) => {
                    if let Expr::Match(m) = e {
                        let disc_ty = self.infer_cp_type(&m.disc);
                        for arm in &m.arms {
                            let old_v = self.var_cp_types.clone();
                            if let Pattern::Err(inner_p) = &arm.pat {
                                if let Pattern::Ident(id) = &**inner_p {
                                    if let Some(Type::ResultType { err, .. }) = &disc_ty {
                                        self.var_cp_types.insert(
                                            id.sym.clone(),
                                            self.error_type_to_cp_type(err),
                                        );
                                    }
                                }
                            } else if let Pattern::Ident(id) = &arm.pat {
                                if let Some(Type::ResultType { ok, .. }) = &disc_ty {
                                    self.var_cp_types.insert(id.sym.clone(), (**ok).clone());
                                } else if let Some(t) = &disc_ty {
                                    self.var_cp_types.insert(id.sym.clone(), t.clone());
                                }
                            }
                            if let Stmt::Block(b) | Stmt::ArenaBlock(b) = &*arm.body {
                                self.extract_locals_simple(b);
                            }
                            for (k, v) in old_v {
                                self.var_cp_types.insert(k, v);
                            }
                        }
                    } else {
                        self.extract_locals_from_expr_simple(e);
                    }
                }
                Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
                    self.extract_locals_simple(b)
                }
                Stmt::If(i) => {
                    let old_v = self.var_cp_types.clone();
                    let old_l = self.locals.clone();
                    if let Some(cap) = &i.capture {
                        let test_cp_ty = self.infer_cp_type(&i.test);
                        let unwrapped_ty = match &test_cp_ty {
                            Some(Type::Optional(inner)) => (**inner).clone(),
                            Some(Type::ErrorUnion(inner)) => (**inner).clone(),
                            Some(Type::ResultType { ok, .. }) => (**ok).clone(),
                            Some(other) => other.clone(),
                            None => Type::Any,
                        };
                        let final_ty = if i.is_ptr_capture {
                            Type::BorrowType {
                                is_mut: true,
                                inner: Box::new(unwrapped_ty.clone()),
                                lifetimes: vec![],
                            }
                        } else {
                            unwrapped_ty.clone()
                        };
                        let zig_ty = self.map_type(&final_ty);
                        self.var_cp_types.insert(cap.sym.clone(), final_ty);
                        self.locals.insert(cap.sym.clone(), zig_ty);
                    }
                    if let Stmt::Block(b) | Stmt::ArenaBlock(b) = &*i.cons {
                        self.extract_locals_simple(b);
                    }
                    if let Some(alt) = &i.alt {
                        if let Stmt::Block(b) | Stmt::ArenaBlock(b) = &**alt {
                            self.extract_locals_simple(b);
                        }
                    }
                    self.var_cp_types = old_v;
                    self.locals = old_l;
                }
                Stmt::While(w) => {
                    let old_v = self.var_cp_types.clone();
                    let old_l = self.locals.clone();
                    if let Some(cap) = &w.capture {
                        let test_cp_ty = self.infer_cp_type(&w.test);
                        let unwrapped_ty = match &test_cp_ty {
                            Some(Type::Optional(inner)) => (**inner).clone(),
                            Some(Type::ErrorUnion(inner)) => (**inner).clone(),
                            Some(Type::ResultType { ok, .. }) => (**ok).clone(),
                            Some(other) => other.clone(),
                            None => Type::Any,
                        };
                        let final_ty = if w.is_ptr_capture {
                            Type::BorrowType {
                                is_mut: true,
                                inner: Box::new(unwrapped_ty.clone()),
                                lifetimes: vec![],
                            }
                        } else {
                            unwrapped_ty.clone()
                        };
                        let zig_ty = self.map_type(&final_ty);
                        self.var_cp_types.insert(cap.sym.clone(), final_ty);
                        self.locals.insert(cap.sym.clone(), zig_ty);
                    }
                    if let Stmt::Block(b) | Stmt::ArenaBlock(b) = &*w.body {
                        self.extract_locals_simple(b);
                    }
                    self.var_cp_types = old_v;
                    self.locals = old_l;
                }
                Stmt::For(f) => {
                    if let Stmt::Block(b) | Stmt::ArenaBlock(b) = &*f.body {
                        self.extract_locals_simple(b);
                    }
                }
                Stmt::TryCatch {
                    try_block,
                    catch_body,
                    ..
                } => {
                    let old_try = self.in_try_region;
                    self.in_try_region = true;
                    self.extract_locals_simple(try_block);
                    self.in_try_region = old_try;
                    if let Some(cb) = catch_body {
                        if let Stmt::Block(b) | Stmt::ArenaBlock(b) = &**cb {
                            self.extract_locals_simple(b);
                        }
                    }
                }
                Stmt::Match(m) => {
                    let disc_ty = self.infer_cp_type(&m.disc);
                    for arm in &m.arms {
                        let old_v = self.var_cp_types.clone();
                        if let Pattern::Err(inner_p) = &arm.pat {
                            if let Pattern::Ident(id) = &**inner_p {
                                if let Some(Type::ResultType { err, .. }) = &disc_ty {
                                    self.var_cp_types
                                        .insert(id.sym.clone(), self.error_type_to_cp_type(err));
                                }
                            }
                        } else if let Pattern::Ident(id) = &arm.pat {
                            if let Some(Type::ResultType { ok, .. }) = &disc_ty {
                                self.var_cp_types.insert(id.sym.clone(), (**ok).clone());
                            } else if let Some(t) = &disc_ty {
                                self.var_cp_types.insert(id.sym.clone(), t.clone());
                            }
                        }
                        if let Stmt::Block(b) | Stmt::ArenaBlock(b) = &*arm.body {
                            self.extract_locals_simple(b);
                        }
                        for (k, v) in old_v {
                            self.var_cp_types.insert(k, v);
                        }
                    }
                }
                _ => {}
            }
        }
    }

    pub(crate) fn extract_local_vars_from_expr_to_str(&mut self, expr: &Expr, output: &mut String) {
        match expr {
            Expr::Arena(b) | Expr::Unsafe(b) | Expr::Comptime(b) => {
                self.extract_local_vars_to_str(b, output);
            }
            Expr::Bin(bin) => {
                self.extract_local_vars_from_expr_to_str(&bin.left, output);
                self.extract_local_vars_from_expr_to_str(&bin.right, output);
            }
            Expr::Call(c) | Expr::OptCall(c) | Expr::New(c) => {
                self.extract_local_vars_from_expr_to_str(&c.callee, output);
                for a in &c.args {
                    self.extract_local_vars_from_expr_to_str(a, output);
                }
            }
            Expr::Deref(inner)
            | Expr::Unwrap(inner)
            | Expr::Question(inner)
            | Expr::Cast { expr: inner, .. } => {
                self.extract_local_vars_from_expr_to_str(inner, output);
            }
            Expr::Member(m) => {
                self.extract_local_vars_from_expr_to_str(&m.obj, output);
            }
            Expr::Array(items) => {
                for item in items {
                    self.extract_local_vars_from_expr_to_str(item, output);
                }
            }
            _ => {}
        }
    }

    pub(crate) fn extract_local_vars_to_str(&mut self, block: &BlockStmt, output: &mut String) {
        for stmt in &block.body {
            match stmt {
                Stmt::Var(var) => {
                    let ty_str = self.infer_zig_type(var.ty.as_ref(), var.init.as_ref());
                    if matches!(&var.pat, Pattern::Wildcard)
                        || matches!(&var.pat, Pattern::Ident(ident) if ident.sym == "_")
                    {
                        continue;
                    }
                    match &var.pat {
                        Pattern::Ident(ident) => {
                            self.locals.insert(ident.sym.clone(), ty_str.clone());
                            if let Some(t) = var.ty.as_ref() {
                                self.var_cp_types.insert(ident.sym.clone(), t.clone());
                            } else if let Some(init_e) = &var.init {
                                if let Some(inferred_ty) = self.infer_cp_type(init_e) {
                                    self.var_cp_types.insert(ident.sym.clone(), inferred_ty);
                                }
                            }
                            writeln!(output, "    {}: {},", ident.sym, ty_str).unwrap();
                        }
                        Pattern::Tuple(el) => {
                            let init_cp_ty = var.init.as_ref().and_then(|e| self.infer_cp_type(e));
                            let mut resolved_ty = var.ty.as_ref().or(init_cp_ty.as_ref()).cloned();
                            while let Some(Type::BorrowType { inner, .. }) = resolved_ty {
                                resolved_ty = Some(*inner);
                            }
                            let rest_pos = el.iter().position(|p| matches!(p, Pattern::Rest(_)));
                            match &resolved_ty {
                                Some(Type::Tuple(tys)) => {
                                    for (idx, p) in el.iter().enumerate() {
                                        match p {
                                            Pattern::Ident(ident) => {
                                                let ty_idx = if let Some(r_idx) = rest_pos {
                                                    if idx < r_idx {
                                                        Some(idx)
                                                    } else {
                                                        let offset = el.len() - idx;
                                                        tys.len().checked_sub(offset)
                                                    }
                                                } else {
                                                    Some(idx)
                                                };
                                                let t_str = ty_idx
                                                    .and_then(|ti| tys.get(ti))
                                                    .map(|t| self.map_type(t))
                                                    .unwrap_or_else(|| "[]const u8".to_string());
                                                self.locals
                                                    .insert(ident.sym.clone(), t_str.clone());
                                                writeln!(output, "    {}: {},", ident.sym, t_str)
                                                    .unwrap();
                                            }
                                            Pattern::Rest(inner) => {
                                                if let Pattern::Ident(ident) = &**inner {
                                                    let elem_ty = tys
                                                        .get(idx)
                                                        .cloned()
                                                        .or_else(|| tys.first().cloned())
                                                        .unwrap_or(Type::I32);
                                                    let is_const =
                                                        var.kind == crate::ast::VarDeclKind::Const;
                                                    let arr_ty = if is_const {
                                                        Type::BorrowType {
                                                            inner: Box::new(Type::Array(Box::new(
                                                                elem_ty,
                                                            ))),
                                                            is_mut: false,
                                                            lifetimes: vec![],
                                                        }
                                                    } else {
                                                        Type::Array(Box::new(elem_ty))
                                                    };
                                                    let t_str = self.map_type(&arr_ty);
                                                    self.locals
                                                        .insert(ident.sym.clone(), t_str.clone());
                                                    writeln!(
                                                        output,
                                                        "    {}: {},",
                                                        ident.sym, t_str
                                                    )
                                                    .unwrap();
                                                }
                                            }
                                            _ => {}
                                        }
                                    }
                                }
                                Some(Type::Object(fields)) => {
                                    for (idx, p) in el.iter().enumerate() {
                                        match p {
                                            Pattern::Ident(ident) => {
                                                let ty_idx = if let Some(r_idx) = rest_pos {
                                                    if idx < r_idx {
                                                        Some(idx)
                                                    } else {
                                                        let offset = el.len() - idx;
                                                        fields.len().checked_sub(offset)
                                                    }
                                                } else {
                                                    Some(idx)
                                                };
                                                let t_str = ty_idx
                                                    .and_then(|ti| fields.get(ti))
                                                    .map(|f| self.map_type(&f.ty))
                                                    .unwrap_or_else(|| "[]const u8".to_string());
                                                self.locals
                                                    .insert(ident.sym.clone(), t_str.clone());
                                                writeln!(output, "    {}: {},", ident.sym, t_str)
                                                    .unwrap();
                                            }
                                            Pattern::Rest(inner) => {
                                                if let Pattern::Ident(ident) = &**inner {
                                                    let elem_ty = fields
                                                        .get(idx)
                                                        .map(|f| f.ty.clone())
                                                        .or_else(|| {
                                                            fields.first().map(|f| f.ty.clone())
                                                        })
                                                        .unwrap_or(Type::I32);
                                                    let is_const =
                                                        var.kind == crate::ast::VarDeclKind::Const;
                                                    let arr_ty = if is_const {
                                                        Type::BorrowType {
                                                            inner: Box::new(Type::Array(Box::new(
                                                                elem_ty,
                                                            ))),
                                                            is_mut: false,
                                                            lifetimes: vec![],
                                                        }
                                                    } else {
                                                        Type::Array(Box::new(elem_ty))
                                                    };
                                                    let t_str = self.map_type(&arr_ty);
                                                    self.locals
                                                        .insert(ident.sym.clone(), t_str.clone());
                                                    writeln!(
                                                        output,
                                                        "    {}: {},",
                                                        ident.sym, t_str
                                                    )
                                                    .unwrap();
                                                }
                                            }
                                            _ => {}
                                        }
                                    }
                                }
                                Some(Type::Array(elem_ty)) => {
                                    let t_str = self.map_type(elem_ty);
                                    let is_const = var.kind == crate::ast::VarDeclKind::Const;
                                    let arr_ty = if is_const {
                                        Type::BorrowType {
                                            inner: Box::new(Type::Array(elem_ty.clone())),
                                            is_mut: false,
                                            lifetimes: vec![],
                                        }
                                    } else {
                                        Type::Array(elem_ty.clone())
                                    };
                                    let arr_t_str = self.map_type(&arr_ty);
                                    for p in el {
                                        match p {
                                            Pattern::Ident(ident) => {
                                                self.locals
                                                    .insert(ident.sym.clone(), t_str.clone());
                                                writeln!(output, "    {}: {},", ident.sym, t_str)
                                                    .unwrap();
                                            }
                                            Pattern::Rest(inner) => {
                                                if let Pattern::Ident(ident) = &**inner {
                                                    self.locals.insert(
                                                        ident.sym.clone(),
                                                        arr_t_str.clone(),
                                                    );
                                                    writeln!(
                                                        output,
                                                        "    {}: {},",
                                                        ident.sym, arr_t_str
                                                    )
                                                    .unwrap();
                                                }
                                            }
                                            _ => {}
                                        }
                                    }
                                }
                                _ => {
                                    for p in el {
                                        match p {
                                            Pattern::Ident(ident) => {
                                                self.locals.insert(
                                                    ident.sym.clone(),
                                                    "[]const u8".to_string(),
                                                );
                                                writeln!(output, "    {}: []const u8,", ident.sym)
                                                    .unwrap();
                                            }
                                            Pattern::Rest(inner) => {
                                                if let Pattern::Ident(ident) = &**inner {
                                                    self.locals.insert(
                                                        ident.sym.clone(),
                                                        "[]const Message".to_string(),
                                                    );
                                                    writeln!(
                                                        output,
                                                        "    {}: []const Message,",
                                                        ident.sym
                                                    )
                                                    .unwrap();
                                                }
                                            }
                                            _ => {}
                                        }
                                    }
                                }
                            }
                        }
                        Pattern::Object(fields) => {
                            let whole_is_mut = matches!(&var.init, Some(Expr::Borrow(_, true)));
                            let base_init = match &var.init {
                                Some(Expr::Move(inner)) | Some(Expr::Borrow(inner, _)) => {
                                    Some(&**inner)
                                }
                                other => other.as_ref(),
                            };
                            let init_cp_ty = base_init.and_then(|e| self.infer_cp_type(e));
                            let mut resolved_ty = var.ty.as_ref().or(init_cp_ty.as_ref()).cloned();
                            while let Some(Type::BorrowType { inner, .. }) = resolved_ty {
                                resolved_ty = Some(*inner);
                            }
                            let obj_fields: Option<Vec<Field>> = match &resolved_ty {
                                Some(Type::Object(fs)) => Some(fs.clone()),
                                Some(Type::Ref(cname)) => {
                                    self.class_defs.get(&cname.sym).map(|c| c.fields.clone())
                                }
                                _ => None,
                            };
                            let non_rest_keys: HashSet<String> = fields
                                .iter()
                                .filter(|p| !p.is_rest)
                                .map(|p| p.key.sym.clone())
                                .collect();

                            for p in fields {
                                if p.key.sym == "*" || matches!(p.val, Some(Pattern::Wildcard)) {
                                    continue;
                                }
                                if p.is_rest {
                                    let remaining_fields = if let Some(ref tys) = obj_fields {
                                        tys.iter()
                                            .filter(|t| !non_rest_keys.contains(&t.ident.sym))
                                            .cloned()
                                            .collect::<Vec<_>>()
                                    } else {
                                        vec![]
                                    };
                                    let rest_cp_ty = Type::Object(remaining_fields);
                                    let t_str = self.map_type(&rest_cp_ty);
                                    self.locals.insert(p.key.sym.clone(), t_str.clone());
                                    writeln!(output, "    {}: {},", p.key.sym, t_str).unwrap();
                                    continue;
                                }

                                let ident = if let Some(Pattern::Ident(i)) = &p.val {
                                    i
                                } else {
                                    &p.key
                                };
                                let f_opt = obj_fields
                                    .as_ref()
                                    .and_then(|tys| tys.iter().find(|t| t.ident.sym == p.key.sym));
                                let is_mut_field = p.mode == DestructureMode::MutBorrow
                                    || (p.mode == DestructureMode::ReadOnly && whole_is_mut);
                                let bound_cp_ty = f_opt.map(|f| {
                                    if is_mut_field {
                                        Type::BorrowType {
                                            inner: Box::new(f.ty.clone()),
                                            is_mut: true,
                                            lifetimes: vec![],
                                        }
                                    } else {
                                        f.ty.clone()
                                    }
                                });
                                let t_str = bound_cp_ty
                                    .as_ref()
                                    .map(|t| self.map_type(t))
                                    .unwrap_or_else(|| {
                                        if is_mut_field {
                                            "*i32".to_string()
                                        } else {
                                            "i32".to_string()
                                        }
                                    });
                                self.locals.insert(ident.sym.clone(), t_str.clone());
                                writeln!(output, "    {}: {},", ident.sym, t_str).unwrap();
                            }
                        }
                        _ => {}
                    }
                    if let Some(init) = &var.init {
                        self.extract_local_vars_from_expr_to_str(init, output);
                    }
                }
                Stmt::Expr(e) => {
                    self.extract_local_vars_from_expr_to_str(e, output);
                }
                Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
                    self.extract_local_vars_to_str(b, output)
                }
                Stmt::If(i) => {
                    match &*i.cons {
                        Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
                            self.extract_local_vars_to_str(b, output)
                        }
                        _ => {}
                    }
                    if let Some(alt) = &i.alt {
                        match &**alt {
                            Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
                                self.extract_local_vars_to_str(b, output)
                            }
                            _ => {}
                        }
                    }
                }
                Stmt::While(w) => match &*w.body {
                    Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
                        self.extract_local_vars_to_str(b, output)
                    }
                    _ => {}
                },
                Stmt::For(f) => {
                    if let Stmt::Block(b) | Stmt::ArenaBlock(b) = &*f.body {
                        self.extract_local_vars_to_str(b, output);
                    }
                }
                Stmt::TryCatch {
                    try_block,
                    catch_body,
                    ..
                } => {
                    self.extract_local_vars_to_str(try_block, output);
                    if let Some(cb) = catch_body {
                        match &**cb {
                            Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
                                self.extract_local_vars_to_str(b, output)
                            }
                            _ => {}
                        }
                    }
                }
                Stmt::Match(m) => {
                    for arm in &m.arms {
                        match &*arm.body {
                            Stmt::Block(b) | Stmt::UnsafeBlock(b) | Stmt::ArenaBlock(b) => {
                                self.extract_local_vars_to_str(b, output);
                            }
                            _ => {}
                        }
                    }
                }
                Stmt::Fuzz(f) => {
                    self.extract_local_vars_to_str(&f.body, output);
                }
                _ => {}
            }
        }
    }

    pub(crate) fn extract_local_vars(&mut self, _block: &BlockStmt) {
        let mut sorted_locals: Vec<(String, String)> = self
            .locals
            .iter()
            .map(|(k, v)| (k.clone(), v.clone()))
            .collect();
        sorted_locals.sort_by(|a, b| a.0.cmp(&b.0));
        for (name, ty_str) in sorted_locals {
            if name == "self" || name == "this" {
                continue;
            }
            if ty_str == "[]const u8" {
                self.writeln(&format!("{}: []const u8 = &[_]u8{{}},", name));
            } else if ty_str == "i32" || ty_str == "u32" {
                self.writeln(&format!("{}: {} = 0,", name, ty_str));
            } else if ty_str == "bool" {
                self.writeln(&format!("{}: bool = false,", name));
            } else {
                self.writeln(&format!("{}: {} = undefined,", name, ty_str));
            }
        }
    }

    pub(crate) fn generate_fuzz_decl(&mut self, f: &FuzzDecl) {
        let test_name = format!("fuzz: {}", f.name);
        self.writeln(&format!("\ntest \"{}\" {{", test_name));
        self.indent();
        self.writeln("// --- CP Fuzz Engine Built-in Corpus & Mutation Driver ---");
        self.writeln("const __corpus = [_][]const u8{");
        self.indent();
        self.writeln("\"\",");
        self.writeln("\"\\x00\",");
        self.writeln("\"{}\",");
        self.writeln("\"[]\",");
        self.writeln("\"\\\"\\\"\",");
        self.writeln("\"null\",");
        self.writeln("\"true\",");
        self.writeln("\"false\",");
        self.writeln("\"0\",");
        self.writeln("\"-1\",");
        self.writeln("\"999999999999999999\",");
        self.writeln("\"3.141592653589793\",");
        self.writeln("\"\\xff\\xfe\\xfd\\x80\\xc0\",");
        self.writeln("\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\",");
        self.writeln("\"\\r\\n\\r\\n\",");
        self.writeln("\"{\\\"a\\\": 1, \\\"b\\\": [2, 3]}\",");
        self.dedent();
        self.writeln("};");

        let saved_case = self.current_case_content.clone();
        self.current_case_content.clear();
        for stmt in &f.body.body {
            self.generate_stmt(stmt);
        }
        let body_code = self.current_case_content.clone();
        self.current_case_content = saved_case;

        self.writeln(&format!("for (__corpus) |__{}_seed| {{", f.param_name));
        self.indent();
        self.writeln(&format!(
            "const {} = __{}_seed;",
            f.param_name, f.param_name
        ));
        self.output.push_buffer(&body_code);
        self.dedent();
        self.writeln("}");

        self.writeln("var __prng = std.Random.DefaultPrng.init(0x1337cafe);");
        self.writeln("const __rand = __prng.random();");
        self.writeln("var __fuzz_buf: [256]u8 = undefined;");
        self.writeln("var __iter: usize = 0;");
        self.writeln("while (__iter < 100) : (__iter += 1) {");
        self.indent();
        self.writeln("const __len = __rand.intRangeAtMost(usize, 0, 256);");
        self.writeln("__rand.bytes(__fuzz_buf[0..__len]);");
        self.writeln(&format!("const {} = __fuzz_buf[0..__len];", f.param_name));
        self.output.push_buffer(&body_code);
        self.dedent();
        self.writeln("}");

        self.dedent();
        self.writeln("}");

        self.fuzz_targets
            .push((f.name.clone(), f.param_name.clone(), body_code));
    }
}
