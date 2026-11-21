use cpc::codegen::Codegen;
use cpc::parser::Parser;
use cpc::sema::Analyzer;

#[test]
fn unpack_prelude_does_not_shadow_root_struct_decls() {
    let src = r#"
@root struct S {
    x: i32,
    pub fn end() {}
    pub fn fields() {}
    pub fn names() {}
    pub fn types() {}
    pub fn count() {}
    pub fn buf() {}
    pub fn s() {}
    pub fn v() {}
    pub fn idx() {}
    pub fn ti() {}
    pub fn n() {}
}
fn pair(): [i32, i32] { return .{ 1, 2 }; }
export fn run(): i32 {
    const [a, b] = pair();
    return a + b;
}
"#;
    let mut parser = Parser::new(src);
    let (mut module, parse_diags) = parser.parse_module_with_recovery();
    assert!(parse_diags.is_empty(), "parse diags: {:?}", parse_diags);

    let mut analyzer = Analyzer::new();
    let res = analyzer.analyze_module(&mut module);
    let sema_diags = analyzer.get_diagnostics();
    assert!(
        res.is_ok() && sema_diags.is_empty(),
        "sema error: res={:?}, diags={:?}",
        res,
        sema_diags
    );

    let mut codegen = Codegen::new();
    codegen.is_lib_mode = true;
    let zig = codegen.generate_module(&module);

    assert!(
        zig.contains("const __cp_end ="),
        "expected __cp_end in:\n{}",
        zig
    );
    assert!(
        !zig.contains("const end ="),
        "unprefixed 'const end =' still present in:\n{}",
        zig
    );
    assert!(
        !zig.contains("const fields ="),
        "unprefixed 'const fields =' still present in:\n{}",
        zig
    );
    assert!(
        !zig.contains("const buf ="),
        "unprefixed 'const buf =' still present in:\n{}",
        zig
    );
    assert!(
        zig.contains("@hasField(@TypeOf(__cp_s), \"fields\")"),
        "expected Zig 0.15/0.17 dual-compat @hasField check"
    );
}
