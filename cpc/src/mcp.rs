use crate::codegen::Codegen;
use crate::dcp::DcpEmitter;
use crate::diagnostic;
use crate::parser::Parser as CPParser;
use crate::sema::Analyzer;
use serde_json::{json, Value};
use std::collections::{BTreeMap, HashSet};
use std::fmt::Write as _;
use std::fs;
use std::io::{self, BufRead, Write};
use std::path::{Path, PathBuf};
use std::process::Command;

const PROTOCOL_VERSION: &str = "2025-03-26";
const SERVER_NAME: &str = "cpc-mcp";
const SERVER_VERSION: &str = "1.0.0";

/// Run the MCP (Model Context Protocol) JSON-RPC 2.0 server over stdio.
/// Reads one JSON-RPC request per line from stdin and writes one JSON-RPC
/// response per line to stdout. Diagnostic logs are written exclusively to stderr.
pub fn run_mcp_stdio_server() -> anyhow::Result<()> {
    let stdin = io::stdin();
    let stdout = io::stdout();
    let mut out = stdout.lock();

    eprintln!("[cpc-mcp] server started (protocol {})", PROTOCOL_VERSION);

    for line_res in stdin.lock().lines() {
        let line = match line_res {
            Ok(l) => l,
            Err(e) => {
                eprintln!("[cpc-mcp] stdin read error: {}", e);
                break;
            }
        };
        let trimmed = line.trim();
        if trimmed.is_empty() {
            continue;
        }
        if let Some(resp) = handle_mcp_request_line(trimmed) {
            writeln!(out, "{}", resp)?;
            out.flush()?;
        }
    }

    eprintln!("[cpc-mcp] server stopped");
    Ok(())
}

/// Handle a single JSON-RPC 2.0 request line and optionally return a serialized response line.
/// Notifications (requests without `id`, such as `notifications/initialized`) return `None`.
pub fn handle_mcp_request_line(line: &str) -> Option<String> {
    let req: Value = match serde_json::from_str(line) {
        Ok(v) => v,
        Err(e) => {
            return Some(json_rpc_error(
                Value::Null,
                -32700,
                &format!("Parse error: {}", e),
            ));
        }
    };

    let id = req.get("id").cloned();
    let method = match req.get("method").and_then(Value::as_str) {
        Some(m) => m,
        None => {
            return id
                .map(|req_id| json_rpc_error(req_id, -32600, "Invalid Request: missing method"));
        }
    };
    let params = req.get("params");

    // Notifications carry no `id` and must not receive a response.
    if method == "notifications/initialized"
        || (id.is_none() && method.starts_with("notifications/"))
    {
        return None;
    }

    let req_id = match id {
        Some(v) => v,
        None => return None,
    };

    match method {
        "initialize" => Some(json_rpc_result(
            req_id,
            json!({
                "protocolVersion": PROTOCOL_VERSION,
                "serverInfo": {
                    "name": SERVER_NAME,
                    "version": SERVER_VERSION
                },
                "capabilities": {
                    "tools": {}
                }
            }),
        )),
        "ping" => Some(json_rpc_result(req_id, json!({}))),
        "tools/list" => Some(json_rpc_result(
            req_id,
            json!({ "tools": tool_definitions() }),
        )),
        "tools/call" => Some(handle_tools_call(req_id, params)),
        _ => Some(json_rpc_error(
            req_id,
            -32601,
            &format!("Method not found: {}", method),
        )),
    }
}

fn json_rpc_result(id: Value, result: Value) -> String {
    json!({
        "jsonrpc": "2.0",
        "id": id,
        "result": result
    })
    .to_string()
}

fn json_rpc_error(id: Value, code: i64, message: &str) -> String {
    json!({
        "jsonrpc": "2.0",
        "id": id,
        "error": {
            "code": code,
            "message": message
        }
    })
    .to_string()
}

fn mcp_text_result(id: Value, text: String, is_error: bool) -> String {
    let mut result = json!({
        "content": [
            {
                "type": "text",
                "text": text
            }
        ]
    });
    if is_error {
        if let Some(obj) = result.as_object_mut() {
            obj.insert("isError".to_string(), Value::Bool(true));
        }
    }
    json_rpc_result(id, result)
}

