import { test, expect, expect_eq } from "cap:test";
import {
    PM_VIEW_MAX_PATH_SEGMENTS,
    PM_VIEW_RECENT_VERSIONS_LIMIT,
    PmViewError,
    PmViewSpec,
    PmPropertyPathSegment,
    PmPropertyPath,
    PmViewDistInfo,
    PmViewKv,
    PmViewMaintainer,
    PmViewSummary,
    formatRecentVersionsHint,
    formatPmViewJsonError,
} from "cap:cli";

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

test("cli pm_view: PmViewSpec parsing, scoped %2F URL encoding, and invalid guards", fn()! {
    expect_eq(PM_VIEW_MAX_PATH_SEGMENTS, 16);
    expect_eq(PM_VIEW_RECENT_VERSIONS_LIMIT, 5);

    const cur1 = PmViewSpec.parse(".")?;
    expect(cur1.is_current_dir);
    expect(strEql(cur1.version, "latest"));

    const cur2 = PmViewSpec.parse("")?;
    expect(cur2.is_current_dir);

    const plain = PmViewSpec.parse("react")?;
    expect(!plain.is_current_dir);
    expect(!plain.isScoped());
    expect(strEql(plain.name, "react"));
    expect(strEql(plain.version, "latest"));

    const scoped = PmViewSpec.parse("@types/react@^18.2.0")?;
    expect(scoped.isScoped());
    expect(strEql(scoped.name, "@types/react"));
    expect(strEql(scoped.version, "^18.2.0"));

    let url_buf: [128]u8 = [0; 128];
    const u1 = plain.buildRegistryUrl("https://registry.npmjs.org///", url_buf[0..])?;
    expect(strEql(url_buf[0..u1], "https://registry.npmjs.org/react"));

    const u2 = scoped.buildRegistryUrl("https://registry.npmjs.org/", url_buf[0..])?;
    expect(strEql(url_buf[0..u2], "https://registry.npmjs.org/@types%2Freact"));

    // Invalid specs rejected
    let rejected: usize = 0;
    if (_ := PmViewSpec.parse("react@")) {} else { rejected += 1; }
    if (_ := PmViewSpec.parse("@scope")) {} else { rejected += 1; }
    if (_ := PmViewSpec.parse("@scope/")) {} else { rejected += 1; }
    if (_ := PmViewSpec.parse("@/pkg")) {} else { rejected += 1; }
    if (_ := cur1.buildRegistryUrl("https://registry.npmjs.org", url_buf[0..])) {} else { rejected += 1; }
    expect_eq(rejected, 5);
});

test("cli pm_view: PmPropertyPath dot and bracket index parsing with invalid guards", fn()! {
    const p1 = PmPropertyPath.parse("repository.url")?;
    expect_eq(p1.count(), 2);
    const s0 = p1.at(0) ?? return err.TestFailed;
    const s1 = p1.at(1) ?? return err.TestFailed;
    expect(strEql(s0.key, "repository"));
    expect(!s0.isIndex());
    expect(strEql(s1.key, "url"));
    expect(p1.at(2) == null);

    const p2 = PmPropertyPath.parse("maintainers[0].email")?;
    expect_eq(p2.count(), 3);
    const m0 = p2.at(0) ?? return err.TestFailed;
    const m1 = p2.at(1) ?? return err.TestFailed;
    const m2 = p2.at(2) ?? return err.TestFailed;
    expect(strEql(m0.key, "maintainers"));
    expect(m1.isIndex());
    expect_eq(m1.index ?? 99, 0);
    expect(strEql(m2.key, "email"));

    const p3 = PmPropertyPath.parse("keywords.2")?;
    expect_eq(p3.count(), 2);
    const k1 = p3.at(1) ?? return err.TestFailed;
    expect(k1.isIndex());
    expect_eq(k1.index ?? 99, 2);

    const p4 = PmPropertyPath.parse("matrix[1][12]")?;
    expect_eq(p4.count(), 3);
    expect_eq((p4.at(2) ?? return err.TestFailed).index ?? 0, 12);

    // Invalid paths rejected
    let rejected: usize = 0;
    if (_ := PmPropertyPath.parse("")) {} else { rejected += 1; }
    if (_ := PmPropertyPath.parse(".leading")) {} else { rejected += 1; }
    if (_ := PmPropertyPath.parse("trailing.")) {} else { rejected += 1; }
    if (_ := PmPropertyPath.parse("a..b")) {} else { rejected += 1; }
    if (_ := PmPropertyPath.parse("a[]")) {} else { rejected += 1; }
    if (_ := PmPropertyPath.parse("a[abc]")) {} else { rejected += 1; }
    if (_ := PmPropertyPath.parse("a[0")) {} else { rejected += 1; }
    if (_ := PmPropertyPath.parse("a]")) {} else { rejected += 1; }
    expect_eq(rejected, 8);
});

