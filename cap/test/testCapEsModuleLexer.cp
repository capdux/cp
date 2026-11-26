// cap/test/testCapEsModuleLexer.cp
// Comprehensive unit tests for @cap/es-module-lexer

import { test, expect_eq } from "cap:test";
import {
    ImportKind,
    ImportSpecifier,
    ExportSpecifier,
    parseEsm,
    isFacade,
} from "../../packages/@cap/es-module-lexer/src/index.cp";

fn main() {
    test("1. @cap/es-module-lexer — static imports & side-effect imports", () => {
        const code = "import fs from 'node:fs';\nimport { join, resolve as res } from \"node:path\";\nimport './polyfill.js';\n";
        let imps: [8]ImportSpecifier = undefined;
        let exps: [8]ExportSpecifier = undefined;
        const r = parseEsm(code, imps[0..8], exps[0..8]);
        expect_eq(r.import_count, 3 as usize);
        expect_eq(r.export_count, 0 as usize);
        expect_eq(r.has_module_syntax, true);

        expect_eq(imps[0].kind == .Static, true);
        expect_eq(std.mem.eql(u8, imps[0].specifier(code), "node:fs"), true);
        expect_eq(imps[1].kind == .Static, true);
        expect_eq(std.mem.eql(u8, imps[1].specifier(code), "node:path"), true);
        expect_eq(imps[2].kind == .Static, true);
        expect_eq(std.mem.eql(u8, imps[2].specifier(code), "./polyfill.js"), true);
    });

    test("2. @cap/es-module-lexer — dynamic import() and import.meta", () => {
        const code = "const url = import.meta.url;\nconst mod = await import('./lazy.js');\n";
        let imps: [8]ImportSpecifier = undefined;
        let exps: [8]ExportSpecifier = undefined;
        const r = parseEsm(code, imps[0..8], exps[0..8]);
        expect_eq(r.import_count, 2 as usize);
        expect_eq(r.facade, false);

        expect_eq(imps[0].kind == .ImportMeta, true);
        expect_eq(std.mem.eql(u8, imps[0].specifier(code), "import.meta"), true);

        expect_eq(imps[1].kind == .Dynamic, true);
        expect_eq(std.mem.eql(u8, imps[1].specifier(code), "./lazy.js"), true);
    });

    test("3. @cap/es-module-lexer — export declarations (const/let/var/function/async/class/default)", () => {
        const code = "export const A = 1;\nexport let b = 2;\nexport function foo() {}\nexport async function bar() {}\nexport class Baz {}\nexport default 42;\n";
        let imps: [8]ImportSpecifier = undefined;
        let exps: [8]ExportSpecifier = undefined;
        const r = parseEsm(code, imps[0..8], exps[0..8]);
        expect_eq(r.export_count, 6 as usize);
        expect_eq(r.facade, false);

        expect_eq(std.mem.eql(u8, exps[0].name(code), "A"), true);
        expect_eq(std.mem.eql(u8, exps[1].name(code), "b"), true);
        expect_eq(std.mem.eql(u8, exps[2].name(code), "foo"), true);
        expect_eq(std.mem.eql(u8, exps[3].name(code), "bar"), true);
        expect_eq(std.mem.eql(u8, exps[4].name(code), "Baz"), true);
        expect_eq(std.mem.eql(u8, exps[5].name(code), "default"), true);
    });

    test("4. @cap/es-module-lexer — export { a, b as c } from 'mod' and export * from 'mod'", () => {
        const code = "export { alpha, beta as gamma } from './lib.js';\nexport * from './all.js';\nexport * as ns from './ns.js';\n";
        let imps: [8]ImportSpecifier = undefined;
        let exps: [8]ExportSpecifier = undefined;
        const r = parseEsm(code, imps[0..8], exps[0..8]);
        expect_eq(r.import_count, 3 as usize);
        expect_eq(r.export_count, 3 as usize);
        expect_eq(r.facade, true);

        expect_eq(std.mem.eql(u8, exps[0].name(code), "alpha"), true);
        expect_eq(std.mem.eql(u8, exps[0].localName(code), "alpha"), true);
        expect_eq(std.mem.eql(u8, exps[1].name(code), "gamma"), true);
        expect_eq(std.mem.eql(u8, exps[1].localName(code), "beta"), true);
        expect_eq(std.mem.eql(u8, exps[2].name(code), "ns"), true);

        expect_eq(std.mem.eql(u8, imps[0].specifier(code), "./lib.js"), true);
        expect_eq(std.mem.eql(u8, imps[1].specifier(code), "./all.js"), true);
        expect_eq(std.mem.eql(u8, imps[2].specifier(code), "./ns.js"), true);
    });

    test("5. @cap/es-module-lexer — ignores comments, strings, and property accesses", () => {
        const code = "// import x from 'fake1'\n/* export const fake2 = 1; */\nconst s = \"import y from 'fake3'\";\nobj.import('not-dynamic');\nimport real from './real.js';\n";
        let imps: [8]ImportSpecifier = undefined;
        let exps: [8]ExportSpecifier = undefined;
        const r = parseEsm(code, imps[0..8], exps[0..8]);
        expect_eq(r.import_count, 1 as usize);
        expect_eq(r.export_count, 0 as usize);
        expect_eq(std.mem.eql(u8, imps[0].specifier(code), "./real.js"), true);
    });

    test("6. @cap/es-module-lexer — isFacade detection", () => {
        expect_eq(isFacade("import './init.js';\nexport { a, b as c } from './m.js';\nexport * from './n.js';\n"), true);
        expect_eq(isFacade("export { a } from './m.js';\nconsole.log(1);\n"), false);
        expect_eq(isFacade("const x = 1; export { x };"), false);
    });
}
