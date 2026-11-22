import { test, assert, assertEq } from "cap:test";
import {
    JS_MEMBER_FLAG_COMPUTED,
    JS_MEMBER_FLAG_PRIVATE,
    JS_MEMBER_FLAG_DECORATED,
    JS_MEMBER_FLAG_STATIC,
    JS_FN_FLAG_ASYNC,
    JS_FN_FLAG_GENERATOR,
    JS_FN_FLAG_ARROW,
    JsClassAstError,
    JsPropertyKind,
    JsExprPurity,
    JsClassMoveBlocker,
    JsClassMember,
    JsClassDesc,
    JsFnParamDesc,
    JsFnSignature,
} from "cap:js";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

test("JsClassDesc.canBeMoved allows pure classes with methods and pure static fields", () => {
    const members: [3]JsClassMember = [
        JsClassMember.method("constructor", false),
        JsClassMember.method("render", false),
        JsClassMember.field("TAG", true, JsExprPurity.pure_movable),
    ];
    const cls = JsClassDesc{
        name: "Button",
        members: members[0..],
    };
    assert(cls.canBeMoved());
    assert(cls.moveBlocker() == JsClassMoveBlocker.none);
    assertEq(cls.countStaticFields(), 1);
    assertEq(cls.countInstanceMembers(), 2);
    assert(!cls.hasStaticBlock());
    assert(!cls.hasPrivateMembers());
    assert(!cls.hasAutoAccessors());

    // Static fields need lowering for ES2020, native in ES2022
    assert(cls.needsLowering(2020));
    assert(!cls.needsLowering(2022));
});

test("JsClassDesc.moveBlocker detects all Bun Class.canBeMoved blockers", () => {
    const empty: [0]JsClassMember = [];

    // 1. extends
    const sub = JsClassDesc{ name: "Sub", has_extends: true, members: empty[0..] };
    assert(!sub.canBeMoved());
    assert(sub.moveBlocker() == JsClassMoveBlocker.has_extends);

    // 2. class-level decorator
    const dec_cls = JsClassDesc{ name: "Dec", has_decorators: true, members: empty[0..] };
    assert(dec_cls.moveBlocker() == JsClassMoveBlocker.has_decorators);

    // 3. static {} block
    const m_sb: [1]JsClassMember = [JsClassMember.staticBlock()];
    const sb = JsClassDesc{ name: "SB", members: m_sb[0..] };
    assert(sb.hasStaticBlock());
    assert(sb.moveBlocker() == JsClassMoveBlocker.has_static_block);

    // 4. computed property key
    const m_comp: [1]JsClassMember = [
        JsClassMember{ name: "sym", kind: JsPropertyKind.normal, flags: JS_MEMBER_FLAG_COMPUTED },
    ];
    const comp = JsClassDesc{ name: "Comp", members: m_comp[0..] };
    assert(comp.moveBlocker() == JsClassMoveBlocker.has_computed_key);

    // 5. impure static field initializer
    const m_impure: [1]JsClassMember = [
        JsClassMember.field("ID", true, JsExprPurity.side_effectful),
    ];
    const imp = JsClassDesc{ name: "Imp", members: m_impure[0..] };
    assert(imp.moveBlocker() == JsClassMoveBlocker.impure_static_field);

    // 6. member-level decorator
    const m_dec: [1]JsClassMember = [
        JsClassMember{ name: "x", kind: JsPropertyKind.normal, flags: JS_MEMBER_FLAG_DECORATED },
    ];
    const mdec = JsClassDesc{ name: "MDec", members: m_dec[0..] };
    assert(mdec.moveBlocker() == JsClassMoveBlocker.has_decorators);
    assert(strEql(mdec.moveBlocker().asStr(), "has_decorators"));
});

