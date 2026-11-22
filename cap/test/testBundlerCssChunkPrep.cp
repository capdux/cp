// Deep production regression tests for cap:bundler CSS Chunk AST preparation (lib/bundler/cssChunkPrep.cp)

import { test, expect, expectError } from "cap:test";
import {
    CssChunkPrepError,
    CssPrepRuleKind,
    CssPrepRule,
    CssPrepRuleList,
    CssPrepCondition,
    stripCssPrefixImportsPreservingLayers,
    wrapCssBodyWithConditions,
    formatNestedExternalImport,
    dedupCssChunkRulesReverse,
} from "cap:bundler";

test("cssChunkPrep: Regression #28914 strips prefix imports while preserving interleaved @layer", () => {
    const src: [5]CssPrepRule = [
        CssPrepRule{ kind: .LayerStatement, text: "@layer theme, base, components, utilities;" },
        CssPrepRule{ kind: .Import, text: "@import \"./theme.css\";" },
        CssPrepRule{ kind: .Ignored, text: "/* ignored */" },
        CssPrepRule{ kind: .LayerStatement, text: "@layer extra;" },
        CssPrepRule{ kind: .StyleRule, text: ".btn{color:red}" },
    ];
    let out = CssPrepRuleList.init();
    const st = stripCssPrefixImportsPreservingLayers(src[0..], &out)?;

    expect(st.original_count).toBe(5 as usize);
    expect(st.stripped_count).toBe(2 as usize);
    expect(st.preserved_prefix_layers).toBe(2 as usize);
    expect(st.kept_count).toBe(3 as usize);
    expect(out.slice()[0].text).toEqual("@layer theme, base, components, utilities;");
    expect(out.slice()[1].text).toEqual("@layer extra;");
    expect(out.slice()[2].text).toEqual(".btn{color:red}");
});

test("cssChunkPrep: inside-out condition wrapping (@layer -> @supports -> @media)", () => {
    const conds: [2]CssPrepCondition = [
        CssPrepCondition{ media: "screen" },
        CssPrepCondition{ has_layer: true, layer_name: "theme", supports: "display:grid" },
    ];
    let buf: [256]u8 = [0; 256];
    const n = wrapCssBodyWithConditions(".a{color:blue}", conds[0..], buf[0..])?;
    expect(buf[0..n]).toEqual("@media screen{@supports (display:grid){@layer theme{.a{color:blue}}}}");
});

test("cssChunkPrep: empty body semantics (anonymous layer omitted, named layer lowers to statement)", () => {
    let buf: [128]u8 = [0; 128];

    // 空匿名 @layer + @supports + @media 全部省略
    const c_anon: [1]CssPrepCondition = [
        CssPrepCondition{ has_layer: true, layer_name: null, supports: "display:flex", media: "print" },
    ];
    const n1 = wrapCssBodyWithConditions("", c_anon[0..], buf[0..])?;
    expect(n1).toBe(0 as usize);

    // 空具名 @layer 降级为 `@layer base;`，并且外层 @media 仍包装该声明
    const c_named: [1]CssPrepCondition = [
        CssPrepCondition{ has_layer: true, layer_name: "base", media: "screen" },
    ];
    const n2 = wrapCssBodyWithConditions("", c_named[0..], buf[0..])?;
    expect(buf[0..n2]).toEqual("@media screen{@layer base;}");
});

test("cssChunkPrep: external @import single-layer and multi-layer data:text/css chain", () => {
    let buf: [512]u8 = [0; 512];

    const c_one: [1]CssPrepCondition = [
        CssPrepCondition{ has_layer: true, layer_name: "ext", supports: "display:grid", media: "screen" },
    ];
    const n1 = formatNestedExternalImport("https://cdn.example.com/a.css", c_one[0..], buf[0..])?;
    expect(buf[0..n1]).toEqual("@import \"https://cdn.example.com/a.css\" layer(ext) supports(display:grid) screen;");

    const c_two: [2]CssPrepCondition = [
        CssPrepCondition{ has_layer: true, layer_name: "outer", media: "screen" },
        CssPrepCondition{ has_layer: true, layer_name: "inner" },
    ];
    const n2 = formatNestedExternalImport("https://cdn.example.com/b.css", c_two[0..], buf[0..])?;
    expect(buf[0..n2]).toEqual(
        "@import \"data:text/css,@import %22https://cdn.example.com/b.css%22 layer(inner);\" layer(outer) screen;",
    );
});

test("cssChunkPrep: reverse-order rule deduplication keeps last declaration", () => {
    const src: [5]CssPrepRule = [
        CssPrepRule{ kind: .StyleRule, text: ".a{color:red}", dedup_key: "r1" },
        CssPrepRule{ kind: .StyleRule, text: ".b{color:green}", dedup_key: "r2" },
        CssPrepRule{ kind: .StyleRule, text: ".a{color:red}", dedup_key: "r1" },
        CssPrepRule{ kind: .LayerStatement, text: "@layer base;", dedup_key: "" },
        CssPrepRule{ kind: .StyleRule, text: ".c{color:blue}", dedup_key: "r3" },
    ];
    let out = CssPrepRuleList.init();
    const removed = dedupCssChunkRulesReverse(src[0..], &out)?;

    expect(removed).toBe(1 as usize);
    expect(out.count).toBe(4 as usize);
    // 第一条 r1 被移除，保留后声明位置的 r1（顺序：r2, r1, layer, r3）
    expect(out.slice()[0].dedup_key).toEqual("r2");
    expect(out.slice()[1].dedup_key).toEqual("r1");
    expect(out.slice()[2].text).toEqual("@layer base;");
    expect(out.slice()[3].dedup_key).toEqual("r3");
});

test("cssChunkPrep: empty external URL and small buffer guards", () => {
    let tiny: [8]u8 = [0; 8];
    const empty_conds: [0]CssPrepCondition = [];

    expectError(
        formatNestedExternalImport("", empty_conds[0..], tiny[0..]),
        CssChunkPrepError.EmptyExternalUrl,
    );
    expectError(
        formatNestedExternalImport("https://example.com/a.css", empty_conds[0..], tiny[0..]),
        CssChunkPrepError.BufferTooSmall,
    );

    const c: [1]CssPrepCondition = [CssPrepCondition{ media: "screen" }];
    expectError(
        wrapCssBodyWithConditions(".btn{color:red}", c[0..], tiny[0..]),
        CssChunkPrepError.BufferTooSmall,
    );
});
