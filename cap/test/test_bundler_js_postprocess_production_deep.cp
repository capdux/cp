// Production Deep Test for cap:bundler (jsPostprocess.cp)
// Covers: bytecode caching eligibility (ESM requires --compile, CJS allowed without --compile),
// entry-point tail decision matrix, JsLineOffsetTracker line/column accounting,
// full chunk assembly order (shebang, banner, IIFE/CJS-bun wrappers, cross-chunk prefix/suffix,
// InternalBakeDev runtime skip, footer), and error boundary guards.

import { test, assert, assertEq } from "cap:test";
import {
    JsChunkOutputFormat,
    JsChunkWrapKind,
    JsChunkCommentStyle,
    JsEntryTailDecision,
    shouldGenerateEsmBytecodeModuleInfo,
    validateBytecodeConfig,
    decideEntryPointTail,
    JsLineOffsetTracker,
    JsChunkPartInput,
    JsChunkAssemblyOptions,
    assembleJsChunk,
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

fn containsSubstr(haystack: []const u8, needle: []const u8): bool {
    if (needle.len == 0) return true;
    if (needle.len > haystack.len) return false;
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        if (strEql(haystack[i .. i + needle.len], needle)) {
            return true;
        }
        i += 1;
    }
    return false;
}

test("jsPostprocess: bytecode eligibility and ESM --compile enforcement", () => {
    assert(shouldGenerateEsmBytecodeModuleInfo(true, JsChunkOutputFormat.Esm, true));
    assert(!shouldGenerateEsmBytecodeModuleInfo(true, JsChunkOutputFormat.Esm, false));
    assert(!shouldGenerateEsmBytecodeModuleInfo(true, JsChunkOutputFormat.Cjs, true));
    assert(!shouldGenerateEsmBytecodeModuleInfo(false, JsChunkOutputFormat.Esm, true));

    // CJS bytecode is valid without --compile
    const ok_cjs = validateBytecodeConfig(true, JsChunkOutputFormat.Cjs, false) catch false;
    assert(ok_cjs);

    // ESM bytecode with --compile is valid
    const ok_esm_compile = validateBytecodeConfig(true, JsChunkOutputFormat.Esm, true) catch false;
    assert(ok_esm_compile);

    // ESM bytecode without --compile must be rejected
    const bad_esm = validateBytecodeConfig(true, JsChunkOutputFormat.Esm, false) catch false;
    assert(!bad_esm);
});

test("jsPostprocess: entry-point tail decision matrix", () => {
    assert(decideEntryPointTail(JsChunkOutputFormat.Esm, JsChunkWrapKind.Cjs, false) == JsEntryTailDecision.ExportDefaultRequire);
    assert(decideEntryPointTail(JsChunkOutputFormat.Esm, JsChunkWrapKind.Esm, true) == JsEntryTailDecision.AwaitInit);
    assert(decideEntryPointTail(JsChunkOutputFormat.Esm, JsChunkWrapKind.Esm, false) == JsEntryTailDecision.SyncInit);
    assert(decideEntryPointTail(JsChunkOutputFormat.Esm, JsChunkWrapKind.None, false) == JsEntryTailDecision.None);
    assert(decideEntryPointTail(JsChunkOutputFormat.Cjs, JsChunkWrapKind.Cjs, false) == JsEntryTailDecision.ModuleExports);
    assert(decideEntryPointTail(JsChunkOutputFormat.Iife, JsChunkWrapKind.Esm, true) == JsEntryTailDecision.AwaitInit);
});

test("jsPostprocess: JsLineOffsetTracker multi-line accounting and reset", () => {
    let tr = JsLineOffsetTracker.init();
    tr.advance("const a = 1;\nconst b = 2;\nabc");
    assertEq(tr.lines, 2);
    assertEq(tr.column, 3);

    tr.advance("de\n");
    assertEq(tr.lines, 3);
    assertEq(tr.column, 0);

    tr.reset();
    assertEq(tr.lines, 0);
    assertEq(tr.column, 0);
});

test("jsPostprocess: IIFE chunk assembly with shebang, banner, comments, tail, and footer", () => {
    const parts: [2]JsChunkPartInput = [
        JsChunkPartInput{ label: "a.ts", code: "const a = 1;" },
        JsChunkPartInput{ label: "b.ts", code: "console.log(a);" },
    ];
    let out: [512]u8 = [0; 512];
    const res = assembleJsChunk(
        JsChunkAssemblyOptions{
            format: JsChunkOutputFormat.Iife,
            shebang: "#!/usr/bin/env bun",
            banner: "// banner",
            footer: "// footer",
            cross_chunk_prefix: "// pre",
            cross_chunk_suffix: "// suf",
            entry_point_tail: "init();",
            emit_part_comments: true,
            comment_style: JsChunkCommentStyle.Multiline,
        },
        parts[0..],
        out[0..],
    ) catch {
        assert(false);
        return;
    };

    assert(res.is_executable);
    assertEq(res.parts_emitted, 2);
    const text = out[0..res.bytes_written];
    assert(containsSubstr(text, "#!/usr/bin/env bun\n// banner\n(() => {\n// pre\n/* a.ts */\nconst a = 1;"));
    assert(containsSubstr(text, "/* b.ts */\nconsole.log(a);\ninit();\n// suf\n})();\n// footer\n"));
});

test("jsPostprocess: CJS bun wrapper and InternalBakeDev runtime skip + error guards", () => {
    const parts: [2]JsChunkPartInput = [
        JsChunkPartInput{ label: "runtime", code: "var __rt = 1;", is_runtime: true },
        JsChunkPartInput{ label: "app.ts", code: "exports.ok = true;", is_runtime: false },
    ];

    // InternalBakeDev skips embedded runtime part
    let bake_out: [256]u8 = [0; 256];
    const bake_res = assembleJsChunk(
        JsChunkAssemblyOptions{
            format: JsChunkOutputFormat.InternalBakeDev,
        },
        parts[0..],
        bake_out[0..],
    ) catch {
        assert(false);
        return;
    };
    assertEq(bake_res.parts_emitted, 1);
    assert(!containsSubstr(bake_out[0..bake_res.bytes_written], "__rt"));
    assert(containsSubstr(bake_out[0..bake_res.bytes_written], "exports.ok = true;"));

    // CJS with is_bun_target wraps in module function
    let cjs_out: [256]u8 = [0; 256];
    const cjs_res = assembleJsChunk(
        JsChunkAssemblyOptions{
            format: JsChunkOutputFormat.Cjs,
            is_bun_target: true,
        },
        parts[0..],
        cjs_out[0..],
    ) catch {
        assert(false);
        return;
    };
    assertEq(cjs_res.parts_emitted, 2);
    assert(containsSubstr(cjs_out[0..cjs_res.bytes_written], "(function(exports, require, module, __filename, __dirname) {"));
    assert(containsSubstr(cjs_out[0..cjs_res.bytes_written], "})\n"));

    // Invalid shebang must be rejected
    let bad_out: [64]u8 = [0; 64];
    let invalid_shebang = false;
    * = assembleJsChunk(
        JsChunkAssemblyOptions{ shebang: "/usr/bin/env bun" },
        parts[0..1],
        bad_out[0..],
    ) catch {
        invalid_shebang = true;
        return;
    };
    assert(!invalid_shebang);
});