fn tool_definitions() -> Value {
    json!([
        {
            "name": "cpc_check",
            "description": "Run static parsing and semantic/borrow checking on a .cp file (with optional in-place --fix), returning a structured JSON diagnostic summary with error codes, line numbers, causes, and suggestions.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "path": {
                        "type": "string",
                        "description": "Path to the .cp source file to check."
                    },
                    "fix": {
                        "type": "boolean",
                        "description": "Optional. If true, apply mechanical fixes in-place before reporting remaining diagnostics."
                    }
                },
                "required": ["path"]
            }
        },
        {
            "name": "cpc_brief",
            "description": "Run the compact all-in-one verifier (cpc check + --lib codegen + zig ast-check + 4-Gate purity audit) and return a token-efficient single-block report.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "path": {
                        "type": "string",
                        "description": "Path to the .cp file to verify."
                    },
                    "max": {
                        "type": "integer",
                        "description": "Optional. Maximum number of distinct errors to display (default: 8)."
                    }
                },
                "required": ["path"]
            }
        },
        {
            "name": "cpc_emit_types",
            "description": "Generate compact .d.cp interface/type signature declarations for a .cp file so AI agents can understand large CP libraries with >90% fewer tokens.",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "path": {
                        "type": "string",
                        "description": "Path to the .cp source file."
                    }
                },
                "required": ["path"]
            }
        },
        {
            "name": "cpc_search_stdlib",
            "description": "Search cp_lib_ref.md for CP standard library (cap:*) and official @web/* plugin contract entries matching the query (returns up to 10 matching lines).",
            "inputSchema": {
                "type": "object",
                "properties": {
                    "query": {
                        "type": "string",
                        "description": "Keyword or phrase to search in cp_lib_ref.md."
                    }
                },
                "required": ["query"]
            }
        }
    ])
}

fn handle_tools_call(id: Value, params: Option<&Value>) -> String {
    let Some(params_obj) = params.and_then(Value::as_object) else {
        return json_rpc_error(id, -32602, "Invalid params: expected object");
    };
    let Some(tool_name) = params_obj.get("name").and_then(Value::as_str) else {
        return json_rpc_error(id, -32602, "Invalid params: missing tool 'name'");
    };
    let empty_args = json!({});
    let args = params_obj.get("arguments").unwrap_or(&empty_args);

    match tool_name {
        "cpc_check" => {
            let Some(path_str) = args.get("path").and_then(Value::as_str) else {
                return json_rpc_error(id, -32602, "Invalid params: missing required 'path'");
            };
            let fix = args.get("fix").and_then(Value::as_bool).unwrap_or(false);
            let (text, is_err) = tool_cpc_check(Path::new(path_str), fix);
            mcp_text_result(id, text, is_err)
        }
        "cpc_brief" => {
            let Some(path_str) = args.get("path").and_then(Value::as_str) else {
                return json_rpc_error(id, -32602, "Invalid params: missing required 'path'");
            };
            let max = args
                .get("max")
                .and_then(Value::as_u64)
                .map(|v| (v as usize).max(1))
                .unwrap_or(8);
            let (text, is_err) = tool_cpc_brief(Path::new(path_str), max);
            mcp_text_result(id, text, is_err)
        }
        "cpc_emit_types" => {
            let Some(path_str) = args.get("path").and_then(Value::as_str) else {
                return json_rpc_error(id, -32602, "Invalid params: missing required 'path'");
            };
            let (text, is_err) = tool_cpc_emit_types(Path::new(path_str));
            mcp_text_result(id, text, is_err)
        }
        "cpc_search_stdlib" => {
            let Some(query) = args.get("query").and_then(Value::as_str) else {
                return json_rpc_error(id, -32602, "Invalid params: missing required 'query'");
            };
            let (text, is_err) = tool_cpc_search_stdlib(query);
            mcp_text_result(id, text, is_err)
        }
        _ => json_rpc_error(id, -32601, &format!("Unknown tool: {}", tool_name)),
    }
}

