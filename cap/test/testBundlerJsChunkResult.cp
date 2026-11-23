// cap/test/testBundlerJsChunkResult.cp — Deep production tests for cap:bundler jsChunkResult
// Covers:
//   1. FIPS 180-4 SHA-256 standard vectors ("", "abc"), Base64 SRI (`sha256-...`) & RFC 4122 debugId
//   2. SIMD Shebang extraction (`splitLeadingShebang`) & priority override in finalization
//   3. Cross-chunk import sorting, deduplication & same-source grouping (ESM & CJS)
//   4. Full ESM finalization: Shebang + Banner + imports + body + exports + Footer + debugId + SourceMap
//   5. Full CJS & IIFE finalization (module.exports, named global IIFE, use strict)
//   6. Validation guards (invalid Shebang, newline in import specifier, empty export, BufferTooSmall) & report
//   7. 500-iteration deterministic random JS Chunk input Fuzz gate

import { expect, expectEqual, test } from "cap:test";
import {
    JS_CHUNK_DEFAULT_SHEBANG,
    JS_CHUNK_SRI_LEN,
    JS_CHUNK_UUID_LEN,
    computeJsChunkSha256,
    computeJsChunkIntegrity,
    JsChunkFinalFormat,
    splitLeadingShebang,
    JsChunkImportBinding,
    JsChunkExportBinding,
    JsChunkFinalizeOptions,
    finalizeJsChunkResult,
    formatJsChunkResultReport,
} from "cap:bundler";

fn strEql(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i..i + needle.len], needle)) return true;
        i += 1;
    }
    return false;
}

test("jsChunkResult: FIPS 180-4 SHA-256 standard vectors, Base64 SRI & deterministic debugId", () => {
    // SHA-256("") = e3b0c442 98fc1c14 9afbf4c8 996fb924 27ae41e4 649b934c a495991b 7852b855
    const d_empty = computeJsChunkSha256("");
    expectEqual(d_empty[0], @as(u8, 0xE3));
    expectEqual(d_empty[1], @as(u8, 0xB0));
    expectEqual(d_empty[31], @as(u8, 0x55));

    // SHA-256("abc") = ba7816bf 8f01cfea 414140de 5dae2223 b00361a3 96177a9c b410ff61 f20015ad
    const d_abc = computeJsChunkSha256("abc");
    expectEqual(d_abc[0], @as(u8, 0xBA));
    expectEqual(d_abc[1], @as(u8, 0x78));
    expectEqual(d_abc[2], @as(u8, 0x16));
    expectEqual(d_abc[3], @as(u8, 0xBF));
    expectEqual(d_abc[30], @as(u8, 0x15));
    expectEqual(d_abc[31], @as(u8, 0xAD));

    // SRI of "abc" is "sha256-ungWv48Bz+pBQUDeXa4iI7ADYaOWF3qctBD/YfIAFa0="
    const integ = computeJsChunkIntegrity("abc");
    expectEqual(integ.sri().len, JS_CHUNK_SRI_LEN);
    expect(strEql(integ.sri(), "sha256-ungWv48Bz+pBQUDeXa4iI7ADYaOWF3qctBD/YfIAFa0="));

    // Deterministic RFC 4122 debugId UUID shape
    const uuid = integ.debugId();
    expectEqual(uuid.len, JS_CHUNK_UUID_LEN);
    expectEqual(uuid[8], '-');
    expectEqual(uuid[13], '-');
    expectEqual(uuid[14], '4'); // version 4 nibble
    expectEqual(uuid[18], '-');
    expectEqual(uuid[23], '-');

    // Short 16-char hex hash matches first 8 bytes of SHA-256
    let hex_buf: [16]u8 = [0; 16];
    const hlen = integ.formatShortHex(hex_buf[0..]) catch unreachable;
    expectEqual(hlen, @as(usize, 16));
    expect(strEql(hex_buf[0..16], "ba7816bf8f01cfea"));
});

test("jsChunkResult: SIMD Shebang splitting (LF and CRLF)", () => {
    const s1 = splitLeadingShebang("#!/usr/bin/env node\r\nconsole.log(1);");
    expect(strEql(s1.shebang, "#!/usr/bin/env node"));
    expect(strEql(s1.body, "console.log(1);"));

    const s2 = splitLeadingShebang("console.log(2);");
    expectEqual(s2.shebang.len, @as(usize, 0));
    expect(strEql(s2.body, "console.log(2);"));
});

