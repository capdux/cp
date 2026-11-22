import { test, assert, assertEq } from "cap:test";
import {
    ManifestUpdaterError,
    ManifestIndentKind,
    ManifestNewlineKind,
    detectManifestFormat,
    ManifestVersionStyle,
    ManifestVersionKind,
    classifyVersionSpec,
    normalizeVersionSpec,
    ManifestDepGroup,
    ManifestMutationKind,
    ManifestMutationOp,
    ManifestChangeKind,
    ManifestChangeRecord,
    ManifestDepEntry,
    ManifestTopPropSpan,
    ManifestUpdateOptions,
    ManifestUpdateBuffers,
    applyManifestMutations,
    formatManifestUpdateReport,
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

test("detectManifestFormat accurately identifies 2-space LF, 4-space CRLF, Tab indent, and trailing newline", () => {
    const f2 = detectManifestFormat("{\n  \"name\": \"a\"\n}\n");
    assert(f2.indent_kind == ManifestIndentKind.Spaces);
    assertEq(f2.indent_width, @as(u8, 2));
    assert(f2.newline_kind == ManifestNewlineKind.Lf);
    assert(f2.has_trailing_newline);
    assert(f2.space_after_colon);

    const f4 = detectManifestFormat("{\r\n    \"name\": \"b\"\r\n}");
    assert(f4.indent_kind == ManifestIndentKind.Spaces);
    assertEq(f4.indent_width, @as(u8, 4));
    assert(f4.newline_kind == ManifestNewlineKind.CrLf);
    assert(!f4.has_trailing_newline);

    const ftab = detectManifestFormat("{\n\t\"name\":\"c\"\n}\n");
    assert(ftab.indent_kind == ManifestIndentKind.Tabs);
    assertEq(ftab.indent_width, @as(u8, 1));
    assert(!ftab.space_after_colon);

    const fsingle = detectManifestFormat("{\"name\": \"d\"}");
    assert(fsingle.is_single_line);
    assert(!fsingle.has_trailing_newline);
});

test("classifyVersionSpec and normalizeVersionSpec handle exact, caret, tilde, workspace, npm alias, and catalog protocols", () => {
    assert(classifyVersionSpec("^1.2.3") == ManifestVersionKind.Semver);
    assert(classifyVersionSpec("workspace:^") == ManifestVersionKind.WorkspaceProtocol);
    assert(classifyVersionSpec("npm:lodash@^4.17.21") == ManifestVersionKind.NpmAlias);
    assert(classifyVersionSpec("catalog:react19") == ManifestVersionKind.CatalogProtocol);
    assert(classifyVersionSpec("latest") == ManifestVersionKind.TagOrRange);

    let buf: [128]u8 = [0; 128];

    const n1 = normalizeVersionSpec("1.2.3", ManifestVersionStyle.Caret, "", "", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n1], "^1.2.3"));

    const n2 = normalizeVersionSpec("^1.2.3", ManifestVersionStyle.Tilde, "", "", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n2], "~1.2.3"));

    const n3 = normalizeVersionSpec("^1.2.3", ManifestVersionStyle.Exact, "", "", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n3], "1.2.3"));

    const n4 = normalizeVersionSpec("", ManifestVersionStyle.WorkspaceCaret, "", "", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n4], "workspace:^"));

    const n5 = normalizeVersionSpec("", ManifestVersionStyle.WorkspaceStar, "", "", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n5], "workspace:*"));

    const n6 = normalizeVersionSpec("", ManifestVersionStyle.CatalogDefault, "tooling", "", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n6], "catalog:tooling"));

    const n7 = normalizeVersionSpec("4.17.21", ManifestVersionStyle.Caret, "", "lodash", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n7], "npm:lodash@^4.17.21"));

    // Re-style existing npm alias to --exact
    const n8 = normalizeVersionSpec("npm:@scope/pkg@^2.0.0", ManifestVersionStyle.Exact, "", "", buf[0..]) catch unreachable;
    assert(strEql(buf[0..n8], "npm:@scope/pkg@2.0.0"));

    // Empty version with Caret rejected
    let bad = false;
    * = normalizeVersionSpec("", ManifestVersionStyle.Caret, "", "", buf[0..]) catch |e| {
        bad = (e == ManifestUpdaterError.InvalidVersionSpec);
    };
    assert(bad);
});

