import { test, expect_eq } from "cap:test";
import {
    parseCjs,
} from "../../packages/cjs-module-lexer/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn hasName(arr: []const string, count: usize, target: string): bool {
    let i: usize = 0;
    while (i < count) : (i += 1) {
        if (strEq(arr[i], target)) return true;
    }
    return false;
}

fn main() {
    test("1. @cap/cjs-module-lexer — exports.foo, module.exports.bar & bracket exports['baz']", () => {
        const src = "exports.foo = 1;\nmodule.exports.bar = function() {};\nexports['baz'] = true;\nmodule.exports[\"qux\"] = 42;";
        const res = parseCjs(src);
        const exps: [32]string = res.exports;

        expect_eq(res.export_count, 4);
        expect_eq(hasName(exps[0..32], res.export_count, "foo"), true);
        expect_eq(hasName(exps[0..32], res.export_count, "bar"), true);
        expect_eq(hasName(exps[0..32], res.export_count, "baz"), true);
        expect_eq(hasName(exps[0..32], res.export_count, "qux"), true);
    });

    test("2. @cap/cjs-module-lexer — Object.defineProperty(exports, 'qux', ...)", () => {
        const src = "Object.defineProperty(exports, 'alpha', { value: 1 });\nObject.defineProperty(module.exports, \"beta\", { get: () => 2 });";
        const res = parseCjs(src);
        const exps: [32]string = res.exports;

        expect_eq(res.export_count, 2);
        expect_eq(hasName(exps[0..32], res.export_count, "alpha"), true);
        expect_eq(hasName(exps[0..32], res.export_count, "beta"), true);
    });

    test("3. @cap/cjs-module-lexer — module.exports = { a, b: 1, 'c': fn() {} } object literal", () => {
        const src = "const a = 10;\nmodule.exports = { a, b: { nested: 1 }, 'c': [1, 2], run() { return 1; } };";
        const res = parseCjs(src);
        const exps: [32]string = res.exports;

        expect_eq(res.export_count, 4);
        expect_eq(hasName(exps[0..32], res.export_count, "a"), true);
        expect_eq(hasName(exps[0..32], res.export_count, "b"), true);
        expect_eq(hasName(exps[0..32], res.export_count, "c"), true);
        expect_eq(hasName(exps[0..32], res.export_count, "run"), true);
    });

    test("4. @cap/cjs-module-lexer — Re-exports via module.exports = require() & __exportStar()", () => {
        const src = "module.exports = require('./core');\n__exportStar(require(\"./utils\"), exports);";
        const res = parseCjs(src);
        const rexps: [16]string = res.reexports;

        expect_eq(res.reexport_count, 2);
        expect_eq(hasName(rexps[0..16], res.reexport_count, "./core"), true);
        expect_eq(hasName(rexps[0..16], res.reexport_count, "./utils"), true);
    });

    test("5. @cap/cjs-module-lexer — Ignores comments and string literals", () => {
        const src = "// exports.commented = 1;\n/* module.exports.blocked = 2; */\nconst s = \"exports.inString = 3;\";\nexports.real = 4;";
        const res = parseCjs(src);
        const exps: [32]string = res.exports;

        expect_eq(res.export_count, 1);
        expect_eq(strEq(exps[0], "real"), true);
    });

    test("6. @cap/cjs-module-lexer — Deduplicates repeated exports and re-exports", () => {
        const src = "exports.dup = 1;\nexports.dup = 2;\nmodule.exports.dup = 3;\n__exportStar(require('./sub'), exports);\n__exportStar(require('./sub'), exports);";
        const res = parseCjs(src);
        const exps: [32]string = res.exports;
        const rexps: [16]string = res.reexports;

        expect_eq(res.export_count, 1);
        expect_eq(strEq(exps[0], "dup"), true);
        expect_eq(res.reexport_count, 1);
        expect_eq(strEq(rexps[0], "./sub"), true);
    });
}