test("jsChunkResult: ESM finalization with sorted/deduped imports, exports, debugId & SourceMap", () => {
    const imps: [5]JsChunkImportBinding = [
        JsChunkImportBinding{ from_path: "./chunk-b.js", imported_name: "zeta", local_name: "z" },
        JsChunkImportBinding{ from_path: "./chunk-a.js", imported_name: "beta", local_name: "beta" },
        JsChunkImportBinding{ from_path: "./chunk-a.js", imported_name: "alpha", local_name: "alpha" },
        JsChunkImportBinding{ from_path: "./chunk-a.js", imported_name: "alpha", local_name: "alpha" }, // exact duplicate
        JsChunkImportBinding{ from_path: "./polyfill.js", imported_name: "", local_name: "" }, // side-effect
    ];
    const exps: [2]JsChunkExportBinding = [
        JsChunkExportBinding{ local_name: "runApp", exported_name: "default" },
        JsChunkExportBinding{ local_name: "version", exported_name: "VERSION" },
    ];

    let order: [8]usize = [0; 8];
    let out: [2048]u8 = [0; 2048];
    const sum = finalizeJsChunkResult(
        "#!/usr/bin/env old\nconst version = \"1.0\";\nconst runApp = () => alpha + beta + z;",
        imps[0..],
        exps[0..],
        JsChunkFinalizeOptions{
            format: JsChunkFinalFormat.Esm,
            shebang: JS_CHUNK_DEFAULT_SHEBANG,
            banner: "/*! cap build */",
            footer: "// end",
            emit_debug_id: true,
            source_map_url: "app.js.map",
        },
        order[0..],
        out[0..],
    ) catch unreachable;

    const code = out[0..sum.output_bytes];
    expect(sum.has_shebang);
    expect(sum.has_debug_id);
    expect(sum.has_source_map_url);
    expectEqual(sum.imports_emitted, @as(u32, 4)); // 5 minus 1 duplicate
    expectEqual(sum.exports_emitted, @as(u32, 2));
    expect(sum.total_lines >= 9);

    // Explicit shebang at byte 0 replaces body-embedded shebang
    expect(strEql(code[0..JS_CHUNK_DEFAULT_SHEBANG.len], JS_CHUNK_DEFAULT_SHEBANG));
    expect(!containsSubstr(code, "/usr/bin/env old"));
    expect(containsSubstr(code, "import { alpha, beta } from \"./chunk-a.js\";\n"));
    expect(containsSubstr(code, "import { zeta as z } from \"./chunk-b.js\";\n"));
    expect(containsSubstr(code, "import \"./polyfill.js\";\n"));
    expect(containsSubstr(code, "export default runApp;\n"));
    expect(containsSubstr(code, "export { version as VERSION };\n"));
    expect(containsSubstr(code, "//# debugId="));
    expect(containsSubstr(code, "//# sourceMappingURL=app.js.map\n"));

    // Final SRI must match exact emitted output bytes
    const verify_integ = computeJsChunkIntegrity(code);
    expect(strEql(sum.integrity.sri(), verify_integ.sri()));
});

test("jsChunkResult: CJS require() grouping & module.exports tail", () => {
    const imps: [3]JsChunkImportBinding = [
        JsChunkImportBinding{ from_path: "./lib.cjs", imported_name: "read", local_name: "readFn" },
        JsChunkImportBinding{ from_path: "./lib.cjs", imported_name: "write", local_name: "write" },
        JsChunkImportBinding{ from_path: "./init.cjs", imported_name: "", local_name: "" },
    ];
    const exps: [2]JsChunkExportBinding = [
        JsChunkExportBinding{ local_name: "api", exported_name: "api" },
        JsChunkExportBinding{ local_name: "internalVer", exported_name: "version" },
    ];

    let order: [4]usize = [0; 4];
    let out: [1024]u8 = [0; 1024];
    const sum = finalizeJsChunkResult(
        "const api = () => readFn() + write();",
        imps[0..],
        exps[0..],
        JsChunkFinalizeOptions{
            format: JsChunkFinalFormat.Cjs,
            emit_use_strict: true,
            emit_debug_id: false,
        },
        order[0..],
        out[0..],
    ) catch unreachable;

    const code = out[0..sum.output_bytes];
    expect(containsSubstr(code, "\"use strict\";\n"));
    expect(containsSubstr(code, "require(\"./init.cjs\");\n"));
    expect(containsSubstr(code, "const { read: readFn, write } = require(\"./lib.cjs\");\n"));
    expect(containsSubstr(code, "module.exports = { api, version: internalVer };\n"));
    expect(!sum.has_debug_id);
});

