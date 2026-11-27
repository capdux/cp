use clap::{Parser, Subcommand};
use cpc::ast::Module;
use cpc::codegen::Codegen;
use cpc::parser::Parser as CPParser;
use cpc::sema::Analyzer;
use std::collections::{BTreeMap, HashSet};
use std::fs;
use std::path::{Path, PathBuf};

use cpc::resolver::{
    extract_top_level_zig_pub_names, find_cap_core_file, resolve_imports_with_cap_hub,
    CapHubResolution,
};

#[derive(Parser)]
#[command(name = "cpc", about = "CP Language Compiler", version = "1.0.0")]
struct Cli {
    /// Input file to compile directly
    #[arg(value_name = "INPUT")]
    input: Option<PathBuf>,

    /// Output file path
    #[arg(short = 'o', long)]
    output: Option<PathBuf>,

    /// Emit modular per-file Zig outputs into directory (incremental)
    #[arg(long)]
    emit_dir: Option<PathBuf>,

    /// Force re-transpile all modules in --emit-dir mode
    #[arg(long)]
    force: bool,

    /// Include paths for module resolution
    #[arg(short = 'I', long)]
    include: Vec<PathBuf>,

    /// Enable BEAM runtime mode (legacy alias for --nest)
    #[arg(short = 'b', long)]
    beam: bool,

    /// Enable Nest runtime mode
    #[arg(short = 'n', long)]
    nest: bool,

    /// Compile as library module (omit main entry point)
    #[arg(long)]
    lib: bool,

    /// Compile with libFuzzer C-ABI entry point
    #[arg(long)]
    fuzz: bool,

    /// Filter fuzz targets by name or index
    #[arg(long)]
    fuzz_target: Option<String>,

    #[command(subcommand)]
    command: Option<Commands>,
}