test("JsClassDesc lowering checks for private fields and auto-accessors", () => {
    const m_priv: [1]JsClassMember = [
        JsClassMember{ name: "#secret", kind: JsPropertyKind.normal, flags: JS_MEMBER_FLAG_PRIVATE | JS_MEMBER_FLAG_STATIC, init_purity: JsExprPurity.pure_movable },
    ];
    const c_priv = JsClassDesc{ name: "Vault", members: m_priv[0..] };
    assert(c_priv.hasPrivateMembers());
    assert(c_priv.canBeMoved());
    assert(c_priv.needsLowering(2020));
    assert(!c_priv.needsLowering(2022));

    const m_acc: [1]JsClassMember = [
        JsClassMember.autoAccessor("count", false, JsExprPurity.pure_movable),
    ];
    const c_acc = JsClassDesc{ name: "Counter", members: m_acc[0..] };
    assert(c_acc.hasAutoAccessors());
    assert(c_acc.needsLowering(2024));
    assert(strEql(JsPropertyKind.auto_accessor.asStr(), "auto_accessor"));
});

test("JsFnSignature minArity, maxArity, tsCtorFields, and formatHeader", () => {
    let buf: [128]u8 = [0; 128];

    // async function* stream(url, opts = {}, ...extra)
    const p1: [3]JsFnParamDesc = [
        JsFnParamDesc.simple("url"),
        JsFnParamDesc.withDefault("opts"),
        JsFnParamDesc.rest("extra"),
    ];
    const sig1 = JsFnSignature{
        name: "stream",
        flags: JS_FN_FLAG_ASYNC | JS_FN_FLAG_GENERATOR,
        params: p1[0..],
    };
    assertEq(sig1.minArity(), 1);
    assert(sig1.maxArity() == null);
    const n1 = sig1.formatHeader(buf[0..]) catch unreachable;
    assert(strEql(buf[0..n1], "async function* stream(url, opts, ...extra)"));

    // Arrow function: async (a, b) =>
    const p2: [2]JsFnParamDesc = [
        JsFnParamDesc.simple("a"),
        JsFnParamDesc.simple("b"),
    ];
    const sig2 = JsFnSignature{
        name: "",
        flags: JS_FN_FLAG_ASYNC | JS_FN_FLAG_ARROW,
        params: p2[0..],
    };
    assertEq(sig2.minArity(), 2);
    if (ma := sig2.maxArity()) {
        assertEq(ma, 2);
    } else {
        assert(false);
    }
    const n2 = sig2.formatHeader(buf[0..]) catch unreachable;
    assert(strEql(buf[0..n2], "async (a, b) =>"));

    // TypeScript constructor parameter properties
    const p3: [2]JsFnParamDesc = [
        JsFnParamDesc.tsCtorField("id", false),
        JsFnParamDesc.tsCtorField("role", true),
    ];
    const ctor = JsFnSignature{ name: "constructor", params: p3[0..] };
    assertEq(ctor.countTsCtorFields(), 2);
    assertEq(ctor.minArity(), 1);
});

test("JsFnSignature validation rejects invalid rest params and tiny buffer", () => {
    // Rest not in last position
    const bad1: [2]JsFnParamDesc = [
        JsFnParamDesc.rest("args"),
        JsFnParamDesc.simple("tail"),
    ];
    const s_bad1 = JsFnSignature{ name: "f", params: bad1[0..] };
    let rest_err1 = false;
    * = s_bad1.validate() catch |e| { rest_err1 = (e == JsClassAstError.InvalidRestParam); };
    assert(rest_err1);

    // Rest with default
    const bad2: [1]JsFnParamDesc = [
        JsFnParamDesc{ name: "args", has_default: true, is_rest: true },
    ];
    const s_bad2 = JsFnSignature{ name: "g", params: bad2[0..] };
    let rest_err2 = false;
    * = s_bad2.validate() catch |e| { rest_err2 = (e == JsClassAstError.InvalidRestParam); };
    assert(rest_err2);

    // BufferTooSmall
    const ok_p: [1]JsFnParamDesc = [JsFnParamDesc.simple("x")];
    const ok_sig = JsFnSignature{ name: "longFunctionName", params: ok_p[0..] };
    let tiny: [5]u8 = [0; 5];
    let buf_err = false;
    * = ok_sig.formatHeader(tiny[0..]) catch |e| { buf_err = (e == JsClassAstError.BufferTooSmall); };
    assert(buf_err);
});