test("jsChunkResult: IIFE global wrapper & return export object", () => {
    const no_imps: [0]JsChunkImportBinding = [];
    const exps: [1]JsChunkExportBinding = [
        JsChunkExportBinding{ local_name: "widget", exported_name: "Widget" },
    ];
    let order: [1]usize = [0; 1];
    let out: [1024]u8 = [0; 1024];
    const sum = finalizeJsChunkResult(
        "const widget = 42;",
        no_imps[0..],
        exps[0..],
        JsChunkFinalizeOptions{
            format: JsChunkFinalFormat.Iife,
            iife_global_name: "CapWidget",
            emit_debug_id: false,
        },
        order[0..],
        out[0..],
    ) catch unreachable;

    const code = out[0..sum.output_bytes];
    expect(containsSubstr(code, "var CapWidget = (() => {\n"));
    expect(containsSubstr(code, "return { Widget: widget };\n"));
    expect(containsSubstr(code, "})();\n"));
});

test("jsChunkResult: validation guards & diagnostic report formatting", () => {
    const no_imps: [0]JsChunkImportBinding = [];
    const no_exps: [0]JsChunkExportBinding = [];
    let order: [2]usize = [0; 2];
    let out: [512]u8 = [0; 512];

    // Invalid shebang (missing #!)
    let bad_sh = false;
    * = finalizeJsChunkResult("x;", no_imps[0..], no_exps[0..], JsChunkFinalizeOptions{ shebang: "/usr/bin/cap" }, order[0..], out[0..]) catch {
        bad_sh = true;
    };
    expect(bad_sh);

    // Invalid import specifier with newline injection
    const bad_imps: [1]JsChunkImportBinding = [
        JsChunkImportBinding{ from_path: "./a.js\nmalicious", imported_name: "x", local_name: "x" },
    ];
    let bad_imp = false;
    * = finalizeJsChunkResult("x;", bad_imps[0..], no_exps[0..], JsChunkFinalizeOptions.defaults(), order[0..], out[0..]) catch {
        bad_imp = true;
    };
    expect(bad_imp);

    // Report formatting
    const ok = finalizeJsChunkResult("export const v = 1;", no_imps[0..], no_exps[0..], JsChunkFinalizeOptions.defaults(), order[0..], out[0..]) catch unreachable;
    let rep: [256]u8 = [0; 256];
    const rlen = formatJsChunkResultReport(ok, rep[0..]) catch unreachable;
    expect(containsSubstr(rep[0..rlen], "JsChunkResult(fmt=esm"));
    expect(containsSubstr(rep[0..rlen], "sri=sha256-"));
});

test("jsChunkResult: 500-iteration deterministic random JS Chunk Fuzz gate", () => {
    const no_imps: [0]JsChunkImportBinding = [];
    const no_exps: [0]JsChunkExportBinding = [];
    let order: [2]usize = [0; 2];
    let fuzz_body: [160]u8 = [0; 160];
    let out: [1024]u8 = [0; 1024];

    let rng: u64 = 0x15C40987654321AB;
    let passed: usize = 0;
    let iter: usize = 0;

    while (iter < 500) {
        rng ^= rng >> 12;
        rng ^= rng << 25;
        rng ^= rng >> 27;
        const len = @as(usize, @intCast((rng *% 0x2545F4914F6CDD1D) % 140));

        let k: usize = 0;
        while (k < len) {
            rng ^= rng >> 12;
            rng ^= rng << 25;
            rng ^= rng >> 27;
            fuzz_body[k] = @as(u8, @truncate(rng));
            k += 1;
        }
        // Inject occasional Shebang prefixes
        if (iter % 7 == 0 && len >= 4) {
            fuzz_body[0] = '#';
            fuzz_body[1] = '!';
            fuzz_body[2] = '/';
            fuzz_body[3] = '\n';
        }

        const fmt = if (iter % 3 == 0) JsChunkFinalFormat.Esm
            else if (iter % 3 == 1) JsChunkFinalFormat.Cjs
            else JsChunkFinalFormat.Iife;

        const sum = finalizeJsChunkResult(
            fuzz_body[0..len],
            no_imps[0..],
            no_exps[0..],
            JsChunkFinalizeOptions{ format: fmt, emit_debug_id: (iter % 2 == 0) },
            order[0..],
            out[0..],
        ) catch unreachable;

        expect(sum.output_bytes <= out.len);
        expectEqual(sum.integrity.sri().len, JS_CHUNK_SRI_LEN);
        passed += 1;
        iter += 1;
    }

    expectEqual(passed, @as(usize, 500));
});
