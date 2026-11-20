// Source: cp/cap/test/test_graphql_production_deep.cp
// Deep Production Test Suite for cap:graphql & cap.graphql
// Covers:
//   1. GqlLexer comments, comma skipping, all punctuators (incl. `&` Amp & `...` Spread), block strings `"""..."""`, escapes & line/col tracking
//   2. Complex named query (variable defaults, aliases, object/list args, `@include`/`@skip` directives, inline & named fragments, mutation & subscription)
//   3. Configurable depth/field/alias/node/token security guardrails & malformed syntax interception
//   4. GqlPrinter Pretty & Minify lossless parse -> print -> parse & parse -> minify -> parse round-trip equivalence
//   5. `import cap from "cap"; cap.graphql` automatic dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_DEPTH,
    GqlError,
    GqlTokenKind,
    GqlToken,
    GqlLexer,
    GqlParseOptions,
    GqlOpKind,
    GqlValue,
    GqlSelection,
    GqlDocument,
    GqlParser,
    GqlPrintMode,
    GqlPrinter,
} from "cap:graphql";

fn testLexerSpecFeatures(): ! {
    const alloc = std.heap.page_allocator;

    const src =
        "# Leading GraphQL comment\n" ++
        "query GetDoc($a: Int = -42, $b: Float = 3.14e-2, $c: String = \"hi\\n\\u0041\") {\n" ++
        "  user(tags: [\"alpha\",, \"beta\"], filter: { active: true, ratio: 2.0 }) {\n" ++
        "    description: \"\"\"\n" ++
        "      Line one\n" ++
        "        Indented line two\n" ++
        "      \\\"\"\"quoted\\\"\"\"\n" ++
        "    \"\"\"\n" ++
        "    ...UserFields\n" ++
        "  }\n" ++
        "}";

    let lex = GqlLexer.init(src);
    const toks = lex.tokenize(alloc)?;
    defer alloc.free(toks);

    // First token: `query` at line 2, col 1 (comment on line 1 skipped)
    expect_eq(toks[0].kind == .Name, true);
    expect_eq(std.mem.eql(u8, toks[0].lexeme, "query"), true);
    expect_eq(toks[0].line, 2 as u32);
    expect_eq(toks[0].col, 1 as u32);

    // Verify Int (-42), Float (3.14e-2), String ("hi\n\u0041")
    let saw_int = false;
    let saw_float = false;
    let saw_spread = false;
    let saw_block_str = false;
    let single_decoded_ok = false;

    for (tok of toks) {
        if (tok.kind == .Int && std.mem.eql(u8, tok.lexeme, "-42")) {
            saw_int = true;
        }
        if (tok.kind == .Float && std.mem.eql(u8, tok.lexeme, "3.14e-2")) {
            saw_float = true;
        }
        if (tok.kind == .Spread && std.mem.eql(u8, tok.lexeme, "...")) {
            saw_spread = true;
        }
        if (tok.kind == .String) {
            const decoded = GqlLexer.decodeString(alloc, tok.lexeme)?;
            defer alloc.free(decoded);
            if (std.mem.eql(u8, decoded, "hi\nA")) {
                single_decoded_ok = true;
            }
            if (tok.lexeme.len >= 6 && tok.lexeme[0] == '"' && tok.lexeme[1] == '"' && tok.lexeme[2] == '"') {
                if (std.mem.eql(u8, decoded, "Line one\n  Indented line two\n\"\"\"quoted\"\"\"")) {
                    saw_block_str = true;
                }
            }
        }
    }

    expect_eq(saw_int, true);
    expect_eq(saw_float, true);
    expect_eq(saw_spread, true);
    expect_eq(single_decoded_ok, true);
    expect_eq(saw_block_str, true);

    // Commas must be completely ignored (last non-Eof token is RBrace)
    expect_eq(toks[toks.len - 1].kind == .Eof, true);
    expect_eq(toks[toks.len - 2].kind == .RBrace, true);

    // Verify all 14 GraphQL punctuators including `&` (Amp) and `|` (Pipe)
    let p_lex = GqlLexer.init("! $ & ( ) ... : = @ [ ] { | }");
    const p_toks = p_lex.tokenize(alloc)?;
    defer alloc.free(p_toks);
    expect_eq(p_toks.len, 15 as usize); // 14 punctuators + Eof
    expect_eq(p_toks[0].kind == .Bang, true);
    expect_eq(p_toks[1].kind == .Dollar, true);
    expect_eq(p_toks[2].kind == .Amp, true);
    expect_eq(p_toks[3].kind == .LParen, true);
    expect_eq(p_toks[4].kind == .RParen, true);
    expect_eq(p_toks[5].kind == .Spread, true);
    expect_eq(p_toks[6].kind == .Colon, true);
    expect_eq(p_toks[7].kind == .Equals, true);
    expect_eq(p_toks[8].kind == .At, true);
    expect_eq(p_toks[9].kind == .LBracket, true);
    expect_eq(p_toks[10].kind == .RBracket, true);
    expect_eq(p_toks[11].kind == .LBrace, true);
    expect_eq(p_toks[12].kind == .Pipe, true);
    expect_eq(p_toks[13].kind == .RBrace, true);

    // Error boundaries: UnterminatedString, InvalidNumber, UnexpectedToken, MaxTokensExceeded
    let lex_bad_str = GqlLexer.init("query { a(x: \"unclosed\n\") }");
    let hit_unterm = false;
    * = lex_bad_str.tokenize(alloc) catch e => {
        if (e == GqlError.UnterminatedString) hit_unterm = true;
    };
    expect_eq(hit_unterm, true);

    let lex_bad_num = GqlLexer.init("query { a(x: 01) }");
    let hit_bad_num = false;
    * = lex_bad_num.tokenize(alloc) catch e => {
        if (e == GqlError.InvalidNumber) hit_bad_num = true;
    };
    expect_eq(hit_bad_num, true);

    let lex_bad_dot = GqlLexer.init("query { ..Foo }");
    let hit_bad_dot = false;
    * = lex_bad_dot.tokenize(alloc) catch e => {
        if (e == GqlError.UnexpectedToken) hit_bad_dot = true;
    };
    expect_eq(hit_bad_dot, true);

    let lex_limit = GqlLexer.initWithLimit("{ a b c }", 3);
    let hit_max_tok = false;
    * = lex_limit.tokenize(alloc) catch e => {
        if (e == GqlError.MaxTokensExceeded) hit_max_tok = true;
    };
    expect_eq(hit_max_tok, true);
}