#[derive(Subcommand)]
enum Commands {
    /// Compile CP source to Zig (default action)
    Compile {
        input: PathBuf,
        #[arg(short = 'o', long)]
        output: Option<PathBuf>,
        #[arg(long)]
        emit_dir: Option<PathBuf>,
        #[arg(long)]
        force: bool,
        #[arg(short = 'I', long)]
        include: Vec<PathBuf>,
        #[arg(short = 'b', long)]
        beam: bool,
        #[arg(short = 'n', long)]
        nest: bool,
        #[arg(long)]
        lib: bool,
        #[arg(long)]
        fuzz: bool,
        #[arg(long)]
        fuzz_target: Option<String>,
    },
    /// Perform static syntax and borrow-checker check
    Check {
        input: PathBuf,
        #[arg(long)]
        json: bool,
        /// Deduplicate diagnostics by error code and fold cascade errors
        #[arg(short = 'b', long)]
        brief: bool,
        /// Maximum number of diagnostics (or groups in --brief mode) to display
        #[arg(long)]
        max: Option<usize>,
        /// Automatically apply mechanical fixes in-place
        #[arg(long)]
        fix: bool,
        /// Strict CP purification gate: reject legacy Zig/TS syntax (orelse, and, or, prefix try, ?T, : void, self, this, |capture|, zig {}, var)
        #[arg(long)]
        strict: bool,
    },
    /// Automatically apply mechanical syntax fixes in-place (alias for 'check --fix')
    Fix {
        input: PathBuf,
        #[arg(long)]
        json: bool,
        #[arg(short = 'b', long)]
        brief: bool,
    },
    /// Token-saving multi-file verifier: check + --lib codegen + zig ast-check + static purity gates
    Brief {
        /// Max distinct errors to display
        #[arg(short = 'n', default_value_t = 8)]
        max: usize,
        /// Path to zig binary for ast-check (priority: --zig > $ZIG_BIN > ./vendor/zig/zig > ../vendor/zig/zig > /data/dev/cp/vendor/zig/zig > zig)
        #[arg(long)]
        zig: Option<PathBuf>,
        /// Fail gate if migration backup file <stem>_.zig is missing
        #[arg(long)]
        require_backup: bool,
        /// Keep emitted <stem>.zig instead of cleaning up when target did not exist
        #[arg(long)]
        keep_zig: bool,
        /// Run static dead-code audit for unused exported symbols (pub fn/struct/enum/const)
        #[arg(long, alias = "deadcode")]
        dead: bool,
        /// Target .cp files or directories
        #[arg(required = true)]
        files: Vec<PathBuf>,
    },
    /// Format CP source files (4-space indent, preserves comments/strings/zig blocks)
    Fmt {
        /// Check if files are formatted without modifying them (exit 1 if unformatted)
        #[arg(long)]
        check: bool,
        /// Write formatted output to stdout instead of modifying files in-place
        #[arg(long)]
        stdout: bool,
        /// Target .cp files or directories (defaults to current directory)
        paths: Vec<PathBuf>,
    },
    /// Emit .d.cp type declarations
    EmitTypes {
        input: PathBuf,
        #[arg(short = 'o', long)]
        output: Option<PathBuf>,
    },
    /// Demangle runtime stack trace using source maps
    Demangle {
        /// Optional stack trace file (reads stdin if omitted)
        input: Option<PathBuf>,
        /// Optional explicit source map path
        #[arg(short = 'm', long)]
        map: Option<PathBuf>,
    },
    /// Migrate a TypeScript/Rust project or absorb an npm/crates.io package to CP
    From {
        /// Migrate from local TypeScript/JavaScript project directory
        #[arg(long)]
        ts: bool,
        /// Migrate from local Rust crate/workspace directory
        #[arg(long)]
        rs: bool,
        /// Absorb an npm package by name (optional @version), deduplicating via @cap/registry.json
        #[arg(long)]
        npm: Option<String>,
        /// Absorb a Rust crate from crates.io by name (optional @version), deduplicating via @cap/registry.json
        #[arg(long = "crate")]
        crate_pkg: Option<String>,
        /// Source project directory (required for --ts / --rs)
        src_dir: Option<PathBuf>,
        /// Output directory (default: <src_dir>_cp for local, or packages/@cap for --npm/--crate)
        #[arg(short = 'o', long)]
        out: Option<PathBuf>,
        /// Path to @cap deduplication registry JSON (default: packages/@cap/registry.json)
        #[arg(long)]
        registry: Option<PathBuf>,
        /// Disable recursive conversion of runtime dependencies
        #[arg(long)]
        no_deps: bool,
        /// Maximum dependency recursion depth (default: 4)
        #[arg(long, default_value_t = 4)]
        max_depth: usize,
        /// Run cpc syntax/semantic verification + mechanical auto-fix loop on emitted .cp files
        #[arg(long)]
        verify: bool,
        /// Include test files (*.test.ts, *.spec.ts, *.test.tsx, *.spec.tsx, __tests__/, test/, tests/)
        #[arg(long)]
        include_tests: bool,
    },
    /// Run Language Server Protocol daemon
    Lsp,
    /// Run Model Context Protocol (MCP) JSON-RPC 2.0 server over stdio
    Mcp,
}

fn compile_embedded_cap_module(
    sub_path: &Path,
    include: &[PathBuf],
    beam: bool,
    rt_import: &str,
    cap_hub: &mut CapHubResolution,
) -> anyhow::Result<String> {
    let mut sub_items = Vec::new();
    let mut sub_proc = HashSet::new();
    resolve_imports_with_cap_hub(
        sub_path,
        include,
        &mut sub_items,
        &mut sub_proc,
        false,
        cap_hub,
    )?;

    let mut sub_module = Module {
        span: cpc::Span { start: 0, end: 0 },
        body: sub_items,
    };

    let mut analyzer = Analyzer::new();
    let _ = analyzer.analyze_module(&mut sub_module);
    let mut diags = analyzer.get_diagnostics();
    if !diags.is_empty() {
        let code = fs::read_to_string(sub_path).ok();
        cpc::diagnostic::mark_cascades(&mut diags, code.as_deref());
        let mut seen_codes = HashSet::new();
        let rendered = diags
            .iter()
            .filter(|d| d.cascade_of.is_none())
            .map(|d| {
                let seen = !seen_codes.insert(d.code.clone());
                d.render_dedup(code.as_deref(), Some(sub_path), seen)
            })
            .collect::<Vec<_>>()
            .join("\n");
        return Err(anyhow::anyhow!("{}", rendered));
    }

    let code_str = fs::read_to_string(sub_path).ok();
    let sub_path_owned = sub_path.to_path_buf();
    let rt_import_owned = rt_import.to_string();
    let res = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        let mut cg = Codegen::new();
        cg.is_beam_mode = beam;
        cg.is_lib_mode = false;
        cg.modular_mode = true;
        cg.modular_is_entry = false;
        cg.single_file_cap_submod = true;
        cg.modular_runtime_import = Some(rt_import_owned);
        cg.source_dir = sub_path_owned.parent().map(|p| p.to_path_buf());
        if let Some(ref c) = code_str {
            cg.init_source(Some(&sub_path_owned.display().to_string()), Some(c));
        }
        cg.generate_module(&sub_module)
    }));

    match res {
        Ok(zig) => Ok(zig),
        Err(err) => {
            let msg = if let Some(s) = err.downcast_ref::<&str>() {
                s.to_string()
            } else if let Some(s) = err.downcast_ref::<String>() {
                s.clone()
            } else {
                "Internal compiler error".to_string()
            };
            let code = fs::read_to_string(sub_path).ok();
            let diag = cpc::diagnostic::from_compiler_error_msg(&msg, code.as_deref());
            Err(anyhow::anyhow!(
                "{}",
                diag.render(code.as_deref(), Some(sub_path))
            ))
        }
    }
}

