import { test, expect, expect_eq } from "cap:test";
import {
    CSS_CHUNK_MAX_PARTS,
    CssChunkPostProcessError,
    CssChunkSourceOffset,
    CssChunkCompileResult,
    CssChunkPostProcessOptions,
    CssSourceMapEntry,
    CssChunkPostProcessResult,
    postProcessCssChunk,
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

test("bundler css_chunk_postprocess: CssChunkSourceOffset LF/CRLF/CR tracking and reset", fn()! {
    expect_eq(CSS_CHUNK_MAX_PARTS, 64);

    let off = CssChunkSourceOffset{};
    off.advance("body {");
    expect_eq(off.lines, 0);
    expect_eq(off.columns, 6);

    off.advance("\n  color: red;\r\n}\r.x");
    expect_eq(off.lines, 3);
    expect_eq(off.columns, 2);

    const expected = CssChunkSourceOffset{ lines: 3, columns: 2 };
    expect(off.eql(expected));

    off.reset();
    expect_eq(off.lines, 0);
    expect_eq(off.columns, 0);
});

test("bundler css_chunk_postprocess: bundle mode comments, separating blank lines, and trailing newline", fn()! {
    const parts: [3]CssChunkCompileResult = [
        CssChunkCompileResult{
            source_index: 10,
            pretty_path: "src/empty.css",
            code: "",
            has_source_map: false,
        },
        CssChunkCompileResult{
            source_index: 11,
            pretty_path: "src/base.css",
            code: "body { margin: 0; }\n",
            has_source_map: false,
        },
        CssChunkCompileResult{
            source_index: 12,
            pretty_path: "src/card.css",
            code: ".card { padding: 8px; }",
            has_source_map: false,
        },
    ];

    let out: [512]u8 = [0; 512];
    const res = postProcessCssChunk(parts[0..], CssChunkPostProcessOptions{}, out[0..])?;
    expect_eq(res.non_empty_parts, 2);
    expect_eq(res.source_map_count, 0);

    // Note: empty.css had no code, so base.css has NO leading blank line;
    // card.css follows non-empty base.css, so it DOES get a blank line before its comment.
    const expected =
        "/* src/empty.css */\n" ++
        "/* src/base.css */\n" ++
        "body { margin: 0; }\n" ++
        "\n" ++
        "/* src/card.css */\n" ++
        ".card { padding: 8px; }\n";
    expect(strEql(out[0..res.output_len], expected));
});

test("bundler css_chunk_postprocess: minify_whitespace and non-bundle modes omit source comments", fn()! {
    const parts: [2]CssChunkCompileResult = [
        CssChunkCompileResult{
            source_index: 0,
            pretty_path: "a.css",
            code: "a{color:red}",
            has_source_map: false,
        },
        CssChunkCompileResult{
            source_index: 1,
            pretty_path: "b.css",
            code: "b{color:blue}",
            has_source_map: false,
        },
    ];

    let out: [256]u8 = [0; 256];
    const min_res = postProcessCssChunk(
        parts[0..],
        CssChunkPostProcessOptions{ is_bundle: true, minify_whitespace: true },
        out[0..],
    )?;
    expect(strEql(out[0..min_res.output_len], "a{color:red}b{color:blue}\n"));

    const non_bundle_res = postProcessCssChunk(
        parts[0..],
        CssChunkPostProcessOptions{ is_bundle: false, minify_whitespace: false },
        out[0..],
    )?;
    expect(strEql(out[0..non_bundle_res.output_len], "a{color:red}b{color:blue}\n"));
});

test("bundler css_chunk_postprocess: SourceMap relative offset tracking and reset across parts", fn()! {
    const parts: [3]CssChunkCompileResult = [
        CssChunkCompileResult{
            source_index: 1,
            pretty_path: "a.css",
            code: "a { color: red; }\n",
            has_source_map: true,
        },
        CssChunkCompileResult{
            source_index: 2,
            pretty_path: "b.css",
            code: "b {\n  margin: 0;\n}\n",
            has_source_map: false,
        },
        CssChunkCompileResult{
            source_index: 3,
            pretty_path: "c.css",
            code: "c { padding: 4px; }\n",
            has_source_map: true,
        },
    ];

    let out: [512]u8 = [0; 512];
    const res = postProcessCssChunk(
        parts[0..],
        CssChunkPostProcessOptions{ is_bundle: true, minify_whitespace: false, emit_source_maps: true },
        out[0..],
    )?;

    expect_eq(res.source_map_count, 2);
    expect(res.sourceMapAt(2) == null);

    // Part 0 ("/* a.css */\n" -> 1 line before code)
    const sm0 = res.sourceMapAt(0) ?? return err.TestFailed;
    expect_eq(sm0.source_index, 1);
    expect_eq(sm0.generated_offset.lines, 1);
    expect_eq(sm0.generated_offset.columns, 0);
    expect(strEql(out[sm0.byte_offset..sm0.byte_offset + 17], "a { color: red; }"));

    // After part 0 reset -> part 1 adds "\n/* b.css */\n" (2 lines) + 3 lines of b.css = 5 lines
    // -> part 2 adds "\n/* c.css */\n" (2 lines) => total 7 lines relative to part 0's end!
    const sm1 = res.sourceMapAt(1) ?? return err.TestFailed;
    expect_eq(sm1.source_index, 3);
    expect_eq(sm1.generated_offset.lines, 7);
    expect_eq(sm1.generated_offset.columns, 0);
    expect(strEql(out[sm1.byte_offset..sm1.byte_offset + 19], "c { padding: 4px; }"));
});

test("bundler css_chunk_postprocess: banner, footer, and empty parts list", fn()! {
    const parts: [1]CssChunkCompileResult = [
        CssChunkCompileResult{
            source_index: 0,
            pretty_path: "main.css",
            code: ".x { display: flex; }",
            has_source_map: true,
        },
    ];

    let out: [512]u8 = [0; 512];
    const res = postProcessCssChunk(
        parts[0..],
        CssChunkPostProcessOptions{
            is_bundle: true,
            minify_whitespace: false,
            emit_source_maps: true,
            banner: "/* MIT License */",
            footer: "/* EOF */",
        },
        out[0..],
    )?;

    const expected =
        "/* MIT License */\n" ++
        "\n" ++
        "/* main.css */\n" ++
        ".x { display: flex; }\n" ++
        "/* EOF */\n";
    expect(strEql(out[0..res.output_len], expected));

    // Banner (1 line) + blank line (1) + comment (1) = 3 lines before main.css code
    const sm0 = res.sourceMapAt(0) ?? return err.TestFailed;
    expect_eq(sm0.generated_offset.lines, 3);

    // Empty parts with no banner/footer produces 0 bytes
    const empty_parts: [0]CssChunkCompileResult = [];
    const empty_res = postProcessCssChunk(empty_parts[0..], CssChunkPostProcessOptions{}, out[0..])?;
    expect_eq(empty_res.output_len, 0);
    expect_eq(empty_res.non_empty_parts, 0);
});

test("bundler css_chunk_postprocess: BufferTooSmall and TooManyParts guards", fn()! {
    const parts: [1]CssChunkCompileResult = [
        CssChunkCompileResult{
            source_index: 0,
            pretty_path: "big.css",
            code: "body { background: white; }",
            has_source_map: false,
        },
    ];

    let tiny: [8]u8 = [0; 8];
    let hit_small = false;
    if (_ := postProcessCssChunk(parts[0..], CssChunkPostProcessOptions{}, tiny[0..])) {} else {
        hit_small = true;
    }
    expect(hit_small);

    let too_many: [65]CssChunkCompileResult = [CssChunkCompileResult{}; 65];
    let big_out: [128]u8 = [0; 128];
    let hit_many = false;
    if (_ := postProcessCssChunk(too_many[0..], CssChunkPostProcessOptions{}, big_out[0..])) {} else {
        hit_many = true;
    }
    expect(hit_many);
});
