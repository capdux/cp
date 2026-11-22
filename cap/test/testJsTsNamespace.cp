// cap/test/testJsTsNamespace.cp
// Production-grade deep tests for cap:js tsNamespace.cp
// (Zero-alloc TypeScript namespace/enum sibling merge & implicit access proxy)

import { test, assert, assertEq } from "cap:test";
import {
    TS_NS_MAX_SCOPES,
    TS_NS_MAX_MEMBERS,
    TS_NS_MAX_PROXIES,
    TsNamespaceError,
    TsNamespaceMemberKind,
    TsNamespaceRegistry,
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

test("TsNamespaceMemberKind: isEnum predicate across all 5 variants", () => {
    assert(!TsNamespaceMemberKind.property.isEnum());
    assert(!TsNamespaceMemberKind.nested_namespace.isEnum());
    assert(TsNamespaceMemberKind.enum_number.isEnum());
    assert(TsNamespaceMemberKind.enum_string.isEnum());
    assert(TsNamespaceMemberKind.enum_property.isEnum());
});

test("TsNamespaceRegistry: sibling namespace blocks share exports & nested namespaces", () => {
    let reg = TsNamespaceRegistry.init();
    const root_gid = reg.createGroup();
    const child_gid = reg.createGroup();

    const blk1 = reg.openBlock(root_gid, 100, "ns2", false) catch unreachable;
    const blk2 = reg.openBlock(root_gid, 101, "ns3", false) catch unreachable;

    assert(reg.exportProperty(root_gid, "y", 10) catch unreachable);
    assert(reg.exportNestedNamespace(root_gid, "Inner", child_gid, 25) catch unreachable);
    // Re-exporting same name updates in place (returns false, memberCount unchanged)
    assert(!(reg.exportProperty(root_gid, "y", 40) catch unreachable));
    assertEq(reg.memberCount(), @as(usize, 2));

    // blk2 implicitly sees `y` exported by sibling blk1
    if (m := reg.lookupImplicitInBlock(blk2, "y")) {
        assert(m.kind == TsNamespaceMemberKind.property);
        assertEq(m.loc_start, @as(u32, 40));
    } else {
        assert(false);
    }

    if (nested := reg.lookupImplicitInBlock(blk1, "Inner")) {
        assert(nested.kind == TsNamespaceMemberKind.nested_namespace);
        assertEq(nested.child_group_id, child_gid);
    } else {
        assert(false);
    }
});

test("TsNamespaceRegistry: sibling enum blocks share numeric, string & computed members", () => {
    let reg = TsNamespaceRegistry.init();
    const enum_gid = reg.createGroup();
    const e1 = reg.openBlock(enum_gid, 200, "Foo2", true) catch unreachable;
    const e2 = reg.openBlock(enum_gid, 201, "Foo3", true) catch unreachable;
    * = e1;

    * = reg.exportEnumNumber(enum_gid, "A", 3.0, 1) catch unreachable;
    * = reg.exportEnumString(enum_gid, "Tag", "hello", 12) catch unreachable;
    * = reg.exportEnumProperty(enum_gid, "Dyn", 24) catch unreachable;

    if (ma := reg.lookupImplicitInBlock(e2, "A")) {
        assert(ma.kind == TsNamespaceMemberKind.enum_number);
        assert(ma.num_value == 3.0);
    } else {
        assert(false);
    }
    if (mt := reg.lookupImplicitInBlock(e2, "Tag")) {
        assert(mt.kind == TsNamespaceMemberKind.enum_string);
        assert(eqStr(mt.str_value, "hello"));
    } else {
        assert(false);
    }
    if (md := reg.lookupImplicitInBlock(e2, "Dyn")) {
        assert(md.kind == TsNamespaceMemberKind.enum_property);
    } else {
        assert(false);
    }
});

test("TsNamespaceRegistry: enum/namespace implicit lookup isolation rule", () => {
    let reg = TsNamespaceRegistry.init();
    const shared_gid = reg.createGroup();
    const enum_blk = reg.openBlock(shared_gid, 300, "a2", true) catch unreachable;
    const ns_blk = reg.openBlock(shared_gid, 301, "a3", false) catch unreachable;

    * = reg.exportEnumNumber(shared_gid, "enumVal", 1.0, 5) catch unreachable;
    * = reg.exportProperty(shared_gid, "nsVal", 15) catch unreachable;

    // enum block sees enumVal, NOT nsVal
    assert(reg.lookupImplicitInBlock(enum_blk, "enumVal") != null);
    assert(reg.lookupImplicitInBlock(enum_blk, "nsVal") == null);

    // namespace block sees nsVal, NOT enumVal
    assert(reg.lookupImplicitInBlock(ns_blk, "nsVal") != null);
    assert(reg.lookupImplicitInBlock(ns_blk, "enumVal") == null);
});

test("TsNamespaceRegistry: per-block lazy property proxy caching & formatPropertyAccess", () => {
    let reg = TsNamespaceRegistry.init();
    const gid = reg.createGroup();
    const b1 = reg.openBlock(gid, 10, "x2", false) catch unreachable;
    const b2 = reg.openBlock(gid, 11, "x3", false) catch unreachable;

    // Same block returns cached proxy ref without incrementing proxyCount
    const p1 = reg.getOrCreatePropertyProxy(b1, "y", 500) catch unreachable;
    const p1_again = reg.getOrCreatePropertyProxy(b1, "y", 501) catch unreachable;
    assertEq(p1, @as(u32, 500));
    assertEq(p1_again, @as(u32, 500));
    assertEq(reg.proxyCount(), @as(usize, 1));

    // Distinct block gets its own proxy ref for the same member name
    const p2 = reg.getOrCreatePropertyProxy(b2, "y", 600) catch unreachable;
    assertEq(p2, @as(u32, 600));
    assertEq(reg.proxyCount(), @as(usize, 2));

    let buf: [32]u8 = [0; 32];
    const n1 = reg.formatPropertyAccess(b1, "y", buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n1], "x2.y"));
    const n2 = reg.formatPropertyAccess(b2, "y", buf[0..]) catch unreachable;
    assert(eqStr(buf[0..n2], "x3.y"));
});

