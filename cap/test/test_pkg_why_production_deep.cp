import { test, expect, expect_eq } from "cap:test";
import {
    WHY_MAX_PACKAGES,
    WHY_MAX_EDGES,
    WHY_MAX_DEPTH,
    WhyError,
    WhyDepType,
    specifierSpecificity,
    WhyGlobKind,
    WhyGlobPattern,
    WhyPackage,
    WhyDependent,
    compareWhyDependents,
    WhyOptions,
    WhyGraph,
} from "cap:pkg";

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

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

test("pkg why: WhyGlobPattern name globs, scoped @version ranges, and invalid guards", fn()! {
    expect_eq(WHY_MAX_PACKAGES, 64);
    expect_eq(WHY_MAX_EDGES, 256);
    expect_eq(WHY_MAX_DEPTH, 32);

    const exact = WhyGlobPattern.parse("react")?;
    expect(exact.kind == .Exact);
    expect(exact.matches("react", "18.2.0"));
    expect(!exact.matches("react-dom", "18.2.0"));

    const pref = WhyGlobPattern.parse("@types/*")?;
    expect(pref.kind == .Prefix);
    expect(pref.matches("@types/react", "18.2.15"));
    expect(pref.matches("@types/node", "20.11.0"));
    expect(!pref.matches("react", "18.2.0"));

    const suff = WhyGlobPattern.parse("*-lodash")?;
    expect(suff.kind == .Suffix);
    expect(suff.matches("es-lodash", "1.0.0"));
    expect(!suff.matches("lodash-es", "1.0.0"));

    const mid = WhyGlobPattern.parse("@acme/*-core")?;
    expect(mid.kind == .Middle);
    expect(mid.matches("@acme/ui-core", "2.0.0"));
    expect(!mid.matches("@acme/ui-extra", "2.0.0"));

    const cont = WhyGlobPattern.parse("*react*")?;
    expect(cont.kind == .Contains);
    expect(cont.matches("@types/react-dom", "18.2.7"));
    expect(!cont.matches("lodash", "4.17.21"));

    // Scoped package with semver range suffix
    const scoped_ver = WhyGlobPattern.parse("@types/react@^18.0.0")?;
    expect(scoped_ver.kind == .Exact);
    expect(scoped_ver.matches("@types/react", "18.2.15"));
    expect(!scoped_ver.matches("@types/react", "19.0.0"));
    expect(!scoped_ver.matches("@types/react", "17.0.2"));

    const tilde_ver = WhyGlobPattern.parse("express@~4.18.0")?;
    expect(tilde_ver.matches("express", "4.18.2"));
    expect(!tilde_ver.matches("express", "4.19.0"));

    const gte_ver = WhyGlobPattern.parse("zod@>=3.20.0")?;
    expect(gte_ver.matches("zod", "3.22.4"));
    expect(!gte_ver.matches("zod", "3.19.9"));

    // Invalid patterns rejected
    let rejected: usize = 0;
    if (_ := WhyGlobPattern.parse("")) {} else { rejected += 1; }
    if (_ := WhyGlobPattern.parse("a*b*c")) {} else { rejected += 1; }
    if (_ := WhyGlobPattern.parse("**foo**")) {} else { rejected += 1; }
    expect_eq(rejected, 3);
});

test("pkg why: specifierSpecificity, dep type priority, and deterministic dependent sorting", fn()! {
    expect_eq(specifierSpecificity("1.2.3"), 10);
    expect_eq(specifierSpecificity("workspace:*"), 9);
    expect_eq(specifierSpecificity(""), 9);
    expect_eq(specifierSpecificity("^1.2.0"), 8);
    expect_eq(specifierSpecificity("~1.2.0"), 7);
    expect_eq(specifierSpecificity(">=1.0.0"), 6);
    expect_eq(specifierSpecificity("1.x"), 5);
    expect_eq(specifierSpecificity("latest"), 3);
    expect_eq(specifierSpecificity("*"), 1);

    expect_eq(WhyDepType.Prod.priority(), 4);
    expect_eq(WhyDepType.Peer.priority(), 3);
    expect_eq(WhyDepType.OptionalPeer.priority(), 2);
    expect_eq(WhyDepType.Optional.priority(), 1);
    expect_eq(WhyDepType.Dev.priority(), 0);

    let g = WhyGraph.init();
    const target = g.addPackage("lodash", "4.17.21", false)?;
    const p_star = g.addPackage("z-star", "1.0.0", false)?;
    const p_caret_dev = g.addPackage("a-dev", "1.0.0", false)?;
    const p_caret_prod = g.addPackage("b-prod", "1.0.0", false)?;
    const p_exact = g.addPackage("c-exact", "1.0.0", false)?;

    g.addDependency(p_star, target, "*", .Prod)?;
    g.addDependency(p_caret_dev, target, "^4.17.0", .Dev)?;
    g.addDependency(p_caret_prod, target, "^4.17.0", .Prod)?;
    g.addDependency(p_exact, target, "4.17.21", .Dev)?;

    let deps: [8]WhyDependent = [WhyDependent{}; 8];
    const n = g.collectDependents(target, deps[0..]);
    expect_eq(n, 4);
    // Order: exact(10) -> caret+prod(8,4) -> caret+dev(8,0) -> star(1)
    expect(strEql(deps[0].name, "c-exact"));
    expect(strEql(deps[1].name, "b-prod"));
    expect(strEql(deps[2].name, "a-dev"));
    expect(strEql(deps[3].name, "z-star"));
});

