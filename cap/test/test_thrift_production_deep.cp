// Source: cp/cap/test/test_thrift_production_deep.cp (Production deep tests for cap:thrift)

import { test, expect_eq } from "cap:test";
import {
    ThriftDeclKind,
    ThriftDecl,
    ThriftField,
    ThriftDeclIter,
    ThriftFieldIter,
    validateThriftBraces,
} from "cap:binfmt";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("1. ThriftDeclIter: namespace, include, typedef, const, enum, struct, exception & service extends") {
    const idl =
        "\xEF\xBB\xBF# Thrift IDL header\n" ++
        "include \"shared.thrift\"\n" ++
        "namespace go com.example.rpc\n" ++
        "// type alias\n" ++
        "typedef map<string, list<i64>> IdMap\n" ++
        "const i32 MAX_ITEMS = 100;\n" ++
        "/* status enum */\n" ++
        "enum Status { OK = 0, ERR = 1 }\n" ++
        "struct User { 1: required i64 id, 2: optional string name }\n" ++
        "exception ApiError { 1: i32 code, 2: string message }\n" ++
        "service UserService extends shared.BaseService {\n" ++
        "  User getUser(1: i64 id) throws (1: ApiError err)\n" ++
        "}\n";

    expect_eq(validateThriftBraces(idl), true);

    let it = ThriftDeclIter.init(idl);

    const d0 = it.next().?;
    expect_eq(d0.kind == .include_decl, true);
    expect_eq(strEq(d0.name, "shared.thrift"), true);

    const d1 = it.next().?;
    expect_eq(d1.kind == .namespace_decl, true);
    expect_eq(strEq(d1.name, "com.example.rpc"), true);

    const d2 = it.next().?;
    expect_eq(d2.kind == .typedef_decl, true);
    expect_eq(strEq(d2.name, "IdMap"), true);
    expect_eq(strEq(d2.body.?, "map<string, list<i64>>"), true);

    const d3 = it.next().?;
    expect_eq(d3.kind == .const_decl, true);
    expect_eq(strEq(d3.name, "MAX_ITEMS"), true);

    const d4 = it.next().?;
    expect_eq(d4.kind == .enum_decl, true);
    expect_eq(strEq(d4.name, "Status"), true);

    const d5 = it.next().?;
    expect_eq(d5.kind == .struct_decl, true);
    expect_eq(strEq(d5.name, "User"), true);
    expect_eq(d5.body != null, true);

    const d6 = it.next().?;
    expect_eq(d6.kind == .exception_decl, true);
    expect_eq(strEq(d6.name, "ApiError"), true);

    const d7 = it.next().?;
    expect_eq(d7.kind == .service_decl, true);
    expect_eq(strEq(d7.name, "UserService"), true);
    expect_eq(strEq(d7.extends_name.?, "shared.BaseService"), true);
    expect_eq(d7.body != null, true);

    expect_eq(it.next() == null, true);
}

test("2. ThriftFieldIter: required/optional qualifiers, nested generics, defaults & annotations") {
    const body =
        "  // primary key\n" ++
        "  1: required string name = \"anon\" (go.tag = \"json:\\\"name\\\"\"),\n" ++
        "  2: optional list<i32> tags = [1, 2, 3];\n" ++
        "  3: map<string, list<i64>> matrix,\n" ++
        "  # implicit field id\n" ++
        "  common.Meta meta\n";

    let it = ThriftFieldIter.init(body);

    const f0 = it.next().?;
    expect_eq(f0.field_id.?, @as(i32, 1));
    expect_eq(f0.is_required, true);
    expect_eq(f0.is_optional, false);
    expect_eq(strEq(f0.ty, "string"), true);
    expect_eq(strEq(f0.name, "name"), true);

    const f1 = it.next().?;
    expect_eq(f1.field_id.?, @as(i32, 2));
    expect_eq(f1.is_required, false);
    expect_eq(f1.is_optional, true);
    expect_eq(strEq(f1.ty, "list<i32>"), true);
    expect_eq(strEq(f1.name, "tags"), true);

    const f2 = it.next().?;
    expect_eq(f2.field_id.?, @as(i32, 3));
    expect_eq(f2.is_required, false);
    expect_eq(f2.is_optional, false);
    expect_eq(strEq(f2.ty, "map<string, list<i64>>"), true);
    expect_eq(strEq(f2.name, "matrix"), true);

    const f3 = it.next().?;
    expect_eq(f3.field_id == null, true);
    expect_eq(strEq(f3.ty, "common.Meta"), true);
    expect_eq(strEq(f3.name, "meta"), true);

    expect_eq(it.next() == null, true);
}

test("3. Union declaration and service without extends") {
    const idl =
        "union Payload {\n" ++
        "  1: string text,\n" ++
        "  2: binary raw,\n" ++
        "}\n" ++
        "service Ping { void ping() }\n";

    expect_eq(validateThriftBraces(idl), true);

    let it = ThriftDeclIter.init(idl);
    const u = it.next().?;
    expect_eq(u.kind == .union_decl, true);
    expect_eq(strEq(u.name, "Payload"), true);

    let fit = ThriftFieldIter.init(u.body.?);
    expect_eq(strEq(fit.next().?.name, "text"), true);
    expect_eq(strEq(fit.next().?.name, "raw"), true);
    expect_eq(fit.next() == null, true);

    const svc = it.next().?;
    expect_eq(svc.kind == .service_decl, true);
    expect_eq(strEq(svc.name, "Ping"), true);
    expect_eq(svc.extends_name == null, true);
    expect_eq(it.next() == null, true);
}

test("4. validateThriftBraces ignores brackets inside strings and all 3 comment styles") {
    const idl =
        "# unbalanced { ( < in hash comment\n" ++
        "// unbalanced } ) > in line comment\n" ++
        "/* unbalanced { [ < in block comment */\n" ++
        "const string S = \"{[(< unbalanced in double string\";\n" ++
        "const string T = '}])> unbalanced in single string';\n" ++
        "struct Valid { 1: map<string, list<i32>> m }\n";

    expect_eq(validateThriftBraces(idl), true);
    expect_eq(validateThriftBraces(""), true);
}

test("5. Malformed Thrift rejection: mismatched/unclosed braces, comments, strings & deep nesting") {
    expect_eq(validateThriftBraces("struct A { 1: i32 x"), false);
    expect_eq(validateThriftBraces("struct A { 1: i32 x }}"), false);
    expect_eq(validateThriftBraces("struct A { 1: list<i32 x }"), false);
    expect_eq(validateThriftBraces("struct A { 1: map<string, i32) x }"), false);
    expect_eq(validateThriftBraces("/* unclosed comment"), false);
    expect_eq(validateThriftBraces("const string S = \"unterminated"), false);
    expect_eq(validateThriftBraces("const string S = 'unterminated"), false);

    // Deep nesting guard (> 64)
    let deep: [140]u8 = [0; 140];
    let i: usize = 0;
    while (i < 70) : (i += 1) {
        deep[i] = '{';
        deep[139 - i] = '}';
    }
    expect_eq(validateThriftBraces(deep[0..]), false);
}
