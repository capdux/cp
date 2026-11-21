use super::{lower_struct_body, strip_redundant_comptime, type_to_display_string, Codegen};
use crate::ast::*;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

impl Codegen {
    fn field_align_str(&mut self, f: &Field) -> String {
        if let Some(m) = f.meta.iter().find(|m| m.name.sym == "align") {
            let a_val = match m.args.first() {
                Some(crate::ast::MetaArg::Positional(e)) => self.generate_expr(e),
                Some(crate::ast::MetaArg::Named(_, e)) => self.generate_expr(e),
                None => "1".to_string(),
            };
            format!(" align({})", a_val)
        } else {
            String::new()
        }
    }

    pub(crate) fn parse_anon_struct_or_union(code: &str) -> Option<Class> {
        let trimmed = code.trim();
        let is_struct_or_union = (trimmed.starts_with("struct")
            && trimmed.as_bytes().get("struct".len()).map_or(false, |b| {
                b.is_ascii_whitespace() || *b == b'{' || *b == b'('
            }))
            || (trimmed.starts_with("union")
                && trimmed.as_bytes().get("union".len()).map_or(false, |b| {
                    b.is_ascii_whitespace() || *b == b'{' || *b == b'('
                }));

        if !is_struct_or_union {
            return None;
        }
        let brace_idx = trimmed.find('{')?;
        let wrapped = format!(
            "{} __CpAnonStruct__ {}",
            trimmed[..brace_idx].trim_end(),
            &trimmed[brace_idx..]
        );
        let mut parser = crate::parser::Parser::new(&wrapped);
        let (module, _) = parser.parse_module_with_recovery();
        if let Some(ModuleItem::Decl(Decl::Class(cls))) = module.body.into_iter().next() {
            return Some(cls);
        }
        None
    }

    fn spawn_sub_codegen(&self) -> Codegen {
        let mut sub = Codegen::new();
        sub.is_lib_mode = self.is_lib_mode;
        sub.is_beam_mode = self.is_beam_mode;
        sub.has_imported_ptr = self.has_imported_ptr;
        sub.modular_mode = self.modular_mode;
        sub.globals = self.globals.clone();
        for name in self.current_fn_params.iter().chain(self.locals.keys()) {
            sub.globals.insert(name.clone());
        }
        sub.ref_classes = self.ref_classes.clone();
        sub.value_classes = self.value_classes.clone();
        sub.fn_sigs = self.fn_sigs.clone();
        sub.class_defs = self.class_defs.clone();
        sub.trait_defs = self.trait_defs.clone();
        sub.enum_defs = self.enum_defs.clone();
        sub.error_sets = self.error_sets.clone();
        sub.type_aliases = self.type_aliases.clone();
        sub.dec_defs = self.dec_defs.clone();
        sub.anon_obj_structs = self.anon_obj_structs.clone();
        sub
    }

    pub(crate) fn lower_extra_zig_member(&mut self, code: &str) -> String {
        let mut parser = crate::parser::Parser::new(code);
        let (module, diags) = parser.parse_module_with_recovery();
        let has_syntax_break = diags
            .iter()
            .any(|d| matches!(d.code.as_str(), "E0100" | "E0111" | "E0199"));
        if !has_syntax_break && module.body.len() == 1 {
            match &module.body[0] {
                ModuleItem::Stmt(Stmt::Var(var)) => {
                    if let Pattern::Ident(ident) = &var.pat {
                        if let Some(init) = &var.init {
                            let pub_prefix = if var.is_pub { "pub " } else { "" };
                            let tls_prefix = if var.is_threadlocal {
                                "threadlocal "
                            } else {
                                ""
                            };
                            let kw = if var.kind == VarDeclKind::Const {
                                "const"
                            } else {
                                "var"
                            };
                            let ty_str = if let Some(t) = &var.ty {
                                format!(": {}", self.map_type(t))
                            } else {
                                String::new()
                            };
                            let init_str = self.generate_expr(init);
                            return format!(
                                "{}{}{} {}{} = {};",
                                pub_prefix, tls_prefix, kw, ident.sym, ty_str, init_str
                            );
                        }
                    }
                }
                ModuleItem::Decl(Decl::Class(cls)) => {
                    let mut sub = self.spawn_sub_codegen();
                    sub.value_classes.insert(cls.ident.sym.clone());
                    sub.class_defs.insert(cls.ident.sym.clone(), cls.clone());
                    sub.generate_class(cls);
                    return sub.output.content.trim_end().to_string();
                }
                ModuleItem::Decl(Decl::Enum(e)) => {
                    let mut sub = self.spawn_sub_codegen();
                    sub.generate_enum(e);
                    return sub.output.content.trim_end().to_string();
                }
                ModuleItem::Decl(Decl::TypeAlias(ta)) => {
                    let pub_prefix = if ta.is_pub { "pub " } else { "" };
                    let target = self.map_type(&ta.target_ty);
                    return format!("{}const {} = {};", pub_prefix, ta.ident.sym, target);
                }
                _ => {}
            }
        }
        lower_struct_body(code)
    }

