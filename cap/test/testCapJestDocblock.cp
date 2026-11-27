import { test, expect_eq } from "cap:test";
import {
    DocPragma,
    extractDocblock,
    parseDocblock,
    printDocblock,
    extract,
    strip,
    stripSlice,
    parse,
    parseWithComments,
    print,
    printWithComments,
    getPragma,
    hasPragma,
} from "../../packages/jest-docblock/src/index.cp";

fn main() {
    test("1. @cap/jest-docblock — extractDocblock finds top-of-file docblock", () => {
        const src = "/**\n * @jest-environment jsdom\n */\nconst x = 1;";
        const db = extractDocblock(src);
        expect_eq(db, "/**\n * @jest-environment jsdom\n */");
        expect_eq(extract(src), db);
    });

    test("2. @cap/jest-docblock — extractDocblock returns empty when absent or not at top", () => {
        expect_eq(extractDocblock("const x = 1;\n/** @foo bar */"), "");
        expect_eq(extractDocblock("// regular comment\nconst x = 1;"), "");
        expect_eq(extractDocblock(""), "");
    });

    test("3. @cap/jest-docblock — extractDocblock skips leading whitespace and shebang", () => {
        const src = "#!/usr/bin/env node\n\n/** @flow */\nexport default 1;";
        expect_eq(extractDocblock(src), "/** @flow */");
    });

    test("4. @cap/jest-docblock — parseDocblock extracts multiple @key value pragmas", () => {
        const db = "/**\n * @jest-environment jsdom\n * @flow strict\n * @noflow\n */";
        let out: [8]DocPragma = undefined;
        const n = parseDocblock(db, out[0..8]);
        expect_eq(n, 3 as usize);
        expect_eq(out[0].key, "jest-environment");
        expect_eq(out[0].value, "jsdom");
        expect_eq(out[1].key, "flow");
        expect_eq(out[1].value, "strict");
        expect_eq(out[2].key, "noflow");
        expect_eq(out[2].value, "");
    });

    test("5. @cap/jest-docblock — printDocblock formats standard docblock", () => {
        let pragmas: [2]DocPragma = undefined;
        pragmas[0] = DocPragma{ key: "jest-environment", value: "node" };
        pragmas[1] = DocPragma{ key: "flow", value: "" };

        let buf: [128]u8 = undefined;
        const n = printDocblock(pragmas[0..2], buf[0..128]);
        expect_eq(buf[0..n], "/**\n * @jest-environment node\n * @flow\n */");
    });

    test("6. @cap/jest-docblock — printDocblock + parseDocblock roundtrip", () => {
        let orig: [2]DocPragma = undefined;
        orig[0] = DocPragma{ key: "pragma", value: "h" };
        orig[1] = DocPragma{ key: "pragmaFrag", value: "Fragment" };

        let buf: [128]u8 = undefined;
        const written = print(orig[0..2], buf[0..128]);

        let parsed: [4]DocPragma = undefined;
        const count = parse(buf[0..written], parsed[0..4]);
        expect_eq(count, 2 as usize);
        expect_eq(parsed[0].key, "pragma");
        expect_eq(parsed[0].value, "h");
        expect_eq(parsed[1].key, "pragmaFrag");
        expect_eq(parsed[1].value, "Fragment");
    });

    test("7. @cap/jest-docblock — strip removes docblock while preserving shebang & code", () => {
        const src = "/**\n * @flow\n */\nconst a = 1;";
        expect_eq(stripSlice(src), "const a = 1;");

        const with_shebang = "#!/usr/bin/env node\n/** @jest-environment node */\nconsole.log(1);";
        let out: [128]u8 = [0; 128];
        const n = strip(with_shebang, out[0..128]);
        expect_eq(out[0..n] as string, "#!/usr/bin/env node\nconsole.log(1);");
    });

    test("8. @cap/jest-docblock — parseWithComments + printWithComments full roundtrip", () => {
        const db = "/**\n * Copyright 2026 CP\n * Licensed under MIT\n *\n * @jest-environment jsdom\n * @flow\n */";
        let comments: [256]u8 = [0; 256];
        let pragmas: [8]DocPragma = undefined;
        const res = parseWithComments(db, comments[0..256], pragmas[0..8]);
        expect_eq(res.pragma_count, 2 as usize);
        expect_eq(comments[0..res.comments_len] as string, "Copyright 2026 CP\nLicensed under MIT");
        expect_eq(hasPragma(pragmas[0..res.pragma_count], "flow"), true);
        expect_eq(getPragma(pragmas[0..res.pragma_count], "jest-environment") ?? "", "jsdom");
        expect_eq(getPragma(pragmas[0..res.pragma_count], "missing") == null, true);

        let printed: [256]u8 = [0; 256];
        const pn = printWithComments(comments[0..res.comments_len] as string, pragmas[0..res.pragma_count], printed[0..256]);
        expect_eq(printed[0..pn] as string, db);
    });
}
