// Source: cp/cap/test/test_cue_production_deep.cp (Production deep tests for cap:cue)

import { test, expect_eq } from "cap:test";
import {
    CueTokenKind,
    CueToken,
    CueIter,
    validateCue,
    extractCuePackage,
} from "cap:conf";

fn main() {
    test("1. CueIter scans definitions, hidden fields, bottom, ellipsis, disjunction/conjunction and attrs", () => {
        const src =
            "// CUE schema\n" ++
            "#Server: {\n" ++
            "  _secret: 'raw_bytes'\n" ++
            "  mode: \"prod\" | \"dev\"\n" ++
            "  merged: #Base & { ... }\n" ++
            "  broken: _|_\n" ++
            "  port: 8080 @protobuf(1, varint)\n" ++
            "}";

        let it = CueIter.init(src);
        const t0 = it.next().?;
        expect_eq(t0.kind, CueTokenKind.def_ident);
        expect_eq(t0.lexeme, "#Server");
        expect_eq(it.next().?.kind, CueTokenKind.colon);
        expect_eq(it.next().?.kind, CueTokenKind.lbrace);

        const t_hid = it.next().?;
        expect_eq(t_hid.kind, CueTokenKind.hidden_ident);
        expect_eq(t_hid.lexeme, "_secret");
        expect_eq(it.next().?.kind, CueTokenKind.colon);
        const t_bytes = it.next().?;
        expect_eq(t_bytes.kind, CueTokenKind.bytes_lit);
        expect_eq(t_bytes.lexeme, "'raw_bytes'");

        // mode: "prod" | "dev"
        * = it.next();
        * = it.next();
        expect_eq(it.next().?.kind, CueTokenKind.string);
        expect_eq(it.next().?.kind, CueTokenKind.pipe);
        expect_eq(it.next().?.kind, CueTokenKind.string);

        // merged: #Base & { ... }
        * = it.next();
        * = it.next();
        expect_eq(it.next().?.kind, CueTokenKind.def_ident);
        expect_eq(it.next().?.kind, CueTokenKind.amp);
        expect_eq(it.next().?.kind, CueTokenKind.lbrace);
        expect_eq(it.next().?.kind, CueTokenKind.ellipsis);
        expect_eq(it.next().?.kind, CueTokenKind.rbrace);

        // broken: _|_
        * = it.next();
        * = it.next();
        const t_bot = it.next().?;
        expect_eq(t_bot.kind, CueTokenKind.bottom);
        expect_eq(t_bot.lexeme, "_|_");

        // port: 8080 @protobuf(1, varint)
        * = it.next();
        * = it.next();
        expect_eq(it.next().?.kind, CueTokenKind.number);
        const t_attr = it.next().?;
        expect_eq(t_attr.kind, CueTokenKind.attr);
        expect_eq(t_attr.lexeme, "@protobuf(1, varint)");
    });

    test("2. CueIter parses underscores, hex/bin/oct, SI/IEC multipliers, bools and null", () => {
        const src = "1_000 10Mi 64Ki 0xff_00 0b1010 -3.5e2 true false null";
        let it = CueIter.init(src);

        expect_eq(it.next().?.lexeme, "1_000");
        expect_eq(it.next().?.lexeme, "10Mi");
        expect_eq(it.next().?.lexeme, "64Ki");
        expect_eq(it.next().?.lexeme, "0xff_00");
        expect_eq(it.next().?.lexeme, "0b1010");
        expect_eq(it.next().?.lexeme, "-3.5e2");
        expect_eq(it.next().?.kind, CueTokenKind.bool_lit);
        expect_eq(it.next().?.kind, CueTokenKind.bool_lit);
        expect_eq(it.next().?.kind, CueTokenKind.null_lit);
        expect_eq(it.next() == null, true);
    });

    test("3. extractCuePackage reads package header with BOM/comments and rejects missing/non-package", () => {
        expect_eq(extractCuePackage("\xEF\xBB\xBF// leading\npackage config\n#A: {}").?, "config");
        expect_eq(extractCuePackage("package k8s_deploy\n").?, "k8s_deploy");
        expect_eq(extractCuePackage("#Schema: { a: 1 }") == null, true);
        expect_eq(extractCuePackage("package") == null, true);
        expect_eq(extractCuePackage("package 123") == null, true);
        expect_eq(extractCuePackage("") == null, true);
    });

    test("4. validateCue checks balanced delimiters, closed strings/attrs, and rejects malformed inputs", () => {
        expect_eq(validateCue("package app\n#Cfg: { items: [1, (2 | 3)], tag: @json(a, \"b)\") }"), true);
        expect_eq(validateCue("msg: \"\"\"\nmultiline\n\"\"\""), true);

        // Empty / comment-only
        expect_eq(validateCue(""), false);
        expect_eq(validateCue("// nothing here\n"), false);

        // Unbalanced delimiters
        expect_eq(validateCue("{ a: [1, 2 } ]"), false);
        expect_eq(validateCue("{ a: 1"), false);
        expect_eq(validateCue("a: 1 }"), false);

        // Unclosed string, bytes, attr, bare #
        expect_eq(validateCue("a: \"unterminated"), false);
        expect_eq(validateCue("a: 'unterminated"), false);
        expect_eq(validateCue("a: 1 @tag(unclosed"), false);
        expect_eq(validateCue("a: #"), false);
    });

    test("5. validateCue enforces 64-level nesting depth protection", () => {
        let ok64: [129]u8 = undefined;
        let i: usize = 0;
        while (i < 64) {
            ok64[i] = '{';
            ok64[128 - i] = '}';
            i += 1;
        }
        ok64[64] = '_';
        expect_eq(validateCue(&ok64), true);

        let bad65: [131]u8 = undefined;
        let j: usize = 0;
        while (j < 65) {
            bad65[j] = '{';
            bad65[130 - j] = '}';
            j += 1;
        }
        bad65[65] = '_';
        expect_eq(validateCue(&bad65), false);
    });
}
