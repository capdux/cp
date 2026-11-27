import { test, expect_eq } from "cap:test";
import {
    PC_RULE,
    PC_DECL,
    PC_AT_RULE,
    PC_COMMENT,
    PostCssSheet,
} from "../../packages/postcss/src/index.cp";

fn eqBytes(buf: []const u8, len: usize, expected: string): bool {
    if (len != expected.len) { return false; }
    let i: usize = 0;
    while (i < len) {
        if (buf[i] != expected[i]) { return false; }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/postcss-parse — Multi-rule parsing, findDecl & isDeclImportant", () => {
        const css = "body { margin: 0; color: #333; }\n.btn { background: #0070f3 !important; padding: 8px 16px; }";
        const sheet = PostCssSheet.parse(css);
        expect_eq(sheet.ok, true);
        expect_eq(sheet.ruleCount(), 2);
        expect_eq(sheet.declCount(), 4);

        let buf: [64]u8 = undefined;
        const n1 = sheet.findDecl("body", "color", buf[0..64]);
        expect_eq(n1, 4);
        expect_eq(eqBytes(buf[0..64], n1 as usize, "#333"), true);
        expect_eq(sheet.isDeclImportant("body", "color"), false);

        const n2 = sheet.findDecl(".btn", "background", buf[0..64]);
        expect_eq(n2, 7);
        expect_eq(eqBytes(buf[0..64], n2 as usize, "#0070f3"), true);
        expect_eq(sheet.isDeclImportant(".btn", "background"), true);

        expect_eq(sheet.findDecl(".missing", "color", buf[0..64]), -1);
    });

    test("2. @cap/postcss-parse — @charset, @import & nested @media At-Rules", () => {
        const css = "@charset \"UTF-8\";\n@import url(\"reset.css\");\n@media (min-width: 768px) { .grid { display: flex; gap: 16px; } }";
        const sheet = PostCssSheet.parse(css);
        expect_eq(sheet.ok, true);
        expect_eq(sheet.atRuleCount(), 3);
        expect_eq(sheet.ruleCount(), 1);
        expect_eq(sheet.declCount(), 2);
        expect_eq(sheet.nodes[0].kind, PC_AT_RULE);
        expect_eq(eqBytes(sheet.nodes[0].nameStr(), sheet.nodes[0].name_len, "charset"), true);
        expect_eq(eqBytes(sheet.nodes[2].nameStr(), sheet.nodes[2].name_len, "media"), true);

        let buf: [64]u8 = undefined;
        const n = sheet.findDecl(".grid", "display", buf[0..64]);
        expect_eq(n, 4);
        expect_eq(eqBytes(buf[0..64], n as usize, "flex"), true);
    });

    test("3. @cap/postcss-parse — Comment stripping & quoted string '; }' protection", () => {
        const css = "/* header comment */\n.icon::before { /* inline */ content: \";}\"; color: red; }";
        const sheet = PostCssSheet.parse(css);
        expect_eq(sheet.ok, true);
        expect_eq(sheet.commentCount(), 2);
        expect_eq(sheet.comments[0].kind, PC_COMMENT);
        expect_eq(eqBytes(sheet.comments[0].textStr(), sheet.comments[0].text_len, "header comment"), true);
        expect_eq(eqBytes(sheet.comments[1].textStr(), sheet.comments[1].text_len, "inline"), true);

        let buf: [64]u8 = undefined;
        const n1 = sheet.findDecl(".icon::before", "content", buf[0..64]);
        expect_eq(n1, 4);
        expect_eq(eqBytes(buf[0..64], n1 as usize, "\";}\""), true);

        const n2 = sheet.findDecl(".icon::before", "color", buf[0..64]);
        expect_eq(n2, 3);
        expect_eq(eqBytes(buf[0..64], n2 as usize, "red"), true);
    });

    test("4. @cap/postcss-parse — setDecl updates existing & appends new declarations", () => {
        let sheet = PostCssSheet.parse(".card { color: black; }");
        expect_eq(sheet.ok, true);
        expect_eq(sheet.declCount(), 1);

        // Update existing property + set !important
        expect_eq(sheet.setDecl(".card", "color", "navy", true), true);
        expect_eq(sheet.declCount(), 1);

        // Append new property to existing rule
        expect_eq(sheet.setDecl(".card", "border-radius", "8px", false), true);
        expect_eq(sheet.declCount(), 2);

        // Append new rule + property
        expect_eq(sheet.setDecl(".badge", "font-size", "12px", false), true);
        expect_eq(sheet.ruleCount(), 2);
        expect_eq(sheet.declCount(), 3);

        let buf: [64]u8 = undefined;
        const n1 = sheet.findDecl(".card", "color", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n1 as usize, "navy"), true);
        expect_eq(sheet.isDeclImportant(".card", "color"), true);

        const n2 = sheet.findDecl(".badge", "font-size", buf[0..64]);
        expect_eq(eqBytes(buf[0..64], n2 as usize, "12px"), true);
    });

    test("5. @cap/postcss-parse — toCss round-trip serialization", () => {
        let sheet = PostCssSheet.parse("@import \"a.css\"; .box { width: 100px !important; height: 50px; }");
        expect_eq(sheet.ok, true);
        let out: [256]u8 = undefined;
        const n = sheet.toCss(out[0..256]);
        expect_eq(n > 0, true);

        // Re-parse the serialized CSS and verify semantic equivalence
        const reparsed = PostCssSheet.parse(out[0..n]);
        expect_eq(reparsed.ok, true);
        expect_eq(reparsed.atRuleCount(), 1);
        expect_eq(reparsed.ruleCount(), 1);
        expect_eq(reparsed.declCount(), 2);
        expect_eq(reparsed.isDeclImportant(".box", "width"), true);

        let vbuf: [64]u8 = undefined;
        const vn = reparsed.findDecl(".box", "height", vbuf[0..64]);
        expect_eq(eqBytes(vbuf[0..64], vn as usize, "50px"), true);
        expect_eq(PC_RULE == 1 && PC_DECL == 2, true);
    });

    test("6. @cap/postcss-parse — Empty CSS & malformed unclosed braces handled safely", () => {
        const empty = PostCssSheet.parse("   \n\t  ");
        expect_eq(empty.ok, true);
        expect_eq(empty.ruleCount(), 0);
        expect_eq(empty.declCount(), 0);

        const unclosed = PostCssSheet.parse(".broken { color: red;");
        expect_eq(unclosed.ok, false);

        const unclosed_comment = PostCssSheet.parse("/* never closed");
        expect_eq(unclosed_comment.ok, false);
    });
}