fn testComplexQueryMutationSubAndFragments(): ! {
    const alloc = std.heap.page_allocator;

    const src =
        "query FetchWorkspace(\n" ++
        "  $ids: [ID!]!,\n" ++
        "  $limit: Int = 25,\n" ++
        "  $withMeta: Boolean = true,\n" ++
        "  $filter: FilterInput = { status: ACTIVE, tags: [\"core\", \"ai\"], weight: 1.5, note: null }\n" ++
        ") @cached(ttl: 60) {\n" ++
        "  primaryUser: user(ids: $ids, limit: $limit, filter: $filter) {\n" ++
        "    id\n" ++
        "    ...UserCore @include(if: $withMeta)\n" ++
        "    ... on AdminUser @skip(if: false) {\n" ++
        "      roleLevel\n" ++
        "      permissions { scope actions }\n" ++
        "    }\n" ++
        "  }\n" ++
        "}\n" ++
        "fragment UserCore on User {\n" ++
        "  email\n" ++
        "  profile {\n" ++
        "    displayName\n" ++
        "  }\n" ++
        "}\n" ++
        "mutation CreateOrder($input: OrderInput!) {\n" ++
        "  createOrder(input: $input) { orderId total }\n" ++
        "}\n" ++
        "subscription OnOrderPaid($id: ID!) {\n" ++
        "  orderPaid(id: $id) { status paidAt }\n" ++
        "}";

    let doc = GqlParser.parse(alloc, src)?;
    defer doc.deinit();

    expect_eq(doc.operations.len, 3 as usize);
    expect_eq(doc.fragments.len, 1 as usize);

    const op = doc.findOperation("FetchWorkspace").?;
    expect_eq(op.op == .Query, true);
    expect_eq(op.shorthand, false);
    expect_eq(op.var_defs.len, 4 as usize);
    expect_eq(op.directives.len, 1 as usize);
    expect_eq(std.mem.eql(u8, op.directives[0].name, "cached"), true);

    // Check $ids: [ID!]!
    expect_eq(std.mem.eql(u8, op.var_defs[0].name, "ids"), true);
    let ids_type_ok = false;
    switch (op.var_defs[0].var_type) {
        .NonNull outer => {
            switch (outer[0]) {
                .List inner_list => {
                    switch (inner_list[0]) {
                        .NonNull leaf_nn => {
                            switch (leaf_nn[0]) {
                                .Named nm => {
                                    if (std.mem.eql(u8, nm, "ID")) ids_type_ok = true;
                                },
                                else => {},
                            }
                        },
                        else => {},
                    }
                },
                else => {},
            }
        },
        else => {},
    }
    expect_eq(ids_type_ok, true);

    // Check $filter default object
    const filter_obj = op.var_defs[3].default_value.?.asObject().?;
    expect_eq(filter_obj.len, 4 as usize);
    expect_eq(std.mem.eql(u8, filter_obj[0].value.asEnum().?, "ACTIVE"), true);
    expect_eq(filter_obj[1].value.asList().?.len, 2 as usize);
    expect_eq(filter_obj[2].value.asFloat().? == 1.5, true);
    expect_eq(filter_obj[3].value.isNull(), true);

    // Check primaryUser alias, args, @include fragment spread, and ... on AdminUser
    let primary_ok = false;
    switch (op.selections[0]) {
        .Field f => {
            expect_eq(std.mem.eql(u8, f.alias.?, "primaryUser"), true);
            expect_eq(std.mem.eql(u8, f.name, "user"), true);
            expect_eq(f.args.len, 3 as usize);
            expect_eq(std.mem.eql(u8, f.args[0].value.asVariable().?, "ids"), true);
            expect_eq(f.selections.len, 3 as usize);

            switch (f.selections[1]) {
                .FragmentSpread fs => {
                    expect_eq(std.mem.eql(u8, fs.name, "UserCore"), true);
                    expect_eq(fs.directives.len, 1 as usize);
                    expect_eq(std.mem.eql(u8, fs.directives[0].name, "include"), true);
                },
                else => expect_eq(false, true),
            }

            switch (f.selections[2]) {
                .InlineFragment inf => {
                    expect_eq(std.mem.eql(u8, inf.type_condition.?, "AdminUser"), true);
                    expect_eq(inf.directives.len, 1 as usize);
                    expect_eq(inf.selections.len, 2 as usize);
                    primary_ok = true;
                },
                else => expect_eq(false, true),
            }
        },
        else => {},
    }
    expect_eq(primary_ok, true);

    const frag = doc.findFragment("UserCore").?;
    expect_eq(std.mem.eql(u8, frag.type_condition, "User"), true);
    expect_eq(frag.selections.len, 2 as usize);

    const m_op = doc.findOperation("CreateOrder").?;
    expect_eq(m_op.op == .Mutation, true);
    expect_eq(m_op.var_defs.len, 1 as usize);

    const s_op = doc.findOperation("OnOrderPaid").?;
    expect_eq(s_op.op == .Subscription, true);
    expect_eq(s_op.selections.len, 1 as usize);

    // Shorthand anonymous query `{ user { id } }`
    let doc_short = GqlParser.parse(alloc, "{ user { id } }")?;
    defer doc_short.deinit();
    expect_eq(doc_short.operations[0].shorthand, true);
    expect_eq(doc_short.maxSelectionDepth(), 2 as usize);
    expect_eq(doc_short.totalFields(), 2 as usize);
    expect_eq(doc_short.totalAliases(), 0 as usize);
}