    pub(crate) fn generate_zig_or_struct(&mut self, code: &str) -> String {
        if let Some(mut cls) = Self::parse_anon_struct_or_union(code) {
            let has_fields = !cls.fields.is_empty();
            for m in &mut cls.methods {
                let has_explicit_self = m.params.first().map_or(false, |p| {
                    p.ident.sym == "this" || p.ident.sym == "_" || p.ident.sym == "self"
                });
                let uses_implicit_self = Self::block_contains_ident(&m.body, "_")
                    || Self::block_contains_ident(&m.body, "this")
                    || Self::block_contains_ident(&m.body, "self");
                let has_callconv = m.meta.iter().any(|mt| mt.name.sym == "callconv");
                if !has_explicit_self
                    && !uses_implicit_self
                    && (!has_fields || has_callconv || m.ident.sym == "init")
                {
                    m.is_static = true;
                }
            }
            let mut sub = Codegen::new();
            sub.is_lib_mode = self.is_lib_mode;
            sub.is_beam_mode = self.is_beam_mode;
            sub.has_imported_ptr = self.has_imported_ptr;
            sub.globals = self.globals.clone();
            for name in self.current_fn_params.iter().chain(self.locals.keys()) {
                sub.globals.insert(name.clone());
            }
            sub.ref_classes = self.ref_classes.clone();
            sub.value_classes = self.value_classes.clone();
            sub.fn_sigs = self.fn_sigs.clone();
            sub.class_defs = self.class_defs.clone();
            sub.trait_defs = self.trait_defs.clone();
            sub.enum_defs = self.enum_defs.clone();
            sub.error_sets = self.error_sets.clone();
            sub.type_aliases = self.type_aliases.clone();
            sub.dec_defs = self.dec_defs.clone();
            sub.anon_obj_structs = self.anon_obj_structs.clone();
            sub.value_classes.insert("@This()".to_string());
            sub.class_defs.insert("@This()".to_string(), cls.clone());
            sub.generate_class_inner(&cls, true);
            return sub.output.trim_end().to_string();
        }
        lower_struct_body(code)
    }

    pub(crate) fn generate_class(&mut self, class: &Class) {
        self.generate_class_inner(class, false);
    }

