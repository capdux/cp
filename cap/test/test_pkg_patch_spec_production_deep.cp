// cap/test/test_pkg_patch_spec_production_deep.cp
// Production-deep regression tests for cap:pkg patch_spec module
// Source: /data/dev/bun/src/install/PackageManager/patchPackage.cp & patch_install.cp & docs/pm/cli/patch.mdx

import { test, expect } from "cap:test";
import {
    PATCH_DEFAULT_DIR,
    PATCH_CACHE_HASH_TAG,
    PATCH_HEX_HASH_LEN,
    PatchSpecError,
    PatchTargetKind,
    PatchTargetArg,
    PatchCommitSpec,
    escapePatchFilenameSegment,
    formatPatchFilename,
    normalizePatchesDir,
    computePatchContentHash,
    formatPatchCacheSubpath,
    parsePatchCacheHash,
    validateUnifiedPatchText,
} from "cap:pkg";

fn strEq(a: []const u8, b: []const u8): bool {
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

test("pkg patch_spec: PatchTargetArg parsing (name, name@ver, scoped, nested node_modules)") {
    const a1 = PatchTargetArg.parse("react") catch { expect(false); return; };
    expect(a1.kind == PatchTargetKind.PackageSpec);
    expect(!a1.isPath());
    expect(!a1.isScoped());
    expect(strEq(a1.package_name, "react"));
    expect(a1.version == null);

    const a2 = PatchTargetArg.parse("react@17.0.2") catch { expect(false); return; };
    expect(strEq(a2.package_name, "react"));
    if (v := a2.version) { expect(strEq(v, "17.0.2")); } else { expect(false); }

    const a3 = PatchTargetArg.parse("@acme/ui@2.1.0-rc.1") catch { expect(false); return; };
    expect(a3.isScoped());
    expect(strEq(a3.package_name, "@acme/ui"));
    if (v := a3.version) { expect(strEq(v, "2.1.0-rc.1")); } else { expect(false); }

    const a4 = PatchTargetArg.parse("./node_modules/react/") catch { expect(false); return; };
    expect(a4.isPath());
    expect(strEq(a4.package_name, "react"));

    const a5 = PatchTargetArg.parse("node_modules/foo/node_modules/@scope/bar") catch { expect(false); return; };
    expect(a5.isPath());
    expect(a5.isScoped());
    expect(strEq(a5.package_name, "@scope/bar"));
}

test("pkg patch_spec: PatchCommitSpec key & filename formatting + percent escaping") {
    const spec = PatchCommitSpec{
        package_name: "@acme/ui",
        version: "2.1.0",
        patches_dir: "./mypatches///",
    };

    let key_buf: [64]u8 = [0; 64];
    const kn = spec.formatPatchKey(key_buf[0..]) catch { expect(false); return; };
    expect(strEq(key_buf[0..kn], "@acme/ui@2.1.0"));

    let path_buf: [128]u8 = [0; 128];
    const pn = spec.formatPatchFilePath(path_buf[0..]) catch { expect(false); return; };
    expect(strEq(path_buf[0..pn], "mypatches/@acme+ui@2.1.0.patch"));

    let fn_buf: [64]u8 = [0; 64];
    const fnn = formatPatchFilename("lodash", "4.17.21", fn_buf[0..]) catch { expect(false); return; };
    expect(strEq(fn_buf[0..fnn], "lodash@4.17.21.patch"));

    // EscapeVal percent encoding (`/` -> %2F, `\` -> %5c, ` ` -> %20, `\n` -> %0A, `\r` -> %0D, `\t` -> %09)
    let esc_buf: [64]u8 = [0; 64];
    const en = escapePatchFilenameSegment("a/b\\c d\n\r\t", esc_buf[0..]) catch { expect(false); return; };
    expect(strEq(esc_buf[0..en], "a%2Fb%5cc%20d%0A%0D%09"));
}

test("pkg patch_spec: Global Cache __buntag_<16hex> hash round-trip & collision sensitivity") {
    expect(strEq(PATCH_DEFAULT_DIR, "patches"));
    expect(strEq(PATCH_CACHE_HASH_TAG, "__buntag_"));
    expect(PATCH_HEX_HASH_LEN == 16);

    const p1 = "--- a/index.js\n+++ b/index.js\n@@ -1 +1 @@\n-const x = 1;\n+const x = 2;\n";
    const p2 = "--- a/index.js\n+++ b/index.js\n@@ -1 +1 @@\n-const x = 1;\n+const x = 3;\n";

    const h1 = computePatchContentHash(p1);
    const h2 = computePatchContentHash(p2);
    expect(h1 != h2);
    expect(h1 == computePatchContentHash(p1));

    let cache_buf: [128]u8 = [0; 128];
    const cn = formatPatchCacheSubpath("react@18.3.1", h1, cache_buf[0..]) catch { expect(false); return; };
    expect(cn == "react@18.3.1".len + PATCH_CACHE_HASH_TAG.len + PATCH_HEX_HASH_LEN);

    const decoded = parsePatchCacheHash(cache_buf[0..cn]) catch { expect(false); return; };
    expect(decoded == h1);

    // Invalid cache tags rejected
    let bad_tag1 = false;
    * = parsePatchCacheHash("react@18.3.1") catch |e| {
        expect(e == PatchSpecError.InvalidCacheTag);
        bad_tag1 = true;
    };
    expect(bad_tag1);

    let bad_tag2 = false;
    * = parsePatchCacheHash("react__buntag_zzzzzzzzzzzzzzzz") catch |e| {
        expect(e == PatchSpecError.InvalidCacheTag);
        bad_tag2 = true;
    };
    expect(bad_tag2);
}

test("pkg patch_spec: patches_dir normalization & path traversal defense") {
    const d1 = normalizePatchesDir("patches") catch { expect(false); return; };
    expect(strEq(d1, "patches"));

    const d2 = normalizePatchesDir("./.patches/sub/") catch { expect(false); return; };
    expect(strEq(d2, ".patches/sub"));

    let trav1 = false;
    * = normalizePatchesDir("../outside") catch |e| {
        expect(e == PatchSpecError.PathTraversalDetected);
        trav1 = true;
    };
    expect(trav1);

    let trav2 = false;
    * = normalizePatchesDir("patches/../../etc") catch |e| {
        expect(e == PatchSpecError.PathTraversalDetected);
        trav2 = true;
    };
    expect(trav2);

    let trav3 = false;
    * = normalizePatchesDir("/etc/patches") catch |e| {
        expect(e == PatchSpecError.PathTraversalDetected);
        trav3 = true;
    };
    expect(trav3);

    let empty_dir = false;
    * = normalizePatchesDir("   ") catch |e| {
        expect(e == PatchSpecError.InvalidPatchesDir);
        empty_dir = true;
    };
    expect(empty_dir);
}

test("pkg patch_spec: unified diff text sanity validation") {
    const good_patch =
        "diff --git a/lib/foo.js b/lib/foo.js\n" ++
        "--- a/lib/foo.js\n" ++
        "+++ b/lib/foo.js\n" ++
        "@@ -1,2 +1,2 @@\n" ++
        "-export const ok = false;\n" ++
        "+export const ok = true;\n";
    validateUnifiedPatchText(good_patch) catch { expect(false); return; };

    let empty_err = false;
    validateUnifiedPatchText("  \n\t  ") catch |e| {
        expect(e == PatchSpecError.EmptyPatchContent);
        empty_err = true;
    };
    expect(empty_err);

    let malformed_err = false;
    validateUnifiedPatchText("--- a/file.js\n+++ b/file.js\nno hunk header here\n") catch |e| {
        expect(e == PatchSpecError.MalformedUnifiedDiff);
        malformed_err = true;
    };
    expect(malformed_err);
}

test("pkg patch_spec: malformed args & output buffer overflow guards") {
    let e_empty = false;
    * = PatchTargetArg.parse("   ") catch |e| {
        expect(e == PatchSpecError.EmptyArgument);
        e_empty = true;
    };
    expect(e_empty);

    let e_bad_scope = false;
    * = PatchTargetArg.parse("@broken-scope") catch |e| {
        expect(e == PatchSpecError.InvalidPackageName);
        e_bad_scope = true;
    };
    expect(e_bad_scope);

    let e_empty_ver = false;
    * = PatchTargetArg.parse("react@") catch |e| {
        expect(e == PatchSpecError.InvalidVersion);
        e_empty_ver = true;
    };
    expect(e_empty_ver);

    let tiny: [6]u8 = [0; 6];
    let e_small = false;
    * = formatPatchCacheSubpath("react@18.3.1", 0x1234, tiny[0..]) catch |e| {
        expect(e == PatchSpecError.BufferTooSmall);
        e_small = true;
    };
    expect(e_small);
}