test("pkg why: WhyGraph renderExplain matches Bun why tree format with glob & dep labels", fn()! {
    let g = WhyGraph.init();
    const app = g.addPackage("my-app", "1.0.0", false)?;
    const types_react = g.addPackage("@types/react", "18.2.15", false)?;
    const types_dom = g.addPackage("@types/react-dom", "18.2.7", false)?;
    g.addDependency(app, types_react, "^18.0.0", .Dev)?;
    g.addDependency(app, types_dom, "^18.0.0", .Dev)?;

    let buf: [512]u8 = [0; 512];
    const n = g.renderExplain("@types/*", WhyOptions{}, buf[0..])?;
    const expected =
        "@types/react@18.2.15\n" ++
        "  └─ dev my-app@1.0.0 (requires ^18.0.0)\n" ++
        "\n" ++
        "@types/react-dom@18.2.7\n" ++
        "  └─ dev my-app@1.0.0 (requires ^18.0.0)\n";
    expect(strEql(buf[0..n], expected));
});

test("pkg why: --top and --depth truncation emit deeper dependencies hidden", fn()! {
    let g = WhyGraph.init();
    const app = g.addPackage("my-app", "1.0.0", true)?;
    const mid = g.addPackage("body-parser", "1.20.1", false)?;
    const leaf = g.addPackage("bytes", "3.1.2", false)?;
    g.addDependency(app, mid, "^1.20.0", .Prod)?;
    g.addDependency(mid, leaf, "3.1.2", .Prod)?;

    let buf: [512]u8 = [0; 512];

    // Full depth: shows leaf -> mid -> app (workspace omits @version)
    const full_n = g.renderExplain("bytes", WhyOptions{}, buf[0..])?;
    const full_expected =
        "bytes@3.1.2\n" ++
        "  └─ body-parser@1.20.1 (requires 3.1.2)\n" ++
        "       └─ my-app (requires ^1.20.0)\n";
    expect(strEql(buf[0..full_n], full_expected));

    // --top (depth=1): hides my-app under body-parser
    const top_n = g.renderExplain("bytes", WhyOptions{ top_only: true }, buf[0..])?;
    expect(containsSubstr(buf[0..top_n], "body-parser@1.20.1"));
    expect(containsSubstr(buf[0..top_n], "(deeper dependencies hidden)"));
    expect(!containsSubstr(buf[0..top_n], "my-app"));

    // --depth 0: immediately hides dependents
    const d0_n = g.renderExplain("bytes", WhyOptions{ max_depth: 0 }, buf[0..])?;
    expect(strEql(buf[0..d0_n], "bytes@3.1.2\n  └─ (deeper dependencies hidden)\n"));
});

test("pkg why: cycle detection safety and orphan package output", fn()! {
    let g = WhyGraph.init();
    const a = g.addPackage("pkg-a", "1.0.0", false)?;
    const b = g.addPackage("pkg-b", "1.0.0", false)?;
    const orphan = g.addPackage("orphan-pkg", "0.1.0", false)?;
    g.addDependency(a, b, "^1.0.0", .Prod)?;
    g.addDependency(b, a, "^1.0.0", .Peer)?;

    let buf: [512]u8 = [0; 512];
    const cyc_n = g.renderExplain("pkg-a", WhyOptions{}, buf[0..])?;
    expect(containsSubstr(buf[0..cyc_n], "peer pkg-b@1.0.0"));
    expect(containsSubstr(buf[0..cyc_n], "pkg-a@1.0.0 (requires ^1.0.0) (circular)"));

    const orph_n = g.renderExplain("orphan-pkg", WhyOptions{}, buf[0..])?;
    expect(strEql(buf[0..orph_n], "orphan-pkg@0.1.0\n  └─ No dependents found\n"));
    * = orphan;
});

test("pkg why: error guards for NoMatchingPackage, InvalidPackageId, and BufferTooSmall", fn()! {
    let g = WhyGraph.init();
    const p0 = g.addPackage("zod", "3.22.4", false)?;
    const p1 = g.addPackage("app", "1.0.0", false)?;
    g.addDependency(p1, p0, "^3.22.0", .Prod)?;

    let errs: usize = 0;
    if (_ := g.addDependency(p1, 99, "^1.0.0", .Prod)) {} else { errs += 1; }

    let buf: [128]u8 = [0; 128];
    if (_ := g.renderExplain("nonexistent", WhyOptions{}, buf[0..])) {} else { errs += 1; }

    let tiny: [8]u8 = [0; 8];
    if (_ := g.renderExplain("zod", WhyOptions{}, tiny[0..])) {} else { errs += 1; }

    expect_eq(errs, 3);
});