fn collect_file_diagnostics(code: &str) -> Vec<diagnostic::Diagnostic> {
    let mut parser = CPParser::new(code);
    let (mut module, mut all_diags) = parser.parse_module_with_recovery();
    let mut analyzer = Analyzer::new();
    let _ = analyzer.analyze_module(&mut module);
    all_diags.extend(analyzer.get_diagnostics());
    diagnostic::mark_cascades(&mut all_diags, Some(code));
    all_diags
}

fn tool_cpc_check(path: &Path, fix: bool) -> (String, bool) {
    let mut code = match fs::read_to_string(path) {
        Ok(c) => c,
        Err(e) => {
            let err_json = json!({
                "status": "error",
                "path": path.display().to_string(),
                "errors": 1,
                "root_errors": 1,
                "diagnostics": [{
                    "code": "E0001",
                    "line": 0,
                    "col": 0,
                    "message": format!("failed to read input file '{}': {}", path.display(), e),
                    "cause": Value::Null,
                    "help": Value::Null,
                    "suggestion": Value::Null
                }]
            });
            return (err_json.to_string(), true);
        }
    };

    let mut all_diags = collect_file_diagnostics(&code);
    let mut total_fixed = 0usize;

    if fix && !all_diags.is_empty() {
        for _ in 0..3 {
            let (fixed_code, applied) = diagnostic::apply_fixes(&code, &all_diags);
            if applied == 0 || fixed_code == code {
                break;
            }
            total_fixed += applied;
            code = fixed_code;
            all_diags = collect_file_diagnostics(&code);
            if all_diags.is_empty() {
                break;
            }
        }
        if total_fixed > 0 {
            if let Err(e) = fs::write(path, &code) {
                let err_json = json!({
                    "status": "error",
                    "path": path.display().to_string(),
                    "fixed": total_fixed,
                    "message": format!("failed to write fixed file '{}': {}", path.display(), e)
                });
                return (err_json.to_string(), true);
            }
        }
    }

    let root_errors = all_diags.iter().filter(|d| d.cascade_of.is_none()).count();
    let diags_json: Vec<Value> = all_diags
        .iter()
        .map(|d| {
            let dj = d.to_diagnostic_json(Some(&code), Some(path));
            json!({
                "code": dj.code,
                "line": dj.line,
                "col": dj.col,
                "message": dj.message,
                "cause": dj.cause,
                "help": dj.help,
                "suggestion": dj.suggestion,
                "cascade_of": dj.cascade_of
            })
        })
        .collect();

    let report = json!({
        "status": if all_diags.is_empty() { "ok" } else { "error" },
        "path": path.display().to_string(),
        "fixed": total_fixed,
        "errors": all_diags.len(),
        "root_errors": root_errors,
        "diagnostics": diags_json
    });

    (report.to_string(), false)
}

fn truncate_chars(s: &str, max_chars: usize) -> &str {
    match s.char_indices().nth(max_chars) {
        Some((idx, _)) => &s[..idx],
        None => s,
    }
}

fn count_zig_blocks(txt: &str) -> usize {
    use crate::lexer::Token;
    use logos::Logos;

    let tokens: Vec<Option<Token>> = Token::lexer(txt).map(Result::ok).collect();
    let mut count = 0usize;
    for i in 0..tokens.len() {
        if matches!(tokens[i], Some(Token::ZigEscape)) {
            let is_member_access =
                i > 0 && matches!(tokens[i - 1], Some(Token::Dot | Token::QuestionDot));
            let followed_by_lbrace =
                i + 1 < tokens.len() && matches!(tokens[i + 1], Some(Token::LBrace));
            if !is_member_access && followed_by_lbrace {
                count += 1;
            }
        }
    }
    count
}

fn count_var_decls(txt: &str) -> usize {
    let mut count = 0usize;
    for line in txt.lines() {
        let mut s = line.trim_start();
        if let Some(rest) = s.strip_prefix("pub") {
            if rest.starts_with(char::is_whitespace) {
                s = rest.trim_start();
            }
        }
        if let Some(rest) = s.strip_prefix("var") {
            if rest.starts_with(char::is_whitespace) {
                count += 1;
            }
        }
    }
    count
}