fn write_if_changed_file(path: &Path, content: &[u8]) -> anyhow::Result<()> {
    if let Ok(existing) = fs::read(path) {
        if existing == content {
            return Ok(());
        }
    }
    if let Some(parent) = path.parent() {
        fs::create_dir_all(parent)?;
    }
    fs::write(path, content)?;
    Ok(())
}

fn indent_zig_struct_body(code: &str) -> String {
    let mut out = String::with_capacity(code.len() + 256);
    for line in code.lines() {
        if line.is_empty() {
            out.push('\n');
        } else {
            out.push_str("    ");
            out.push_str(line);
            out.push('\n');
        }
    }
    out
}

fn build_single_file_cap_hub_prelude(
    output: Option<&PathBuf>,
    include: &[PathBuf],
    cap_hub: &mut CapHubResolution,
    beam: bool,
) -> anyhow::Result<String> {
    let rt_import = if output.is_some() {
        "./__cp_runtime.zig"
    } else {
        "__cap_hub_rt"
    };

    let mut core_zig = String::new();
    if let Some(core_abs) = find_cap_core_file(include) {
        core_zig = compile_embedded_cap_module(&core_abs, include, beam, rt_import, cap_hub)?;
    }

    let mut compiled_ns: BTreeMap<String, String> = BTreeMap::new();
    loop {
        let pending: Vec<(String, PathBuf)> = cap_hub
            .mounted_ns
            .iter()
            .filter(|(ns, _)| !compiled_ns.contains_key(*ns))
            .map(|(ns, p)| (ns.clone(), p.clone()))
            .collect();
        if pending.is_empty() {
            break;
        }
        for (ns, sub_abs) in pending {
            let sub_zig = compile_embedded_cap_module(&sub_abs, include, beam, rt_import, cap_hub)?;
            compiled_ns.insert(ns, sub_zig);
        }
    }

    if let Some(out_path) = output {
        let out_parent = out_path.parent().unwrap_or(Path::new("."));
        let stem = out_path
            .file_stem()
            .and_then(|s| s.to_str())
            .unwrap_or("module");
        let hub_dir_name = format!("__cap_hub_{}", stem);
        let hub_dir = out_parent.join(&hub_dir_name);
        fs::create_dir_all(&hub_dir)?;

        write_if_changed_file(
            &hub_dir.join("__cp_runtime.zig"),
            Codegen::generate_shared_runtime().as_bytes(),
        )?;
        write_if_changed_file(&hub_dir.join("core.zig"), core_zig.as_bytes())?;

        for (ns, sub_zig) in &compiled_ns {
            write_if_changed_file(&hub_dir.join(format!("ns_{}.zig", ns)), sub_zig.as_bytes())?;
        }

        let mut index_zig = String::from(
            "// Generated by CP Compiler — Lazy Cap Hub Index (Single-File Isolated)\n",
        );
        index_zig.push_str("const __core = @import(\"./core.zig\");\n\n");
        let mut emitted: HashSet<String> = HashSet::new();
        for sym in extract_top_level_zig_pub_names(&core_zig) {
            if compiled_ns.contains_key(&sym) {
                continue;
            }
            if emitted.insert(sym.clone()) {
                let esc = Codegen::escape_zig_ident(&sym);
                index_zig.push_str(&format!("pub const {0} = __core.{0};\n", esc));
            }
        }
        if !compiled_ns.is_empty() {
            index_zig.push_str("\n// Lazily mounted sub-namespaces:\n");
            for ns in compiled_ns.keys() {
                if emitted.insert(ns.clone()) {
                    let esc = Codegen::escape_zig_ident(ns);
                    index_zig.push_str(&format!(
                        "pub const {} = @import(\"./ns_{}.zig\");\n",
                        esc, ns
                    ));
                }
            }
        }
        write_if_changed_file(&hub_dir.join("index.zig"), index_zig.as_bytes())?;

        return Ok(format!(
            "const __cap_hub = @import(\"./{}/index.zig\");\n",
            hub_dir_name
        ));
    }

    // Fallback when emitting to stdout without --output
    let mut out = String::new();
    out.push_str("// --- Lazy Cap Hub (Inline Preview) ---\n");
    out.push_str("const __cap_hub_rt = struct {\n");
    out.push_str(&indent_zig_struct_body(&Codegen::generate_shared_runtime()));
    out.push_str("};\n\n");

    out.push_str("const __cap_hub_core = struct {\n");
    out.push_str(&indent_zig_struct_body(&core_zig));
    out.push_str("};\n\n");

    for (ns, sub_zig) in &compiled_ns {
        out.push_str(&format!("const __cap_hub_ns_{} = struct {{\n", ns));
        out.push_str(&indent_zig_struct_body(sub_zig));
        out.push_str("};\n\n");
    }

    out.push_str("const __cap_hub = struct {\n");
    let mut emitted: HashSet<String> = HashSet::new();
    for sym in extract_top_level_zig_pub_names(&core_zig) {
        if compiled_ns.contains_key(&sym) {
            continue;
        }
        if emitted.insert(sym.clone()) {
            let esc = Codegen::escape_zig_ident(&sym);
            out.push_str(&format!("    pub const {0} = __cap_hub_core.{0};\n", esc));
        }
    }
    for ns in compiled_ns.keys() {
        if emitted.insert(ns.clone()) {
            let esc = Codegen::escape_zig_ident(ns);
            out.push_str(&format!("    pub const {} = __cap_hub_ns_{};\n", esc, ns));
        }
    }
    out.push_str("};\n");
    Ok(out)
}