    pub(crate) fn generate_class_inner(&mut self, class: &Class, is_anon: bool) {
        if class.ident.sym == "Task"
            || class.ident.sym == "TypeInfo"
            || class.ident.sym == "FieldInfo"
            || class.ident.sym == "MethodInfo"
            || class.ident.sym == "Map"
            || class.ident.sym == "Set"
            || (!self.is_lib_mode && class.ident.sym == "Box")
        {
            return;
        }
        let has_cp_deinit = class.methods.iter().any(|m| m.ident.sym == "__cp_deinit");
        let is_c_abi = class.meta.iter().any(|m| {
            m.name.sym == "extern"
                || m.name.sym == "c"
                || m.name.sym == "fix"
                || m.name.sym == "packed"
                || m.name.sym == "opaque"
        });
        let needs_dropped_guard = has_cp_deinit && !is_c_abi && !class.is_union && !class.is_static;
        let is_root = class.meta.iter().any(|m| m.name.sym == "root");
        if is_root {
            if (class.is_ref || class.meta.iter().any(|m| m.name.sym == "ref"))
                && !class.meta.iter().any(|m| m.name.sym == "value")
            {
                self.ref_classes.insert(class.ident.sym.clone());
            }
            writeln!(self.output, "pub const {} = @This();\n", class.ident.sym).unwrap();
            if needs_dropped_guard {
                self.writeln("__cp_dropped: bool = false,");
            }
            for f in &class.fields {
                let default_init = if let Some(def) = &f.default {
                    format!(" = {},", self.generate_expr(def))
                } else if f.ident.sym == "ref_count" {
                    " = .init(),".to_string()
                } else if f.ident.sym == "pending"
                    || f.ident.sym == "waiters"
                    || f.ident.sym == "header_block"
                    || f.ident.sym == "body_buffer"
                    || f.ident.sym == "decoded_bytes"
                    || f.ident.sym == "decoded_headers"
                {
                    " = .{},".to_string()
                } else if f.ident.sym == "state" {
                    " = .open,".to_string()
                } else if f.ident.sym == "len" {
                    " = 0,".to_string()
                } else {
                    "".to_string()
                };
                let comma = if default_init.is_empty() { "," } else { "" };
                self.writeln(&format!(
                    "{}: {}{}{}",
                    f.ident.sym,
                    self.map_type(&f.ty),
                    default_init,
                    comma
                ));
            }
            for m in &class.methods {
                self.generate_function_decorated(m, Some(&class.ident.sym));
            }
            for code in &class.extra_zig {
                let lowered = self.lower_extra_zig_member(code);
                for line in lowered.lines() {
                    self.writeln(line);
                }
            }
            if let Some(rc_meta) = class
                .meta
                .iter()
                .find(|m| m.name.sym == "refCount" || m.name.sym == "refcount")
            {
                let deinit_fn = if let Some(arg) = rc_meta.args.first() {
                    match arg {
                        crate::ast::MetaArg::Positional(Expr::Ident(id)) => id.sym.clone(),
                        crate::ast::MetaArg::Named(_, Expr::Ident(id)) => id.sym.clone(),
                        _ => "deinit".to_string(),
                    }
                } else {
                    "deinit".to_string()
                };
                self.writeln(&format!(
                    "const Self = @This(); const RefCount = if (@hasDecl(@import(\"root\"), \"bun\")) @import(\"root\").bun.ptr.RefCount(Self, \"ref_count\", {deinit_fn}, .{{}}) else struct {{ pub fn ref(self: *Self) *Self {{ if (@TypeOf(self.ref_count) == u32) {{ self.ref_count += 1; }} else {{ _ = self.ref_count.ref(); }} return self; }} pub fn deref(self: *Self) void {{ if (@TypeOf(self.ref_count) == u32) {{ if (self.ref_count > 0) {{ self.ref_count -= 1; if (self.ref_count == 0) self.{deinit_fn}(); }} }} else {{ if (self.ref_count.deref()) self.{deinit_fn}(); }} }} }};"
                ));
                self.writeln("pub fn ref(self: *@This()) *@This() { return RefCount.ref(self); }");
                self.writeln("pub fn deref(self: *@This()) void { RefCount.deref(self); }");
            }
            return;
        }
        let type_id = Self::fnv1a_type_id(&class.ident.sym);
        let is_packed = class.meta.iter().find(|m| m.name.sym == "packed");
        let is_extern = class
            .meta
            .iter()
            .any(|m| m.name.sym == "extern" || m.name.sym == "c" || m.name.sym == "fix");
        let is_opaque = class.meta.iter().any(|m| m.name.sym == "opaque");
        let struct_kind = if class.is_union {
            let tag_meta = class.meta.iter().find(|m| m.name.sym == "tag");
            if let Some(tag) = tag_meta {
                let tag_str = match tag.args.first() {
                    Some(crate::ast::MetaArg::Positional(e)) => self.generate_expr(e),
                    Some(crate::ast::MetaArg::Named(_, e)) => self.generate_expr(e),
                    None => "enum".to_string(),
                };
                format!("union({})", tag_str)
            } else if let Some(p) = is_packed {
                if let Some(arg) = p.args.first() {
                    let expr = match arg {
                        crate::ast::MetaArg::Positional(e) => e,
                        crate::ast::MetaArg::Named(_, e) => e,
                    };
                    format!("packed union({})", self.generate_expr(expr))
                } else {
                    "packed union".to_string()
                }
            } else if is_extern {
                "extern union".to_string()
            } else if is_opaque {
                "opaque".to_string()
            } else {
                "union".to_string()
            }
        } else if let Some(p) = is_packed {
            if let Some(arg) = p.args.first() {
                let expr = match arg {
                    crate::ast::MetaArg::Positional(e) => e,
                    crate::ast::MetaArg::Named(_, e) => e,
                };
                format!("packed struct({})", self.generate_expr(expr))
            } else {
                "packed struct".to_string()
            }
        } else if is_extern {
            "extern struct".to_string()
        } else if is_opaque {
            "opaque".to_string()
        } else {
            "struct".to_string()
        };
        let is_generic = !is_anon && !class.generic_params.is_empty();
        if is_anon {
            writeln!(self.output, "{} {{", struct_kind).unwrap();
        } else if is_generic {
            let mut params_str = Vec::new();
            for gp in &class.generic_params {
                match gp {
                    crate::ast::GenericParam::Type { ident, .. } => {
                        params_str.push(format!("comptime {}: type", ident.sym))
                    }
                    crate::ast::GenericParam::Const { ident, ty } => {
                        params_str.push(format!("comptime {}: {}", ident.sym, self.map_type(ty)));
                    }
                }
            }
            writeln!(
                self.output,
                "pub fn {}({}) type {{\n    return {} {{",
                class.ident.sym,
                params_str.join(", "),
                struct_kind
            )
            .unwrap();
            self.indent();
        } else {
            let cls_pub_prefix = if class.meta.iter().any(|m| m.name.sym == "c")
                && !class.is_pub
                && !self.modular_local_exports.contains(&class.ident.sym)
            {
                ""
            } else {
                "pub "
            };
            writeln!(
                self.output,
                "{}const {} = {} {{",
                cls_pub_prefix, class.ident.sym, struct_kind
            )
            .unwrap();
        }
        self.indent();
        if let Some(m) = class.meta.iter().find(|m| m.name.sym == "align") {
            let a_val = match m.args.first() {
                Some(crate::ast::MetaArg::Positional(e)) => self.generate_expr(e),
                Some(crate::ast::MetaArg::Named(_, e)) => self.generate_expr(e),
                None => "1".to_string(),
            };
            self.writeln(&format!("_align: [0]u8 align({}) = undefined,", a_val));
        }
        let needs_trait_infra =
            !class.implements.is_empty() || class.meta.iter().any(|m| m.name.sym == "reflect");
        if needs_trait_infra {
            self.writeln(&format!("pub const __TYPE_ID: u64 = 0x{:016x};", type_id));
        }
        if class.implements.iter().any(|i| i.sym == "Agent") {
            self.writeln("id: u32 = 0,");
        }
        if class.is_static {
            for f in &class.fields {
                let align_str = self.field_align_str(f);
                let default_init = if let Some(def) = &f.default {
                    let old_array_elem = self.expected_array_elem_type.clone();
                    if let crate::ast::Type::FixedArray(elem, _) | crate::ast::Type::Array(elem) =
                        &f.ty
                    {
                        self.expected_array_elem_type = Some((**elem).clone());
                    }
                    let res = format!(" = {}", self.generate_expr(def));
                    self.expected_array_elem_type = old_array_elem;
                    res
                } else {
                    " = undefined".to_string()
                };
                let pub_prefix = if f.is_pub { "pub " } else { "" };
                self.writeln(&format!(
                    "{}var {}: {}{}{};",
                    pub_prefix,
                    f.ident.sym,
                    self.map_type(&f.ty),
                    align_str,
                    default_init
                ));
            }
        } else {
            for f in &class.fields {
                if f.is_static {
                    continue;
                }
                let align_str = self.field_align_str(f);
                let default_init = if let Some(def) = &f.default {
                    let old_array_elem = self.expected_array_elem_type.clone();
                    if let crate::ast::Type::FixedArray(elem, _) | crate::ast::Type::Array(elem) =
                        &f.ty
                    {
                        self.expected_array_elem_type = Some((**elem).clone());
                    }
                    let res = format!(" = {},", self.generate_expr(def));
                    self.expected_array_elem_type = old_array_elem;
                    res
                } else {
                    ",".to_string()
                };
                self.writeln(&format!(
                    "{}: {}{}{}",
                    f.ident.sym,
                    self.map_type(&f.ty),
                    align_str,
                    default_init
                ));
            }
            if needs_dropped_guard {
                self.writeln("__cp_dropped: bool = false,");
            }
            if !class.is_union
                && !class.meta.iter().any(|m| {
                    m.name.sym == "value"
                        || m.name.sym == "packed"
                        || m.name.sym == "extern"
                        || m.name.sym == "opaque"
                })
            {
                let mut default_assigns = String::new();
                if needs_dropped_guard {
                    default_assigns.push_str(" self_ptr.__cp_dropped = false;");
                }
                for f in &class.fields {
                    if f.is_static {
                        continue;
                    }
                    if let Some(def) = &f.default {
                        let old_array_elem = self.expected_array_elem_type.clone();
                        if let crate::ast::Type::FixedArray(elem, _)
                        | crate::ast::Type::Array(elem) = &f.ty
                        {
                            self.expected_array_elem_type = Some((**elem).clone());
                        }
                        let def_str = self.generate_expr(def);
                        self.expected_array_elem_type = old_array_elem;
                        default_assigns
                            .push_str(&format!(" self_ptr.{} = {};", f.ident.sym, def_str));
                    }
                }
                self.writeln(&format!(
                    "\npub fn _create(allocator: std.mem.Allocator) !*@This() {{ const self_ptr = try allocator.create(@This()); self_ptr.* = undefined;{} return self_ptr; }}",
                    default_assigns
                ));
            }
            for f in &class.fields {
                if !f.is_static {
                    continue;
                }
                let align_str = self.field_align_str(f);
                let default_init = if let Some(def) = &f.default {
                    let old_array_elem = self.expected_array_elem_type.clone();
                    if let crate::ast::Type::FixedArray(elem, _) | crate::ast::Type::Array(elem) =
                        &f.ty
                    {
                        self.expected_array_elem_type = Some((**elem).clone());
                    }
                    let res = format!(" = {}", self.generate_expr(def));
                    self.expected_array_elem_type = old_array_elem;
                    res
                } else {
                    " = undefined".to_string()
                };
                let pub_prefix = if f.is_pub { "pub " } else { "" };
                let tls_prefix = if f
                    .meta
                    .iter()
                    .any(|m| m.name.sym == "tls" || m.name.sym == "threadlocal")
                {
                    "threadlocal "
                } else {
                    ""
                };
                self.writeln(&format!(
                    "{}{}var {}: {}{}{};",
                    pub_prefix,
                    tls_prefix,
                    f.ident.sym,
                    self.map_type(&f.ty),
                    align_str,
                    default_init
                ));
            }
            for nc in &class.nested_classes {
                self.generate_class_inner(nc, false);
            }
        }
        if needs_trait_infra {
            self.writeln(&format!(
                "pub fn __box_drop(ptr: *anyopaque) void {{ const self_ptr: *{0} = @ptrCast(@alignCast(ptr)); if (@hasDecl({0}, \"__cp_deinit\")) {{ self_ptr.__cp_deinit(); }} std.heap.page_allocator.destroy(self_ptr); }}",
                class.ident.sym
            ));
        }
        let method_owner = if is_anon {
            Some("@This()".to_string())
        } else {
            Some(class.ident.sym.clone())
        };
        for m in &class.methods {
            self.generate_function_decorated(m, method_owner.as_ref());
        }
        for code in &class.extra_zig {
            let lowered = self.lower_extra_zig_member(code);
            for line in lowered.lines() {
                self.writeln(line);
            }
        }
        if let Some(rc_meta) = class
            .meta
            .iter()
            .find(|m| m.name.sym == "refCount" || m.name.sym == "refcount")
        {
            let deinit_fn = if let Some(arg) = rc_meta.args.first() {
                match arg {
                    crate::ast::MetaArg::Positional(Expr::Ident(id)) => id.sym.clone(),
                    crate::ast::MetaArg::Named(_, Expr::Ident(id)) => id.sym.clone(),
                    _ => "deinit".to_string(),
                }
            } else {
                "deinit".to_string()
            };
            self.writeln(&format!(
                "const Self = @This(); const RefCount = if (@hasDecl(@import(\"root\"), \"bun\")) @import(\"root\").bun.ptr.RefCount(Self, \"ref_count\", {deinit_fn}, .{{}}) else struct {{ pub fn ref(self: *Self) *Self {{ if (@TypeOf(self.ref_count) == u32) {{ self.ref_count += 1; }} else {{ _ = self.ref_count.ref(); }} return self; }} pub fn deref(self: *Self) void {{ if (@TypeOf(self.ref_count) == u32) {{ if (self.ref_count > 0) {{ self.ref_count -= 1; if (self.ref_count == 0) self.{deinit_fn}(); }} }} else {{ if (self.ref_count.deref()) self.{deinit_fn}(); }} }} }};"
            ));
            self.writeln("pub fn ref(self: *@This()) *@This() { return RefCount.ref(self); }");
            self.writeln("pub fn deref(self: *@This()) void { RefCount.deref(self); }");
        }
        self.dedent();
        if is_anon {
            self.writeln("}");
            return;
        }
        self.writeln("};");
        if is_generic {
            self.dedent();
            self.writeln("}");
        }
        // Emit VTables for all implemented traits
        for tr_id in &class.implements {
            if let Some(tr) = self.trait_defs.get(&tr_id.sym).cloned() {
                let mut fields_str = format!(
                    ".__type_id = {}.__TYPE_ID, .__drop = &{}.__box_drop",
                    class.ident.sym, class.ident.sym
                );
                for tm in &tr.methods {
                    fields_str.push_str(&format!(
                        ", .{} = @ptrCast(&{}.{})",
                        tm.ident.sym, class.ident.sym, tm.ident.sym
                    ));
                }
                self.writeln(&format!(
                    "const vtable_{}_{} = {}.VTable {{ {} }};",
                    tr.ident.sym, class.ident.sym, tr.ident.sym, fields_str
                ));
            }
        }
        self.writeln("");
    }

