import { test, assert, assertEq } from "cap:test";
import {
    CHUNK_CODE_COMMONJS_HELPER,
    CHUNK_CODE_ESM_HELPER,
    CHUNK_CODE_ASYNC_ESM_HELPER,
    ChunkCodeEmitError,
    MinifyWhitespaceOptions,
    minifyJsWhitespaceSafe,
    ModuleWrapMode,
    moduleWrapModeName,
    ModuleEmitInput,
    ModuleEmitOptions,
    emitWrappedModuleInto,
    ModuleEmitSpan,
    ChunkCodeAssembler,
    formatChunkEmitSpanTable,
    resolveGeneratedLineToModule,
    emitModuleBatchIntoChunk,
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

test("emitWrappedModuleInto supports all 4 wrap modes and validates wrapper identifiers", () => {
    let out: [1024]u8 = [0; 1024];

    // 1. FlatEsm
    const r_flat = emitWrappedModuleInto(
        ModuleEmitInput{
            source_path: "src/flat.js",
            wrap_mode: ModuleWrapMode.FlatEsm,
            body_code: "export const x = 1;",
        },
        ModuleEmitOptions.defaults(),
        out[0..],
    ) catch unreachable;
    assert(strContains(out[0..r_flat.bytes_written], "// src/flat.js\nexport const x = 1;\n"));
    assertEq(r_flat.lines_written, @as(u32, 2));

    // 2. CommonJsClosure with "use strict"
    const r_cjs = emitWrappedModuleInto(
        ModuleEmitInput{
            source_path: "node_modules/pkg/index.js",
            wrapper_name: "require_pkg",
            wrap_mode: ModuleWrapMode.CommonJsClosure,
            emit_use_strict: true,
            body_code: "module.exports = 42;",
        },
        ModuleEmitOptions.defaults(),
        out[0..],
    ) catch unreachable;
    const cjs_code = out[0..r_cjs.bytes_written];
    assert(strContains(cjs_code, "var require_pkg = __commonJS((exports, module) => {\n\"use strict\";\nmodule.exports = 42;\n});\n"));

    // 3. LazyEsmInit
    const r_esm = emitWrappedModuleInto(
        ModuleEmitInput{
            wrapper_name: "init_lazy",
            wrap_mode: ModuleWrapMode.LazyEsmInit,
            hoisted_imports: "import { a } from \"./a.js\";",
            body_code: "const b = a + 1;",
        },
        ModuleEmitOptions.defaults(),
        out[0..],
    ) catch unreachable;
    assert(strContains(out[0..r_esm.bytes_written], "import { a } from \"./a.js\";\nvar init_lazy = __esm(() => {\nconst b = a + 1;\n});\n"));

    // 4. AsyncEsmInit (Top-Level Await)
    const r_async = emitWrappedModuleInto(
        ModuleEmitInput{
            wrapper_name: "init_tla",
            wrap_mode: ModuleWrapMode.AsyncEsmInit,
            body_code: "const data = await fetch('/api');",
        },
        ModuleEmitOptions.defaults(),
        out[0..],
    ) catch unreachable;
    assert(strContains(out[0..r_async.bytes_written], "var init_tla = __asyncEsm(async () => {\nconst data = await fetch('/api');\n});\n"));
    assert(strEql(moduleWrapModeName(ModuleWrapMode.AsyncEsmInit), "async_esm_init"));

    // 5. Invalid wrapper name rejected
    let invalid_wrap = false;
    * = emitWrappedModuleInto(
        ModuleEmitInput{
            wrapper_name: "123-bad",
            wrap_mode: ModuleWrapMode.CommonJsClosure,
            body_code: "exports.a = 1;",
        },
        ModuleEmitOptions.defaults(),
        out[0..],
    ) catch {
        invalid_wrap = true;
    };
    assert(invalid_wrap);
});

test("ChunkCodeAssembler tracks exact SIMD line/column spans and O(log M) reverse line lookup", () => {
    let out: [4096]u8 = [0; 4096];
    let spans: [8]ModuleEmitSpan = [ModuleEmitSpan.empty(); 8];
    let assembler = ChunkCodeAssembler.init(out[0..], spans[0..], ModuleEmitOptions.defaults());

    assembler.emitBanner("/*! Built by cap */\n") catch unreachable; // line 0
    assembler.emitRuntimeHelpers(true, true, false) catch unreachable; // lines 1, 2

    const sp0 = assembler.emitModule(ModuleEmitInput{
        source_index: 10,
        source_path: "src/cjs.js",
        wrapper_name: "require_cjs",
        wrap_mode: ModuleWrapMode.CommonJsClosure,
        body_code: "exports.ok = true;\nexports.n = 7;",
    }) catch unreachable;
    assertEq(sp0.start_line, @as(u32, 3));
    assertEq(sp0.end_line, @as(u32, 8)); // // src + var + 2 body lines + }); = 5 lines -> 3..8

    const sp1 = assembler.emitModule(ModuleEmitInput{
        source_index: 20,
        source_path: "src/entry.js",
        wrap_mode: ModuleWrapMode.FlatEsm,
        body_code: "const c = require_cjs();\nconsole.log(c.n);",
    }) catch unreachable;
    assertEq(sp1.start_line, @as(u32, 8));
    assertEq(sp1.end_line, @as(u32, 11));

    assembler.emitExportTail("export { require_cjs };") catch unreachable;
    assembler.emitFooter("//# sourceMappingURL=app.js.map") catch unreachable;
    const final_code = assembler.finalize();

    assert(strContains(final_code, CHUNK_CODE_COMMONJS_HELPER));
    assert(strContains(final_code, CHUNK_CODE_ESM_HELPER));
    assert(strContains(final_code, "export { require_cjs };"));
    assert(strContains(final_code, "//# sourceMappingURL=app.js.map"));

    // Reverse lookup line 5 -> belongs to sp0 (source_index 10, rel line 2)
    const owner0 = resolveGeneratedLineToModule(assembler.emittedSpans(), 5) orelse unreachable;
    assertEq(owner0.source_index, @as(u32, 10));
    assertEq(owner0.module_relative_line, @as(u32, 2));

    // Reverse lookup line 9 -> belongs to sp1 (source_index 20, rel line 1)
    const owner1 = resolveGeneratedLineToModule(assembler.emittedSpans(), 9) orelse unreachable;
    assertEq(owner1.source_index, @as(u32, 20));
    assertEq(owner1.module_relative_line, @as(u32, 1));

    // Banner line 0 is outside all module spans -> null
    assert(resolveGeneratedLineToModule(assembler.emittedSpans(), 0) == null);

    // Writing after finalize fails with AlreadyFinalized
    let after_fin = false;
    assembler.emitFooter("// late") catch {
        after_fin = true;
    };
    assert(after_fin);

    let tbl: [512]u8 = [0; 512];
    const tlen = formatChunkEmitSpanTable(assembler.emittedSpans(), tbl[0..]) catch unreachable;
    assert(strContains(tbl[0..tlen], "src/cjs.js (commonjs_closure) L3:0..L8:0"));
});

test("minifyJsWhitespaceSafe strips comments while strictly preserving strings, templates, regexes, and legal comments", () => {
    const src =
        "/*! Copyright (c) 2026 Cap */\n" ++
        "// regular comment to strip\n" ++
        "const   url   =   \"https://example.com/*not-comment*/\";\n" ++
        "const   tpl   =   `hello   // not comment\n  world`;\n" ++
        "/* block comment\n   spanning lines */\n" ++
        "const   re    =   /https:\\/\\/[a-z]+\\/foo/gi;\n" ++
        "const   sum   =   a + +b;\n" ++
        "//# sourceMappingURL=out.js.map\n";

    let out: [1024]u8 = [0; 1024];
    const st = minifyJsWhitespaceSafe(src, MinifyWhitespaceOptions.defaults(), out[0..]) catch unreachable;
    const min = out[0..st.output_bytes];

    assert(st.output_bytes < st.input_bytes);
    assertEq(st.line_comments_stripped, @as(u32, 1));
    assertEq(st.block_comments_stripped, @as(u32, 1));
    assertEq(st.legal_comments_preserved, @as(u32, 2));
    assertEq(st.strings_preserved, @as(u32, 2));
    assertEq(st.regexes_preserved, @as(u32, 1));

    assert(strContains(min, "/*! Copyright (c) 2026 Cap */"));
    assert(strContains(min, "\"https://example.com/*not-comment*/\""));
    assert(strContains(min, "`hello   // not comment\n  world`"));
    assert(strContains(min, "/https:\\/\\/[a-z]+\\/foo/gi"));
    assert(strContains(min, "a+ +b") || strContains(min, "a + +b"));
    assert(strContains(min, "//# sourceMappingURL=out.js.map"));
    assert(!strContains(min, "regular comment to strip"));
    assert(!strContains(min, "spanning lines"));
});

test("minifyJsWhitespaceSafe preserves keyword/identifier spacing and handles unclosed comments safely", () => {
    const src = "export   async   function   run(x) { return   x; } /* unclosed block";
    let out: [256]u8 = [0; 256];
    const st = minifyJsWhitespaceSafe(src, MinifyWhitespaceOptions.defaults(), out[0..]) catch unreachable;
    const min = out[0..st.output_bytes];
    assert(strContains(min, "export async function run(x)"));
    assert(strContains(min, "return x;"));
    assertEq(st.block_comments_stripped, @as(u32, 1));
});

test("emitModuleBatchIntoChunk auto-detects needed runtime helpers and minifies modules cleanly", () => {
    const mods: [3]ModuleEmitInput = [
        ModuleEmitInput{
            source_index: 0,
            source_path: "src/tla.js",
            wrapper_name: "init_tla",
            wrap_mode: ModuleWrapMode.AsyncEsmInit,
            body_code: "// init\nconst   v   =   await Promise.resolve(1);",
        },
        ModuleEmitInput{
            source_index: 1,
            source_path: "src/cjs.js",
            wrapper_name: "require_cjs",
            wrap_mode: ModuleWrapMode.CommonJsClosure,
            body_code: "/* cjs */ exports.x = 2;",
        },
        ModuleEmitInput{
            source_index: 2,
            source_path: "src/app.js",
            wrap_mode: ModuleWrapMode.FlatEsm,
            body_code: "console.log(require_cjs().x);",
        },
    ];

    let out: [4096]u8 = [0; 4096];
    let spans: [4]ModuleEmitSpan = [ModuleEmitSpan.empty(); 4];
    const st = emitModuleBatchIntoChunk(
        "",
        mods[0..],
        "",
        "",
        ModuleEmitOptions{ emit_source_comments: false, minify_whitespace: true },
        out[0..],
        spans[0..],
    ) catch unreachable;

    assertEq(st.modules_emitted, @as(usize, 3));
    assertEq(st.helpers_injected, @as(u32, 2)); // CJS + AsyncEsm (no LazyEsm needed!)
    assertEq(st.async_esm_wrapped, @as(u32, 1));
    assertEq(st.commonjs_wrapped, @as(u32, 1));
    assertEq(st.flat_esm_emitted, @as(u32, 1));

    const code = out[0..st.total_bytes];
    assert(strContains(code, CHUNK_CODE_COMMONJS_HELPER));
    assert(strContains(code, CHUNK_CODE_ASYNC_ESM_HELPER));
    assert(!strContains(code, CHUNK_CODE_ESM_HELPER));
});

test("emitWrappedModuleInto reports BufferTooSmall when output buffer cannot fit emitted module", () => {
    let tiny: [16]u8 = [0; 16];
    let too_small = false;
    * = emitWrappedModuleInto(
        ModuleEmitInput{
            wrapper_name: "init_big",
            wrap_mode: ModuleWrapMode.LazyEsmInit,
            body_code: "const long_statement = 1234567890;",
        },
        ModuleEmitOptions.defaults(),
        tiny[0..],
    ) catch {
        too_small = true;
    };
    assert(too_small);
});

test("deterministic 500-iteration fuzz over random JS comments, strings, templates, and regexes causes 0 panics", () => {
    let seed: u64 = 0xbf58476d1ce4e5b9;
    const frags: [12][]const u8 = [
        "const a = 1; ",
        "// comment\n",
        "/* block */",
        "/*! legal */",
        "\"str // /* \\\" esc\"",
        "'single \\' str'",
        "`tpl\n${x}`",
        " = /[a-z\\/]+/gi;",
        " + +x - -y ",
        "/* unclosed",
        "\"unterminated\n",
        "//# sourceMappingURL=x.map\n",
    ];

    let in_buf: [256]u8 = [0; 256];
    let out_buf: [512]u8 = [0; 512];
    let iter: u32 = 0;
    let total_in: usize = 0;
    let total_out: usize = 0;

    while (iter < 500) {
        seed = seed ^ (seed >> 12);
        seed = seed ^ (seed << 25);
        seed = seed ^ (seed >> 27);
        let r = seed *% 2685821657736338717;

        let len: usize = 0;
        const pieces: usize = @as(usize, @intCast(r % 7)) + 1;
        let p: usize = 0;
        while (p < pieces && len < 200) {
            r = r *% 6364136223846793005 +% 1442695040888963407;
            const frag = frags[@as(usize, @intCast((r >> 16) % 12))];
            const take = if ((r & 3) == 0 && frag.len > 1)
                @as(usize, @intCast((r >> 8) % frag.len)) + 1
            else
                frag.len;
            let j: usize = 0;
            while (j < take && len < in_buf.len) {
                in_buf[len] = frag[j];
                len += 1;
                j += 1;
            }
            p += 1;
        }

        const st = minifyJsWhitespaceSafe(
            in_buf[0..len],
            MinifyWhitespaceOptions{ preserve_legal_comments: (iter & 1) == 0, collapse_newlines: (iter & 2) == 0 },
            out_buf[0..],
        ) catch unreachable;
        assert(st.output_bytes <= out_buf.len);
        total_in += st.input_bytes;
        total_out += st.output_bytes;
        iter += 1;
    }

    assert(total_in > 10000);
    assert(total_out > 0);
});
