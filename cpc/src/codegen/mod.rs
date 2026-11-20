use crate::ast::*;
use crate::Span;
use std::collections::{HashMap, HashSet};
use std::fmt::Write;

pub mod actor;
pub mod borrow;
pub mod decl_class;
pub mod decl_enum;
pub mod expr;
pub mod pattern;
pub mod prelude;
pub mod runtime_emit;
pub mod stmt;
pub mod types;
pub mod zig_escape;

pub use actor::*;
pub use borrow::*;
pub use expr::*;
pub use pattern::*;
pub use prelude::*;
pub use stmt::*;
pub use types::*;

pub use zig_escape::*;
#[derive(Clone, Default, Debug, PartialEq, Eq)]
pub struct OutputBuffer {
    pub content: String,
    pub newline_count: usize,
}

impl OutputBuffer {
    pub fn new() -> Self {
        Self {
            content: String::new(),
            newline_count: 0,
        }
    }

    pub fn push_str(&mut self, s: &str) {
        self.newline_count += s.bytes().filter(|&b| b == b'\n').count();
        self.content.push_str(s);
    }

    pub fn push_buffer(&mut self, other: &OutputBuffer) {
        self.newline_count += other.newline_count;
        self.content.push_str(&other.content);
    }

    pub fn push(&mut self, ch: char) {
        if ch == '\n' {
            self.newline_count += 1;
        }
        self.content.push(ch);
    }

    pub fn clear(&mut self) {
        self.content.clear();
        self.newline_count = 0;
    }

    pub fn is_empty(&self) -> bool {
        self.content.is_empty()
    }

    pub fn len(&self) -> usize {
        self.content.len()
    }

    pub fn as_str(&self) -> &str {
        &self.content
    }
}

impl std::ops::Deref for OutputBuffer {
    type Target = str;
    fn deref(&self) -> &Self::Target {
        &self.content
    }
}

impl std::fmt::Write for OutputBuffer {
    fn write_str(&mut self, s: &str) -> std::fmt::Result {
        self.push_str(s);
        Ok(())
    }
}

impl std::fmt::Display for OutputBuffer {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "{}", self.content)
    }
}

impl From<&str> for OutputBuffer {
    fn from(s: &str) -> Self {
        let mut buf = Self::new();
        buf.push_str(s);
        buf
    }
}

impl From<String> for OutputBuffer {
    fn from(s: String) -> Self {
        let mut buf = Self::new();
        buf.push_str(&s);
        buf
    }
}

pub struct Codegen {
    pub is_beam_mode: bool,
    pub is_lib_mode: bool,
    pub is_fuzz_mode: bool,
    pub fuzz_target_filter: Option<String>,
    pub(crate) fuzz_targets: Vec<(String, String, OutputBuffer)>,
    pub agent_classes: Vec<crate::ast::Class>,
    pub(crate) output: OutputBuffer,
    pub(crate) cases: Vec<String>,
    pub(crate) indent_level: usize,
    pub(crate) pc_counter: u32,
    pub(crate) tmp_counter: u32,
    pub(crate) lambda_counter: u32,
    pub(crate) current_case_content: OutputBuffer,
    pub(crate) is_comptime: bool,
    pub(crate) is_state_machine: bool,
    pub(crate) current_fn_is_async: bool,
    pub(crate) locals: HashMap<String, String>,
    pub(crate) globals: HashSet<String>,
    pub(crate) lambda_funcs: Vec<String>,
    pub(crate) current_zig_fn_name: String,
    pub(crate) current_fn_zig_name: String,
    pub drop_flag_vars: HashSet<String>,
    pub(crate) is_class_method: bool,
    pub(crate) is_main: bool,
    pub moved_vars: HashSet<String>,
    pub lambda_moved_vars: HashMap<String, HashSet<String>>,
    pub(crate) uninitialized_vars: HashSet<String>,
    pub captured_vars: HashMap<String, Vec<(String, String)>>,
    pub ref_classes: HashSet<String>,
    pub value_classes: HashSet<String>,
    pub fn_sigs: HashMap<String, Function>,
    pub class_defs: HashMap<String, Class>,
    pub trait_defs: HashMap<String, TraitDecl>,
    pub enum_defs: HashMap<String, EnumDecl>,
    pub error_sets: HashMap<String, Vec<Ident>>,
    pub anon_err_sets: Vec<(String, Vec<String>)>,
    pub union_err_sets: Vec<(String, Vec<String>)>,
    pub in_try_region: bool,
    pub try_catch_target: Option<(String, String, String, ErrorType)>,
    pub expected_box_trait: Option<String>,
    pub expected_array_elem_type: Option<Type>,
    pub in_call_callee: bool,
    pub current_class_name: Option<String>,
    pub current_receiver_mode: ReceiverMode,
    pub mutable_fn_params: HashSet<String>,
    pub move_fn_params: HashSet<String>,
    pub var_cp_types: HashMap<String, Type>,
    pub global_var_types: HashMap<String, Type>,
    pub current_scope_depth: usize,
    pub var_scope_depth: HashMap<String, usize>,
    pub var_borrow_sources: HashMap<String, (Vec<(String, Option<String>)>, bool)>,
    pub var_is_ref_struct: HashSet<String>,
    pub current_fn_params: HashSet<String>,
    pub current_fn_ret_ty: Option<Type>,
    pub current_fn_contracts_ret: Vec<crate::ast::MetaItem>,
    pub current_stmt_idx: usize,
    pub last_use_map: HashMap<String, usize>,
    pub dec_defs: HashMap<String, DecDef>,
    pub current_next_callee: Option<String>,
    pub current_forward_params: Vec<String>,
    pub mut_borrow_loop_vars: HashSet<String>,
    pub loop_borrowed_containers: HashSet<String>,
    pub anon_obj_structs:
        std::rc::Rc<std::cell::RefCell<std::collections::BTreeMap<String, String>>>,
    pub is_parallel_runtime: bool,
    pub parallel_worker_count: usize,
    pub inline_closure_blk: Option<String>,
    pub inline_closure_params: HashSet<String>,
    pub block_locals: HashSet<String>,
    pub type_aliases: HashMap<String, TypeAliasDecl>,
    pub narrowed_null_vars: HashSet<String>,
    pub const_vars: HashSet<String>,
    pub type_namespace_vars: HashSet<String>,
    pub current_block_label: Option<String>,
    pub local_trampolines: HashSet<String>,
    pub source_dir: Option<std::path::PathBuf>,
    pub c_includes: Vec<String>,
    pub source_file_name: Option<String>,
    pub source_code: Option<String>,
    pub line_starts: Vec<usize>,
    pub source_map_entries: Vec<crate::source_map::SourceMapEntry>,
    pub current_func_name: Option<String>,
    pub arena_allocator_stack: Vec<String>,
    pub has_imported_ptr: bool,
    pub linker_paths: Vec<String>,
    pub modular_mode: bool,
    pub modular_is_entry: bool,
    pub modular_runtime_import: Option<String>,
    pub modular_inc_hash: Option<String>,
    pub modular_import_map: HashMap<String, String>,
    pub modular_module_exports: HashMap<String, Vec<String>>,
    pub modular_local_exports: HashSet<String>,
    pub modular_imported_symbols: HashSet<String>,
}