    pub(crate) fn meta_expr_to_zig(&self, expr: &Expr) -> String {
        match expr {
            Expr::Lit(Lit::Str(s)) => format!("\"{}\"", s),
            Expr::Lit(Lit::Int(n)) => format!("{}", n),
            Expr::Lit(Lit::Float(f)) => format!("{}", f),
            Expr::Lit(Lit::Bool(b)) => format!("{}", b),
            Expr::Lit(Lit::Null) => "null".to_string(),
            Expr::Ident(id) => {
                if id.sym == "true" || id.sym == "false" {
                    id.sym.clone()
                } else {
                    format!("\"{}\"", id.sym)
                }
            }
            _ => "\"\"".to_string(),
        }
    }

    pub(crate) fn generate_meta_population(
        &self,
        meta_items: &[MetaItem],
        map_var: &str,
    ) -> String {
        let mut out = String::new();
        let mut grouped: HashMap<String, Vec<&MetaItem>> = HashMap::new();
        let mut order: Vec<String> = Vec::new();
        for item in meta_items {
            if !grouped.contains_key(&item.name.sym) {
                order.push(item.name.sym.clone());
            }
            grouped.entry(item.name.sym.clone()).or_default().push(item);
        }
        for name in order {
            let items = &grouped[&name];
            if name == "meta" {
                out.push_str("{\nconst __sub = Map._create(alloc) catch unreachable;\n");
                for item in items {
                    for arg in &item.args {
                        match arg {
                            MetaArg::Named(id, expr) => {
                                let v = self.meta_expr_to_zig(expr);
                                out.push_str(&format!("_ = __sub.set(\"{}\", {});\n", id.sym, v));
                                out.push_str(&format!(
                                    "_ = {}.set(\"{}\", {});\n",
                                    map_var, id.sym, v
                                ));
                            }
                            MetaArg::Positional(expr) => {
                                let v = self.meta_expr_to_zig(expr);
                                out.push_str(&format!("_ = __sub.set(\"value\", {});\n", v));
                                out.push_str(&format!("_ = {}.set(\"value\", {});\n", map_var, v));
                            }
                        }
                    }
                }
                out.push_str(&format!(
                    "_ = {}.set(\"meta\", cp_message_from(__sub));\n}}\n",
                    map_var
                ));
                continue;
            }
            if items.len() == 1 {
                let item = items[0];
                if item.args.is_empty() {
                    out.push_str(&format!("_ = {}.set(\"{}\", true);\n", map_var, name));
                } else if item.args.len() == 1 {
                    match &item.args[0] {
                        MetaArg::Positional(expr) => {
                            let val_code = self.meta_expr_to_zig(expr);
                            out.push_str(&format!(
                                "_ = {}.set(\"{}\", {});\n",
                                map_var, name, val_code
                            ));
                        }
                        MetaArg::Named(id, expr) => {
                            let val_code = self.meta_expr_to_zig(expr);
                            out.push_str(&format!("{{\nconst __sub = Map._create(alloc) catch unreachable;\n_ = __sub.set(\"{}\", {});\n_ = {}.set(\"{}\", cp_message_from(__sub));\n}}\n", id.sym, val_code, map_var, name));
                        }
                    }
                } else {
                    let has_named = item.args.iter().any(|a| matches!(a, MetaArg::Named(_, _)));
                    if has_named {
                        out.push_str("{\nconst __sub = Map._create(alloc) catch unreachable;\n");
                        for arg in &item.args {
                            match arg {
                                MetaArg::Named(id, expr) => {
                                    let v = self.meta_expr_to_zig(expr);
                                    out.push_str(&format!(
                                        "_ = __sub.set(\"{}\", {});\n",
                                        id.sym, v
                                    ));
                                }
                                MetaArg::Positional(expr) => {
                                    let v = self.meta_expr_to_zig(expr);
                                    out.push_str(&format!("_ = __sub.set(\"value\", {});\n", v));
                                }
                            }
                        }
                        out.push_str(&format!(
                            "_ = {}.set(\"{}\", cp_message_from(__sub));\n}}\n",
                            map_var, name
                        ));
                    } else {
                        out.push_str(&format!(
                            "{{\nconst __arr = alloc.alloc(Message, {}) catch unreachable;\n",
                            item.args.len()
                        ));
                        for (i, arg) in item.args.iter().enumerate() {
                            if let MetaArg::Positional(expr) = arg {
                                let v = self.meta_expr_to_zig(expr);
                                out.push_str(&format!("__arr[{}] = cp_message_from({});\n", i, v));
                            }
                        }
                        out.push_str(&format!(
                            "_ = {}.set(\"{}\", Message{{ .array = __arr }});\n}}\n",
                            map_var, name
                        ));
                    }
                }
            } else {
                out.push_str(&format!(
                    "{{\nconst __arr = alloc.alloc(Message, {}) catch unreachable;\n",
                    items.len()
                ));
                for (i, item) in items.iter().enumerate() {
                    if item.args.is_empty() {
                        out.push_str(&format!("__arr[{}] = cp_message_from(true);\n", i));
                    } else if item.args.len() == 1 {
                        match &item.args[0] {
                            MetaArg::Positional(expr) => {
                                let v = self.meta_expr_to_zig(expr);
                                out.push_str(&format!("__arr[{}] = cp_message_from({});\n", i, v));
                            }
                            MetaArg::Named(id, expr) => {
                                let v = self.meta_expr_to_zig(expr);
                                out.push_str(&format!("{{\nconst __sub = Map._create(alloc) catch unreachable;\n_ = __sub.set(\"{}\", {});\n__arr[{}] = cp_message_from(__sub);\n}}\n", id.sym, v, i));
                            }
                        }
                    } else {
                        out.push_str(&format!("__arr[{}] = cp_message_from(true);\n", i));
                    }
                }
                out.push_str(&format!(
                    "_ = {}.set(\"{}\", Message{{ .array = __arr }});\n}}\n",
                    map_var, name
                ));
            }
        }
        out
    }

