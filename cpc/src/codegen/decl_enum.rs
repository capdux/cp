use super::Codegen;
use crate::ast::*;
use std::fmt::Write;

impl Codegen {
    pub(crate) fn generate_trait(&mut self, t: &TraitDecl) {
        writeln!(
            self.output,
            "pub const {} = struct {{ ptr: *anyopaque, vtable: *const @This().VTable, pub const VTable = struct {{",
            t.ident.sym
        )
        .unwrap();
        self.indent();
        self.writeln("__type_id: u64,");
        self.writeln("__drop: *const fn(ptr: *anyopaque) void,");
        for m in &t.methods {
            let mut p_str = String::from("ptr: *anyopaque");
            for p in &m.params {
                if p.ident.sym == "this" || p.ident.sym == "_" {
                    continue;
                }
                p_str.push_str(", ");
                p_str.push_str(&self.map_type(&p.ty));
            }
            let ret = m
                .return_type
                .as_ref()
                .map(|t| self.map_type(t))
                .unwrap_or("void".to_string());
            let ret_fn_ptr = if ret.starts_with('!') {
                format!("anyerror{}", ret)
            } else {
                ret
            };
            self.writeln(&format!(
                "{}: *const fn({}) {},",
                m.ident.sym, p_str, ret_fn_ptr
            ));
        }
        self.dedent();
        self.writeln("};");
        self.indent();
        for m in &t.methods {
            let mut fn_params = format!("self: {}", t.ident.sym);
            let mut call_args = String::from("self.ptr");
            for p in &m.params {
                if p.ident.sym == "this" || p.ident.sym == "_" {
                    continue;
                }
                fn_params.push_str(&format!(", {}: {}", p.ident.sym, self.map_type(&p.ty)));
                call_args.push_str(&format!(", {}", p.ident.sym));
            }
            let ret = m
                .return_type
                .as_ref()
                .map(|t| self.map_type(t))
                .unwrap_or("void".to_string());
            self.writeln(&format!(
                "pub fn {}({}) {} {{ return self.vtable.{}({}); }}",
                m.ident.sym, fn_params, ret, m.ident.sym, call_args
            ));
        }
        self.writeln(&format!(
            "pub fn downcast(__cp_self: {}, comptime __CpTarget: type) ?*__CpTarget {{ if (__cp_self.vtable.__type_id == __CpTarget.__TYPE_ID) return @ptrCast(@alignCast(__cp_self.ptr)); return null; }}",
            t.ident.sym
        ));
        self.writeln(&format!(
            "pub fn release(self: {}) void {{ self.vtable.__drop(self.ptr); }}",
            t.ident.sym
        ));
        self.dedent();
        self.writeln("};\n");
    }

    pub(crate) fn generate_impl(&mut self, _i: &ImplDecl) {
        // VTables are emitted uniformly in generate_class for all traits in class.implements
    }