fn compile_file(
    input: &PathBuf,
    output: Option<&PathBuf>,
    include: &[PathBuf],
    beam: bool,
    lib: bool,
    fuzz: bool,
    fuzz_target: Option<String>,
) -> anyhow::Result<()> {
    let mut all_items = Vec::new();
    let mut processed_sources = HashSet::new();
    let mut all_diags = Vec::new();
    let mut cap_hub = CapHubResolution::default();

    if lib {
        let code = fs::read_to_string(input)?;
        let mut parser = CPParser::new(&code);
        let (parsed_module, p_diags) = parser.parse_module_with_recovery();
        all_diags.extend(p_diags);
        let referenced = cpc::ast_visitor::collect_module_referenced_idents(&parsed_module);
        for item in parsed_module.body {
            match &item {
                cpc::ast::ModuleItem::Import(imp) => {
                    // In `--lib` mode, preserve all top-level imports in `all_items` so library
                    // barrel files (e.g. `import Async from "async";` -> `pub const Async = @import("async");`)
                    // never drop re-exported module bindings; only emit `// @cap-used-pkg:` when referenced.
                    if cpc::resolver::is_external_package_source(&imp.source) {
                        if cpc::resolver::is_import_used(imp, &referenced) {
                            cap_hub.used_packages.insert(imp.source.clone());
                        }
                    } else if imp.source.starts_with("cap:")
                        && cpc::resolver::is_import_used(imp, &referenced)
                    {
                        cap_hub.used_packages.insert(imp.source.clone());
                    }
                }
                cpc::ast::ModuleItem::Export(exp) => {
                    if let Some(src) = &exp.source {
                        if cpc::resolver::is_external_package_source(src) || src.starts_with("cap:")
                        {
                            cap_hub.used_packages.insert(src.clone());
                        }
                    }
                }
                _ => {}
            }
            all_items.push(item);
        }
    } else {
        resolve_imports_with_cap_hub(
            input,
            include,
            &mut all_items,
            &mut processed_sources,
            true,
            &mut cap_hub,
        )?;
    }

    let cap_hub_prelude = if !lib && cap_hub.uses_cap_hub {
        Some(build_single_file_cap_hub_prelude(
            output,
            include,
            &mut cap_hub,
            beam,
        )?)
    } else {
        None
    };

    let mut module = Module {
        span: cpc::Span { start: 0, end: 0 },
        body: all_items,
    };

    let mut analyzer = Analyzer::new();
    if !lib
        && !cap_hub.root_item_indices.is_empty()
        && cap_hub.root_item_indices.len() < module.body.len()
    {
        let mut root_items = Vec::with_capacity(cap_hub.root_item_indices.len());
        for (idx, item) in module.body.iter().enumerate() {
            if cap_hub.root_item_indices.contains(&idx) {
                root_items.push(item.clone());
            } else {
                match item {
                    cpc::ast::ModuleItem::Decl(d) => analyzer.register_imported_decl(d),
                    cpc::ast::ModuleItem::Stmt(cpc::ast::Stmt::Var(v)) => {
                        analyzer.register_imported_var(v);
                    }
                    _ => {}
                }
            }
        }
        let mut root_module = Module {
            span: module.span.clone(),
            body: root_items,
        };
        let _ = analyzer.analyze_module(&mut root_module);
    } else {
        let _ = analyzer.analyze_module(&mut module);
    }
    all_diags.extend(analyzer.get_diagnostics());

    if !all_diags.is_empty() {
        let code = fs::read_to_string(input).ok();
        cpc::diagnostic::mark_cascades(&mut all_diags, code.as_deref());
        let mut seen_codes = HashSet::new();
        let rendered = all_diags
            .iter()
            .filter(|d| d.cascade_of.is_none())
            .map(|d| {
                let seen = !seen_codes.insert(d.code.clone());
                d.render_dedup(code.as_deref(), Some(input), seen)
            })
            .collect::<Vec<_>>()
            .join("\n");
        return Err(anyhow::anyhow!("{}", rendered));
    }

    let is_fuzz = fuzz || std::env::var("CP_FUZZ").is_ok();
    let fuzz_tgt = fuzz_target.or_else(|| std::env::var("CP_FUZZ_TARGET").ok());

    let code_str = fs::read_to_string(input).ok();
    let used_packages = cap_hub.used_packages;
    let zig_code_result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        let mut codegen = Codegen::new();
        codegen.is_beam_mode = beam;
        codegen.is_lib_mode = lib;
        codegen.is_fuzz_mode = is_fuzz;
        codegen.fuzz_target_filter = fuzz_tgt;
        codegen.cap_hub_prelude = cap_hub_prelude;
        codegen.used_packages = used_packages;
        codegen.source_dir = input.parent().map(|p| p.to_path_buf());
        if let Some(ref c) = code_str {
            codegen.init_source(Some(&input.display().to_string()), Some(c));
        }
        let code = codegen.generate_module(&module);
        (code, codegen.source_map_entries)
    }));

    let (zig_code, map_entries) = match zig_code_result {
        Ok(res) => res,
        Err(err) => {
            let msg = if let Some(s) = err.downcast_ref::<&str>() {
                s.to_string()
            } else if let Some(s) = err.downcast_ref::<String>() {
                s.clone()
            } else {
                "Internal compiler error".to_string()
            };
            eprintln!("DEBUG PANIC: {}", msg);
            let code = fs::read_to_string(input).ok();
            let diag = cpc::diagnostic::from_compiler_error_msg(&msg, code.as_deref());
            return Err(anyhow::anyhow!(
                "{}",
                diag.render(code.as_deref(), Some(input))
            ));
        }
    };

    match output {
        Some(path) => {
            let mut need_write = true;
            if let Ok(existing) = fs::read_to_string(path) {
                if existing == zig_code {
                    need_write = false;
                }
            }
            if need_write {
                fs::write(path, &zig_code)?;
            }
            let mut sm = cpc::source_map::SourceMap::new(
                input.display().to_string(),
                path.display().to_string(),
            );
            sm.mappings = map_entries;
            let map_path = path.with_extension("zig.map");
            let _ = fs::write(map_path, sm.to_json());
        }
        None => println!("{}", zig_code),
    }
    Ok(())
}