test("applyManifestMutations inserts alphabetically while preserving unrelated fields and nested JSON verbatim", () => {
    const src =
        "{\n" ++
        "  \"name\": \"cap-demo\",\n" ++
        "  \"private\": true,\n" ++
        "  \"scripts\": {\n" ++
        "    \"build\": \"cap build --opt=fast\"\n" ++
        "  },\n" ++
        "  \"dependencies\": {\n" ++
        "    \"zod\": \"^3.22.0\"\n" ++
        "  },\n" ++
        "  \"workspaces\": [\"packages/*\"]\n" ++
        "}\n";

    const ops: [2]ManifestMutationOp = [
        ManifestMutationOp{
            kind: ManifestMutationKind.Add,
            package_name: "axios",
            version_spec: "1.7.2",
            target_group: ManifestDepGroup.Dependencies,
            version_style: ManifestVersionStyle.Caret,
        },
        ManifestMutationOp{
            kind: ManifestMutationKind.Add,
            package_name: "@acme/core",
            version_spec: "",
            target_group: ManifestDepGroup.Dependencies,
            version_style: ManifestVersionStyle.WorkspaceCaret,
        },
    ];

    let props: [16]ManifestTopPropSpan = [ManifestTopPropSpan.empty(); 16];
    let deps: [32]ManifestDepEntry = [ManifestDepEntry.empty(); 32];
    let specs: [256]u8 = [0; 256];
    let changes: [8]ManifestChangeRecord = [ManifestChangeRecord.empty(); 8];
    let bufs = ManifestUpdateBuffers{
        top_props: props[0..],
        dep_entries: deps[0..],
        spec_scratch: specs[0..],
        out_changes: changes[0..],
    };
    let out: [2048]u8 = [0; 2048];

    const sum = applyManifestMutations(src, ops[0..], ManifestUpdateOptions.defaults(), &bufs, out[0..]) catch unreachable;
    assertEq(sum.added_count, @as(u32, 2));
    assert(sum.has_mutations);

    const expected =
        "{\n" ++
        "  \"name\": \"cap-demo\",\n" ++
        "  \"private\": true,\n" ++
        "  \"scripts\": {\n" ++
        "    \"build\": \"cap build --opt=fast\"\n" ++
        "  },\n" ++
        "  \"dependencies\": {\n" ++
        "    \"@acme/core\": \"workspace:^\",\n" ++
        "    \"axios\": \"^1.7.2\",\n" ++
        "    \"zod\": \"^3.22.0\"\n" ++
        "  },\n" ++
        "  \"workspaces\": [\"packages/*\"]\n" ++
        "}\n";
    assert(strEql(out[0..sum.bytes_written], expected));
});

test("applyManifestMutations deduplicates across groups on Add/MoveGroup and upgrades in-place", () => {
    const src =
        "{\n" ++
        "  \"name\": \"cross-group\",\n" ++
        "  \"dependencies\": {\n" ++
        "    \"react\": \"^18.2.0\"\n" ++
        "  },\n" ++
        "  \"devDependencies\": {\n" ++
        "    \"typescript\": \"^5.4.0\",\n" ++
        "    \"zod\": \"^3.21.0\"\n" ++
        "  }\n" ++
        "}\n";

    // 1) Add zod into dependencies -> moves out of devDependencies (no duplicate!)
    // 2) Upgrade typescript -> updates in-place inside devDependencies
    // 3) MoveGroup react -> moves to peerDependencies preserving version
    const ops: [3]ManifestMutationOp = [
        ManifestMutationOp{
            kind: ManifestMutationKind.Add,
            package_name: "zod",
            version_spec: "3.23.8",
            target_group: ManifestDepGroup.Dependencies,
            version_style: ManifestVersionStyle.Caret,
        },
        ManifestMutationOp{
            kind: ManifestMutationKind.Upgrade,
            package_name: "typescript",
            version_spec: "5.6.2",
            target_group: ManifestDepGroup.Dependencies,
            version_style: ManifestVersionStyle.Exact,
        },
        ManifestMutationOp{
            kind: ManifestMutationKind.MoveGroup,
            package_name: "react",
            version_spec: "",
            target_group: ManifestDepGroup.PeerDependencies,
            version_style: ManifestVersionStyle.Preserve,
        },
    ];

    let props: [16]ManifestTopPropSpan = [ManifestTopPropSpan.empty(); 16];
    let deps: [32]ManifestDepEntry = [ManifestDepEntry.empty(); 32];
    let specs: [256]u8 = [0; 256];
    let changes: [8]ManifestChangeRecord = [ManifestChangeRecord.empty(); 8];
    let bufs = ManifestUpdateBuffers{
        top_props: props[0..],
        dep_entries: deps[0..],
        spec_scratch: specs[0..],
        out_changes: changes[0..],
    };
    let out: [2048]u8 = [0; 2048];

    const sum = applyManifestMutations(src, ops[0..], ManifestUpdateOptions.defaults(), &bufs, out[0..]) catch unreachable;
    assertEq(sum.moved_count, @as(u32, 2));
    assertEq(sum.updated_count, @as(u32, 1));
    assert(changes[0].kind == ManifestChangeKind.Moved);
    assert(changes[0].from_group == ManifestDepGroup.DevDependencies);
    assert(changes[0].to_group == ManifestDepGroup.Dependencies);
    assert(changes[1].kind == ManifestChangeKind.Updated);
    assert(changes[1].to_group == ManifestDepGroup.DevDependencies);
    assert(changes[2].kind == ManifestChangeKind.Moved);
    assert(changes[2].to_group == ManifestDepGroup.PeerDependencies);

    const rendered = out[0..sum.bytes_written];
    assert(strContains(rendered, "\"dependencies\": {\n    \"zod\": \"^3.23.8\"\n  }"));
    assert(strContains(rendered, "\"devDependencies\": {\n    \"typescript\": \"5.6.2\"\n  }"));
    assert(strContains(rendered, "\"peerDependencies\": {\n    \"react\": \"^18.2.0\"\n  }"));
});

