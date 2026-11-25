// Item #4: CPC Compiler Golden Regression & Deterministic Parser/Sema Fuzz Suite
// Guarantees zero-regression across core CP language constructs and zero-panic resilience
// on malformed / boundary inputs.

use cpc::codegen::Codegen;
use cpc::parser::Parser;
use cpc::sema::Analyzer;

fn compile_snippet(src: &str, lib_mode: bool) -> Result<String, String> {
    let mut parser = Parser::new(src);
    let (mut module, parse_diags) = parser.parse_module_with_recovery();
    if !parse_diags.is_empty() {
        return Err(format!("parse error: {:?}", parse_diags));
    }
    let mut analyzer = Analyzer::new();
    let res = analyzer.analyze_module(&mut module);
    let sema_diags = analyzer.get_diagnostics();
    if res.is_err() || !sema_diags.is_empty() {
        return Err(format!("sema error: res={:?}, diags={:?}", res, sema_diags));
    }
    let mut cg = Codegen::new();
    cg.is_lib_mode = lib_mode;
    Ok(cg.generate_module(&module))
}

#[test]
fn test_golden_null_coalesce_short_circuit_codegen() {
    let src = r#"
fn getOpt(): i32? { return 10; }
fn main() {
    const a: i32 = getOpt() ?? @panic("boom");
    const b: i32 = getOpt() ?? 42;
    * = a;
    * = b;
}
"#;
    let zig = compile_snippet(src, false).expect("should compile");
    assert!(
        zig.contains("orelse (@panic(\"boom\"))"),
        "expected native orelse for @panic RHS, got:\n{}",
        zig
    );
    assert!(
        zig.contains("comptime @typeInfo(@TypeOf(__nc_l_"),
        "expected comptime short-circuit block for ?? operator, got:\n{}",
        zig
    );
}

#[test]
fn test_golden_optional_slice_return_type() {
    let src = r#"
pub fn maybeSlice(ok: bool, buf: []u8): ([]u8)? {
    if (!ok) return null;
    return buf;
}
"#;
    let zig = compile_snippet(src, true).expect("should compile");
    assert!(
        zig.contains("?[]u8") || zig.contains("?[]const u8"),
        "expected optional slice return type in Zig output, got:\n{}",
        zig
    );
}

#[test]
fn test_golden_export_alias_reexport() {
    let src = r#"
pub struct OrigType { x: i32 = 0; }
export { OrigType as AliasedType };
"#;
    let zig = compile_snippet(src, true).expect("should compile");
    assert!(
        zig.contains("OrigType"),
        "expected OrigType in output:\n{}",
        zig
    );
}

#[test]
fn test_golden_struct_extern_fn_static_no_self() {
    let src = r#"
pub struct NativeApi {
    extern fn c_ping(x: i32): i32;
}
"#;
    let zig = compile_snippet(src, true).expect("should compile");
    assert!(
        zig.contains("extern \"c\" fn c_ping(x: i32) callconv(.c) i32;")
            || zig.contains("extern fn c_ping(x: i32)"),
        "expected static extern fn without self injection, got:\n{}",
        zig
    );
}

#[test]
fn test_golden_reserved_dec_diagnostic_code() {
    let src = "fn main() { const x = dec[0]; }";
    let mut parser = Parser::new(src);
    let (_module, parse_diags) = parser.parse_module_with_recovery();
    assert!(
        parse_diags.iter().any(|d| d.code == "E0131"),
        "expected E0131 diagnostic for reserved 'dec', got {:?}",
        parse_diags
    );
}

#[test]
fn test_deterministic_parser_and_sema_fuzz_zero_panic() {
    let handle = std::thread::Builder::new()
        .stack_size(16 * 1024 * 1024)
        .spawn(|| {
            let fragments = [
                "fn ", "pub ", "struct ", "enum ", "let ", "const ", "if (", "while (", "for (",
                "return ", "match ", "switch (", "?? ", "=> ", ":= ", "*_", "[]u8?", "([]u8)?",
                "{", "}", "(", ")", "[", "]", ";", ":", ",", ".", "..", "@c ", "@panic(", "null",
                "undefined", "123", "\"str\"", "// comment\n", "extern fn f(a: i32): i32;",
                "export { A as B } from \"./x.cp\";", "import { X } from \"cap:web\";",
            ];
            let mut state: u64 = 0x9E3779B97F4A7C15;
            for _iter in 0..300 {
                let mut src = String::with_capacity(256);
                state ^= state << 13;
                state ^= state >> 7;
                state ^= state << 17;
                let len = (state % 24) as usize + 1;
                for _ in 0..len {
                    state ^= state << 13;
                    state ^= state >> 7;
                    state ^= state << 17;
                    let idx = (state as usize) % fragments.len();
                    src.push_str(fragments[idx]);
                }
                let mut parser = Parser::new(&src);
                let (mut module, _diags) = parser.parse_module_with_recovery();
                let mut analyzer = Analyzer::new();
                let _ = analyzer.analyze_module(&mut module);
            }
        })
        .expect("spawn fuzz thread");
    handle.join().expect("fuzz thread panicked");
}
