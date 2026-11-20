use clap::{Parser, Subcommand};
use cpc::ast::Module;
use cpc::codegen::Codegen;
use cpc::parser::Parser as CPParser;
use cpc::sema::Analyzer;
use std::collections::HashSet;
use std::fs;
use std::path::PathBuf;

use cpc::resolver::resolve_imports;

#[derive(Parser)]
#[command(name = "cpc", about = "CP Language Compiler", version)]
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
    },
    /// Automatically apply mechanical syntax fixes in-place (alias for 'check --fix')
    Fix {
        input: PathBuf,
        #[arg(long)]
        json: bool,
        #[arg(short = 'b', long)]
        brief: bool,
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
    /// Run Language Server Protocol daemon
    Lsp,
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

    if lib {
        let code = fs::read_to_string(input)?;
        let mut parser = CPParser::new(&code);
        let (parsed_module, p_diags) = parser.parse_module_with_recovery();
        all_diags.extend(p_diags);
        all_items = parsed_module.body;
    } else {
        resolve_imports(input, include, &mut all_items, &mut processed_sources, true)?;
    }

    let mut module = Module {
        span: cpc::Span { start: 0, end: 0 },
        body: all_items,
    };

    let mut analyzer = Analyzer::new();
    let _ = analyzer.analyze_module(&mut module);
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
    let zig_code_result = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
        let mut codegen = Codegen::new();
        codegen.is_beam_mode = beam;
        codegen.is_lib_mode = lib;
        codegen.is_fuzz_mode = is_fuzz;
        codegen.fuzz_target_filter = fuzz_tgt;
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
            } => {
                run_check(&input, json, brief, max, fix)?;
            }
            Commands::Fix { input, json, brief } => {
                run_check(&input, json, brief, None, true)?;
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
            Commands::Lsp => {
                let rt = tokio::runtime::Runtime::new()?;
                rt.block_on(cpc::lsp::run_lsp());
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

fn collect_diagnostics(code: &str) -> Vec<cpc::diagnostic::Diagnostic> {
    let mut parser = CPParser::new(code);
    let (mut module, mut all_diags) = parser.parse_module_with_recovery();
    let mut analyzer = Analyzer::new();
    let _ = analyzer.analyze_module(&mut module);
    all_diags.extend(analyzer.get_diagnostics());
    cpc::diagnostic::mark_cascades(&mut all_diags, Some(code));
    all_diags
}

fn run_check(
    input: &PathBuf,
    json: bool,
    brief: bool,
    max: Option<usize>,
    fix: bool,
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

    let mut all_diags = collect_diagnostics(&code);
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
            all_diags = collect_diagnostics(&code);
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
