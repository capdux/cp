import { test, expect, expect_eq } from "cap:test";
import {
    CSS_DOCUMENT_MAX_CONDITIONS,
    CSS_DOCUMENT_MAX_RULES,
    CssDocumentMatchKind,
    CssMozDocumentRule,
    CssMozDocumentRuleList,
} from "cap:css";

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

test("css document_rule: strict Firefox/Bun @-moz-document url-prefix() parse & serialize", fn() {
    const rule = CssMozDocumentRule.parseStrict(
        "/* ff hack */ @-MOZ-DOCUMENT url-prefix() { .only-ff { display: grid; } }",
    ) catch {
        expect(false);
        return;
    };
    expect(rule.isFirefoxHack());
    expect_eq(rule.conditionCount(), 1);
    expect(rule.matchesUrl("https://any.example.org/path"));
    expect(rule.matchesUrl(""));

    let out: [256]u8 = [0; 256];
    const pn = rule.toCss(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..pn], "@-moz-document url-prefix() {\n  .only-ff { display: grid; }\n}"));

    const mn = rule.toCssMinified(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..mn], "@-moz-document url-prefix(){.only-ff { display: grid; }}"));

    const constructed = CssMozDocumentRule.urlPrefixEmpty(".only-ff { display: grid; }");
    expect(rule.eql(constructed));
});

test("css document_rule: multi-condition url(), url-prefix(), domain() & OR matching", fn() {
    const rule = CssMozDocumentRule.parse(
        "@-moz-document url(\"https://a.com/exact\"), url-prefix('https://b.com/docs/'), domain(example.com) { body { color: green; } }",
    ) catch {
        expect(false);
        return;
    };
    expect(!rule.isFirefoxHack());
    expect_eq(rule.conditionCount(), 3);

    // url() 精确匹配
    expect(rule.matchesUrl("https://a.com/exact"));
    expect(!rule.matchesUrl("https://a.com/exact/more"));

    // url-prefix() 前缀匹配
    expect(rule.matchesUrl("https://b.com/docs/intro"));
    expect(!rule.matchesUrl("https://b.com/blog/intro"));

    // domain() 精确与子域名边界安全匹配（自动剥离 userinfo 与 port）
    expect(rule.matchesUrl("https://example.com/"));
    expect(rule.matchesUrl("https://sub.EXAMPLE.com:8443/app?q=1"));
    expect(rule.matchesUrl("http://user:pass@api.example.com:8080/v1"));
    // 严禁误匹配同后缀不同域名
    expect(!rule.matchesUrl("https://evil-example.com/"));
    expect(!rule.matchesUrl("https://example.com.evil.org/"));
});

test("css document_rule: regexp() and media-document() parsing & canonical output", fn() {
    const rule = CssMozDocumentRule.parse(
        "@-moz-document regexp(\"https://.*\\\\.example\\\\.com/.*\"), media-document(all) { img { border: 0; } }",
    ) catch {
        expect(false);
        return;
    };
    expect_eq(rule.conditionCount(), 2);
    if (c0 := rule.conditionAt(0)) {
        expect(c0.kind == CssDocumentMatchKind.Regexp);
    } else {
        expect(false);
    }
    if (c1 := rule.conditionAt(1)) {
        expect(c1.kind == CssDocumentMatchKind.MediaDocument);
        expect(strEq(c1.value, "all"));
    } else {
        expect(false);
    }
    expect(rule.matchesUrl("https://anything.invalid/"));

    let out: [256]u8 = [0; 256];
    const n = rule.toCssMinified(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(
        out[0..n],
        "@-moz-document regexp(\"https://.*\\\\.example\\\\.com/.*\"),media-document(\"all\"){img { border: 0; }}",
    ));
});