test("applyManifestMutations Remove clears across all 4 groups and prunes emptied groups cleanly", () => {
    const src =
        "{\n" ++
        "  \"name\": \"prune-test\",\n" ++
        "  \"dependencies\": {\n" ++
        "    \"only-dep\": \"^1.0.0\"\n" ++
        "  },\n" ++
        "  \"optionalDependencies\": {\n" ++
        "    \"fsevents\": \"^2.3.3\"\n" ++
        "  }\n" ++
        "}\n";

    const ops: [3]ManifestMutationOp = [
        ManifestMutationOp{ kind: ManifestMutationKind.Remove, package_name: "only-dep" },
        ManifestMutationOp{ kind: ManifestMutationKind.Remove, package_name: "fsevents" },
        ManifestMutationOp{ kind: ManifestMutationKind.Remove, package_name: "ghost-pkg" },
    ];

    let props: [16]ManifestTopPropSpan = [ManifestTopPropSpan.empty(); 16];
    let deps: [32]ManifestDepEntry = [ManifestDepEntry.empty(); 32];
    let specs: [256]u8 = [0; 256];
    let changes: [8]ManifestChangeRecord = [ManifestChangeRecord.empty(); 8];
    let bufs = ManifestUpdateBuffers{
        top_props: props[0..],
        dep_entries: deps[0..],
        spec_scratch: specs[0..],
        out_changes: changes[0..],
    };
    let out: [1024]u8 = [0; 1024];

    const sum = applyManifestMutations(src, ops[0..], ManifestUpdateOptions.defaults(), &bufs, out[0..]) catch unreachable;
    assertEq(sum.removed_count, @as(u32, 2));
    assertEq(sum.not_found_count, @as(u32, 1));
    assert(strEql(out[0..sum.bytes_written], "{\n  \"name\": \"prune-test\"\n}\n"));
});