fn buildNestedQuery(alloc: std.mem.Allocator, depth: usize): ([]u8)! {
    let buf: std.ArrayList(u8) = .empty;
    errdefer buf.deinit(alloc);

    let i: usize = 0;
    while (i < depth) : (i += 1) {
        buf.appendSlice(alloc, "{ f ") catch return GqlError.OutOfMemory;
    }
    buf.appendSlice(alloc, "leaf") catch return GqlError.OutOfMemory;
    i = 0;
    while (i < depth) : (i += 1) {
        buf.appendSlice(alloc, " }") catch return GqlError.OutOfMemory;
    }
    return buf.toOwnedSlice(alloc) catch return GqlError.OutOfMemory;
}

fn testSecurityGuardsAndMalformedRejection(): ! {
    const alloc = std.heap.page_allocator;

    // 1. Default MAX_DEPTH == 32 passes, 33 fails with MaxDepthExceeded
    const ok_src = buildNestedQuery(alloc, MAX_DEPTH)?;
    defer alloc.free(ok_src);
    let ok_doc = GqlParser.parse(alloc, ok_src)?;
    defer ok_doc.deinit();
    expect_eq(ok_doc.maxSelectionDepth(), MAX_DEPTH);

    const bad_src = buildNestedQuery(alloc, MAX_DEPTH + 1)?;
    defer alloc.free(bad_src);
    let depth_blocked = false;
    * = GqlParser.parse(alloc, bad_src) catch e => {
        if (e == GqlError.MaxDepthExceeded) depth_blocked = true;
    };
    expect_eq(depth_blocked, true);

    // 2. Custom configurable max_depth = 3 (depth 3 ok, depth 4 blocked)
    const custom_depth_opts = GqlParseOptions{ max_depth: 3 };
    let d3_doc = GqlParser.parseWithOptions(alloc, "{ a { b { c } } }", custom_depth_opts)?;
    defer d3_doc.deinit();
    expect_eq(d3_doc.maxSelectionDepth(), 3 as usize);

    let custom_depth_blocked = false;
    * = GqlParser.parseWithOptions(alloc, "{ a { b { c { d } } } }", custom_depth_opts) catch e => {
        if (e == GqlError.MaxDepthExceeded) custom_depth_blocked = true;
    };
    expect_eq(custom_depth_blocked, true);

    // 3. Alias amplification guard (`max_aliases`)
    const alias_opts = GqlParseOptions{ max_aliases: 2 };
    let alias_ok_doc = GqlParser.parseWithOptions(alloc, "{ a1: user { id } a2: user { id } }", alias_opts)?;
    defer alias_ok_doc.deinit();
    expect_eq(alias_ok_doc.totalAliases(), 2 as usize);

    let alias_blocked = false;
    * = GqlParser.parseWithOptions(alloc, "{ a1: u a2: u a3: u }", alias_opts) catch e => {
        if (e == GqlError.MaxAliasesExceeded) alias_blocked = true;
    };
    expect_eq(alias_blocked, true);

    // 4. Field count guard (`max_fields`) & Node count guard (`max_nodes`)
    let fields_blocked = false;
    * = GqlParser.parseWithOptions(alloc, "{ a b c d }", GqlParseOptions{ max_fields: 3 }) catch e => {
        if (e == GqlError.MaxFieldsExceeded) fields_blocked = true;
    };
    expect_eq(fields_blocked, true);

    let nodes_blocked = false;
    * = GqlParser.parseWithOptions(alloc, "{ a(x: [1, 2, 3, 4]) }", GqlParseOptions{ max_nodes: 5 }) catch e => {
        if (e == GqlError.MaxNodesExceeded) nodes_blocked = true;
    };
    expect_eq(nodes_blocked, true);

    // 5. Malformed syntax rejection
    const bad_inputs: [6]string = [
        "",
        "   # comment only",
        "query { }",
        "query($x: Int = $y) { user { id } }",
        "fragment on on User { id }",
        "query Foo { user { id }",
    ];
    for (bad of bad_inputs[0..]) {
        let rejected = false;
        * = GqlParser.parse(alloc, bad) catch e => {
            if (e == GqlError.UnexpectedToken) rejected = true;
        };
        expect_eq(rejected, true);
    }
}

