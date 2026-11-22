// Source: cp/cap/test/test_uritemplate_production_deep.cp
// Comprehensive Production-Grade Verification Suite for cap:uritemplate & cap.uritemplate

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    UriTemplateError,
    TemplateOp,
    AssocPair,
    TemplateVars,
    UriTemplate,
} from "cap:web";

fn expectExpanded(
    alloc: std.mem.Allocator,
    pattern: []const u8,
    vars: *TemplateVars,
    expected: []const u8,
): ! {
    let tmpl = UriTemplate.parse(alloc, pattern)?;
    defer tmpl.deinit();
    const out = tmpl.expand(alloc, vars)?;
    defer alloc.free(out);
    expect_eq(std.mem.eql(u8, out, expected), true);
}

fn testRfc6570Levels1To3(): ! {
    const alloc = std.heap.page_allocator;
    let vars = TemplateVars.init(alloc);
    defer vars.deinit();

    vars.set("var", "value")?;
    vars.set("hello", "Hello World!")?;
    vars.set("path", "/foo/bar")?;
    vars.set("x", "1024")?;
    vars.set("y", "768")?;
    vars.set("empty", "")?;

    // Level 1: Simple expansion
    expectExpanded(alloc, "{var}", &vars, "value")?;
    expectExpanded(alloc, "{hello}", &vars, "Hello%20World%21")?;

    // Level 2: Reserved (+) and Fragment (#)
    expectExpanded(alloc, "{+path}/here", &vars, "/foo/bar/here")?;
    expectExpanded(alloc, "{+hello}", &vars, "Hello%20World!")?;
    expectExpanded(alloc, "{#var}", &vars, "#value")?;
    expectExpanded(alloc, "{#hello}", &vars, "#Hello%20World!")?;

    // Level 3: Label (.), Path (/), PathParam (;), Query (?), QueryCont (&)
    expectExpanded(alloc, "X{.var}", &vars, "X.value")?;
    expectExpanded(alloc, "{/var,x}", &vars, "/value/1024")?;
    expectExpanded(alloc, "{;x,y,empty}", &vars, ";x=1024;y=768;empty")?;
    expectExpanded(alloc, "{?x,y,empty}", &vars, "?x=1024&y=768&empty=")?;
    expectExpanded(alloc, "{&x,y}", &vars, "&x=1024&y=768")?;

    // Undefined variables must be skipped cleanly without stray separators
    expectExpanded(alloc, "{?undef1,y,undef2}", &vars, "?y=768")?;
    expectExpanded(alloc, "{/undef1,x,undef2}", &vars, "/1024")?;
    expectExpanded(alloc, "{?undef1,undef2}", &vars, "")?;
}

fn testRfc6570Level4PrefixAndExplode(): ! {
    const alloc = std.heap.page_allocator;
    let vars = TemplateVars.init(alloc);
    defer vars.deinit();

    vars.set("var", "value")?;
    vars.set("semi", ";")?;
    vars.set("path", "/foo/bar")?;
    vars.set("utf8", "你好世界")?;
    vars.setList("list", ["red", "green", "blue"])?;
    vars.setAssoc("keys", [
        AssocPair{ key: "semi", value: ";" },
        AssocPair{ key: "dot", value: "." },
        AssocPair{ key: "comma", value: "," },
    ])?;

    // Prefix modifier {var:N} truncates BEFORE percent-encoding
    expectExpanded(alloc, "{var:3}", &vars, "val")?;
    expectExpanded(alloc, "{var:30}", &vars, "value")?;
    expectExpanded(alloc, "{semi:1}", &vars, "%3B")?;
    expectExpanded(alloc, "{+path:6}/here", &vars, "/foo/b/here")?;
    expectExpanded(alloc, "{utf8:2}", &vars, "%E4%BD%A0%E5%A5%BD")?;

    // Explode modifier on list (*)
    expectExpanded(alloc, "{list}", &vars, "red,green,blue")?;
    expectExpanded(alloc, "{list*}", &vars, "red,green,blue")?;
    expectExpanded(alloc, "{/list*}", &vars, "/red/green/blue")?;
    expectExpanded(alloc, "{?list}", &vars, "?list=red,green,blue")?;
    expectExpanded(alloc, "{?list*}", &vars, "?list=red&list=green&list=blue")?;

    // Explode modifier on assoc (*)
    expectExpanded(alloc, "{keys}", &vars, "semi,%3B,dot,.,comma,%2C")?;
    expectExpanded(alloc, "{keys*}", &vars, "semi=%3B,dot=.,comma=%2C")?;
    expectExpanded(alloc, "{?keys*}", &vars, "?semi=%3B&dot=.&comma=%2C")?;
}