fn main() {
    std::panic::set_hook(Box::new(|_| {}));
    let cli = Cli::parse();

    if let Err(e) = run(cli) {
        let err_str = e.to_string();
        if err_str.starts_with("error[") {
            eprint!("{}", err_str);
        } else {
            eprintln!("Error: {}", err_str);
        }
        std::process::exit(1);
    }
}

fn run(cli: Cli) -> anyhow::Result<()> {
    if let Some(cmd) = cli.command {
        match cmd {
            Commands::Compile {
                input,
                output,
                emit_dir,
                force,
                include,
                beam,
                nest,
                lib,
                fuzz,
                fuzz_target,
            } => {
                if let Some(ref dir) = emit_dir {
                    cpc::modular::compile_modular(&cpc::modular::ModularOptions {
                        entry: &input,
                        emit_dir: dir,
                        include_paths: &include,
                        beam: beam || nest,
                        lib,
                        fuzz,
                        fuzz_target,
                        force,
                    })?;
                } else {
                    compile_file(
                        &input,
                        output.as_ref(),
                        &include,
                        beam || nest,
                        lib,
                        fuzz,
                        fuzz_target,
                    )?;
                }
            }
            Commands::Check {
                input,
                json,
                brief,
                max,
                fix,
                strict,
            } => {
                run_check(&input, json, brief, max, fix, strict)?;
            }
            Commands::Fix { input, json, brief } => {
                run_check(&input, json, brief, None, true, false)?;
            }
            Commands::Brief {
                max,
                zig,
                require_backup,
                keep_zig,
                dead,
                files,
            } => {
                let ok = if dead {
                    cpc::deadcode::run_deadcode(&files)?
                } else {
                    cpc::brief::run_brief(&cpc::brief::BriefOptions {
                        max,
                        zig,
                        require_backup,
                        keep_zig,
                        files,
                    })?
                };
                if !ok {
                    std::process::exit(1);
                }
            }
            Commands::Fmt {
                check,
                stdout,
                paths,
            } => {
                let ok = cpc::formatter::run_fmt(&paths, check, stdout)?;
                if !ok {
                    std::process::exit(1);
                }
            }
            Commands::EmitTypes { input, output } => {
                let code = fs::read_to_string(&input)?;
                let mut parser = CPParser::new(&code);
                let module = parser
                    .parse_module()
                    .map_err(|e| anyhow::anyhow!("Parse Error in {:?}: {}", input, e))?;
                let mut emitter = cpc::dcp::DcpEmitter::new();
                let type_defs = emitter.emit_module(&module);

                let out_path = output.unwrap_or_else(|| input.with_extension("d.cp"));
                if let Some(parent) = out_path.parent() {
                    fs::create_dir_all(parent)?;
                }
                fs::write(&out_path, type_defs)?;
            }
            Commands::Demangle { input, map } => {
                let trace = match input {
                    Some(p) => fs::read_to_string(&p)?,
                    None => {
                        use std::io::Read;
                        let mut buf = String::new();
                        std::io::stdin().read_to_string(&mut buf)?;
                        buf
                    }
                };
                let sm = match map {
                    Some(m_path) => Some(cpc::source_map::SourceMap::from_file(&m_path)?),
                    None => None,
                };
                let demangled = cpc::source_map::demangle_stack_trace(&trace, sm.as_ref());
                print!("{}", demangled);
            }
            Commands::From {
                ts,
                rs,
                npm,
                crate_pkg,
                src_dir,
                out,
                registry,
                no_deps,
                max_depth,
                verify,
                include_tests,
            } => {
                if npm.is_some() || crate_pkg.is_some() {
                    use cpc::migrate::registry::{
                        locate_default_registry_path, run_registry_import, PackageEcosystem,
                        RegistryImportOptions,
                    };
                    let (ecosystem, spec) = match (npm, crate_pkg) {
                        (Some(s), None) => (PackageEcosystem::Npm, s),
                        (None, Some(s)) => (PackageEcosystem::Crate, s),
                        _ => {
                            return Err(anyhow::anyhow!(
                                "Specify only one of `--npm <pkg>` or `--crate <name>`"
                            ));
                        }
                    };
                    let registry_path = registry.unwrap_or_else(locate_default_registry_path);
                    let packages_root = out.unwrap_or_else(|| PathBuf::from("packages"));
                    let outcome = run_registry_import(RegistryImportOptions {
                        ecosystem,
                        spec: spec.clone(),
                        registry_path: registry_path.clone(),
                        packages_root,
                        max_depth,
                        verify,
                        no_deps,
                    })
                    .map_err(|e| anyhow::anyhow!("{}", e))?;
                    if outcome.already_existed {
                        if outcome.kind == "stdlib" {
                            println!(
                                "[cap from] '{}' is already built into CP stdlib -> use: import {{ ... }} from \"{}\"; (zero installation needed, skipped duplicate conversion)",
                                spec, outcome.cp_target
                            );
                        } else {
                            println!(
                                "[cap from] '{}' is already available in packages/registry.json -> use: import {{ ... }} from \"{}\"; (skipped duplicate conversion)",
                                spec, outcome.cp_target
                            );
                        }
                    } else {
                        println!(
                            "[cap from] Absorbed '{}' -> {} (independent packages generated: {}) | registry updated: {}",
                            spec,
                            outcome.cp_target,
                            outcome.converted_packages.join(", "),
                            registry_path.display()
                        );
                    }
                } else {
                    use cpc::migrate::deps::SourceLang;
                    use cpc::migrate::{run_migrate, MigrateOptions};
                    let lang = if ts && !rs {
                        SourceLang::Ts
                    } else if rs && !ts {
                        SourceLang::Rs
                    } else {
                        return Err(anyhow::anyhow!(
                            "Specify one mode: `--ts <dir>`, `--rs <dir>`, `--npm <pkg[@ver]>`, or `--crate <name[@ver]>`"
                        ));
                    };
                    let src_dir = src_dir.ok_or_else(|| {
                        anyhow::anyhow!("Missing <src_dir> argument for `--ts` / `--rs`")
                    })?;
                    let out_dir = out.unwrap_or_else(|| {
                        let base = src_dir
                            .file_name()
                            .and_then(|n| n.to_str())
                            .unwrap_or("migrated");
                        PathBuf::from(format!("{}_cp", base))
                    });
                    let summary = run_migrate(MigrateOptions {
                        lang,
                        src_dir,
                        out_dir: out_dir.clone(),
                        recursive_deps: !no_deps,
                        max_depth,
                        verify,
                        include_tests,
                    })
                    .map_err(|e| anyhow::anyhow!("{}", e))?;
                    let total = summary.files.len();
                    let parse_ok = summary.files.iter().filter(|f| f.parse_ok).count();
                    let total_todos: usize = summary.files.iter().map(|f| f.todo_count).sum();
                    let pct = if total == 0 {
                        100.0
                    } else {
                        (parse_ok as f64) * 100.0 / (total as f64)
                    };
                    println!(
                        "[cap from] Migrated '{}' -> {} | files={} (cpc parse ok: {}/{}, {:.1}%) | deps={} (recursive pkgs: {}) | TODOs={}",
                        summary.project_name,
                        summary.out_dir,
                        total,
                        parse_ok,
                        total,
                        pct,
                        summary.deps.len(),
                        summary.converted_dep_pkgs.len(),
                        total_todos
                    );
                    if let Some(ref v) = summary.verify {
                        println!("[cap from] {}", v.format_line());
                    }
                    println!(
                        "[cap from] Report written to: {}/migrate.md",
                        summary.out_dir
                    );
                }
            }
            Commands::Lsp => {
                let rt = tokio::runtime::Runtime::new()?;
                rt.block_on(cpc::lsp::run_lsp());
            }
            Commands::Mcp => {
                cpc::mcp::run_mcp_stdio_server()?;
            }
        }
    } else if let Some(input) = cli.input {
        if let Some(ref dir) = cli.emit_dir {
            cpc::modular::compile_modular(&cpc::modular::ModularOptions {
                entry: &input,
                emit_dir: dir,
                include_paths: &cli.include,
                beam: cli.beam || cli.nest,
                lib: cli.lib,
                fuzz: cli.fuzz,
                fuzz_target: cli.fuzz_target,
                force: cli.force,
            })?;
        } else {
            compile_file(
                &input,
                cli.output.as_ref(),
                &cli.include,
                cli.beam || cli.nest,
                cli.lib,
                cli.fuzz,
                cli.fuzz_target,
            )?;
        }
    } else {
        use clap::CommandFactory;
        Cli::command().print_help()?;
        println!();
    }
    Ok(())
}