    pub(crate) fn generate_enum(&mut self, e: &EnumDecl) {
        let has_payloads = e
            .variants
            .iter()
            .any(|v| v.fields.is_some() || v.tuple_type.is_some() || v.from_err.is_some());
        if !has_payloads {
            let tag_str = if let Some(tag) = &e.tag_type {
                format!("({})", self.map_type(tag))
            } else if e.variants.iter().any(|v| v.value.is_some()) {
                "(u32)".to_string()
            } else {
                "".to_string()
            };
            self.writeln(&format!("pub const {} = enum{} {{", e.ident.sym, tag_str));
            self.indent();
            for v in &e.variants {
                let mut line = v.ident.sym.clone();
                if let Some(val) = &v.value {
                    line.push_str(" = ");
                    line.push_str(&self.generate_expr(val));
                }
                line.push(',');
                self.writeln(&line);
            }
            let has_variant = |vname: &str| {
                let escaped = format!("@\"{}\"", vname);
                e.variants
                    .iter()
                    .any(|v| v.ident.sym == vname || v.ident.sym == escaped)
            };
            let has_get = e.methods.iter().any(|m| m.ident.sym == "get")
                || e.extra_zig.iter().any(|z| z.contains("fn get("))
                || has_variant("get");
            let has_from_string = e.methods.iter().any(|m| m.ident.sym == "fromString")
                || e.extra_zig.iter().any(|z| z.contains("fn fromString("))
                || has_variant("fromString");
            let has_has = e.methods.iter().any(|m| m.ident.sym == "has")
                || e.extra_zig.iter().any(|z| z.contains("fn has("))
                || has_variant("has");
            let has_from_name = e.methods.iter().any(|m| m.ident.sym == "fromName")
                || e.extra_zig.iter().any(|z| z.contains("fn fromName("))
                || has_variant("fromName");
            let has_name = e.methods.iter().any(|m| m.ident.sym == "name")
                || e.extra_zig.iter().any(|z| z.contains("fn name("))
                || has_variant("name");
            let has_from_string_fn = (e.methods.iter().any(|m| m.ident.sym == "fromString")
                || e.extra_zig.iter().any(|z| z.contains("fn fromString(")))
                && !has_variant("fromString");
            if !has_get {
                if has_from_string_fn {
                    self.writeln("pub fn get(__cp_s: []const u8) ?@This() { return @This().fromString(__cp_s); }");
                } else {
                    self.writeln("pub fn get(__cp_s: []const u8) ?@This() { return std.meta.stringToEnum(@This(), __cp_s); }");
                }
            }
            if !has_has {
                self.writeln("pub fn has(__cp_s: []const u8) bool { return std.meta.stringToEnum(@This(), __cp_s) != null; }");
            }
            if !has_from_name {
                if has_from_string_fn {
                    self.writeln("pub fn fromName(__cp_s: []const u8) ?@This() { return @This().fromString(__cp_s); }");
                } else {
                    self.writeln("pub fn fromName(__cp_s: []const u8) ?@This() { return std.meta.stringToEnum(@This(), __cp_s); }");
                }
            }
            if !has_name {
                self.writeln(
                    "pub fn name(__cp_self: @This()) []const u8 { return @tagName(__cp_self); }",
                );
            }
            if !e.methods.is_empty() {
                for m in &e.methods {
                    self.generate_function_decorated(m, Some(&e.ident.sym));
                }
            }
            for code in &e.extra_zig {
                self.writeln("");
                let lowered = self.lower_extra_zig_member(code);
                for line in lowered.lines() {
                    self.writeln(line);
                }
            }
            self.dedent();
            self.writeln("};\n");
            return;
        }
        let enum_type = if e.variants.iter().any(|v| v.value.is_some()) {
            "enum(u32)"
        } else {
            "enum"
        };
        self.writeln(&format!(
            "pub const {} = union({}) {{",
            e.ident.sym, enum_type
        ));
        self.indent();
        for v in &e.variants {
            let mut line = v.ident.sym.clone();
            if let Some(val) = &v.value {
                line.push_str(" = ");
                line.push_str(&self.generate_expr(val));
            }
            if let Some(tuple_ty) = &v.tuple_type {
                line.push_str(&format!(": {},", self.map_type(tuple_ty)));
                self.writeln(&line);
            } else if let Some(fields) = &v.fields {
                line.push_str(": struct {");
                self.writeln(&line);
                self.indent();
                for f in fields {
                    self.writeln(&format!("{}: {},", f.ident.sym, self.map_type(&f.ty)));
                }
                self.dedent();
                self.writeln("},");
            } else {
                line.push(',');
                self.writeln(&line);
            }
        }
        let from_variants: Vec<&EnumVariant> =
            e.variants.iter().filter(|v| v.from_err.is_some()).collect();
        if !from_variants.is_empty() {
            self.writeln(&format!("pub fn __from(v: anytype) {} {{", e.ident.sym));
            self.indent();
            self.writeln("const V = @TypeOf(v);");
            for (idx, fv) in from_variants.iter().enumerate() {
                if let Some(src_err) = &fv.from_err {
                    let prefix = if idx == 0 { "if" } else { "else if" };
                    self.writeln(&format!(
                        "{} (comptime V == {}) {{ return {}{{ .{} = v }}; }}",
                        prefix, src_err.sym, e.ident.sym, fv.ident.sym
                    ));
                }
            }
            self.writeln("else { @compileError(\"Unsupported error conversion\"); }");
            self.dedent();
            self.writeln("}");
        }
        if !e.methods.iter().any(|m| m.ident.sym == "has")
            && !e.extra_zig.iter().any(|z| z.contains("fn has("))
            && !e
                .variants
                .iter()
                .any(|v| v.ident.sym == "has" || v.ident.sym == "@\"has\"")
        {
            self.writeln("pub fn has(s: []const u8) bool { if (comptime @typeInfo(@This()).@\"union\".tag_type) |TagType| { return std.meta.stringToEnum(TagType, s) != null; } return false; }");
        }
        if !e.methods.iter().any(|m| m.ident.sym == "name")
            && !e.extra_zig.iter().any(|z| z.contains("fn name("))
            && !e
                .variants
                .iter()
                .any(|v| v.ident.sym == "name" || v.ident.sym == "@\"name\"")
        {
            self.writeln("pub fn name(self: @This()) []const u8 { return @tagName(self); }");
        }
        if !e.methods.is_empty() {
            for m in &e.methods {
                self.generate_function_decorated(m, Some(&e.ident.sym));
            }
        }
        for code in &e.extra_zig {
            self.writeln("");
            let lowered = self.lower_extra_zig_member(code);
            for line in lowered.lines() {
                self.writeln(line);
            }
        }
        self.dedent();
        self.writeln("};\n");
    }

    pub(crate) fn generate_interface(&mut self, iface: &Interface) {
        writeln!(
            self.output,
            "const {} = struct {{ ptr: *anyopaque, vtable: *const @This().VTable, const VTable = struct {{",
            iface.ident.sym
        )
        .unwrap();
        self.indent();
        for m in &iface.methods {
            let mut p_str = String::from("ptr: *anyopaque");
            for p in &m.params {
                p_str.push_str(", ");
                p_str.push_str(&self.map_type(&p.ty));
            }
            let ret = m
                .return_type
                .as_ref()
                .map(|t| self.map_type(t))
                .unwrap_or("void".to_string());
            let ret_fn_ptr = if ret.starts_with('!') {
                format!("anyerror{}", ret)
            } else {
                ret
            };
            self.writeln(&format!(
                "{}: *const fn({}) {},",
                m.ident.sym, p_str, ret_fn_ptr
            ));
        }
        self.dedent();
        self.writeln("}; };\n");
    }
}