fn testPrettyMinifyAndRoundTrip(): ! {
    const alloc = std.heap.page_allocator;

    const src =
        "# comment ignored on print\n" ++
        "query RewriteTest($x: Int = 10, $y: Float = 2.0, $msg: String = \"hello\\n\\\"cp\\\"\") {\n" ++
        "  me: viewer {\n" ++
        "    id\n" ++
        "    items(filter: { min: 1, max: 5, nums: [1, -2], tags: [\"a\", \"b\"] }) {\n" ++
        "      ...ItemFrag\n" ++
        "      ... on SpecialItem @include(if: true) {\n" ++
        "        badge\n" ++
        "      }\n" ++
        "    }\n" ++
        "  }\n" ++
        "}\n" ++
        "fragment ItemFrag on Item {\n" ++
        "  sku\n" ++
        "  details { weight }\n" ++
        "}";

    let doc1 = GqlParser.parse(alloc, src)?;
    defer doc1.deinit();

    // viewer(1) -> items(2) -> ...ItemFrag -> details(3) -> weight(4)
    expect_eq(doc1.maxSelectionDepth(), 4 as usize);
    expect_eq(doc1.totalFields(), 7 as usize);
    expect_eq(doc1.totalAliases(), 1 as usize);

    // Pretty print round-trip
    const pretty1 = GqlPrinter.printPretty(alloc, doc1)?;
    defer alloc.free(pretty1);

    let doc_pretty = GqlParser.parse(alloc, pretty1)?;
    defer doc_pretty.deinit();
    expect_eq(doc1.eql(doc_pretty), true);

    const pretty2 = GqlPrinter.print(alloc, doc_pretty)?;
    defer alloc.free(pretty2);
    expect_eq(std.mem.eql(u8, pretty1, pretty2), true);

    // Minify print: zero newlines, significantly shorter than pretty, and lossless AST round-trip
    const mini1 = GqlPrinter.minify(alloc, doc1)?;
    defer alloc.free(mini1);

    expect_eq(mini1.len < pretty1.len, true);
    let has_newline = false;
    for (ch of mini1) {
        if (ch == '\n' || ch == '\r') {
            has_newline = true;
            break;
        }
    }
    expect_eq(has_newline, false);

    let doc_mini = GqlParser.parse(alloc, mini1)?;
    defer doc_mini.deinit();
    expect_eq(doc1.eql(doc_mini), true);

    const mini2 = GqlPrinter.printWithMode(alloc, doc_mini, GqlPrintMode.Minify)?;
    defer alloc.free(mini2);
    expect_eq(std.mem.eql(u8, mini1, mini2), true);
}

