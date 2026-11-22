// Deep production regression tests for cap:css StyleRule (lib/css/styleRule.cp)

import { test, expect, expectError } from "cap:test";
import {
    CssStyleRuleError,
    CssStyleNestedRule,
    CssStyleRule,
    resolveNestedSelector,
} from "cap:css";

test("cssStyleRule: isEmpty, hashKey determinism and case rules", () => {
    let r1 = CssStyleRule.init();
    expect(r1.isEmpty()).toBe(true);

    * = r1.addSelector(".card")?;
    expect(r1.isEmpty()).toBe(true); // 有选择器但无声明/子规则仍为空

    * = r1.setDecl("Color", "red", false)?;
    * = r1.setDecl("--Theme", "dark", false)?;
    expect(r1.isEmpty()).toBe(false);

    // 标准属性大小写无关产生相同 hashKey；自定义属性大小写敏感产生不同 hashKey
    let r2 = CssStyleRule.init();
    * = r2.addSelector(".card")?;
    * = r2.setDecl("color", "blue", false)?;
    * = r2.setDecl("--Theme", "light", false)?;
    expect(r1.hashKey()).toBe(r2.hashKey());

    let r3 = CssStyleRule.init();
    * = r3.addSelector(".card")?;
    * = r3.setDecl("color", "blue", false)?;
    * = r3.setDecl("--theme", "light", false)?;
    expect(r1.hashKey() != r3.hashKey()).toBe(true);
});

test("cssStyleRule: declaration cascade priority (!important and custom property case)", () => {
    let r = CssStyleRule.init();
    * = r.addSelector(".btn")?;
    * = r.setDecl("color", "red", true)?;
    * = r.setDecl("color", "blue", false)?; // 普通声明不可覆盖 !important
    * = r.setDecl("--accent", "1", false)?;
    * = r.setDecl("--Accent", "2", false)?; // 自定义属性严格区分大小写

    let buf: [256]u8 = [0; 256];
    const n = r.toCss(buf[0..], true, true)?;
    expect(buf[0..n]).toEqual(".btn{color:red!important;--accent:1;--Accent:2}");
});

test("cssStyleRule: native nesting vs lowered flattening with & substitution", () => {
    let r = CssStyleRule.init();
    * = r.addSelector(".card")?;
    * = r.setDecl("padding", "1rem", false)?;

    let hover = CssStyleNestedRule.init();
    * = hover.addSelector("&:hover")?;
    * = hover.setDecl("color", "blue", false)?;
    * = r.addNested(hover)?;

    let title = CssStyleNestedRule.init();
    * = title.addSelector(".title")?;
    * = title.setDecl("font-weight", "bold", false)?;
    * = r.addNested(title)?;

    let buf: [512]u8 = [0; 512];
    const n_native = r.toCss(buf[0..], true, true)?;
    expect(buf[0..n_native]).toEqual(".card{padding:1rem;&:hover{color:blue}.title{font-weight:bold}}");

    const n_flat = r.toCss(buf[0..], true, false)?;
    expect(buf[0..n_flat]).toEqual(".card{padding:1rem}.card:hover{color:blue}.card .title{font-weight:bold}");
});

test("cssStyleRule: nesting lowering omits empty parent block and expands cartesian product", () => {
    let r = CssStyleRule.init();
    * = r.addSelector(".a")?;
    * = r.addSelector(".b")?;

    let child = CssStyleNestedRule.init();
    * = child.addSelector("&.active")?;
    * = child.addSelector("span")?;
    * = child.setDecl("display", "block", false)?;
    * = r.addNested(child)?;

    let buf: [256]u8 = [0; 256];
    // 父规则无自身声明，展平模式省略空父块 `.a,.b{}`
    const n = r.toCss(buf[0..], true, false)?;
    expect(buf[0..n]).toEqual(".a.active,.a span,.b.active,.b span{display:block}");
});

test("cssStyleRule: mergeFrom combines matching selectors and rejects mismatches", () => {
    let a = CssStyleRule.init();
    * = a.addSelector(".box")?;
    * = a.setDecl("margin", "0", false)?;
    * = a.setDecl("padding", "4px", true)?;

    let b = CssStyleRule.init();
    * = b.addSelector(".box")?;
    * = b.setDecl("margin", "8px", false)?;
    * = b.setDecl("padding", "12px", false)?; // 不覆盖 !important

    const merged = a.mergeFrom(b)?;
    expect(merged).toBe(2 as usize);

    let buf: [256]u8 = [0; 256];
    const n = a.toCss(buf[0..], true, true)?;
    expect(buf[0..n]).toEqual(".box{margin:8px;padding:4px!important}");

    let other = CssStyleRule.init();
    * = other.addSelector(".other")?;
    expectError(a.mergeFrom(other), CssStyleRuleError.SelectorMismatch);
});

test("cssStyleRule: resolveNestedSelector and boundary error guards", () => {
    let buf: [64]u8 = [0; 64];
    const n1 = resolveNestedSelector(".btn", "&:focus, & > icon", buf[0..])?;
    expect(buf[0..n1]).toEqual(".btn:focus, .btn > icon");

    const n2 = resolveNestedSelector(".nav", "a", buf[0..])?;
    expect(buf[0..n2]).toEqual(".nav a");

    expectError(resolveNestedSelector("  ", "a", buf[0..]), CssStyleRuleError.EmptySelector);

    let r = CssStyleRule.init();
    expectError(r.addSelector("   "), CssStyleRuleError.EmptySelector);
    expectError(r.setDecl("", "1px", false), CssStyleRuleError.EmptyProperty);

    * = r.addSelector(".long-selector")?;
    * = r.setDecl("background-color", "rebeccapurple", false)?;
    let tiny: [8]u8 = [0; 8];
    expectError(r.toCss(tiny[0..], true, true), CssStyleRuleError.BufferTooSmall);
});
