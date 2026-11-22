// cap/test/testJsTsMetadata.cp
// Production-grade deep tests for cap:js tsMetadata.cp
// (TypeScript Decorator Metadata union/intersection algebra & type-arg lookahead)

import { test, assert, assertEq } from "cap:test";
import {
    TsMetadataError,
    TsMetadataKind,
    TsDecoratorMetadata,
    TsTypeArgLookaheadToken,
    canFollowTsTypeArguments,
} from "cap:js";

fn eqStr(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("TsDecoratorMetadata: foldUnion nullish stripping, heterogeneous fallback & identifier merge", () => {
    // string | null | undefined -> String
    const u1: [3]TsDecoratorMetadata = [
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_string),
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_null),
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_undefined),
    ];
    const r1 = TsDecoratorMetadata.foldUnion(u1[0..]);
    assert(r1.kind == TsMetadataKind.m_string);
    assert(eqStr(r1.runtimeRefStr(), "String"));

    // string | number -> Object
    const u2: [2]TsDecoratorMetadata = [
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_string),
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_number),
    ];
    const r2 = TsDecoratorMetadata.foldUnion(u2[0..]);
    assert(r2.kind == TsMetadataKind.m_object);

    // Same identifier preserved: User | undefined | User -> User
    const u3: [3]TsDecoratorMetadata = [
        TsDecoratorMetadata.identifier("User"),
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_undefined),
        TsDecoratorMetadata.identifier("User"),
    ];
    const r3 = TsDecoratorMetadata.foldUnion(u3[0..]);
    assert(r3.eql(TsDecoratorMetadata.identifier("User")));

    // Distinct identifiers degrade to Object: User | Admin -> Object
    const u4: [2]TsDecoratorMetadata = [
        TsDecoratorMetadata.identifier("User"),
        TsDecoratorMetadata.identifier("Admin"),
    ];
    assert(TsDecoratorMetadata.foldUnion(u4[0..]).kind == TsMetadataKind.m_object);

    // Ident "Object" short-circuits to m_object
    const u5: [2]TsDecoratorMetadata = [
        TsDecoratorMetadata.identifier("Object"),
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_string),
    ];
    assert(TsDecoratorMetadata.foldUnion(u5[0..]).kind == TsMetadataKind.m_object);
});

test("TsDecoratorMetadata: foldIntersection never dominance & unknown-only serialization", () => {
    // never & string -> never (serializes to void 0)
    const i1: [2]TsDecoratorMetadata = [
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_string),
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_never),
    ];
    const r1 = TsDecoratorMetadata.foldIntersection(i1[0..]);
    assert(r1.kind == TsMetadataKind.m_never);
    assert(eqStr(r1.runtimeRefStr(), "void 0"));

    // unknown & unknown -> undefined (void 0), NOT Object
    const i2: [2]TsDecoratorMetadata = [
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_unknown),
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_unknown),
    ];
    const r2 = TsDecoratorMetadata.foldIntersection(i2[0..]);
    assert(r2.kind == TsMetadataKind.m_undefined);
    assert(eqStr(r2.runtimeRefStr(), "void 0"));

    // string & unknown -> String
    const i3: [2]TsDecoratorMetadata = [
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_string),
        TsDecoratorMetadata.fromKind(TsMetadataKind.m_unknown),
    ];
    const r3 = TsDecoratorMetadata.foldIntersection(i3[0..]);
    assert(r3.kind == TsMetadataKind.m_string);

    // Distinct classes -> Object
    const i4: [2]TsDecoratorMetadata = [
        TsDecoratorMetadata.identifier("Readable"),
        TsDecoratorMetadata.identifier("Writable"),
    ];
    assert(TsDecoratorMetadata.foldIntersection(i4[0..]).kind == TsMetadataKind.m_object);
});

test("TsDecoratorMetadata: runtimeRefStr & emitRuntimeRef across built-in constructors", () => {
    let buf: [32]u8 = [0; 32];

    const p = TsDecoratorMetadata.fromKind(TsMetadataKind.m_promise);
    const n1 = p.emitRuntimeRef(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n1], "Promise"));

    const arr = TsDecoratorMetadata.fromKind(TsMetadataKind.m_array);
    const n2 = arr.emitRuntimeRef(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n2], "Array"));

    const bi = TsDecoratorMetadata.fromKind(TsMetadataKind.m_bigint);
    const n3 = bi.emitRuntimeRef(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n3], "BigInt"));

    const cls = TsDecoratorMetadata.identifier("HttpService");
    const n4 = cls.emitRuntimeRef(buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n4], "HttpService"));
});

test("canFollowTsTypeArguments: expression generic call vs comparison disambiguation", () => {
    // Legal continuations for foo<T>(...) / foo<T>`...`
    assert(canFollowTsTypeArguments(TsTypeArgLookaheadToken.open_paren, false));
    assert(canFollowTsTypeArguments(TsTypeArgLookaheadToken.no_sub_template, false));
    assert(canFollowTsTypeArguments(TsTypeArgLookaheadToken.template_head, false));

    // Ambiguous comparison / unary operators reject type argument list
    assert(!canFollowTsTypeArguments(TsTypeArgLookaheadToken.less_than, false));
    assert(!canFollowTsTypeArguments(TsTypeArgLookaheadToken.greater_than, false));
    assert(!canFollowTsTypeArguments(TsTypeArgLookaheadToken.plus, false));
    assert(!canFollowTsTypeArguments(TsTypeArgLookaheadToken.minus, false));
    assert(!canFollowTsTypeArguments(TsTypeArgLookaheadToken.greater_than_eq, false));
    assert(!canFollowTsTypeArguments(TsTypeArgLookaheadToken.shift_right, false));

    // expr_start depends on line break; binary_op & non_expr_punct accept
    assert(!canFollowTsTypeArguments(TsTypeArgLookaheadToken.expr_start, false));
    assert(canFollowTsTypeArguments(TsTypeArgLookaheadToken.expr_start, true));
    assert(canFollowTsTypeArguments(TsTypeArgLookaheadToken.binary_op, false));
    assert(canFollowTsTypeArguments(TsTypeArgLookaheadToken.non_expr_punct, false));
});

test("TsMetadataError: BufferTooSmall guard on emitRuntimeRef", () => {
    let tiny: [3]u8 = [0; 3];
    let buf_err = false;
    const m = TsDecoratorMetadata.fromKind(TsMetadataKind.m_function);
    const r = m.emitRuntimeRef(tiny[0..]) catch |e| {
        if (e == TsMetadataError.BufferTooSmall) buf_err = true;
        @as(usize, 0)
    };
    * = r;
    assert(buf_err);
    assertEq(m.runtimeRefStr().len, @as(usize, 8));
});