fn collect_diagnostics(code: &str, strict: bool) -> Vec<cpc::diagnostic::Diagnostic> {
    let mut parser = CPParser::new(code);
    let (mut module, mut all_diags) = parser.parse_module_with_recovery();
    let mut analyzer = Analyzer::new();
    let _ = analyzer.analyze_module(&mut module);
    all_diags.extend(analyzer.get_diagnostics());
    if strict {
        let strict_diags = cpc::strict::collect_strict_diagnostics(code, &all_diags);
        all_diags.extend(strict_diags);
    }
    cpc::diagnostic::mark_cascades(&mut all_diags, Some(code));
    all_diags
}

fn run_check(
    input: &PathBuf,
    json: bool,
    brief: bool,
    max: Option<usize>,
    fix: bool,
    strict: bool,
) -> anyhow::Result<()> {
    let mut code = match fs::read_to_string(input) {
        Ok(c) => c,
        Err(e) => {
            if json {
                let diag = cpc::diagnostic::Diagnostic::new(
                    "E0001",
                    format!("failed to read input file '{}': {}", input.display(), e),
                );
                println!("{}", diag.to_json(None, Some(input)));
                std::process::exit(1);
            } else {
                return Err(anyhow::anyhow!("failed to read file {:?}: {}", input, e));
            }
        }
    };

    let mut all_diags = collect_diagnostics(&code, strict);
    let mut total_fixed = 0usize;

    if fix && !all_diags.is_empty() {
        // Run up to 3 passes of mechanical non-overlapping edits
        for _ in 0..3 {
            let (fixed_code, applied) = cpc::diagnostic::apply_fixes(&code, &all_diags);
            if applied == 0 || fixed_code == code {
                break;
            }
            total_fixed += applied;
            code = fixed_code;
            all_diags = collect_diagnostics(&code, strict);
            if all_diags.is_empty() {
                break;
            }
        }
        if total_fixed > 0 {
            fs::write(input, &code)?;
            if !json {
                eprintln!(
                    "Applied {} mechanical fix(es) to {}",
                    total_fixed,
                    input.display()
                );
            }
        }
    }

    if brief {
        let report = cpc::diagnostic::build_brief_report(&all_diags, Some(&code), Some(input), max);
        if json {
            println!(
                "{}",
                serde_json::to_string(&report).unwrap_or_else(|_| "{}".to_string())
            );
        } else {
            eprint!(
                "{}",
                cpc::diagnostic::render_brief_text(&report, Some(input))
            );
        }
        if report.errors > 0 {
            std::process::exit(1);
        } else {
            std::process::exit(0);
        }
    }

    if !all_diags.is_empty() {
        let total_errors = all_diags.len();
        let root_errors = all_diags.iter().filter(|d| d.cascade_of.is_none()).count();
        if json {
            let mut diagnostics: Vec<_> = all_diags
                .iter()
                .map(|d| d.to_diagnostic_json(Some(&code), Some(input)))
                .collect();
            if let Some(limit) = max {
                diagnostics.truncate(limit);
            }
            let report = cpc::diagnostic::DiagnosticReport {
                status: "error".to_string(),
                errors: total_errors,
                root_errors,
                diagnostics,
            };
            println!(
                "{}",
                serde_json::to_string(&report)
                    .unwrap_or_else(|_| "{\"status\":\"error\",\"diagnostics\":[]}".to_string())
            );
            std::process::exit(1);
        } else {
            let mut seen_codes = HashSet::new();
            let root_iter = all_diags.iter().filter(|d| d.cascade_of.is_none());
            let items: Vec<_> = if let Some(limit) = max {
                root_iter.take(limit).collect()
            } else {
                root_iter.collect()
            };
            let rendered = items
                .into_iter()
                .map(|d| {
                    let seen = !seen_codes.insert(d.code.clone());
                    d.render_dedup(Some(&code), Some(input), seen)
                })
                .collect::<Vec<_>>()
                .join("\n");
            return Err(anyhow::anyhow!("{}", rendered));
        }
    }

    if json {
        println!("{}", cpc::diagnostic::DiagnosticReport::ok_json());
        std::process::exit(0);
    } else {
        println!("Check successful");
        Ok(())
    }
}
