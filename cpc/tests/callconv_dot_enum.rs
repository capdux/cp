use cpc::codegen::Codegen;
use cpc::parser::Parser;
use cpc::sema::Analyzer;

fn compile_lib(src: &str) -> String {
    let mut parser = Parser::new(src);
    let (mut module, parse_diags) = parser.parse_module_with_recovery();
    assert!(
        parse_diags.is_empty(),
        "unexpected parse diagnostics: {:?}",
        parse_diags
    );

    let mut analyzer = Analyzer::new();
    let res = analyzer.analyze_module(&mut module);
    let sema_diags = analyzer.get_diagnostics();
    assert!(
        res.is_ok() && sema_diags.is_empty(),
        "unexpected sema error: res={:?}, diags={:?}",
        res,
        sema_diags
    );

    let mut codegen = Codegen::new();
    codegen.is_lib_mode = true;
    codegen.generate_module(&module)
}

#[test]
fn root_struct_method_callconv_dot_enum_literals() {
    let src = r#"
@root struct Foo {
    pub fn create() callconv(.c): i32 { return 0; }
    @callconv(.c)
    pub fn create_meta(): i32 { return 1; }
    pub fn helper() callconv(.auto): i32 { return 2; }
    pub fn bare() callconv(.naked) {}
}
"#;
    let zig = compile_lib(src);
    assert!(
        zig.contains("pub fn create() callconv(.c) i32"),
        "missing callconv(.c) on create:\n{}",
        zig
    );
    assert!(
        zig.contains("pub fn create_meta() callconv(.c) i32"),
        "missing callconv(.c) on create_meta:\n{}",
        zig
    );
    assert!(
        zig.contains("pub fn helper() callconv(.auto) i32"),
        "missing callconv(.auto) on helper:\n{}",
        zig
    );
    assert!(
        zig.contains("pub fn bare() callconv(.naked) void"),
        "missing callconv(.naked) on bare:\n{}",
        zig
    );
}

#[test]
fn top_level_fn_callconv_dot_enum() {
    let src = r#"
pub fn top_c() callconv(.c): i32 { return 42; }
@callconv(.auto)
pub fn top_auto(): i32 { return 7; }
"#;
    let zig = compile_lib(src);
    assert!(
        zig.contains("fn top_c() callconv(.c) i32"),
        "missing callconv(.c) on top_c:\n{}",
        zig
    );
    assert!(
        zig.contains("fn top_auto() callconv(.auto) i32"),
        "missing callconv(.auto) on top_auto:\n{}",
        zig
    );
}
