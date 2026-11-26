import { test, expect_eq } from "cap:test";
import {
    parseJsDoc,
} from "../../packages/@cap/doctrine/src/index.cp";

fn strEq(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn main() {
    test("1. @cap/doctrine — Top description & @param with required/optional names", () => {
        const src = "/**\n * Compute user greeting.\n * @param {string} name - Full name\n * @param {number} [age=18] - Optional age\n */";
        const doc = parseJsDoc(src);

        expect_eq(strEq(doc.description, "Compute user greeting."), true);
        expect_eq(doc.tagCount(), 2);

        if (t0 := doc.getTag(0)) {
            expect_eq(strEq(t0.title, "param"), true);
            expect_eq(strEq(t0.type_expr, "string"), true);
            expect_eq(strEq(t0.name, "name"), true);
            expect_eq(strEq(t0.description, "Full name"), true);
            expect_eq(t0.optional, false);
        } else {
            expect_eq(true, false);
        }

        if (t1 := doc.getTag(1)) {
            expect_eq(strEq(t1.title, "param"), true);
            expect_eq(strEq(t1.type_expr, "number"), true);
            expect_eq(strEq(t1.name, "age"), true);
            expect_eq(strEq(t1.description, "Optional age"), true);
            expect_eq(t1.optional, true);
        } else {
            expect_eq(true, false);
        }
    });

    test("2. @cap/doctrine — @returns, @throws & nested generic {Type} expressions", () => {
        const src = "/**\n * Fetch records.\n * @returns {Promise<{id: number}>} Loaded record\n * @throws {RangeError} When id is negative\n */";
        const doc = parseJsDoc(src);

        expect_eq(doc.tagCount(), 2);
        if (ret := doc.findTag("returns")) {
            expect_eq(strEq(ret.type_expr, "Promise<{id: number}>"), true);
            expect_eq(strEq(ret.description, "Loaded record"), true);
        } else {
            expect_eq(true, false);
        }

        if (thr := doc.findTag("throws")) {
            expect_eq(strEq(thr.type_expr, "RangeError"), true);
            expect_eq(strEq(thr.description, "When id is negative"), true);
        } else {
            expect_eq(true, false);
        }
    });

    test("3. @cap/doctrine — @type and @template tags", () => {
        const src = "/**\n * @template T - Element type\n * @type {Array<T>}\n */";
        const doc = parseJsDoc(src);

        expect_eq(doc.tagCount(), 2);
        if (tpl := doc.findTag("template")) {
            expect_eq(strEq(tpl.name, "T"), true);
            expect_eq(strEq(tpl.description, "Element type"), true);
        } else {
            expect_eq(true, false);
        }

        if (ty := doc.findTag("type")) {
            expect_eq(strEq(ty.type_expr, "Array<T>"), true);
        } else {
            expect_eq(true, false);
        }
    });

    test("4. @cap/doctrine — @deprecated detection via isDeprecated()", () => {
        const dep_doc = parseJsDoc("/**\n * Legacy helper.\n * @deprecated Use newHelper() instead\n */");
        expect_eq(dep_doc.isDeprecated(), true);
        if (dtag := dep_doc.findTag("deprecated")) {
            expect_eq(strEq(dtag.description, "Use newHelper() instead"), true);
        } else {
            expect_eq(true, false);
        }

        const fresh_doc = parseJsDoc("/** Active helper. */");
        expect_eq(fresh_doc.isDeprecated(), false);
        expect_eq(strEq(fresh_doc.description, "Active helper."), true);
    });

    test("5. @cap/doctrine — Closure optional type syntax {string=} & single-line JSDoc", () => {
        const doc = parseJsDoc("/** @param {string=} prefix Prefix string */");
        expect_eq(doc.tagCount(), 1);
        if (p := doc.getTag(0)) {
            expect_eq(strEq(p.name, "prefix"), true);
            expect_eq(strEq(p.type_expr, "string="), true);
            expect_eq(p.optional, true);
            expect_eq(strEq(p.description, "Prefix string"), true);
        } else {
            expect_eq(true, false);
        }
    });

    test("6. @cap/doctrine — Empty comments, out-of-bounds getTag & missing findTag", () => {
        const empty = parseJsDoc("/** */");
        expect_eq(empty.tagCount(), 0);
        expect_eq(strEq(empty.description, ""), true);
        expect_eq(empty.isDeprecated(), false);

        const missing_idx = empty.getTag(0);
        expect_eq(missing_idx == null, true);

        const missing_tag = empty.findTag("returns");
        expect_eq(missing_tag == null, true);
    });
}
