import { test, expect_eq } from "cap:test";
import {
    GQL_TOK_EOF,
    GQL_TOK_DOLLAR,
    GQL_TOK_LPAREN,
    GQL_TOK_RPAREN,
    GQL_TOK_SPREAD,
    GQL_TOK_COLON,
    GQL_TOK_AT,
    GQL_TOK_LBRACE,
    GQL_TOK_RBRACE,
    GQL_TOK_NAME,
    GQL_TOK_INT,
    GQL_TOK_FLOAT,
    GQL_TOK_STRING,
    GQL_TOK_BLOCK_STRING,
    GQL_OP_QUERY,
    GQL_OP_MUTATION,
    GQL_OP_SUBSCRIPTION,
    GqlLexer,
    parseGraphql,
    minifyGraphql,
} from "../../packages/graphql/src/index.cp";

fn strEq(a: []const u8, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn main() {
    test("1. @cap/graphql-parse — GqlLexer tokens, comments, commas, numbers, strings & line/col", () => {
        const src = "# comment line\nquery ($x: Int = -42, $f: Float = 3.14e2)\n{\n  msg(a: \"hi\\n\", b: \"\"\"block\"\"\") ... @dir\n}";
        let lex = GqlLexer.new(src);

        const t1 = lex.nextToken();
        expect_eq(t1.kind, GQL_TOK_NAME);
        expect_eq(strEq(lex.tokenText(t1), "query"), true);
        expect_eq(t1.line, 2);
        expect_eq(t1.col, 1);

        expect_eq(lex.nextToken().kind, GQL_TOK_LPAREN);
        expect_eq(lex.nextToken().kind, GQL_TOK_DOLLAR);
        expect_eq(strEq(lex.tokenText(lex.nextToken()), "x"), true);
        expect_eq(lex.nextToken().kind, GQL_TOK_COLON);
        expect_eq(strEq(lex.tokenText(lex.nextToken()), "Int"), true);
        lex.nextToken(); // '='

        const t_int = lex.nextToken();
        expect_eq(t_int.kind, GQL_TOK_INT);
        expect_eq(strEq(lex.tokenText(t_int), "-42"), true);

        // Comma is ignored; next is $f: Float = 3.14e2 )
        expect_eq(lex.nextToken().kind, GQL_TOK_DOLLAR);
        lex.nextToken(); // f
        lex.nextToken(); // :
        lex.nextToken(); // Float
        lex.nextToken(); // =
        const t_flt = lex.nextToken();
        expect_eq(t_flt.kind, GQL_TOK_FLOAT);
        expect_eq(strEq(lex.tokenText(t_flt), "3.14e2"), true);
        expect_eq(lex.nextToken().kind, GQL_TOK_RPAREN);

        expect_eq(lex.nextToken().kind, GQL_TOK_LBRACE);
        expect_eq(lex.nextToken().kind, GQL_TOK_NAME); // msg
        expect_eq(lex.nextToken().kind, GQL_TOK_LPAREN);
        lex.nextToken(); // a
        lex.nextToken(); // :
        expect_eq(lex.nextToken().kind, GQL_TOK_STRING);
        lex.nextToken(); // b
        lex.nextToken(); // :
        expect_eq(lex.nextToken().kind, GQL_TOK_BLOCK_STRING);
        expect_eq(lex.nextToken().kind, GQL_TOK_RPAREN);
        expect_eq(lex.nextToken().kind, GQL_TOK_SPREAD);
        expect_eq(lex.nextToken().kind, GQL_TOK_AT);
        expect_eq(lex.nextToken().kind, GQL_TOK_NAME); // dir
        expect_eq(lex.nextToken().kind, GQL_TOK_RBRACE);
        expect_eq(lex.nextToken().kind, GQL_TOK_EOF);
    });

    test("2. @cap/graphql-parse — Shorthand query & nested selection depth", () => {
        const res = parseGraphql("{ viewer { id profile { displayName avatarUrl } } }");
        expect_eq(res.ok, true);
        expect_eq(res.operation_count, 1);
        expect_eq(res.fragment_count, 0);
        expect_eq(res.field_count, 5);
        expect_eq(res.max_depth, 3);
        expect_eq(res.primary_op_type, GQL_OP_QUERY);
        expect_eq(res.primary_op_name.len, 0);
    });

    test("3. @cap/graphql-parse — Named query with variables, directives, aliases & complex args", () => {
        const doc = "query GetOrders($uid: ID! = \"1\", $limit: [Int!]!, $flag: Boolean) {\n"
            ++ "  u: user(id: $uid, filter: { active: true, tags: [\"vip\", null], role: ADMIN }) @include(if: $flag) {\n"
            ++ "    id\n"
            ++ "    orders(limit: $limit) { total }\n"
            ++ "  }\n"
            ++ "}";
        const res = parseGraphql(doc);
        expect_eq(res.ok, true);
        expect_eq(res.operation_count, 1);
        expect_eq(res.field_count, 4);
        expect_eq(res.max_depth, 3);
        expect_eq(res.primary_op_type, GQL_OP_QUERY);
        expect_eq(strEq(res.primary_op_name, "GetOrders"), true);
    });

    test("4. @cap/graphql-parse — Mutations, subscriptions, inline fragments & named fragments", () => {
        const doc = "mutation UpdateUser($in: UserInput!) {\n"
            ++ "  updateUser(input: $in) {\n"
            ++ "    ...UserFields\n"
            ++ "    ... on AdminUser { permissions }\n"
            ++ "  }\n"
            ++ "}\n"
            ++ "subscription OnUserChanged { userChanged { id } }\n"
            ++ "fragment UserFields on User { id email }";
        const res = parseGraphql(doc);
        expect_eq(res.ok, true);
        expect_eq(res.operation_count, 2);
        expect_eq(res.fragment_count, 1);
        expect_eq(res.field_count, 6);
        expect_eq(res.primary_op_type, GQL_OP_MUTATION);
        expect_eq(strEq(res.primary_op_name, "UpdateUser"), true);

        const sub_res = parseGraphql("subscription LiveFeed { events { id } }");
        expect_eq(sub_res.ok, true);
        expect_eq(sub_res.primary_op_type, GQL_OP_SUBSCRIPTION);
        expect_eq(strEq(sub_res.primary_op_name, "LiveFeed"), true);
    });

    test("5. @cap/graphql-parse — Syntax error detection & error_pos reporting", () => {
        // Empty document
        expect_eq(parseGraphql("   # only comment").ok, false);
        // Empty selection set
        expect_eq(parseGraphql("query Bad { }").ok, false);
        // Missing closing brace
        const r_unclosed = parseGraphql("{ user { id }");
        expect_eq(r_unclosed.ok, false);
        expect_eq(r_unclosed.error_pos > 0, true);
        // Variable used in const default value
        expect_eq(parseGraphql("query Bad($a: Int = $b) { x }").ok, false);
        // Fragment named 'on' is forbidden
        expect_eq(parseGraphql("fragment on on User { id }").ok, false);
        // Unterminated string
        expect_eq(parseGraphql("{ user(name: \"oops) { id } }").ok, false);
    });

    test("6. @cap/graphql-parse — minifyGraphql strips comments/commas & preserves separators/strings", () => {
        const doc = "# fetch user\nquery   GetUser ( $id : ID! = \"1\", ) {\n  u: user(id: $id, label: \"hello   world\") {\n    id, name\n    ...UserFields\n  }\n}\nfragment UserFields on User { email }";
        let out: [512]u8 = undefined;
        const n = minifyGraphql(doc, &out);
        const expected = "query GetUser($id:ID!=\"1\"){u:user(id:$id label:\"hello   world\"){id name ...UserFields}}fragment UserFields on User{email}";
        expect_eq(strEq(out[0..n], expected), true);
    });
}