fn testVariablesExtractionAndDedup(): ! {
    const alloc = std.heap.page_allocator;
    let tmpl = UriTemplate.parse(
        alloc,
        "https://api.cp-lang.org/{version}/users/{user_id}/items{/version}{?user_id,filter:10,tags*}",
    )?;
    defer tmpl.deinit();

    const names = tmpl.variables(alloc)?;
    defer {
        for (n of names) {
            alloc.free(n);
        }
        alloc.free(names);
    }

    expect_eq(names.len, 4 as usize);
    expect_eq(std.mem.eql(u8, names[0], "version"), true);
    expect_eq(std.mem.eql(u8, names[1], "user_id"), true);
    expect_eq(std.mem.eql(u8, names[2], "filter"), true);
    expect_eq(std.mem.eql(u8, names[3], "tags"), true);
}

fn testMalformedTemplatesRejected(): ! {
    const alloc = std.heap.page_allocator;

    let got_unclosed = false;
    * = UriTemplate.parse(alloc, "/users/{id") catch e => {
        if (std.mem.eql(u8, @errorName(e), "UnclosedExpression")) got_unclosed = true;
    };
    expect_eq(got_unclosed, true);

    let got_unexpected_close = false;
    * = UriTemplate.parse(alloc, "/users/id}") catch e => {
        if (std.mem.eql(u8, @errorName(e), "UnexpectedCloseBrace")) got_unexpected_close = true;
    };
    expect_eq(got_unexpected_close, true);

    let got_empty_expr = false;
    * = UriTemplate.parse(alloc, "/users/{}") catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidVarName")) got_empty_expr = true;
    };
    expect_eq(got_empty_expr, true);

    let got_invalid_mod = false;
    * = UriTemplate.parse(alloc, "{var:abc}") catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidModifier")) got_invalid_mod = true;
    };
    expect_eq(got_invalid_mod, true);

    let got_invalid_op = false;
    * = UriTemplate.parse(alloc, "{=var}") catch e => {
        if (std.mem.eql(u8, @errorName(e), "InvalidOperator")) got_invalid_op = true;
    };
    expect_eq(got_invalid_op, true);
}

fn testCapUriTemplateDynamicMount(): ! {
    const alloc = std.heap.page_allocator;

    let tmpl = cap.web.UriTemplate.parse(alloc, "https://mcp.example.com/v1{/tool}{?format,limit:2}")?;
    defer tmpl.deinit();

    let vars = cap.web.TemplateVars.init(alloc);
    defer vars.deinit();
    vars.set("tool", "search docs")?;
    vars.set("format", "json")?;
    vars.set("limit", "100")?;

    const out = tmpl.expand(alloc, &vars)?;
    defer alloc.free(out);
    expect_eq(
        std.mem.eql(u8, out, "https://mcp.example.com/v1/search%20docs?format=json&limit=10"),
        true,
    );
}

fn main() {
    test("1. RFC 6570 Levels 1-3 Classic Vectors & Undefined Variable Skipping", () => {
        testRfc6570Levels1To3()?;
    });
    test("2. RFC 6570 Level 4 Prefix Truncation (:N) & Explode Modifier (*) for List/Assoc", () => {
        testRfc6570Level4PrefixAndExplode()?;
    });
    test("3. Template variables() Extraction & Deduplication", () => {
        testVariablesExtractionAndDedup()?;
    });
    test("4. Malformed URI Template Rejection (Unclosed, Unexpected Brace, Empty, Invalid Modifier/Operator)", () => {
        testMalformedTemplatesRejected()?;
    });
    test("5. import cap from \"cap\"; cap.uritemplate Dynamic Lazy Mounting", () => {
        testCapUriTemplateDynamicMount()?;
    });
}
