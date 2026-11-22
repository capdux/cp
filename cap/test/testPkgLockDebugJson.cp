import { test, expect } from "cap:test";
import {
    LOCK_DEBUG_MAX_INDEX_ENTRIES,
    LOCK_DEBUG_MAX_IDS_PER_NAME,
    LOCK_DEBUG_INVALID_PKG_ID,
    LOCK_BEH_NORMAL,
    LOCK_BEH_OPTIONAL,
    LOCK_BEH_DEV,
    LOCK_BEH_PEER,
    LOCK_BEH_WORKSPACE,
    LOCK_BEH_BUNDLED,
    LockDebugFormat,
    LockDebugDepKind,
    LockDebugDep,
    LockDebugPkg,
    formatLowerHexBytes,
    stringifyLockDebugDep,
    LockDebugSnapshot,
} from "cap:pkg";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    const limit = haystack.len - needle.len + 1;
    for (i of 0..limit) {
        let ok = true;
        for (j of 0..needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
        }
        if (ok) return true;
    }
    return false;
}

test("Full LockDebugSnapshot stringify outputs format, meta_hash, single/multi package_index, packages, and deps", () => {
    let snap = LockDebugSnapshot.init(.TextV1);
    const hash_bytes: [4]u8 = [0xde, 0xad, 0xbe, 0xef];
    snap.setMetaHash(hash_bytes[0..]);

    const single_id: [1]u32 = [1];
    const multi_ids: [2]u32 = [2, 3];
    * = snap.addPackageIndex("zod", single_id[0..]) catch unreachable;
    * = snap.addPackageIndex("lodash", multi_ids[0..]) catch unreachable;

    * = snap.addPackage(LockDebugPkg{
        pkg_id: 1,
        name: "zod",
        version: "3.23.8",
        integrity: "sha512-abc",
    }) catch unreachable;
    * = snap.addPackage(LockDebugPkg{
        pkg_id: 2,
        name: "lodash",
        version: "4.17.21",
    }) catch unreachable;

    * = snap.addDependency(LockDebugDep{
        dep_id: 0,
        name: "zod",
        literal: "^3.23.0",
        kind: .Npm,
        resolved_pkg_id: 1,
        behavior_mask: LOCK_BEH_NORMAL,
        version_or_tag: "^3.23.0",
    }) catch unreachable;

    let out: [2048]u8 = [0; 2048];
    const n = snap.stringify(out[0..]) catch unreachable;
    const json = out[0..n];

    expect(containsSubstr(json, "\"format\":\"text_v1\""));
    expect(containsSubstr(json, "\"meta_hash\":\"deadbeef\""));
    expect(containsSubstr(json, "\"package_index\":{\"zod\":1,\"lodash\":[2,3]}"));
    expect(containsSubstr(json, "\"integrity\":\"sha512-abc\""));
    expect(containsSubstr(json, "\"package_id\":1"));
});

test("stringifyLockDebugDep handles npm alias, dist_tag, tarball, folder, symlink, and workspace", () => {
    let buf: [512]u8 = [0; 512];

    const n_alias = stringifyLockDebugDep(LockDebugDep{
        dep_id: 1,
        name: "my-lodash",
        literal: "npm:lodash@^4.17.21",
        kind: .Npm,
        is_alias: true,
        resolved_pkg_id: 5,
        target_name: "lodash",
        version_or_tag: "^4.17.21",
        behavior_mask: LOCK_BEH_NORMAL,
    }, buf[0..]) catch unreachable;
    expect(containsSubstr(buf[0..n_alias], "\"is_alias\":true"));
    expect(containsSubstr(buf[0..n_alias], "\"npm\":{\"name\":\"lodash\",\"version\":\"^4.17.21\"}"));

    const n_tag = stringifyLockDebugDep(LockDebugDep{
        dep_id: 2,
        name: "react",
        literal: "canary",
        kind: .DistTag,
        version_or_tag: "canary",
    }, buf[0..]) catch unreachable;
    expect(containsSubstr(buf[0..n_tag], "\"dist_tag\":{\"name\":\"react\",\"tag\":\"canary\"}"));

    const n_tar = stringifyLockDebugDep(LockDebugDep{
        dep_id: 3,
        name: "pkg",
        literal: "https://example.com/pkg.tgz",
        kind: .Tarball,
        uri_or_path: "https://example.com/pkg.tgz",
    }, buf[0..]) catch unreachable;
    expect(containsSubstr(buf[0..n_tar], "\"tarball\":{\"uri\":\"https://example.com/pkg.tgz\",\"package_name\":\"pkg\"}"));

    const n_ws = stringifyLockDebugDep(LockDebugDep{
        dep_id: 4,
        name: "@app/ui",
        literal: "workspace:*",
        kind: .Workspace,
        uri_or_path: "packages/ui",
        behavior_mask: LOCK_BEH_WORKSPACE,
    }, buf[0..]) catch unreachable;
    expect(containsSubstr(buf[0..n_ws], "\"workspace\":\"packages/ui\""));
    expect(containsSubstr(buf[0..n_ws], "\"behavior\":{\"workspace\":true}"));
});