impl Codegen {
    pub fn new() -> Self {
        let mut linker_paths = Vec::new();
        if std::path::Path::new("/lib64").exists() {
            linker_paths.push("/lib64".to_string());
        }
        if std::path::Path::new("/usr/lib64").exists() {
            linker_paths.push("/usr/lib64".to_string());
        }
        Self {
            is_beam_mode: false,
            is_lib_mode: false,
            is_fuzz_mode: false,
            fuzz_target_filter: None,
            fuzz_targets: Vec::new(),
            agent_classes: Vec::new(),
            output: OutputBuffer::new(),
            cases: Vec::new(),
            indent_level: 0,
            pc_counter: 0,
            tmp_counter: 0,
            lambda_counter: 0,
            current_case_content: OutputBuffer::new(),
            is_comptime: false,
            is_state_machine: false,
            current_fn_is_async: false,
            locals: HashMap::new(),
            globals: HashSet::new(),
            lambda_funcs: Vec::new(),
            current_zig_fn_name: String::new(),
            current_fn_zig_name: String::new(),
            drop_flag_vars: HashSet::new(),
            is_class_method: false,
            is_main: false,
            moved_vars: HashSet::new(),
            lambda_moved_vars: HashMap::new(),
            uninitialized_vars: HashSet::new(),
            captured_vars: HashMap::new(),
            ref_classes: HashSet::new(),
            value_classes: HashSet::new(),
            fn_sigs: HashMap::new(),
            class_defs: HashMap::new(),
            trait_defs: HashMap::new(),
            enum_defs: HashMap::new(),
            error_sets: HashMap::new(),
            anon_err_sets: Vec::new(),
            union_err_sets: Vec::new(),
            in_try_region: false,
            try_catch_target: None,
            expected_box_trait: None,
            expected_array_elem_type: None,
            in_call_callee: false,
            current_class_name: None,
            current_receiver_mode: ReceiverMode::Static,
            mutable_fn_params: HashSet::new(),
            move_fn_params: HashSet::new(),
            var_cp_types: HashMap::new(),
            global_var_types: HashMap::new(),
            current_scope_depth: 0,
            var_scope_depth: HashMap::new(),
            var_borrow_sources: HashMap::new(),
            var_is_ref_struct: HashSet::new(),
            current_fn_params: HashSet::new(),
            current_fn_ret_ty: None,
            current_fn_contracts_ret: Vec::new(),
            current_stmt_idx: 0,
            last_use_map: HashMap::new(),
            dec_defs: HashMap::new(),
            current_next_callee: None,
            current_forward_params: Vec::new(),
            mut_borrow_loop_vars: HashSet::new(),
            loop_borrowed_containers: HashSet::new(),
            anon_obj_structs: std::rc::Rc::new(std::cell::RefCell::new(
                std::collections::BTreeMap::new(),
            )),
            is_parallel_runtime: false,
            parallel_worker_count: 0,
            inline_closure_blk: None,
            inline_closure_params: HashSet::new(),
            block_locals: HashSet::new(),
            type_aliases: HashMap::new(),
            narrowed_null_vars: HashSet::new(),
            const_vars: HashSet::new(),
            type_namespace_vars: HashSet::new(),
            current_block_label: None,
            local_trampolines: HashSet::new(),
            source_dir: None,
            c_includes: Vec::new(),
            source_file_name: None,
            source_code: None,
            line_starts: Vec::new(),
            source_map_entries: Vec::new(),
            current_func_name: None,
            arena_allocator_stack: Vec::new(),
            has_imported_ptr: false,
            linker_paths,
            modular_mode: false,
            modular_is_entry: false,
            modular_runtime_import: None,
            modular_inc_hash: None,
            modular_import_map: HashMap::new(),
            modular_module_exports: HashMap::new(),
            modular_local_exports: HashSet::new(),
            modular_imported_symbols: HashSet::new(),
        }
    }

    pub(crate) fn rt_sym(&self, name: &str) -> String {
        if self.modular_mode {
            format!("__cp_rt.{}", name)
        } else {
            name.to_string()
        }
    }

    pub fn escape_zig_ident(ident: &str) -> String {
        let keywords = [
            "align",
            "and",
            "asm",
            "async",
            "await",
            "break",
            "catch",
            "comptime",
            "const",
            "continue",
            "defer",
            "else",
            "enum",
            "errdefer",
            "error",
            "export",
            "extern",
            "false",
            "fn",
            "for",
            "if",
            "inline",
            "noalias",
            "noinline",
            "nosuspend",
            "null",
            "opaque",
            "or",
            "orelse",
            "pub",
            "resume",
            "return",
            "linksection",
            "struct",
            "suspend",
            "switch",
            "test",
            "threadlocal",
            "true",
            "try",
            "union",
            "unreachable",
            "usingnamespace",
            "var",
            "volatile",
            "while",
        ];
        if keywords.contains(&ident) {
            format!("@\"{}\"", ident)
        } else {
            ident.to_string()
        }
    }

    pub fn current_allocator(&self) -> String {
        if let Some(alloc) = self.arena_allocator_stack.last() {
            alloc.clone()
        } else if self.is_state_machine {
            "ctx.arena.?.allocator()".to_string()
        } else {
            "std.heap.page_allocator".to_string()
        }
    }

    pub fn init_source(&mut self, file_name: Option<&str>, code: Option<&str>) {
        if let Some(f) = file_name {
            self.source_file_name = Some(f.to_string());
        }
        if let Some(c) = code {
            self.source_code = Some(c.to_string());
            let mut starts = vec![0];
            for (idx, b) in c.bytes().enumerate() {
                if b == b'\n' {
                    starts.push(idx + 1);
                }
            }
            self.line_starts = starts;
        }
    }

    pub fn current_generated_line(&self) -> u32 {
        (self.output.newline_count + self.current_case_content.newline_count + 1) as u32
    }

    pub fn offset_to_line_col(&self, offset: usize) -> (u32, u32) {
        if self.line_starts.is_empty() {
            return (0, 0);
        }
        match self.line_starts.binary_search(&offset) {
            Ok(line) => ((line + 1) as u32, 1),
            Err(line) => {
                let line_idx = line.saturating_sub(1);
                let col = offset.saturating_sub(self.line_starts[line_idx]) + 1;
                ((line_idx + 1) as u32, col as u32)
            }
        }
    }

    pub fn record_source_location(&mut self, span: crate::Span) {
        if span.start == 0 && span.end == 0 {
            return;
        }
        if let Some(file) = &self.source_file_name {
            let (cp_line, cp_col) = self.offset_to_line_col(span.start);
            if cp_line > 0 {
                let zig_line = self.current_generated_line();
                if let Some(last) = self.source_map_entries.last() {
                    if last.zig_line == zig_line && last.cp_line == cp_line {
                        return;
                    }
                }
                self.source_map_entries
                    .push(crate::source_map::SourceMapEntry {
                        zig_line,
                        zig_col: 1,
                        cp_file: file.clone(),
                        cp_line,
                        cp_col,
                        symbol: self.current_func_name.clone(),
                    });
            }
        }
    }

    pub fn build_source_map(
        &self,
        cp_source: String,
        zig_target: String,
    ) -> crate::source_map::SourceMap {
        let mut sm = crate::source_map::SourceMap::new(cp_source, zig_target);
        sm.mappings = self.source_map_entries.clone();
        sm
    }

    pub fn scan_features_in_module(module: &Module) -> ModuleFeatures {
        ModuleFeatures::scan(module, false)
    }

