// Source: cp/cap/test/test_sparql_production_deep.cp (Production deep tests for cap:sparql)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    SparqlQueryForm,
    SparqlPrefix,
    SparqlTokenKind,
    SparqlToken,
    SparqlIter,
    SparqlPrefixIter,
    detectSparqlQueryForm,
    isReadOnlySparql,
    validateSparqlDelimiters,
} from "cap:compiler";

const SAMPLE_SELECT: []const u8 =
    "# Knowledge graph query with BASE & multiple PREFIXes\n" ++
    "BASE <http://example.org/graph/>\n" ++
    "PREFIX : <http://example.org/default#>\n" ++
    "prefix foaf: <http://xmlns.com/foaf/0.1/>\n" ++
    "Prefix schema-org: <https://schema.org/>\n" ++
    "SELECT ?person $score\n" ++
    "WHERE {\n" ++
    "  ?person a foaf:Person ;\n" ++
    "          foaf:name \"Alice \\\"Wonder\\\"\"@en ;\n" ++
    "          :bio '''multi-line\n" ++
    "bio with { [ ( unbalanced in string''' ;\n" ++
    "          schema-org:rating 4.75e0 .\n" ++
    "  FILTER ($score >= 10 && $score < 100)\n" ++
    "}\n";

test("1. SparqlIter lexes comments, IRIs, ?/$ vars, short/triple strings, numbers, and delimiters", () => {
    const src =
        "# leading comment\n" ++
        "SELECT ?s $o <http://example.org/p> \"hi \\\"world\\\"\" \"\"\"triple\nline\"\"\" 42 3.14 .5 1.2e-3 { ( [ ] ) } ; , . <= <";
    let it = SparqlIter.init(src);

    const t0 = it.next() ?? unreachable;
    expect_eq(t0.kind == SparqlTokenKind.ident, true);
    expect_eq(std.mem.eql(u8, t0.lexeme, "SELECT"), true);

    const t1 = it.next() ?? unreachable;
    expect_eq(t1.kind == SparqlTokenKind.var_name, true);
    expect_eq(std.mem.eql(u8, t1.lexeme, "?s"), true);

    const t2 = it.next() ?? unreachable;
    expect_eq(t2.kind == SparqlTokenKind.var_name, true);
    expect_eq(std.mem.eql(u8, t2.lexeme, "$o"), true);

    const t3 = it.next() ?? unreachable;
    expect_eq(t3.kind == SparqlTokenKind.iri, true);
    expect_eq(std.mem.eql(u8, t3.lexeme, "<http://example.org/p>"), true);

    const t4 = it.next() ?? unreachable;
    expect_eq(t4.kind == SparqlTokenKind.string, true);
    expect_eq(std.mem.eql(u8, t4.lexeme, "\"hi \\\"world\\\"\""), true);

    const t5 = it.next() ?? unreachable;
    expect_eq(t5.kind == SparqlTokenKind.string, true);
    expect_eq(std.mem.eql(u8, t5.lexeme, "\"\"\"triple\nline\"\"\""), true);

    const n0 = it.next() ?? unreachable;
    expect_eq(n0.kind == SparqlTokenKind.number, true);
    expect_eq(std.mem.eql(u8, n0.lexeme, "42"), true);

    const n1 = it.next() ?? unreachable;
    expect_eq(n1.kind == SparqlTokenKind.number, true);
    expect_eq(std.mem.eql(u8, n1.lexeme, "3.14"), true);

    const n2 = it.next() ?? unreachable;
    expect_eq(n2.kind == SparqlTokenKind.number, true);
    expect_eq(std.mem.eql(u8, n2.lexeme, ".5"), true);

    const n3 = it.next() ?? unreachable;
    expect_eq(n3.kind == SparqlTokenKind.number, true);
    expect_eq(std.mem.eql(u8, n3.lexeme, "1.2e-3"), true);

    expect_eq((it.next() ?? unreachable).kind == SparqlTokenKind.lbrace, true);
    expect_eq((it.next() ?? unreachable).kind == SparqlTokenKind.lparen, true);
    expect_eq((it.next() ?? unreachable).kind == SparqlTokenKind.lbracket, true);
    expect_eq((it.next() ?? unreachable).kind == SparqlTokenKind.rbracket, true);
    expect_eq((it.next() ?? unreachable).kind == SparqlTokenKind.rparen, true);
    expect_eq((it.next() ?? unreachable).kind == SparqlTokenKind.rbrace, true);
    expect_eq((it.next() ?? unreachable).kind == SparqlTokenKind.semicolon, true);
    expect_eq((it.next() ?? unreachable).kind == SparqlTokenKind.comma, true);
    expect_eq((it.next() ?? unreachable).kind == SparqlTokenKind.dot, true);

    const op0 = it.next() ?? unreachable;
    expect_eq(op0.kind == SparqlTokenKind.punct, true);
    expect_eq(std.mem.eql(u8, op0.lexeme, "<="), true);

    const op1 = it.next() ?? unreachable;
    expect_eq(op1.kind == SparqlTokenKind.punct, true);
    expect_eq(std.mem.eql(u8, op1.lexeme, "<"), true);

    expect_eq(it.next() == null, true);
});

