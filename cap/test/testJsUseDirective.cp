// Production deep regression tests for lib/js/useDirective.cp (cap:js)

import { test, expect_eq } from "cap:test";
import {
    UseDirectiveBoundering,
    UseDirectiveFlags,
    UseDirective,
    scanUseDirectives,
} from "cap:js";

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    for (i of 0..a.len) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("useDirective: parse double/single/backtick quotes and leading trivia", () => {
    expect_eq((UseDirective.parse("\"use client\";\nexport function App() {}") ?? UseDirective.None) == UseDirective.Client, true);
    expect_eq((UseDirective.parse("'use server';") ?? UseDirective.None) == UseDirective.Server, true);
    expect_eq((UseDirective.parse("`use strict`;") ?? UseDirective.None) == UseDirective.Strict, true);

    // Leading comments, whitespace, and semicolons
    const with_comments = "  // file header\n /* license */ ; \"use client\"\nimport x from 'y';";
    expect_eq((UseDirective.parse(with_comments) ?? UseDirective.None) == UseDirective.Client, true);

    // Leading UTF-8 BOM
    const with_bom = "\xEF\xBB\xBF\"use server\";";
    expect_eq((UseDirective.parse(with_bom) ?? UseDirective.None) == UseDirective.Server, true);
});

test("useDirective: none vs unknown quoted directive distinction", () => {
    // Non-directive code returns .None
    expect_eq((UseDirective.parse("export const x = 1;") ?? UseDirective.Client) == UseDirective.None, true);
    expect_eq((UseDirective.parse("") ?? UseDirective.Client) == UseDirective.None, true);

    // Quoted string expression followed by operator is NOT a directive -> .None
    expect_eq((UseDirective.parse("\"use client\" + x;") ?? UseDirective.Client) == UseDirective.None, true);

    // Unterminated quote -> .None
    expect_eq((UseDirective.parse("\"use client") ?? UseDirective.Client) == UseDirective.None, true);

    // Unrecognized standalone quoted directive returns null (Bun UseDirective.parse parity)
    expect_eq(UseDirective.parse("\"use asm\";") == null, true);
    expect_eq(UseDirective.parse("'use unknown';") == null, true);
});

test("useDirective: boundary transition & boundering checks", () => {
    // None -> Client / Server crosses boundary
    expect_eq(UseDirective.None.isBoundary(.Client), true);
    expect_eq(UseDirective.None.isBoundary(.Server), true);
    expect_eq(UseDirective.None.isBoundary(.None), false);
    expect_eq(UseDirective.None.isBoundary(.Strict), false);

    // Client -> Server crosses to Server; Server -> Client crosses to Client
    expect_eq(UseDirective.Client.isBoundary(.Server), true);
    expect_eq((UseDirective.Client.boundering(.Server) ?? UseDirectiveBoundering.Client) == UseDirectiveBoundering.Server, true);
    expect_eq(UseDirective.Server.isBoundary(.Client), true);
    expect_eq((UseDirective.Server.boundering(.Client) ?? UseDirectiveBoundering.Server) == UseDirectiveBoundering.Client, true);

    // Same directive or dropping to None does NOT create a new boundary
    expect_eq(UseDirective.Client.isBoundary(.Client), false);
    expect_eq(UseDirective.Client.boundering(.Client) == null, true);
    expect_eq(UseDirective.Server.isBoundary(.Server), false);
    expect_eq(UseDirective.Client.boundering(.None) == null, true);
});

test("useDirective: literals and bundler --banner formatting", () => {
    expect_eq(strEq(UseDirective.Client.asLiteral(), "use client"), true);
    expect_eq(strEq(UseDirective.Server.asLiteral(), "use server"), true);
    expect_eq(strEq(UseDirective.Strict.asLiteral(), "use strict"), true);
    expect_eq(strEq(UseDirective.None.asLiteral(), ""), true);

    expect_eq(strEq(UseDirective.Client.asBanner(), "\"use client\";"), true);
    expect_eq(strEq(UseDirective.Server.asBanner(), "\"use server\";"), true);
    expect_eq(strEq(UseDirective.Strict.asBanner(), "\"use strict\";"), true);
    expect_eq(strEq(UseDirective.None.asBanner(), ""), true);
});

test("useDirective: scanUseDirectives multi-directive prologue & flags", () => {
    const src = "// header\n\"use strict\";\n'use client';\nconst a = \"use server\";";
    const flags = scanUseDirectives(src);
    expect_eq(flags.has_use_strict, true);
    expect_eq(flags.has_any_client, true);
    // "use server" inside const statement is NOT part of prologue
    expect_eq(flags.has_any_server, false);
    expect_eq(flags.hasAnyBoundary(), true);

    let manual = UseDirectiveFlags{};
    expect_eq(manual.hasAnyBoundary(), false);
    manual.record(.Strict);
    expect_eq(manual.has_use_strict, true);
    expect_eq(manual.hasAnyBoundary(), false);
    manual.record(.Server);
    expect_eq(manual.has_any_server, true);
    expect_eq(manual.hasAnyBoundary(), true);
});