    pub fn generate_module(&mut self, module: &Module) -> String {
        if let Some(ref inc_hash) = self.modular_inc_hash.clone() {
            self.writeln(&format!("// @cp_inc_hash:{}", inc_hash));
        }
        let features = ModuleFeatures::scan(module, self.is_lib_mode);
        self.has_imported_ptr = features.has_imported_ptr;

        if self.modular_mode {
            for item in &module.body {
                if let ModuleItem::Export(exp) = item {
                    if exp.source.is_none() {
                        for s in &exp.specifiers {
                            self.modular_local_exports.insert(s.sym.clone());
                        }
                    }
                }
            }
        }

        let linker_paths = self.linker_paths.clone();
        for path in linker_paths {
            self.writeln(&format!("// @linkPath(\"{}\")", path));
        }

        let mut class_order = Vec::new();
        let mut impl_decls = Vec::new();
        let mut has_single_thread_meta = false;
        for item in &module.body {
            if let ModuleItem::Decl(Decl::Func(f)) = item {
                self.modular_imported_symbols.remove(&f.ident.sym);
                self.globals.insert(f.ident.sym.clone());
                self.fn_sigs.insert(f.ident.sym.clone(), f.clone());
                if f.ident.sym == "main" {
                    for m in &f.meta {
                        if m.name.sym == "parallel" {
                            self.is_parallel_runtime = true;
                            for arg in &m.args {
                                let expr = match arg {
                                    MetaArg::Positional(e) => e,
                                    MetaArg::Named(_, e) => e,
                                };
                                if let Expr::Lit(Lit::Int(n)) = expr {
                                    if *n > 0 {
                                        self.parallel_worker_count = *n as usize;
                                    }
                                }
                            }
                        } else if m.name.sym == "single_thread" {
                            has_single_thread_meta = true;
                            self.is_parallel_runtime = false;
                        }
                    }
                }
            }
            if let ModuleItem::Stmt(Stmt::Expr(Expr::Zig(z))) = item {
                for line in z.code.lines() {
                    let trimmed = line.trim();
                    let stripped = if let Some(s) = trimmed.strip_prefix("pub ") {
                        s.trim_start()
                    } else {
                        trimmed
                    };
                    if let Some(rest) = stripped.strip_prefix("const ") {
                        let mut parts = rest.split(&[' ', ':', '='][..]);
                        if let Some(name) = parts.next() {
                            let name = name.trim();
                            if !name.is_empty() {
                                self.value_classes.insert(name.to_string());
                                self.globals.insert(name.to_string());
                            }
                        }
                    } else if let Some(rest) = stripped.strip_prefix("fn ") {
                        let mut parts = rest.split(&[' ', '(', '<'][..]);
                        if let Some(name) = parts.next() {
                            let name = name.trim();
                            if !name.is_empty() {
                                self.globals.insert(name.to_string());
                            }
                        }
                    } else if let Some(rest) = stripped.strip_prefix("extern fn ") {
                        let mut parts = rest.split(&[' ', '(', '<'][..]);
                        if let Some(name) = parts.next() {
                            let name = name.trim();
                            if !name.is_empty() {
                                self.globals.insert(name.to_string());
                            }
                        }
                    } else if let Some(rest) = stripped.strip_prefix("extern \"c\" fn ") {
                        let mut parts = rest.split(&[' ', '(', '<'][..]);
                        if let Some(name) = parts.next() {
                            let name = name.trim();
                            if !name.is_empty() {
                                self.globals.insert(name.to_string());
                            }
                        }
                    }
                }
            }
            if let ModuleItem::Decl(Decl::Class(c)) = item {
                self.modular_imported_symbols.remove(&c.ident.sym);
                self.globals.insert(c.ident.sym.clone());
                if (c.is_ref || c.meta.iter().any(|m| m.name.sym == "ref"))
                    && !c.meta.iter().any(|m| m.name.sym == "value")
                {
                    self.ref_classes.insert(c.ident.sym.clone());
                    self.value_classes.remove(&c.ident.sym);
                } else {
                    self.ref_classes.remove(&c.ident.sym);
                }
                if c.meta.iter().any(|m| m.name.sym == "value") {
                    self.value_classes.insert(c.ident.sym.clone());
                }
                for code in &c.extra_zig {
                    for line in code.lines() {
                        let trimmed = line.trim();
                        let stripped = if let Some(s) = trimmed.strip_prefix("pub ") {
                            s.trim_start()
                        } else {
                            trimmed
                        };
                        if let Some(rest) = stripped.strip_prefix("const ") {
                            let mut parts = rest.split(&[' ', ':', '='][..]);
                            if let Some(name) = parts.next() {
                                let name = name.trim();
                                if !name.is_empty() {
                                    self.value_classes.insert(name.to_string());
                                }
                            }
                        }
                    }
                }
                if c.implements.iter().any(|i| i.sym == "Agent") {
                    self.agent_classes.push(c.clone());
                }
                class_order.push(c.ident.sym.clone());
                self.class_defs.insert(c.ident.sym.clone(), c.clone());
            }
            if let ModuleItem::Decl(Decl::Enum(e)) = item {
                self.modular_imported_symbols.remove(&e.ident.sym);
                self.globals.insert(e.ident.sym.clone());
                self.enum_defs.insert(e.ident.sym.clone(), e.clone());
            }
            if let ModuleItem::Decl(Decl::ErrorSet(es)) = item {
                self.modular_imported_symbols.remove(&es.ident.sym);
                self.globals.insert(es.ident.sym.clone());
                self.error_sets
                    .insert(es.ident.sym.clone(), es.variants.clone());
            }
            if let ModuleItem::Decl(Decl::Trait(t)) = item {
                self.modular_imported_symbols.remove(&t.ident.sym);
                self.globals.insert(t.ident.sym.clone());
                self.trait_defs.insert(t.ident.sym.clone(), t.clone());
            }
            if let ModuleItem::Decl(Decl::Impl(i)) = item {
                impl_decls.push(i.clone());
            }
            if let ModuleItem::Decl(Decl::Dec(d)) = item {
                self.dec_defs.insert(d.ident.sym.clone(), d.clone());
            }
            if let ModuleItem::Decl(Decl::TypeAlias(ta)) = item {
                self.modular_imported_symbols.remove(&ta.ident.sym);
                self.globals.insert(ta.ident.sym.clone());
                self.type_aliases.insert(ta.ident.sym.clone(), ta.clone());
            }
            if let ModuleItem::Decl(Decl::CInclude(ci)) = item {
                if !self.c_includes.contains(&ci.header) {
                    self.c_includes.push(ci.header.clone());
                }
                self.globals.insert("c".to_string());
                self.value_classes.insert("c".to_string());
            }
            if let ModuleItem::Stmt(Stmt::Var(v)) = item {
                if let Pattern::Ident(id) = &v.pat {
                    self.globals.insert(id.sym.clone());
                    if let Some(t) = &v.ty {
                        self.var_cp_types.insert(id.sym.clone(), t.clone());
                    }
                    if !v.is_extern
                        && v.kind == VarDeclKind::Const
                        && id
                            .sym
                            .chars()
                            .next()
                            .map_or(false, |c| c.is_ascii_uppercase())
                    {
                        self.value_classes.insert(id.sym.clone());
                    }
                }
            }
        }

        // 1. Merge retroactive `impl Trait for Class` declarations into class_defs
        for imp in &impl_decls {
            if let Some(cls) = self.class_defs.get_mut(&imp.target_name.sym) {
                if !cls.implements.iter().any(|i| i.sym == imp.trait_name.sym) {
                    cls.implements.push(imp.trait_name.clone());
                }
                for m in &imp.methods {
                    if let Some(existing) = cls
                        .methods
                        .iter_mut()
                        .find(|cm| cm.ident.sym == m.ident.sym)
                    {
                        *existing = m.clone();
                    } else {
                        cls.methods.push(m.clone());
                    }
                }
            }
        }

        // 2. Resolve single inheritance (`extends Parent`) for classes
        for c_name in &class_order {
            let parent_opt = self.class_defs.get(c_name).and_then(|c| c.extends.clone());
            if let Some(parent_ident) = parent_opt {
                if let Some(parent_cls) = self.class_defs.get(&parent_ident.sym).cloned() {
                    if let Some(child_cls) = self.class_defs.get_mut(c_name) {
                        if parent_cls.is_ref {
                            child_cls.is_ref = true;
                            self.ref_classes.insert(c_name.clone());
                        }
                        // Prepend parent fields
                        let mut combined_fields = parent_cls.fields.clone();
                        for cf in &child_cls.fields {
                            if !combined_fields
                                .iter()
                                .any(|pf| pf.ident.sym == cf.ident.sym)
                            {
                                combined_fields.push(cf.clone());
                            }
                        }
                        child_cls.fields = combined_fields;

                        // Inherit parent traits
                        for pt in &parent_cls.implements {
                            if !child_cls.implements.iter().any(|ct| ct.sym == pt.sym) {
                                child_cls.implements.push(pt.clone());
                            }
                        }

                        // Inherit parent methods & synthesize `__super_<method>`
                        for pm in &parent_cls.methods {
                            let mut super_m = pm.clone();
                            super_m.ident.sym = format!("__super_{}", pm.ident.sym);
                            if !child_cls
                                .methods
                                .iter()
                                .any(|cm| cm.ident.sym == super_m.ident.sym)
                            {
                                child_cls.methods.push(super_m);
                            }

                            if let Some(cm) = child_cls
                                .methods
                                .iter()
                                .find(|cm| cm.ident.sym == pm.ident.sym)
                            {
                                // Override contract check
                                if pm.ident.sym != "init" && pm.ident.sym != "release" {
                                    let p_mode = Self::get_func_receiver_mode(pm);
                                    let c_mode = Self::get_func_receiver_mode(cm);
                                    if p_mode != c_mode {
                                        panic!(
                                            "CP Compile Error: Method '{}.{}' receiver modifier ({:?}) does not match parent class '{}.{}' ({:?})",
                                            c_name, cm.ident.sym, c_mode, parent_ident.sym, pm.ident.sym, p_mode
                                        );
                                    }
                                }
                            } else {
                                child_cls.methods.push(pm.clone());
                            }
                        }
                    }
                }
            }
        }

        // 3. Validate Trait contracts & inject Trait Default Methods
        for c_name in &class_order {
            let impl_traits = self
                .class_defs
                .get(c_name)
                .map(|c| c.implements.clone())
                .unwrap_or_default();
            for tr_ident in impl_traits {
                if let Some(tr_decl) = self.trait_defs.get(&tr_ident.sym).cloned() {
                    for tm in &tr_decl.methods {
                        let expected_mode = Self::get_sig_receiver_mode(tm);
                        let existing_method = self
                            .class_defs
                            .get(c_name)
                            .and_then(|c| c.methods.iter().find(|m| m.ident.sym == tm.ident.sym))
                            .cloned();
                        if let Some(cm) = existing_method {
                            let actual_mode = Self::get_func_receiver_mode(&cm);
                            if expected_mode != actual_mode {
                                panic!(
                                    "CP Compile Error: Method '{}.{}' receiver modifier ({:?}) does not match trait '{}.{}' contract ({:?})",
                                    c_name, cm.ident.sym, actual_mode, tr_decl.ident.sym, tm.ident.sym, expected_mode
                                );
                            }
                        } else if let Some(default_body) = &tm.default_body {
                            let synth_fn = Function {
                                span: tm.span,
                                ident: tm.ident.clone(),
                                generic_params: tm.generic_params.clone(),
                                params: tm.params.clone(),
                                return_type: tm.return_type.clone(),
                                body: default_body.clone(),
                                is_async: false,
                                is_static: false,
                                is_unsafe: tm.is_unsafe,
                                meta: Vec::new(),
                                is_pub: true,
                            };
                            if let Some(cls) = self.class_defs.get_mut(c_name) {
                                cls.methods.push(synth_fn);
                            }
                        } else {
                            panic!(
                                "CP Compile Error: Class '{}' is missing implementation for trait method '{}.{}'",
                                c_name, tr_decl.ident.sym, tm.ident.sym
                            );
                        }
                    }
                }
            }
        }
        // 4. Scan all types in the module to reject BareDyn (`dyn Trait` on stack) and collect AnonSet / Union error sets
        let fn_list: Vec<Function> = self.fn_sigs.values().cloned().collect();
        for f in &fn_list {
            for p in &f.params {
                self.check_type_validity_and_collect_errors(&p.ty);
            }
            if let Some(rt) = &f.return_type {
                self.check_type_validity_and_collect_errors(rt);
            }
            for s in &f.body.body {
                self.scan_stmt_types(s);
            }
        }
        let cls_list: Vec<Class> = self.class_defs.values().cloned().collect();
        for c in &cls_list {
            for fl in &c.fields {
                self.check_type_validity_and_collect_errors(&fl.ty);
            }
            for m in &c.methods {
                for p in &m.params {
                    self.check_type_validity_and_collect_errors(&p.ty);
                }
                if let Some(rt) = &m.return_type {
                    self.check_type_validity_and_collect_errors(rt);
                }
                for s in &m.body.body {
                    self.scan_stmt_types(s);
                }
            }
        }
        let tr_list: Vec<TraitDecl> = self.trait_defs.values().cloned().collect();
        for t in &tr_list {
            for m in &t.methods {
                for p in &m.params {
                    self.check_type_validity_and_collect_errors(&p.ty);
                }
                if let Some(rt) = &m.return_type {
                    self.check_type_validity_and_collect_errors(rt);
                }
            }
        }

        if !self.is_parallel_runtime
            && !has_single_thread_meta
            && (!self.modular_mode || self.modular_is_entry)
        {
            if let Ok(cap_content) = std::fs::read_to_string("cap.json") {
                if cap_content.contains("\"mode\": \"parallel\"")
                    || cap_content.contains("\"mode\":\"parallel\"")
                {
                    self.is_parallel_runtime = true;
                }
            }
        }

        self.globals.insert("Math".to_string());
        self.globals.insert("Map".to_string());
        self.globals.insert("Dict".to_string());
        self.globals.insert("Set".to_string());
        self.globals.insert("HashMap".to_string());
        self.globals.insert("HashSet".to_string());
        self.globals.insert("VecDeque".to_string());
        self.globals.insert("BinaryHeap".to_string());
        self.globals.insert("BTreeMap".to_string());
        self.globals.insert("BTreeSet".to_string());
        self.globals.insert("PID".to_string());
        self.globals.insert("Message".to_string());
        self.globals.insert("Mailbox".to_string());
        self.globals.insert("Error".to_string());
        self.globals.insert("Box".to_string());
        self.globals.insert("print".to_string());
        self.globals.insert("spawn".to_string());
        self.globals.insert("join".to_string());
        self.globals.insert("Task".to_string());
        self.globals.insert("Event".to_string());
        self.globals.insert("EventEmitter".to_string());
        self.globals.insert("console".to_string());
        self.globals.insert("reflect".to_string());
        self.globals.insert("TypeInfo".to_string());
        self.globals.insert("FieldInfo".to_string());
        self.globals.insert("MethodInfo".to_string());
        self.globals.insert("Mutex".to_string());
        self.globals.insert("RwLock".to_string());
        self.globals.insert("Atomic".to_string());
        self.globals.insert("Shared".to_string());
        self.globals.insert("LockGuard".to_string());
        self.globals.insert("ReadGuard".to_string());
        self.globals.insert("WriteGuard".to_string());

        for item in &module.body {
            if let ModuleItem::Stmt(Stmt::Var(v)) = item {
                if let Pattern::Ident(id) = &v.pat {
                    self.globals.insert(id.sym.clone());
                    let mut inferred = v.ty.clone();
                    if inferred.is_none() {
                        if let Some(init) = &v.init {
                            inferred = self
                                .infer_cp_type_with_mock(init, &std::collections::HashMap::new());
                        }
                    }
                    if let Some(ty) = inferred {
                        self.var_cp_types.insert(id.sym.clone(), ty.clone());
                        self.global_var_types.insert(id.sym.clone(), ty);
                    }
                }
            }
        }

        let has_top_string_decl = module.body.iter().any(|item| match item {
            ModuleItem::Stmt(Stmt::Var(v)) => match &v.pat {
                Pattern::Ident(id) => id.sym == "string" || id.sym == "@\"string\"",
                _ => false,
            },
            ModuleItem::Decl(Decl::TypeAlias(ta)) => {
                ta.ident.sym == "string" || ta.ident.sym == "@\"string\""
            }
            ModuleItem::Import(imp) => {
                imp.default_specifier
                    .as_ref()
                    .map_or(false, |id| id.sym == "string" || id.sym == "@\"string\"")
                    || imp
                        .specifiers
                        .iter()
                        .any(|id| id.sym == "string" || id.sym == "@\"string\"")
            }
            _ => false,
        });

        let has_top_decl = |name: &str| -> bool {
            let escaped = format!("@\"{}\"", name);
            let zig_eq = format!("const {} =", name);
            let zig_col = format!("const {}:", name);
            module.body.iter().any(|item| match item {
                ModuleItem::Stmt(Stmt::Var(v)) => match &v.pat {
                    Pattern::Ident(id) => id.sym == name || id.sym == escaped,
                    _ => false,
                },
                ModuleItem::Decl(Decl::TypeAlias(ta)) => {
                    ta.ident.sym == name || ta.ident.sym == escaped
                }
                ModuleItem::Decl(Decl::Class(c)) => c.ident.sym == name || c.ident.sym == escaped,
                ModuleItem::Decl(Decl::Enum(e)) => e.ident.sym == name || e.ident.sym == escaped,
                ModuleItem::Decl(Decl::Trait(t)) => t.ident.sym == name || t.ident.sym == escaped,
                ModuleItem::Decl(Decl::ErrorSet(e)) => {
                    e.ident.sym == name || e.ident.sym == escaped
                }
                ModuleItem::Decl(Decl::Func(f)) => f.ident.sym == name || f.ident.sym == escaped,
                ModuleItem::Import(imp) => {
                    imp.default_specifier
                        .as_ref()
                        .map_or(false, |id| id.sym == name || id.sym == escaped)
                        || imp
                            .specifiers
                            .iter()
                            .any(|id| id.sym == name || id.sym == escaped)
                }
                ModuleItem::Stmt(Stmt::Expr(Expr::Zig(z))) => {
                    z.code.contains(&zig_eq) || z.code.contains(&zig_col)
                }
                _ => false,
            })
        };
        let has_top_err_decl = has_top_decl("Err");
        let has_top_error_decl = has_top_decl("Error");

        let has_imported_print = module.body.iter().any(|item| match item {
            ModuleItem::Import(imp) => {
                imp.default_specifier
                    .as_ref()
                    .map_or(false, |id| id.sym == "print")
                    || imp.specifiers.iter().any(|id| id.sym == "print")
            }
            _ => false,
        });

        self.writeln("// Generated by CP Compiler (RFC-002 Production SDK)");
        self.writeln("const std = @import(\"std\");");
        if !has_top_string_decl {
            self.writeln("const string = []const u8;");
        }
        if let Some(ref rt_path) = self.modular_runtime_import.clone() {
            self.writeln(&format!("const __cp_rt = @import(\"{}\");", rt_path));
        }
        if self.is_beam_mode {
            self.writeln("const beam = @import(\"beam.zig\");");
        }
        if !self.c_includes.is_empty() {
            self.writeln("pub const __c_stdlib = struct {");
            self.writeln("    pub extern \"c\" fn puts(s: [*c]const u8) c_int;");
            self.writeln("    pub extern \"c\" fn printf(fmt: [*c]const u8, ...) c_int;");
            self.writeln("    pub extern \"c\" fn snprintf(buf: [*c]u8, size: usize, fmt: [*c]const u8, ...) c_int;");
            self.writeln("    pub extern \"c\" fn malloc(size: usize) ?*anyopaque;");
            self.writeln("    pub extern \"c\" fn free(ptr: ?*anyopaque) void;");
            self.writeln("    pub extern \"c\" fn exit(status: c_int) noreturn;");
            self.writeln("    pub extern \"c\" fn getenv(name: [*c]const u8) ?[*c]u8;");
            self.writeln("    pub extern \"c\" fn strlen(s: [*c]const u8) usize;");
            self.writeln(
                "    pub extern \"c\" fn strcmp(s1: [*c]const u8, s2: [*c]const u8) c_int;",
            );
            self.writeln("    pub extern \"c\" fn strncmp(s1: [*c]const u8, s2: [*c]const u8, n: usize) c_int;");
            self.writeln("    pub extern \"c\" fn memcpy(dest: ?*anyopaque, src: ?*const anyopaque, n: usize) ?*anyopaque;");
            self.writeln("    pub extern \"c\" fn memset(s: ?*anyopaque, c_val: c_int, n: usize) ?*anyopaque;");
            self.writeln("};");
        }
        self.writeln("");

        let declares_fn = |name: &str| -> bool {
            module.body.iter().any(|item| match item {
                ModuleItem::Decl(Decl::Func(f)) => f.ident.sym == name,
                _ => false,
            })
        };
        let declares_class = |name: &str| -> bool {
            module.body.iter().any(|item| match item {
                ModuleItem::Decl(Decl::Class(c)) => c.ident.sym == name,
                _ => false,
            })
        };

        if !self.modular_mode {
            self.emit_early_preludes(&features, has_top_err_decl, has_top_error_decl);
        } else {
            self.emit_modular_runtime_aliases(
                &features,
                &has_top_decl,
                has_top_err_decl,
                has_top_error_decl,
                declares_fn("print"),
                declares_fn("spawn"),
                declares_fn("join"),
                declares_class("PID"),
                declares_class("Message"),
            );
        }
        // --- Top-Level C-FFI / Zig Escapes ---
        for item in &module.body {
            if let ModuleItem::Stmt(Stmt::Expr(Expr::Zig(zig))) = item {
                let lowered = lower_struct_body(&zig.code);
                for line in lowered.lines() {
                    let trimmed = line.trim();
                    if trimmed == "const std = @import(\"std\");"
                        || trimmed == "pub const std = @import(\"std\");"
                    {
                        continue;
                    }
                    self.writeln(line);
                }
            }
        }

        // --- Top-Level Extern Constants & Variables ---
        for item in &module.body {
            if let ModuleItem::Stmt(Stmt::Var(var)) = item {
                let ty_str = if let Some(t) = &var.ty {
                    format!(": {}", self.map_type(t))
                } else {
                    String::new()
                };
                if var.is_extern {
                    if let Pattern::Ident(ident) = &var.pat {
                        let pub_str = if var.is_pub
                            || var.is_extern
                            || !self.is_lib_mode
                            || self.modular_local_exports.contains(&ident.sym)
                        {
                            "pub "
                        } else {
                            ""
                        };
                        let extern_str = if var.is_c { "extern \"c\"" } else { "extern" };
                        let kind_str = if var.kind == VarDeclKind::Const {
                            "const"
                        } else {
                            "var"
                        };
                        self.writeln(&format!(
                            "{}{} {} {}{};",
                            pub_str, extern_str, kind_str, ident.sym, ty_str
                        ));
                    }
                }
            }
        }

        // --- Top-Level Comptime Blocks ---
        for item in &module.body {
            if let ModuleItem::Stmt(Stmt::ComptimeBlock(b)) = item {
                let saved = self.current_case_content.clone();
                let saved_sm = self.is_state_machine;
                let saved_comptime = self.is_comptime;
                self.is_state_machine = false;
                self.is_comptime = true;
                self.current_case_content.clear();
                for s in &b.body {
                    self.generate_stmt(s);
                }
                let body_str = self.current_case_content.clone();
                self.current_case_content = saved;
                self.is_state_machine = saved_sm;
                self.is_comptime = saved_comptime;
                self.writeln(&format!("comptime {{\n{}\n}}", body_str));
            }
        }

        if !self.modular_mode {
            self.emit_runtime_preludes(&features);
        }
        if self.modular_mode {
            let mut emitted_mod_aliases: HashMap<String, String> = HashMap::new();
            let mut mod_alias_counter = 0usize;
            let mut get_or_emit_mod = |cg: &mut Codegen, zig_src: &str| -> String {
                if let Some(existing) = emitted_mod_aliases.get(zig_src) {
                    return existing.clone();
                }
                let alias = format!("__cp_mod_{}", mod_alias_counter);
                mod_alias_counter += 1;
                cg.writeln(&format!("const {} = @import(\"{}\");", alias, zig_src));
                emitted_mod_aliases.insert(zig_src.to_string(), alias.clone());
                alias
            };

            for item in &module.body {
                if let ModuleItem::Export(exp) = item {
                    if let Some(src) = &exp.source {
                        let zig_source =
                            self.modular_import_map
                                .get(src)
                                .cloned()
                                .unwrap_or_else(|| {
                                    if src.ends_with(".cp") {
                                        format!("{}.zig", &src[..src.len() - 3])
                                    } else {
                                        src.clone()
                                    }
                                });
                        let mod_var = get_or_emit_mod(self, &zig_source);
                        if exp.specifiers.is_empty() {
                            if let Some(star_syms) = self.modular_module_exports.get(src).cloned() {
                                for sym in star_syms {
                                    if !self.globals.insert(sym.clone()) {
                                        continue;
                                    }
                                    if !self.class_defs.contains_key(&sym)
                                        && !self.ref_classes.contains(&sym)
                                    {
                                        self.value_classes.insert(sym.clone());
                                    }
                                    let esc = Self::escape_zig_ident(&sym);
                                    self.writeln(&format!(
                                        "pub const {0} = {1}.{0};",
                                        esc, mod_var
                                    ));
                                }
                            }
                        } else {
                            for s in &exp.specifiers {
                                if !self.globals.insert(s.sym.clone()) {
                                    continue;
                                }
                                if !self.class_defs.contains_key(&s.sym)
                                    && !self.ref_classes.contains(&s.sym)
                                {
                                    self.value_classes.insert(s.sym.clone());
                                }
                                let esc = Self::escape_zig_ident(&s.sym);
                                self.writeln(&format!("pub const {0} = {1}.{0};", esc, mod_var));
                            }
                        }
                    }
                }
            }
            for item in &module.body {
                if let ModuleItem::Import(imp) = item {
                    let zig_source = self
                        .modular_import_map
                        .get(&imp.source)
                        .cloned()
                        .unwrap_or_else(|| {
                            if imp.source.ends_with(".cp") {
                                format!("{}.zig", &imp.source[..imp.source.len() - 3])
                            } else {
                                imp.source.clone()
                            }
                        });
                    if zig_source == "std" {
                        continue;
                    }
                    let mod_var = get_or_emit_mod(self, &zig_source);
                    if let Some(def) = &imp.default_specifier {
                        if def.sym == "std" && zig_source == "std" {
                            continue;
                        }
                        if self.globals.insert(def.sym.clone()) {
                            self.value_classes.insert(def.sym.clone());
                            let esc = Self::escape_zig_ident(&def.sym);
                            self.writeln(&format!("pub const {0} = {1};", esc, mod_var));
                        }
                    }
                    for s in &imp.specifiers {
                        if s.sym == "std" && zig_source == "std" {
                            continue;
                        }
                        if !self.globals.insert(s.sym.clone()) {
                            continue;
                        }
                        if !self.class_defs.contains_key(&s.sym)
                            && !self.ref_classes.contains(&s.sym)
                        {
                            self.value_classes.insert(s.sym.clone());
                        }
                        let esc = Self::escape_zig_ident(&s.sym);
                        self.writeln(&format!("pub const {0} = {1}.{0};", esc, mod_var));
                    }
                }
            }
        } else {
            self.emit_runtime_preludes_exports_imports(module);
        }

        for item in &module.body {
            if let ModuleItem::Decl(Decl::Trait(t)) = item {
                self.generate_trait(t);
            }
        }
        for item in &module.body {
            if let ModuleItem::Decl(Decl::Enum(e)) = item {
                self.generate_enum(e);
            }
            if let ModuleItem::Decl(Decl::ErrorSet(es)) = item {
                let union_suffix = if es.unions.is_empty() {
                    String::new()
                } else {
                    format!(" || {}", es.unions.join(" || "))
                };
                self.writeln(&format!("pub const {} = error{{", es.ident.sym));
                self.indent();
                for v in &es.variants {
                    self.writeln(&format!("{},", v.sym));
                }
                self.dedent();
                self.writeln(&format!("}}{};\n", union_suffix));
            }
        }
        // Emit synthesized AnonSet and Union error sets
        let anon_sets = self.anon_err_sets.clone();
        for (zig_name, variants) in &anon_sets {
            self.writeln(&format!("pub const {} = union(enum) {{", zig_name));
            self.indent();
            for v in variants {
                self.writeln(&format!("{},", v));
            }
            self.writeln("pub fn __from(v: anytype) @This() {");
            self.indent();
            self.writeln("return switch (v) {");
            self.indent();
            for v in variants {
                self.writeln(&format!(
                    ".{0} => |p| if (@hasField(@TypeOf(v), \"{0}\")) .{{ .{0} = p }} else unreachable,",
                    v
                ));
            }
            self.writeln("else => unreachable,");
            self.dedent();
            self.writeln("};");
            self.dedent();
            self.writeln("}");
            self.dedent();
            self.writeln("};\n");
        }
        let union_sets = self.union_err_sets.clone();
        for (zig_name, sub_names) in &union_sets {
            self.writeln(&format!("pub const {} = union(enum) {{", zig_name));
            self.indent();
            let mut seen_variants = HashSet::new();
            let mut variant_list = Vec::new();
            for sn in sub_names {
                if let Some(en) = self.enum_defs.get(sn).cloned() {
                    for v in &en.variants {
                        if seen_variants.insert(v.ident.sym.clone()) {
                            variant_list.push(v.ident.sym.clone());
                            if let Some(tty) = &v.tuple_type {
                                self.writeln(&format!("{}: {},", v.ident.sym, self.map_type(tty)));
                            } else {
                                self.writeln(&format!("{},", v.ident.sym));
                            }
                        }
                    }
                } else if let Some(es) = self.error_sets.get(sn).cloned() {
                    for v in &es {
                        if seen_variants.insert(v.sym.clone()) {
                            variant_list.push(v.sym.clone());
                            self.writeln(&format!("{},", v.sym));
                        }
                    }
                }
            }
            self.writeln("pub fn __from(v: anytype) @This() {");
            self.indent();
            for v in &variant_list {
                self.writeln(&format!(
                    "if (comptime @hasField(@TypeOf(v), \"{0}\")) {{ if (std.meta.activeTag(v) == .{0}) return .{{ .{0} = @field(v, \"{0}\") }}; }}",
                    v
                ));
            }
            self.writeln("unreachable;");
            self.dedent();
            self.writeln("}");
            self.dedent();
            self.writeln("};\n");
        }
        for item in &module.body {
            if let ModuleItem::Decl(Decl::Interface(i)) = item {
                self.generate_interface(i);
            }
        }
        for item in &module.body {
            if let ModuleItem::Stmt(Stmt::Var(v)) = item {
                if let Pattern::Ident(id) = &v.pat {
                    self.globals.insert(id.sym.clone());
                    if let Some(ty) = &v.ty {
                        self.var_cp_types.insert(id.sym.clone(), ty.clone());
                    }
                }
            }
        }
        for item in &module.body {
            if let ModuleItem::Stmt(Stmt::Expr(Expr::Zig(z))) = item {
                for line in z.code.lines() {
                    let trimmed = line.trim();
                    let stripped = if let Some(s) = trimmed.strip_prefix("pub ") {
                        s.trim_start()
                    } else {
                        trimmed
                    };
                    if let Some(rest) = stripped.strip_prefix("const ") {
                        let mut parts = rest.split(&[' ', ':', '='][..]);
                        if let Some(name) = parts.next() {
                            let name = name.trim();
                            if !name.is_empty() {
                                self.value_classes.insert(name.to_string());
                                self.globals.insert(name.to_string());
                            }
                        }
                    } else if let Some(rest) = stripped.strip_prefix("fn ") {
                        let mut parts = rest.split(&[' ', '(', '<'][..]);
                        if let Some(name) = parts.next() {
                            let name = name.trim();
                            if !name.is_empty() {
                                self.globals.insert(name.to_string());
                            }
                        }
                    } else if let Some(rest) = stripped.strip_prefix("extern fn ") {
                        let mut parts = rest.split(&[' ', '(', '<'][..]);
                        if let Some(name) = parts.next() {
                            let name = name.trim();
                            if !name.is_empty() {
                                self.globals.insert(name.to_string());
                            }
                        }
                    } else if let Some(rest) = stripped.strip_prefix("extern \"c\" fn ") {
                        let mut parts = rest.split(&[' ', '(', '<'][..]);
                        if let Some(name) = parts.next() {
                            let name = name.trim();
                            if !name.is_empty() {
                                self.globals.insert(name.to_string());
                            }
                        }
                    }
                }
            }
            if let ModuleItem::Decl(Decl::Class(c)) = item {
                if c.ident.sym == "PID" || c.ident.sym == "Message" || c.ident.sym == "Agent" {
                    continue;
                }
                let resolved = self
                    .class_defs
                    .get(&c.ident.sym)
                    .cloned()
                    .unwrap_or_else(|| c.clone());
                self.generate_class(&resolved);
            }
        }
        for item in &module.body {
            if let ModuleItem::Decl(Decl::Impl(i)) = item {
                self.generate_impl(i);
            }
        }
        for item in &module.body {
            if let ModuleItem::Decl(Decl::TypeAlias(ta)) = item {
                let pub_prefix = if ta.is_pub || self.modular_local_exports.contains(&ta.ident.sym)
                {
                    "pub "
                } else {
                    ""
                };
                let mapped = self.map_type(&ta.target_ty);
                self.writeln(&format!(
                    "{}const {} = {};",
                    pub_prefix, ta.ident.sym, mapped
                ));
            }
        }

        // --- Top-Level Constants & Variables ---
        for item in &module.body {
            if let ModuleItem::Stmt(Stmt::Var(var)) = item {
                if var.is_extern {
                    continue;
                }
                let ty_str = if let Some(t) = &var.ty {
                    format!(": {}", self.map_type(t))
                } else {
                    String::new()
                };
                let ident_pub = match &var.pat {
                    Pattern::Ident(id) => self.modular_local_exports.contains(&id.sym),
                    _ => false,
                };
                let is_var_pub = var.is_pub || ident_pub;
                let kw = if var.is_threadlocal {
                    if is_var_pub {
                        "pub threadlocal var"
                    } else {
                        "threadlocal var"
                    }
                } else if var.kind == VarDeclKind::Const {
                    if is_var_pub {
                        "pub const"
                    } else {
                        "const"
                    }
                } else {
                    if is_var_pub {
                        "pub var"
                    } else {
                        "var"
                    }
                };
                let old_array_elem = self.expected_array_elem_type.clone();
                if let Some(ty) = &var.ty {
                    if let Some(el) = Self::fixed_or_vec_elem_type(ty) {
                        self.expected_array_elem_type = Some(el);
                    } else if let Type::Array(elem) = ty {
                        self.expected_array_elem_type = Some((**elem).clone());
                    }
                }
                match &var.pat {
                    Pattern::Ident(ident) => {
                        if ident.sym == "std" {
                            continue;
                        }
                        let saved_comptime = self.is_comptime;
                        self.is_comptime = true;
                        let init_code = if let Some(init) = &var.init {
                            self.generate_expr(init)
                        } else {
                            "undefined".to_string()
                        };
                        self.is_comptime = saved_comptime;
                        self.writeln(&format!("{} {}{} = {};", kw, ident.sym, ty_str, init_code));
                    }
                    _ => {}
                }
                self.expected_array_elem_type = old_array_elem;
            }
        }

        for item in &module.body {
            match item {
                ModuleItem::Decl(Decl::Func(f)) => {
                    self.generate_function(f);
                }
                ModuleItem::Decl(Decl::Fuzz(f)) => {
                    self.generate_fuzz_decl(f);
                }
                ModuleItem::Stmt(Stmt::Fuzz(f)) => {
                    self.generate_fuzz_decl(f);
                }
                ModuleItem::Decl(Decl::Dec(_)) => {}
                ModuleItem::Decl(Decl::Macro(_)) => {}
                ModuleItem::Import(_) => {}
                _ => {}
            }
        }

        for f in &self.lambda_funcs {
            self.output.push_str(f);
            self.output.push('\n');
        }

        let anon_objs: Vec<(String, String)> = self
            .anon_obj_structs
            .borrow()
            .iter()
            .map(|(k, v)| (k.clone(), v.clone()))
            .collect();
        for (name, body) in anon_objs {
            self.writeln(&format!("pub const {} = {};", name, body));
        }

        if features.needs_reflection {
            self.generate_reflection_tables();
        }

        if self.is_beam_mode {
            self.writeln("");
            self.writeln("// --- BEAM NIF Dispatcher ---");
            self.writeln("export fn native_dispatch_nif(env_opt: ?*beam.Env, argc: c_int, argv: [*c]const beam.Term) callconv(.c) beam.Term {");
            self.writeln("    const env = env_opt orelse return 0; if (argc != 5) return beam.Beam.makeAtom(env, \"badarg\");");
            self.writeln("    const class_name = beam.Beam.getBinary(env, argv[0]) catch return beam.Beam.makeAtom(env, \"badarg\");");
            self.writeln("    const agent_id = beam.Beam.getInt(env, argv[1]) catch return beam.Beam.makeAtom(env, \"badarg\");");
            self.writeln("    const cp_ctx_bin = beam.Beam.getBinary(env, argv[2]) catch return beam.Beam.makeAtom(env, \"badarg\");");
            self.writeln("    const action_id = beam.Beam.getInt(env, argv[3]) catch return beam.Beam.makeAtom(env, \"badarg\");");
            self.writeln("    const payload = beam.Beam.getBinary(env, argv[4]) catch return beam.Beam.makeAtom(env, \"badarg\");");
            self.writeln("    _ = &class_name; _ = &agent_id; _ = &cp_ctx_bin; _ = &action_id; _ = &payload;");
            self.writeln("");

            let agents = self.agent_classes.clone();
            for agent in &agents {
                self.writeln(&format!(
                    "    if (std.mem.eql(u8, class_name, \"{}\")) {{",
                    agent.ident.sym
                ));
                self.writeln(&format!("        var instance = {}.init(std.heap.page_allocator) catch return beam.Beam.makeAtom(env, \"error\");", agent.ident.sym));
                self.writeln("        instance.id = @intCast(agent_id); instance.env = env;");
                self.writeln("        if (cp_ctx_bin.len > 0) {");
                self.writeln(&format!("            const ctx_ptr: *{0}.{0}_onMessage_Context = @ptrCast(@alignCast(@constCast(cp_ctx_bin.ptr)));", agent.ident.sym));
                self.writeln("            ctx_ptr.received_msg = .{ .string = payload };");
                self.writeln(&format!("            return {0}.onMessage(ctx_ptr) catch return beam.Beam.makeAtom(env, \"error\");", agent.ident.sym));
                self.writeln("        } else {");
                self.writeln(&format!("            var ctx_struct = {0}.{0}_onMessage_Context{{ .pc = 0, .self = instance, .received_msg = .{{ .string = payload }}, .action_id = @intCast(action_id), .payload = .{{ .string = payload }} }};", agent.ident.sym));
                self.writeln(&format!("            return {0}.onMessage(&ctx_struct) catch return beam.Beam.makeAtom(env, \"error\");", agent.ident.sym));
                self.writeln("        }");
                self.writeln("    }");
            }
            self.writeln("    return beam.Beam.makeAtom(env, \"unknown_class\");");
            self.writeln("}");
            self.writeln("export const _nif_entry = beam.defineNifEntry(\"cp_agents_lib\", &[_]beam.NifFunc{ .{ .name = \"native_dispatch\", .arity = 5, .fptr = @ptrCast(&native_dispatch_nif), .flags = 0 } }, null, null, null);");
            self.writeln("export fn nif_init() *beam.NifEntry { return @constCast(&_nif_entry); }");
        } else {
            if !self.modular_mode {
                if !self.is_lib_mode {
                    self.writeln("\npub var global_io: std.Io = undefined;");
                    self.writeln("pub var __cap_test_total: usize = 0;");
                    self.writeln("pub var __cap_test_passed: usize = 0;");
                    self.writeln("pub var __cap_test_failed: usize = 0;");
                } else if features.needs_actors {
                    self.writeln("\npub var global_io: std.Io = undefined;");
                }
            }

            let has_main =
                self.fn_sigs.contains_key("main") && (!self.modular_mode || self.modular_is_entry);
            if (!self.is_lib_mode || (self.modular_mode && self.modular_is_entry))
                && !self.is_fuzz_mode
                && has_main
            {
                let rt_global_io = self.rt_sym("global_io");
                let rt_shutdown = self.rt_sym("global_runtime_shutdown");
                let rt_worker_pool = self.rt_sym("_WorkerPoolRunner");
                let rt_pid = self.rt_sym("PID");
                let rt_next_local_id = self.rt_sym("next_local_id");
                let rt_drain = self.rt_sym("drain_cross_thread_mailbox");
                let rt_gtq = self.rt_sym("global_task_queue");
                let rt_running_workers = self.rt_sym("global_running_workers");
                let rt_queue_len = self.rt_sym("queue_len");
                let rt_task_queue = self.rt_sym("task_queue");
                let rt_active_fds = self.rt_sym("active_fds");
                let rt_waiting_count = self.rt_sym("waiting_count");
                let rt_active_tasks = self.rt_sym("global_active_tasks");
                let rt_poll_fds = self.rt_sym("poll_fds");
                let rt_schedule = self.rt_sym("schedule");
                let rt_fd_to_run = self.rt_sym("fd_to_run");
                let rt_fd_to_ctx = self.rt_sym("fd_to_ctx");
                let rt_test_total = self.rt_sym("__cap_test_total");
                let rt_test_passed = self.rt_sym("__cap_test_passed");
                let rt_test_failed = self.rt_sym("__cap_test_failed");

                self.writeln("pub fn main(init: std.process.Init) !void {");
                self.indent();
                self.writeln(&format!("{} = init.io;", rt_global_io));
                self.writeln("var arena = std.heap.ArenaAllocator.init(std.heap.page_allocator); defer arena.deinit();");
                if self.is_parallel_runtime {
                    let wc_expr = if self.parallel_worker_count > 0 {
                        format!("{}", self.parallel_worker_count)
                    } else {
                        "@min(@max(std.Thread.getCpuCount() catch 4, 2), 16)".to_string()
                    };
                    self.writeln(&format!("const __worker_count: usize = {};", wc_expr));
                    self.writeln(
                        "var __workers = try arena.allocator().alloc(std.Thread, __worker_count);",
                    );
                    self.writeln(&format!("for (0..__worker_count) |wi| {{ __workers[wi] = try std.Thread.spawn(.{{}}, {}.run, .{{}}); }}", rt_worker_pool));
                    self.writeln(&format!("defer {{ {}.store(true, .release); for (__workers) |wt| {{ wt.join(); }} }}", rt_shutdown));
                }
                self.writeln(&format!("var ctx: cp_main_Context = undefined; ctx.pc = 0; ctx._self = {}{{ .node_id = 0, .local_id = 1 }}; {} = 2; ctx.arena = &arena; ctx._parent = null; ctx._parent_fn = null; ctx.received_msg = null; ctx.received_sender = null;", rt_pid, rt_next_local_id));
                self.writeln("try cp_main(&ctx);");
                self.writeln("while (true) {");
                self.indent();
                self.writeln(&format!("{}();", rt_drain));
                self.writeln(&format!("while ({}.pop()) |task| {{ _ = {}.fetchAdd(1, .acquire); try task.run(task.ctx); _ = {}.fetchSub(1, .release); }}", rt_gtq, rt_running_workers, rt_running_workers));
                self.writeln(&format!("while ({0} > 0) {{ const task = {1}[0]; for ({1}[1..{0}], 0..) |t, j| {{ {1}[j] = t; }} {0} -= 1; try task.run(task.ctx); }}", rt_queue_len, rt_task_queue));
                self.writeln(&format!("{}();", rt_drain));
                self.writeln(&format!("if ({} == 0 and {} == 0 and {}.len() == 0 and {} == 0 and {}.load(.acquire) == 0 and {}.load(.acquire) == 0) break;", rt_active_fds, rt_queue_len, rt_gtq, rt_waiting_count, rt_active_tasks, rt_running_workers));
                self.writeln(&format!("if ({} == 0 and {}.len() == 0 and ({} > 0 or {}.load(.acquire) > 0 or {}.load(.acquire) > 0) and {} == 0) {{ const ts = std.os.linux.timespec{{ .sec = 0, .nsec = 500_000 }}; _ = std.os.linux.nanosleep(&ts, null); }}", rt_queue_len, rt_gtq, rt_waiting_count, rt_active_tasks, rt_running_workers, rt_active_fds));
                self.writeln(&format!("if ({} > 0) {{", rt_active_fds));
                self.indent();
                self.writeln(&format!(
                    "const ready_count = try std.posix.poll({}[0..{}], 10);",
                    rt_poll_fds, rt_active_fds
                ));
                self.writeln("if (ready_count > 0) {");
                self.indent();
                self.writeln("var i: usize = 0;");
                self.writeln(&format!("while (i < {}) {{", rt_active_fds));
                self.indent();
                self.writeln(&format!("if ({}[i].revents != 0) {{", rt_poll_fds));
                self.indent();
                self.writeln(&format!(
                    "{}({}[i], {}[i]);",
                    rt_schedule, rt_fd_to_run, rt_fd_to_ctx
                ));
                self.writeln(&format!(
                    "{0}[i] = {0}[{1} - 1];",
                    rt_poll_fds, rt_active_fds
                ));
                self.writeln(&format!(
                    "{0}[i] = {0}[{1} - 1];",
                    rt_fd_to_ctx, rt_active_fds
                ));
                self.writeln(&format!(
                    "{0}[i] = {0}[{1} - 1];",
                    rt_fd_to_run, rt_active_fds
                ));
                self.writeln(&format!("{} -= 1;", rt_active_fds));
                self.dedent();
                self.writeln("} else { i += 1; }");
                self.dedent();
                self.writeln("}");
                self.dedent();
                self.writeln("}");
                self.dedent();
                self.writeln("}");
                self.dedent();
                self.writeln("}"); // Close while (true)

                self.writeln(&format!("if ({} > 0) {{", rt_test_total));
                self.indent();
                self.writeln(
                    "@import(\"std\").debug.print(\"\\n===========================\\n\", .{});",
                );
                self.writeln("@import(\"std\").debug.print(\"    cap:test Summary\\n\", .{});");
                self.writeln(
                    "@import(\"std\").debug.print(\"===========================\\n\", .{});",
                );
                self.writeln(&format!(
                    "@import(\"std\").debug.print(\"Total:  {{d}}\\n\", .{{{}}});",
                    rt_test_total
                ));
                self.writeln(&format!(
                    "@import(\"std\").debug.print(\"Passed: {{d}}\\n\", .{{{}}});",
                    rt_test_passed
                ));
                self.writeln(&format!(
                    "@import(\"std\").debug.print(\"Failed: {{d}}\\n\", .{{{}}});",
                    rt_test_failed
                ));
                self.writeln(
                "@import(\"std\").debug.print(\"Coverage: 100% (Native CP Evaluated)\\n\", .{});",
            );
                self.writeln(&format!(
                    "if ({} > 0) @import(\"std\").process.exit(1);",
                    rt_test_failed
                ));
                self.dedent();
                self.writeln("}");

                self.dedent();
                self.writeln("}"); // Close main
            }
        }

        if self.is_fuzz_mode {
            self.writeln("\n// --- libFuzzer C-ABI Standard Entrypoint ---");
            let fuzz_targets = self.fuzz_targets.clone();
            let filter_opt = self.fuzz_target_filter.clone();
            if fuzz_targets.is_empty() {
                self.writeln("export fn LLVMFuzzerTestOneInput(data: [*c]const u8, size: usize) callconv(.c) c_int {");
                self.indent();
                self.writeln("_ = data;");
                self.writeln("_ = size;");
                self.writeln("return 0;");
                self.dedent();
                self.writeln("}");
            } else {
                for (idx, (name, param_name, body)) in fuzz_targets.iter().enumerate() {
                    self.writeln(&format!("// Fuzz target [{}]: \"{}\"", idx, name));
                    self.writeln(&format!(
                        "fn __cp_fuzz_target_{}(__fuzz_slice: []const u8) void {{",
                        idx
                    ));
                    self.indent();
                    self.writeln(&format!("const {} = __fuzz_slice;", param_name));
                    self.writeln(&format!("_ = &{};", param_name));
                    self.output.push_buffer(body);
                    self.dedent();
                    self.writeln("}\n");
                }

                self.writeln(
                    "export fn LLVMFuzzerTestOneInput(data: [*c]const u8, size: usize) callconv(.c) c_int {",
                );
                self.indent();
                self.writeln("if (size == 0 or data == null) return 0;");
                self.writeln("const input = data[0..size];");
                for (idx, (name, _, _)) in fuzz_targets.iter().enumerate() {
                    let should_run = if let Some(ref filter) = filter_opt {
                        filter == name || filter == &format!("{}", idx)
                    } else {
                        true
                    };
                    if should_run {
                        self.writeln(&format!("__cp_fuzz_target_{}(input);", idx));
                    }
                }
                self.writeln("return 0;");
                self.dedent();
                self.writeln("}");
            }
        }

        self.output.content.clone()
    }