test("2. SparqlPrefixIter extracts default and named prefixes case-insensitively", () => {
    let it = SparqlPrefixIter.init(SAMPLE_SELECT);

    const p0 = it.next() ?? unreachable;
    expect_eq(p0.alias.len, 0);
    expect_eq(std.mem.eql(u8, p0.iri, "http://example.org/default#"), true);

    const p1 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p1.alias, "foaf"), true);
    expect_eq(std.mem.eql(u8, p1.iri, "http://xmlns.com/foaf/0.1/"), true);

    const p2 = it.next() ?? unreachable;
    expect_eq(std.mem.eql(u8, p2.alias, "schema-org"), true);
    expect_eq(std.mem.eql(u8, p2.iri, "https://schema.org/"), true);

    expect_eq(it.next() == null, true);

    // Ensure PREFIX inside string or comment is ignored
    const tricked = "# PREFIX fake: <http://fake/>\nSELECT \"PREFIX nope: <http://nope/>\" WHERE {}";
    let it2 = SparqlPrefixIter.init(tricked);
    expect_eq(it2.next() == null, true);
});

test("3. detectSparqlQueryForm classifies all read-only forms and update forms after BASE/PREFIX", () => {
    expect_eq(detectSparqlQueryForm(SAMPLE_SELECT) == SparqlQueryForm.select, true);
    expect_eq(detectSparqlQueryForm("PREFIX ex: <http://ex/> ask { ?s ex:p ?o }") == SparqlQueryForm.ask, true);
    expect_eq(detectSparqlQueryForm("BASE <http://b/> CONSTRUCT { ?s ?p ?o } WHERE { ?s ?p ?o }") == SparqlQueryForm.construct, true);
    expect_eq(detectSparqlQueryForm("describe <http://example.org/alice>") == SparqlQueryForm.describe, true);

    expect_eq(detectSparqlQueryForm("PREFIX ex: <http://ex/> INSERT DATA { ex:a ex:b ex:c }") == SparqlQueryForm.update, true);
    expect_eq(detectSparqlQueryForm("DELETE WHERE { ?s ?p ?o }") == SparqlQueryForm.update, true);
    expect_eq(detectSparqlQueryForm("LOAD <http://example.org/data.ttl>") == SparqlQueryForm.update, true);
    expect_eq(detectSparqlQueryForm("CLEAR GRAPH <http://example.org/g>") == SparqlQueryForm.update, true);
    expect_eq(detectSparqlQueryForm("DROP ALL") == SparqlQueryForm.update, true);
    expect_eq(detectSparqlQueryForm("CREATE SILENT GRAPH <http://example.org/g>") == SparqlQueryForm.update, true);

    expect_eq(detectSparqlQueryForm("   # only comment\n") == SparqlQueryForm.unknown, true);
    expect_eq(detectSparqlQueryForm("FOOBAR ?x") == SparqlQueryForm.unknown, true);
});

test("4. isReadOnlySparql enforces read-only policy and blocks update or malformed queries", () => {
    expect_eq(isReadOnlySparql(SAMPLE_SELECT), true);
    expect_eq(isReadOnlySparql("ASK { ?s a :Node ; :label \"DROP ALL\" }"), true);
    expect_eq(isReadOnlySparql("CONSTRUCT { ?s ?p ?o } WHERE { ?s ?p ?o }"), true);
    expect_eq(isReadOnlySparql("DESCRIBE ?s WHERE { ?s a :Item }"), true);

    expect_eq(isReadOnlySparql("INSERT DATA { <http://a> <http://b> <http://c> }"), false);
    expect_eq(isReadOnlySparql("DELETE DATA { <http://a> <http://b> <http://c> }"), false);
    expect_eq(isReadOnlySparql("DROP ALL"), false);
    expect_eq(isReadOnlySparql("SELECT ?s WHERE { ?s ?p ?o } ; DROP ALL"), false);
    expect_eq(isReadOnlySparql("SELECT ?s WHERE { ?s :p \"unclosed"), false);
    expect_eq(isReadOnlySparql(""), false);
});

test("5. validateSparqlDelimiters handles string/IRI/comment awareness, mismatches, and 64-depth limit", () => {
    expect_eq(validateSparqlDelimiters(SAMPLE_SELECT), true);
    // Unbalanced brackets inside comment, string, and IRI-like text are ignored
    expect_eq(validateSparqlDelimiters("SELECT ?x WHERE { # {{((\n ?x :p \"(([\" ; :q '''}]]''' }"), true);

    // Mismatched / unclosed delimiters & literals
    expect_eq(validateSparqlDelimiters("SELECT ?x WHERE { ( [ ) ] }"), false);
    expect_eq(validateSparqlDelimiters("SELECT ?x WHERE { ?x ?y ?z"), false);
    expect_eq(validateSparqlDelimiters("SELECT ?x WHERE { ?x ?y ?z } }"), false);
    expect_eq(validateSparqlDelimiters("SELECT ?x WHERE { ?x :p \"unterminated }"), false);
    expect_eq(validateSparqlDelimiters("SELECT ?x WHERE { ?x :p <http://unclosed }"), false);

    // Exact 64-level nesting succeeds; 65-level nesting is rejected
    let ok64: [128]u8 = undefined;
    let i: usize = 0;
    while (i < 64) {
        ok64[i] = '{';
        ok64[127 - i] = '}';
        i += 1;
    }
    expect_eq(validateSparqlDelimiters(ok64[0..128]), true);

    let bad65: [130]u8 = undefined;
    let j: usize = 0;
    while (j < 65) {
        bad65[j] = '(';
        bad65[129 - j] = ')';
        j += 1;
    }
    expect_eq(validateSparqlDelimiters(bad65[0..130]), false);
});
