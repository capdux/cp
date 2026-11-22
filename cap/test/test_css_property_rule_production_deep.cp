// Production Deep Tests for cap:css @property rule (lib/css/property_rule.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_PROPERTY_SYNTAX_COMPONENTS,
    CssPropertyRuleError,
    isValidDashedIdent,
    PropertySyntax,
    CssPropertyRule,
} from "cap:css";

test("css property_rule: standard color/length/multi-branch @property parse & roundtrip", fn() {
    const rule = CssPropertyRule.parse(
        "@property --brand-accent { syntax: '<color>'; inherits: true; initial-value: #3b82f6; }"
    ) catch unreachable;
    expect_eq(rule.name, "--brand-accent");
    expect_eq(rule.inherits, true);
    expect_eq(rule.syntax.isUniversal(), false);
    expect_eq(rule.syntax.len, 1);
    if (iv := rule.initial_value) {
        expect_eq(iv, "#3b82f6");
    } else {
        unreachable;
    }
    expect_eq(rule.acceptsValue("rgb(255, 0, 128)"), true);
    expect_eq(rule.acceptsValue("oklch(0.7 0.15 240)"), true);
    expect_eq(rule.acceptsValue("rebeccapurple"), true);
    expect_eq(rule.acceptsValue("12px"), false);

    let pretty_buf: [256]u8 = undefined;
    const pretty = rule.toCss(pretty_buf[0..]) catch unreachable;
    const expected_pretty =
        "@property --brand-accent {\n  syntax: \"<color>\";\n  inherits: true;\n  initial-value: #3b82f6;\n}";
    expect_eq(pretty, expected_pretty);

    let min_buf: [256]u8 = undefined;
    const minified = rule.toCssMinified(min_buf[0..]) catch unreachable;
    expect_eq(
        minified,
        "@property --brand-accent{syntax:\"<color>\";inherits:true;initial-value:#3b82f6}"
    );

    // Roundtrip re-parse
    const reparsed = CssPropertyRule.parse(minified) catch unreachable;
    expect_eq(rule.eql(reparsed), true);
});

test("css property_rule: universal syntax '*' allows omitted or arbitrary initial-value", fn() {
    const no_init = CssPropertyRule.parse(
        "@PROPERTY --any-token { SYNTAX: \"*\"; INHERITS: FALSE }"
    ) catch unreachable;
    expect_eq(no_init.name, "--any-token");
    expect_eq(no_init.inherits, false);
    expect_eq(no_init.syntax.isUniversal(), true);
    expect_eq(no_init.initial_value == null, true);
    expect_eq(no_init.acceptsValue("anything goes 123px !"), true);

    let min_buf: [128]u8 = undefined;
    const minified = no_init.toCssMinified(min_buf[0..]) catch unreachable;
    expect_eq(minified, "@property --any-token{syntax:\"*\";inherits:false}");

    const with_init = CssPropertyRule.parse(
        "@property --any-with-val { syntax: \"*\"; inherits: true; initial-value: foo bar; }"
    ) catch unreachable;
    if (iv := with_init.initial_value) {
        expect_eq(iv, "foo bar");
    } else {
        unreachable;
    }
});

test("css property_rule: multipliers (+, #), union branches (|), literals, and comments", fn() {
    const rule = CssPropertyRule.parse(
        "/* register spacing list */ @property --gaps { /* order agnostic */ initial-value: 8px 16px 0; inherits: false; syntax: \"<length>+ | auto\"; }"
    ) catch unreachable;
    expect_eq(rule.name, "--gaps");
    expect_eq(rule.syntax.len, 2);
    expect_eq(rule.acceptsValue("10px 20px"), true);
    expect_eq(rule.acceptsValue("auto"), true);
    expect_eq(rule.acceptsValue("10px, 20px"), false);
    expect_eq(rule.acceptsValue("5em"), false); // relative length not computationally independent

    // Comma-separated '#' multiplier with function values containing commas
    const colors = CssPropertyRule.parse(
        "@property --palette { syntax: \"<color>#\"; inherits: true; initial-value: #fff, rgb(10, 20, 30), coral; }"
    ) catch unreachable;
    expect_eq(colors.acceptsValue("red, blue, #00ff00"), true);
    expect_eq(colors.acceptsValue("red, "), false); // trailing comma invalid
    expect_eq(colors.acceptsValue("red blue"), false); // space-separated fails '#'

    // Last-wins duplicate descriptor override (matches Bun PropertyRuleDeclarationParser)
    const dup = CssPropertyRule.parse(
        "@property --dup { syntax: \"<color>\"; syntax: \"<integer>\"; inherits: true; inherits: false; initial-value: 42; }"
    ) catch unreachable;
    expect_eq(dup.inherits, false);
    expect_eq(dup.acceptsValue("-7"), true);
    expect_eq(dup.acceptsValue("3.14"), false);
});