    pub(crate) fn generate_reflection_tables(&mut self) {
        let class_names: Vec<String> = self.class_defs.keys().cloned().collect();
        for cls_name in class_names {
            if self.modular_imported_symbols.contains(&cls_name) {
                continue;
            }
            let cls = self.class_defs[&cls_name].clone();
            if !cls.meta.iter().any(|m| m.name.sym == "reflect") {
                continue;
            }
            if cls_name == "TypeInfo"
                || cls_name == "FieldInfo"
                || cls_name == "MethodInfo"
                || cls_name == "Map"
                || cls_name == "Set"
                || cls_name == "Box"
                || cls_name == "Error"
                || cls_name == "RegExp"
            {
                continue;
            }
            self.writeln(&format!("\nvar _typeinfo_{}: ?*TypeInfo = null;", cls_name));
            self.writeln(&format!(
                "fn _get_typeinfo_{}() *const TypeInfo {{",
                cls_name
            ));
            self.indent();
            self.writeln(&format!("if (_typeinfo_{}) |ti| return ti;", cls_name));
            self.writeln("const alloc = std.heap.page_allocator;");
            self.writeln("const cls_meta = Map._create(alloc) catch unreachable;");
            let cls_meta_code = self.generate_meta_population(&cls.meta, "cls_meta");
            for line in cls_meta_code.lines() {
                self.writeln(line);
            }

            let user_fields: Vec<&Field> = cls.fields.iter().collect();
            if user_fields.is_empty() {
                self.writeln("const fields = &[_]FieldInfo{};");
            } else {
                self.writeln(&format!(
                    "const fields = alloc.alloc(FieldInfo, {}) catch unreachable;",
                    user_fields.len()
                ));
                for (f_idx, f) in user_fields.iter().enumerate() {
                    let f_meta_var = format!("f{}_meta", f_idx);
                    self.writeln(&format!(
                        "const {} = Map._create(alloc) catch unreachable;",
                        f_meta_var
                    ));
                    let f_meta_code = self.generate_meta_population(&f.meta, &f_meta_var);
                    for line in f_meta_code.lines() {
                        self.writeln(line);
                    }
                    self.writeln(&format!(
                        "fields[{}] = FieldInfo{{ .name = \"{}\", .typeName = \"{}\", .meta = {} }};",
                        f_idx,
                        f.ident.sym,
                        type_to_display_string(&f.ty),
                        f_meta_var
                    ));
                }
            }

            let user_methods: Vec<&Function> = cls
                .methods
                .iter()
                .filter(|m| !m.ident.sym.starts_with("__") && m.ident.sym != "release")
                .collect();
            if user_methods.is_empty() {
                self.writeln("const methods = &[_]MethodInfo{};");
            } else {
                self.writeln(&format!(
                    "const methods = alloc.alloc(MethodInfo, {}) catch unreachable;",
                    user_methods.len()
                ));
                for (m_idx, m) in user_methods.iter().enumerate() {
                    let m_meta_var = format!("m{}_meta", m_idx);
                    self.writeln(&format!(
                        "const {} = Map._create(alloc) catch unreachable;",
                        m_meta_var
                    ));
                    let m_meta_code = self.generate_meta_population(&m.meta, &m_meta_var);
                    for line in m_meta_code.lines() {
                        self.writeln(line);
                    }

                    let non_recv_params: Vec<String> = m
                        .params
                        .iter()
                        .filter(|p| {
                            p.ident.sym != "this" && p.ident.sym != "_" && p.ident.sym != "self"
                        })
                        .map(|p| format!("\"{}\"", p.ident.sym))
                        .collect();
                    let params_str = if non_recv_params.is_empty() {
                        "&[_][]const u8{}".to_string()
                    } else {
                        format!("&[_][]const u8{{ {} }}", non_recv_params.join(", "))
                    };
                    let ret_type_str = m
                        .return_type
                        .as_ref()
                        .map(type_to_display_string)
                        .unwrap_or_else(|| "void".to_string());

                    self.writeln(&format!(
                        "methods[{}] = MethodInfo{{ .name = \"{}\", .returnTypeName = \"{}\", .paramNames = {}, .isAsync = {}, .isStatic = {}, .meta = {} }};",
                        m_idx,
                        m.ident.sym,
                        ret_type_str,
                        params_str,
                        m.is_async,
                        m.is_static,
                        m_meta_var
                    ));
                }
            }

            self.writeln("const ti_ptr = alloc.create(TypeInfo) catch unreachable;");
            self.writeln(&format!(
                "ti_ptr.* = TypeInfo{{ .name = \"{}\", .isRef = {}, .fields = fields, .methods = methods, .meta = cls_meta }};",
                cls_name,
                cls.is_ref
            ));
            self.writeln(&format!("_typeinfo_{} = ti_ptr;", cls_name));
            self.writeln("return ti_ptr;");
            self.dedent();
            self.writeln("}");
        }
    }
}