test("css document_rule: parseStrict rejects non-empty url-prefix or multi-condition", fn() {
    let ok1 = true;
    * = CssMozDocumentRule.parseStrict("@-moz-document url-prefix(\"https://a.com\") { a {} }") catch {
        ok1 = false;
    };
    expect(!ok1);

    let ok2 = true;
    * = CssMozDocumentRule.parseStrict("@-moz-document domain(example.com) { a {} }") catch {
        ok2 = false;
    };
    expect(!ok2);

    let ok3 = true;
    * = CssMozDocumentRule.parseStrict("@-moz-document url-prefix(), url-prefix() { a {} }") catch {
        ok3 = false;
    };
    expect(!ok3);
});

test("css document_rule: extract multiple rules from stylesheet safely", fn() {
    const sheet =
        "/* @-moz-document url-prefix() { .ignored {} } */\n" ++
        ".note { content: \"@-moz-document url-prefix() { .str {} }\"; }\n" ++
        "@-moz-document url-prefix() { .a { color: red; } }\n" ++
        "@media screen { .b { color: blue; } }\n" ++
        "@-moz-document domain(\"docs.cp.dev\") { .c { font-size: 14px; } }\n";

    const list = CssMozDocumentRuleList.extractFromStylesheet(sheet) catch {
        expect(false);
        return;
    };
    expect_eq(list.count(), 2);

    if (r0 := list.at(0)) {
        expect(r0.isFirefoxHack());
        expect(strEq(r0.body, ".a { color: red; }"));
    } else {
        expect(false);
    }
    if (r1 := list.at(1)) {
        expect(!r1.isFirefoxHack());
        expect(r1.matchesUrl("https://docs.cp.dev/guide"));
        expect(!r1.matchesUrl("https://evil.dev/guide"));
    } else {
        expect(false);
    }
    expect(list.at(2) == null);
});

test("css document_rule: unsupported prefix, malformed conditions & unclosed block guards", fn() {
    // 拒绝无前缀 @document 与其他厂商前缀
    let ok1 = true;
    * = CssMozDocumentRule.parse("@document url-prefix() { a {} }") catch {
        ok1 = false;
    };
    expect(!ok1);

    let ok2 = true;
    * = CssMozDocumentRule.parse("@-webkit-document url-prefix() { a {} }") catch {
        ok2 = false;
    };
    expect(!ok2);

    // 空条件列表
    let ok3 = true;
    * = CssMozDocumentRule.parse("@-moz-document { a {} }") catch {
        ok3 = false;
    };
    expect(!ok3);

    // 未知条件函数 / 空 domain() / 无引号 regexp()
    let ok4 = true;
    * = CssMozDocumentRule.parse("@-moz-document host(example.com) { a {} }") catch {
        ok4 = false;
    };
    expect(!ok4);

    let ok5 = true;
    * = CssMozDocumentRule.parse("@-moz-document domain() { a {} }") catch {
        ok5 = false;
    };
    expect(!ok5);

    let ok6 = true;
    * = CssMozDocumentRule.parse("@-moz-document regexp(abc) { a {} }") catch {
        ok6 = false;
    };
    expect(!ok6);

    // 未闭合块与尾随非法内容
    let ok7 = true;
    * = CssMozDocumentRule.parse("@-moz-document url-prefix() { a {}") catch {
        ok7 = false;
    };
    expect(!ok7);

    let ok8 = true;
    * = CssMozDocumentRule.parse("@-moz-document url-prefix() { a {} } trailing") catch {
        ok8 = false;
    };
    expect(!ok8);
});

test("css document_rule: empty block, buffer-too-small & capacity constants", fn() {
    expect_eq(CSS_DOCUMENT_MAX_CONDITIONS, 16);
    expect_eq(CSS_DOCUMENT_MAX_RULES, 16);

    const empty = CssMozDocumentRule.parse("@-moz-document url-prefix() { }") catch {
        expect(false);
        return;
    };
    let out: [64]u8 = [0; 64];
    const n1 = empty.toCss(out[0..]) catch {
        expect(false);
        return;
    };
    expect(strEq(out[0..n1], "@-moz-document url-prefix() {}"));

    let tiny: [8]u8 = [0; 8];
    let ok_buf = true;
    * = empty.toCssMinified(tiny[0..]) catch {
        ok_buf = false;
    };
    expect(!ok_buf);
});
