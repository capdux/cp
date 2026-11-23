import { test, assert, assertEq } from "cap:test";
import {
    DepListNormalizeError,
    DepProtocolKind,
    parseDependencyProtocolSpec,
    DepNormGroup,
    RawDepItem,
    NormalizedDepItem,
    normalizeDependencyList,
    DepOverrideRuleKind,
    DepOverrideRule,
    DepOverrideAuditEntry,
    applyDependencyOverrides,
    formatNormalizedDepReport,
    summarizeDepProtocols,
    findNormalizedDep,
} from "cap:pkg";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn strContains(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("parseDependencyProtocolSpec accurately classifies all 10 npm/cap dependency protocols", () => {
    assert((parseDependencyProtocolSpec("^1.2.3") catch unreachable).kind == DepProtocolKind.SemverRange);
    assert((parseDependencyProtocolSpec("latest") catch unreachable).kind == DepProtocolKind.DistTag);

    const alias = parseDependencyProtocolSpec("npm:@scope/pkg@^2.0.0") catch unreachable;
    assert(alias.kind == DepProtocolKind.NpmAlias);
    assert(strEql(alias.target_name, "@scope/pkg") && strEql(alias.version_or_ref, "^2.0.0"));

    const ws = parseDependencyProtocolSpec("workspace:^") catch unreachable;
    assert(ws.kind == DepProtocolKind.WorkspaceProtocol && strEql(ws.version_or_ref, "^"));

    const cat = parseDependencyProtocolSpec("catalog:react19") catch unreachable;
    assert(cat.kind == DepProtocolKind.CatalogProtocol && strEql(cat.version_or_ref, "react19"));

    const f = parseDependencyProtocolSpec("file:./packages/local") catch unreachable;
    assert(f.kind == DepProtocolKind.FileProtocol && strEql(f.local_or_url_path, "./packages/local"));

    const lnk = parseDependencyProtocolSpec("link:../shared") catch unreachable;
    assert(lnk.kind == DepProtocolKind.LinkProtocol);

    const gh = parseDependencyProtocolSpec("github:oven-sh/bun#v1.2") catch unreachable;
    assert(gh.kind == DepProtocolKind.GitHubShorthand);
    assert(strEql(gh.target_name, "oven-sh/bun") && strEql(gh.version_or_ref, "v1.2"));

    const git = parseDependencyProtocolSpec("git+https://github.com/cap-lang/cp.git#main") catch unreachable;
    assert(git.kind == DepProtocolKind.GitUrl && strEql(git.version_or_ref, "main"));

    const tgz = parseDependencyProtocolSpec("https://registry.npmjs.org/zod/-/zod-3.23.8.tgz") catch unreachable;
    assert(tgz.kind == DepProtocolKind.TarballUrl);
});

test("parseDependencyProtocolSpec blocks unsafe schemes, shell metachars, and path traversal", () => {
    let e1 = false;
    * = parseDependencyProtocolSpec("javascript:alert(1)") catch |e| { e1 = (e == DepListNormalizeError.UnsafeProtocol); };
    assert(e1);

    let e2 = false;
    * = parseDependencyProtocolSpec("http://insecure.example.com/pkg.tgz") catch |e| { e2 = (e == DepListNormalizeError.UnsafeProtocol); };
    assert(e2);

    let e3 = false;
    * = parseDependencyProtocolSpec("^1.0.0;rm -rf /") catch |e| { e3 = (e == DepListNormalizeError.UnsafeProtocol); };
    assert(e3);

    let e4 = false;
    * = parseDependencyProtocolSpec("https://cdn.example.com/../../etc/passwd.tgz") catch |e| { e4 = (e == DepListNormalizeError.PathTraversalDetected); };
    assert(e4);

    let e5 = false;
    * = parseDependencyProtocolSpec("catalog:../secret") catch |e| { e5 = (e == DepListNormalizeError.PathTraversalDetected); };
    assert(e5);
});

test("normalizeDependencyList merges duplicates by group priority and sorts lexicographically", () => {
    // Priority: Dependencies > OptionalDependencies > DevDependencies > PeerDependencies
    const raw: [6]RawDepItem = [
        RawDepItem{ name: "zod", spec: "^3.20.0", group: DepNormGroup.PeerDependencies },
        RawDepItem{ name: "zod", spec: "^3.22.0", group: DepNormGroup.DevDependencies },
        RawDepItem{ name: "zod", spec: "^3.23.8", group: DepNormGroup.Dependencies },
        RawDepItem{ name: "axios", spec: "^1.7.0", group: DepNormGroup.OptionalDependencies },
        RawDepItem{ name: "axios", spec: "^1.6.0", group: DepNormGroup.DevDependencies },
        RawDepItem{ name: "@acme/core", spec: "workspace:^", group: DepNormGroup.Dependencies },
    ];

    let out: [8]NormalizedDepItem = [NormalizedDepItem.empty(); 8];
    const sum = normalizeDependencyList(raw[0..], out[0..]) catch unreachable;

    assertEq(sum.input_count, @as(u32, 6));
    assertEq(sum.output_count, @as(u32, 3));
    assertEq(sum.deduplicated_count, @as(u32, 3));
    assertEq(sum.conflict_resolved_count, @as(u32, 3));

    // Sorted: @acme/core, axios, zod
    assert(strEql(out[0].name, "@acme/core") && out[0].protocol == DepProtocolKind.WorkspaceProtocol);
    assert(strEql(out[1].name, "axios") && out[1].group == DepNormGroup.OptionalDependencies && strEql(out[1].spec, "^1.7.0"));
    assert(strEql(out[2].name, "zod") && out[2].group == DepNormGroup.Dependencies && strEql(out[2].spec, "^3.23.8"));
});

test("findNormalizedDep performs O(log N) lookup and summarizeDepProtocols tallies protocol families", () => {
    const raw: [4]RawDepItem = [
        RawDepItem{ name: "zod", spec: "^3.23.8" },
        RawDepItem{ name: "@acme/ui", spec: "workspace:*" },
        RawDepItem{ name: "react", spec: "catalog:" },
        RawDepItem{ name: "fast-qs", spec: "npm:qs@^6.12.0" },
    ];
    let out: [4]NormalizedDepItem = [NormalizedDepItem.empty(); 4];
    const sum = normalizeDependencyList(raw[0..], out[0..]) catch unreachable;
    const slice = out[0..@as(usize, sum.output_count)];

    const idx = findNormalizedDep(slice, "react", "");
    assert(idx != null && strEql(slice[idx ?? 0].spec, "catalog:"));
    assert(findNormalizedDep(slice, "missing", "") == null);

    const counts = summarizeDepProtocols(slice);
    assertEq(counts.semver_count, @as(u32, 1));
    assertEq(counts.workspace_count, @as(u32, 1));
    assertEq(counts.catalog_count, @as(u32, 1));
    assertEq(counts.npm_alias_count, @as(u32, 1));
});

test("applyDependencyOverrides enforces ParentChildPath > VersionScoped > ExactName precedence and emits audit trail", () => {
    const raw: [3]RawDepItem = [
        RawDepItem{ name: "lodash", spec: "^4.17.15", group: DepNormGroup.Dependencies },
        RawDepItem{ name: "qs", spec: "^6.9.0", group: DepNormGroup.Dependencies, parent_name: "express" },
        RawDepItem{ name: "qs", spec: "^6.11.0", group: DepNormGroup.Dependencies, parent_name: "other" },
    ];
    let norm: [4]NormalizedDepItem = [NormalizedDepItem.empty(); 4];
    const nsum = normalizeDependencyList(raw[0..], norm[0..]) catch unreachable;
    const items = norm[0..@as(usize, nsum.output_count)];

    const rules: [3]DepOverrideRule = [
        DepOverrideRule{ raw_key: "qs", replacement_spec: "6.11.2" },
        DepOverrideRule{ raw_key: "lodash@^4.17.15", replacement_spec: "4.17.21" },
        DepOverrideRule{ raw_key: "express>qs", replacement_spec: "6.12.1" },
    ];
    let audit: [4]DepOverrideAuditEntry = [DepOverrideAuditEntry.empty(); 4];
    const osum = applyDependencyOverrides(items, rules[0..], audit[0..]) catch unreachable;

    assertEq(osum.applied_count, @as(u32, 3));
    assertEq(osum.parent_path_applied, @as(u32, 1));
    assertEq(osum.version_scoped_applied, @as(u32, 1));
    assertEq(osum.exact_name_applied, @as(u32, 1));

    const l_idx = findNormalizedDep(items, "lodash", "") ?? 0;
    assert(strEql(items[l_idx].spec, "4.17.21") && items[l_idx].was_overridden);

    const eqs_idx = findNormalizedDep(items, "qs", "express") ?? 0;
    assert(strEql(items[eqs_idx].spec, "6.12.1"));

    const oqs_idx = findNormalizedDep(items, "qs", "other") ?? 0;
    assert(strEql(items[oqs_idx].spec, "6.11.2"));
    assert(audit[0].rule_kind == DepOverrideRuleKind.VersionScoped);
});

test("applyDependencyOverrides re-parses replacement protocol and rejects malformed override keys", () => {
    const raw: [1]RawDepItem = [
        RawDepItem{ name: "qs", spec: "^6.9.0" },
    ];
    let norm: [2]NormalizedDepItem = [NormalizedDepItem.empty(); 2];
    const nsum = normalizeDependencyList(raw[0..], norm[0..]) catch unreachable;
    const items = norm[0..@as(usize, nsum.output_count)];

    const alias_rule: [1]DepOverrideRule = [
        DepOverrideRule{ raw_key: "qs", replacement_spec: "npm:patched-qs@^6.12.0" },
    ];
    let audit: [2]DepOverrideAuditEntry = [DepOverrideAuditEntry.empty(); 2];
    const osum = applyDependencyOverrides(items, alias_rule[0..], audit[0..]) catch unreachable;
    assertEq(osum.applied_count, @as(u32, 1));
    assert(items[0].protocol == DepProtocolKind.NpmAlias);

    const bad_rule: [1]DepOverrideRule = [
        DepOverrideRule{ raw_key: "parent>", replacement_spec: "1.0.0" },
    ];
    let bad_err = false;
    * = applyDependencyOverrides(items, bad_rule[0..], audit[0..]) catch |e| {
        bad_err = (e == DepListNormalizeError.InvalidOverrideRule);
    };
    assert(bad_err);
});

test("formatNormalizedDepReport renders counts, protocols, and override audit trails", () => {
    const raw: [2]RawDepItem = [
        RawDepItem{ name: "lodash", spec: "^4.17.15" },
        RawDepItem{ name: "lodash", spec: "^4.17.10", group: DepNormGroup.DevDependencies },
    ];
    let norm: [2]NormalizedDepItem = [NormalizedDepItem.empty(); 2];
    const nsum = normalizeDependencyList(raw[0..], norm[0..]) catch unreachable;
    const items = norm[0..@as(usize, nsum.output_count)];

    const rules: [1]DepOverrideRule = [
        DepOverrideRule{ raw_key: "lodash", replacement_spec: "4.17.21" },
    ];
    let audit: [2]DepOverrideAuditEntry = [DepOverrideAuditEntry.empty(); 2];
    const osum = applyDependencyOverrides(items, rules[0..], audit[0..]) catch unreachable;

    let rbuf: [512]u8 = [0; 512];
    const rlen = formatNormalizedDepReport(items, nsum, osum, rbuf[0..]) catch unreachable;
    const report = rbuf[0..rlen];
    assert(strContains(report, "Normalized Dependencies: 1 unique (from 2, 1 deduped, 1 overridden)"));
    assert(strContains(report, "lodash@4.17.21 [dependencies/semver] (override: ^4.17.15 -> 4.17.21)"));
});