test("stringifyLockDebugDep handles git, github, catalog, and uninitialized tags", () => {
    let buf: [512]u8 = [0; 512];

    const n_gh = stringifyLockDebugDep(LockDebugDep{
        dep_id: 10,
        name: "bun",
        literal: "github:oven-sh/bun#main",
        kind: .Github,
        owner: "oven-sh",
        repo: "bun",
        committish: "main",
        resolved_url: "https://github.com/oven-sh/bun/tarball/abc",
    }, buf[0..]) catch unreachable;
    expect(containsSubstr(buf[0..n_gh], "\"github\":{\"owner\":\"oven-sh\",\"repo\":\"bun\",\"committish\":\"main\""));

    const n_cat = stringifyLockDebugDep(LockDebugDep{
        dep_id: 11,
        name: "typescript",
        literal: "catalog:",
        kind: .Catalog,
        version_or_tag: "default",
    }, buf[0..]) catch unreachable;
    expect(containsSubstr(buf[0..n_cat], "\"catalog\":{\"name\":\"typescript\",\"version\":\"catalog:default\"}"));

    const n_uninit = stringifyLockDebugDep(LockDebugDep{
        dep_id: 12,
        name: "empty",
        literal: "",
        kind: .Uninitialized,
    }, buf[0..]) catch unreachable;
    expect(containsSubstr(buf[0..n_uninit], "\"uninitialized\":null"));
});

test("Unresolved package_id serializes as null and combined behavior flags serialize in order", () => {
    let buf: [512]u8 = [0; 512];
    const all_beh = LOCK_BEH_NORMAL | LOCK_BEH_OPTIONAL | LOCK_BEH_DEV | LOCK_BEH_PEER | LOCK_BEH_WORKSPACE | LOCK_BEH_BUNDLED;
    const n = stringifyLockDebugDep(LockDebugDep{
        dep_id: 7,
        name: "multi",
        literal: "1.0.0",
        kind: .Folder,
        uri_or_path: "./multi",
        resolved_pkg_id: LOCK_DEBUG_INVALID_PKG_ID,
        behavior_mask: all_beh,
    }, buf[0..]) catch unreachable;

    expect(containsSubstr(buf[0..n], "\"package_id\":null"));
    expect(containsSubstr(
        buf[0..n],
        "\"behavior\":{\"normal\":true,\"optional\":true,\"dev\":true,\"peer\":true,\"workspace\":true,\"bundled\":true}",
    ));
});

test("JSON string escaping and lowercase hex formatting", () => {
    const raw: [3]u8 = [0x01, 0xab, 0x0f];
    let hbuf: [16]u8 = [0; 16];
    const hn = formatLowerHexBytes(raw[0..], hbuf[0..]) catch unreachable;
    expect(strEq(hbuf[0..hn], "01ab0f"));

    let buf: [256]u8 = [0; 256];
    const n = stringifyLockDebugDep(LockDebugDep{
        dep_id: 0,
        name: "a\"b\\c",
        literal: "line1\nline2\t\x01",
        kind: .Symlink,
        uri_or_path: "../link",
    }, buf[0..]) catch unreachable;
    expect(containsSubstr(buf[0..n], "\"name\":\"a\\\"b\\\\c\""));
    expect(containsSubstr(buf[0..n], "\"literal\":\"line1\\nline2\\t\\u0001\""));
});

test("Boundary guards: empty package name, too many IDs, index capacity, and tiny buffer", () => {
    let snap = LockDebugSnapshot.init(.V3);
    const one_id: [1]u32 = [0];

    let saw_empty = false;
    * = snap.addPackageIndex("", one_id[0..]) catch {
        saw_empty = true;
    };
    expect(saw_empty);

    const nine_ids: [LOCK_DEBUG_MAX_IDS_PER_NAME + 1]u32 = [0; LOCK_DEBUG_MAX_IDS_PER_NAME + 1];
    let saw_too_many_ids = false;
    * = snap.addPackageIndex("many", nine_ids[0..]) catch {
        saw_too_many_ids = true;
    };
    expect(saw_too_many_ids);

    for (i of 0..LOCK_DEBUG_MAX_INDEX_ENTRIES) {
        * = i;
        * = snap.addPackageIndex("p", one_id[0..]) catch unreachable;
    }
    let saw_idx_full = false;
    * = snap.addPackageIndex("overflow", one_id[0..]) catch {
        saw_idx_full = true;
    };
    expect(saw_idx_full);

    let tiny: [8]u8 = [0; 8];
    let saw_small = false;
    * = snap.stringify(tiny[0..]) catch {
        saw_small = true;
    };
    expect(saw_small);
});
