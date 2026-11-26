import { test, expect_eq } from "cap:test";
import {
    CssSelectorToken,
    parseCssSelector,
    stringifyCssSelector,
} from "../../packages/@cap/css-what/src/index.cp";

fn main() {
    test("1. @cap/css-what — Complex compound & combinator selector parsing", () => {
        let toks: [32]CssSelectorToken = undefined;
        const sel = "div#app.btn[data-x^=\"foo\" i]:not(.hidden) > span + a ~ p, h1::before";
        const n = parseCssSelector(sel, toks[0..32]);
        expect_eq(n, 14 as i64);

        expect_eq(toks[0].kind == .Tag, true);
        expect_eq(std.mem.eql(u8, toks[0].name, "div"), true);

        expect_eq(toks[1].kind == .Id, true);
        expect_eq(std.mem.eql(u8, toks[1].name, "app"), true);

        expect_eq(toks[2].kind == .Class, true);
        expect_eq(std.mem.eql(u8, toks[2].name, "btn"), true);

        expect_eq(toks[3].kind == .Attribute, true);
        expect_eq(std.mem.eql(u8, toks[3].name, "data-x"), true);
        expect_eq(std.mem.eql(u8, toks[3].value, "foo"), true);
        expect_eq(toks[3].action == .Start, true);
        expect_eq(toks[3].ignore_case, true);

        expect_eq(toks[4].kind == .Pseudo, true);
        expect_eq(std.mem.eql(u8, toks[4].name, "not"), true);
        expect_eq(std.mem.eql(u8, toks[4].value, ".hidden"), true);

        expect_eq(toks[5].kind == .Child, true);
        expect_eq(toks[6].kind == .Tag, true);
        expect_eq(toks[7].kind == .Adjacent, true);
        expect_eq(toks[8].kind == .Tag, true);
        expect_eq(toks[9].kind == .Sibling, true);
        expect_eq(toks[10].kind == .Tag, true);
        expect_eq(toks[11].kind == .Comma, true);
        expect_eq(toks[12].kind == .Tag, true);
        expect_eq(std.mem.eql(u8, toks[12].name, "h1"), true);
        expect_eq(toks[13].kind == .PseudoElement, true);
        expect_eq(std.mem.eql(u8, toks[13].name, "before"), true);
    });

    test("2. @cap/css-what — All attribute operators & case flags", () => {
        let toks: [16]CssSelectorToken = undefined;
        const sel = "[disabled][a=1][b^='hi'][c$=\"lo\"][d*=mid][e!=no][f~=word][g|=en S]";
        const n = parseCssSelector(sel, toks[0..16]);
        expect_eq(n, 8 as i64);

        expect_eq(toks[0].action == .Exists, true);
        expect_eq(toks[1].action == .Equals, true);
        expect_eq(std.mem.eql(u8, toks[1].value, "1"), true);
        expect_eq(toks[2].action == .Start, true);
        expect_eq(std.mem.eql(u8, toks[2].value, "hi"), true);
        expect_eq(toks[3].action == .End, true);
        expect_eq(toks[4].action == .Any, true);
        expect_eq(toks[5].action == .Not, true);
        expect_eq(toks[6].action == .Element, true);
        expect_eq(toks[7].action == .Hyphen, true);
        expect_eq(toks[7].ignore_case, false);
    });

    test("3. @cap/css-what — Descendant, universal & nested functional pseudo", () => {
        let toks: [16]CssSelectorToken = undefined;
        const n = parseCssSelector("* :is(ul > li:nth-child(2n+1))", toks[0..16]);
        expect_eq(n, 3 as i64);
        expect_eq(toks[0].kind == .Universal, true);
        expect_eq(toks[1].kind == .Descendant, true);
        expect_eq(toks[2].kind == .Pseudo, true);
        expect_eq(std.mem.eql(u8, toks[2].name, "is"), true);
        expect_eq(std.mem.eql(u8, toks[2].value, "ul > li:nth-child(2n+1)"), true);
    });

    test("4. @cap/css-what — stringifyCssSelector round-trip", () => {
        let toks: [32]CssSelectorToken = undefined;
        const sel = "div#app.btn[data-x^=\"foo\" i]:not(.hidden) > span + a ~ p, h1::before";
        const n = parseCssSelector(sel, toks[0..32]);
        expect_eq(n > 0, true);

        let buf: [256]u8 = undefined;
        const len = stringifyCssSelector(toks[0..(n as usize)], buf[0..256]);
        expect_eq(std.mem.eql(u8, buf[0..len], sel), true);
    });

    test("5. @cap/css-what — Syntax error rejection", () => {
        let toks: [16]CssSelectorToken = undefined;
        expect_eq(parseCssSelector("", toks[0..16]), 0 as i64);
        expect_eq(parseCssSelector("div >", toks[0..16]), -1 as i64);
        expect_eq(parseCssSelector("> div", toks[0..16]), -1 as i64);
        expect_eq(parseCssSelector("div,,p", toks[0..16]), -1 as i64);
        expect_eq(parseCssSelector("[data-x=\"unclosed]", toks[0..16]), -1 as i64);
        expect_eq(parseCssSelector(":not(.unclosed", toks[0..16]), -1 as i64);
        expect_eq(parseCssSelector("#", toks[0..16]), -1 as i64);
        expect_eq(parseCssSelector("[a=b z]", toks[0..16]), -1 as i64);
    });

    test("6. @cap/css-what — Output buffer overflow guards", () => {
        let small: [2]CssSelectorToken = undefined;
        expect_eq(parseCssSelector("div > span", small[0..2]), -1 as i64);

        let toks: [8]CssSelectorToken = undefined;
        const n = parseCssSelector("div#main.active", toks[0..8]);
        expect_eq(n, 3 as i64);
        let tiny: [4]u8 = undefined;
        expect_eq(stringifyCssSelector(toks[0..3], tiny[0..4]), 0 as usize);
    });
}
