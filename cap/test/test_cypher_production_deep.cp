// Source: cp/cap/test/test_cypher_production_deep.cp (Production deep tests for cap:cypher)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CYPHER_MAX_DEPTH,
    CypherTokenKind,
    CypherClauseKind,
    CypherToken,
    CypherIter,
    CypherParamIter,
    classifyCypherKeyword,
    validateCypherDelimiters,
    isReadOnlyCypher,
} from "cap:compiler";

test("1. CypherIter tokenizes arrows, strings, backticks, params, numbers, and skips comments", () => {
    const query =
        "// find related concepts\n" ++
        "MATCH (a:`Concept``Node` {id: $node_id})-[r:RELATES_TO*1..3]->(b)<-[:CITES]-(c)\n" ++
        "/* filter with escapes */\n" ++
        "WHERE a.name = 'Alice\\'s \"Graph\"' AND b.score >= 1.5e-2 AND c.mask = 0xFF\n" ++
        "RETURN a.name, $0";

    let it = CypherIter.init(query);

    const t0 = it.next() ?? unreachable;
    expect_eq(t0.kind == CypherTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t0.lexeme, "MATCH"), true);

    const t1 = it.next() ?? unreachable;
    expect_eq(t1.kind == CypherTokenKind.lparen, true);

    const t2 = it.next() ?? unreachable;
    expect_eq(t2.kind == CypherTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t2.lexeme, "a"), true);

    const t3 = it.next() ?? unreachable;
    expect_eq(t3.kind == CypherTokenKind.colon, true);

    const t4 = it.next() ?? unreachable;
    expect_eq(t4.kind == CypherTokenKind.backtick_ident, true);
    expect_eq(std.mem.eql(u8, t4.lexeme, "`Concept``Node`"), true);

    // Scan remaining tokens and verify key kinds appear accurately
    let arrow_right_count: usize = 0;
    let arrow_left_count: usize = 0;
    let param_count: usize = 0;
    let string_count: usize = 0;
    let number_count: usize = 0;
    let found_range_dotdot = false;
    let found_hex = false;
    let found_sci = false;

    while (tok := it.next()) {
        expect_eq(tok.kind != CypherTokenKind.invalid, true);
        if (tok.kind == CypherTokenKind.arrow_right) arrow_right_count += 1;
        if (tok.kind == CypherTokenKind.arrow_left) arrow_left_count += 1;
        if (tok.kind == CypherTokenKind.param) param_count += 1;
        if (tok.kind == CypherTokenKind.string) string_count += 1;
        if (tok.kind == CypherTokenKind.number) {
            number_count += 1;
            if (std.mem.eql(u8, tok.lexeme, "0xFF")) found_hex = true;
            if (std.mem.eql(u8, tok.lexeme, "1.5e-2")) found_sci = true;
        }
        if (tok.kind == CypherTokenKind.punct && std.mem.eql(u8, tok.lexeme, "..")) {
            found_range_dotdot = true;
        }
    }

    expect_eq(arrow_right_count, 1);
    expect_eq(arrow_left_count, 1);
    expect_eq(param_count, 2);
    expect_eq(string_count, 1);
    expect_eq(number_count, 4); // 1, 3, 1.5e-2, 0xFF
    expect_eq(found_range_dotdot, true);
    expect_eq(found_hex, true);
    expect_eq(found_sci, true);
});

test("2. isReadOnlyCypher allows read queries and blocks all write/schema keywords case-insensitively", () => {
    expect_eq(isReadOnlyCypher("MATCH (n:Entity)-[r:LINKS]->(m) WHERE n.id = $id RETURN m.title LIMIT 10"), true);
    expect_eq(isReadOnlyCypher("OPTIONAL MATCH (u:User {active: true}) WITH u UNWIND u.tags AS tag RETURN tag ORDER BY tag"), true);
    expect_eq(isReadOnlyCypher("EXPLAIN MATCH (n) RETURN count(n)"), true);

    // Mutating and schema-changing queries must be rejected regardless of case
    expect_eq(isReadOnlyCypher("CREATE (n:Person {name: 'Bob'}) RETURN n"), false);
    expect_eq(isReadOnlyCypher("match (n) create (m:Temp) return m"), false);
    expect_eq(isReadOnlyCypher("MERGE (n:Doc {id: $id}) RETURN n"), false);
    expect_eq(isReadOnlyCypher("MATCH (n:Doc {id: $id}) DELETE n"), false);
    expect_eq(isReadOnlyCypher("MATCH (n) detach delete n"), false);
    expect_eq(isReadOnlyCypher("MATCH (n) Set n.flag = true RETURN n"), false);
    expect_eq(isReadOnlyCypher("MATCH (n) REMOVE n.temp RETURN n"), false);
    expect_eq(isReadOnlyCypher("DROP INDEX idx_entity_name"), false);
    expect_eq(isReadOnlyCypher("MATCH (n) FOREACH (x IN [1,2] | SET n.v = x)"), false);
    expect_eq(isReadOnlyCypher("CALL { MATCH (n) DELETE n }"), false);
});

