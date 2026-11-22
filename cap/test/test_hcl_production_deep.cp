// Source: cp/cap/test/test_hcl_production_deep.cp (Production deep tests for cap:hcl)

import { test, expect_eq } from "cap:test";
import {
    HclTokenKind,
    HclToken,
    HclIter,
    HclBlockHeader,
    HclBlockIter,
    validateHcl,
} from "cap:conf";

const SAMPLE_TF =
    "# Terraform config\n" ++
    "// line comment\n" ++
    "/* block comment */\n" ++
    "variable \"region\" {\n" ++
    "  default = \"us-east-1\"\n" ++
    "}\n\n" ++
    "resource \"aws_s3_bucket\" \"assets\" {\n" ++
    "  bucket = \"my-${var.env == \"prod\" ? \"live\" : \"dev\"}-bucket\"\n" ++
    "  acl    = null\n" ++
    "  force_destroy = true\n" ++
    "  count = 3\n" ++
    "  tags = {\n" ++
    "    Name = \"assets\"\n" ++
    "  }\n" ++
    "  policy = <<-EOF\n" ++
    "    {\"Version\": \"2012-10-17\"}\n" ++
    "  EOF\n" ++
    "}\n";

fn main() {
    test("1. HclIter scans comments, idents, literals, interpolation strings and indented heredocs", () => {
        const src =
            "# hash comment\n" ++
            "name = \"hello-${lookup(m, \"k\")}\"\n" ++
            "flag = false\n" ++
            "empty = null\n" ++
            "num = -42.5e1\n" ++
            "doc = <<-END\n" ++
            "  line 1\n" ++
            "  END\n";

        let it = HclIter.init(src);
        expect_eq(it.next().?.lexeme, "name");
        expect_eq(it.next().?.kind, HclTokenKind.equals);
        const s_tok = it.next().?;
        expect_eq(s_tok.kind, HclTokenKind.string);
        expect_eq(s_tok.lexeme, "\"hello-${lookup(m, \"k\")}\"");

        * = it.next();
        * = it.next();
        const b_tok = it.next().?;
        expect_eq(b_tok.kind, HclTokenKind.bool_lit);
        expect_eq(b_tok.lexeme, "false");

        * = it.next();
        * = it.next();
        const n_tok = it.next().?;
        expect_eq(n_tok.kind, HclTokenKind.null_lit);
        expect_eq(n_tok.lexeme, "null");

        * = it.next();
        * = it.next();
        const num_tok = it.next().?;
        expect_eq(num_tok.kind, HclTokenKind.number);
        expect_eq(num_tok.lexeme, "-42.5e1");

        * = it.next();
        * = it.next();
        const hd_tok = it.next().?;
        expect_eq(hd_tok.kind, HclTokenKind.heredoc);
        expect_eq(it.next() == null, true);
    });

    test("2. HclBlockIter iterates top-level blocks and skips nested blocks/attributes", () => {
        const tf =
            "top_attr = [1, 2, 3]\n" ++
            "variable \"region\" {\n" ++
            "  default = \"us-east-1\"\n" ++
            "}\n" ++
            "resource \"aws_instance\" \"web\" {\n" ++
            "  nested_block \"inner\" {\n" ++
            "    k = \"v\"\n" ++
            "  }\n" ++
            "  count = 2\n" ++
            "}\n" ++
            "locals {\n" ++
            "  env = \"prod\"\n" ++
            "}\n";

        let it = HclBlockIter.init(tf);

        const b1_opt = it.next();
        expect_eq(b1_opt != null, true);
        if (b1 := b1_opt) {
            expect_eq(b1.block_type, "variable");
            expect_eq(b1.label1.?, "region");
            expect_eq(b1.label2 == null, true);
        }

        const b2_opt = it.next();
        expect_eq(b2_opt != null, true);
        if (b2 := b2_opt) {
            expect_eq(b2.block_type, "resource");
            expect_eq(b2.label1.?, "aws_instance");
            expect_eq(b2.label2.?, "web");
            expect_eq(b2.body.len > 10, true);
        }

        const b3_opt = it.next();
        expect_eq(b3_opt != null, true);
        if (b3 := b3_opt) {
            expect_eq(b3.block_type, "locals");
            expect_eq(b3.label1 == null, true);
            expect_eq(b3.label2 == null, true);
        }

        expect_eq(it.next() == null, true);
    });

    test("3. validateHcl validates balanced HCL and rejects unclosed heredocs/strings/comments/brackets", () => {
        expect_eq(validateHcl("resource \"a\" \"b\" { list = [1, (2)] }"), true);
        expect_eq(validateHcl("doc = <<EOF\nhello\nEOF\n"), true);

        // Empty or comment-only
        expect_eq(validateHcl(""), false);
        expect_eq(validateHcl("# only comment\n"), false);

        // Unbalanced brackets
        expect_eq(validateHcl("a = { b = [1, 2 } ]"), false);
        expect_eq(validateHcl("a = { b = 1"), false);
        expect_eq(validateHcl("a = 1 }"), false);

        // Unclosed string / interpolation / heredoc / block comment
        expect_eq(validateHcl("a = \"unclosed"), false);
        expect_eq(validateHcl("a = \"bad ${unclosed\""), false);
        expect_eq(validateHcl("a = <<EOF\nno terminator"), false);
        expect_eq(validateHcl("/* unclosed comment\na = 1"), false);
    });

    test("4. validateHcl enforces 64-level nesting depth limit", () => {
        let ok64: [129]u8 = undefined;
        let i: usize = 0;
        while (i < 64) {
            ok64[i] = '[';
            ok64[128 - i] = ']';
            i += 1;
        }
        ok64[64] = '1';
        expect_eq(validateHcl(&ok64), true);

        let bad65: [131]u8 = undefined;
        let j: usize = 0;
        while (j < 65) {
            bad65[j] = '[';
            bad65[130 - j] = ']';
            j += 1;
        }
        bad65[65] = '1';
        expect_eq(validateHcl(&bad65), false);
    });

    test("5. Standard heredoc <<EOF rejects indented terminator while <<-EOF accepts it", () => {
        const strict_fail = "x = <<EOF\n  hello\n  EOF\n";
        expect_eq(validateHcl(strict_fail), false);

        const indented_ok = "x = <<-EOF\n  hello\n  EOF\n";
        expect_eq(validateHcl(indented_ok), true);

        // Full Terraform sample with interpolation & indented heredoc is valid
        expect_eq(validateHcl(SAMPLE_TF), true);
    });
}