fn count_tuple_at_n(txt: &str) -> usize {
    let bytes = txt.as_bytes();
    let len = bytes.len();
    let mut count = 0usize;
    let mut i = 0usize;
    while i + 4 < len {
        if &bytes[i..i + 3] == b".@\"" {
            let mut j = i + 3;
            let digits_start = j;
            while j < len && bytes[j].is_ascii_digit() {
                j += 1;
            }
            if j > digits_start && j < len && bytes[j] == b'"' {
                count += 1;
                i = j + 1;
                continue;
            }
        }
        i += 1;
    }
    count
}

fn count_anyopaque(txt: &str) -> usize {
    let bytes = txt.as_bytes();
    let len = bytes.len();
    let mut count = 0usize;
    let mut i = 0usize;
    while i < len {
        if bytes[i] == b'*' {
            if bytes[i..].starts_with(b"*anyopaque") {
                count += 1;
                i += 10;
                continue;
            }
            if bytes[i..].starts_with(b"*any!") {
                count += 1;
                i += 5;
                continue;
            }
        }
        i += 1;
    }
    count
}

fn is_zig_ast_error_line(line: &str) -> bool {
    let Some(zig_idx) = line.find(".zig:") else {
        return false;
    };
    let rest = &line[zig_idx + 5..];
    let mut parts = rest.splitn(3, ':');
    let Some(line_num) = parts.next() else {
        return false;
    };
    let Some(col_num) = parts.next() else {
        return false;
    };
    let Some(tail) = parts.next() else {
        return false;
    };
    !line_num.is_empty()
        && line_num.bytes().all(|b| b.is_ascii_digit())
        && !col_num.is_empty()
        && col_num.bytes().all(|b| b.is_ascii_digit())
        && tail.starts_with(" error")
}