test("TsNamespaceError: InvalidScopeId, BufferTooSmall & ScopesFull guards", () => {
    assertEq(TS_NS_MAX_SCOPES, @as(usize, 32));
    assertEq(TS_NS_MAX_MEMBERS, @as(usize, 128));
    assertEq(TS_NS_MAX_PROXIES, @as(usize, 128));

    let reg = TsNamespaceRegistry.init();
    let invalid_err = false;
    const r1 = reg.getOrCreatePropertyProxy(99, "y", 1) catch |e| {
        if (e == TsNamespaceError.InvalidScopeId) invalid_err = true;
        @as(u32, 0)
    };
    * = r1;
    assert(invalid_err);

    const gid = reg.createGroup();
    const b0 = reg.openBlock(gid, 1, "ns", false) catch unreachable;
    let tiny: [3]u8 = [0; 3];
    let buf_err = false;
    const r2 = reg.formatPropertyAccess(b0, "member", tiny[0..]) catch |e| {
        if (e == TsNamespaceError.BufferTooSmall) buf_err = true;
        @as(usize, 0)
    };
    * = r2;
    assert(buf_err);

    let k: usize = 1;
    while (k < TS_NS_MAX_SCOPES) {
        * = reg.openBlock(gid, @as(u32, @intCast(k)), "ns", false) catch unreachable;
        k += 1;
    }
    let full_err = false;
    const r3 = reg.openBlock(gid, 999, "ns", false) catch |e| {
        if (e == TsNamespaceError.ScopesFull) full_err = true;
        @as(u32, 0)
    };
    * = r3;
    assert(full_err);
});