test("cli pm_view: PmViewSummary full card formatting with human sizes and optional sections", fn()! {
    const kws: [2][]const u8 = ["react", "ui"];
    const deps: [2]PmViewKv = [
        PmViewKv{ key: "loose-envify", value: "^1.1.0" },
        PmViewKv{ key: "scheduler", value: "^0.23.0" },
    ];
    const tags: [2]PmViewKv = [
        PmViewKv{ key: "latest", value: "18.2.0" },
        PmViewKv{ key: "next", value: "19.0.0-rc.1" },
    ];
    const maints: [2]PmViewMaintainer = [
        PmViewMaintainer{ name: "fb", email: "opensource+npm@fb.com" },
        PmViewMaintainer{ name: "gaearon", email: "" },
    ];

    const summary = PmViewSummary{
        name: "react",
        version: "18.2.0",
        license: "MIT",
        description: "React is a JavaScript library for building user interfaces.",
        homepage: "https://reactjs.org/",
        versions_count: 420,
        keywords: kws[0..],
        dependencies: deps[0..],
        dist: PmViewDistInfo{
            tarball: "https://registry.npmjs.org/react/-/react-18.2.0.tgz",
            shasum: "555bd98592883255fa00de14f1151a917b5d77d5",
            integrity: "sha512-/3IjMdb2L9QbBdWiW5e3P2/npwMBaU9mHCSCUzNln0ZCYbcfTsGbTJrU/kGemdH2IWmB2ioZ+zkxtmq6g09fGQ==",
            unpacked_size: 316416,
        },
        dist_tags: tags[0..],
        maintainers: maints[0..],
        published_at: "2022-06-14T19:50:19.086Z",
    };

    let out: [1024]u8 = [0; 1024];
    const n = summary.formatCard(out[0..])?;
    const text = out[0..n];

    expect(containsSubstr(text, "react@18.2.0 | MIT | deps: 2 | versions: 420\n"));
    expect(containsSubstr(text, "keywords: react, ui\n"));
    expect(containsSubstr(text, "dependencies (2):\n- loose-envify: ^1.1.0\n- scheduler: ^0.23.0\n"));
    expect(containsSubstr(text, " .unpackedSize: 309.0 KB\n"));
    expect(containsSubstr(text, "dist-tags:\nlatest: 18.2.0\nnext: 19.0.0-rc.1\n"));
    expect(containsSubstr(text, "maintainers:\n- fb <opensource+npm@fb.com>\n- gaearon\n"));
    expect(containsSubstr(text, "Published: 2022-06-14T19:50:19.086Z\n"));
});

test("cli pm_view: formatRecentVersionsHint caps at 5 and reports remaining count", fn()! {
    const small: [3][]const u8 = ["1.0.0", "1.1.0", "1.2.0"];
    let buf: [256]u8 = [0; 256];
    const n1 = formatRecentVersionsHint(small[0..], buf[0..])?;
    expect(strEql(buf[0..n1], "Recent versions:\n- 1.0.0\n- 1.1.0\n- 1.2.0\n"));

    const many: [7][]const u8 = ["0.1.0", "0.2.0", "1.0.0", "1.1.0", "1.2.0", "1.3.0", "2.0.0"];
    const n2 = formatRecentVersionsHint(many[0..], buf[0..])?;
    const expected =
        "Recent versions:\n" ++
        "- 1.0.0\n" ++
        "- 1.1.0\n" ++
        "- 1.2.0\n" ++
        "- 1.3.0\n" ++
        "- 2.0.0\n" ++
        "  ... and 2 more\n";
    expect(strEql(buf[0..n2], expected));

    const empty: [0][]const u8 = [];
    const n3 = formatRecentVersionsHint(empty[0..], buf[0..])?;
    expect_eq(n3, 0);
});

test("cli pm_view: formatPmViewJsonError escapes JSON strings cleanly", fn()! {
    let buf: [256]u8 = [0; 256];
    const n1 = formatPmViewJsonError("No matching version found", "react@99.0.0", null, buf[0..])?;
    expect(strEql(buf[0..n1], "{ \"error\": \"No matching version found\", \"version\": \"react@99.0.0\" }\n"));

    const n2 = formatPmViewJsonError("Property not found", "react@18.2.0", "bad\"key\\x\n", buf[0..])?;
    expect(strEql(
        buf[0..n2],
        "{ \"error\": \"Property not found\", \"version\": \"react@18.2.0\", \"property\": \"bad\\\"key\\\\x\\n\" }\n",
    ));
});

test("cli pm_view: BufferTooSmall guards across all formatters", fn()! {
    const spec = PmViewSpec.parse("@types/node@20.0.0")?;
    const summary = PmViewSummary{ name: "pkg", version: "1.0.0", license: "MIT", versions_count: 1 };
    const vers: [2][]const u8 = ["1.0.0", "2.0.0"];

    let tiny: [8]u8 = [0; 8];
    let errs: usize = 0;
    if (_ := spec.buildRegistryUrl("https://registry.npmjs.org", tiny[0..])) {} else { errs += 1; }
    if (_ := summary.formatCard(tiny[0..])) {} else { errs += 1; }
    if (_ := formatRecentVersionsHint(vers[0..], tiny[0..])) {} else { errs += 1; }
    if (_ := formatPmViewJsonError("err", "pkg@1", null, tiny[0..])) {} else { errs += 1; }
    expect_eq(errs, 4);
});