fn testCapGraphqlDynamicMount(): ! {
    const alloc = std.heap.page_allocator;

    const query_src = "{ agentStatus(id: \"cp-52\") { ready latencyMs } }";
    let doc = cap.graphql.GqlParser.parseWithOptions(alloc, query_src, cap.graphql.GqlParseOptions{ max_depth: 8 })?;
    defer doc.deinit();

    expect_eq(doc.maxSelectionDepth(), 2 as usize);
    const pretty = cap.graphql.GqlPrinter.print(alloc, doc)?;
    defer alloc.free(pretty);

    const mini = cap.graphql.GqlPrinter.minify(alloc, doc)?;
    defer alloc.free(mini);

    let lex = cap.graphql.GqlLexer.init(mini);
    const tok = lex.nextToken()?;
    expect_eq(tok.kind == cap.graphql.GqlTokenKind.LBrace, true);

    let reparsed = cap.graphql.GqlParser.parse(alloc, mini)?;
    defer reparsed.deinit();
    expect_eq(doc.eql(reparsed), true);
}

fn main() {
    test("1. GqlLexer Comments, Commas, All Punctuators, Block Strings & Position Tracking", () => {
        testLexerSpecFeatures()?;
    });

    test("2. Complex Query, Variables, Directives, Aliases, Fragments, Mutation & Subscription", () => {
        testComplexQueryMutationSubAndFragments()?;
    });

    test("3. Configurable Depth/Field/Alias/Node Security Guards & Malformed Rejection", () => {
        testSecurityGuardsAndMalformedRejection()?;
    });

    test("4. GqlPrinter Pretty & Minify Lossless Round-Trip Equivalence", () => {
        testPrettyMinifyAndRoundTrip()?;
    });

    test("5. import cap from \"cap\"; cap.graphql Automatic Dynamic Mounting", () => {
        testCapGraphqlDynamicMount()?;
    });
}
