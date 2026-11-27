import { test, expect_eq } from "cap:test";
import {
    minifyCss,
    expandNestedSelector,
    compileNestedCss,
} from "../../packages/stylis/src/index.cp";

fn main() {
    test("1. @cap/stylis — minifyCss strips block and line comments and collapses whitespace", () => {
        let buf: [256]u8 = [0; 256];
        const css: string = "/* header */\n// single line\n.btn {\n  color: red;\n  margin: 0   auto;\n}\n";
        const n: usize = minifyCss(css, buf[0..256]);
        expect_eq(buf[0..n] as string, ".btn{color:red;margin:0 auto;}");
    });

    test("2. @cap/stylis — minifyCss preserves quoted strings and descendant spaces", () => {
        let buf: [256]u8 = [0; 256];
        const css: string = ".card   .title { content: \"hello  /* keep */  world\"; background: url(https://a.com/x.png); }";
        const n: usize = minifyCss(css, buf[0..256]);
        expect_eq(buf[0..n] as string, ".card .title{content:\"hello  /* keep */  world\";background:url(https://a.com/x.png);}");
    });

    test("3. @cap/stylis — expandNestedSelector replaces & placeholders", () => {
        let buf: [128]u8 = [0; 128];
        const n1: usize = expandNestedSelector(".btn", "&:hover", buf[0..128]);
        expect_eq(buf[0..n1] as string, ".btn:hover");
        const n2: usize = expandNestedSelector(".card", "&--active", buf[0..128]);
        expect_eq(buf[0..n2] as string, ".card--active");
        const n3: usize = expandNestedSelector(".item", ".dark &", buf[0..128]);
        expect_eq(buf[0..n3] as string, ".dark .item");
    });

    test("4. @cap/stylis — expandNestedSelector joins descendant selectors and comma lists", () => {
        let buf: [128]u8 = [0; 128];
        const n1: usize = expandNestedSelector(".card", ".title", buf[0..128]);
        expect_eq(buf[0..n1] as string, ".card .title");
        const n2: usize = expandNestedSelector("h1, h2", "&:focus", buf[0..128]);
        expect_eq(buf[0..n2] as string, "h1:focus,h2:focus");
    });

    test("5. @cap/stylis — compileNestedCss flattens base declarations and nested blocks", () => {
        let buf: [512]u8 = [0; 512];
        const body: string = "color: red; padding: 8px; &:hover { color: blue; } .icon { width: 16px; }";
        const n: usize = compileNestedCss(".btn", body, buf[0..512]);
        expect_eq(buf[0..n] as string, ".btn{color:red;padding:8px;}.btn:hover{color:blue;}.btn .icon{width:16px;}");
    });

    test("6. @cap/stylis — compileNestedCss handles nested-only blocks and comments cleanly", () => {
        let buf: [512]u8 = [0; 512];
        const body: string = "/* hover state */\n&:hover { opacity: 0.8; }\n// active\n&--active { display: block; }";
        const n: usize = compileNestedCss(".card", body, buf[0..512]);
        expect_eq(buf[0..n] as string, ".card:hover{opacity:0.8;}.card--active{display:block;}");
    });
}
