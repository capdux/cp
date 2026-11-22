// Production Deep Test for cap:bundler (lazyExport.cp)
// Covers: JS identifier & reserved-word checks, default export identifier sanitization,
// ESM tree-shakeable named export promotion vs CJS / N-API __require planning,
// CSS Modules recursive composes expansion (local / cross-file / global), cycle protection,
// non-single-class rejection, and buffer boundary guards.

import { test, assert, assertEq } from "cap:test";
import {
    isLazyExportJsIdentifier,
    isPromotableLazyNamedExport,
    formatLazyDefaultIdent,
    LazyExportKind,
    LazyExportPropDecision,
    LazyExportPlanner,
    CssComposeGraph,
} from "cap:bundler";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

test("lazyExport: JS identifier and promotable named export validation", () => {
    assert(isLazyExportJsIdentifier("version"));
    assert(isLazyExportJsIdentifier("$foo_1"));
    assert(isLazyExportJsIdentifier("_bar"));

    assert(!isLazyExportJsIdentifier(""));
    assert(!isLazyExportJsIdentifier("123abc"));
    assert(!isLazyExportJsIdentifier("foo-bar"));
    assert(!isLazyExportJsIdentifier("class"));
    assert(!isLazyExportJsIdentifier("default"));

    assert(isPromotableLazyNamedExport("dependencies"));
    assert(!isPromotableLazyNamedExport("default"));
    assert(!isPromotableLazyNamedExport("__esModule"));
    assert(!isPromotableLazyNamedExport("foo-bar"));
});

test("lazyExport: formatLazyDefaultIdent sanitization and buffer guard", () => {
    let buf: [64]u8 = [0; 64];

    const n1 = formatLazyDefaultIdent("package", buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n1], "package_default"));

    const n2 = formatLazyDefaultIdent("my-config.prod", buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n2], "my_config_prod_default"));

    const n3 = formatLazyDefaultIdent("404-page", buf[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(buf[0..n3], "_404_page_default"));

    let tiny: [4]u8 = [0; 4];
    const fail_n = formatLazyDefaultIdent("pkg", tiny[0..]) catch 999;
    assertEq(fail_n, 999);
});

test("lazyExport: ESM tree-shakeable promotion vs CJS and N-API __require", () => {
    let esm = LazyExportPlanner.init(LazyExportKind.Esm, false);
    const d1 = esm.addProperty("name") catch {
        assert(false);
        return;
    };
    const d2 = esm.addProperty("version") catch {
        assert(false);
        return;
    };
    const d3 = esm.addProperty("default") catch {
        assert(false);
        return;
    };
    const d4 = esm.addProperty("__esModule") catch {
        assert(false);
        return;
    };
    const d5 = esm.addProperty("peer-deps") catch {
        assert(false);
        return;
    };

    assert(d1 == LazyExportPropDecision.PromotedNamedExport);
    assert(d2 == LazyExportPropDecision.PromotedNamedExport);
    assert(d3 == LazyExportPropDecision.DefaultOnlyReserved);
    assert(d4 == LazyExportPropDecision.DefaultOnlyReserved);
    assert(d5 == LazyExportPropDecision.DefaultOnlyNonIdent);

    const esm_sum = esm.summary();
    assertEq(esm_sum.promoted_named_count, 2);
    assertEq(esm_sum.default_only_count, 3);
    assertEq(esm_sum.total_props, 5);
    assert(!esm_sum.needs_runtime_require);

    if (p0 := esm.propAt(0)) {
        assertEq(p0.part_index, 2);
    } else {
        assert(false);
    }
    if (p1 := esm.propAt(1)) {
        assertEq(p1.part_index, 3);
    } else {
        assert(false);
    }

    // N-API addon in ESM output needs __require; in CJS output does not
    const napi_esm = LazyExportPlanner.init(LazyExportKind.CjsNapiAddon, false);
    assert(napi_esm.summary().needs_runtime_require);

    const napi_cjs = LazyExportPlanner.init(LazyExportKind.CjsNapiAddon, true);
    assert(!napi_cjs.summary().needs_runtime_require);
});

test("lazyExport: CssComposeGraph local, cross-file, and global expansion", () => {
    let g = CssComposeGraph.init();

    const base = g.registerClass(0, "base", "base_a1", true) catch {
        assert(false);
        return;
    };
    const shared = g.registerClass(1, "shared", "shared_b2", true) catch {
        assert(false);
        return;
    };
    const primary = g.registerClass(1, "primary", "primary_c3", true) catch {
        assert(false);
        return;
    };

    * = g.addComposeGlobal(base, "u-reset") catch {
        assert(false);
        return;
    };
    * = g.addComposeRef(shared, base) catch {
        assert(false);
        return;
    };
    * = g.addComposeRef(primary, shared) catch {
        assert(false);
        return;
    };
    // Diamond duplicate reference to `base` must be deduplicated
    * = g.addComposeRef(primary, base) catch {
        assert(false);
        return;
    };

    let out: [128]u8 = [0; 128];
    const n = g.formatExpandedClassExpr(primary, out[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(out[0..n], "u-reset base_a1 shared_b2 primary_c3"));
});

test("lazyExport: CssComposeGraph cycle safety and non-single-class guard", () => {
    let g = CssComposeGraph.init();

    const a = g.registerClass(0, "a", "a_1", true) catch {
        assert(false);
        return;
    };
    const b = g.registerClass(0, "b", "b_2", true) catch {
        assert(false);
        return;
    };
    const multi = g.registerClass(0, "complex", "complex_3", false) catch {
        assert(false);
        return;
    };

    // Mutual cycle a <-> b must terminate cleanly without infinite recursion
    * = g.addComposeRef(a, b) catch {
        assert(false);
        return;
    };
    * = g.addComposeRef(b, a) catch {
        assert(false);
        return;
    };

    let out: [64]u8 = [0; 64];
    const n = g.formatExpandedClassExpr(a, out[0..]) catch {
        assert(false);
        return;
    };
    assert(strEql(out[0..n], "b_2 a_1"));

    // Composing a non-single-class selector must be rejected
    const bad1 = g.addComposeRef(a, multi) catch false;
    assert(!bad1);
    const bad2 = g.addComposeGlobal(multi, "global-cls") catch false;
    assert(!bad2);

    // Duplicate class in same source_index must be rejected
    const dup = g.registerClass(0, "a", "a_dup", true) catch 9999;
    assertEq(dup, 9999);
});