test("3. isReadOnlyCypher does not false-positive on strings, comments, backticks, properties, or map keys", () => {
    const safe_query =
        "// CREATE (fake) DELETE x\n" ++
        "/* MERGE DROP SET REMOVE DETACH */\n" ++
        "MATCH (n:CREATE {set: 'DELETE', drop: \"MERGE\", remove: true})\n" ++
        "WHERE n.delete = $create AND n.`DETACH` <> 'SET'\n" ++
        "RETURN n.create, n.set, 'DROP TABLE'";
    expect_eq(isReadOnlyCypher(safe_query), true);

    // Empty, comment-only, and malformed queries must return false
    expect_eq(isReadOnlyCypher(""), false);
    expect_eq(isReadOnlyCypher("   // only comment\n /* block */ "), false);
    expect_eq(isReadOnlyCypher("MATCH (n RETURN n"), false);
    expect_eq(isReadOnlyCypher("MATCH (n) RETURN 'unterminated"), false);
    expect_eq(isReadOnlyCypher("MATCH (n) RETURN `unterminated"), false);
    expect_eq(isReadOnlyCypher("MATCH (n) /* unclosed comment"), false);
});

test("4. CypherParamIter extracts $param names without $ prefix and ignores strings/comments/backticks", () => {
    const query =
        "// $comment_param\n" ++
        "/* $block_param */\n" ++
        "MATCH (a:Node {id: $tenant_id, tag: '$not_a_param', label: \"$also_not\", `$bt_param`: 1})\n" ++
        "WHERE a.score > $min_score AND a.rank = $0\n" ++
        "RETURN a, $limit";

    let pit = CypherParamIter.init(query);

    const p0 = pit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p0, "tenant_id"), true);

    const p1 = pit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p1, "min_score"), true);

    const p2 = pit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p2, "0"), true);

    const p3 = pit.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p3, "limit"), true);

    expect_eq(pit.next() == null, true);
});

test("5. validateCypherDelimiters checks balance, escapes, comments, and enforces 64-level depth cap", () => {
    expect_eq(validateCypherDelimiters("MATCH (a {k: [1, {x: '(unbalanced [}' }]}) // (\n /* [ */ RETURN a"), true);
    expect_eq(validateCypherDelimiters("MATCH (a {s: 'escaped \\' ) ] }'}) RETURN `a(b[c{`"), true);

    // Mismatches and unclosed structures
    expect_eq(validateCypherDelimiters("MATCH (a [b)] RETURN a"), false);
    expect_eq(validateCypherDelimiters("MATCH (a)) RETURN a"), false);
    expect_eq(validateCypherDelimiters("MATCH (a {b: 1} RETURN a"), false);
    expect_eq(validateCypherDelimiters("MATCH (a) RETURN \"unclosed )"), false);
    expect_eq(validateCypherDelimiters("MATCH (a) /* unclosed )"), false);

    // Exact 64-level nesting passes; 65-level nesting is rejected
    let ok_buf: [128]u8 = std.mem.zeroes([128]u8);
    let i: usize = 0;
    while (i < CYPHER_MAX_DEPTH) {
        ok_buf[i] = '(';
        ok_buf[CYPHER_MAX_DEPTH * 2 - 1 - i] = ')';
        i += 1;
    }
    expect_eq(validateCypherDelimiters(ok_buf[0..128]), true);

    let deep_buf: [130]u8 = std.mem.zeroes([130]u8);
    i = 0;
    while (i < CYPHER_MAX_DEPTH + 1) {
        deep_buf[i] = '(';
        deep_buf[(CYPHER_MAX_DEPTH + 1) * 2 - 1 - i] = ')';
        i += 1;
    }
    expect_eq(validateCypherDelimiters(deep_buf[0..130]), false);
});

test("6. classifyCypherKeyword and malformed lexical token edge cases", () => {
    expect_eq(classifyCypherKeyword("match") == CypherClauseKind.read, true);
    expect_eq(classifyCypherKeyword("RETURN") == CypherClauseKind.read, true);
    expect_eq(classifyCypherKeyword("DeLeTe") == CypherClauseKind.write, true);
    expect_eq(classifyCypherKeyword("drop") == CypherClauseKind.schema, true);
    expect_eq(classifyCypherKeyword("CALL") == CypherClauseKind.call, true);
    expect_eq(classifyCypherKeyword("Person") == CypherClauseKind.other, true);

    // Bare '$' without parameter name -> invalid
    let it1 = CypherIter.init("$ ");
    const bad_param = it1.next() ?? unreachable;
    expect_eq(bad_param.kind == CypherTokenKind.invalid, true);

    // Incomplete hex literal -> invalid
    let it2 = CypherIter.init("0x ");
    const bad_hex = it2.next() ?? unreachable;
    expect_eq(bad_hex.kind == CypherTokenKind.invalid, true);

    // Incomplete scientific notation -> invalid
    let it3 = CypherIter.init("1e+");
    const bad_sci = it3.next() ?? unreachable;
    expect_eq(bad_sci.kind == CypherTokenKind.invalid, true);

    // Unclosed block comment -> invalid
    let it4 = CypherIter.init("/* unclosed");
    const bad_comment = it4.next() ?? unreachable;
    expect_eq(bad_comment.kind == CypherTokenKind.invalid, true);
});