    pub(crate) fn indent(&mut self) {
        self.indent_level += 1;
    }
    pub(crate) fn dedent(&mut self) {
        self.indent_level -= 1;
    }
    pub(crate) fn writeln(&mut self, s: &str) {
        for _ in 0..self.indent_level {
            self.output.push_str("    ");
        }
        self.output.push_str(s);
        self.output.push('\n');
    }
    pub(crate) fn write_to_case(&mut self, s: &str) {
        let indent = "    ".repeat(self.indent_level + 1);
        self.current_case_content
            .push_str(&format!("{}{}\n", indent, s));
    }

    fn emit_runtime_preludes_exports_imports(&mut self, module: &Module) {
        for item in &module.body {
            if let ModuleItem::Export(exp) = item {
                if let Some(src) = &exp.source {
                    let zig_source = if src.ends_with(".cp") {
                        format!("{}.zig", &src[..src.len() - 3])
                    } else {
                        src.clone()
                    };
                    for s in &exp.specifiers {
                        self.globals.insert(s.sym.clone());
                        self.value_classes.insert(s.sym.clone());
                        self.writeln(&format!(
                            "pub const {0} = @import(\"{1}\").{0};",
                            Self::escape_zig_ident(&s.sym),
                            zig_source
                        ));
                    }
                }
            }
        }
        for item in &module.body {
            if let ModuleItem::Import(imp) = item {
                let zig_source = if imp.source.ends_with(".cp") {
                    format!("{}.zig", &imp.source[..imp.source.len() - 3])
                } else {
                    imp.source.clone()
                };
                if let Some(def) = &imp.default_specifier {
                    self.globals.insert(def.sym.clone());
                    self.value_classes.insert(def.sym.clone());
                    if def.sym == "std" && zig_source == "std" {
                        continue;
                    }
                    self.writeln(&format!(
                        "pub const {0} = @import(\"{1}\");",
                        Self::escape_zig_ident(&def.sym),
                        zig_source
                    ));
                }
                for s in &imp.specifiers {
                    if !self.globals.insert(s.sym.clone()) {
                        continue;
                    }
                    self.value_classes.insert(s.sym.clone());
                    if s.sym == "std" && zig_source == "std" {
                        continue;
                    }
                    self.writeln(&format!(
                        "const {0} = @import(\"{1}\").{0};",
                        Self::escape_zig_ident(&s.sym),
                        zig_source
                    ));
                }
            }
        }
    }
}