/// In-process compact verifier mirroring `cpc::brief::run_brief` without writing to stdout.
fn tool_cpc_brief(cp_arg: &Path, max: usize) -> (String, bool) {
    let raw = cp_arg.to_string_lossy();
    let stem = raw.strip_suffix(".cp").unwrap_or(&raw);
    let cp = PathBuf::from(format!("{}.cp", stem));
    let zig = PathBuf::from(format!("{}.zig", stem));
    let bak = PathBuf::from(format!("{}_.zig", stem));
    let zig_bin = crate::brief::resolve_zig_bin(None);

    if !cp.exists() {
        return (format!("== {}: FAIL(not found)", cp.display()), true);
    }

    let code = match fs::read_to_string(&cp) {
        Ok(c) => c,
        Err(e) => {
            return (
                format!("== {}: FAIL(read error: {})", cp.display(), e),
                true,
            );
        }
    };
    let src_lines: Vec<&str> = code.lines().collect();
    let zig_existed = zig.exists();

    let mut parser = CPParser::new(&code);
    let (mut module, mut all_diags) = parser.parse_module_with_recovery();
    let mut analyzer = Analyzer::new();
    let _ = analyzer.analyze_module(&mut module);
    all_diags.extend(analyzer.get_diagnostics());
    diagnostic::mark_cascades(&mut all_diags, Some(&code));

    struct BriefErr {
        code: String,
        msg: String,
        line: usize,
        suggestion: String,
    }

    let mut errs: Vec<BriefErr> = Vec::new();
    let has_root = all_diags.iter().any(|d| d.cascade_of.is_none());
    for d in &all_diags {
        if has_root && d.cascade_of.is_some() {
            continue;
        }
        let dj = d.to_diagnostic_json(Some(&code), Some(&cp));
        let suggestion = dj
            .suggestion
            .filter(|s| !s.is_empty())
            .or_else(|| dj.help.filter(|s| !s.is_empty()))
            .unwrap_or_default();
        errs.push(BriefErr {
            code: dj.code,
            msg: dj.message,
            line: dj.line,
            suggestion,
        });
    }

    let mut status = "PASS";
    let mut ast_check_lines: Vec<String> = Vec::new();
    let mut ast_check_total = 0usize;

    if !errs.is_empty() {
        status = "FAIL(cpc)";
    } else {
        let cp_clone = cp.clone();
        let code_clone = code.clone();
        let cg_res = std::panic::catch_unwind(std::panic::AssertUnwindSafe(|| {
            let mut codegen = Codegen::new();
            codegen.is_beam_mode = false;
            codegen.is_lib_mode = true;
            codegen.is_fuzz_mode = false;
            codegen.fuzz_target_filter = None;
            codegen.cap_hub_prelude = None;
            codegen.source_dir = cp_clone.parent().map(|p| p.to_path_buf());
            codegen.init_source(Some(&cp_clone.display().to_string()), Some(&code_clone));
            codegen.generate_module(&module)
        }));

        match cg_res {
            Err(err) => {
                status = "FAIL(codegen)";
                let msg = if let Some(s) = err.downcast_ref::<&str>() {
                    (*s).to_string()
                } else if let Some(s) = err.downcast_ref::<String>() {
                    s.clone()
                } else {
                    "Internal compiler error".to_string()
                };
                errs.push(BriefErr {
                    code: "E_CODEGEN".to_string(),
                    msg,
                    line: 0,
                    suggestion: String::new(),
                });
            }
            Ok(zig_code) => {
                let mut need_write = true;
                if let Ok(existing) = fs::read_to_string(&zig) {
                    if existing == zig_code {
                        need_write = false;
                    }
                }
                if need_write {
                    if let Err(e) = fs::write(&zig, &zig_code) {
                        status = "FAIL(codegen)";
                        errs.push(BriefErr {
                            code: "E_IO".to_string(),
                            msg: format!("failed to write {}: {}", zig.display(), e),
                            line: 0,
                            suggestion: String::new(),
                        });
                    }
                }

                if status == "PASS" {
                    match Command::new(&zig_bin).arg("ast-check").arg(&zig).output() {
                        Err(e) => {
                            status = "FAIL(ast-check)";
                            ast_check_lines.push(format!(
                                "failed to execute '{}' ast-check: {}",
                                zig_bin.display(),
                                e
                            ));
                            ast_check_total = 1;
                        }
                        Ok(out) => {
                            if !out.status.success() {
                                status = "FAIL(ast-check)";
                                let combined = format!(
                                    "{}\n{}",
                                    String::from_utf8_lossy(&out.stderr),
                                    String::from_utf8_lossy(&out.stdout)
                                );
                                let matched: Vec<String> = combined
                                    .lines()
                                    .filter(|l| is_zig_ast_error_line(l))
                                    .map(|l| l.to_string())
                                    .collect();
                                if !matched.is_empty() {
                                    ast_check_total = matched.len();
                                    ast_check_lines = matched.into_iter().take(max).collect();
                                } else {
                                    let fallback: Vec<String> = combined
                                        .lines()
                                        .map(|l| l.trim())
                                        .filter(|l| !l.is_empty())
                                        .map(|l| l.to_string())
                                        .collect();
                                    ast_check_total = fallback.len();
                                    ast_check_lines = fallback.into_iter().take(max).collect();
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    let map_path = PathBuf::from(format!("{}.zig.map", stem));
    if map_path.exists() {
        let _ = fs::remove_file(&map_path);
    }
    if !zig_existed && zig.exists() {
        let _ = fs::remove_file(&zig);
    }

    let mut out = String::new();
    let _ = writeln!(out, "== {}: {}", cp.display(), status);

    if !ast_check_lines.is_empty() {
        for l in &ast_check_lines {
            let _ = writeln!(out, "    [ast-check] {}", truncate_chars(l, 180));
        }
        let _ = writeln!(out, "    total ast-check errors: {}", ast_check_total);
    }

    if !errs.is_empty() {
        let mut cnt: BTreeMap<&str, usize> = BTreeMap::new();
        for e in &errs {
            *cnt.entry(e.code.as_str()).or_insert(0) += 1;
        }
        let mut sorted_cnt: Vec<(&str, usize)> = cnt.into_iter().collect();
        sorted_cnt.sort_by(|a, b| b.1.cmp(&a.1).then_with(|| a.0.cmp(b.0)));
        let by_code: Vec<String> = sorted_cnt
            .into_iter()
            .map(|(k, v)| format!("{} x{}", k, v))
            .collect();
        let _ = writeln!(
            out,
            "   total={} by code: {}",
            errs.len(),
            by_code.join(", ")
        );

        let mut seen: HashSet<(String, String)> = HashSet::new();
        let mut shown = 0usize;
        for e in &errs {
            let key = (e.code.clone(), truncate_chars(&e.msg, 60).to_string());
            if !seen.insert(key) {
                continue;
            }
            let src_line = if e.line > 0 && e.line <= src_lines.len() {
                truncate_chars(src_lines[e.line - 1].trim(), 100)
            } else {
                ""
            };
            let _ = writeln!(
                out,
                "   {} L{}: {}",
                e.code,
                e.line,
                truncate_chars(&e.msg, 100)
            );
            if !src_line.is_empty() {
                let _ = writeln!(out, "        | {}", src_line);
            }
            if !e.suggestion.is_empty() {
                let _ = writeln!(
                    out,
                    "        -> suggestion: {}",
                    truncate_chars(&e.suggestion, 100)
                );
            }
            shown += 1;
            if shown >= max {
                break;
            }
        }
    }

    // 4-Gate static purity audit
    let backup = bak.exists();
    let zig_block = count_zig_blocks(&code);
    let var_decl = count_var_decls(&code);
    let tuple_at_n = count_tuple_at_n(&code);
    let anyopaque = count_anyopaque(&code);
    let (ratio, ratio_ok) = if backup {
        let a = code.lines().count();
        let b = fs::read_to_string(&bak)
            .map(|s| s.lines().count())
            .unwrap_or(0);
        if b > 0 {
            let r = a as f64 / b as f64;
            let rounded = (r * 1000.0).round() / 1000.0;
            (Some(rounded), Some(r <= 1.2))
        } else {
            (Some(0.0), Some(true))
        }
    } else {
        (None, None)
    };

    let mut bad = Vec::new();
    if zig_block > 0 {
        bad.push("zig{}");
    }
    if var_decl > 0 {
        bad.push("var");
    }
    if tuple_at_n > 0 {
        bad.push("tuple.@N");
    }
    if anyopaque > 0 {
        bad.push("anyopaque");
    }
    if ratio_ok == Some(false) {
        bad.push("ratio_ok");
    }

    let mut dict = format!(
        "{{'backup': {}, 'zig{{}}': {}, 'var': {}, 'tuple.@N': {}, 'anyopaque': {}",
        if backup { "True" } else { "False" },
        zig_block,
        var_decl,
        tuple_at_n,
        anyopaque
    );
    if let (Some(r), Some(ok)) = (ratio, ratio_ok) {
        dict.push_str(&format!(
            ", 'ratio': {}, 'ratio_ok': {}",
            r,
            if ok { "True" } else { "False" }
        ));
    }
    dict.push('}');

    let suffix = if bad.is_empty() {
        "  OK".to_string()
    } else {
        let items: Vec<String> = bad.iter().map(|k| format!("'{}'", k)).collect();
        format!("  VIOLATIONS=[{}]", items.join(", "))
    };
    let _ = write!(out, "   gates: {}{}", dict, suffix);

    (out, false)
}

fn tool_cpc_emit_types(path: &Path) -> (String, bool) {
    let code = match fs::read_to_string(path) {
        Ok(c) => c,
        Err(e) => {
            return (format!("Failed to read '{}': {}", path.display(), e), true);
        }
    };
    let mut parser = CPParser::new(&code);
    match parser.parse_module() {
        Ok(module) => {
            let mut emitter = DcpEmitter::new();
            let dcp = emitter.emit_module(&module);
            (dcp, false)
        }
        Err(e) => (format!("Parse error in '{}': {}", path.display(), e), true),
    }
}

fn locate_cp_lib_ref() -> Option<PathBuf> {
    let candidates = [
        PathBuf::from("/data/dev/cp/cp_lib_ref.md"),
        PathBuf::from("cp_lib_ref.md"),
        PathBuf::from("../cp_lib_ref.md"),
    ];
    candidates.into_iter().find(|p| p.is_file())
}

fn tool_cpc_search_stdlib(query: &str) -> (String, bool) {
    let trimmed = query.trim();
    if trimmed.is_empty() {
        return ("Query must not be empty.".to_string(), true);
    }

    let Some(ref_path) = locate_cp_lib_ref() else {
        return (
            "cp_lib_ref.md not found in /data/dev/cp/cp_lib_ref.md or current directory."
                .to_string(),
            true,
        );
    };

    let content = match fs::read_to_string(&ref_path) {
        Ok(c) => c,
        Err(e) => {
            return (
                format!("Failed to read '{}': {}", ref_path.display(), e),
                true,
            );
        }
    };

    let terms: Vec<String> = trimmed
        .split_whitespace()
        .map(|t| t.to_lowercase())
        .collect();
    let mut matches: Vec<String> = Vec::new();
    let mut current_heading = String::new();

    for (idx, line) in content.lines().enumerate() {
        let ltrim = line.trim();
        if ltrim.starts_with('#') {
            current_heading = ltrim.to_string();
        }
        if ltrim.is_empty() {
            continue;
        }
        let lower = line.to_lowercase();
        let matched = terms.iter().all(|t| lower.contains(t.as_str()));
        if matched {
            if !current_heading.is_empty() && !ltrim.starts_with('#') {
                matches.push(format!("L{} [{}]: {}", idx + 1, current_heading, ltrim));
            } else {
                matches.push(format!("L{}: {}", idx + 1, ltrim));
            }
            if matches.len() >= 10 {
                break;
            }
        }
    }

    if matches.is_empty() {
        (
            format!(
                "No matching entries found in {} for query '{}'.",
                ref_path.display(),
                trimmed
            ),
            false,
        )
    } else {
        (
            format!(
                "Found {} match(es) in {}:\n{}",
                matches.len(),
                ref_path.display(),
                matches.join("\n")
            ),
            false,
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::time::{SystemTime, UNIX_EPOCH};

    fn make_temp_cp(name: &str, content: &str) -> PathBuf {
        let nanos = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        let path = std::env::temp_dir().join(format!(
            "cpc_mcp_test_{}_{}_{}.cp",
            name,
            std::process::id(),
            nanos
        ));
        fs::write(&path, content).expect("write temp .cp file");
        path
    }

    #[test]
    fn test_initialize_and_notification() {
        let init_req = json!({
            "jsonrpc": "2.0",
            "id": 1,
            "method": "initialize",
            "params": {}
        })
        .to_string();

        let resp_str = handle_mcp_request_line(&init_req).expect("initialize must respond");
        let resp: Value = serde_json::from_str(&resp_str).unwrap();
        assert_eq!(resp["jsonrpc"], "2.0");
        assert_eq!(resp["id"], 1);
        assert_eq!(resp["result"]["protocolVersion"], "2025-03-26");
        assert_eq!(resp["result"]["serverInfo"]["name"], "cpc-mcp");
        assert_eq!(resp["result"]["serverInfo"]["version"], "1.0.0");
        assert!(resp["result"]["capabilities"]["tools"].is_object());

        let notif = json!({
            "jsonrpc": "2.0",
            "method": "notifications/initialized"
        })
        .to_string();
        assert!(handle_mcp_request_line(&notif).is_none());
    }

    #[test]
    fn test_tools_list() {
        let req = json!({
            "jsonrpc": "2.0",
            "id": 2,
            "method": "tools/list"
        })
        .to_string();

        let resp_str = handle_mcp_request_line(&req).expect("tools/list must respond");
        let resp: Value = serde_json::from_str(&resp_str).unwrap();
        let tools = resp["result"]["tools"].as_array().expect("tools array");
        assert_eq!(tools.len(), 4);
        let names: Vec<&str> = tools.iter().filter_map(|t| t["name"].as_str()).collect();
        assert!(names.contains(&"cpc_check"));
        assert!(names.contains(&"cpc_brief"));
        assert!(names.contains(&"cpc_emit_types"));
        assert!(names.contains(&"cpc_search_stdlib"));
    }

    #[test]
    fn test_tools_call_search_stdlib() {
        let req = json!({
            "jsonrpc": "2.0",
            "id": 3,
            "method": "tools/call",
            "params": {
                "name": "cpc_search_stdlib",
                "arguments": {
                    "query": "cap:"
                }
            }
        })
        .to_string();

        let resp_str = handle_mcp_request_line(&req).expect("tools/call must respond");
        let resp: Value = serde_json::from_str(&resp_str).unwrap();
        let content = resp["result"]["content"].as_array().expect("content array");
        assert_eq!(content.len(), 1);
        assert_eq!(content[0]["type"], "text");
        let text = content[0]["text"].as_str().unwrap();
        assert!(text.contains("Found"), "expected matches, got: {}", text);
        // Header + at most 10 matching lines
        assert!(text.lines().count() <= 11);
    }

    #[test]
    fn test_tools_call_cpc_check_and_emit_types() {
        let ok_path = make_temp_cp(
            "ok",
            "pub fn add(a: i32, b: i32): i32 {\n    return a + b;\n}\n",
        );

        let check_req = json!({
            "jsonrpc": "2.0",
            "id": 4,
            "method": "tools/call",
            "params": {
                "name": "cpc_check",
                "arguments": {
                    "path": ok_path.to_string_lossy()
                }
            }
        })
        .to_string();
        let resp_str = handle_mcp_request_line(&check_req).unwrap();
        let resp: Value = serde_json::from_str(&resp_str).unwrap();
        let text = resp["result"]["content"][0]["text"].as_str().unwrap();
        let diag_report: Value = serde_json::from_str(text).unwrap();
        assert_eq!(diag_report["status"], "ok");
        assert_eq!(diag_report["errors"], 0);

        let emit_req = json!({
            "jsonrpc": "2.0",
            "id": 5,
            "method": "tools/call",
            "params": {
                "name": "cpc_emit_types",
                "arguments": {
                    "path": ok_path.to_string_lossy()
                }
            }
        })
        .to_string();
        let emit_resp_str = handle_mcp_request_line(&emit_req).unwrap();
        let emit_resp: Value = serde_json::from_str(&emit_resp_str).unwrap();
        let dcp_text = emit_resp["result"]["content"][0]["text"].as_str().unwrap();
        assert!(dcp_text.contains("fn add"), "d.cp output: {}", dcp_text);

        let _ = fs::remove_file(&ok_path);

        // Error case for cpc_check
        let err_path = make_temp_cp("err", "pub fn broken(a: i32) {\n    let x = ;\n}\n");
        let err_req = json!({
            "jsonrpc": "2.0",
            "id": 6,
            "method": "tools/call",
            "params": {
                "name": "cpc_check",
                "arguments": {
                    "path": err_path.to_string_lossy()
                }
            }
        })
        .to_string();
        let err_resp_str = handle_mcp_request_line(&err_req).unwrap();
        let err_resp: Value = serde_json::from_str(&err_resp_str).unwrap();
        let err_text = err_resp["result"]["content"][0]["text"].as_str().unwrap();
        let err_report: Value = serde_json::from_str(err_text).unwrap();
        assert_eq!(err_report["status"], "error");
        assert!(err_report["errors"].as_u64().unwrap() >= 1);

        let _ = fs::remove_file(&err_path);
    }

    #[test]
    fn test_unknown_method_and_unknown_tool() {
        let unknown_method = json!({
            "jsonrpc": "2.0",
            "id": 7,
            "method": "nonexistent/method"
        })
        .to_string();
        let resp: Value =
            serde_json::from_str(&handle_mcp_request_line(&unknown_method).unwrap()).unwrap();
        assert_eq!(resp["error"]["code"], -32601);

        let unknown_tool = json!({
            "jsonrpc": "2.0",
            "id": 8,
            "method": "tools/call",
            "params": {
                "name": "does_not_exist",
                "arguments": {}
            }
        })
        .to_string();
        let tool_resp: Value =
            serde_json::from_str(&handle_mcp_request_line(&unknown_tool).unwrap()).unwrap();
        assert_eq!(tool_resp["error"]["code"], -32601);
    }
}