test("css property_rule: dashed-ident validation & malformed rule rejection", fn() {
    expect_eq(isValidDashedIdent("--a"), true);
    expect_eq(isValidDashedIdent("--my-var_123"), true);
    expect_eq(isValidDashedIdent("--"), false);
    expect_eq(isValidDashedIdent("-foo"), false);
    expect_eq(isValidDashedIdent("foo"), false);
    expect_eq(isValidDashedIdent("--bad space"), false);

    // Empty input
    let empty_err = false;
    * = CssPropertyRule.parse("   ") catch { empty_err = true; };
    expect_eq(empty_err, true);

    // Wrong at-rule keyword
    let kw_err = false;
    * = CssPropertyRule.parse("@media screen { syntax: \"*\"; inherits: true; }") catch { kw_err = true; };
    expect_eq(kw_err, true);

    // Invalid property name (not dashed-ident)
    let name_err = false;
    * = CssPropertyRule.parse("@property color { syntax: \"<color>\"; inherits: true; initial-value: red; }") catch { name_err = true; };
    expect_eq(name_err, true);

    // Missing required `syntax`
    let miss_syn = false;
    * = CssPropertyRule.parse("@property --x { inherits: true; initial-value: 10px; }") catch { miss_syn = true; };
    expect_eq(miss_syn, true);

    // Missing required `inherits`
    let miss_inh = false;
    * = CssPropertyRule.parse("@property --x { syntax: \"*\"; }") catch { miss_inh = true; };
    expect_eq(miss_inh, true);

    // Invalid `inherits` value
    let bad_inh = false;
    * = CssPropertyRule.parse("@property --x { syntax: \"*\"; inherits: yes; }") catch { bad_inh = true; };
    expect_eq(bad_inh, true);

    // Unknown descriptor rejected
    let bad_decl = false;
    * = CssPropertyRule.parse("@property --x { syntax: \"*\"; inherits: true; color: red; }") catch { bad_decl = true; };
    expect_eq(bad_decl, true);

    // Unclosed block or trailing garbage
    let bad_block = false;
    * = CssPropertyRule.parse("@property --x { syntax: \"*\"; inherits: true;") catch { bad_block = true; };
    expect_eq(bad_block, true);
});

test("css property_rule: non-universal initial-value strict validation & var()/CSS-wide rejection", fn() {
    // Missing initial-value when syntax != "*"
    let miss_iv = false;
    * = CssPropertyRule.parse("@property --len { syntax: \"<length>\"; inherits: false; }") catch { miss_iv = true; };
    expect_eq(miss_iv, true);

    // Type mismatch: color given for <angle>
    let mismatch = false;
    * = CssPropertyRule.parse("@property --ang { syntax: \"<angle>\"; inherits: false; initial-value: red; }") catch { mismatch = true; };
    expect_eq(mismatch, true);

    // Reject var() in initial-value (not computationally independent)
    let var_err = false;
    * = CssPropertyRule.parse("@property --c { syntax: \"<color>\"; inherits: false; initial-value: var(--other); }") catch { var_err = true; };
    expect_eq(var_err, true);

    // Reject CSS-wide keywords in initial-value
    let wide_err = false;
    * = CssPropertyRule.parse("@property --c { syntax: \"<custom-ident>\"; inherits: false; initial-value: inherit; }") catch { wide_err = true; };
    expect_eq(wide_err, true);

    // Reject multiplier on <transform-list>
    let tf_mult_err = false;
    * = PropertySyntax.parse("<transform-list>+") catch { tf_mult_err = true; };
    expect_eq(tf_mult_err, true);

    // Valid <transform-list>, <angle>, <time>, <resolution>, <percentage>
    const tf = CssPropertyRule.parse(
        "@property --tx { syntax: \"<transform-list>\"; inherits: false; initial-value: translateX(10px) rotate(45deg); }"
    ) catch unreachable;
    expect_eq(tf.acceptsValue("scale(1.5) translate(0, 10px)"), true);

    const ang = CssPropertyRule.parse(
        "@property --rot { syntax: \"<angle>\"; inherits: true; initial-value: 0.25turn; }"
    ) catch unreachable;
    expect_eq(ang.acceptsValue("90deg"), true);
    expect_eq(ang.acceptsValue("0"), true);

    const tm = CssPropertyRule.parse(
        "@property --dur { syntax: \"<time>\"; inherits: false; initial-value: 250ms; }"
    ) catch unreachable;
    expect_eq(tm.acceptsValue("1.5s"), true);
    expect_eq(tm.acceptsValue("0"), false); // CSS <time> requires unit even for 0
});

test("css property_rule: buffer-too-small guard & syntax component capacity limit", fn() {
    const rule = CssPropertyRule.parse(
        "@property --small { syntax: \"<number>\"; inherits: true; initial-value: 1.5; }"
    ) catch unreachable;

    let tiny: [8]u8 = undefined;
    let pretty_oob = false;
    * = rule.toCss(tiny[0..]) catch { pretty_oob = true; };
    expect_eq(pretty_oob, true);

    let min_oob = false;
    * = rule.toCssMinified(tiny[0..]) catch { min_oob = true; };
    expect_eq(min_oob, true);

    // Exceeding MAX_PROPERTY_SYNTAX_COMPONENTS (16) must return TooManySyntaxComponents cleanly
    expect_eq(MAX_PROPERTY_SYNTAX_COMPONENTS, 16);
    let cap_err = false;
    * = PropertySyntax.parse(
        "a | b | c | d | e | f | g | h | i | j | k | l | m | n | o | p | q"
    ) catch { cap_err = true; };
    expect_eq(cap_err, true);
});