test("applyManifestMutations auto-creates missing dependency groups preserving 4-space CRLF format", () => {
    const src = "{\r\n    \"name\": \"fresh-pkg\"\r\n}";
    const ops: [2]ManifestMutationOp = [
        ManifestMutationOp{
            kind: ManifestMutationKind.Add,
            package_name: "vitest",
            version_spec: "2.1.0",
            target_group: ManifestDepGroup.DevDependencies,
            version_style: ManifestVersionStyle.Tilde,
        },
        ManifestMutationOp{
            kind: ManifestMutationKind.Add,
            package_name: "lodash-compat",
            version_spec: "4.17.21",
            target_group: ManifestDepGroup.Dependencies,
            version_style: ManifestVersionStyle.Caret,
            npm_alias_target: "lodash",
        },
    ];

    let props: [16]ManifestTopPropSpan = [ManifestTopPropSpan.empty(); 16];
    let deps: [32]ManifestDepEntry = [ManifestDepEntry.empty(); 32];
    let specs: [256]u8 = [0; 256];
    let changes: [8]ManifestChangeRecord = [ManifestChangeRecord.empty(); 8];
    let bufs = ManifestUpdateBuffers{
        top_props: props[0..],
        dep_entries: deps[0..],
        spec_scratch: specs[0..],
        out_changes: changes[0..],
    };
    let out: [1024]u8 = [0; 1024];

    const sum = applyManifestMutations(src, ops[0..], ManifestUpdateOptions.defaults(), &bufs, out[0..]) catch unreachable;
    const expected =
        "{\r\n" ++
        "    \"name\": \"fresh-pkg\",\r\n" ++
        "    \"dependencies\": {\r\n" ++
        "        \"lodash-compat\": \"npm:lodash@^4.17.21\"\r\n" ++
        "    },\r\n" ++
        "    \"devDependencies\": {\r\n" ++
        "        \"vitest\": \"~2.1.0\"\r\n" ++
        "    }\r\n" ++
        "}";
    assert(strEql(out[0..sum.bytes_written], expected));
});

test("applyManifestMutations guards malformed JSON & invalid inputs, and formatManifestUpdateReport renders summary", () => {
    let props: [8]ManifestTopPropSpan = [ManifestTopPropSpan.empty(); 8];
    let deps: [16]ManifestDepEntry = [ManifestDepEntry.empty(); 16];
    let specs: [128]u8 = [0; 128];
    let changes: [8]ManifestChangeRecord = [ManifestChangeRecord.empty(); 8];
    let bufs = ManifestUpdateBuffers{
        top_props: props[0..],
        dep_entries: deps[0..],
        spec_scratch: specs[0..],
        out_changes: changes[0..],
    };
    let out: [512]u8 = [0; 512];
    const no_ops: [0]ManifestMutationOp = [];

    // 1. Root not object
    let root_err = false;
    * = applyManifestMutations("[1, 2, 3]", no_ops[0..], ManifestUpdateOptions.defaults(), &bufs, out[0..]) catch |e| {
        root_err = (e == ManifestUpdaterError.RootNotObject);
    };
    assert(root_err);

    // 2. Unterminated string
    let str_err = false;
    * = applyManifestMutations("{\"name\": \"unterminated}", no_ops[0..], ManifestUpdateOptions.defaults(), &bufs, out[0..]) catch |e| {
        str_err = (e == ManifestUpdaterError.UnterminatedString);
    };
    assert(str_err);

    // 3. Empty mutation package name
    const bad_ops: [1]ManifestMutationOp = [
        ManifestMutationOp{ kind: ManifestMutationKind.Add, package_name: "", version_spec: "1.0.0" },
    ];
    let empty_err = false;
    * = applyManifestMutations("{\"name\": \"ok\"}", bad_ops[0..], ManifestUpdateOptions.defaults(), &bufs, out[0..]) catch |e| {
        empty_err = (e == ManifestUpdaterError.EmptyMutationName);
    };
    assert(empty_err);

    // 4. Readable terminal diff summary report
    const good_ops: [2]ManifestMutationOp = [
        ManifestMutationOp{
            kind: ManifestMutationKind.Add,
            package_name: "react",
            version_spec: "",
            target_group: ManifestDepGroup.Dependencies,
            version_style: ManifestVersionStyle.CatalogDefault,
        },
        ManifestMutationOp{
            kind: ManifestMutationKind.Remove,
            package_name: "old-lib",
        },
    ];
    const sum = applyManifestMutations(
        "{\n  \"name\": \"app\",\n  \"dependencies\": {\n    \"old-lib\": \"^0.1.0\"\n  }\n}\n",
        good_ops[0..],
        ManifestUpdateOptions.defaults(),
        &bufs,
        out[0..],
    ) catch unreachable;

    let report: [512]u8 = [0; 512];
    const rlen = formatManifestUpdateReport(sum, changes[0..], report[0..]) catch unreachable;
    const text = report[0..rlen];
    assert(strContains(text, "Manifest updated: +1 added, ~0 updated, ->0 moved, -1 removed"));
    assert(strContains(text, "+ react@catalog: [dependencies]"));
    assert(strContains(text, "- old-lib@^0.1.0 [dependencies]"));
});
